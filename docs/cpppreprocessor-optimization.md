# CppPreprocessor 跨边界 Replay 设计

> 本文描述当前实现的设计与正确性约束。正确性优先于性能；任何缓存都必须能证明结果等价，并能通过测试证明缓存确实命中。

## 1. 目标与范围

预处理管线为 preprocess → tokenize → analysis。本次改造处理：

- 单次预处理的 Session 状态与跨轮 Registry 数据分离；
- include 扫描、replay、include 保护和失效的统一规则；
- Statement 的实际文件与跨边界祖先双重归属；
- Session 文件内容缓存，以及 include 路径解析和 replay 缓存的复用。

pp-token 宏展开器不在本次重写范围内。

硬性验收条件：边界外用户宏、跨边界包含前缀和文件内容均未变化时，`bits/stdc++.h` 不得因 reparse 被重新扫描。

## 2. 术语与生命周期

- **Session**：一次 `preprocess()` 的临时状态，包括 include 栈、当前行游标、分支栈、当前文件、本轮有序跨边界前缀和文件内容缓存。每轮从空状态开始。
- **Registry**：跨 preprocess 轮次保留的数据，包括文件记录、include 闭包、replay 记录和路径配置。
- **边界外文件**：当前 TU 根文件和项目文件。被编辑器作为根打开的文件始终属于边界外。
- **边界内文件**：编译器 include 路径中的文件，以及通过 include 路径找到但不属于项目的文件。非项目文件位于项目 include 路径中时仍属于边界内。
- **跨边界头文件**：由边界外文件直接或经边界外文件链包含进入的边界内文件。直接作为根打开的系统文件也构成一次跨边界进入。
- **跨边界前缀**：本轮 preprocessing 中，跨边界记录生效的完整有序路径序列。重扫和 replay 都会使记录生效；保护导致的 no-op 不会。
- **用户宏状态**：进入边界时可见的非 hard-defined 宏，包括宏名、参数和替换内容；“未定义”也是状态。编译器或编译参数变化由 IDE reset parser，不在 include 时比较 hard define。
- **实际文件**：define/declare statement 文本所在文件，可以是边界内子树文件或无保护的 `.tpl`。
- **跨边界祖先**：使该 statement 在本轮生效的跨边界头文件。边界外 statement 的祖先为空。

边界内文件继续包含边界内文件属于系统子树内部包含，不追加跨边界前缀。边界外文件之间的包含也不追加。正常代码不存在边界内→边界外包含；若出现，必须记录诊断并补充测试，不能静默生成错误归属。

## 3. 数据归属

### 3.1 Registry 文件记录

只为以下文件建立独立 replay/fileInfo 记录：

- 边界外文件；
- 跨边界头文件。

边界内子树文件不建立独立 replay 记录，而是并入其跨边界祖先的记录。每条记录包含：

- 文件名和传递 include 闭包；
- 进入时的用户宏状态；
- 本次生效时的有序跨边界前缀；
- 磁盘文件元数据通过实际文件元数据表关联；
- 本次扫描产生的 define/undef；
- statement 归属及重放所需的信息。

include 闭包用于级联失效；它不是 replay 条件本身。

### 3.2 实际文件元数据

Registry 用一张覆盖所有实际文件的元数据表保存 mtime/size 和 include-once 保护状态。边界内子树文件也有该元数据，但不因此获得独立 replay/fileInfo 记录。

保护状态是文件内容固有属性，文件变化时随 stamp 一起失效并重新检测。不能通过 Session 的 processed/filesCouldRepeatInclude 状态反推，也不能在 ParsedFileInfo 中重复维护。

### 3.3 Statement 位置与归属

每个 Statement 分别记录声明侧和定义侧的位置与归属：

- 声明侧：实际所在文件、跨边界祖先；
- 定义侧：实际所在文件、跨边界祖先；没有独立定义时定义侧归属为空。

`fileName`/`definitionFileName` 继续表示声明和定义的实际位置，新增的两侧祖先只用于生命周期管理，不改变跳转的位置语义。

定义合并到已有声明时，保留声明侧归属，同时记录定义侧自身的跨边界祖先。两侧可以属于不同记录，失效时必须分别处理：

- 定义侧失效：清除定义位置、定义侧祖先和 `HasDefinition`，保留有效声明；
- 声明侧失效：若定义仍有效，将定义位置同时作为声明位置，并把声明侧祖先设为定义侧祖先；两侧都失效才删除 Statement。

继承产生的成员 Statement 复制被继承成员的声明侧、定义侧归属，不能使用复制时的当前文件或空祖先。父类处理、别名解析和继承覆盖都通过 Statement 的跨边界祖先定位 Registry 记录；边界内实际文件没有独立记录。

声明侧和定义侧各自只有一个当前祖先，失效时不转移所有权。共享子树由第 5 节规则处理：相关边界记录立即失效或在入口处惰性重扫，因此不需要假设不同包含点首次分析结果相同，也不需要复制 Statement 或维护祖先集合。

## 4. 每轮处理流程

### 4.1 开始预处理

- 创建新的 Session，恢复 hard define。
- 根文件按边界外文件处理；其上一轮记录先按 Parser 的编辑/失效流程处理。
- 磁盘文件以 mtime/size 检测外部变化。文件变化后，其自身及所有依赖它的记录均不可靠，必须级联失效。
- 编辑器提供的内容不以磁盘缓存代替；编辑器修改必须进入现有 invalidate 流程。

### 4.2 include 入口统一顺序

每次进入 include 都按以下顺序处理：

1. 检查 parser reset/stop 状态和最大嵌套深度。深度检查不依赖当前栈是否存在重复文件。
2. 规范化文件名，确定包含者、被包含者和是否跨边界。
3. 更新必要的直接/传递 include 关系；保护导致的 no-op 不追加跨边界前缀。
4. 若磁盘 stamp 变化，先级联失效。当前 include 栈上正在扫描的文件不得被失效。
5. 若 include guard 或 `#pragma once` 使本次包含成为 no-op，直接跳过；不校验或追加跨边界前缀。
6. 若存在跨边界记录，同时检查用户宏状态、当前跨边界前缀，以及本轮已生效前缀中的记录是否全部 replay 成功：
   - 三项均满足且 include 闭包未失效：replay；
   - 任一项不满足：失效该记录上一轮的 define/undef、statement、fileInfo 和派生缓存，然后重扫。
7. 无可用记录时正常扫描。
8. 重扫或 replay 使记录生效后，将完整路径追加到本轮跨边界前缀。

前缀必须包含 replay 生效的记录；否则首轮记录的前缀在第二轮永远无法匹配。保护 no-op 不能追加，否则“上轮被跳过、本轮首次真正生效”的场景会误命中。只要已生效前缀中有一项经过重扫，本轮所有后续记录都不能 replay；Session 用一个布尔状态记录这一条件，无需逐项保存前缀的扫描来源。

### 4.3 Replay 与重扫

- Replay 恢复该记录的 define/undef 和 include 关系，不重新读取或扫描系统子树。
- 有效记录的 Statement 保留在 StatementMap 中，replay 直接复用，不重复创建。Statement 需要重建即说明记录已失效，必须走重扫。
- 重扫前必须清除该记录上一轮产物；不能等到扫描后清理，也不能与 Parser 中其它失效合并成一次模糊的全局删除。
- 同一文件在同一 include 栈中重复出现时，两次包含的宏环境和传递闭包可能不同。不能仅按文件名复用当前栈记录。
- 无保护文件允许按编译器语义重进；保护文件在环上重进是 no-op。所有路径都受统一嵌套深度上限约束。

## 5. 失效规则

文件变化或 replay 校验失败时，失效范围包括：

- 该记录产生的 StatementMap 条目；
- ParsedFileInfo::statements；
- namespace 缓存中的对应条目；
- define/undef 及派生路径/内容缓存；
- include 关系和继承处理状态。

按“声明/定义实际文件 + 对应侧的跨边界祖先”识别 statement。处理 declaration/definition 合并条目时，两侧分别失效；一侧仍有效时保留该侧位置，并维护另一侧位置及 namespace 缓存。

生产环境中 Preprocessor 必须隶属于 Parser，不能脱离 Parser 独立使用。Preprocessor 因 stamp 或 replay 条件自动失效记录时，必须在删除 Registry 记录前通知所属 Parser，由 Parser 使用与显式编辑失效相同的清理入口。只有直接测试 Preprocessor 的单元测试可以不安装失效处理器，此时仅清理 Preprocessor 自身的宏和文件记录。不能在 Preprocessor 中另写一套 Statement 删除规则，也不能等 Registry 记录删除后再让 Parser 推测失效范围。

文件 A 变化时，立即失效集合统一按失效前的 include 闭包计算：

```text
D = {A} ∪ {文件 f | A ∈ f 的 include 闭包}
只立即失效 D。
```

D 中记录的传递 include 闭包已覆盖直接和间接包含者，无需递归扫描 directIncludes，也不提前失效后续跨界记录。重扫 D 中任一跨界记录后，本轮已生效前缀不再全部来自 replay，因此后续记录即使路径前缀和用户宏相同，也会在各自入口处失效并重扫。这与提前删除有序后缀等价，但保留前序及无关记录，并避免立即计算和删除整个后缀。

共享内部头的后续边界记录按同一规则惰性重扫，不需要转移 Statement 所有权。重扫某条记录前必须删除其旧 Statement 和 define/undef，避免与尚未处理到的其它旧记录混杂。

若后续记录在新一轮中根本没有被包含，不做全局 orphan GC：它仍是可供其它 TU 校验复用的 Registry 缓存，但不属于当前 TU 的生效集合。当前 TU 的新 include 闭包必须使其旧 Statement 在该 TU 的上下文查找中不可见；以后只有在用户宏、前缀及前缀全部 replay 等条件重新满足时才能复用。

扫描中的当前栈记录不得立即失效，但失效请求必须保留到扫描结束后执行，不能丢弃。失效操作不得只清 define 而留下 statement，也不得只清 statement 而留下 define。

## 6. 正确性与已知代价

- 用户宏状态捕获包含点可见的非 hard define 差异。
- 有序前缀捕获系统头生效顺序的差异；即使某个头未改变宏表，也不能省略顺序校验。
- 文件内容变化通过编辑失效或 mtime/size 级联失效处理。
- 两个共享 Parser 的根文件以相反顺序包含相同系统头时，切换根文件会重扫相关记录。这是保证正确性的预期代价。
- 多 root 复用以切换时保留并共享 Parser 为前提。IDE 默认启用“编辑器隐藏时自动清空 Parser”时，隐藏编辑器会同时清空 Registry；切回后的完整重扫是该设置的正常结果，不属于 replay 未命中。
- 跨 TU 的全局 Registry 可能保留由另一 TU 引入的宏和文件记录。它只用于复用可校验的扫描结果，不代表各 TU 的宏集合天然隔离。
- 对普通库用户，优先保证公开接口在 StatementMap 中完整可见；不能为了命中率接受符号丢失。

## 7. 缓存与性能

Replay 正确后再处理其它缓存：

- **Session 文件内容缓存**：缓存续行合并和注释剥离后的行表，仅供同一轮中重复扫描的文件复用；每轮 preprocess 开始时清空，不做 Registry 级内容缓存或全局 LRU。
- **include 路径缓存**：只缓存“名字→路径”的正结果；每次做存在性校验，路径配置变化时清空，负结果不缓存。
- **文件解码**：文件按字节整块读取并一次解码，再拆分为行；保留空行、CRLF 和末尾换行语义，非法编码仍按原有候选编码顺序重试。
- **普通行展开**：同一遍 pp-token 扫描中，第一个已定义宏之前的原文直接按源行截取，不逐 token 或逐字符拼接；整行无宏时直接保留原行。只有宏及其后缀进入完整展开器，不改变宏空白、raw string 或多行宏调用处理。条件表达式仍走完整展开路径，因为未定义标识符也有条件求值语义。
- **条件表达式求值**：先展开整个 `#if`/`#elif` 表达式，再按优先级递归求值。内部 `evaluate` 参数区分计算与仅推进：`0 && rhs`、`1 || rhs` 以及 `?:` 的未选分支以 `false` 递归，从而定位外层运算符但不执行算术、位运算、比较或逻辑计算。`#if` 条件块本身的跳过由 `#else`/`#elif`/`#endif` 处理，与表达式内部短路无关。
- 性能验收记录首扫、reparse 和缓存命中。正确性测试失败时不得以性能数据替代验收。preprocessor 和完整 parse 的性能测量以独立新进程连续运行两次并采用第二次结果，排除系统文件缓存 miss；同一进程内的 reparse 只用于测量 Parser 自身缓存行为，不能替代该基线。

## 8. 验证

- 场景和正确性证明见 `cpppreprocessor-replay-scenarios.md`。
- 每个场景同时检查预处理结果、defines、StatementMap 和 scan/replay/skip/invalidate 计数。
- 使用无 replay 的全量扫描作为 oracle；结果一致但发生全扫的用例仍算失败。
- 标准库公开接口验收测试由独立 CMake 开关控制，默认关闭。本机启用方式：配置时添加 `-DREDPANDA_PARSER_STDLIB_TEST=ON -DREDPANDA_PARSER_STDLIB_MINGW_ROOT=D:/mingw`，然后运行 `test-cppparser --suite=TestCppParser test_bits_stdcpp_not_rescanned_on_stable_reparse test_standard_library_public_api_complete`。
- 完整 `test-cppparser`、`test-editor` 和真实 IDE 编辑/切换/重解析均通过后，才能进行性能验收。
