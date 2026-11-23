# Parser 状态生命周期与清理接口设计

## 1. 目标与边界

本设计统一 CppParser、CppTokenizer 和 CppPreprocessor 的状态分类及清理接口命名，使调用方能直接判断操作影响的是单次会话、跨轮分析结果还是配置。

本轮只进行字段归类、接口命名、空接口删除和契约测试，不调整现有清理范围，不解决配置重复保存、缓存策略或并发问题。相关类均为应用内部 API，直接迁移调用点，不保留旧名兼容包装。

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

Session 是一次 `preprocess()` 的工作状态，包括 include 栈、当前文件、输入输出缓冲区、分支栈、本轮 processed 集合、有序跨边界前缀和文件内容缓存。

当前 `Session` 同时保存 defines。`clearTransientSessionState()` 只清临时工作状态，保留当前 defines；不能用含义过宽的 `clearSession()` 暗示整个 Session 对象都被清空。是否应在其它清理操作中清空当前 defines 是独立的行为问题，不在本轮修改。

### 2.3 Registry

Registry 只保存跨 preprocess 轮次保留的派生分析结果：

- `fileInfos`；
- `replayRecords`；
- `pendingInvalidations`。

Project files 和路径配置不属于 Registry。StatementModel 是 Parser 层的语句注册中心，与 Preprocessor Registry 并列，不并入 Preprocessor。

## 3. 命名规则

- `clearXxx()`：丢弃指定范围的分析数据；配置是否保留必须由名称和契约明确。
- `resetForReuse()`：组合清理，使现有对象可以在后续重新配置和解析前回到可复用基线；不表示恢复构造器状态。
- `reconfigureXxx()`：执行必要清理后应用一套编译器配置。
- `initializeXxx()`：初始化全局或对象能力，不用于日常清理。
- `invalidateXxx()`：使指定已有结果失效，名称必须说明是否级联。

不使用 `resetToInitialState()`：现有完整清理仍保留部分语言、启用状态、扫描选项、include 最大嵌套深度和回调，不等价于重新构造对象。

## 4. 核心接口

| 接口 | 契约 |
| --- | --- |
| `CppTokenizer::resetTokenizationState()` | 清输入、tokens、游标、配对和文件边界，保留停止回调 |
| `CppPreprocessor::clearTransientSessionState()` | 清本轮临时 Session，保留 defines、Registry 和 Configuration |
| `CppPreprocessor::clearAnalysisResults()` | 清临时 Session 和 Registry，保留 defines 与 Configuration |
| `CppPreprocessor::resetForReuse()` | 清分析结果、当前 defines、project files、include paths 和 hard defines，重置 GCC/C++17，保留扫描选项、include 最大嵌套深度和回调 |
| `CppParser::clearAnalysisResults()` | 清 StatementModel 和 Preprocessor 分析结果，再恢复 hard-define statements |
| `CppParser::resetForReuse()` | 停止并等待当前解析，再清分析结果、project files 及上述 Preprocessor 配置 |

`clearAnalysisResults()` 的后置条件不是所有容器均为空：

- CppPreprocessor 保留当前 defines 和 Configuration；
- CppParser 恢复配置生成的 hard-define statements，因此 StatementModel 可以非空。

内部组合关系：

```cpp
void CppPreprocessor::clearAnalysisResults()
{
    clearTransientSessionState();
    mRegistry.clearAnalysisResults();
}
```

`resetForReuse()` 复用 `clearAnalysisResults()`，再清当前 defines、project files、include paths 和 hard defines，并重置 GCC/C++17；扫描选项、include 最大嵌套深度和回调保持。`Registry` 只提供分析结果清理，不再提供包含 project files 的笼统 `clear()`。

三个公共清理层级不能合并：Parser 在正常解析结束和 include 关系预扫描后需要只清临时 Session；中止解析时需要清全部分析结果但保留配置；完整重置还需要清 project files、include paths 和 hard defines，并重置编译器类型与 C++ 版本。不使用 `clear(ClearScope)` 或布尔参数掩盖这些差别。

## 5. 相关接口

- `reconfigureCppParser()` 清理后设置编译器 include、defines、语言并连接信号。
- `initializeParserGlobals()` 初始化全局关键字和运算符等集合。
- CppParser 和 CppPreprocessor 的 `invalidateFileAndDependents()` 执行级联失效。
- CppParser 的 `invalidateSingleFile()` 只失效一个文件；测试入口为 `invalidateSingleFileForTest()`。
- `clearCompilerIncludePaths()` 与项目 include paths 区分。
- `resetApplicationExitFlagForTest()` 只恢复测试中的应用退出标志。

以下保持原名：`clearProjectFiles()`、`clearProjectIncludePaths()`、`clearEventCounts()`、`removeFileInfo()`、`removeReplayRecord()`。`StatementModel::clear()` 和 `CppScopes::clear()` 是单一容器操作，也保持 Qt 容器风格。

## 6. 测试契约

改名前先补或确认以下测试，迁移后不得通过修改预期来接受行为变化：

1. Tokenizer 同一实例连续使用时，不残留前次输入、tokens、配对或文件边界。
2. 临时 Session 清理后，当前 defines、fileInfo/replay、project files、include paths、编译器类型和 C++ 版本保持。
3. Preprocessor 分析结果清理后，fileInfo/replay 被删除，project files 和编译器配置保持。
4. Preprocessor 完整重置后，项目文件、include、defines 和 replay 被清空，并恢复 C++17/GCC。
5. Parser 分析结果清理后，普通文件 statements 消失，hard-define statements 恢复。
6. Parser 完整重置仍先停止并等待解析结束；现有关闭、退出和并发测试不变。
7. 级联失效继续覆盖声明、定义、static/extern、namespace 和多 TU 场景；单文件内部入口不被误当成级联入口。

## 7. 实施与验证

1. 补契约测试并更新生命周期文档。
2. 移动 Preprocessor 的 project files 字段归属，不改读写行为。
3. 用 IDE Rename Symbol 迁移核心接口及全部生产、测试和 stub 调用点。
4. 迁移 reconfigure、全局初始化、失效和测试标志接口，删除空接口。
5. 同步 shutdown、concurrency、replay 文档中的名称。
6. 专项运行 Tokenizer、Preprocessor、Parser invalidation、shutdown 和 concurrency 相关测试。
7. 完整执行 Release、Debug 主程序与测试目标构建，并分别运行全部 CTest。

本轮不合并 CppParser 与 CppPreprocessor 的两份 project files，不改变清理 defines 的时机，不调整 invalidate 范围，不引入兼容包装或新的配置类。

## 8. 范围审核

该方案保留现有三个必要清理层级，只把重复的 Registry 清理实现内部化。字段移动限于 `projectFiles`，公共接口只表达调用方需要知道的生命周期，不暴露 `clearTransientSessionAndFileRegistry()` 之类实现细节。所有行为问题均留待独立设计和测试，避免命名重构与语义修改相互掩盖。
