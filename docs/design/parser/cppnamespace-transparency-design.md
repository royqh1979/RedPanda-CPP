# C++ namespace 透明性设计

## 1. 现有模型

StatementModel 使用一张 namespace 多值弱引用索引：

- key 为 namespace 的实际 fullName 时，登记该实际 namespace Statement；
- 对 inline/匿名 namespace，还在其直接父 namespace 的 fullName 键下登记该子 Statement。

因此同一索引结果可能包含两类节点：fullName 精确等于 key 的同名节点，以及实际 fullName 不同的直接透明子节点。该索引同时服务最长 namespace 前缀匹配和成员透明遍历，但两种用途必须区分节点类型。

namespace 重开沿用现有声明合并策略，Resolver 的最长前缀匹配和成员 DFS 保持不变。

## 2. 同名组与透明性

一个 namespace 同名组是否透明，只能由实际 fullName 与该组相同的节点决定：

- 匿名节点透明；
- 任一精确同名节点带 inline 属性时，该组透明；
- fullName 不同的透明子节点不得参与同名组判断。

例如：

```cpp
namespace outer {
    namespace regular {
        inline namespace abi {
            int value;
        }
    }
}
```

`abi` 对 `regular` 透明，但 `regular` 不会对 `outer` 透明。处理 regular 重开时，即使索引结果同时包含 regular 和 abi，也只能用 regular 节点判断 regular 同名组的属性。

真实标准库中的同类场景是 `std::chrono::_V2`：`_V2` 是 chrono 的 inline 子节点，不使普通 namespace chrono 成为 std 的透明子节点。顶层 `std::_V2` 则是独立存在的合法 inline namespace，两者不能混为一谈。

## 3. 直接透明边与 DFS

`transparentNamespaceStatements(name)` 从索引中筛选 name 下的透明节点，成员查找通过现有栈 DFS 逐层穿透直接透明子节点。

透明边沿实际父子关系延伸，不产生组合路径展开。visited 按 Statement 身份去重，避免同名重开或空 fullName 匿名节点造成重复处理。

## 4. 失效

文件失效沿用现有单 Statement 来源规则。删除 namespace 时，同名组的父层关联删除和透明性重算只使用精确同名节点；实际透明子节点的清理由 parentScope 关系单独负责，不得在父节点删除后重新插入其子节点别名。索引不得保留指向已删除节点的关联。

## 5. 回归验收

最小回归必须同时检查：

- 普通 regular 含 inline abi 并重开后，regular 同名组可以正确查询；
- abi 出现在 regular 的透明结果中；
- regular/abi 不出现在 outer 的透明结果中；
- `outer::regular::value` 可查，`outer::value` 不可查；
- 文件失效后相关索引项全部清除；
- 既有 namespace、模板和标准库测试保持通过。

性能验证使用 Direct Release、GCC 15.3 系统头和相同 profile 项，与修复前基线比较 namespace/member 查找次数及耗时，确认无关分支消失且没有引入算法回退。
