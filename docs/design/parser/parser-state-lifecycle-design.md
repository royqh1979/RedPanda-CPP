# Parser 状态生命周期与清理接口设计

## 1. 目标与边界

本设计统一 CppParser、CppTokenizer 和 CppPreprocessor 的状态分类及清理接口命名，使调用方能直接判断操作影响的是单次会话、跨轮分析结果还是配置。

接口重命名不改变现有清理范围。后续配置流程整合必须保持“完整 reset 负责清理，reconfigure 负责应用新配置”的单向关系，不以保留配置为由增加另一套 reset 语义。相关类均为应用内部 API，直接迁移调用点，不保留旧名兼容包装。

## 2. 状态分类

### 2.1 Configuration

Configuration 是调用方提供的输入和项目上下文，不是解析产物，包括：

- project files；
- 编译器和项目 include paths；
- hard defines；
- 编译器类型和 C++ 语言版本；
- 扫描选项及 include 最大嵌套深度等运行选项。

Project files 决定文件的项目归属和 Statement 属性，属于 Configuration，不属于 Registry。CppPreprocessor 的 `mProjectFiles` 放在现有配置成员区域；本轮不引入新的 Configuration 类。

文件读取、失效通知和停止请求回调不因清理分析结果而改变。

### 2.2 Session

Session 是一次 parse/preprocess 调用私有的数据，包括 include 栈、当前文件、输入输出缓冲区、分支栈、本轮 processed 集合、有序跨边界前缀、文件内容缓存、当前 defines 和扫描进度。

CppParser 和 CppPreprocessor 不再持有 Session 成员。调用方在栈上创建 Session，并通过 `preprocess(session, fileName)` 等接口传递；preprocess 开始时对 Session 值初始化，因此同一栈对象可以重复使用而不需要显式清理。result、defines 和 boundary 信息在调用结束后仍由调用方读取，生命周期到调用方作用域结束。

锁、解析/预处理停止标志、冻结计数、待处理解析命令和串行版本属于对象控制状态，不属于 Session。跨调用测试诊断缓冲和匿名类型 ID 生成器也不属于单次 Session。

### 2.3 Registry

Registry 只保存跨 preprocess 轮次保留的派生分析结果：

- `fileInfos`；
- `replayRecords`；
- `pendingInvalidations`。

Project files 和路径配置不属于 Registry。StatementModel 是 Parser 层的语句注册中心，与 Preprocessor Registry 并列，不并入 Preprocessor。

CppParser 的 `mLanguage`、启用/共享策略和 project files 属于 Configuration；StatementModel 属于分析结果。`mUniqId` 用于分析结果生命周期内的匿名符号命名，不并入 Preprocessor Registry。

## 3. 命名规则

- `clearXxx()`：丢弃指定范围的分析数据；配置是否保留必须由名称和契约明确。
- `resetForReuse()`：完整清理分析结果、项目上下文和编译器输入配置，使对象可以在随后统一应用新配置；不表示恢复构造器状态，也不承担保留 Configuration 的职责。
- `reconfigureXxx()`：在完整 reset 后应用一套新配置，不重复执行 reset 已经完成的清理。
- `initializeXxx()`：初始化全局或对象能力，不用于日常清理。
- `invalidateXxx()`：使指定已有结果失效，名称必须说明是否级联。

不使用 `resetToInitialState()`：现有完整清理仍保留部分语言、启用状态、扫描选项、include 最大嵌套深度和回调，不等价于重新构造对象。

## 4. 核心接口

| 接口 | 契约 |
| --- | --- |
| `CppTokenizer::resetTokenizationState()` | 清输入、tokens、游标、配对和文件边界，保留停止回调 |
| `CppPreprocessor::preprocess(Session &, fileName)` | 值初始化调用方栈上 Session，完成本轮预处理并写入 result、defines 和 boundary 输出 |
| `CppPreprocessor::clearAnalysisResults()` | 只清 Registry，保留 Configuration，不影响调用方持有的 Session |
| `CppPreprocessor::resetForReuse()` | 清 Registry、project files、include paths 和 hard defines，重置 GCC/C++17，保留扫描选项、include 最大嵌套深度和回调 |
| `CppParser::clearAnalysisResults()` | 清 StatementModel 和 Preprocessor Registry，再恢复 hard-define statements |
| `CppParser::resetForReuse()` | 停止并等待当前解析，再清分析结果、project files 及上述 Preprocessor 配置 |

`clearAnalysisResults()` 的后置条件不是所有容器均为空：CppParser 会恢复配置生成的 hard-define statements，因此 StatementModel 可以非空。调用方 Session 不属于 Preprocessor 对象，不由这些清理接口处理。

内部组合关系：

```cpp
void CppPreprocessor::clearAnalysisResults()
{
    mRegistry.clearAnalysisResults();
}
```

`resetForReuse()` 复用 `clearAnalysisResults()`，再清 project files、include paths 和 hard defines，并重置 GCC/C++17；扫描选项、include 最大嵌套深度和回调保持。`Registry` 只提供分析结果清理，不再提供包含 project files 的笼统 `clear()`。

原“临时 Session 清理”层级已经取消：正常解析结束和 include 关系预扫描后，栈上 Session 由调用方作用域自动释放；重复调用时通过值初始化刷新。中止解析时清全部分析结果但保留配置，完整重置再清配置，不使用 `clear(ClearScope)` 或布尔参数。

## 5. 相关接口

- `reconfigureCppParser()` 在完整 reset 后设置编译器 include、defines、语言并连接信号；不再调用 `clearCompilerIncludePaths()` 重复清理已空容器。
- `initializeParserGlobals()` 初始化全局关键字和运算符等集合。
- CppParser 和 CppPreprocessor 的 `invalidateFileAndDependents()` 执行级联失效。
- CppParser 的 `invalidateSingleFile()` 只失效一个文件；测试入口为 `invalidateSingleFileForTest()`。
- `clearCompilerIncludePaths()` 在当前调用链中没有独立作用；删除 reconfigure 中的冗余调用，若没有其它调用方则一并删除该公开清理接口。
- `resetApplicationExitFlagForTest()` 只恢复测试中的应用退出标志。

以下保持原名：`clearEventCounts()`、`removeFileInfo()`、`removeReplayRecord()`。`StatementModel::clear()` 和 `CppScopes::clear()` 是单一容器操作，也保持 Qt 容器风格。编译器和项目 include/path 的独立公开清理接口已由统一 reset 取代。

## 6. 测试契约

相关操作必须保持以下契约，不得通过修改预期来接受行为变化：

1. Tokenizer 同一实例连续使用时，不残留前次输入、tokens、配对或文件边界。
2. 同一栈上 Session 重复 preprocess 时自动刷新 result、defines 和文件缓存，无需显式清理；Registry 与 Configuration 保持。
3. Preprocessor 分析结果清理后，fileInfo/replay 被删除，project files 和编译器配置保持。
4. Preprocessor 完整重置后，项目文件、include、hard defines 和 replay 被清空，并恢复 C++17/GCC。
5. Parser 分析结果清理后，普通文件 statements 消失，hard-define statements 恢复。
6. Parser 完整重置仍先停止并等待解析结束；现有关闭、退出和并发测试不变。
7. 级联失效继续覆盖声明、定义、static/extern、namespace 和多 TU 场景；单文件内部入口不被误当成级联入口。

## 7. 配置重建流程

完整 reset 与配置应用必须分开：`resetForReuse()` 负责把旧分析结果、project files、include paths 和 defines 清干净；`reconfigureCppParser()` 只负责应用新编译器配置。目标流程为：

```text
reconfigureCppParser
  → resetForReuse
  → 设置语言和启用状态
  → 添加新编译器 include paths 和 hard defines
  → parseHardDefines，建立新配置对应的 statements
  → 连接所需信号
```

`parseHardDefines()` 不能在添加新 defines 之前执行，也不应并入 reset。`clearCompilerIncludePaths()` 不得再夹在 reset 和添加新路径之间重复清理。

项目配置由 Project 组合，不放进不了解 Project 的通用 `reconfigureCppParser()`。当前所有 `scanActiveProject` 调用都显式传入 `true`；声明中的默认 `false` 及函数内单独更新项目配置的 `else` 分支没有实际调用场景。因此不引入 `updateParserConfiguration()` 或 `applyProjectContextToParser()`，只保留一个完整入口：

```cpp
void Project::reconfigureParser()
{
    ::reconfigureCppParser(mParser, mOptions.compilerSet);

    // 完整 reset 已清项目配置，直接应用当前项目输入
    for (const PProjectUnit &unit : mUnits) {
        if (isCFile(unit->fileName()) || isHFile(unit->fileName()))
            mParser->addProjectFile(unit->fileName(), true);
    }
    for (const QString &path : mOptions.includeDirs)
        mParser->addProjectIncludePath(path);
}
```

MainWindow 入口改名为 `reparseActiveProject()`，删除 bool 参数、默认参数和不可达 `else`，固定执行 `Project::reconfigureParser()` 后 reparse 当前 Editor。该名称表达“使当前项目的分析结果按最新代码和配置重建”的用户意图，不要求 eager 遍历所有项目 TU；当前 Parser 以活动 Editor 及其 include 上下文作为按需解析入口，即可提供后续代码补全、导航和结构浏览所需结果。

`resetParserProjectFiles()` 删除，不再提供脱离完整 reconfigure 的项目配置更新入口。

完整 reset 已清 project files 和项目 include paths，Project 不再重复调用 `clearProjectFiles()` / `clearProjectIncludePaths()`；若整合后没有其它调用方，这些公开清理接口也一并删除。

## 8. Registry 所有权与协作

当前不物理合并 StatementModel 和 Preprocessor Registry。FileInfo 同时保存 include/branch 信息和 Statement 弱引用，失效时由 CppParser 统一清理 StatementModel、FileInfo 和 replay；Preprocessor 扫描中检测到变化时仍通过失效通知让 Parser 参与。

物理合并不能消除失效回调、include 栈和 pending invalidation，反而会使 Preprocessor 独立测试及应用退出资源管理依赖 Parser 的所有权结构。因此以 `CppParser::clearAnalysisResults()` 和 `invalidateSingleFile()` 作为统一协调点即可。

若未来出现独立的所有权问题需要合并，统一 AnalysisRegistry 应由 CppParser 持有，Preprocessor 只持有非拥有引用；该工作必须另行设计，不与配置流程整理同时实施。

## 9. 重复 reconfigure 与实施顺序

项目打开/新建时，`Project::load/create()` 已执行一次 reconfigure，MainWindow 随后的 `scanActiveProject(true)` 又执行一次。后续应明确由 Project 或 MainWindow 其中一方负责完整配置，消除双重完整 reset；不能在未调整调用契约前直接删除其中一处。

实施顺序：

1. 整合单一的 `Project::reconfigureParser()`，将 MainWindow 入口改名为 `reparseActiveProject()`，删除原 bool/默认参数、不可达 `else` 和 `resetParserProjectFiles()`。
2. 删除 reconfigure 中冗余的 `clearCompilerIncludePaths()`；无独立调用方时删除该接口及不再使用的项目配置清理接口。
3. 补连续 reconfigure 和项目打开/新建测试，再消除打开/新建路径的双重配置。
4. 每阶段执行 Parser 专项测试及完整 Release、Debug 回归。

必须覆盖：所有原 `scanActiveProject(true)` 调用仍执行完整 reset、项目配置应用和 reparse；连续 reconfigure 不残留旧 include/defines；完整项目 reconfigure 后 project files 和项目 include paths 正确恢复；Statement 与 FileInfo/replay 的失效同步不变。

## 10. 范围审核

当前方案采用最简单的“全清 → 应用新配置”流程，不引入保留配置的第二种 reset 语义，不为不可达的 `scanActiveProject(false)` 预留独立配置接口，不引入 ClearScope 或新的 Configuration 类。Project 负责自己的项目上下文，CppParser 负责分析结果协调，Preprocessor 继续负责预处理和 replay。配置流程整合、重复 reset 移除和 Registry 所有权调整不得混成同一批行为修改。
