# CppTokenizer 纯切分化重构

> 状态：**整体方案已定，本次不动产品代码**。实施前以完整的 Parser/Tokenizer 测试为前置门禁。

## 重构目标

`CppTokenizer` 只做纯 token 切分；所有"合并/解析"决策移到 `CppParser`。

动机：tokenizer 目前没有语法上下文，只能用括号配平等启发式猜测 token 边界，猜错时 parser 没有纠正机会（它拿到的已经是合并好的字符串）。哪些字符构成一个"名字"是解析决策，应归 parser。

## 方向决策（2026-11，Roy 批准）：方案 A —— 声明级递归下降

讨论过的候选方向：

- **完整 AST + visit**：否决。对 IDE 需求（声明级符号表、作用域、include 关系和有限的表达式类型推导）是过度设计，不需要完整表达式 AST 或运行时求值。且对 C++ 不成立——`T::x * p;` 这类上下文敏感语法决定了没有语义反馈就无法正确建树，所以 clang 也是 Parse 与 Sema 交织而非两阶段，"纯语法树"要么内嵌歧义要么仍靠启发式，问题只是搬家。成本（工程量、全量头文件 AST 的内存/性能、打字中残破代码的容错）均不可接受。
- **CST/绿树（rust-analyzer/Roslyn 路线）**：现代 IDE 标准答案，但工程量与项目规模不匹配。
- **换 libclang/clangd 或 tree-sitter**：前者等于重写整个 C++ 分析子系统；后者只有语法没有语义，符号层仍需自己写。
- **方案 A：声明级递归下降（选定）**。token 流保持扁平，parser 侧按 C++ 声明文法组织成递归下降函数（`parseDeclaration`/`parseDeclSpecifierSeq`/`parseDeclarator`/`parseTemplateParameterList` 等，每个产生式一个函数）。

**关键澄清**：A 不是"把平循环改成递归"本身。现状本就是混合体——`handleStatement` 平循环 + scope 显式栈为主体，`handleLambda`/`evaluateConstExpr` 已是递归孤岛。方案 A 的本质是**分发依据从"关键字模式猜测"改为"上下文随调用链下传"**：被调的产生式函数知道自己在什么语法位置，`<` 是否模板参数、`[` 是否 lambda capture 由它本地判定，不需要 tokenizer 或扁平分发器猜。scope 显式栈保留（它是 `fileInfo->scopes` 的构建器，不是解析控制的替代品；clang 的 Sema 同样如此）。

**A 要消灭的现有痛点**：

- tokenizer 启发式误合并后无法纠正（本文件"现状问题清单"各项）；
- `handleStatement` 扁平巨链：任何嵌套层级的任何 token 汇集一处，靠 `checkForStructs` 等前瞻启发式猜构造；
- `mIndex` 被各 handler 全局突变（与旧 preprocessor `handleBuffer` 同味，已在那里根治过一次）；
- parser 改写 token 流的 hack（`cppparser.cpp:2930-2937` 把 `()` 改写成 `{}`/`;` 并回填 matchIndex）；
- 平循环误判缺少文件和作用域边界，一次错判可能级联污染后续解析；产生式化后可以结束当前文件的错误路径，include 文件再返回包含者。

**A 明确不变的**：显式 scope 栈；单遍、不建 AST、statement 直接提取；token 列表 + `matchIndex` 配对；除枚举等整数常量外不求表达式的运行值，只做有限的类型推导，其余表达式跳过。

**已知的真实成本**：C++ 声明语法本身复杂（declarator 的指针/数组/函数嵌套），`parseDeclarator` 写对是真功夫；现有启发式分发经真实头文件多年捶打，重写必须达到同等覆盖率——故测试补全是硬门禁。

**可参考的现存范本**：`handleLambda`（`cppparser.cpp:2987`）已是"保存现场→递归→恢复"模式，是全文件最干净的 handler，说明该方向在现有代码里顺手。

## 现状问题清单

`RedPandaIDE/src/parser/cpptokenizer.cpp` 中越权的解析决策：

1. **`<...>` 模板参数合并**：`getWord()` 在非关键字标识符后遇到 `<` 时调用 `skipAngleBracketPair()`（cpptokenizer.cpp:684）做括号配平猜测，配平成功则把整段 `<...>` 合并进 token。`a<b>c` 这类表达式与模板参数无法区分，启发式猜错即错到底。
2. **`[...]` 数组合并**：`getWord()` 把变量名后的 `[...]`（含多维，循环处理）合并进 token，并用 `simplifyArgs()` 做空白规整（cpptokenizer.cpp:527-541）。
3. **`[...]` lambda capture 吞并**：token 起始处的单 `[` 走 `TokenType::LambdaCaptures` 分支，整段 `[...]` 被一个 token 吞掉并记入 `mLambdas`（cpptokenizer.cpp:302-313）。"`[` 是 lambda capture 还是数组下标"是解析决策。
4. **`::` 限定名合并**：`getWord()` 循环合并 `A::B::C`（cpptokenizer.cpp:548-552）；case ':' 把 `::word` 前缀合并（cpptokenizer.cpp:262-273）。
5. **`template<...>` 参数整体跳过**：`skipTemplateArgs()`（cpptokenizer.cpp:813），parser 拿不到模板形参列表的 token。
6. `[[...]]` 和 GNU `__attribute__((...))` 属性整体跳过——纯词法决策，可保留在 tokenizer（不算越权）。属性参数中的括号和字符串必须正确配平，不能影响后续 inline namespace 等声明的起始位置。

parser 侧对合并结果的依赖（重构的工作量主体）：420+ 处直接消费合并后 token 文本的字符串手术——`splitLastMember`、`removeTemplateParams`、`endsWith('>')`、`startsWith('[')`、`indexOf('<')`、`matchIndex` 导航等，集中在 `cppparser.cpp` 的 `handleVar`/`handleMethod`/`checkAndHandleMethodOrVar`/`handleStructs`/`handleInheritance` 等。

## 实施设计

### Token 模型

- Token 使用值语义，由 `QVector<Token>` 连续存储；不再为每个 token 单独分配 `shared_ptr`。
- 对外提供最小词法类别：标识符、整数字面量、浮点数字面量、字符串/字符字面量、运算符或标点、预处理结果中的文件边界标记。关键字仍按文本判断，不为每个关键字建立 Token 类型。整数与浮点数按词法结构分类，包括指数、十六进制浮点和后缀；符号运算符不并入数字。
- 所有字面量都保留原文。Token 只存在于本次解析的 Session 中，保存字符串等原文不会增加 Registry 或 Statement 的长期内存负担；Token 类别用于解析判断，是否使用内容由 Parser 决定。
- 参数默认值不参与语义建模。Parameter Statement 不保存默认值；函数 `args` 在迁移期仍可能包含默认值文本，但它只是兼容和展示字符串。若以后需要高保真参数提示，应单独保存参数声明原文。
- Token 记录由 0 基 line/ch 组成的 SourcePosition，并保留 `matchIndex`。同一行内的嵌套声明、lambda 和块必须能通过 ch 区分；不另建一套偏移坐标。
- Tokenizer 计算 `()`、`[]`、`{}` 配对，不计算 `<>` 配对。未闭合结构是正常输入状态，不能中止后续切分；文件内未配对结构以当前文件尾作为虚拟匹配位置。根文件尾由不进入 token 列表的静态 FileEnd 哨兵表示，Cursor 在该位置返回哨兵而不越界；include 文件尾继续使用 FileBoundary 标记。
- matchIndex 按整个预处理 TU 的 token 流计算，使 Parser 能识别配对目标是否越过 include 文件边界。本 Parser 不支持跨文件配对；发现后记录诊断并跳到当前文件结束。include 文件随后返回包含者继续分析，根文件则自然分析到文件尾。
- `#include` 文件边界标记的行号映射继续保留，其职责无语法歧义。
- `>>` 保持单个词法 token。Parser 只有在模板闭合上下文才分两次消费；普通移位表达式和 `operator>>` 不得误拆。

### TokenCursor

Parser 通过 TokenCursor 消费 token，不再由各 handler 任意改写全局 `mIndex`。游标提供当前值、前瞻、消费、配对区域跳过以及保存/恢复位置。所有配对跳转集中在 TokenCursor 接口检查目标文件：同文件配对正常跳过；文件内未配对结构跳到当前文件尾；跨文件配对产生诊断并跳到当前文件尾，但不终止整个 TU 的分析。

游标位置包含 token 索引和 `>>` 内已经消费的 `>` 数量，因此半 token 消费也能正确回滚。试探解析只能改变并回滚游标，不得创建 Statement、修改作用域或更新其它全局解析结果。

lambda 的识别由 Parser 根据上下文完成，删除 Tokenizer 的 `mLambdas` 候选队列。表达式跳过仍使用括号和分隔符结构，不建立表达式 AST。

### DeclaratorInfo

`parseDeclarator()` 返回一个仅在声明解析期间存在的临时结果，不建立完整 AST。它由名称、外层限定作用域、是否全局限定、SourcePosition 以及有序 derivation 列表组成。名称中的模板实例参数在迁移期保持现有格式。

derivation 按从名称向外的顺序记录 Pointer、LvalueReference、RvalueReference、MemberPointer、Array 和 Function。顺序是语义的一部分：`int *a[5]` 为 Array→Pointer，而 `int (*a)[5]` 为 Pointer→Array；`int (*factory())(double)` 为 Function→Pointer→Function。不能用无序字段或单独的 pointerLevel/arraySuffix 替代。

- Pointer 保存指针自身的 cv 限定；MemberPointer 还保存成员所属类。
- Array 保存维度文本，迁移期不要求值。
- Function 保存已经解析的 ParameterInfo 列表，并根据它生成迁移期兼容用的 `args/noNameArgs`，避免函数定义后再用 `scanMethodArgs()` 二次解析。ParameterInfo 只保存创建参数 Statement 所需的类型、名称、数组或函数指针兼容信息和行号；默认值不参与语义建模。
- Function 的 const、volatile 和可变参数属性使用 bit flags；互斥的 `&`/`&&` 使用独立 ReferenceQualifier 枚举。当前 Statement 不保存异常说明，因此 `noexcept` 只需正确消费，不必在 derivation 中保留。

trailing return type 保存在其左侧最近的 Function derivation 中，以规范化原文记录，不要求值，也不需要进一步解析其中的抽象 declarator。当基础类型为 auto 且存在 trailing return type 时，生成 Statement 类型时用后者替代 auto，再应用其它 derivations。没有 trailing return type 的 auto 函数不做函数体返回类型推导。

第一个 derivation 为 Function 时，名称直接声明函数；第一个为 Pointer/Reference 时，即使后面存在 Function，名称仍是函数指针或函数引用变量。不能按最后一个 derivation 判断 StatementKind。

转换成现有 Statement 时，`command` 使用名称，`qualifier` 用于定位父作用域，声明说明符与有序 derivations 共同生成 TypeInfo。普通函数和函数指针变量都从对应 Function derivation 取得参数；数组变量继续将维度写入 `args`，函数指针变量继续设置 FunctionPointer 属性。

### DeclarationInfo 与 Statement 的边界

DeclarationInfo 是声明产生式的临时值对象，通常保存在栈上，declarators、children 和枚举值等成员也按值保存。它不是长期语义索引，不需要 shared_ptr。Statement 是进入名称查找、代码补全、类型推导和失效清理流程的单个命名实体，由 StatementModel 通过 shared_ptr 唯一持有；StatementMap 是 `command -> weak_ptr<Statement>` 的多重索引，不承担所有权。

CppStatementBuilder 是两者之间唯一的创建层。Parser 每得到一个可提交的 DeclarationInfo，就在继续后续声明前调用创建层，不积累整文件的 DeclarationInfo，也不延迟到代码补全时转换。转换后 DeclarationInfo 随作用域结束释放。解析失败的声明不提交；未闭合但已按文件尾恢复的有效声明仍可提交。

转换不是一对一复制。例如 `int a, *p, f(double value), (*pf)(char);` 只产生一个 DeclarationInfo，但会创建 a、p、f、pf 四个 Statement，并为 f 的参数 value 另建 Parameter Statement。class/namespace 的 children 递归转换为 Statement 作用域树，枚举值也分别创建 Statement。Statement 额外记录 parentScope、fullName、声明与定义文件、跨边界祖先及各类 weak 索引关系。

创建层同时把 Statement 写入实际源码文件的 ParsedFileInfo。class、namespace 和函数只以 bodyRange 建立代码作用域，父节点按父 Statement 的身份关联；声明范围仍覆盖完整声明。不同 include 文件的位置和作用域分别写入各自的 ParsedFileInfo，不跨文件比较 SourcePosition。`struct S {} value;` 中类体只属于 S 的定义，不会被当作变量 value 的函数定义。

internalParse 遍历预处理输出 token，遇到 `#define` 时将其单独转换为 Preprocessor Statement，记录名称、参数、值、实际文件、token 位置和跨边界祖先，使普通补全与 Macros 补全共享同一失效清理模型。Macro Statement 不是从 ParsedFileInfo::defines() 生成的；Define 只有名称、参数、值和文件等数据，没有行列位置，预处理输出 token 才是声明位置的来源。编译器 hard defines 由 parseHardDefines 转换为同类 Statement，没有源码文件或位置，也参与普通补全与 Macros 补全。函数或 Block 内的用户标签使用独立的 DeclarationKind::Label 和 StatementKind::Label；case/default 标签及三元表达式中的冒号不生成 Label Statement。

声明说明符由独立的 DeclSpecifierInfo 记录。`static`、`extern`、`register`、`thread_local`、`mutable` 等存储类别使用 bit flags。Statement 持久保留必要的声明 flags，`type()` 在生成兼容文本时将其与 TypeInfo 组合；static 继续映射 StatementProperty::Static，extern 继续影响声明/定义判定，register 等即使不参与补全语义也不能在兼容输出中丢失。

初始化表达式、auto 类型推导、Statement 创建及声明/定义合并不属于 DeclaratorInfo 或 DeclSpecifierInfo，由 `parseDeclaration()` 在 declarator 完成后处理。

### 声明产生式与错误恢复

`parseScope()` 分别分发 namespace、class、enum、using/alias、template 和普通声明。普通声明由 `parseDeclSpecifierSeq()`、`parseDeclarator()` 和声明后缀组成；一条声明共享 DeclSpecifierInfo，每个 declarator 分别生成 TypeInfo 和 Statement。namespace/class/enum 保留各自产生式，不强行套入普通 declarator。构造函数识别只收集限定名各段的标识符文本，并跳过配对模板参数；只有这些文本用于判断类内名称是否等于当前类名、类外是否符合 `A::A`。模板参数不参与构造函数识别，避免在每次说明符循环中重复进行类型语义解析。

前瞻和试探只移动 TokenCursor 并创建临时结构。Statement、作用域、namespace 索引和继承状态只在声明结构确定后提交；class/function 可在头部确定后创建自身 Statement，再进入正文。模板形参查找层使用作用域保护，失败和提前返回都必须弹出。

函数体内通过声明关键字、可见类型查询和有限前瞻区分声明与表达式，不建立表达式 AST。确认的解析错误可以结束当前文件的错误路径，但不因此终止整个 TU。跨文件配对记录诊断并跳到当前文件尾；include 文件随后返回包含者继续分析。文件内未配对结构不是致命错误，当前文件尾就是其虚拟匹配位置，仍按正常流程分析到文件尾。

声明入口失败时必须恢复起始 checkpoint 并丢弃临时 DeclarationInfo，不能把部分消费的位置留给调用方。class、namespace 和 Block 以开 `{` 的 matchIndex 作为闭合硬边界；子声明越界时恢复到匹配闭合符，防止一个未支持构造吞掉外层作用域和后续 include 文件。未支持语法属于可恢复错误，只在 `PARSER_DEBUG_LOG` 下记录日志。include 文件应是完整源码，debug 下越界恢复同时触发断言以尽早暴露分析错误；根文件可能正在编辑，只记录日志。matchIndex 与闭合 token 类型、恢复后的游标位置等实现不变量也使用断言检查。所有断言都只用于 debug，release 下无论根文件还是 include 文件都执行安全恢复，不能依赖断言避免崩溃。

编辑器以行输入触发重解析，因此不为缺少分号的残缺声明增加提前提交路径，也不为未闭合表达式设计按行恢复。输入未完成或文件存在语法错误时，不保证该文件后续内容的解析结果正确，等待修正后的重解析即可。

文件结束和错误退出路径都必须正常展开游标、模板形参和作用域等临时状态。所有已经创建的 Statement 都必须具有有效的实际文件和跨边界祖先归属，后续 invalidate/rescan 能完整清理，并且不崩溃、不死循环。

对应确定性测试：

- `test_unclosed_brace_uses_file_end_as_match`：函数或类的 `{` 到文件尾仍未闭合；文件尾前的成员/局部 Statement 仍生成，作用域正常展开，重扫不重复。
- `test_unclosed_parenthesis_uses_file_end_as_match`：`if (x > 0` 等未完成条件以文件尾结束；不崩溃、不死循环，已生成 Statement 的归属有效。
- `test_cross_file_match_returns_to_includer_after_diagnostic`：include 文件中的开括号配对到包含者时产生诊断并跳到 include 文件尾；随后继续分析包含者，根文件后续 Statement 可见。
- `test_partial_statements_are_removed_after_correction_rescan`：先解析含错误文件，再通过编辑器内容回调修正并重扫；旧 Statement 完整清理，新 Statement 正确生成。
- `test_repeated_error_rescan_does_not_duplicate_statements`：同一错误文件反复 invalidate/rescan，Statement 数量不增长，过期 weak 引用不会导致崩溃。
- `test_expired_statement_indexes_are_ignored_during_invalidation`：文件失效时遇到已释放 Statement 的 weak 索引，安全跳过并移除索引。
- `test_scope_recovery_preserves_following_declarations`：带说明符和初始化列表的构造函数发生误判时，类/namespace 闭合不被吞掉，后续声明仍可见。
- `test_failed_declaration_restores_cursor`：声明入口失败后游标恢复到起始 checkpoint。

### 调试诊断

预置默认关闭的 Parser 调试日志，复用现有 `PARSER_DEBUG_LOG` 开关，不增加用户可见诊断或运行时开销。日志至少记录实际文件、0 基行号、token 索引及半消费状态、当前产生式、期望与实际 token、include/边界祖先上下文，以及终止位置。

必须覆盖文件结束时的括号或作用域不匹配、无效 matchIndex、产生式失败、回溯和终止、模板闭合失败及 include 边界异常。首个错误应保留足够上下文，避免后续级联日志淹没真正原因；现有完整 token/Statement dump 继续作为可选的详细输出。

诊断只辅助定位“文件合法但 Parser 不支持”的情况，不改变错误恢复、Statement 提交或失效清理规则。代码级不变量仍使用断言；日志不能代替断言或测试。

### 直接迁移

Token 流只保留纯切分形式，Parser 直接使用 TokenCursor 和声明产生式，不生成旧式合并 token，也不设置临时 token 兼容层。旧 handler 按声明类别逐批替换，避免复制旧启发式、维护两套索引或在最终删除前增加额外迁移工作。

`command`/`fullName` 的存储格式初期保持不变：实例化名仍可包含 `<...>`，`fullName` 仍用 `::` 连接。Statement::type() 提供的文本兼容只属于对外输出，不影响 token 流。模板声明需要新增模板类别和形参绑定信息；StatementMap 容器替换和键冗余消除不在同一批次进行。

不保留新旧两套生产解析路径。旧实现用于事先确定测试基线；新实现直接替换旧实现。对旧实现已经明确错误的行为，不以兼容性为理由保留。

### CppParser 生产接入

CppParser 的唯一生产路径为 preprocess → CppTokenizer → CppSyntaxParser → CppStatementBuilder。FileBoundary token 在读取后保存新文件名，DeclarationInfo 也携带实际文件；递归创建子声明时使用 child 的文件，include 内容不登记到宿主的 ParsedFileInfo。旧 handler、token 改写和字符串类型手术不保留。

同一实体的声明和定义合并到一个 Statement，分别保存 declarationRange/definitionRange、实际文件和跨边界祖先，并合并声明说明符，避免重开 namespace 时丢失 inline 等信息；参数不重复创建。变量统一使用 StatementKind::Variable，global/local/class-local 由 StatementScope 区分。typeInfo.toString 保留完整结构化类型，Statement::type() 生成旧 API 所需的存储类别、返回类型及数组/函数指针兼容文本。

merge 的候选查询使用独立的无锁 doFindXXX 实现。局部声明只枚举当前词法作用域的直接 children，不同 Block 的同名局部声明不合并。非局部声明按实际 fullName 查找最长 namespace 前缀，枚举索引中的全部重开及 inline/匿名关联节点，再沿 children 取得全部同名和重载候选，最后按 kind、noNameArgs 和跨边界祖先筛选。候选不存在时直接返回空，不再线性扫描全量 Statement，也不使用单结果查询失败后的兜底。

class/namespace/function 以 bodyRange 建立作用域；for/if/catch 等 Block 以整个控制语句范围建立作用域，header 中的初始化、循环变量和异常参数均可查询。按精确行列查询使用 findScopeAt，按行兼容查询使用 findScopeAtLine，不能用最大列号模拟。findStatementOf 只返回当前 root 及其 include 闭包中可见的 Statement，已移除 include 的全局缓存不会泄漏到该 root。inline namespace 保留实际 fullName，但限定名称查找递归穿透其成员，因而 std::__cxx11 中的类型也可通过 std 名称找到；该规则与 using namespace 的未限定名称查找分开处理。

实施批次按消费边界划分：

1. 以 cppreference 示例建立语法覆盖和现有缺口基线；
2. 纯 Tokenizer、TokenCursor、文件边界检查和调试诊断；
3. DeclSpecifierInfo、DeclaratorInfo、TypeInfo 及 Statement::type() 兼容接口；
4. 模板参数解析、`<` 消歧和有限模板绑定；
5. 变量、函数、类和模板声明的产生式化；
6. EvalResult、通用类型操作及 container/iterator/algorithm/range 验收；
7. StatementModel 唯一所有权和各类 weak 索引；
8. 删除旧字符串手术和 token 改写 hack。

每批都必须独立构建、通过完整测试并检查真实标准库解析。验收同时记录 StatementMap、作用域、声明/定义位置和继承结果，以及 token 数量、内存和耗时。

## 设计结论（2026-11 讨论）

### StatementMap 结构

- **作用域树 + 每层索引的整体结构合理，不动**——C++ 名字查找天然是作用域相对的（scope 内精确匹配、枚举 scope 成员、沿链向上），树-of-maps 正好镜像它。
- **trie 出局**：所有查询（parse 期 + 补全）都是精确匹配或全量枚举，没有前缀查询；补全是模糊打分必须全扫。键短、多数 scope 很小，trie 的节点开销纯浪费。
- **per-scope 容器从 QMultiMap 换 hash 是"有正确性依据的优化"**（见下第 1 条），但仍建议等基准数据再动手。换之前须验证一个依赖：QMultiMap 迭代按名有序，补全列表初始按字母分组；换 hash 后顺序随机，需确认补全排序比较器（codecompletionpopup.cpp）在同分时有确定性 tie-break。
- 键冗余（map key 与 `statement->command` 双份存储）随容器改造一并消除。

### Statement 所有权

StatementModel 是 Statement 的唯一所有权中心，在生产配置中保存全量 Statement 列表。全局索引、children、namespace 索引、作用域索引和 ParsedFileInfo::statements 都只保存 weak_ptr；TypeInfo 的声明引用也遵守同一规则。namespace A::B::C 按真实嵌套关系建立。除各作用域的 command 索引外，StatementModel 还维护 namespace 专用的 fullName 多值索引：每个 namespace 登记到自身 fullName，inline/匿名 namespace 同时登记到直接父 namespace 的 fullName 下。该索引只服务多候选 namespace 查询，不扩展为通用 fullName 索引。

查询时将 weak 引用临时 lock 为 PStatement，保证本次操作期间对象存活。新增、声明属性合并、失效删除和 clear 都必须同步维护索引；父 namespace 先删除时也要清理父键下的透明子节点关联。weak_ptr 只避免所有权环，不能代替索引清理。Statement 的释放不再取决于哪个文件记录最后引用它。

### 重开 namespace 的限定名查找

只有 namespace 会因重开而由多个 Statement 表示同一逻辑作用域。class/enum 的声明与定义应合并，函数重载不是同一作用域的重复节点，Block 只是局部作用域占位；它们不参与这种重名分支查找。namespace 不能声明在 class 内，因此 qualified name 的 namespace 分支只会出现在前部的 namespace 前缀中，进入 class/enum 等非 namespace 作用域后不再需要 namespace 回退。

查找以显式栈实现 DFS，栈项只保存 namespace 作用域和当前名称段下标。它遍历当前名称段上的全部同名 namespace 和 inline/匿名透明分支；某个分支缺少后续名称时，回退并尝试同层其他重开声明，每一层都遵守同一规则。透明分支直接读取 namespace 索引，不再扫描全量 Statement 收集 inline flags，也不只看当前 Statement 自身 flags。找到首个非 namespace 候选后，namespace 前缀即告结束，余下部分沿用普通成员查找。词法作用域优先级保持不变：最近作用域存在首段名称时只遍历该层的重开 namespace，不与外层另一个同名 namespace 混合。public findNamespace 默认只返回实际 fullName 相同的节点，调用方可通过 includeInline 参数同时取得挂到父键下的 inline 节点。

本批只修正普通、inline 和匿名 namespace 的重开查找，不扩展到 Block、函数/lambda 局部作用域、通用多义符号查找或 namespace alias 的新能力。函数、lambda 和 Block 不作为全局限定名的中间作用域，`f::local`、`block::inner` 等伪路径返回空，但其内部局部名称仍可从对应 scope 开始查找。class 与变量/函数同名虽然合法，但属于明确不支持的低价值消歧场景；此类代码的限定名查找结果不作保证，也不为它增加候选类型过滤或额外回退。

查找测试矩阵包括：顶层和第二、第三层重开 namespace；不同构建顺序；inline/匿名 namespace 的透明查找及重开分支；直接 `B` 与 inline 中另一个 `B` 都参与查找；inline 声明先于或后于成员定义，以及删除 inline 声明后不再透明；进入 class 后的普通成员查找；全局 `::`；最近词法作用域优先且不混入外层同名 namespace；局部变量 shadowing；函数、lambda 和 Block 的局部可见性与全局限定路径隔离。以上场景分别由 `test_reopened_namespace_lookup_checks_all_declarations`、`test_reopened_nested_namespace_lookup_checks_all_branches`、`test_namespace_lookup_transparency_and_scope_boundaries`、`test_qualified_lookup_respects_lexical_and_local_scopes` 和 `test_template_forward_declaration_with_default_arguments` 固化。

### 继承成员查找

继承成员默认采用按需递归查找，不复制基类 children。derived 的直接成员优先，随后递归补充基类成员；override 按函数签名去重，inline/匿名 namespace 作为透明作用域处理。树形视图和补全列表通过统一的 membersOf(scope) 获得临时成员列表，不读取持久 children 作为继承结果。

复制 children 虽然能降低单次查询成本，但会增加 Statement 和索引数量，并要求在基类变化时级联失效。它还必须处理 override 签名、继承访问级别、模板参数替换、声明范围和 boundary ancestor，容易破坏 replay 与唯一所有权模型。继承访问级别只在临时查询结果中计算，不改写基类 Statement。

递归查找的代价是单次查询为 O(继承深度)，生成补全列表需要遍历继承成员。若实测成为瓶颈，可以增加按 derived、receiver 实际模板参数和 parser 代次区分的缓存；缓存仅保存 weak_ptr 和有效访问信息，不复制 Statement。缓存不是默认前提，必须以性能数据决定。

### auto 类型推导与结构化绑定的边界

expression 的类型必须首先由表达式求值独立确定，结构化绑定不参与 expression 返回类型的推导。普通 `auto x = expression` 只使用表达式结果，不需要结构化绑定规则。当前不根据 `auto [a,b] = expression` 的拆分规则继续推导各绑定名的类型；解析器仍保留声明结构，但不把 initializer 的整体类型错误赋给每个绑定名，也不为此维护 publicProperties 索引。若以后确需支持绑定名类型推导，再作为独立功能设计，不与普通 auto 推导或继承成员枚举混合。

### 源码位置与范围查询

SourcePosition 由 0 基 line/ch 组成，SourceRange 使用包含端点的起止位置。Statement 分别记录声明和定义范围；class/namespace/function 的作用域使用 bodyRange。未闭合到文件尾时，范围在文件内最后 token 处结束，不产生无效跨文件位置。

CppScope 保存完整 SourceRange 和父索引。查询先按起点二分找到最后一个起点不大于目标位置的 scope；若目标超出其结束位置，则沿父索引向外层回退，直到找到包含目标的范围或到达全局。复杂度为 O(log 节点数 + 嵌套深度)，不需要通用 interval tree。

测试必须同时校验声明/定义位置、body 范围、嵌套和兄弟范围、范围外的全局位置，以及同一行内按 ch 区分的作用域。文件结束和 include 返回路径也纳入范围测试。

### 局部作用域、using 与 auto

函数体中的局部声明和控制块参与 Statement/Scope 构建，表达式语句不建立语句 AST。CppScope 不另存作用域类型，直接从其 StatementKind 取得 namespace、class、function 或 Block 语义；StatementScope 只描述声明实体的 global/local/class-local 归属。Block 使用 DummyStatement 避免作为普通符号参与补全，但 children 和 bodyRange 正常用于查询。

for 和 catch 各自拥有独立 local Block：普通 for 的初始化声明、range-for 的循环变量以及 catch 的异常参数都挂在该 Block 下，块外不可见。while、if、try 和普通嵌套 `{}` 也建立 Block，外层变量通过 parentIndex 链查找。普通 for 与 range-for 不在 Statement 层区分；临时 DeclarationInfo 以 Block、rangeExpression 和循环变量 declarator 表示 range-for，range 表达式在 for 外部上下文求值。

using 不创建作用域。using namespace/using enum 记录到当前 Statement 的 usingList（全局则记录到 ParsedFileInfo），using alias 在当前作用域创建 Alias Statement，using declaration 引入目标名称及类型。名称查找沿词法父链处理这些指令，局部 using 不因函数体解析而丢失。

`auto x = expression` 在创建变量 Statement 时求初始化表达式，普通 auto 去除结果引用和顶层 cv，auto 引用声明保留相应引用；算术运算应用通常算术转换，条件表达式合并两侧算术类型，并识别整数字面量和浮点字面量后缀。range-for 按声明形式使用 rangeValue 或 rangeReference。求值失败保留 auto，不影响其它声明。测试必须验证循环内部变量方法/属性、循环外不可见性、`while (++it != end)`、range-for、普通 for、catch 和局部 using。

### 模板与类型推导

声明上下文遇到 `<` 时须先解析前面的名字并查它是不是模板。parse 路径本就有大量精确名字查询，模板判定新增同类查询，per-scope 容器换 hash 因此从优化升级为设计支撑。跳过表达式/函数体时无需配平 `<>`，只有解析声明时才判定；查不到时回退到结构配平。

Statement 需要区分非模板、主模板、部分特化和全特化，并保存模板形参名称、类别及 parameter-pack 标记，供模板绑定使用。`templateSpecializationParams` 继续表示名称上的实际参数，不与声明形参混用。默认模板实参和约束表达式在第一版中只正确消费，不持久保存。类型和模板模板参数的默认实参按 `<...>` 层级读取，内层 `>`/`>>` 逐层消费，外层逗号和闭合符留给参数列表；非类型默认值仍按表达式处理，不把比较运算符 `<` 当作模板起点。`()[]{}` 在各默认值中均保持配平，覆盖嵌套模板、连续默认值、比较表达式和带逗号函数调用。

类型推导采用有限通用模板绑定优先：将形参名称绑定到实际 TypeInfo 或常量表达式，在类成员、别名、嵌套类型和简单函数模板返回值中做替换。模板声明内部使用 Session 级模板形参查找层，避免形参泄漏到声明之外。先用自定义 `Box<std::string>` 等类型验证通用性，再接入 STL，不能把 STL 名称识别冒充通用实现。

Parser 解析声明时只保存模板声明、模板实参原文和 TypeInfo 的其它结构，不实例化模板，也不生成实例化 Statement。TypeInfo 以 `templateArgumentsText` 保存包括尖括号在内的实参 token 文本，此时 `templateArguments` 保持为空；只有类型求值、模板绑定或类型操作确实需要实参结构时，才调用 `parseTemplateArguments()` 生成有序 `TemplateArgument`。求值结果以结构化参数为准，避免已绑定的参数仍显示原始形参文本。为区分 `<` 是否模板参数而进行的名称查询不属于实例化。类模板绑定、别名展开、特化选择、成员类型和函数返回类型均在查找类型信息时按需计算。`auto x = expression` 是声明阶段的例外：需要立即求初始化表达式的类型来确定 x 的 TypeInfo，但同样只计算所需类型链，不生成模板实例。普通 auto 函数的返回类型仍留待后续专门能力处理，不做函数体推导。

当前不支持 CTAD。deduction guide 的语法必须正确消费，但不生成 Statement：识别依据是函数 declarator 带 trailing return 且没有 decl-specifier 返回类型，不能误忽略普通 `auto f() -> T`。测试必须验证类定义和成员可查找、guide 不产生同名函数条目，以及普通 trailing-return 函数仍保留。

未来若实现 CTAD，可在 Class Statement 内增加 `guides` 值列表，联合 class 构造函数和 guides 进行推导。guides 是与类关联的语义信息，不进入 children、fullName 或普通函数查找，也不作为独立 shared_ptr Statement 持有；每项保存模板参数、函数参数、推导结果类型、源码文件和位置。文件失效时按各自文件/边界祖先清理，Class 删除时随 Class 一并释放。该列表仅是后续可选设计，当前不实现。

函数 Statement 同时保留结构化 ParameterInfo 列表，不只保存 args 文本。调用时逐参数比较 formal/actual TypeInfo：直接模板形参绑定实际类型，引用先去除，指针/数组按层递归，Container<T> 等实际模板参数继续递归绑定；返回 TypeInfo 再应用这些绑定。该机制用于 find_if、transform 等算法，不做重载决议、SFINAE、参数包展开或完整隐式转换序列。

无法仅从声明签名得到的规则通过可插拔的 TypeOperationRegistry 添加，例如算法回调的调用参数、range adaptor 的元素转换和 tuple/range 类型操作。专用规则按标准库操作集中注册，不散落成各成员函数的硬编码。通用机制过于复杂的局部能力可以先由专用规则实现，但对外使用同一类型操作接口。

rangeElement 统一计算 rangeValue/rangeReference：iota_view 使用数值类型；filter_view 递归保持输入 range 的元素或引用；transform_view 使用函数、函数指针或 operator() 的返回类型；join_view 连续两次取元素以展平内层 range；elements_view 从 tuple、pair 或 array 的实际模板实参中取第 N 项。查不到专用规则时回退到普通容器的 value_type/front，不把 view 名称识别代替通用声明解析。

函数对象调用的实参也按完整表达式求值，逗号只分隔实参。TypeOperationRegistry 将 filter/transform 调用转换为带实际回调 TypeInfo 的闭包；operator| 再把输入 range 与闭包合成为 filter_view/transform_view，join adaptor 直接合成为 join_view。显式非类型模板实参仅在 Registry 确认目标支持时消费，否则 `<` 仍按比较运算处理；views::elements<N> 因此可以合成为带 N 的 elements_view。管道类型可以传给 auto 和 range-for，最终成员访问继续使用同一表达式求值路径。

初始化表达式重新切分时使用 relocateTokens 应用原始文件、起始 line/ch 和首行列偏移，lambda 等 Statement 不会错误落到表达式副本的 0 行。lambda 是实际声明而非模板实例：StatementModel 持有 Lambda Statement，参数归属于其 operator()，返回表达式在该函数上下文复用完整求值器；显式 trailing return 优先，无 return 时为 void。有限推导覆盖具名类型参数、参数成员调用和算术返回表达式，复杂控制流、多返回合并及 generic lambda 的 dependent 参数绑定仍属于后续能力。

完整模板实例化、SFINAE、concepts 求值和完整 dependent-name 语义仍不在范围内；不生成整套实例化 Statement 副本。auto 函数不做函数体返回类型推导，transform 所需的 lambda 返回表达式推导是独立的有限功能。

验收以常用 STL 容器、C++20 range/view 以及 cppreference 的 container/iterator/algorithm 示例为目标。例如 `std::vector<std::string> lst; lst.front().` 应能列出 string 成员。range/view 的代表性场景取自：

- [filter_view](https://en.cppreference.com/w/cpp/ranges/filter_view.html)：`iota | filter | transform` 后用于 range-for，filter 保持输入元素类型。
- [transform_view](https://en.cppreference.com/w/cpp/ranges/transform_view.html)：支持直接调用和 `|` 管道；普通函数从声明取得返回类型，lambda 使用有限的返回类型推导。
- [elements_view](https://en.cppreference.com/w/cpp/ranges/elements_view.html)：`views::elements<N>` 从 tuple-like 元素中取第 N 项类型。
- [join_view](https://en.cppreference.com/w/cpp/ranges/join_view.html)：嵌套 range 展平为内层元素类型。

验收覆盖 view 对象成员、begin 迭代器解引用、range-for 元素、算法返回类型以及回调中的 lambda 参数和返回类型；连续 `.`, `->`, `*`, `[]` 和函数调用都应沿类型推导链处理。示例及预期提示位置复制到仓库内资源，不依赖外部网页或工具链文件。每个缺失能力都登记为具体场景并分别验证。

### TypeInfo 与表达式结果

TypeInfo 只表示类型，包含规范化名称、可选的已解析声明、模板实参原文及按需生成的有序实际模板参数、复用 declarator 规则的有序 TypeDerivation 列表，以及基础类型 cv flags。实际模板参数使用 TemplateArgument 区分 TypeInfo 和非类型常量表达式，因此能表示 elements_view 等混合参数。声明对象通常只有实参原文；求值完成后才以结构化参数为准，二者不作为两套语义同时消费。

Statement 直接保存结构化 TypeInfo，不再同时保存一份类型文本。原有 `statement->type` 读取迁移为 `statement->type()`，需要兼容文本时按既有格式生成；类型修改通过明确接口完成。声明解析、auto 推导和 view 管道都直接写入 TypeInfo，避免重复解析及双份数据同步。`command/fullName/args/value` 不在这一阶段一起结构化。

TypeInfo 中持久保存的声明引用使用 weak_ptr，防止类、成员 Statement 和类型声明之间形成所有权环。模板绑定上下文由求值过程持有，不形成额外的 Statement 持久所有权。

表达式求值返回 EvalResult，包含 TypeInfo、表达式类别、原始 Statement 和必要的模板绑定上下文。容器元素、迭代器值/引用和 range 元素不增加固定字段，而由 containerValue、iteratorValue、iteratorReference、rangeReference、invokeResult 等统一类型操作计算。

求值器保持旧实现的递归表达式语义，不建立表达式 AST，也不在解析其它声明时预先计算。二元运算按 comma、赋值、条件、逻辑、位运算、比较、移位、加减、乘除和成员指针的优先级递归；括号内调用完整子表达式，类型转换与普通分组分别处理。`new`、一元运算、字面量、类型/`::` 名称、函数调用、初始化、`[]`、`.`、`->` 和前后缀自增减都进入同一求值路径。实参和下标内容利用 matchIndex 跳过。

类型解析先尝试相应的运算符重载，再应用内建指针、引用和算术规则。`->` 递归处理内建指针或 operator->，解引用处理内建指针或 operator*，自增减使用 operator++/operator-- 的返回类型。每一步只保留下一步所需的 TypeInfo，因此 `container.front().size()` 和 `(*it++)->size()` 等链条可以逐段计算。查不到声明或尚不支持的表达式返回无效 EvalResult，不把部分前缀当作完整结果。

配套设计点：

- **解析顺序保证信息可用**：头文件先于包含者解析、文件内声明按出现顺序入表，遇到 `<` 时前序模板声明已在表中，同 phase 查找可行（与 clang 的 Parse/Sema 交织同理）。
- **查不到的兜底策略**：代码敲到一半、include 未解析、dependent name 都会查不到。定为"查找优先，查不到回退结构启发式（配平检测）"；宁可多合并不可漏合并——漏合并会让模板代码解析直接散架，误合并只是表达式里多个废 token。

## cppreference 语法覆盖测试

测试资源位于 `RedPandaIDE/test/resources/syntax/`，文件内记录来源链接。默认测试不依赖外部标准库文件。

- `test_cppreference_declarator_syntax`：多 declarator、指针、函数指针、复杂参数、trailing return、noexcept 和可变参数。
- `test_cppreference_function_syntax`：函数声明/定义、默认参数、函数返回函数指针、局部函数声明、函数 try block 和 delete。
- `test_cppreference_class_syntax`：数据成员、成员函数、嵌套类型、枚举、typedef、继承、using 和成员模板。
- `test_cppreference_template_syntax`：类/函数/别名模板、全特化、模板模板及非类型参数、parameter pack。
- `test_cppreference_enum_syntax`：scoped/unscoped enum、底层类型、枚举值求值、匿名 enum 和 using enum。
- `test_cppreference_namespace_syntax`：namespace 重开、外部定义、嵌套/inline/匿名 namespace 和全特化。
- `test_cppreference_lambda_syntax`：捕获、默认参数、递归、显式模板参数和 init-capture。
- `test_cppreference_structured_binding_syntax`：if 条件绑定、bit-field 绑定、lambda 初始化和引用绑定。
- `test_cppreference_range_for_syntax`：容器、数组、引用、属性、init-statement、typedef/using。
- `test_cppreference_ranges_syntax`：iota/filter/transform/join 管道及 range-for。

当前实现由此确认的覆盖缺口：

- 同一声明中的多个成员函数 declarator，例如 `virtual int f(char) const, g(int) &&`；
- trailing return 的函数指针声明，例如 `auto fp11() -> void(*)(const std::string&)`；
- `operator()` 成员函数；
- 模板类中的成员函数，例如 `A<T>::f`；
- namespace 成员函数体内的 `extern` 声明归属，例如 `Q::V::h`；
- if 条件中的结构化绑定，例如 `if (auto [iter, success] = ...; success)`；
- range-for 中 `char const c` 形式的循环变量。

这些场景的预期以保持 C++ 声明结构为准，不根据当前实现的错误结果修改测试。

## IDE 查询与重命名约定

`getIncludedFiles()` 只返回 include 依赖文件，不包含关系起点。补全所需的是“当前文件可见的源码文件集合”，由 `CodeCompletionPopup::prepareSearch()` 在依赖集合上另外加入当前文件，不能改变 Parser 接口的依赖语义。

`gotoDeclaration`/`gotoDefinition` 只要求鼠标位于标识符上时正确跳转。函数提示分两条路径：输入调用参数时，光标位于 `()` 内展示被调用函数的声明；`getParserHint` 用于鼠标悬停在函数标识符上时显示声明。类名与构造函数同名时，类声明位置必须选择 Class，构造函数调用位置仍按实际语义选择 Constructor 或 Class。

重命名按 syntax token 工作：只处理 `TokenType::Identifier`，用该位置表达式查找 Statement，并比较声明文件与声明位置，不能只按文本相同替换。`findOccurenceInFile` 和 `renameSymbolInEditor` 都采用该规则。`renameSymbol` 只允许声明和定义位于当前文件的符号，私有实现直接操作传入的当前 Editor，不再通过 MainWindow 二次查找，也不处理未打开文件。`getWordForCompletionSearch` 只面向补全光标前文本，不能作为通用标识符提取 API。

补全和类浏览器使用统一的临时继承成员列表，derived 直接成员优先，基类递归补充；继承访问级别按路径计算，不修改原 Statement。widgets 测试使用独立 `test-widgets` 目标，不依赖 Editor 测试类，覆盖所有 `CodeCompletionType`、继承访问、类浏览器、goto、function tips 和 rename。
