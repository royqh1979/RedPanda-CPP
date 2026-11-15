# C++ 模板表达式求值设计

## 目标与状态

本文是模板类型求值的专项设计，目标是让常用 STL 容器、迭代器、智能指针和传统 algorithm 的表达式能够用于代码补全、声明/定义跳转、ParserHint 和函数参数提示。

当前首要问题是：真实 libstdc++ 中 `std::vector<std::string>::operator[]` 的返回类型经过多层模板别名后停留在 `typename _Alloc_traits::reference`，没有展开为 `std::string &`。此前按 STL 名称硬编码返回类型的方案已撤回，不再采用。

本文约束本轮实现范围，防止在排错过程中临时扩大为完整 C++ 模板编译器。**range/view 明确不在本轮范围内。**

## 不可突破的边界

### 只在表达式求值时实例化

模板实例化只能由表达式求值触发，包括：

- `evalExpression()` 及其内部的类型解析；
- `auto x = expression` 的初始化表达式求值；
- range-for 的 range 表达式求值；
- 补全、goto、ParserHint、function tips 对上述表达式求值入口的调用。

声明解析阶段只做以下工作：

- 建立模板声明、模板形参和普通 Statement；
- 保存实际模板实参原文；
- 保存默认模板实参原文；
- 正确消费模板语法，不计算其语义。

声明解析阶段不得：

- 将默认模板实参提前实例化为声明；
- 为 `vector<string>` 等实例生成新的 class/成员 Statement；
- 把实例化结果写入 StatementModel；
- 为可能的后续补全预先展开所有模板。

`auto` 变量创建时虽然发生在 StatementBuilder 中，但其类型来源仍然是初始化表达式求值，不属于声明阶段的额外实例化通道。

### “实例化”的有限含义

本文的实例化仅指在求值上下文中生成临时的 `TypeInfo`、模板绑定和声明引用。它不是完整的 C++ 编译器实例化，不生成函数体 AST，不生成一套实例化 Statement，也不持久保存整棵实例化类型树。

第一版优先保证正确性。若以后需要缓存，只能缓存求值上下文中的临时结果，且必须先证明键包含声明身份、全部实参和上下文；不得把 Session 共享的实例化 Statement 作为缓存方案。

### 保留真实名称与作用域

`fullName` 保留实际完整路径，`parentScope` 保留真实嵌套关系。`std::__cxx11::basic_string` 不压缩为 `std::basic_string`。inline/匿名 namespace 的对外可见性由现有透明 namespace 索引和查找逻辑处理，不能通过改名实现。

STL 名称不得作为容器、智能指针或 iterator 返回类型的替代解析规则。现有 range/view 专用操作暂时保留，但不在本轮扩展或验收。

## 本轮必须支持的模板特性

每项都列出最低实现要求和真实 STL 依据。只有能够证明目标场景需要的能力才纳入本轮。

### 1. 显式实参绑定与默认实参的按需补齐

最低要求：

- 将类模板实参与形参按位置绑定；
- 缺少实参时，从声明中保存的默认实参原文生成绑定；
- 默认实参可以引用前面的形参，按形参顺序依次求值；
- 同时支持类型实参和非类型实参。

STL 依据：

- `template<typename _Tp, typename _Alloc = std::allocator<_Tp>> class vector`：只写 `vector<string>` 时仍必须得到 `_Alloc = allocator<string>`，否则 `_Vector_base`、allocator traits 和 `reference` 都无法继续解析。
- `basic_string` 的 traits 和 allocator 也有默认值，不能只处理显式写出的第一个实参。
- `unique_ptr<T>` 的 deleter 有默认值，智能指针本身的声明查找和成员类型同样需要补齐。

验收：`vector<string>`、`basic_string<char>`、`unique_ptr<string>` 的默认实参只在求值时补齐，StatementModel 中不出现实例化副本。

### 2. typedef/using 别名链与依赖嵌套类型

最低要求：

- 在别名声明所在作用域中解析其目标；
- 将当前模板绑定代入别名目标；
- 支持 `typename A::B` 形式的依赖嵌套类型；
- 保留指针、引用和 cv 限定；
- 检测别名循环，失败时返回未解析类型，不能崩溃或无限递归。

STL 依据：

`vector<string>::reference` 的真实链条包括：

```text
vector::reference
  -> typename _Alloc_traits::reference
  -> __gnu_cxx::__alloc_traits<_Tp_alloc_type>::reference
  -> value_type&
```

而 `_Tp_alloc_type` 又来自 `_Vector_base<_Tp, _Alloc>`。只替换 `_Tp = string` 而不递归处理依赖别名，就只能得到本轮要修复的 `typename _Alloc_traits::reference`。

`list<string>::front()` 也经过 `_Tp_alloc_traits::reference`，证明该能力不能只针对 vector 实现。

验收：`lst[0]`、`lst.front()`、`lst.back()`、`lst.at(0)` 和 `linked.front()` 都能继续定位到 `basic_string` 的成员。

### 3. 成员别名模板、template 关键字与 rebind

最低要求：

- 识别依赖限定名中的 `template` 关键字；
- 支持类内成员别名模板；
- 支持用新实参替换成员模板的形参；
- 支持 `rebind<_Tp>::other` 这种成员模板返回的嵌套类型。

STL 依据：

libstdc++ 的 `_Vector_base<_Tp, _Alloc>` 包含：

```cpp
typedef typename __gnu_cxx::__alloc_traits<_Alloc>::template
    rebind<_Tp>::other _Tp_alloc_type;
typedef typename __gnu_cxx::__alloc_traits<_Tp_alloc_type>::pointer pointer;
```

`vector::pointer` 和 `vector::iterator` 都依赖这条链。缺少成员别名模板或 `template` 限定解析，`begin()`、`operator*` 和 `operator->` 就无法恢复元素类型。

验收：`lst.begin()` 得到可解析的 iterator 类型，`it->size()`、`(*it).size()` 均定位到 `basic_string::size`。

### 4. 主模板、部分特化和全特化选择

最低要求：

- 按实际类型和非类型实参匹配主模板、部分特化和全特化；
- 全特化优先于部分特化，部分特化优先于主模板；
- 特化匹配只处理本轮需要的类型结构、指针/引用/cv 和常量实参；
- 不做 SFINAE，不把约束求值混入特化选择。

STL 依据：

- `allocator_traits<allocator<_Tp>>` 有针对 `std::allocator` 的部分特化，直接定义 `value_type = _Tp`、`pointer = _Tp*` 和 `rebind_alloc = allocator<_Up>`。
- `iterator_traits<T*>` 是指针迭代器的必要特化；`__normal_iterator` 的 `reference` 和 `pointer` 都来自它。
- `_MakeUniq<_Tp>` 决定 `make_unique<T>` 的返回类型，数组形式使用不同特化。
- `vector<bool>` 是特化，不能错误套用主模板的 `bool&` 语义。

验收：默认 allocator 的 vector iterator 能解析；`make_unique<string>` 选择对象版本；`vector<bool>` 使用其真实代理 `reference`，不得伪造 `bool&`。

### 5. 依赖基类及基类成员的模板绑定

最低要求：

- 解析带实际实参的基类；
- 在基类中查找成员时，将派生类的实参映射到基类形参；
- 成员返回类型使用“成员所属基类”的绑定，而不是只使用最外层派生类绑定；
- 检测基类循环。

STL 依据：

- `vector<_Tp, _Alloc>` 继承 `_Vector_base<_Tp, _Alloc>`，其 `pointer` 等嵌套类型依赖基类绑定。
- `__alloc_traits<_Alloc>` 继承 `std::allocator_traits<_Alloc>`，`value_type` 等成员来自基类。
- `shared_ptr<T>` 经 `__shared_ptr<T>` 继承 `__shared_ptr_access<T>`，`operator*` 和 `operator->` 在基类中定义。

验收：vector 的 pointer/iterator 链和 `shared->size()`、`(*shared).size()` 均可用。

### 6. 本轮所需的有限类型特征计算

最低要求：只实现被目标 STL 路径实际使用的类型特征，并以 `TypeInfo` 为输入直接计算，不展开函数体，不做通用 constexpr 元编程。

本轮候选项及依据：

- `add_lvalue_reference<T>`：`unique_ptr<T>::operator*` 的返回类型是 `typename add_lvalue_reference<element_type>::type`。
- `is_array<T>::value`、`is_void<T>::value`：`__shared_ptr_access` 用这两个值作为默认非类型模板实参，决定对象、数组和 void 的访问接口。
- `remove_extent<T>`：数组形式的 shared/unique pointer 若纳入验收，需要它恢复元素类型；不实现数组形式时也不得误判其类型。

这些类型特征仍是通用语义规则，不是按 `shared_ptr` 名称伪造返回类型。

验收：普通对象 `shared_ptr<string>`、`unique_ptr<string>` 可解引用；不支持的数组形式应返回未解析结果，而不是返回错误元素类型。

### 7. 函数模板的显式实参与调用实参推导

最低要求：

- 显式模板实参先绑定到对应函数模板形参；
- 再从调用实参推导未指定的形参；
- 将绑定代入函数返回类型；
- 支持返回类型本身是模板别名或依赖嵌套类型；
- parameter pack 必须能正确消费，但只要求在返回类型不依赖包展开时求值。

STL 依据：

- `make_shared<string>(...)` 的返回类型是 `shared_ptr<_Tp>`，`_Tp` 来自显式实参，`_Args&&...` 不影响返回类型。
- `make_unique<string>(...)` 的返回类型是 `typename _MakeUniq<_Tp>::__single_object`，同时依赖显式实参、特化和别名。
- `find_if(first, last, pred)` 的返回类型是 `_InputIterator`，来自调用实参推导。
- `transform(first, last, output, op)` 的返回类型是 `_OutputIterator`，必须从对应实参位置推导，不能只处理第一个参数。

验收：`make_shared`、`make_unique`、`find_if`、`transform` 的 auto 变量类型正确，后续成员访问可用。

### 8. 非类型模板实参与有限常量语义

最低要求：

- 保存并绑定整数、枚举常量及简单的默认非类型实参；
- 支持数组维度和模板布尔实参；数组维度只携带，不求值；
- 支持第 6 项明确列出的类型特征布尔值；
- 显式非类型实参经过别名或嵌套模板转发时，先在当前绑定环境中代入形参并计算上述有限布尔值，再参与下一层默认绑定或特化匹配；无法计算的表达式保持未知，不按字面文本伪造匹配；
- 不实现通用 constexpr 表达式求值。

STL 依据：

- `array<string, 2>` 只要求正确携带非类型实参 `2`，不得把它误当成类型。元素类型只由第一个模板实参 `string` 决定，**不需要求出或使用数值 `2`，也不让 `N` 参与元素类型推导**。数组越界检查不属于本任务。
- `__shared_ptr_access` 的 bool 形参需要默认值和特化匹配。
- libstdc++ 的内部智能指针基类还使用 `_Lock_policy` 枚举实参，必须能携带该实参进行声明匹配。
- `_NonArray<_Tp>` 经 `__enable_if_t<!is_array<_Tp>::value, _Tp>` 转发布尔实参。这里只复用有限的 `is_array` 和逻辑非计算：非数组得到实际类型，数组保持未解析，不引入通用 SFINAE。

验收：`array<string, 2>` 在不计算 `N` 的前提下，下标返回 string 引用；普通智能指针的默认非类型实参可匹配；`make_shared<string>()` 后的 `->` 补全和成员访问可用。

### 9. cv、指针、左值/右值引用和数组声明结构

最低要求：

- 模板替换时保留并正确合并 `TypeDerivation`；
- 区分 `T`、`T&`、`const T&`、`T*`、`T&&`；
- auto 值推导去除引用和顶层 cv，auto 引用按声明保留；
- 类型特征计算读取同一套结构，不另建字符串特判。

STL 依据：

- vector/list 的元素访问返回 `T&` 或 `const T&`；
- `iterator_traits<T*>::reference` 为 `T&`，`pointer` 为 `T*`；
- `unique_ptr<T>::operator*` 通过 `add_lvalue_reference<T>` 生成 `T&`；
- 智能指针和函数模板参数中普遍存在 `&&`。

验收：表达式最终声明、引用层级和 const 语义正确，成员访问不因多余或缺失引用而失败。

### 10. 重载候选的有限选择

最低要求：

- 排除 `=delete` 候选；
- 先应用显式模板实参，再做调用实参推导；
- 使用参数数量和本轮已支持的类型结构筛选候选；
- parameter pack 可匹配剩余实参；
- 候选仍有歧义时返回未解析结果，不按名称猜测，不实现完整转换序列排名。

STL 依据：

- `make_unique` 有单对象、未知边界数组和已删除的有界数组重载；普通对象调用必须选择单对象版本。
- algorithm 可能同时提供多个重载，function tips 需要保留可展示的候选，而求值只能使用能确定返回类型的候选。

验收：普通 `make_unique<string>` 求值正确；function tips 能显示实际调用处的函数签名；歧义场景不产生错误跳转。

## 明确不在本轮支持的内容

以下内容不得因某个测试暂时失败而顺手加入，必须另行讨论并更新设计：

- range/view 的真实标准库验收和 concepts/range 约束求值；
- 通用 SFINAE、detection idiom、`void_t` 语义；
- 完整重载决议、隐式转换序列和模板特化排序的完整规则；
- 通用 constexpr 求值和任意模板元编程；
- parameter pack 展开、包索引和依赖包的复杂返回类型；
- CTAD、deduction guide 求值；
- 函数体模板实例化、auto 函数返回类型的通用函数体推导；
- 完整表达式 AST 或实例化 Statement 树；
- shared_ptr/unique_ptr 数组形式的验收；
- 为支持上述内容而引入的持久实例化缓存。

已有的 range/view 模拟测试继续作为既有能力回归，但本轮不得宣称真实 range/view 已支持。

## 数据结构设计

### 持久声明与一次型求值数据

代码分析阶段不填、只有 expression eval 才填的属性一律是一次型数据，不得回写到 Statement
或其持有的持久 TypeInfo。这包括结构化模板实参、绑定帧、特化选择结果、基类绑定环境、
候选筛选状态、递归检测和求值缓存。它们由一次求值的上下文持有，求值结束即释放。

代码分析阶段应当填写的声明事实由 Statement 保存，包括模板形参、默认实参原文、特化模式、
原始类型和基类。某个声明字段尚未实现填写，不表示它可以改为 eval 时回写。`auto` 变量的
最终类型在 StatementBuilder 分析阶段已经求出，因此可以保存；其初始化表达式和求值过程不保存。

### 声明期保存完整限定名原文

单份 `templateArguments` 不能表示 `A<T>::template B<U>::type` 中各段各自的实参。
声明侧不需要持久化逐段结构。`TypeInfo` 以完整限定名原文 `qualifiedNameText` 保存：

- 最前面的 `typename`、elaborated type 关键字和全局 `::`；
- 每一段自己的名字和模板实参；
- 段间的 `::` 和可选 `template` 关键字。

现有 `qualifiedName` 保留当前查找语义，现有 `templateArguments` 只用于当前单段实体化路径，
不得把它当作完整多段限定名的实参集合。完整原文是后续模板求值的输入，不能通过
`QString::split("::")` 分段，因为模板实参和 `decltype(...)` 内部也可能包含 `::`。

模板求值所需的声明文本在 parse 时优先按起止位置从连续的预处理原文一次截取，避免不必要的
逐 token 拼接。该规则适用于完整限定名、模板实参、默认实参、特化模式和基类文本；如果目标
文本需要跳过中间内容、合并非连续片段或做明确要求的规范化，则可以按需拼接。直接截取前必须
校验起止 token 与缓冲区对应文本一致；预处理行映射不一致时回退到 token 规范化拼接，不能返回
错位内容。起止位置只用于本次截取，不随 TypeInfo 持久保存；跨行处以空格连接。结束位置必须精确处理 `>>` 拆分：
只消费第一个 `>` 时，不能把第二个 `>` 纳入当前类型名。

确需拼接时复用 `CppDeclarationParser` 的 `appendToken()`，不要自行判断空格。参考代码：

```cpp
QString result;
for (const QString &text : selectedTokens)
    appendToken(result, text);
```

`selectedTokens` 是已经跳过无关内容、按目标顺序保留的 token。`appendToken()` 会在相邻两个
标识符字符之间自动补空格，避免把 `unsigned long` 拼成 `unsignedlong`；对于 `::`、`<>`、
`*` 等运算符则不添加额外空格。配平区域需要保留内部结构时，继续使用
`readBalancedToken(opening, closing, false)` 取得片段后追加。

expression eval 按需把完整原文 tokenize 为临时 `TypeNameSegment` 列表。每段记录自己的名字、
实参、是否显式写出实参列表和 `templateKeyword`，但该列表只存在于求值上下文中，不写入 StatementModel。
这样 `A<T>::template B<U>::type` 中 `T` 属于 `A`，`U` 属于 `B`，最终 `type` 没有实参。
若结构化 `TypeInfo` 已带有实参，也必须同步标记为显式实参，不能仅因名称原文省略了 `<...>` 就按无实参处理。

类作用域内的 injected-class-name（例如 `vector<T>` 定义内单独写 `vector`）表示当前实例化，复用当前类
绑定帧中的实参；只有未显式写出实参列表时才适用。普通类名查找不要求实例化：Class/Enum 的表达式结果
直接保留声明，只有 Alias/Typedef 才展开目标。using declaration 引入类模板时，引用处的实参继续传给目标；
普通别名模板仍按自己的绑定帧解释，不能套用这条转发规则。

`Statement::fullName` 不变，仍表示声明的真实作用域路径。上述结构调整只针对类型引用，不压缩
inline namespace，也不改变 Statement 树。

### 函数签名的规范化

`Statement::args` 保留普通参数名和模板形参名，用于函数提示等界面显示。`Statement::normalizedArgs`
仅用于声明合并、重载区分和内部索引：普通参数名省略，cv/ref、指针、数组、函数修饰和参数包等
签名结构保留，默认实参不参与比较。模板形参按声明位置规范化为内部占位符：类型、非类型和模板
模板形参分别使用 `@T0`、`@N0`、`@TT0` 等形式。占位符不是合法 C++ 标识符，避免与源码中的真实
类型冲突；替换按 token 精确进行，不做全字符串替换。

因此，同一函数模板在声明与定义中可以使用不同的模板形参名，仍合并为一个 Statement；真正不同的
重载必须保持不同的 `normalizedArgs`。该规范化不实例化模板，也不改变 `args` 的显示内容。

函数后缀的 const、volatile、`&` 和 `&&` 同时保存在现有 `Statement::properties` 位标志中，
不另设平行的函数类型对象。`Statement::typeInfo` 继续只表示函数返回类型；static、extern 等声明
说明符仍由既有声明属性保存，访问级别仍由 `accessibility` 保存。

### 声明期只补充原文信息

`TemplateParameterInfo` 增加：

- `defaultArgumentText`：`=` 后的原文，不区分类型或非类型默认值都先保存；
- 非类型形参的声明类型：用于识别 bool、整数和枚举实参，不用于通用常量求值。

特化实参保留在 Statement 的 `command/fullName` 完整特化名中，不另设重复字段；eval 需要匹配时
从该名称现场解析。`Statement::baseClasses` 保存基类原文。它们都在求值时复用同一个受限
类型名/模板实参解析器，不在声明期展开。

声明期必须完整消费并保存每一段的信息，但不得根据默认实参、基类或别名创建实例化 Statement。

### 非类型实参同时区分原文和已知值

`TemplateArgument` 保留 `constantExpression` 原文。只有特化匹配或明确支持的类型特征确实需要值时，
才附加一个“已知常量值”：

- bool 规范化为 `false/true`；
- 整数字面量可规范化为十进制，但数组维度不要求进行这一步；
- 枚举常量优先按声明身份比较，相同符号原文也可直接比较；
- 其它表达式保持未知，不调用通用表达式求值器计算。

“原文相同”“已知值相同”和“无法判断”必须区分。无法判断不能当作不相等，否则会在证据不足时
错误退回主模板。

### 绑定环境按声明层级连接

当前扁平的 `QHash<QString, TemplateArgument>` 会让不同模板中的同名 `_Tp`、`T` 相互覆盖。
求值期改用非持久化的绑定帧链：

```cpp
struct TemplateBindingFrame {
    const Statement *owner;                 // 当前主模板、特化或别名声明
    QHash<QString, TemplateArgument> values; // 仅当前声明自己的形参
    std::shared_ptr<TemplateBindingFrame> parent;
};
```

`owner` 只是求值上下文内的非 owning 指针；表达式求值持有 Parser 读锁，StatementModel 在此期间不变。
父帧不是任意外层调用者，而是声明的词法模板环境：

- 类成员别名模板的父帧是该类的绑定帧；
- 嵌套类可以连接外层类帧；
- 基类成员使用由基类实参新建的基类帧，不能回退到派生类的同名形参；
- 函数模板帧连接其声明所在类或 namespace 的环境；
- 普通 namespace 不产生绑定帧。

绑定查找只能沿这条词法链进行，不允许把外层 Receiver 的所有绑定复制进每个新声明。

### 临时解析结果必须携带成员所属环境

仅返回 `PStatement` 不足以继续解析依赖基类成员。求值器内部使用两个临时结果：

- `ClassInstance`：实际 `TypeInfo`、选中的主模板/特化 Statement、该声明的绑定帧；
- `MemberResolution`：成员 Statement、实际声明该成员的 `ClassInstance`、继承后的可访问性。

`ClassInstance` 同时区分两组信息：

1. 对主模板名的实际实参，用于标识当前类型；
2. 对选中特化自身形参的绑定，用于解释该特化体内的声明。

例如 `A<int*>` 选中 `template<class T> class A<T*>` 后，第一组实参是 `int*`，第二组绑定是
`T = int`。不能把特化形参按位置直接绑定到第一组实参。

这些对象只存在于一次表达式求值中，不加入 StatementModel，不被 Statement 持有。

## 核心算法

### 1. 实参实体化与默认实参绑定

“实体化”只指把某一段的实参原文解析为 `TemplateArgument`，不解析模板声明体。

主模板和别名模板按形参顺序建立绑定：

1. 使用调用处显式实参；
2. 缺少实参时，在该声明的词法环境中解析默认实参原文；
3. 默认实参可以引用已经绑定的前序形参；
4. 把完整实参代入后续默认实参，但不修改声明中的原文；
5. 实参数量、种类不符合要求且没有默认值时返回未解析；
6. parameter pack 可以接收剩余实参，但本轮不展开依赖包的返回类型。

偏特化不能按上述位置规则直接绑定，必须先通过特化匹配从实际实参推导它自己的形参。

显式实参的类型只在限定名分段解析时实体化一次。`declarationBindings()` 接收的 `TypeInfo` 必须已经携带解析后的结构化实参，它只按位置消费这些实参并补充默认实参，不得再次解析显式实参。`classInstance()` 同样只消费 `resolveType()` 已解析的结果。这样每一层模板绑定只处理自己新增的默认实参，避免沿别名和依赖基类链重复展开相同类型。

### 2. 类型替换与限定名解析

解析一个 `TypeInfo` 时按以下顺序执行：

1. 如果单段名字是当前绑定帧中的类型形参，代入其实际类型；
2. 逐段实体化模板实参，并在当前环境中解析实参内部的类型；
3. 从当前词法作用域解析第一段，再沿嵌套类型逐段解析后续段；
4. 每一段只使用属于自己的实参，`template` 关键字不改变查找结果；
5. 找到类模板时执行特化选择，得到 `ClassInstance`；
6. 找到 typedef/using 时，在该别名的绑定环境中解析目标；
7. 别名目标解析完成后，再把引用处的 cv、指针、引用和数组结构应用到结果上；
8. 找不到声明、种类不符或遇到不支持的结构时返回未解析。

`typename A::B` 只是允许把依赖成员当作类型，不意味着可以直接返回名为 `A::B` 的已知类型。
只有最终得到实际声明或本轮可计算的内置类型，才算解析成功。

### 3. 主模板与特化选择

候选来自同一实际作用域中同名类模板的主模板、全部偏特化和全特化；inline namespace 仍由现有
透明索引处理。选择流程为：

1. 先检查全特化；唯一精确匹配时立即选择；
2. 再检查全部偏特化，用模式匹配同时推导特化形参；
3. 只有一个偏特化可匹配时选择它；
4. 多个偏特化可匹配时，本轮不实现完整“更特化”排序，返回未解析；
5. 所有特化都明确不匹配时才选择主模板。

匹配结果是三态：匹配、不匹配、无法判断。任一可能影响结果的候选为“无法判断”时，不得直接退回
主模板并伪造成功。

偏特化优先级是明确的后续工作，本轮不实现。典型场景是 `iterator_traits<T*>` 与
`iterator_traits<const T*>` 同时匹配 `const std::string*`；当前必须保持未解析，不能按声明顺序或
const 数量猜测。后续设计至少需要区分“声明结构相同、cv 更具体”和结构不同的候选，并验证
`T*` 与 `const T` 等反例，避免把简单的 cv 计数误当成 C++ 偏序规则。

类型模式匹配按结构递归：

- 模板形参捕获对应实际类型；重复出现时，各次绑定必须相同；
- 已知名称按解析后的声明身份匹配，不仅依赖可能包含 inline namespace 的字符串；
- 指针、引用、cv、数组和外层模板实参逐层匹配；
- 非类型实参只比较字面量、已绑定形参和本轮可计算的 bool/枚举值；
- 不执行 SFINAE、约束求值、用户定义转换或通用 constexpr。

别名模板不能特化，本轮也不为它们引入特化候选逻辑。

### 4. 依赖基类与成员查找

进入类成员前，必须先有该类实际实参对应的 `ClassInstance`。基类处理流程为：

1. 把基类原文解析为带逐段实参的 `TypeInfo`；
2. 在派生类绑定帧中解析基类实参；
3. 对基类执行特化选择，建立独立的基类绑定帧；
4. 在基类实例中继续查找成员，并返回成员所属的基类 `ClassInstance`；
5. 成员类型和成员函数返回类型只在它自己的所属环境中解析。

直接成员优先于继承成员，继承访问控制和构造函数/析构函数不继承等现有规则保持不变。
查找到重载函数时返回候选列表，不能由第一个同名成员遮蔽其余候选。当前有限成员选择只额外处理
函数 const 后缀：const 接收对象排除非 const 非静态成员；非 const 接收对象优先选择非 const
重载，没有时才使用 const 重载。完整 cv/ref 重载决议仍不在本轮范围内。

补全列表可以从选中特化及其基类收集 Statement 用于展示，但表达式求值必须保留对应的
`MemberResolution`，否则会在后续 `front()`、`operator*` 等返回类型处丢失基类绑定。

### 5. 有限类型特征与已知 bool 值

类型特征按声明的语义身份识别，只在请求其结果成员时计算：

- `add_lvalue_reference<T>::type` 生成 `T&`；
- `is_array<T>::value` 根据 `TypeDerivation` 是否包含数组生成已知 bool；
- `is_void<T>::value` 只匹配解析后的内置 `void`；
- `remove_extent` 留到数组智能指针单独纳入范围时再实现，本轮不提前支持。

特征结果仍是临时 `TypeInfo` 或已知常量，不生成 Statement。标准 traits 名称用于定位语言规定的
语义操作是允许的；禁止把同样的方式用于伪造容器、iterator 或智能指针的普通成员返回类型。

### 6. 函数模板实参推导与候选筛选

每个候选独立建立绑定帧，禁止把一次失败推导留下的绑定复用到下一个候选。

1. 显式模板实参先按函数模板形参顺序绑定；
2. 将调用实参与非包形参逐一做结构匹配，推导尚未绑定的形参；
3. 同一形参被推导出不同类型时排除该候选；
4. `const T&` 等参数按本轮已有规则忽略引用和顶层 cv 后推导；
5. 对转发引用 `T&&`，左值实参绑定 `T = U&`，右值实参绑定 `T = U`，随后使用引用折叠；
6. parameter pack 可以消费剩余实参；若返回类型需要展开包，则该候选返回未解析；
7. 用完整绑定解析候选返回类型，包括别名和依赖嵌套类型。

候选筛选只使用显式实参、参数数量、pack 数量和本轮支持的结构匹配。排除 `=delete` 后仍有多个
候选时返回未解析；function tips 仍可展示全部声明候选，但不能据此猜测表达式类型或跳转目标。

### 7. cv、指针和引用的组合

类型替换分成“取得形参实际类型”和“应用引用处声明结构”两步，不能通过字符串前后拼接完成。

- 指针和数组按 `TypeDerivation` 原顺序组合；
- 顶层 cv 与引用处的 cv 取并集，不重复添加派生节点；
- 引用折叠只实现 C++ 的四条基本规则：`& + & -> &`、`& + && -> &`、
  `&& + & -> &`、`&& + && -> &&`；
- `auto` 值推导去除引用和顶层 cv，`auto&/auto&&` 按声明保留并应用上述规则；
- 不做额外数组到指针 decay，除非具体函数参数推导规则明确要求。

## 求值上下文、终止与缓存

每次表达式求值创建独立 `TemplateEvaluationContext`，包含：

- 当前词法作用域和绑定帧链；
- 当前正在解析的别名、类实例和基类路径；
- 递归深度及本次求值的操作预算；
- 可选的本次求值内临时缓存；
- Debug 诊断所需的解析路径。

递归检测键至少包含“声明身份 + 完整结构化实参 + 操作种类”。只按 Statement 指针检测会把同一
别名模板以不同实参进行的合法嵌套误判为循环；只按名称则会混淆不同作用域的声明。

出现以下情况时返回未解析，不崩溃、不无限递归：

- 相同检测键已经位于当前活动栈；
- 基类实例沿当前继承路径重复；
- 超过预设深度或操作预算；
- 实参无法实体化、特化匹配无法判断或绑定冲突。

预算只用于终止失控求值，不能把正常 STL 深度硬编码成某个容器专用上限。Debug 日志应输出失败阶段、
类型原文、声明位置和活动栈摘要；Release 不输出逐类型日志。

第一版不做跨表达式缓存。若后续 profile 证明需要本次求值内缓存，缓存键必须包含声明身份、全部
结构化实参、cv/派生结构和操作种类；缓存生命期不得超过本次表达式求值。StatementModel 的锁、
invalidate 和 Registry 生命周期均不因该缓存改变。

## IDE 查询的统一入口

补全、goto、ParserHint 和 function tips 不得各自实现一套特化或别名逻辑：

1. 表达式求值得到带实际实参和选中声明的 `TypeInfo`；
2. 通过同一个 `ClassInstance` 入口取得成员或重载候选；
3. 补全只使用展示所需的 Statement 信息；
4. goto/ParserHint 使用 `MemberResolution` 中的真实声明位置；
5. 函数调用和 function tips 共用候选收集，但只有表达式求值执行模板绑定和返回类型解析。

补全将 owner 表达式与 `.`/`->` 一并交给表达式求值器。以成员操作符结尾的不完整表达式返回成员接收
类型，供成员列表枚举；带最终成员名的完整表达式仍返回该成员的类型。裸指针和重载 `operator->` 共用
同一接收类型路径，不按 STL 名称特判，也不需要单独的公开接收类型查询。裸指针误用 `.` 仍由补全层拒绝。

function tips 使用函数名位置的完整表达式（例如 `std::find_if`、`it->size`）先解析目标函数，再收集
其实际 namespace/class 中的全部可见重载。声明是否覆盖调用行不参与筛选；头文件声明和成员调用与当前
文件中的函数使用同一规则。展示继续使用原始 `args`，声明合并和候选区分使用 `normalizedArgs`。

任何求值前后，StatementModel 的 Statement 数量、声明级 `TypeInfo` 和模板实参原文都必须保持不变。

## 分批实施顺序

每批都先增加最小通用语法测试，再增加真实 STL 测试；不得只以 STL 通过证明通用性，也不得只以模拟模板通过证明真实库可用。

1. **类型名原文与默认实参**：保存完整限定名原文，覆盖 `A<T>::template B<U>::type`；求值时再临时分段；保存默认实参原文并在求值时补齐，用自定义 `Box<T, U = T>` 验证。
2. **别名与依赖嵌套类型**：实现分层绑定、typedef/using、typename 和 cv/ref 合并；打通 vector/list 元素访问。
3. **成员别名模板、特化与依赖基类**：覆盖 rebind、allocator traits、iterator traits 和基类绑定；打通 begin/end、解引用和两种循环。
4. **有限类型特征与智能指针**：实现本轮明确列出的 traits；打通 make_shared/make_unique、`->` 和 `*`。
5. **函数模板与传统 algorithm**：完善显式实参、调用实参推导和有限重载选择；打通 find_if、transform 等返回迭代器的场景。
6. **IDE 消费端验收**：使用同一批真实表达式分别验证补全、gotoDeclaration、gotoDefinition、ParserHint 和 function tips。

前一批未通过对应测试时，不得靠后一批的专用规则掩盖问题。

## 验收要求

### 通用测试

每个特性都必须有不依赖 STL 名称的最小声明测试，覆盖：

- 限定名不同段分别携带实参，且后一段不会覆盖前一段；
- 默认实参引用前序形参；
- 多级别名和依赖嵌套类型；
- 外层类、成员别名和基类使用相同形参名时绑定不串层；
- 成员别名模板和 rebind；
- 主模板、单个偏特化、全特化，以及多个偏特化匹配时的未解析结果；
- 非类型值无法判断时不得错误退回主模板；
- 依赖基类成员返回类型使用基类绑定；
- 显式函数模板实参、调用实参推导、绑定冲突和转发引用；
- 非类型实参、cv/ref、参数包消费；
- 别名/基类循环和无法解析时的安全退出；
- 求值前后 StatementModel 数量及声明级模板原文均不变。

### 真实 STL 测试

使用本机 GCC 8.1 的真实头文件，至少覆盖：

- `vector` 的 `front/back/at/operator[]`；
- `deque`、`array` 的下标；
- `list::front`；
- `map::operator[]`；
- `auto it = lst.begin()` 后的 `it->`、`(*it).`；
- `while (++it != lst.end())`、普通 for 和 range-for 内的元素访问；
- `make_shared`、`make_unique` 后的 `->` 和 `*`；
- `find_if`、`transform` 返回值后的元素访问；
- `vector<bool>` 不误返回 `bool&`。

同一表达式必须能用于补全、声明/定义跳转、ParserHint 和函数参数提示，不能只验证内部类型字符串。

### 失败语义

遇到超出本文范围的合法 C++ 时，允许返回“未解析”，但不得：

- 返回一个看似合理但错误的类型；
- 跳转到错误 Statement；
- 修改声明级模板数据；
- 无限递归、崩溃或卡死；
- 通过 STL 名称硬编码伪装成通用解析成功。

## 扩大范围的条件

后续若确需新增模板能力，必须先提供：

1. 一个目标用户场景；
2. 对应的实际 STL 声明或项目代码证据；
3. 为什么现有规则不能解析的分析；
4. 对实现复杂度和失败语义的影响；
5. 新的通用测试与真实库测试。

在该评审完成前，不得以“再支持一点就能通过”为理由把 SFINAE、concepts、完整重载决议或通用 constexpr 引入本轮实现。
