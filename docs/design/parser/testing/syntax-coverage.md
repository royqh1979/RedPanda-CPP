# C/C++ 语法覆盖与支持状态

## 1. 目的与判定标准

本表描述 Parser 对代表性 C/C++ 语法的当前覆盖，不等同于完整编译器实现，也不证明同一类别的所有组合均已支持。

状态含义：

- **支持**：代表性语法可解析，关键 Statement/作用域或位置有正式断言。
- **部分支持**：只支持列举形式，或语法可消费但语义、求值、导航能力有限。
- **不支持**：当前明确不建立目标语义或不提供该能力。
- **未覆盖**：没有足够测试作出支持判断；不能据此推断实现一定不支持。

主要测试入口：

- `TestCppDeclarationParser`：声明级组件测试。
- `TestCppParserSyntax`：仓库内 cppreference 代表性资源。
- `TestCppParser` / `TestCppStatementBuilder`：Statement、作用域和合并。
- `TestCppTypeResolver` / `TestCppExpressionEvaluator`：类型与表达式语义。

## 2. 基本声明与函数

| 语法形式 | 状态 | 覆盖与边界 | 主要证据 |
| --- | --- | --- | --- |
| 普通变量、函数声明和定义 | 支持 | 全局、namespace、类成员和局部声明 | `test_cppreference_function_syntax`、`test_cppreference_class_syntax` |
| 同一声明的多个 declarator | 支持 | 包括成员函数及各自函数后缀 | `test_parse_member_function_declarators`、`declarators.cpp` |
| 指针、左值/右值引用 | 支持 | 保留有序 derivation 和 cv/ref | Declarator 组件测试 |
| 数组、函数指针和复杂嵌套 declarator | 支持 | 包括返回函数指针和多维/参数嵌套 | `declarators.cpp`、`functions.cpp` |
| trailing return type | 支持 | 包括返回函数指针；不因此进行通用函数体推导 | `fpif`、`fp11` 断言 |
| 默认参数、无名参数和可变参数 | 支持 | 保存默认原文；不参与 normalizedArgs | `complex_defaults`、`unnamed_parameters`、`printf_like` |
| `noexcept` / `noexcept(expr)` | 部分支持 | 声明保存和选中调用求值已覆盖；部分构造函数、成员指针等路径仍返回 Unknown | `cpptype-evaluation-design.md` 的异常规格边界 |
| `static`、`extern`、`register`、`thread_local`、`mutable` 等说明符 | 部分支持 | 保存声明属性并参与定义/合并；不同语境的语义分别测试 | `TestCppParserInvalidation`、static/extern 测试 |
| `[[...]]`、GNU attribute、`alignas` | 部分支持 | 正确消费并保留后续声明；不建立属性语义模型 | Tokenizer/DeclarationParser 测试 |
| 函数 `=default` / `=delete` | 支持 | 区分定义存在性；deleted 不作为可用重载候选 | 函数语法与 goto/重载测试 |
| 函数 try block 和 catch 参数 | 支持 | 保存函数定义、作用域和异常参数 | `functions.cpp` 中 `f2` |

## 3. 类、成员与继承

| 语法形式 | 状态 | 覆盖与边界 | 主要证据 |
| --- | --- | --- | --- |
| class/struct、嵌套类型和成员声明 | 支持 | 包括声明中同时定义嵌套类型和变量 | `classes.cpp` |
| 构造函数、析构函数和成员运算符 | 支持 | 包括类内外定义、构造初始化和 `operator()` | `test_parse_operator_declaration`、类语法及导航测试 |
| public/protected/private 和虚函数 | 支持 | 访问控制、override 和继承可见性分别验证 | `TestWidgets`、Resolver 成员测试 |
| 单级/多级继承和 using 引入基类成员 | 支持 | 按需查找，不复制基类 children | `classes.cpp`、继承成员测试 |
| 成员函数模板和成员类模板 | 支持 | 包括类内外模板前缀和默认实参 | `MemberTemplates::f`、模板组件测试 |
| bit-field 及带默认初始化的 bit-field | 支持 | 声明结构可解析 | `structured_bindings.cpp` 的 `BitFields` |
| union | 部分支持 | 已有匿名 union 的 Class Browser 场景；命名 union、成员和导航覆盖不足 | `test_widgets.cpp` 的匿名类型场景；待补完整测试 |
| friend 声明 | 部分支持 | 已有友元运算符参与 Eval 的场景；类/函数友元的声明和可见性覆盖不足 | `test_cpptyperesolver.cpp` 的友元 `operator+`；待补完整测试 |

## 4. enum 与 namespace

| 语法形式 | 状态 | 覆盖与边界 | 主要证据 |
| --- | --- | --- | --- |
| 普通/scoped enum、底层类型和显式枚举值 | 支持 | 包括枚举值递增和有限整数表达式 | `enums.cpp` |
| 匿名 enum、类/namespace 内 enum | 支持 | 外层与 enum type 中的作用域分别验证 | `E98`、`N98` 等场景 |
| `using enum` | 部分支持 | 语法及当前可见性场景已覆盖；不宣称所有组合完整 | `cxx20::using_enum_demo` |
| 命名 namespace、重开和嵌套简写 | 支持 | 同一实际 fullName 全局合并，保留 occurrences | `namespaces.cpp`、跨 root namespace 测试 |
| inline/匿名 namespace | 支持 | 实际路径不变，InlineAlias 表达直接父级可见性 | namespace/Resolver/Widgets 测试 |
| namespace alias 和 using namespace | 部分支持 | 已有代表性测试；与 InlineAlias、局部 using 的组合需分别验证 | `TestCppParser` / `TestCppParserSyntax` |
| namespace 成员的类外定义及局部 extern | 支持 | 包括 `Q::V::f`、`Q::V::C::m` 和函数体内 `extern h` | `test_cppreference_namespace_syntax` |

## 5. 模板

| 语法形式 | 状态 | 覆盖与边界 | 主要证据 |
| --- | --- | --- | --- |
| class/function/alias 模板 | 支持 | 声明、Statement 和有限类型替换 | `templates.cpp`、模板组件测试 |
| 模板默认实参、非类型和模板模板参数 | 部分支持 | 声明结构已覆盖；求值按实际场景有限支持 | DeclarationParser/Resolver 测试 |
| 主模板、偏特化和显式特化 | 部分支持 | 代表性匹配可用；不宣称完整特化排序 | `TestCppTypeResolver`、真实 STL 测试 |
| 成员模板和多层类外模板前缀 | 支持 | 外围类与函数自身形参分别处理 | Parse-time merge 及模板测试 |
| parameter pack 语法 | 支持 | 模板形参和函数参数可解析、保存包标记 | `variadic_template`、DeclarationParser 测试 |
| parameter pack 求值与展开 | 部分支持 | 只支持当前明确验收的绑定/展开；复杂依赖包返回类型不属于完整支持 | `cpptype-evaluation-design.md` 的范围限制 |
| `extern template` | 部分支持 | 按显式实例化声明消费并保留 extern；实例化语义不展开 | `test_extern_template_class_declaration`、真实标准库场景 |
| SFINAE、通用 detection idiom | 不支持 | 仅支持明确验收的有限形式，不实现通用替换失败规则 | `cpptype-evaluation-design.md` |
| CTAD / deduction guide 求值 | 不支持 | guide 语法可消费但不生成 Statement，也不进行 CTAD | `cppparser-design.md` 的 CTAD 边界 |

## 6. Lambda、语句与控制流

| 语法形式 | 状态 | 覆盖与边界 | 主要证据 |
| --- | --- | --- | --- |
| 普通捕获、引用捕获和 init-capture | 支持 | 包括默认参数和引用捕获 | `lambdas.cpp` |
| generic lambda、显式模板参数和递归调用 | 部分支持 | 声明与代表性求值可用；dependent 参数等仍有边界 | `generic`、`nth_fibonacci` |
| trailing return 和有限返回类型推导 | 部分支持 | 显式返回优先；复杂控制流、多返回合并不完整支持 | Lambda/Eval 测试 |
| 函数实参及普通表达式中的嵌套 lambda | 支持 | 建立形参、局部声明、bodyRange 和导航作用域 | `test_lambda_function_argument_scope`、`test_nested_lambda_function_argument_scope` |
| 普通 Block、声明与表达式语句 | 支持 | 不建立表达式 AST；声明和表达式按上下文区分 | `TestCppParser` |
| if/switch/for/while/do、跳转和标签 | 部分支持 | 代表性控制流和标签已覆盖；所有声明语境组合不视为完整 | Parser/Editor 测试 |
| range-for | 支持 | 容器、数组、引用、属性、init-statement、typedef/using | `test_cppreference_range_for_syntax` |
| try/catch 和函数 try block | 支持 | catch 有独立局部 Block 和异常参数 | `functions.cpp`、作用域测试 |
| 结构化绑定声明 | 部分支持 | 声明语法和绑定名可见性已覆盖；绑定名仍主要是 auto 类型，三类完整推导未实现 | `test_cppreference_structured_binding_syntax` |

## 7. 有限 C++20 语法与 range/view

这些能力不属于“完整 C++20 支持”，只记录已经验证的局部场景。

| 语法形式 | 状态 | 覆盖与边界 |
| --- | --- | --- |
| requires 子句和 requires-expression 的结构扫描 | 部分支持 | 声明期保存/消费，求值仅覆盖有限约束；不实现完整 Concepts |
| concept 定义 | 不支持 | 当前不生成 concept Statement |
| iota/filter/transform/join 的代表性管道 | 部分支持 | 只保证仓库内资源和 TypeOperationRegistry 已验收路径 |
| `views::elements<N>` 等其它 view | 部分支持 | 仅覆盖明确注册和测试的操作，不从名称推导任意 view |
| modules、协程等其它 C++20 文法 | 未覆盖 | 不属于当前代表性语法验收范围，不作支持承诺 |

## 8. 使用与更新规则

1. 新增语法缺陷时，先加入最小资源和 Statement/作用域断言，再更新对应行。
2. “支持”必须能指出实际测试；没有测试时标记“未覆盖”，不凭实现分支名称推测。
3. 语法可消费但类型、补全或导航受限时标“部分支持”，并写明限制。
4. 本表不作为完整 C++ 标准支持声明，也不把标准库头文件 Parse 成功等同于语法全覆盖。
5. D06 的旧“当前覆盖缺口”清单已由本表取代；后续新增或修复场景直接更新本表和对应测试。
