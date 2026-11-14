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
- 不实现通用 constexpr 表达式求值。

STL 依据：

- `array<string, 2>` 只要求正确携带非类型实参 `2`，不得把它误当成类型。元素类型只由第一个模板实参 `string` 决定，**不需要求出或使用数值 `2`，也不让 `N` 参与元素类型推导**。数组越界检查不属于本任务。
- `__shared_ptr_access` 的 bool 形参需要默认值和特化匹配。
- libstdc++ 的内部智能指针基类还使用 `_Lock_policy` 枚举实参，必须能携带该实参进行声明匹配。

验收：`array<string, 2>` 在不计算 `N` 的前提下，下标返回 string 引用；普通智能指针的默认非类型实参可匹配。

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

## 求值上下文与解析流程

每次表达式求值使用独立上下文，至少包含：

- 当前词法作用域；
- 模板形参到实际 `TemplateArgument` 的绑定；
- 当前声明及其实际特化；
- 别名和基类解析的递归检测；
- 可选的本次求值内临时缓存。

类型解析按以下顺序进行：

1. 按需把模板实参原文解析为结构化 `TemplateArgument`；
2. 用显式实参和默认实参建立当前声明的绑定；
3. 对类型名先执行模板形参替换；
4. 查找主模板或特化声明；
5. 若是 typedef/using，进入别名目标并携带绑定；
6. 若是依赖基类，先建立基类绑定，再查成员；
7. 若是本轮支持的类型特征，直接根据 `TypeInfo` 计算；
8. 合并 cv、指针、引用等声明结构；
9. 无法按上述规则解析时返回未解析结果，不伪造名称或类型。

声明查询继续使用真实 `fullName` 和 `parentScope`。类型替换不得修改原 Statement 中的声明级 `TypeInfo`，所有结构化替换都发生在求值上下文持有的副本中。

## 分批实施顺序

每批都先增加最小通用语法测试，再增加真实 STL 测试；不得只以 STL 通过证明通用性，也不得只以模拟模板通过证明真实库可用。

1. **默认实参与绑定基础**：保存默认实参原文；求值时补齐显式和默认实参；用自定义 `Box<T, U = T>` 验证。
2. **别名与依赖嵌套类型**：实现 typedef/using、typename 和 cv/ref 合并；打通 vector/list 元素访问。
3. **成员别名模板、特化与依赖基类**：覆盖 rebind、allocator traits、iterator traits 和基类绑定；打通 begin/end、解引用和两种循环。
4. **有限类型特征与智能指针**：实现本轮明确列出的 traits；打通 make_shared/make_unique、`->` 和 `*`。
5. **函数模板与传统 algorithm**：完善显式实参、调用实参推导和有限重载选择；打通 find_if、transform 等返回迭代器的场景。
6. **IDE 消费端验收**：使用同一批真实表达式分别验证补全、gotoDeclaration、gotoDefinition、ParserHint 和 function tips。

前一批未通过对应测试时，不得靠后一批的专用规则掩盖问题。

## 验收要求

### 通用测试

每个特性都必须有不依赖 STL 名称的最小声明测试，覆盖：

- 默认实参引用前序形参；
- 多级别名和依赖嵌套类型；
- 成员别名模板和 rebind；
- 主模板、部分特化、全特化；
- 依赖基类成员返回类型；
- 显式函数模板实参和调用实参推导；
- 非类型实参、cv/ref、参数包消费；
- 别名/基类循环和无法解析时的安全退出。

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
