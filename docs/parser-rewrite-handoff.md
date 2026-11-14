# Parser 重写工作交接

## 当前分支与工作区

- 分支：`parser-rewrite`
- 本批提交前基线：`ccc28f16 Fix standard header declarations and lookup`
- 当前修改尚未提交，包含 Parser、Editor、Widgets、测试及设计文档。
- 不擅自合并或 push。锁相关后续修改必须逐项讨论，不能批量调整。

## 本批已完成

- 继承成员按需枚举，不复制基类 children；补全和类浏览器共用成员列表，覆盖 public/protected/private 继承和 override。
- Constructor 查询同时保留表达式类型与实际 Statement：类型是 Class，匹配 Statement 是 Constructor。`resolveClassOrConstructorAt()` 按源码语境处理 goto/parser hint。
- 类名与构造函数的 goto declaration/definition、parser hint 和 rename 测试均已覆盖，包含声明与定义互相跳转。
- Statement 查询接口按 public 入口锁和 private `do*` 分工，并检查 `mParsing`。没有修改解析流程、配置项或 reset 的锁。
- 普通 `auto` 推导不扩展结构化绑定；`publicProperties` 和旧的绑定成员推导路径已移除，绑定名保持 `auto`。
- 所有 `CodeCompletionType` 均有 Widgets 测试，Normal 路径也覆盖宏。
- 用户宏和 hard defines 生成 Preprocessor Statement，并出现在 Macros 与 Normal 补全中。
- 用户标签生成 Label Statement；case/default 和三元表达式冒号不误判为标签。
- rename 私有实现直接操作当前 Editor，不再通过 MainWindow 二次查找或处理未打开文件。
- `test-widgets` 独立测试目标已接入 CTest，AGENTS.md 已同步。

设计说明位于：

- `docs/cpptokenizer-refactor.md`
- `docs/parser-concurrency-review.md`

Macro Statement 来自预处理输出 token，不是 `ParsedFileInfo::defines()`；后者没有行列位置。

## 验证结果

默认构建目录 `R:/build-redpanda-qt5-debug-preprocessor-rewrite-default` 的 `all-test-targets` 构建成功，完整回归全部通过：

- TestCppPreprocessor：66 passed
- TestCppPreprocessorInclude：67 passed
- TestCppTokenizer：85 passed
- TestPpTokenizer：16 passed
- TestPpExpander：24 passed
- TestCppParser：42 passed
- TestWidgets：19 passed
- TestEditor：60 passed
- QSynedit 全套测试通过

标准库构建目录 `R:/build-redpanda-qt5-debug-parser-stdlib`：

- `test_standard_library_string` 已通过。
- `test_standard_library_public_api_complete` 已通过，耗时约 445 秒；此前 300 秒超时只是运行时间较长，不是失败。

## Release 性能分析

现已按既定设计实现惰性解析：声明阶段只在 `TypeInfo::templateArgumentsText` 中保存模板实参原文；类型求值、模板绑定及 range/tuple 类型操作需要结构时才调用 `parseTemplateArguments()`。函数模板的 `Container<T>` 递归绑定也会先取得结构，新增 `front_value(std::vector<T>)` 测试防止该路径因惰性保存而失效。

**性能测量必须直接运行 Release 可执行文件，不能在 gdb 下计时。** Windows 下直接运行时，Qt 的 `qWarning()` 默认可能只发往调试器；设置 `QT_FORCE_STDERR_LOGGING=1` 后可在终端取得日志。此前 gdb 下得到的 12.7 秒 `<string>`、89.5 秒完整验收、63 秒 replay 和 21 秒销毁等数据均受调试器干扰，不能用于性能结论或优化前后比较。

`DECLARATION_PROFILE`、`MERGE_DECLARATION_PROFILE` 等详细 profile 默认关闭，只在显式设置 `REDPANDA_PARSER_PROFILE=1` 时启用。嵌套细项计时本身会显著增加开销，优化前后对比必须使用未启用详细 profile 的运行；细项计时使用 `QElapsedTimer::nsecsElapsed()` 累计，不能用逐次 `restart()` 的毫秒取整，否则大量微秒级操作会被错误记为 0。

Direct Release、设置 `QT_FORCE_STDERR_LOGGING=1`、未启用详细 profile 的当前有效实测结果：

- GCC 8.1 完整公开接口验收连续三次总计 1.066、0.946、0.938 秒；首次解析 936、820、808 毫秒。后两次 preprocess 为 332、329 毫秒，tokenize 为 95、91 毫秒，parseDeclarations 为 164、160 毫秒。首轮文件缓存较冷时 preprocess 为 450 毫秒，因此比较时应同时报告多次结果，不能只取首轮。
- GCC 15.3 完整公开接口验收连续三次总计 1.507、1.514、1.506 秒；首次解析 1335、1327、1321 毫秒，preprocess 约 590 毫秒，tokenize 约 130 毫秒，parseDeclarations 约 323 毫秒，buildStatements 约 56 毫秒。
- 本轮优化前，GCC 8.1 完整验收约 1.420 秒，GCC 15.3 约 2.110 秒；tokenize 分别约 514 和 642 毫秒。当前主要热点已转为 preprocess 和 parseDeclarations。
- Tokenizer 原先在每个非空白 token 上无条件检查所有字符串字面量前缀，并逐个扫描运算符表。`startsWith()` 最初调用约 1900 万次；运算符改为按首字符分派并保持最长匹配后约 1050 万次；字面量、attribute 和注释检测增加首字符守卫后降至约 12 万次。实际字面量只有 1791 个，这是 tokenize 降至约 130 毫秒的主要原因。详细插桩曾把运算符优化单独显示为约 250 毫秒收益，但未插桩结果没有同等幅度提升，说明嵌套计时开销会扭曲细项结论。
- `TOKENIZER_PROFILE`、`TOKENIZER_STARTS_WITH` 和 `PREPROCESSOR_PROFILE` 与既有详细计时一样，仅在 `REDPANDA_PARSER_PROFILE=1` 时输出。GCC 15.3 当前插桩数据中，`openInclude()` 约 168 毫秒，`expandLineWithEngine()` 约 126 毫秒，`evaluateIf()` 约 60 毫秒，注释清理约 26 毫秒；这些是包含式耗时，不能直接相加。
- `userMacroSnapshot()` 仅在边界入口计算。A/B 对比中，所有文件都计算快照时 preprocess 为 1118 毫秒，仅边界入口计算时为 656 毫秒，约降低 41%。非边界文件不生成 replay 条件，因此不需要快照。标准库 include 路径必须规范化，GCC 输出中的 `bin/../` 等片段会导致 replay 缓存键不一致；规范化后 scan=1、replay=1、invalidate=1，Statement 数保持一致。
- 优化前完整验收约为 5.8 秒；namespace 索引后约为 3.572 秒，currentText 改为只读引用后又降至约 3.1 秒。模板原文改为按 line/ch 直接截取预处理输出后约为 1.95 秒；限定名在格式一致时也直接截取后，GCC 8.1 当时约为 1.39 秒。
- `mergeDeclaration()` 从优化前约 2.1 秒降为约 56 毫秒，不再是热点。名称查找和名称收集也不是热点。非 auto 初始化及被丢弃的表达式只跳过 token，不再拼接临时文本；该调整没有明显独立提速。字符串 reserve 预分配没有收益，已撤回。
- 高频 cursor 操作内联没有稳定收益，已撤回。模板原文改存 QStringList 后稳定慢约 20%，已撤回。完整 TokenList 还涉及 matchIndex 重定位、cursor 接口和更高常驻内存，未采用。
- 同一 Direct Release 环境下，主干 GCC 8.1 完整公开接口验收为 888 毫秒，当前重写实现已与主干接近；GCC 15.3 仍有差距。后续优先分析 preprocess 和 parseDeclarations，不再围绕 merge、模板拼接或 tokenize 前缀检查优化。
- 惰性解析之前没有同条件的 Direct Release 数据，因此不能用旧的 gdb 数字声称本次优化的实际加速比例。

最终实现使用 namespace 专用的 fullName 多值 weak 索引：每个 namespace 登记到自身 fullName，inline/匿名 namespace 同时登记到直接父 namespace 键下；同名组整体更新，以支持 late inline 和删除后取消透明。merge 对局部声明只查当前词法作用域，对非局部声明按最长 namespace 前缀取得全部节点，再沿 children 枚举全部重载；没有全表扫描或失败兜底。CppTypeResolver 的透明分支也统一使用该索引。`test_overloaded_declaration_definition_merge`、`test_local_declarations_in_disjoint_blocks_remain_distinct` 和 `test_namespace_index_inline_filter_and_invalidation` 覆盖关键场景，既有 late-inline 测试继续验证删除后的语义。

用户反馈的 `std::` 补全缺少 `string` / `basic_string` 已定位并修复：Popup 的 namespace 成员收集原先调用 `findNamespace(name)`，没有纳入 inline 节点，现统一改为 `findNamespace(name, true)`。新增 `test_code_completion_inline_namespaces` 验证 `std::__cxx11` 中的 `basic_string` 和别名 `string` 均进入补全列表；测试同时保证 ownerExpression=`std`、memberOperator=`::`、memberExpression 为空，光标位于声明之后。`test_code_completion_standard_library_string` 进一步使用 GCC 8.1 的真实 `<string>` 头文件验证通过；标准库 include 目录由 `g++ -xc++ -E -v` 获取，与 IDE 的 resetCppParser 配置方式一致。

## 后续顺序

1. 继续分析 Static Release 完整公开接口验收的剩余性能回退，优先分解 preprocess 的 include 打开、宏展开和条件求值，其次分析 parseDeclarations；tokenize、merge 和模板拼接已不是当前热点。
2. 逐项审查 `docs/parser-concurrency-review.md`：`mParsing` 读取必须持锁，但是否需要读取及如何判断逐场景分析；修改标志的临界区尽量小。
3. 单独分析活动引用接口（尤其 `statementList()`）的合法使用时限，不能用一次入口加锁掩盖引用生命周期问题。
4. 评估普通嵌套 Block 的既有问题：`void f() { int local; { int inner; } }` 中局部声明后的 `{` 可能被当作前一声明 body。不要与其他修复混合。
5. 继续模板、表达式和标准库 container/iterator/algorithm/range 的实际验收；CTAD 和 deduction guides 只按文档中的后续可选设计评估，不提前实现。

## 构建与测试环境

- 编译器：`D:/qt/mingw64/bin`
- Qt：`D:/qt/qt5-debug-64/bin`
- 默认构建目录：`R:/build-redpanda-qt5-debug-preprocessor-rewrite-default`
- 标准库构建目录：`R:/build-redpanda-qt5-debug-parser-stdlib`
- 标准库测试开关：`REDPANDA_PARSER_STDLIB_TEST=ON`
- 标准库接口验收以后优先使用 Release 构建，失败或需要断言、栈信息时再切换 Debug。
- 标准库根目录：`REDPANDA_PARSER_STDLIB_MINGW_ROOT=D:/mingw`
- Direct Release 标准库构建目录：`R:/build-redpanda-qt5-static-release-parser-stdlib-gcc81`；主干对照目录：`R:/build-redpanda-master-static-release-parser-stdlib-gcc81`。
- 普通功能/崩溃测试可用 gdb；性能测试直接运行 Release，并设置 `QT_FORCE_STDERR_LOGGING=1` 取得 Qt 日志。完整命令见 `AGENTS.md` 的 Windows 测试说明。
- R 盘构建目录重启后可能需要重建。
