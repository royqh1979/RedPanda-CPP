# Parser 状态、并发与生命周期设计

## 1. 状态分类

### Configuration

Configuration 是调用方提供的输入和项目上下文，不是解析产物，包括：

- project files；
- 编译器和项目 include paths；
- hard defines；
- 编译器类型、C/C++ 语言版本；
- 扫描选项及 include 最大嵌套深度；
- 文件读取、失效通知和停止请求回调。

不引入单独的 Configuration 类；这些状态由 CppParser/CppPreprocessor 的现有配置成员承担。

### Session

Session 是一次 parse/preprocess 调用私有的数据，包括 include 栈、当前文件、输入输出缓冲区、分支栈、processed 集合、有序跨边界前缀、文件内容缓存、当前 defines 和扫描进度。

CppParser 和 CppPreprocessor 不持有 Session 成员。内部执行链在栈上创建 Session，通过 `preprocess(session, fileName)` 等私有接口传递；preprocess 开始时值初始化，因此可以重复使用该栈对象而不需要显式清理。result、defines 和 boundary 输出在调用结束后仍可读取，生命周期到调用方作用域结束。Session 和 Registry 不提供公开访问入口；Preprocessor 组件测试通过 friend 使用内部接口，不为此增加生产公开 getter。

锁、停止标志、冻结计数、pending parse command 和 serial id 属于对象控制状态，不属于 Session。测试诊断缓冲和匿名类型 ID 生成器也不属于 Session。

### Registry

Preprocessor Registry 只保存跨 preprocess 轮次保留的派生结果：

- `fileInfos`；
- `replayRecords`；
- `pendingInvalidations`。

StatementModel 是 Parser 层的 Statement 所有权中心，与 Preprocessor Registry 并列，不并入 Preprocessor。FileInfo 保存 include/branch 信息和 Statement 弱引用，StatementModel 拥有实际 Statement。

## 2. 清理接口与配置重建

命名约定：

- `clearXxx()`：丢弃明确范围的分析数据；
- `resetForReuse()`：完整清理分析结果、项目上下文和编译器输入，以便重新配置；
- `reconfigureXxx()`：在完整 reset 后应用新配置；
- `invalidateXxx()`：使指定已有结果失效，名称说明是否级联；
- `initializeXxx()`：初始化全局或对象能力，不用于日常清理。

核心契约：

| 接口 | 契约 |
| --- | --- |
| `CppTokenizer::resetTokenizationState()` | 清输入、tokens、游标、配对和文件边界，保留停止回调 |
| `CppPreprocessor::clearAnalysisResults()` | 只清 Registry，保留 Configuration，不影响调用方 Session |
| `CppPreprocessor::resetForReuse()` | 清 Registry、project files、include paths 和 hard defines，重置 GCC/C++17；保留扫描选项、最大嵌套深度和回调 |
| `CppParser::clearAnalysisResults()` | 清 StatementModel 和 Preprocessor Registry，再恢复 hard-define statements |
| `CppParser::resetForReuse()` | 停止并等待当前解析，再清分析结果、project files 和 Preprocessor 配置 |

`clearAnalysisResults()` 的后置条件不是所有容器都为空：CppParser 会恢复 hard-define statements。栈上 Session 不由这些清理接口处理。

统一配置流程为：

```text
reconfigureCppParser
  → resetForReuse
  → 设置语言和启用状态
  → 添加新编译器 include paths 和 hard defines
  → parseHardDefines
  → 连接所需信号
```

`Project::reconfigureParser()` 随后应用 project files 和 project include paths。通用配置入口不了解 Project；项目文件配置不放入 Registry。

`reparseActiveProject()` 固定执行 Project 配置重建并重解析当前 Editor，不遍历所有项目 TU。`resetParserProjectFiles()` 和独立的 include 清理接口不属于该流程。

## 3. Parser 所有权

| 持有者 | 引用方式 | 生命周期约定 |
| --- | --- | --- |
| EditorManager 的共享 Parser 表 | `weak_ptr` | 缓存索引不延长 Parser 生命周期 |
| Editor | `shared_ptr` | 非项目共享 Parser 的主要长期所有者 |
| Project | `shared_ptr` | 项目 Parser 的独立长期所有者 |
| ClassBrowserModel | `weak_ptr` | 操作入口 lock 一次，并在执行链内使用该 shared_ptr |
| CodeCompletionPopup / HeaderCompletionPopup | `shared_ptr`，hide 时清空 | 属于 Editor，随 Editor 释放 |
| Compiler | 一次性任务对象持有 `shared_ptr` | 构造器注入，任务对象销毁时释放 |
| CppFileParserThread | `shared_ptr` | 保证后台执行期间 Parser 存活 |
| NewClassDialog / Refactor 等局部操作 | 短生命周期 `shared_ptr` | 只在操作期间持有 |

生命周期不变量是：空闲的非项目共享 Parser 只能由 Editor 长期持有；视图不得持有 strong ref；后台任务只能在执行期间持有 strong ref。Project 和后台任务的合法强引用不违反该原则。

ClassBrowserModel 的 `parser()` 返回锁定后的 shared_ptr 值。绘制或递归子节点不得反复 lock；freeze/unfreeze 必须作用于同一实例。

Compiler 的 Parser 是准备 autolink 参数的必需输入。Compiler 任务通过既有 `finished → deleteLater` 释放，不需要额外 setter 或在 `run()` 末尾手动 reset。

## 4. 并发边界

读取 StatementModel、ParsedFileInfo 或其它共享分析结果的 public 查询接口，在入口加锁并检查 `mParsing`；解析中按类型返回空结果。private `do*` 实现不加锁，内部调用只走 `do*`，避免重复加锁。

只使用调用方传入 Statement、且不读取 Parser 共享状态的接口不机械加锁，例如 `prettyPrintStatement()` 和 `getFunctionParameterNames()`。

实际解析期间不能全程持锁，否则 `resetForReuse()` 无法及时设置停止状态，public 查询也不能快速返回。解析停止和清理必须在发布结果前完成。

以下事项不能用一次入口加锁代替分析：

- `statementList()` 等活动引用的合法使用时限；
- include path、project files 等配置在解析中的读写策略；
- `enabled()`、`setEnabled()` 与 reset 的交错；
- pending command 在工作线程命令边界的锁；
- `mParsing` 的每次读取是否有必要、是否持锁以及对应的同步不变量。

`statementList()` 的入口锁只能保护返回动作，不能保护调用方随后对引用的使用。类似 const 引用接口必须明确调用方线程约定、快照要求或使用时限。

## 5. 停止、隐藏与批量操作

CppParser 有两个独立原子标志：

- 进程级应用退出标志：由 `prepareForApplicationExit()` 设置，同时控制解析停止和退出析构的资源策略；
- 实例级停止标志：只要求当前 Parser 尽快结束正在执行的解析。

预处理、分词、声明解析和多文件扫描通过同一停止谓词读取这两个标志。任一为真即停止后续工作。

仅实例级停止时，本次结果不完整，Parser 在 `mParsing=false` 和 `parseFinished` 之前清空分析结果并保留配置。应用退出标志为真时不清理半成品，最终按退出析构策略处理。

Editor 隐藏时调用实例级停止接口，不等待后台线程结束；显示后调用 `reparse()`，由 Parser 判断是否能跳过。若隐藏后很快重新显示，沿用 pending command 处理显示请求。

批量打开和关闭复用 EditorManager 的嵌套 `beginUpdate()` / `endUpdate()`：

- 批量打开期间，中间 Editor 的 show 不发起解析，结束时只解析最终 active Editor；
- 批量关闭内层只停止并失效被关闭文件，最外层结束后只解析仍存在的 active Editor；
- Close All 成功后没有 active Editor，不再解析；
- 应用退出、Close All 和项目关闭期间不启动新解析。

## 6. 保存与普通关闭

### 单个 Editor

`Editor::setCppParser(nullptr)` 在解除关联前调用旧 Parser 的 `invalidateFile()`。共享 Parser 仍可服务其它 Editor；非共享 Parser 随后自然析构。

### Close All

1. 按左右面板顺序逐一完成保存确认；
2. 任一取消或保存失败，立即停止，不关闭剩余 Editor；
3. 保存全部完成后逐一关闭 Editor；
4. 不调用 `reconfigureCppParser()`，不额外遍历共享 Parser 表执行 reset。

最后一个有效所有者释放后，Parser 自然析构。后台任务仍持有 Parser 时，析构延迟到任务结束，这是安全要求。

Close All 不等于 closeProject。项目 Parser 由 Project 持有，项目关闭由 `closeProject()` 负责。

保存确认函数由单个关闭、Close All 和应用退出复用：Yes 保存失败返回 false；No 放弃修改并清除 modified；Cancel 返回 false 并保留 Editor。

## 7. 应用退出

CppParser 使用两个 `std::unique_ptr` 持有 StatementModel 和 CppPreprocessor。仅跳过 reset 不能避免成员容器自动析构，因此退出优化必须改变资源所有权。

普通析构执行 reset 并正常释放资源。

应用退出析构：

1. 不调用 reset；
2. 释放两个 unique_ptr 的所有权但不 delete 资源；
3. 由操作系统在进程结束时回收大型分析数据。

MainWindow 的退出顺序：

1. 完成所有 Editor 的保存确认；取消或失败时 ignore，不进入退出状态；
2. 调用一次 `CppParser::prepareForApplicationExit()`；
3. 保存 last-opens、项目布局、书签、调试信息和应用配置；此后不再提供取消入口；
4. 关闭项目和 Editor，不执行额外 Parser 清理；
5. 停止编译、运行和调试活动；
6. 沿用现有解析等待和完成信号机制；
7. 正常接受关闭并析构 Qt 与应用对象。

进程级退出状态覆盖与 Editor 解除关联、但仍被后台任务或局部操作持有的 Parser。生产流程不提供恢复操作；测试恢复接口由条件编译隔离。

不使用 `_Exit`，不把资源放弃用于 Close All、closeProject、切换编译器或普通 Editor 析构。

## 8. 验证契约

- Tokenizer 连续使用不残留前次输入、配对或文件边界。
- 栈上 Session 重复 preprocess 自动刷新，Registry 与 Configuration 保持。
- clear/reset 后的 Registry、配置和 hard-define Statement 符合各自契约。
- public Statement 查询在解析中按类型返回空结果，内部 `do*` 不重复加锁。
- 实例级停止在 parseFinished 前清除不完整结果；应用退出不清理半成品。
- 普通析构使外部 statement/fileInfo weak_ptr 失效；退出析构时有意的资源保留可由 weak_ptr 验证。
- ClassBrowser 的 weak_ptr 不阻止 Parser 析构，过期后安全清空或返回。
- Close All 在取消或保存失败时不关闭剩余 Editor，不执行显式 reset。
- 后台引用未释放时 Parser 不提前析构。
- 所有保存完成后才保存配置并进入不可取消阶段。
