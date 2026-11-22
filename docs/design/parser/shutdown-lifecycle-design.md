# Parser 退出与 Editor 关闭生命周期设计

## 目标

按所有权处理 Parser 生命周期，不在 Close All 中按 Editor 或按面板重复 reset，也不通过强制结束进程绕过析构。

三种操作必须分开：

1. 关闭单个 Editor：共享 Parser 仍服务其它 Editor，因此 invalidate 当前文件及其依赖。
2. Close All：先完成所有保存确认，再直接关闭 Editor；不预先 reset。最后一个有效所有者释放后，Parser 自然析构并执行普通 reset。
3. 退出应用：先保存全部内容和配置，然后正常析构 Qt 与应用对象；已标记退出的 Parser 在析构时放弃大型资源的所有权，由操作系统统一回收。

## 已确认的所有权现状

| 持有者 | 当前引用方式 | 设计结论 |
| --- | --- | --- |
| EditorManager 的共享 Parser 表 | `weak_ptr` | 保持；缓存索引不能延长 Parser 生命周期 |
| Editor | `shared_ptr` | 非项目共享 Parser 的主要长期所有者 |
| Project | `shared_ptr` | 项目 Parser 的独立长期所有者，不套用非项目共享 Parser 规则 |
| ClassBrowserModel | `weak_ptr` | 视图不延长 Parser 生命周期；操作入口 lock 后在执行链内共享 |
| CodeCompletionPopup | `shared_ptr`，hide 时清空 | 保持；属于 Editor，关闭时随 Editor 释放 |
| HeaderCompletionPopup | `shared_ptr`，hide 时清空 | 同上 |
| Compiler | 一次性任务对象持有 `shared_ptr` | 保持；构造器注入，`finished → deleteLater` 后随任务对象释放 |
| CppFileParserThread | `shared_ptr` | 保持；后台运行期间必须保证 Parser 和资源存活 |
| NewClassDialog / Refactor 等局部操作 | 局部或短生命周期 `shared_ptr` | 保持；只在操作期间持有 |

不能字面要求“任何时刻只有 Editor 持有 shared_ptr”：Project 和后台任务都有合法的强引用需求。真正的生命周期不变量是：

> 空闲的非项目共享 Parser 只能由 Editor 长期持有；视图不得持有 strong ref，后台任务只能在执行期间持有 strong ref。

## 普通关闭设计

### 关闭单个 Editor

`Editor::setCppParser(nullptr)` 保持现有行为：解除关联前调用旧 Parser 的 `invalidateFile()`。

这适用于共享 Parser 仍有其它 Editor 使用的情况。非共享 Parser 虽会随 Editor 析构，但保留同一路径不会改变正确性。

### Close All

顺序为：

1. 按左右面板顺序逐一完成保存确认；
2. 任一取消或保存失败，立即停止，不关闭剩余 Editor；
3. 保存全部完成后，逐一关闭 Editor；
4. 不调用 `resetCppParser()`，也不额外遍历共享 Parser 表执行 reset。

EditorManager 的 weak_ptr 不影响析构。ClassBrowser 改为 weak_ptr 后，最后一个 Editor 关闭时，如果没有后台任务或局部操作仍在使用 Parser，引用计数归零，Parser 立即执行普通析构和 reset。

若后台任务仍持有 Parser，则 Parser 延迟到任务结束时析构。这是必要的安全等待，不应通过提前 reset 或释放资源规避。

左右分栏使用不同 Parser 实例不再影响 Close All 算法：不需要发现、计数或逐个 reset 这些实例，所有权自然决定每个实例的析构时机。

Close All 不等于 closeProject。项目 Parser 由 Project 持有，项目未关闭时继续存在；项目关闭由 `closeProject()` 负责，不在 EditorManager 中重复处理。

## 保存确认

从 `closeEditor()` 提取统一的单 Editor 保存确认函数，供单个关闭、Close All 和应用退出复用：

- force 模式直接保存；
- Yes 保存失败时返回 false；
- No 表示放弃本次修改并清除 modified，避免批量流程第二阶段重复询问；
- Cancel 返回 false，保留 Editor 和修改状态。

批量保存函数按当前左右面板顺序处理所有 Editor。任何取消或保存失败立即停止并返回 false。

Close All 先完整执行批量保存，再进入关闭循环。这样 reset、invalidate 或对象析构都不可能发生在用户尚未完成保存选择之前。

## 应用退出设计

### 为什么不能只跳过 resetParser

CppParser 的 StatementModel 和 CppPreprocessor 原本是直接成员。即使析构函数不调用 `resetParser()`，直接成员仍会自动析构，statement map、fileInfo、replay 等容器仍会逐项释放。

因此，“析构时不清理大型资源”必须改变资源所有权，不能只增加一个跳过 reset 的布尔值。

### 资源持有方式

CppParser 使用两个 `std::unique_ptr` 分别持有 StatementModel 和 CppPreprocessor。正常运行时，接口、锁和线程模型不变，仅调整成员持有方式。

普通析构：

1. 保持现有 `resetParser()`；
2. 正常释放 StatementModel 和 CppPreprocessor。

应用退出析构：

1. 不调用 reset；
2. 释放两个 `unique_ptr` 的所有权但不 delete 资源对象；
3. 由操作系统在进程结束时回收语句、fileInfo、replay 和预处理缓存。

这是严格限定于进程退出的有意资源放弃，不得用于 Close All、closeProject、切换编译器或普通 Editor 析构。

### 解析停止与应用退出状态

CppParser 有两个独立的原子标志：

- 进程级应用退出标志：由 `prepareForApplicationExit()` 设置，同时控制解析停止和析构时的资源放弃策略；
- 实例级停止当前解析标志：由 `resetParser()` 设置，只要求当前 Parser 尽快结束正在进行的解析。

预处理、分词、声明解析和多文件扫描都通过同一停止谓词检查这两个标志；任一为真即停止后续工作，不再开始下一个文件或解析阶段。后台预处理和分词通过停止回调读取 Parser 状态，不反向依赖 CppParser，也不另设一套生命周期状态。

两种停止路径的资源语义不同：

- 仅实例级停止为真：本次结果不完整，不能保留或发布。Parser 在 `mParsing=false` 和 `parseFinished` 之前清空语句、fileInfo、replay 和临时解析结果；include 路径、hard defines、编译器类型、语言版本和项目文件清单等配置保持不变。随后 `resetParser()` 再执行其原有的完整重置。
- 应用退出标志为真：不清理半成品，也不继续后续解析；最终析构时直接放弃 StatementModel 和 CppPreprocessor 的所有权，由操作系统回收。
- 两个标志同时为真：按应用退出处理，不执行普通中止清理。

应用退出是进程级状态，不能只标记当前 Editor 和 Project 指向的 Parser。某个 Parser 可能已与 Editor 解除关联，却仍被 Compiler 或 CppFileParserThread 单独持有；逐实例枚举会漏掉这类对象。因此退出标志由所有实例共享，所有 Parser 的停止检查和析构都读取它。

生产流程只进入退出状态，不提供恢复操作；测试构建提供专用恢复接口，避免污染同一测试进程中的其它用例。该状态不涉及 Close All 或 closeProject，只有 MainWindow 在全部保存确认成功后才能设置。

### 退出顺序

MainWindow 首次 closeEvent：

1. 逐一完成所有 Editor 的保存确认；取消或失败时恢复退出状态并 ignore，不保存配置、不关闭 Editor；
2. 调用一次 `CppParser::prepareForApplicationExit()`；
3. 保存 last-opens、项目布局、书签、调试信息和应用配置；此后不再提供保存取消入口；
4. 关闭项目和 Editor，不执行额外的 Parser 清理；
5. 停止编译、运行和调试活动；
6. 若仍有解析，沿用现有 `mParsingCount` 等待和完成信号机制；
7. 正常接受关闭并析构对象。当前 Editor、Project、后台线程、Compiler 或局部操作持有的 Parser，无论何时最终析构，都遵守同一进程级退出策略。

`prepareForApplicationExit()` 只设置进程级标志，不直接访问或清空某个 Parser 的资源。正在运行的解析通过停止检查尽早退出，退出后的对象析构遵守同一资源放弃策略。

不使用 `_Exit`。Qt 对象、Editor、Project 和其它应用对象仍按正常所有权关系析构。

## Editor 隐藏与显示

Editor 隐藏时调用 Parser 的实例级停止接口，不等待后台线程结束；显示后由 EditorManager 的 show 处理入口调用 `reparseIfNeeded()`。实例级停止不会禁用 Parser，下一次实际开始解析时清除该停止标志。已有完整 replay 时无需重复解析；中止清理后 replay 不存在，因此一定会重新解析。

若隐藏后很快重新显示，旧解析尚未退出，则沿用现有 pending command：旧解析按停止语义收尾，随后处理显示时排队的新解析。共享 Parser 不增加可见 Editor 计数或任务所有权判断；任一关联 Editor 隐藏都停止当前任务，显示时重新建立当前文件所需的完整分析结果。

批量打开和关闭复用 EditorManager 现有的嵌套 `beginUpdate()` / `endUpdate()` 边界：

- 批量打开期间，中间 Editor 的 show 不发起解析；`MainWindow::openFiles()` 结束时只解析最终 active Editor。启动恢复、命令行多文件和拖拽多文件都经过该入口。
- 单个关闭完成后，只解析最终 active Editor。
- `closeOthers()` 和 `closeAll()` 建立外层批量边界，内层关闭只停止并 invalidate 被关闭文件，不响应中间 tab 的 show 发起解析；最外层结束后仅对仍存在的 active Editor 解析一次。Close All 成功后没有 active Editor，因此不再解析。
- 应用退出、Close All 和项目关闭沿用 MainWindow 现有状态保护，不在关闭过程中启动新解析。

## ClassBrowser 弱引用改造

ClassBrowserModel 保存 `std::weak_ptr<CppParser>`，`setParser()` 仍接收 shared_ptr 并建立 Qt 信号连接。

`parser()` 返回锁定后的 shared_ptr 值，不再返回内部 strong ref 的常量引用。每个需要访问 Parser 的操作只在入口 lock 一次，并在整个操作内使用该局部 shared_ptr：

- 避免同一操作中反复 lock；
- 保证 freeze/unfreeze 作用于同一实例；
- Parser 已析构时按无 Parser 处理，ClassBrowser 清空或不展开对应节点；
- 不在绘制或递归子节点函数中反复 lock。

## Compiler 构造器注入与任务生命周期

`mParserForFile` 是 CompilerManager 在启动任务前从 Editor 取得的输入，实际只用于准备 autolink 相关编译参数。Compiler 是 QThread，但每条 CompilerManager 创建路径都连接了 `finished → deleteLater`，任务对象不会被重复用于后续编译。

因此，成员 strong ref 是合理的一次性任务所有权：保证准备参数和执行期间 Parser 存活，任务对象销毁时自然释放，不需要在 `run()` 末尾手动 reset，也不需要改为 weak_ptr 或在各辅助函数间传递局部 Parser。

Parser 改为通过各 Compiler 构造器注入，删除单独的 `setParserForFile()` 和无调用方的 `parserForFile()` getter。项目 Compiler 不接收额外 Parser，沿用 Project 提供的上下文。

## 测试保护

### Parser

- 普通析构后，外部只保存的 statement/fileInfo weak_ptr 失效；
- 实例级停止在 `parseFinished` 前清除当前文件的语句和 fileInfo，同时保留并重建 hard define 语句；
- 应用退出在预处理期间中止后续分词和声明解析，不清理半成品；同时设置实例级停止时也遵守这一规则；
- `prepareForApplicationExit()` 本身不清空语句和文件信息；
- 设置进程级退出状态后析构，statement/fileInfo weak_ptr 仍有效，证明大型资源所有权确实被分离；
- 不与 Editor 或 Project 关联、仅由局部或后台任务持有的 Parser，最终析构也遵守退出状态；
- 每个退出测试立即通过测试专用接口恢复全局状态，且只构造极小数据，其有意保留的资源随测试进程结束回收。

### 所有权与关闭

- ClassBrowserModel 设置 Parser 后，外部释放最后一个 strong ref，weak_ptr 能正常过期，Model 不阻止析构；
- ClassBrowser 在 Parser 已过期时安全清空或返回，不访问空指针；
- Compiler 构造后持有传入 Parser；一次性任务对象释放后，外部 weak_ptr 自然过期，无需在 `run()` 末尾手动清理；
- 单个 Editor 解除共享 Parser 时仍 invalidate 当前文件及依赖；
- Close All 在取消或保存失败时不关闭剩余 Editor；
- Close All 不调用显式 reset；最后一个 Editor 和临时所有者释放后 Parser 自然析构；
- 后台引用未释放时 Parser 不提前析构，后台结束后正常析构；
- MainWindow 在所有保存完成后才保存配置和进入不可取消阶段。

UI 测试通过状态、引用计数和 weak_ptr 过期验证所有权，不以耗时或人工点击作为通过条件。

## 客观评审

### 采用自然析构，而不是 Close All 预 reset

优点：

- 符合现有 shared_ptr/weak_ptr 所有权，不需要维护 Parser 实例清单；
- 自动覆盖未分栏、左右分栏和未来新增面板；
- 避免每个 Editor 重复 reset，也避免“多个实例是否都要 reset”的伪问题；
- 后台任务仍安全持有 Parser，不会提前清理其工作数据。

代价与风险：

- 若遗漏一个长期 strong ref，Parser 会延迟析构；因此必须修正长期存在的 ClassBrowser，并用 weak_ptr 测试防止回归；一次性 Compiler 和后台线程只需要求任务对象按既有路径销毁；
- 后台任务存在时，资源清理延迟到任务结束，而不是 Close All 当场完成；这是安全要求，不应优化成提前析构。

结论：这是普通 Close All 最简且语义最可靠的方案。

### ClassBrowser 改 weak_ptr，而不是仅在关闭时手动清空

手动清空依赖每条 UI 路径都记得调用，未来新增持有者或关闭路径容易回归。weak_ptr 直接表达“视图不是所有者”，成本是各操作入口需要 lock 并处理过期，改动集中在 ClassBrowserModel，值得采用。

### Compiler 保持一次性任务成员 strong ref

编译参数生成会读取 Parser 的 include 信息，成员 strong ref 能保证任务执行期间输入稳定。所有 CompilerManager 创建路径都在 finished 后 deleteLater，故任务对象析构会自然释放 Parser，不存在长期残留。构造器注入比事后 setter 更完整地表达必需输入，也避免任务启动后替换 Parser。

### Parser 退出时放弃资源，而不是仅跳过 reset

只有这样才能避免成员容器自动析构。代价是 CppParser 内部成员访问需要改为指针形式，并产生限定于进程退出的有意泄漏。改动集中在 CppParser，但必须进行完整 Release/Debug 回归，不能把机械替换视为无风险。

### 采用进程级退出状态，而不是逐实例标记

逐实例标记依赖调用方找全所有 Parser；仅遍历 Editor 和 Project 会漏掉后台任务单独持有的对象。进程级状态与实际语义一致，也不需要维护全局实例注册表。

风险是退出状态会影响此后析构的所有 Parser，因此必须满足两个约束：只能在全部保存成功、不再允许取消之后设置；生产代码不提供恢复接口。测试恢复接口必须由条件编译隔离，并在每个测试中立即恢复。

### 不采用 `_Exit` 或全局“停止一切析构”

这些方案会绕过配置落盘后的其它清理、Qt 对象销毁和正常线程生命周期，影响范围不可控。仅让 Parser 在退出析构时放弃大型资源，影响范围最小。

## 范围限制

- 不改变解析、invalidate、freeze/unfreeze、表达式求值和编译参数生成算法；
- 不把 CppFileParserThread、Refactor 等必要的临时 strong ref 改成 weak_ptr；
- 不改变 Project 对项目 Parser 的所有权；
- 不要求 Close All 当场销毁仍被后台任务使用的 Parser；
- 不强制结束进程，不跳过 Qt 和应用对象析构；
- 不增加通用资源泄漏选项，退出放弃资源只能由 MainWindow 的应用退出流程触发。
