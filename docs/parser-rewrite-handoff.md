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
- TestCppParser：39 passed
- TestWidgets：18 passed
- TestEditor：60 passed
- QSynedit 全套测试通过

标准库构建目录 `R:/build-redpanda-qt5-debug-parser-stdlib`：

- `test_standard_library_string` 已通过。
- `test_standard_library_public_api_complete` 已通过，耗时约 445 秒；此前 300 秒超时只是运行时间较长，不是失败。

## 后续顺序

1. 取得标准库完整公开接口验收结果；若失败，先按预处理、token、声明产生式、Statement 归属的顺序定位，不先打补丁。
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
- 测试通过 gdb 运行；R 盘构建目录重启后可能需要重建。
