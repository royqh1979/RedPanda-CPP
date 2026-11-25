# CppParser 设计

## 总体目标与声明级解析

CppTokenizer 只负责纯 token 切分；语法合并和解析决策由 Parser 根据调用链中的上下文完成。采用声明级递归下降：token 流保持扁平，声明产生式显式传递当前语法位置，`<`、`[` 等符号不再由 Tokenizer 脱离上下文猜测。

不建立完整 AST/CST，不引入 libclang/tree-sitter，也不把表达式运行时求值纳入 Parser。保留单遍 Statement 提取、显式作用域构建、Token 配对信息和有限表达式类型推导。残破代码以当前文件/作用域恢复为主，不为追求完整语法树增加长期内存和复杂度。

### Token 模型

- Token 使用值语义，由 `QVector<Token>` 连续存储；不再为每个 token 单独分配 `shared_ptr`。`addToken()` 按值接收文本并移动构造 Token，调用点也移动局部 QString，避免局部 Token 和 append 的重复复制；FileBoundary 改从向量中保存的文本读取边界信息。Token 声明为 `Q_RELOCATABLE_TYPE`，QVector 扩容时可整体重定位，避免逐个复制含 QString 的 Token。
- 对外提供最小词法类别：标识符、整数字面量、浮点数字面量、字符串/字符字面量、运算符或标点、预处理结果中的文件边界标记。关键字仍按文本判断，不为每个关键字建立 Token 类型。整数与浮点数按词法结构分类，包括指数、十六进制浮点和后缀；符号运算符不并入数字。
- 所有字面量都保留原文。Token 只存在于本次解析的 Session 中，保存字符串等原文不会增加 Registry 或 Statement 的长期内存负担；Token 类别用于解析判断，是否使用内容由 Parser 决定。
- 参数默认值不做通用表达式求值。Parameter Statement 不保存默认值；函数 ParameterInfo 以 `defaultArgumentText` 保存原文，供有限重载筛选和构造转换使用；函数 `args` 中的默认值文本只用于兼容和展示，不参与签名比较。
- Token 记录由 0 基 line/ch 组成的 SourcePosition，并保留 `matchIndex`。同一行内的嵌套声明、lambda 和块必须能通过 ch 区分；不另建一套偏移坐标。Tokenizer 的源缓冲区同时被 `mCurrent` 原始指针引用，读取时不得使用非 const `QString::operator[]`，否则可能 detach 并使指针失效；只读访问统一使用 `at()` 或其它不触发 detach 的接口。
- 标识符、运算符和连续非换行空白直接批量推进指针并一次更新 column；只有换行才逐行更新位置。Tokenizer 的输入已由 Preprocessor 去除注释，`skipWhitespace()` 不再承担注释解析。直接使用原始 cppreference 资源的测试入口必须先调用相同的注释替换，不能为了让原始输入绕过预处理而在生产 Tokenizer 中保留无效注释分支。
- Tokenizer 计算 `()`、`[]`、`{}` 配对，不计算 `<>` 配对。未闭合结构是正常输入状态，不能中止后续切分；文件内未配对结构以当前文件尾作为虚拟匹配位置。根文件尾由不进入 token 列表的静态 FileEnd 哨兵表示，Cursor 在该位置返回哨兵而不越界；include 文件尾继续使用 FileBoundary 标记。
- matchIndex 按整个预处理 TU 的 token 流计算，使 Parser 能识别配对目标是否越过 include 文件边界。本 Parser 不支持跨文件配对；发现后记录诊断并跳到当前文件结束。include 文件随后返回包含者继续分析，根文件则自然分析到文件尾。
- `#include` 文件边界标记的行号映射继续保留，其职责无语法歧义。
- `>>` 保持单个词法 token。Parser 只有在模板闭合上下文才分两次消费；普通移位表达式和 `operator>>` 不得误拆。

### TokenCursor

Parser 通过 TokenCursor 消费 token，不再由各 handler 任意改写全局 `mIndex`。游标提供当前值、前瞻、消费、配对区域跳过以及保存/恢复位置。所有配对跳转集中在 TokenCursor 接口检查目标文件：同文件配对正常跳过；文件内未配对结构跳到当前文件尾；跨文件配对产生诊断并跳到当前文件尾，但不终止整个 TU 的分析。

游标位置包含 token 索引和 `>>` 内已经消费的 `>` 数量，因此半 token 消费也能正确回滚。试探解析只能改变并回滚游标，不得创建 Statement、修改作用域或更新其它全局解析结果。

TokenCursor 和 Tokenizer 的字面量文本匹配使用 `const char *`/`QLatin1String` 重载，不为每次 `consume("...")` 或 `startsWith("...")` 构造临时 QString；动态文本仍使用 QString 重载。该约束适用于声明解析的高频比较路径，不能以统一接口为由保留隐式字符串分配。

lambda 的识别由 Parser 根据上下文完成，删除 Tokenizer 的 `mLambdas` 候选队列。表达式跳过仍使用括号和分隔符结构，不建立表达式 AST。

### DeclaratorInfo

`parseDeclarator()` 返回一个仅在声明解析期间存在的临时结果，不建立完整 AST。它由名称、外层限定作用域、是否全局限定、SourcePosition 以及有序 derivation 列表组成。名称中的模板实例参数在迁移期保持现有格式。

derivation 按从名称向外的顺序记录 Pointer、LvalueReference、RvalueReference、MemberPointer、Array 和 Function。顺序是语义的一部分：`int *a[5]` 为 Array→Pointer，而 `int (*a)[5]` 为 Pointer→Array；`int (*factory())(double)` 为 Function→Pointer→Function。不能用无序字段或单独的 pointerLevel/arraySuffix 替代。

- Pointer 保存指针自身的 cv 限定；MemberPointer 还保存成员所属类。
- Array 保存维度文本，迁移期不要求值。
- Function 保存已经解析的 ParameterInfo 列表和声明解析时按模板形参位置生成的 `normalizedArgs`；StatementBuilder 根据结构化信息生成兼容用 `args` 并复制规范签名，避免函数定义后再用 `scanMethodArgs()` 或对签名重新 Tokenize。ParameterInfo 保存创建参数 Statement 所需的类型、名称、数组或函数指针兼容信息和行号，并以 `defaultArgumentText` 保存默认参数原文。默认原文只用于有限重载候选筛选和单参数构造转换，不参与通用常量求值，也不进入 `normalizedArgs`。
- Function 的 const、volatile 和可变参数属性使用 bit flags；互斥的 `&`/`&&` 使用独立 ReferenceQualifier 枚举。声明解析还保存无规格、无条件 `noexcept` 和条件 `noexcept(expr)` 的类别与表达式原文；StatementBuilder 持久保存并在声明/定义合并时同步。表达式求值读取实际选中调用及其模板绑定；尚未覆盖的路径返回 Unknown，不猜测确定结果。

trailing return type 保存在其左侧最近的 Function derivation 中，以规范化原文记录，不要求值，也不需要进一步解析其中的抽象 declarator。当基础类型为 auto 且存在 trailing return type 时，生成 Statement 类型时用后者替代 auto，再应用其它 derivations。没有 trailing return type 的 auto 函数不做函数体返回类型推导。

第一个 derivation 为 Function 时，名称直接声明函数；第一个为 Pointer/Reference 时，即使后面存在 Function，名称仍是函数指针或函数引用变量。不能按最后一个 derivation 判断 StatementKind。

转换成现有 Statement 时，`command` 使用名称，`qualifier` 用于定位父作用域，声明说明符与有序 derivations 共同生成 TypeInfo。普通函数和函数指针变量都从对应 Function derivation 取得参数；数组变量继续将维度写入 `args`，函数指针变量继续设置 FunctionPointer 属性。

### DeclarationInfo 与 Statement 的边界

DeclarationInfo 是声明产生式的临时值对象，通常保存在栈上，declarators、children 和枚举值等成员也按值保存。它不是长期语义索引，不需要 shared_ptr。Statement 是进入名称查找、代码补全、类型推导和失效清理流程的单个命名实体，由 StatementModel 通过 shared_ptr 唯一持有；StatementMap 是 `command -> weak_ptr<Statement>` 的多重索引，不承担所有权。

CppStatementBuilder 是两者之间唯一的创建层。Parser 每得到一个可提交的 DeclarationInfo，就在继续后续声明前调用创建层，不积累整文件的 DeclarationInfo，也不延迟到代码补全时转换。转换后 DeclarationInfo 随作用域结束释放。解析失败的声明不提交；未闭合但已按文件尾恢复的有效声明仍可提交。

转换不是一对一复制。例如 `int a, *p, f(double value), (*pf)(char);` 只产生一个 DeclarationInfo，但会创建 a、p、f、pf 四个 Statement，并为 f 的参数 value 另建 Parameter Statement。class/namespace 的 children 递归转换为 Statement 作用域树，枚举值也分别创建 Statement。Statement 额外记录 parentScope、fullName、声明与定义文件、跨边界祖先及各类 weak 索引关系。

普通变量、typedef/using 别名、枚举值、参数和标签的声明位置同时就是定义位置，必须填写 definition occurrence 并设置 HasDefinition。extern 变量、无函数体函数以及类/枚举前置声明保持 declaration-only；有函数体或 `=default` 的函数具有 definition。`=delete` 声明保留为 declaration-only Statement，用于名称查找和展示，但不作为可用重载候选，也不提供 definition 跳转。没有有效源码位置的 hard define 不参与跳转。

Label 和 Preprocessor 在表达式名称解析中使用各自独立的结果类别，只提供 Statement 供 goto 等声明导航，不伪装成变量、函数或类型，也不提供表达式类型。gotoDefinition 只接受 HasDefinition 且定义位置有效的 Statement，不得回退到 declaration。gotoDeclaration 优先使用当前文件中的 declaration，其次使用其它 declaration；没有 declaration 时才回退到 definition。

创建层同时把 Statement 写入实际源码文件的 ParsedFileInfo。class、namespace 和函数只以 bodyRange 建立代码作用域，父节点按父 Statement 的身份关联；声明范围仍覆盖完整声明。不同 include 文件的位置和作用域分别写入各自的 ParsedFileInfo，不跨文件比较 SourcePosition。`struct S {} value;` 中类体只属于 S 的定义，不会被当作变量 value 的函数定义。

internalParse 在声明分析前统一收集预处理输出中的 `#define`，再逐个转换为 Preprocessor Statement，记录名称、参数、值、实际文件、token 位置和跨边界祖先，使普通补全与 Macros 补全共享同一失效清理模型。宏是否登记不再取决于外层 namespace/class/function 的声明分发路径。Macro Statement 不是从 ParsedFileInfo::defines() 生成的；Define 只有名称、参数、值和文件等数据，没有行列位置，预处理输出 token 才是声明位置的来源。编译器 hard defines 由 parseHardDefines 转换为同类 Statement，没有源码文件或位置，也参与普通补全与 Macros 补全。函数或 Block 内的用户标签使用独立的 DeclarationKind::Label 和 StatementKind::Label；case/default 标签及三元表达式中的冒号不生成 Label Statement。

声明说明符由独立的 DeclSpecifierInfo 记录。`static`、`extern`、`register`、`thread_local`、`mutable` 等存储类别使用 bit flags。Statement 持久保留必要的声明 flags，`type()` 在生成兼容文本时将其与 TypeInfo 组合；static 继续映射 StatementProperty::Static，extern 继续影响声明/定义判定，register 等即使不参与补全语义也不能在兼容输出中丢失。

初始化表达式、auto 类型推导、Statement 创建及声明/定义合并不属于 DeclaratorInfo 或 DeclSpecifierInfo，由 `parseDeclaration()` 在 declarator 完成后处理。auto 变量声明保存 initializerText 供类型推导；默认参数保存 `defaultArgumentText` 供有限调用筛选。非 auto 初始化和被丢弃的括号表达式只跳过 token，不拼接临时文本。TokenCursor::currentText 返回只读引用，避免声明解析中大量无意义的 QString 引用计数操作。

### 声明产生式与错误恢复

`parseScope()` 分别分发 namespace、class、enum、using/alias、template 和普通声明。普通声明由 `parseDeclSpecifierSeq()`、`parseDeclarator()` 和声明后缀组成；一条声明共享 DeclSpecifierInfo，每个 declarator 分别生成 TypeInfo 和 Statement。namespace/class/enum 保留各自产生式，不强行套入普通 declarator。构造函数识别只收集限定名各段的标识符文本，并跳过配对模板参数；只有这些文本用于判断类内名称是否等于当前类名、类外是否符合 `A::A`。模板参数不参与构造函数识别，避免在每次说明符循环中重复进行类型语义解析。识别路径只配平并推进游标，不生成模板原文；只有真正的类型解析需要保存类型信息时才读取原文。配对括号优先使用 matchIndex 识别；需要比较尖括号等运算符文本时，先判断 TokenKind::Operator，再在同一个分支中比较具体文本，避免对标识符和字面量做无意义比较。

声明入口按关键字角色分类，不能把任意保留字当作用户类型名。语句、控制流、字面量和表达式专用关键字不能开始声明，普通声明解析直接失败并由外层语句扫描跳过，因此 `return value;` 不会生成局部变量 Statement。`alignas`、`_Pragma`、两种 GNU `__attribute` 拼写和 `__extension__` 在 Tokenizer 中跳过其标记及配对区域，保留后续声明；`export` 在 template/声明前缀中消费。当前不生成 concept Statement，concept 定义按不支持语法跳过。`final`、`override` 等上下文标识符只在对应 declarator 后缀处理，不进入全局拒绝集合。

成员函数模板的类外定义可以包含多层连续 `template<...>` 前缀；声明解析逐层消费并按外层到内层顺序累计模板形参，不能把剩余前缀交给普通 declarator。`extern template` 是显式实例化声明而不是模板声明前缀：保留 `extern` 属性并消费这两个 token 后，按后续 class 或函数声明正常分派。

模板参数列表后和函数 declarator 后的 requires 子句在声明阶段只消费并保存原文，不求值；扫描时区分声明起点、函数体 `{` 与嵌套 requires-expression 的 `{ ... }`，包括带参数列表的 requires-expression，确保别名、类、函数、构造函数的声明和后续成员不受约束子句影响。C++20 起的有限 lazy 约束求值见 `cpptype-evaluation-design.md`，不扩展完整 Concepts 或 dependent-name 语义。条件 `explicit(...)` 同样只跳过条件表达式并保留 Explicit 标志。

模板参数试探不得越过当前表达式或语句。分号和 include 文件边界直接使试探失败；非负且小于当前索引的 matchIndex 表示遇到 `)`、`]`、`}` 等闭合符，也立即失败。`(`、`[`、`{` 等开括号则由 matchIndex 大于当前索引的配对分支跳过。这样，`if (value < limit)` 中的比较运算符不会导致 `<...>` 试探扫到后续无关声明。

CppPreprocessor 从 hard define `__cplusplus` 保存去除整数后缀后的版本值，并通过 `cppVersion()` 提供查询；CppParser 的同名公共接口在锁内读取该值。测试依据 Parser 实际取得的版本选择对应标准库场景，不根据请求使用的 `-std=` 参数猜测实际预处理结果。

前瞻和试探只移动 TokenCursor 并创建临时结构。Statement、作用域、namespace occurrences 和继承状态只在声明结构确定后提交；class/function 可在头部确定后创建自身 Statement，再进入正文。模板形参查找层使用作用域保护，失败和提前返回都必须弹出。

函数体内先由 for、catch、if/while、普通 Block 等专用产生式处理对应语句，其余位置通过声明入口的关键字分类和有限前瞻区分声明与表达式，不建立表达式 AST。确认的解析错误可以结束当前文件的错误路径，但不因此终止整个 TU。跨文件配对记录诊断并跳到当前文件尾；include 文件随后返回包含者继续分析。文件内未配对结构不是致命错误，当前文件尾就是其虚拟匹配位置，仍按正常流程分析到文件尾。

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

### CppParser 生产接入

CppParser 的唯一生产路径为 preprocess → CppTokenizer → CppSyntaxParser → CppStatementBuilder。FileBoundary token 在读取后保存新文件名，DeclarationInfo 也携带实际文件；递归创建子声明时使用 child 的文件，include 内容不登记到宿主的 ParsedFileInfo。旧 handler、token 改写和字符串类型手术不保留。

同一实体的声明和定义合并到一个 Statement。所有 Statement 以 StatementOccurrence 集合保存多个 declarations；普通 Statement 的 definition 单独保存当前位置及 HasDefinition，不重复登记为 declaration occurrence。失效只移除对应 occurrence；definition 失效后保持为空，不得用 declaration 回填。只有 declarations 与 definition 均不存在时才删除普通 Statement。声明说明符合并及 namespace 重开的 inline 属性按剩余 occurrences 计算，参数不重复创建。变量统一使用 StatementKind::Variable，global/local/class-local 由 StatementScope 区分。typeInfo.toString 保留完整结构化类型，Statement::type() 生成旧 API 所需的存储类别、返回类型及数组/函数指针兼容文本。

merge 的候选查询使用独立的无锁 doFindXXX 实现。局部声明只枚举当前词法作用域的直接 children，不同 Block 的同名局部声明不合并。非局部声明按实际 fullName 沿 children 和 InlineAlias 查找候选，再按 kind、normalizedArgs、约束和跨边界祖先筛选。命名 namespace 已按 fullName 全局合并，不通过重开节点索引收集多个 namespace Statement。候选不存在时直接返回空，不线性扫描全量 Statement，也不使用单结果查询失败后的兜底。

class/namespace/function 以 bodyRange 建立作用域；for/if/catch 等 Block 以整个控制语句范围建立作用域，header 中的初始化、循环变量和异常参数均可查询。namespace scope 的父索引取该 Statement 自身的 parentScope，不能以 namespace 自身作为父节点；内层闭合后必须能沿父链返回外层。按精确行列查询使用 findScopeAt，按行兼容查询使用 findScopeAtLine，不能用最大列号模拟。findStatementOf 只返回当前 root 及其 include 闭包中可见的 Statement，已移除 include 的全局缓存不会泄漏到该 root；`visibleStatement` 按 declaration/definition 的实际 `fileName` 是否等于当前文件或位于其 include 闭包判断，不以 `boundaryAncestor` 代替实际文件；hard define 等没有源码文件的 Statement 不受文件闭包限制。`statementDeclarationForNavigation` 优先选择实际文件等于 currentFile 的 declaration，其次选择以 currentFile 为 parse-root/失效归属的 declaration，再回退到任意 declaration，最后回退 definition。表达式名称解析与 merge 都沿 children 和 InlineAlias 处理命名 namespace 与可见成员；命名 namespace 按实际 fullName 全局合并。查询目标本身就是 namespace 时，终止 Statement 必须是 fullName 精确相等的实际 NamespaceStatement。inline namespace 保留实际 fullName，通过 InlineAlias 使成员在直接父 namespace 中可见；该规则与 using namespace 的未限定名称查找分开处理。

## Statement 与查询设计

### StatementMap 结构

- **作用域树 + 每层索引的整体结构合理，不动**——C++ 名字查找天然是作用域相对的（scope 内精确匹配、枚举 scope 成员、沿链向上），树-of-maps 正好镜像它。
- **trie 出局**：所有查询（parse 期 + 补全）都是精确匹配或全量枚举，没有前缀查询；补全是模糊打分必须全扫。键短、多数 scope 很小，trie 的节点开销纯浪费。
- **per-scope 容器从 QMultiMap 换 hash 是"有正确性依据的优化"**（见下第 1 条），但仍建议等基准数据再动手。换之前须验证一个依赖：QMultiMap 迭代按名有序，补全列表初始按字母分组；换 hash 后顺序随机，需确认补全排序比较器（codecompletionpopup.cpp）在同分时有确定性 tie-break。
- 键冗余（map key 与 `statement->command` 双份存储）随容器改造一并消除。

### Statement 所有权

StatementModel 是 Statement 的唯一所有权中心，在生产配置中保存全量 Statement 列表。全局索引、children、作用域索引和 ParsedFileInfo::statements 都只保存 weak_ptr；TypeInfo 的声明引用也遵守同一规则。namespace A::B::C 按真实嵌套关系建立。命名 namespace 按实际 fullName 全局合并，inline/匿名可见性通过 InlineAliasStatement 表达，不另维护 namespace 专用 fullName 索引。

查询时将 weak 引用临时 lock 为 PStatement，保证本次操作期间对象存活。新增、声明属性合并、失效删除和 clear 都必须同步维护 children、occurrences 和 InlineAlias。weak_ptr 只避免所有权环，不能代替索引清理。Statement 的释放不再取决于哪个文件记录最后引用它。

### 重开 namespace 的限定名查找

命名 namespace 重开只增加当前 occurrence，并把新成员加入同一个 NamespaceStatement。class/enum 的声明与定义也按各自规则合并；函数重载保留不同候选，Block 只是局部作用域占位。

名称查找从当前词法作用域开始，沿 children 和 InlineAlias 逐段处理。最近词法作用域的首段匹配优先，不与外层同名 namespace 混合。进入 class/enum 后沿用普通成员和基类查找。函数、lambda 和 Block 不作为全局限定名的中间作用域，`f::local`、`block::inner` 等伪路径返回空，但其内部局部名称仍可从对应 scope 开始查找。

声明和成员查询在 EvaluationContext 中按作用域身份、完整名称和 typesOnly 复用命中与 miss；只允许模型不变的查询期使用，构建期禁用，避免旧 miss 泄漏到新增声明之后。

测试覆盖顶层和嵌套重开、不同构建顺序、inline/匿名投影、late inline、部分失效、全局 `::`、最近词法作用域优先、局部 shadowing 和函数/Block 的局部边界。同一 fullName 的对象身份及各来源 occurrence 都必须验证。

### 继承成员查找

继承成员默认采用按需递归查找，不复制基类 children。derived 的直接成员优先，随后递归补充基类成员；override 按函数签名去重，inline/匿名 namespace 作为透明作用域处理。class 补全通过 `resolveMembersOfClass(classStatement, accessScope)` 获取从访问域可见的临时成员列表，继承访问级别按路径计算；非 class 作用域直接读取持久 children。Class Browser 只显示直接 children，不做继承投影。

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

声明上下文遇到 `<` 时须先解析前面的名字并查它是不是模板。parse 路径本就有大量精确名字查询，模板判定新增同类查询，per-scope 容器换 hash 因此从优化升级为设计支撑。跳过函数体时无需配平 `<>`；声明初始化器和默认参数的边界扫描需要识别标识符后的可配平模板实参区域，避免把模板实参中的逗号误当作 declarator 或参数分隔符。无法配平时按普通运算符处理，查不到模板声明时回退到结构配平。

Statement 需要区分非模板、主模板、部分特化和全特化，并保存模板形参名称、类别及 parameter-pack 标记，供模板绑定使用。特化实参保留在 Statement 的 `command/fullName` 完整特化名中，不另设重复字段；eval 需要匹配时从该名称现场解析，不与声明形参混用。默认模板实参只保存原文，不在声明阶段实例化；约束表达式只正确消费。类型和模板模板参数的默认实参按 `<...>` 层级读取，内层 `>`/`>>` 逐层消费，外层逗号和闭合符留给参数列表；非类型默认值仍按表达式处理，不把比较运算符 `<` 当作模板起点。`()[]{}` 在各默认值中均保持配平，覆盖嵌套模板、连续默认值、比较表达式和带逗号函数调用。模板求值的严格边界、本轮必要特性及 STL 依据见 `cpptype-evaluation-design.md`。

类型推导采用有限通用模板绑定优先：将形参名称绑定到实际 TypeInfo 或常量表达式，在类成员、别名、嵌套类型和简单函数模板返回值中做替换。模板声明内部使用 Session 级模板形参查找层，避免形参泄漏到声明之外。先用自定义 `Box<std::string>` 等类型验证通用性，再接入 STL，不能把 STL 名称识别冒充通用实现。

Parser 解析声明时只保存模板声明、模板实参原文和 TypeInfo 的其它结构，不实例化模板，也不生成实例化 Statement。TypeInfo 以 `qualifiedNameText` 保存包含各段尖括号实参的完整限定名原文。完整限定名、模板实参、默认实参、特化模式和基类等求值输入优先从预处理输出的连续原文按起止位置直接截取，避免不必要的逐 token 拼接；需要跳过中间内容、合并非连续片段或做明确要求的规范化时可以按需拼接。直接截取前校验起止 token 与缓冲区文本一致，预处理行映射不一致时回退到 token 规范化拼接。跨行时用空格分隔，截取后立即保存 QString，不保留位置、TokenList 或源缓冲区引用。用于 fullName 比较和索引的规范化名字是另一用途，不得据此为可连续截取的模板求值原文增加拼接路径。只有类型求值、模板绑定或类型操作确实需要实参结构时，才重新 tokenize 并生成有序 `TemplateArgument`。代码分析阶段不填、只有 expression eval 才填的属性均为一次型数据，不得回写 Statement 或其持久 TypeInfo；结构化实参、绑定、特化选择、基类环境和缓存都由求值上下文持有。为区分 `<` 是否模板参数而进行的名称查询不属于实例化。类模板绑定、别名展开、特化选择、成员类型和函数返回类型均在查找类型信息时按需计算。`auto x = expression` 在 StatementBuilder 阶段立即求初始化表达式并保存最终 TypeInfo，初始化表达式和求值过程不持久保存。普通 auto 函数的返回类型仍留待后续专门能力处理，不做函数体推导。

当前不支持 CTAD。deduction guide 的语法必须正确消费，但不生成 Statement：识别依据是函数 declarator 带 trailing return 且没有 decl-specifier 返回类型，不能误忽略普通 `auto f() -> T`。测试必须验证类定义和成员可查找、guide 不产生同名函数条目，以及普通 trailing-return 函数仍保留。

未来若实现 CTAD，可在 Class Statement 内增加 `guides` 值列表，联合 class 构造函数和 guides 进行推导。guides 是与类关联的语义信息，不进入 children、fullName 或普通函数查找，也不作为独立 shared_ptr Statement 持有；每项保存模板参数、函数参数、推导结果类型、源码文件和位置。文件失效时按各自文件/边界祖先清理，Class 删除时随 Class 一并释放。该列表仅是后续可选设计，当前不实现。

函数 Statement 同时保留结构化 ParameterInfo 列表，不只保存 args 文本。调用时逐参数比较 formal/actual TypeInfo：直接模板形参绑定实际类型，引用先去除，指针/数组按层递归，Container<T> 等实际模板参数继续递归绑定；返回 TypeInfo 再应用这些绑定。该机制用于 find_if、transform 等算法，不做 SFINAE、参数包展开或完整隐式转换序列。

成员调用先递归求出实参类型，再收集当前类及依赖基类的同名候选，并按实际声明类建立模板绑定。const 用于筛选成员；当前变量成员访问按左值处理，排除仅 `&&` 限定的候选，不扩展为通用右值重载决议。非模板候选组优先于函数模板候选组；每组必须只有一个可行候选才返回，多个可行候选按歧义处理，不能取第一个或靠跳过失败候选伪造成功。类型结构相同且可行时，额外允许一种有限用户定义转换：目标形参为类类型、实参为指针式类型，且目标类存在可接受该实参的单参数非 explicit 构造函数；构造函数后续参数必须都有默认值。除此之外不实现隐式转换序列排名。

无法仅从声明签名得到的规则通过可插拔的 TypeOperationRegistry 添加，例如算法回调的调用参数、range adaptor 的元素转换和 tuple/range 类型操作。专用规则按标准库操作集中注册，不散落成各成员函数的硬编码。通用机制过于复杂的局部能力可以先由专用规则实现，但对外使用同一类型操作接口。

rangeElement 统一计算 rangeValue/rangeReference：iota_view 使用数值类型；filter_view 递归保持输入 range 的元素或引用；transform_view 使用函数、函数指针或 operator() 的返回类型；join_view 连续两次取元素以展平内层 range；elements_view 从 tuple、pair 或 array 的实际模板实参中取第 N 项。查不到专用规则时回退到普通容器的 value_type/front，不把 view 名称识别代替通用声明解析。

函数对象调用的实参也按完整表达式求值，逗号只分隔实参。TypeOperationRegistry 将 filter/transform 调用转换为带实际回调 TypeInfo 的闭包；operator| 再把输入 range 与闭包合成为 filter_view/transform_view，join adaptor 直接合成为 join_view。显式非类型模板实参仅在 Registry 确认目标支持时消费，否则 `<` 仍按比较运算处理；views::elements<N> 因此可以合成为带 N 的 elements_view。管道类型可以传给 auto 和 range-for，最终成员访问继续使用同一表达式求值路径。

初始化表达式重新切分时使用 relocateTokens 应用原始文件、起始 line/ch 和首行列偏移。lambda 是实际声明而非模板实例：声明解析直接在原 token 流中保存参数、body 的结构化 DeclarationInfo 和顶层 return 表达式 token，不重新拼接或切分 body；Builder 在 auto 变量类型推导前建立 Lambda Statement、operator()、参数、body 内 statement 和源码位置作用域。auto 求值按稳定名称复用该 Lambda Statement。显式 trailing return 优先；否则在 operator() 上下文对保留的 return token 复用完整求值器，无 return 时为 void。有限推导覆盖具名类型参数、参数成员调用和算术返回表达式，复杂控制流、多返回合并及 generic lambda 的 dependent 参数绑定仍属于后续能力。

类型求值的各级缓存由 EvaluationContext::caches 独立控制，默认全部启用。完整类型结果只在模型不变时缓存成功解析；键包含类型、声明/context 指针以及逐帧指针和修订号，绑定链变化后旧结果自然失效。帧内 resolvedValues 作为模板参数快路径独立保留。对照 profile 可通过 REDPANDA_PARSER_DISABLE_CACHES 的逗号分隔列表关闭 all、type-text、type-name、type-pattern、binding-frame、resolved-value、specialization、declaration、member 或 resolved-type。

绑定帧可保存参数包并在模板实参列表中展开；无法求值的非类型实参保留原文，不因缺少布局/求值信息而丢失别名链。完整模板实例化、完整 SFINAE、完整 concepts 求值和完整 dependent-name 语义仍不在范围内；不生成整套实例化 Statement 副本。auto 函数不做函数体返回类型推导，transform 所需的 lambda 返回表达式推导是独立的有限功能。

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

求值器保持旧实现的递归表达式语义，不建立表达式 AST，也不在解析其它声明时预先计算。二元运算按 comma、赋值、条件、逻辑、位运算、比较、移位、加减、乘除和成员指针的优先级递归；括号内调用完整子表达式，类型转换与普通分组分别处理。`new`、一元运算、字面量、类型/`::` 名称、函数调用、初始化、`[]`、`.`、`->` 和前后缀自增减都进入同一求值路径。下标内容利用 matchIndex 跳过；成员调用的各实参分别递归求值，逗号只作为实参分隔符。

类型解析先尝试相应的运算符重载，再应用内建指针、引用和算术规则。`->` 递归处理内建指针或 operator->，解引用处理内建指针或 operator*，自增减使用 operator++/operator-- 的返回类型。每一步只保留下一步所需的 TypeInfo，因此 `container.front().size()` 和 `(*it++)->size()` 等链条可以逐段计算。查不到声明或尚不支持的表达式返回无效 EvalResult，不把部分前缀当作完整结果。无调用括号的成员函数访问是函数引用，即使模板返回类型尚不能推导，只要成员 Statement 已找到，表达式结果仍有效；实际调用仍必须成功解析返回类型。

Resolver 接口按“返回什么、执行什么动作”区分：

- `resolve()` 只返回解析后的 TypeInfo；`resolveInstance()` 同时返回声明和绑定环境；`resolveInContext()` 是显式提供现有绑定环境与词法上下文的同一解析入口，不另建一套规则。
- `memberStatement()` 只查找成员声明；`memberType()` 进一步求成员类型；`memberAccess()` 返回完整成员访问结果，但不进行调用重载决议。
- `invokeMember()` 处理带实参的成员调用并执行候选匹配；`invokeFunction()` 处理普通函数及显式模板实参。`invokeResult()` 用于不需要候选匹配的已知成员运算符等路径，只求直接找到的可调用成员的返回类型；需要参数匹配的调用不得用它替代 `invokeMember()`。
- `findMemberDeclaration()` 对未绑定 Statement 做成员声明及基类查找；面向 ClassInstance 的 `findMember()` 则在已绑定类环境中查找成员并保留其 owner。二者不得仅因最终都返回成员而混用。
- `classInstanceFromResolvedType()` 接收已解析类型，只负责建立类绑定并选择偏特化；`selectSpecialization()` 只负责候选匹配。`resolveSegments()` 按限定名的有序分段推进声明、别名和类实例，不再存在一个仅转发它的 `classInstance()` 接口。
- TemplateBindingFrame 表示一层词法模板绑定环境，而非调用栈帧。`parent` 连接外层模板环境；namespace 中的声明不得捕获调用方绑定。`declarationBindings()` 建立这一环境，默认实参求值属于绑定过程，不属于名称查找。

配套设计点：

- **解析顺序保证信息可用**：头文件先于包含者解析、文件内声明按出现顺序入表，遇到 `<` 时前序模板声明已在表中，同 phase 查找可行（与 clang 的 Parse/Sema 交织同理）。
- **查不到的兜底策略**：代码敲到一半、include 未解析、dependent name 都会查不到。定为"查找优先，查不到回退结构启发式（配平检测）"；宁可多合并不可漏合并——漏合并会让模板代码解析直接散架，误合并只是表达式里多个废 token。

## 主文件解析与项目引用查找边界

CppParser 的正确性承诺以当前主文件为单位：每次解析后，当前主文件及其 include 闭包中的语句、可见性、作用域和表达式分析结果必须准确。`visibleStatement()` 继续按 declaration/definition 的实际文件及当前主文件的 include 闭包判断，不改为 FileInfo 的 statement 索引，也不承担跨 TU 的 statement 合并或项目符号管理。

项目级 Find Occurrences/Refactor 由调用方逐个将 TU 主文件交给 Parser 重新解析并查找；Parser 不提供文件列表解析入口或项目符号表，也不要求不同 TU 共享 Statement 对象。只有查找从头文件发起时，才额外把该头文件作为主文件解析一次以确定目标；之后仍逐个解析项目 TU。项目查找覆盖当前各 TU 的 include 闭包，不搜索未被任何 TU 包含的其他头文件，也不把注释或字符串中的同名文本当作语义引用。同一头文件经多个 TU 命中相同位置时，结果按文件及 0 起始 line/ch 去重。

跨 TU 或重解析后 Statement 对象可能重建，`PStatement` 指针只在同一冻结快照内有效。每个 TU 中都必须重新识别目标，并按 kind、完整 scope/name、链接方式、函数签名、模板信息及关联符号关系判断语义等价。外部链接符号可跨 TU 匹配；全局 static 和匿名 namespace 符号增加 TU 边界；普通局部符号不跨 TU；局部 extern 声明使用外部声明上下文并与外部 statement 合并。匿名类型使用定义 occurrence，不使用自动生成的名称作为跨解析身份。

`StatementOccurrence::declSpecifierProperties` 保存各 declaration/definition 的 static/extern 等来源属性。合并可以累计这些属性；失效后必须根据剩余 definition 和 declarations 重算，删除 definition 不得凭空制造 extern，删除最后一个 extern declaration 后也不得保留过期标志。extern 变量只有无初始化器时才仅声明；类内 static 数据成员无初始化器且非 inline 时仅声明。

`TestCppParser` 负责主文件、include 可见性、static/extern、合并顺序及局部 extern 的最小测试，`TestCppParserInvalidation` 负责 occurrence 失效和属性重算，`TestCppExpressionEvaluator` 负责表达式求值，`test-refactor` 负责逐 TU 查找和重命名的端到端行为。测试夹具只建立项目、文件与内容，不代替被测组件匹配名称或查找引用。同一缺陷可以同时保留 Parser 最小根因测试和 Refactor 行为测试，但两层断言不得混合职责。多 TU 同名类、重载、scope、链接方式及项目边界均须有正向和反向场景。

重载选择结果保存在 `EvalResult::invocation.statement`；需要导航到实际调用目标时必须优先使用该结果，而不是最初的同名候选。普通类模板的显式实参应进入既有特化选择路径，并区分主模板、偏特化和显式特化。

### 普通表达式中的 Lambda

函数体普通表达式不建立完整 AST，但需要保留其中 lambda 的导航作用域。例如 `win.run([&](Ui &ui) { Stack col; });` 中的形参和函数体局部声明不能只随表达式一起跳过。

- `DeclarationKind::Expression` 区分表达式语句，不是新的 StatementKind，也不进入名称索引。
- `DeclarationInfo::expressionLambdas` 保存表达式扫描发现的 LambdaInfo。
- 表达式扫描复用 skipExpression() 和 parseLambda()，以分号及当前 Block 的闭合符为边界，并递归处理嵌套 lambda。
- CppStatementBuilder 遇到 Expression 时只构建 lambda，不创建表达式 Statement。
- 初始化 declarator 与普通表达式共用 buildLambdas()，Lambda Statement、operator()、参数、bodyRange 和作用域查询不另建第二套逻辑。

该建模只承担 lambda 作用域和导航，不保存普通表达式文本，不提前求值调用接收者，也不扩展为通用控制头表达式建模。

测试同时覆盖函数实参 lambda、嵌套 lambda、形参类型、局部声明和成员导航；真实 Aino 场景只随标准库专项开关运行。

## 性能测量

设置 `REDPANDA_PARSER_PROFILE=1` 后，`EVAL_PROFILE` 记录每次表达式求值耗时，`RESOLVER_PROFILE` 汇总类型解析、限定名分段、类实例建立、特化选择、成员/声明查找、声明绑定、模板实参结构化和类型键文本生成的次数和耗时。Resolver 各阶段会嵌套调用，汇总值均为包含子调用的耗时，不能直接相加当作总时间。标准库测试中的 `STDLIB_TEST_TIMING` 用于单独测量指定查询，`STDLIB_CONTAINER_EVAL` 记录各容器表达式求值耗时。

`DECLARATION_PROFILE` 保留包含子调用的声明解析耗时，`DECLARATION_SELF` 额外输出 self-time。它们由 CMake 选项 `REDPANDA_PARSER_DECLARATION_PROFILE` 控制，默认 OFF；普通构建使用空 scope 且移除相关计数，不引入逐声明插桩成本。需要这些输出时，配置 `-DREDPANDA_PARSER_DECLARATION_PROFILE=ON`，运行时再设置 `REDPANDA_PARSER_PROFILE=1`。整体 `PARSE_TIMING` 及各阶段总耗时不依赖此开关，普通 Release 仍可测量。

self-time 由显式 scope 栈计算：每层只累计直接子 scope 的耗时，当前 scope 从自身总耗时中扣除；出栈顺序必须匹配。不能用全局累计的嵌套耗时相减，否则不同调用链交叠时会产生无效负值。

优化前后的正式对比必须直接运行 Release 可执行文件并关闭详细 profile，不能在 gdb 下计时。测量标准库表达式时保留完整 parse，但以 parse 完成后的单次查询耗时作为 eval 基线，避免用端到端总耗时替代求值成本。需要单独分析某个容器表达式时，可用 `REDPANDA_STDLIB_CONTAINER_CASE=unordered_map.find_second` 这样的用例名过滤 `test_standard_library_container_eval_profile`；`std::function` 专项可类似使用 `REDPANDA_STDLIB_FUNCTION_CASE` 并按表达式文本过滤；不设置时运行全部用例。

正式性能基线和优化结果统一由 `docs/profile/README.md` 索引，核心设计文档不复制逐轮数字。测量方法保持不变：Release 直接运行、关闭详细 profile、区分 inclusive/self-time，并同时检查 Statement 和求值正确性。

## 语法覆盖测试

语法分类、支持状态、限制和测试证据统一维护在 `testing/syntax-coverage.md`。测试资源位于 `RedPandaIDE/test/resources/syntax/`，默认测试不依赖外部标准库文件。

新增语法场景必须同时更新资源、正式断言和覆盖表；不得把已经修复的场景继续列为当前缺口。

### 函数局部预定义标识符 `__func__`

`__func__` 按函数局部预定义变量处理，而不是全局宏。Builder 只为具有函数体的 Statement 添加 `function::__func__`，其类型为 `const char[]`，值为未限定的函数名；普通声明没有函数体时不创建。该 Statement 复用当前函数的 definition occurrence 作为定义位置和 parse-root 归属，并随所属 context 失效。补全、局部名称查找、类型求值和 Parser Hint 复用普通局部变量路径，Hint 同时显示函数名字符串。

## IDE 查询与重命名约定

`getIncludedFiles()` 只返回 include 依赖文件，不包含关系起点。补全所需的是“当前文件可见的源码文件集合”，由 `CodeCompletionPopup::prepareSearch()` 在依赖集合上另外加入当前文件，不能改变 Parser 接口的依赖语义。namespace 补全读取唯一实体的直接 children，并解析其中的 InlineAlias；命名 namespace 重开不会产生另一组需要单独收集的节点。

普通补全不展示 OverloadedOperator 或 LiteralOperator，也不展示 command 为空的匿名 namespace 等无效候选项。现有 LiteralOperators 专用补全在字符串、字符和数字后缀输入语境中单独提供候选，不经普通补全的过滤器，不能因专用能力而让重载运算符泄漏到普通列表。变量及图标类别按 StatementScope 的 global/local/class-local 判定，不能用是否存在 parentScope 区分全局变量与类成员，否则 namespace 变量会被误标为 private。

`gotoDeclaration`/`gotoDefinition` 只要求鼠标位于标识符上时正确跳转，目标位置使用 occurrence 的 `range.start.line` 和 `range.start.ch`，不能只跳到行首。函数提示分两条路径：输入调用参数时，光标位于 `()` 内展示被调用函数的声明；`getParserHint` 用于鼠标悬停在函数标识符上时显示声明。类名与构造函数同名时，类声明位置必须选择 Class，构造函数调用位置仍按实际语义选择 Constructor 或 Class。

重命名按 syntax token 和语义 Statement 身份工作，只处理标识符，不按文本相同替换。非项目文件的 Rename 仍限当前文件；项目中的 Rename 可以跨文件，但相关目标的声明、定义和引用不得超出项目允许边界。项目查找由 CppRefactor 逐 TU 重新识别目标并累积结果，open Editor 的内容和 Undo/Redo 按当前 Editor 接口处理。`getWordForCompletionSearch` 只面向补全光标前文本，不能作为通用标识符提取 API。

class 补全使用临时继承成员列表，derived 直接成员优先，基类递归补充；继承访问级别按路径计算，不修改原 Statement。Class Browser 只从当前文件的 FileInfo 收集 statements 及其祖先，再沿直接 children 生成树；即使文件属于项目，也不收集其它项目 FileInfo。祖先节点仅用于组织当前集合中的声明和定义，不引入其它文件成员。widgets 测试使用独立 `test-widgets` 目标，不依赖 Editor 测试类，覆盖所有 `CodeCompletionType`、继承访问、类浏览器、goto、function tips 和 rename。goto 测试覆盖变量、typedef/using/namespace 别名、枚举值、参数、标签、宏、普通及 `=default` 函数，以及 extern、前置声明和 `=delete` 等无定义的拒绝路径。
