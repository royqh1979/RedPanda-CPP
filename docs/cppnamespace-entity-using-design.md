# C++ namespace 唯一实体与 InlineAliasStatement 设计

## 1. 目标

将命名 namespace 建模为唯一逻辑实体，并使用 InlineAliasStatement 表达 inline namespace 成员在直接父 namespace 中的可见性。

目标包括：

- 统一 namespace 重开、声明合并、名称查找、补全、树形显示和缓存身份；
- 保留实际 fullName 与实际父子关系，不压平 inline namespace；
- 不使用 fullName 路径组合展开，不设置 inline 嵌套上限；
- 普通 Statement 与 using Statement 继续沿用现有 FileInfo 失效和 replay 机制；
- namespace 多定义位置和部分失效由明确的 occurrence 模型承担；
- 分阶段实现，每个阶段保持可编译并有对应测试。

## 2. 核心模型

### 2.1 唯一命名 namespace

同一个实际词法父实体中的同名命名 namespace 只有一个 Statement：

```cpp
namespace N { int a; }
namespace N { int b; }
```

对应：

```text
N（唯一 Statement）
├── a
└── b
```

唯一身份由“实际父 Statement + 名称”决定，而不是单独由 fullName 字符串决定。实际父实体已经唯一化，因此嵌套 namespace 也逐层唯一。

namespace 重开时复用现有 Statement，添加本次 occurrence，并把新成员加入同一 children。函数重载、类特化和普通声明合并均在这个唯一 parent 中进行。

### 2.2 匿名 namespace

匿名 namespace 不参与跨父实体或跨翻译单元的全局唯一化。它仍使用 NamespaceStatement，并作为实际 parent 下的独立透明实体。

匿名 namespace 的成员通过using 投影到直接 parent；不得仅因 command 为空就合并不同匿名 namespace。

### 2.3 实际路径

Statement 的 fullName 和 parentScope 始终保留实际词法路径。例如：

```cpp
namespace std {
    inline namespace __cxx11 {
        class basic_string {};
    }
}
```

实际类型路径仍为 `std::__cxx11::basic_string`。using 提供 `std::basic_string` 的可见性，但不改写目标 Statement 的 fullName。

## 3. 使用 InlineAliasStatement 表达 inline 可见性

### 3.1 基本规则

处理 inline/匿名 namespace 的直接成员时，在它的实际 parent 中创建 `InlineAliasStatement`。它沿用 `StatementKind::Alias` 进行分类，但使用独立 C++ 类型明确表达自动生成的 inline 投影：

```text
std
├── __cxx11
│   └── basic_string
└── basic_string（using → std::__cxx11::basic_string）
```

InlineAliasStatement：

- 保存可见名称、来源和目标 weak_ptr，不拥有目标；
- 通过目标指针直接取得实际 Statement，不进行 fullName 查找；
- 不包装或转发目标的类型、参数、children 等属性；
- 正常加入 StatementModel 和 FileInfo；
- 正常参与失效、replay、补全、树形显示和声明位置更新；
- 不需要额外的 SyntheticAliasKind。

它由 inline namespace 处理自动生成。目标在创建时已知，直接保存 weak_ptr；普通源码 Alias 仍按名称解析。两者共用外层 Alias 处理流程，但 InlineAlias 不进行名称查找，也不另建属性访问接口。

### 3.2 创建时机

先完成 inline namespace 内部的正常声明构建和 merge，再判断是否创建了新的实际 Statement：

- 新增实际 Statement：在直接 parent 中创建对应 using；
- 仅合并已有声明/定义：不重复创建 using；
- 新增函数重载：新增实际 Statement，因此新增对应 using；
- 类前置声明与定义合并：using 继续指向同一 Statement；
- replay 未新增 Statement：不重复创建 using。

同一 parent 中同一目标只创建一份对应 using。该约束由创建路径保证：仅新增实际 Statement、或已有 namespace 首次获得 inline 标志时进入创建；merge 和 replay 不进入，因此不通过扫描 parent 做重复检查。

using 的创建仍走普通 StatementFactory、StatementModel::add 和 FileInfo::addStatement。

### 3.3 多层 inline

多层 inline 逐层向直接 parent 投影：

```text
C::value（实际 Statement）
B::value（using → C::value）
A::value（using → C::value）
```

中间节点也是 InlineAliasStatement，并直接保存最终实际目标的 weak_ptr，不形成需要逐次解析的 alias 链。

如果新增 using 位于 inline/匿名 namespace 中，它也属于该 namespace 的新增可见成员，因此按同一规则继续向直接 parent 投影。实际父子链无环，不需要全路径组合。

### 3.4 重开与重载

using 只加入 inline namespace 的实际 parent Statement。由于命名 parent 已唯一化，不存在向多个 parent 重开节点复制 using 的问题。

parent 的 children 继续使用多值映射：

- 同名函数重载保留全部候选；
- 不同透明分支中的同名实体不互相覆盖；
- 查询和补全按目标身份及签名去重；
- 不使用单值 name → Statement 映射覆盖冲突。

## 4. Statement 类系与工厂

### 4.1 采用有限类系，不一次拆完整类型树

第一阶段采用：

```text
AbstractStatement
├── Statement
├── NamespaceStatement
└── InlineAliasStatement
```

AbstractStatement 是三种 Statement 的共同祖先，只保存真正公共的身份、可见名称、实际父子关系、分类和来源数据。

Statement 保持普通语句的单声明/单定义模型，以及类型、参数、模板、children 等普通声明所需的数据。NamespaceStatement 保存多个 occurrence，并实现多定义来源语义。InlineAliasStatement 与 Statement 平级，只保存目标 weak_ptr 等投影所需数据，不继承普通 Statement 的类型、参数、模板、children、基类等字段；它使用独立的 StatementKind::InlineAlias。

本阶段不继续拆 VariableStatement、FunctionStatement、ClassStatement、ScopeStatement 等完整类系。普通 Statement 内部字段是否继续下沉属于后续独立重构，不与 namespace 唯一化同时进行。

### 4.2 集中处理行为，不建立行为虚函数体系

文件归属、可见性和来源贡献扣除等复杂逻辑集中在 StatementModel 等现有模型层处理，不为这些行为建立 Statement 虚函数接口：

- 普通 Statement 与 InlineAliasStatement 共用单来源处理；
- NamespaceStatement 仅扣除当前文件的 occurrence，仍有来源则保留，全部消失后才删除空壳；
- FileInfo 和 StatementModel 中条目的添加、删除由统一的公共流程执行；
- StatementFactory 只负责创建与初始化，不承担失效或可见性逻辑。

直接属性访问不得做成虚函数，不得引入 data() 或逐字段目标转发接口。children 仍是具体 Statement 类型的普通成员，不提供虚函数访问接口。调用方确需访问具体类型的属性或 children 时，先判断 StatementKind，再显式使用 dynamic_pointer_cast，不得以转换失败代替 Kind 判断。

普通源码 Alias 按名称解析，InlineAliasStatement 直接读取目标 weak_ptr。StatementKind 用于分类、显示和集中处理时的类型判断；UI 需要时可将 InlineAlias 映射为 Alias 图标。集中类型处理的代价是新增行为不同的 Statement 类型时必须同步修改模型层；当前类系是封闭的有限集合，因此不提前为假设中的扩展建立虚函数体系。

### 4.3 StatementFactory

所有 Statement 创建统一经过 StatementFactory。工厂为每种实际 StatementKind 提供独立方法，例如：

- createNamespace() → NamespaceStatement；
- createClass()、createFunction()、createVariable() 等 → Statement；
- createAlias() → Statement，用于现有源码 Alias；
- createInlineAlias() → InlineAliasStatement，用于目标已知的投影，显式传入目标 shared_ptr 并保存为 weak_ptr。

工厂负责实际 C++ 类型与 StatementKind 的一致性，并通过必传参数 parentScope、command、fullName 完成公共成员初始赋值，同时初始化 scope、accessibility 和补全计数等基础默认值；工厂方法不得使用默认参数。Builder 负责从 DeclarationInfo 填充其它语义数据。不得先创建普通 Statement 再把 kind 改成 Namespace。

本轮仍保留 PStatement 作为 AbstractStatement 的 shared_ptr，StatementModel 保持唯一所有权，children 和其它索引使用 weak_ptr。

## 5. Namespace occurrence 与失效

### 5.1 occurrence 内容

NamespaceStatement 的每次定义记录：

- 实际位置文件 fileName；
- 起止 SourceRange；
- 失效归属 boundaryAncestor；
- 本次定义是否带 inline 属性。

namespace 每次出现都是定义，不需要额外的 isDefinition。失效归属为：

```text
boundaryAncestor 非空：boundaryAncestor
否则：fileName
```

occurrence 可按失效归属建立 QMultiHash，value 保存实际文件、范围和 inline 属性。

### 5.2 新增与重开

新增命名 namespace 时：

1. 在实际 parent 的直接 children 中查找同名 NamespaceStatement；
2. 不存在则通过工厂创建并加入模型；
3. 存在则复用；
4. 添加本次 occurrence；
5. 将新 children 构建到该唯一实体；
6. 本次带 inline 且实体此前没有 inline 标志时，为已有 children 补生成 InlineAlias；之后新增的实际成员在创建时直接投影。

嵌套 namespace 逐层执行相同规则。inline 标志一旦在合并时建立，本次实现不根据剩余 occurrence 反推或撤销。

### 5.3 文件失效

失效一个归属文件时：

1. 通过 FileInfo 删除该文件贡献的普通 Statement 和 using；
2. 从 NamespaceStatement 删除对应 occurrence；
3. 若仍有 occurrence，保留 namespace，更新可见性和可用跳转位置，但不重算已有 inline 标志；
4. 若 occurrence 为空，删除 namespace 壳；
5. 不使用 shared_ptr::use_count() 判断是否仍有来源。

同一头文件被多个边界根包含时，occurrence 按归属分别保存。失效根 A 不得删除根 B 的贡献。

### 5.4 位置和 scope

每个 ParsedFileInfo 的 scope 索引继续保存本文件范围，并指向唯一 NamespaceStatement。多个文件范围可以共享同一目标。

namespace 跳转可返回多个定义位置；若现有 API 只返回一个位置，应明确选择与当前文件相关的 occurrence，而不是任意保留最后一次覆盖的位置。

## 6. 查找、补全与视图

### 6.1 名称查找

命名 namespace 唯一化后，实际路径查询沿 children 逐层进行：

- 普通 namespace 子节点直接匹配名称段；
- InlineAlias 通过目标 weak_ptr 取得实际 Statement 后继续；
- 同一级存在实际节点和 Alias 同名候选时，链式查询保留并遍历所有分支，按目标身份去重；
- class/enum 成员沿用现有规则；
- class 基类成员仍需基类图遍历。

inline 成员已经通过 using 出现在直接 children 中。不维护 namespace 透明索引，不执行透明 namespace DFS，也不通过最长 fullName 前缀猜测作用域；所有查询直接走 children/Alias 链。

### 6.2 findNamespace

findNamespace 返回按实际或 using 可见路径找到的实际 NamespaceStatement。目标为 Alias 时返回其解析目标，不把 Alias 本身冒充 NamespaceStatement。

命名 namespace 唯一化后，精确路径通常只有一个目标；现有列表 API 可在迁移期保留，以减少调用方改动。

### 6.3 补全

namespace 补全读取唯一实体的 children，实际成员和 using 投影均可见。函数重载按签名保留，显示列表按名称及目标身份去重。

class 补全的基类遍历不因本设计改变。

### 6.4 树形与导航

树形视图同时显示实际声明和using。using 的类型解析、跳转、parse hint 和 function tips 均指向实际目标。

唯一命名 namespace 实现后，class browser 直接显示唯一实体，不再需要为重开节点创建 dummy Statement。该调用方调整已完成；Block 和 Lambda 使用的 `DummyStatement` 属性与 namespace 展示无关，继续保留。

refactor/rename 不属于本轮改造范围，后续统一完善。

## 7. 声明与定义合并

唯一 namespace parent 使现有 merge 可以直接在 children 中查找候选：

- class 前置声明与定义合并；
- 函数声明与定义按规范化参数签名合并；
- extern 变量声明与定义合并；
- 模板主声明、偏特化和显式特化仍按各自规则区分；
- 函数重载不得合并；
- 不同匿名 namespace 不得合并；
- 多份前置声明仍需保留正确的失效来源。

using 的目标指向稳定的实际 Statement，后续定义合并不会改变目标身份。

## 8. 分阶段实施计划

实施过程中严禁擅自修改设计或扩大范围。发现问题时严禁打补丁或增加兜底逻辑；必须停止相关实现，生成可复现的测试用例，记录问题现象、根因分析和影响范围，报告并等待明确指示后再继续。不得通过修改测试预期迎合现有实现，也不得提交覆盖本次改动且仍未通过的阶段性工作。

### 阶段 1：StatementFactory

- 为所有 StatementKind 增加工厂方法；
- 将直接 make_shared 创建迁移到工厂；
- 不改变行为；
- 运行现有完整测试。

### 阶段 2：有限 Statement 类系

- 引入平级的 Statement、NamespaceStatement 和 InlineAliasStatement，共同继承 AbstractStatement；
- 普通声明字段保留在具体类型中，调用方通过 dynamic_pointer_cast 显式访问，不增加属性虚函数或 data()；
- InlineAliasStatement 只保存目标实际 fullName，普通 using 与 InlineAlias 的名称解析共用现有 Resolver 路径；
- 此阶段暂不唯一化 namespace，也不改变来源、失效和可见性逻辑；
- 验证构建、跳转、失效和锁边界不变。

### 阶段 3：occurrence 与唯一命名 namespace

- 实现 occurrence 添加、删除、inline 重算和多位置查询；
- namespace 重开按实际 parent + 名称复用；
- 保持现有using 之外的 namespace 查找暂时可用，直到调用方迁移完成；
- 完成同文件、跨文件、跨根、replay 和部分失效测试。

### 阶段 4：using 投影

- 在 inline/匿名 namespace 成员完成 merge 后创建 InlineAliasStatement；
- 实现多层投影、直接目标解析和幂等创建；
- 将 InlineAliasStatement 正常加入 FileInfo；
- 验证补全、树形、跳转、hint 和 function tips。

### 阶段 5：删除旧透明索引

- Resolver、Builder、findNamespace、重载/特化收集和补全全部迁移到唯一实体 + using；
- 删除透明 namespace 索引及不再需要的 DFS；
- 不保留两套并行语义作为兜底；
- 清理临时插桩和过渡代码。

### 阶段 6：完整验收

- 运行全部 parser、tokenizer、widget 和 IDE 构建测试；
- 使用 GCC 8.1 与 GCC 15.3 标准库验收；
- Direct Release 测量完整 parse、失效和容器 eval；
- 与原基线比较调用次数、耗时和索引/Statement 数量；
- 不用 cache 命中掩盖重复操作。

## 9. 测试矩阵

### 9.1 唯一实体与来源

- 同一文件、不同文件和不同边界根重开命名 namespace；
- 多层 `namespace A::B::C`；
- occurrence 的实际文件、归属和范围；
- 删除一个来源后其它成员和位置保留；
- 最后一个来源删除后空壳消失；
- 同一头文件被多个根包含及 replay。

### 9.2 inline 与 using

- 单层 inline 的类型、变量、函数和嵌套 namespace；
- 外层 inline + 中间普通 + 内层 inline；
- 全 inline 多层链；
- late inline、相反解析顺序和部分失效；
- 匿名及嵌套匿名 namespace；
- 同一目标的重复构建不产生重复 using；
- 多个重载和同名候选不丢失；
- `std::chrono::_V2` 不提升普通 chrono，顶层 `std::_V2` 保持合法可见。

### 9.3 声明合并

- class 前置声明和定义；
- 函数多次声明、定义和重载；
- extern 变量声明和定义；
- 模板主声明、偏特化、显式特化；
- 声明或定义单独失效后的 using 目标完整性。

### 9.4 IDE 查询

- 普通和 inline 路径的 findStatement/findStatementOf；
- findNamespace 返回实际目标；
- 补全、goto declaration/definition、parse hint、function tips；
- 树形同时显示实际声明和 using；
- class 基类成员查找保持正确。

### 9.5 标准库与性能

- GCC 8.1/15.3 的 std::string、容器、迭代器、智能指针和 algorithm 调用链；
- bits/stdc++.h 完整解析与稳定重扫；
- vector/map/unordered_map 等现有 eval 基线；
- 失效后的重建和根切换不产生重复 Statement 或 using。

每个阶段都应把调试中发现的有效场景保留为正式断言，同时检查中间作用域、目标身份和位置，而不只检查最终端到端结果。
