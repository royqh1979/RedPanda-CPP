# Statement、namespace 与可见性设计

本文统一管理 Statement 类系、StatementModel 所有权、StatementOccurrence、声明/定义来源、失效和可见性。namespace、InlineAlias 与透明性是其中的专题；occurrence 和失效模型适用于所有 Statement，不属于 namespace 专属设计。

## 1. 核心模型

全局具有相同实际 fullName 的命名 namespace 是同一个逻辑实体：

```cpp
namespace N { int a; }
namespace N { int b; }
```

无论在同一文件、不同文件或不同 TU 中重开，都复用同一个 NamespaceStatement，新增 occurrence，并把成员加入同一 children。唯一身份由实际 fullName 确定，而不是由“文件 + 名称”或每次源码出现的位置确定。嵌套 namespace 因实际父实体唯一化而逐层唯一。

匿名 namespace 不参与跨父实体或跨 TU 的全局唯一化。不同匿名 namespace 不得仅因 command 为空而合并。

Statement 的 fullName 和 parentScope 始终保留实际词法路径。`std::__cxx11::basic_string` 不因可见别名而改写为 `std::basic_string`。

## 2. Statement 类系与所有权

采用有限的平级类系：

```text
AbstractStatement
├── Statement
├── NamespaceStatement
└── InlineAliasStatement
```

AbstractStatement 保存公共身份、可见名称、实际父子关系、分类和 occurrence。普通 Statement 另存当前 definition 及 HasDefinition，definition 不重复存入 declarations，失效清空后不得用 declaration 回填。

InlineAliasStatement 保存可见名称、实际目标和目标 weak_ptr，不拥有目标，也不继承普通 Statement 的类型、参数、children、模板和基类字段。它使用独立的 `StatementKind::InlineAlias`。

不继续拆 VariableStatement、FunctionStatement 等完整类型树，不建立行为虚函数体系。调用方需要具体字段时先判断 StatementKind，再显式 dynamic_pointer_cast。

StatementFactory 负责实际 C++ 类型与 StatementKind 一致，以及 parentScope、command、fullName 等公共初始化：

- createNamespace() 创建 NamespaceStatement；
- 普通 class/function/variable 创建 Statement；
- 普通源码 Alias 创建 Statement；
- createInlineAlias() 创建 InlineAliasStatement，并保存目标 weak_ptr。

工厂不使用默认参数，不承担失效和可见性逻辑。StatementModel 是唯一所有权中心，children 和其它索引使用 weak_ptr。

## 3. StatementOccurrence 与失效归属

所有 Statement 使用 StatementOccurrence 保存：

- 实际文件名；
- SourceRange；
- 失效归属 boundaryAncestor；
- DeclSpecifierProperties。

namespace 的每次出现都登记 declaration occurrence；inline 属性保存在 occurrence 中。失效归属为：

```text
boundaryAncestor 非空：boundaryAncestor
否则：fileName
```

declarations 按失效归属建立 QMultiHash，value 保留实际文件、范围和声明属性。

文件失效时：

1. 删除该文件贡献的普通 Statement 和 InlineAlias；
2. 从 NamespaceStatement 删除对应 occurrences；
3. 仍有 occurrences 时保留实体，并重算导航位置和 inline 属性；
4. occurrences 为空时才删除 namespace 壳；
5. 不使用 shared_ptr::use_count() 判断是否仍有来源。

同一头文件被多个边界根包含时，各 occurrence 按归属分别保存；失效一个根不得删除其它根的合法贡献。

ParsedFileInfo 的 scope 索引可以指向共享的唯一 NamespaceStatement。namespace 跳转若只能返回一个位置，应优先选择与当前文件相关的 occurrence。

## 4. InlineAlias 投影

inline/匿名 namespace 的成员在实际 parent 中创建 InlineAliasStatement，表达直接父 namespace 中的可见性：

```text
std
├── __cxx11
│   └── basic_string
└── basic_string（InlineAlias → std::__cxx11::basic_string）
```

创建规则：

- 新增实际 Statement 时创建对应投影；
- 仅合并声明/定义时不重复创建；
- 新增函数重载时新增对应投影；
- replay 未新增 Statement 时不重复创建；
- 已有 namespace 首次获得 inline 标志时，为已有 children 补建投影。

多层 inline 逐层向直接 parent 投影，中间 InlineAlias 直接保存最终实际目标，不形成需要逐次解析的 alias 链。实际父子链无环，不生成 fullName 路径组合。

parent children 使用多值映射，保留函数重载和不同透明分支中的同名实体；查询与补全按目标身份和签名去重。

## 5. 直接 children 与 InlineAlias 可见性

StatementModel 不维护 namespace 专用的 fullName 索引，也不对重开 namespace 做透明分支 DFS。命名 namespace 已经按实际 fullName 合并，inline/匿名可见性由 InlineAlias 表达，因此不需要另一套平行索引。

NamespaceStatement 保留实际父子关系和实际 fullName。inline 属性根据剩余 declaration occurrences 重算；InlineAlias 只负责把实际成员投影到直接父 namespace。

例如 `regular` 内的 inline `abi` 通过 `regular::value` 形式的投影只对 `regular` 可见，不会使普通 `regular` 对 `outer` 透明。`std::chrono::_V2` 与顶层 `std::_V2` 也保持不同的实际父子关系。

文件失效时删除对应 occurrences 和 InlineAlias；没有额外的透明索引或父键关联需要维护。

## 6. 查找、补全与视图

名称查找沿实际 children 和 InlineAlias 目标进行：

- 普通子节点直接匹配名称段；
- InlineAlias 通过 weak_ptr 解析实际目标；
- 同一级存在多个合法候选时保留并遍历，按目标身份去重；
- class/enum 成员和基类图遍历沿用各自规则；
- 查询目标为 namespace 时必须得到实际 NamespaceStatement，不能把普通成员或 Alias 冒充 namespace。

`findNamespace()` 通过同一 Resolver 查找实际 NamespaceStatement。现有列表 API 是调用方兼容形式，不代表同一 fullName 应有多个逻辑实体。`includeInline` 在当前实现中没有作用，不再作为透明索引开关使用；是否删除该参数作为独立接口清理处理。

成员枚举区分语义查询和 UI 直接子节点：

- CppTypeResolver 的语义查询解析 InlineAlias 并处理继承；
- CppParser::membersOfStatement() 面向 UI，只返回直接 children；InlineAlias 自身没有 children；
- class 补全通过 resolveMembersOfClass() 处理继承成员和访问域；
- namespace 补全读取直接 children，并解析 InlineAlias。

树形视图可以同时显示实际声明和投影。using 的跳转、hint、function tips 和类型求值均指向实际目标。Class Browser 直接显示唯一实体，不需要为重开 namespace 创建 dummy Statement；Block/Lambda 的 DummyStatement 与此无关。

## 7. 声明与定义合并

唯一 namespace parent 使 StatementBuilder 能在 children 中查找 merge 候选：

- class 前置声明与定义合并；
- 函数声明与定义按规范化参数签名合并；
- extern 变量声明与定义合并；
- 函数重载保留全部候选；
- 模板主声明、偏特化和显式特化按各自规则区分；
- 不同匿名 namespace 不合并；
- 多份声明保留各自的失效来源。

InlineAlias 指向稳定的实际 Statement，后续定义合并不改变目标身份。

## 8. 验收矩阵

### 唯一实体与来源

- 同文件、不同文件和不同边界根重开命名 namespace；
- 多层 `namespace A::B::C`；
- occurrence 的实际文件、归属和范围；
- 删除一个来源后其它成员和位置保留；
- 最后一个来源删除后空壳消失；
- 同一头文件被多个根包含及 replay。

### inline 与透明性

- 单层和多层 inline 的类型、变量、函数及嵌套 namespace；
- 外层 inline + 中间普通 + 内层 inline；
- late inline、相反解析顺序和部分失效；
- 匿名及嵌套匿名 namespace；
- 重复构建和 replay 不产生重复投影；
- 同名函数重载不丢失；
- `std::chrono::_V2` 不提升普通 chrono，顶层 `std::_V2` 保持合法可见。

### 合并与 IDE 查询

- class 前置声明/定义、函数多声明/定义/重载和 extern 变量；
- 模板主声明、偏特化和显式特化；
- declaration 或 definition 单独失效后的目标完整性；
- 普通和 inline 路径的 findStatement/findStatementOf/findNamespace；
- 补全、导航、Parser Hint、function tips 和 Class Browser；
- 标准库完整解析、稳定重扫、根切换和失效不重复。

调试中形成的有效场景必须保留为正式断言，同时检查中间作用域、目标身份和位置，而不只检查最终结果。
