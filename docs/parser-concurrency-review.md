# CppParser 并发边界审查

本文记录 CppParser 并发约定和仍待逐项分析的问题。未列作结论的事项都不是已经确认需要修改的 bug，不能据此直接加锁或改变解析流程。

## Statement 查询接口约定

读取 StatementModel、ParsedFileInfo 或其他解析结果的 public 查询接口，在入口加锁并检查 `mParsing`。解析中返回空结果：指针返回 `nullptr`，容器返回空容器，枚举返回对应的空值。private `do*` 实现不加锁，内部调用只走 `do*`，避免内部反复加锁。

只使用调用方传入的 Statement、且不读取 CppParser 共享状态的接口不机械加锁。例如 `prettyPrintStatement()` 和 `getFunctionParameterNames()` 不属于上述共享状态查询。

`findStatement()`、`findStatementOf()` 与 `evalExpression()` 使用 public 包装和 private `doFind*` / `doEval*` 实现。namespace 和继承成员查询也按同样方式拆分，避免内部 public 调用重复加锁。尤其在 `mParsing == true` 的 Statement 构建阶段，public 查询会按约定返回空结果，内部复用查询逻辑时必须直接调用无锁的 `doFindXXX()`，不能经过 public 包装。

## 待分析：解析入口与结束状态

- `parseFile()` 在入口锁之外读取 `mEnabled`。需要确认该读取应移入现有入口锁，还是由调用方提供更强的线程约定。
- `parseFile()` 只在短临界区持锁，实际解析期间不持锁。不能为消除数据竞争简单改为全程持锁，否则 `resetParser()` 无法及时设置停止状态，public 查询也不能在解析中快速返回。操作级不变量是：`mParsing == false` 时不存在其他正在执行的分析操作，再次置为 true 前不应启动新分析。需要单独判断 bool 和计数的内存同步，不能把该问题误述为两个分析 writer 并发修改 StatementMap。
- `retrievePendingParseFileCommand()` 位于工作线程的命令边界，现有锁不能仅按 private 方法名称机械删除。

## 待分析：resetParser

`resetParser()` 需要等待当前解析停止，再清理状态。锁必须能介入正在执行的解析，不能变成全程互斥。finally 中恢复 `mParsing` 和停止标志时是否加锁、锁覆盖哪些字段，应单独审查；不能把锁随意扩大到整个等待和清理过程。

## 待分析：mParsing 标志的读写

逐项检查 parser 中所有读取和修改 `mParsing` 的位置。读取该标志必须持锁；同时需要分析各位置是否有必要读取、读取后应按什么场景判断，不能只审查 Statement 查询入口或批量照抄判断。修改标志时的锁范围应尽可能小，只覆盖必须同步的状态变化，避免扩大竞争、引入死锁或阻止 reset 及时介入。

## 待分析：配置及其他共享状态

以下接口需要逐项确认线程约定、是否允许在解析中调用，以及返回值是否要求空结果，不能只按 public/private 名称批量加锁：

- include path、project files、filesToScan 的读写；
- `enabled()` / `setEnabled()`，尤其禁用时会调用 `resetParser()` 的路径；
- `parsing()`、`hasReplayRecord()`、`isFileParsedInContext()`、`isProjectHeaderFile()` 等状态查询；
- language、sharedByFiles、serialId 和 onGetFileStream 的读写。

配置 setter 即使在入口加锁，也仍可能与未持锁的解析过程并发修改预处理器状态。需要明确解析中应拒绝、延迟还是先停止解析。

## 待分析：活动引用和快照

`statementList()` 返回活动 StatementModel 的 const 引用。入口加锁只能保护返回动作，不能保护调用方随后对引用的使用；调用方也可能在解析开始前取得引用，并在解析中继续使用。`mParsing` 检查本身不能解决该问题。

`includePaths()`、`projectIncludePaths()`、`projectFiles()`、`filesToScan()` 和 `serialId()` 等返回 const 引用的接口有类似限制。需要逐项决定维持调用方线程约定、改为快照返回值，还是限制解析中访问，不能用一次无意义的加锁掩盖生命周期问题。

## 待补测试

- public Statement 查询在 `mParsing == true` 时按类型返回空结果，且 public 与 `do*` 调用关系不导致重复加锁。
- 解析开始、结束、reset 和禁用 parser 的交错时序。
- 配置修改与解析并发时的既定策略。
- 对返回活动引用的接口，明确测试或文档化其合法使用时限。
