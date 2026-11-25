# Parser 按需重解析与多 TU 设计

## 1. 目标与边界

Editor 只提出“保证当前文件分析结果可用且未过期”的请求。是否跳过、失效和扫描由 Parser 在执行请求时统一判断，不由调用方选择 force/non-force，也不由项目身份选择另一套扫描算法。

正确性目标分两层：

1. 当前 root/context 及其 include 闭包中的语句、作用域、宏分支和表达式分析结果准确。
2. 项目 Refactor 能逐个解析所有 TU 并累积完整结果，但不要求 Parser 同时持有完整项目符号表。

Parser 可以保留多个 TU 的缓存。缓存保留范围是性能与生命周期问题，不是当前 TU 或 Refactor 正确性的前提。

## 2. 单一重解析入口

`Editor::reparse()` 是唯一入口。编辑、打开、显示、切换、显式 Reparse Code 和重构后的恢复都提交同一种请求。

Editor 不使用 ReplayRecord 或 include 覆盖情况提前拦截请求：ReplayRecord 不能识别 Document 版本变化，include 覆盖也不能判断当前生效的 file/context。

Parser 在请求真正执行时判断，而不是只依赖入队时的 UI 状态。排队请求沿用现有“保留最后一个 pending 命令”的规则。

### 跳过条件

只有同时满足以下条件才跳过：

- 请求的 file/context 与当前生效的 file/context 完全相同；
- 当前 root/context 的 FileInfo 存在；
- 当前 root/context 已记录的 include 闭包中，没有因失效而缺失的 FileInfo；
- include 闭包中的 Document 版本或磁盘 mtime/size 均未变化。

配置变化通过 reset 清空当前生效状态，因此不需要额外的 force 请求。切换 TU 时，即使所有文件内容都没变，也不能因目标 FileInfo 仍存在就直接跳过。

`hasChangedFiles()` 只负责文件变化检查；其中缺失 FileInfo 时的跳过行为不承担“分析结果完整性”语义。Parser 的 `analysisResultValid` 统一封装 FileInfo 完整性和文件变化检查。

内存内容提供者通过 `setOnGetFileStream(stream, version)` 同时配置读取和版本查询回调，不再存在单独配置版本或只配置读取的入口。读取回调同时返回内容与本次版本；版本回调用于在不读取全文的情况下检查变化。纯磁盘文件仍使用 mtime/size，可变化的内存内容必须提供真实递增版本。

## 3. 当前 TU 的扫描与旧 Statement 清理

### 原则

不为了防重复而清理其它 TU，也不要求所有 TU 的宏分支结果同时生效。只有已经缓存的本次 file/context 才能在 Parse 前提供旧 include 闭包；首次打开的 TU 不预扫描。

每次实际扫描按以下流程：

1. 若本次 file/context 已有旧 FileInfo，在删除它之前失效其自身及旧的边界外 include 闭包。这会同时回收本次编辑后不再使用的 include。
2. 对 context 执行 Preprocess；context 为空时以 file 为根。
3. file 与 context 不同且 file 未被 context 包含时，再对 file 执行 Preprocess。
4. 每次 Preprocess 都得到该根本轮真实的 include 闭包，并建立新的 FileInfo/ReplayRecord。
5. 在 collectPreprocessorStatements 和 StatementBuilder 之前，清理该根闭包中边界外文件的旧 Statement。
6. 使用本轮 Preprocess 结果建立 Statement 和逐文件索引。

首次打开的 TU 跳过第 1 步，直接由第 4、5 步处理可能来自其它旧缓存的共享文件；已经缓存的 TU 不需要额外预扫描或根文件排序。

### 清理内容

对本轮根文件及其 include 闭包中的边界外文件，清除旧的：

- declaration 和 definition occurrence；
- namespace occurrence；
- 局部 Block、局部变量、参数和 enum 值；
- 宏 Statement；
- 对应的 StatementModel 索引。

清理必须沿用现有单侧失效规则：declaration 失效不影响其它 declaration/definition，definition 失效不得由 declaration 回填，namespace 在最后一个 occurrence 消失后才删除。

本轮 Preprocess 刚建立的 FileInfo 和 ReplayRecord 必须保留。不能调用会同时删除它们的完整失效入口，否则 Builder 无法登记逐文件索引，系统头也无法 replay。

现有 `invalidateSingleFile()` 可拆成内部的两步：

```text
removeStatementsInFile()
removeFileInfoAndReplayRecord()
```

显式编辑失效仍执行两步；当前 TU 流重建只执行第一步。该拆分只复用现有清理规则，不增加第二套 Statement 身份或合并算法。

### 显式失效与关闭 Editor

关闭 Editor 时，在删除 FileInfo 前取得目标旧的 include 闭包，失效目标自身及其中的边界外文件。这里沿“被它包含”的方向清理，不主动失效“包含它”的 root。

例如关闭头 H 时，包含 H 的 root A 保留自己的 FileInfo；A 下次使用时由 `analysisResultValid` 发现 H 缺失并重建。即使 A 尚未重扫就关闭，A 的旧 FileInfo 仍在，因此仍能正常取得并清理 A 自己的旧 include 闭包。

Preprocessor 为 Replay 校验而进行的依赖者级联失效是另一条规则，不与 Editor 关闭时的文件回收方向混用。

不增加全局 orphan GC，也不按 Replay 根重新计算全项目可达性。

### 边界内文件

边界内外必须使用 Preprocessor 的同一判定：统一 includePaths 中的路径参与判断，projectFiles 中的文件仍保持边界外身份。Parser 不得另写近似版本，否则可能误清 replay 成功的非项目头 Statement。

有效 replay 的边界内系统头不清理、不重建。Replay 校验失败或文件变化时，仍由 Preprocessor 现有入口失效并重扫。

直接作为根打开的边界内文件也遵守 replay 条件；只有实际需要重扫时才走现有失效流程。

### 不再采用的旧 TU 清理方案

不保存“当前边界外文件快照”，也不在切换时清理旧 root 或旧 include 集合。

旧 TU 中与本次无关的 Statement 保持缓存。共享文件成为本次 TU 的一部分时，其旧 Statement 由上述本轮清理替换；旧 TU 再次激活时会重新执行同一流程，因此不依赖旧的共享文件 Statement 一直存在。

## 4. occurrence 判重的职责

现有 declaration 按 owner、实际文件和 SourceRange 判重的规则继续保留，作为合并过程中的防重复措施。

occurrence 判重不能替代本轮旧 Statement 清理：

- 同一位置在不同宏环境下可能具有不同属性；
- 不同条件分支可能生成不同名称的 Statement；
- 当前 `visibleStatement()` 只按实际文件和 include 闭包过滤，不能区分同一文件在不同 TU 中的宏分支。

因此本轮清理负责当前 TU 正确性，occurrence 判重只负责避免合并索引中的完全重复项。

## 5. 多 TU 缓存与查询

不同 TU 的根文件、namespace occurrences 和与当前 TU 无关的用户文件可以保留在 StatementModel 中。不得为了简化当前 TU 重建而强制只剩最新 TU。

当前文件相关的查询仍通过实际文件及 include 闭包判断可见性。已经不属于当前 TU 的旧缓存不能仅因仍存在于 StatementModel 就泄漏到带文件上下文的查询结果中。

无文件上下文的全局查询不承诺表示某一个 TU 的完整视图；项目语义查询由 Refactor 逐 TU 执行。

## 6. 项目 Refactor

Project 负责 TU 列表、项目文件边界、编译器配置和可修改范围。CppParser 不提供项目文件列表解析入口。

项目 Find Occurrences/Rename 按以下流程：

1. 在当前 Editor 的冻结快照中定位目标，并提取跨 Parse 可比较的 SymbolIdentity。
2. 检查目标定义和声明是否超出当前文件或项目允许边界；超出时直接拒绝 Rename。
3. 由 Project 枚举全部 TU。
4. 对每个 TU 调用同一个按需 Parse 接口。
5. 冻结该 TU 的当前结果，按 kind、scope、链接方式、签名、模板及关联符号关系重新识别等价目标。
6. 查找该 TU 的定义、声明和引用，立即累积到 SymbolReferenceResults。
7. 全部 TU 完成后，通过同一 Parse 入口恢复当前 Editor 的 file/context。

Refactor 的正确性依赖“每个 TU 使用前都完成当前 TU 重建”，不依赖旧 TU 的 Statement 是否仍在 Parser 中。

局部符号无需进入项目查找面板。static、匿名 namespace、局部 extern、重载、覆盖和模板特化继续按现有身份规则处理。

## 7. 项目配置与统一 include paths

删除的是 Parse 接口、ParseFileCommand 和线程中的 `inProject` 参数，不是项目配置本身。

Parser 只保留一个有序 includePaths 列表。Project 在 reconfigure 时将项目 include 目录并入该列表，并保持项目目录先于编译器目录的现有搜索优先级。不得在编译器目录之后简单追加而改变同名头的解析结果。

普通 quote include 仍先查当前文件目录，再查统一列表；angle include 直接查统一列表。`#include_next` 从当前文件目录在统一列表中的位置继续向后搜索，不再维护跨两个列表的拼接规则。

边界判断使用统一 includePaths，同时保留 projectFiles 例外：位于 include 路径中的项目文件仍是边界外文件。Statement 的 InSystemHeader 也必须服从该边界语义，不能只按路径把项目 root 标成系统头。

头部补全读取同一有序列表，不再通过两个列表推断项目/系统来源。

因此可以删除 addProjectIncludePath、projectIncludePaths/projectIncludePathList 及对应查询接口。保留：

- projectFiles 对边界分类的影响；
- Statement 的 InProject 属性及新建类候选；
- Project 对 Parser 生命周期、编译器配置和 Refactor 边界的管理。

旧项目分支中的依赖者排序和主动扫描不再用于 Parse。`invalidateFileAndDependents()` 仍按“文件及其依赖者”处理显式编辑失效。

## 8. KISS 与范围审核

本方案只增加当前 file/context 状态，用于跳过判断；不维护 TU 文件快照、TU 级别 StatementModel 或全项目符号表。

Statement 清理复用现有失效实现。唯一需要拆分的是 Statement 清理与 FileInfo/ReplayRecord 删除，避免在本轮 Preprocess 后删除刚建立的 Registry 数据。

不修改：

- StatementOccurrence 的结构；
- Statement 的跨 Parse 身份规则；
- Preprocessor 的 replay 条件和边界算法；
- Refactor 的结果结构与逐项替换流程；
- EditorManager 的打开/关闭批量策略。

## 9. 验收测试

### 单 TU 与缓存有效性

- 相同 file/context 且内容未变化时跳过，Statement 对象和扫描次数不变。
- 根文件或 include 的 mtime/size、Document 版本变化后重新扫描。
- 编辑后即使 ReplayRecord 仍存在，也必须进入 Parser 并按版本解析。
- declaration/definition 单侧失效后能够恢复，不残留或错误回填。
- 配置 reset 后无需 force 即完整重建。
- 有效系统头 replay 不重扫。
- 非项目文件位于项目提供的 include path 且 replay 成功时，其 Statement 不会因边界误判而被清理。
- quote/angle include 保持项目目录优先于编译器目录；同名头用于验证搜索顺序。
- `#include_next` 在项目目录和编译器目录之间仍只向后搜索。
- 头部补全读取统一列表且不遗漏项目目录。

### 多 TU

- 两个以上独立 TU 往返切换时，当前符号和宏分支始终正确。
- 首次打开的新 TU 与已缓存 TU 使用相同规则。
- 共享项目头中的局部变量、Block、enum 值和宏 Statement 不重复。
- 旧 root 的 FileInfo 已提前失效时，当前 TU 仍能正确重建。
- 不同 TU 的 namespace occurrences 可以共存。
- 与当前 TU 无关的旧缓存不被清空。
- 项目头首次带 context 打开时按 context 分析；未被 context 包含时才单独扫描。
- 编辑移除 include 后，不再被当前缓存使用的边界外文件及其 Statement 被回收，其它 TU 的共享引用不受影响。
- 关闭 root Editor 时清理其旧边界外闭包；关闭被包含文件后，相关 root 在下次使用时识别缺失并重建。
- 先关闭共享头、再关闭尚未重扫的 root 时，root 的 FileInfo 不会因前一次关闭而被主动删除，双方旧闭包都能正确回收。

### 项目 Refactor

- Find Occurrences 覆盖所有项目 TU，且不搜索未被任何 TU 包含的头文件。
- 同名不同作用域、static/extern、重载、覆盖和模板特化按语义匹配。
- Rename 的项目边界拒绝规则不变。
- open Editor 未保存内容、Undo/Redo 和 ContentChanged 校验不变。
- 完成后恢复当前 Editor 的分析结果。

## 10. 性能约束

- 相同当前 file/context 的重复请求不得进入 Preprocess。
- 有效系统头 replay 不得因当前 TU 重建而失效。
- 不增加为获得 include 闭包而进行的额外预扫描。
- Statement 清理仍沿用现有按文件失效算法；如发现多文件清理热点，应作为独立 profile/优化批次处理。

## 11. 复审问题与讨论状态

### 11.1 文件身份变化时的旧闭包清理（已确认）

关闭、另存为、重命名和项目移除都必须在移除或更换旧文件身份前，清理旧文件的边界外 include 闭包，不能只处理 Editor 关闭。

已确认需要覆盖：

- `Editor::saveAs()` 和 `Editor::rename()`；
- 项目单元移除和重命名；
- `Project::renameUnit()` 当前先 `removeProjectFile()`、后失效的顺序问题。

`removeProjectFile()` 自身会删除单文件结果，因此不能在它之后才尝试读取旧 FileInfo。

### 11.2 Replay 失效级联的方向与旧闭包生命周期（规则已澄清，生命周期待讨论）

Replay 失效不按普通 include 包含者笼统级联，而是按两类真实依赖计算：

- 内容依赖：可 replay 记录的 FileInfo include 闭包包含失效文件，说明其缓存子树不再可靠；
- 顺序依赖：记录的 boundaryPrefix 包含失效文件，说明其生效前的宏环境和有序前提不再可靠。

第二类可以没有 include 关系。现有 `calculateAffectedFiles()` 只检查 include 闭包，不能完整表达上述规则。本提案同时保留相关场景审核结论；方案实施后再同步核心设计文档。

仍需讨论的是 FileInfo 生命周期：如果内容级联删除了非当前 root A 的 FileInfo，而 A 的其它旧 include U 仍保留，A 尚未重扫就关闭时，如何避免 U 成为孤儿。

该问题只针对 Replay 自动级联，不改变“关闭 H 时只清理 H 自身及其被包含闭包，不主动失效包含 H 的 root”的已确认方向。
