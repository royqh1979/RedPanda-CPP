# C++17 支持计划

## 目标与边界

目标是提高 Red Panda C++ 在常用 C++17 代码中的声明解析、表达式求值、补全、导航和 Parser Hint 正确性，不以“完整实现 C++17 编译器”为目标。

表达式求值能力应可以按教学和竞赛中的使用范围分层启用。分层用于控制复杂度和查询成本，不改变 C++ 语法解析的基本行为；关闭某层时，对应表达式可以返回未知，不能猜测类型或选择任意候选。

本计划不预先扩大到通用 constexpr 求值、任意模板元编程、完整函数体实例化、完整 Concepts 偏序、PMR、并行算法或 ranges/views。所有能力必须通过实际声明和表达式验证，不能仅因标准库头文件能够完成 parse 就视为支持。

## 当前表达式基线

测量环境：GCC 15.3、C++17、Release。每项测试在独立新进程中连续运行两次，采用第二次结果；CSV 保留纳秒原始值，不把单次时间当作稳定统计值。

完整的逐场景数据见 [cpp17-stl-eval-profile.csv](cpp17-stl-eval-profile.csv)，字段为：

- `suite`：容器访问或算法测试套件；
- `scenario`：容器、元素类型和操作；
- `elapsed_ns`：单次表达式求值耗时；
- `valid`：是否得到有效结果；
- `type`：完整求值类型，包含模板实参、cv 和引用信息。

当前 133 个场景均得到有效且符合预期的结果：

| 场景组 | 场景数 | 中位耗时（ms） | 最大耗时（ms） |
|---|---:|---:|---:|
| 容器访问、遍历、智能指针和 optional | 38 | 0.379 | 1.304 |
| `vector<string>` 自由算法 | 32 | 0.845 | 1.399 |
| `vector<int>` 自由算法 | 21 | 0.658 | 1.126 |
| `deque<string>` | 9 | 0.792 | 1.349 |
| `list<string>` | 7 | 0.324 | 1.047 |
| `forward_list<int>` | 7 | 0.248 | 0.603 |
| `array<string>` | 8 | 0.349 | 0.394 |
| `map<int,int>` | 3 | 0.655 | 0.698 |
| `set<string>` | 4 | 0.705 | 1.250 |
| `unordered_map<int,int>` | 2 | 0.821 | 0.860 |
| `unordered_set<int>` | 2 | 0.738 | 0.751 |

覆盖插入、删除、排序、求和、查找、乱序、复制、变换、成员访问和迭代器遍历。矩阵按迭代器类别选择合法操作，不把随机访问算法硬套到 list、set 等容器上，也不以无效表达式填充笛卡尔积。关联容器返回类型必须完成模板实参替换；range-for 和 `auto` 变量必须保留正确的引用类别。

`test_standard_library_container_eval_profile` 和 `test_standard_library_algorithm_eval_profile` 分别保留 `REDPANDA_STDLIB_CONTAINER_CASE`、`REDPANDA_STDLIB_ALGORITHM_CASE` 过滤入口。后续优化必须与 CSV 对比，同时检查完整类型，不能只比较耗时或 `isValid()`。

## 能力分层

### 默认基础层

默认支持高频且不需要单独开启的基础表达式：

- `std::string`、字符串字面量及常用字符串操作；
- 内置数值类型、指针、引用和常用运算符；
- iostream 及常见流对象、流操作和链式表达式；
- 基础 auto 推导、值类别和引用折叠；
- 常用容器、迭代器、智能指针和 optional 的当前已验收表达式。

这里的“默认支持”只覆盖有测试的能力，不表示对应头文件的全部 API。

### 常用容器与算法层

作为第一层可配置的标准库增强能力，面向教学和竞赛代码：

- 顺序容器、关联容器、无序容器及容器适配器；
- 常用迭代器访问、range-for 和传统 for 遍历；
- 查找、复制、插入、删除、排序、求和、变换、乱序等常用算法；
- `string`/`int` 与上述容器、算法的代表性组合；
- 无参数或简单参数 lambda 作为算法回调。

性能优化先通过 `RESOLVER_SELF` 和调用来源审计定位成本，不用 cache 命中掩盖重复 tokenize、重复声明查找或不必要绑定。

### 智能指针与 optional 层

在基础层之外继续扩展：

- `unique_ptr`、`shared_ptr`、`weak_ptr` 的完整常用操作；
- `make_unique`、`make_shared`、指针转换和自定义 deleter；
- optional 的值访问、状态查询、值或默认值及 monadic 操作；
- 与容器、算法组合后的表达式链。

### chrono 层

单独验收：

- duration、time_point、clock 的声明和常用运算；
- duration_cast、时间差及单位转换；
- 与容器、算法和 auto 推导组合的场景。

### 其它 C++17 高频库

按实际使用优先级补充 `string_view`、tuple/variant/any、functional 和 filesystem。每类至少覆盖对象声明、成员访问、自由函数返回类型和表达式链。没有表达式验收的子系统不得仅凭 parse 成功标记为支持。

### ranges/views 层

ranges/views 最后处理，并在共同基础稳定后单独设计。开始前至少确认：

- 当前容器和算法矩阵无已知错误绑定；
- 与目标 view 适配器相关的 `operator()`、模板成员和约束求值路径可用；
- parameter pack 方案已经实现并通过真实标准库场景；
- 已建立可比较的表达式正确性与性能基线。

## 已确认的共同能力缺口

### 声明与绑定

- `operator()` 成员函数和模板类成员函数（如 `A<T>::f`）等 cppreference 已记录缺口。
- 同一声明中的多个成员函数 declarator、trailing-return 函数指针、namespace 成员函数体内 `extern` 归属、range-for 的 `char const c` 变量形式。
- if 条件中的结构化绑定。
- 结构化绑定目前只创建类型为 `auto` 的变量，没有数组、公开数据成员和 tuple-like 三类推导。

### 模板与表达式

- parameter pack 展开、包索引和依赖包的返回类型。
- 通用 SFINAE 和 detection idiom；当前 `void_t` 只覆盖已验收的有限形式，不能外推为通用支持。
- CTAD 与 deduction guide 求值；guide 当前只消费语法，不生成 Statement。
- 完整重载决议、隐式转换序列和模板特化排序。
- auto 函数返回类型的通用函数体推导。
- 通用 constexpr 与任意模板元编程不在本计划内。

## 后续实施顺序

1. 清理声明正确性和结构化绑定，保持当前 133 个场景不退化。
2. 实现 parameter pack、有限 detection idiom 和 CTAD 的共同基础，每部分独立设计、测试和 profile。
3. 扩展 optional、智能指针、tuple/variant、functional、chrono 等高频场景。
4. 完成上述分层开关及配置语义，保证关闭时返回未知而非错误类型。
5. 重新评估 ranges/views，不把完成“全部 C++17”作为开始该工作的前置条件。

## 验证规则

- 每阶段先保留失败场景，再修根因；不得删除测试或用任意候选使结果看似成功。
- 同时检查 `isValid()`、实际声明、完整类型和必要的后续表达式链。
- 性能对比使用相同 GCC 15.3/C++17 场景、Release、独立新进程第二次结果。
- 先审计重复 tokenize、声明查找和绑定来源，再决定算法、数据结构或 memo 改进。
- 完整回归先 Release 后 Debug；标准库测试以实际工具链为准。
