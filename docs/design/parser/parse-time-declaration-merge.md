# Parse 阶段声明/定义合并实验

## 目标

在不把模板实例化移入 Parser 的前提下，让源码中同一函数实体的声明和定义尽量在 StatementBuilder 阶段合并，减少 Eval 重载选择中的重复候选。显式模板实参必须绑定到函数自身的模板形参，而不是外围类模板形参。

本实验先评估 Parse 归一化的成本，不预先删除 Eval 的等价候选保护。只有在正确性和 profile 均支持时，才继续评估能否将 Eval 等价判断简化为 Statement 身份去重。

## 根因与不变量

成员函数的模板前缀可能同时包含外围类和函数自身的形参：

```cpp
template<class R, class... Args>
struct Function<R(Args...)> {
    R operator()(Args...);
    template<class T, class U = double> T* target();
};

template<class R, class... Args>
R Function<R(Args...)>::operator()(Args...) { }

template<class R, class... Args>
template<class T, class U>
T* Function<R(Args...)>::target() { return nullptr; }
```

`operator()` 的函数自身模板形参为空，`target` 的函数自身模板形参为 `T, U`。完整模板形参列表仍用于声明合并和默认实参继承，但必须明确函数自身部分。

当前实现有两个独立表现、同一边界缺失的问题：

- 类内声明与类外定义的规范签名不同，导致同一 `operator()` 被建成两个 Statement。空参数包使两个候选同时可行，Eval 将它们误判为不同重载。
- `target<int>()` 从完整列表的索引起点绑定显式实参，把 `int` 错绑到外围类的 `R`，而非函数自身的 `T`。

实验必须保持以下不变量：

1. 同一源码实体的声明和定义只产生一个逻辑 Statement；Eval 不负责弥补可避免的 Parse 重复。
2. 不同 Statement 即使实例化后签名相同，也不能仅因此视为同一实体。
3. 显式实参和函数模板默认实参只作用于函数自身形参；外围类形参来自 outer bindings。
4. 不针对 GCC、Clang、libstdc++ 或 libc++ 写名称特判，不把模板实例化移入 Parse。

## 实验方案：Parse 全量位置归一化

DeclarationParser 为模板形参按“外围到内层”的位置分配 `@T`、`@N`、`@TT` 编号，并在读取参数 token 时同步生成规范文本。原始 `TypeInfo`、默认实参和表达式继续保存原名，不做全局替换。

模板上下文按作用域保存和恢复：

- 类内成员声明：class 上下文提供外围类形参，当前 declaration 再追加函数自身形参。
- 类外成员定义：完整声明已经包含外围前缀，直接按位置归一化。
- 自由函数：全部模板前缀都属于函数，按位置归一化。

`parseParameter()` 在默认实参前生成规范文本，跳过 declarator 名称；`parseFunctionSuffix()` 拼接参数和函数限定符后清空临时文本。未发生替换的参数仍走原结构化渲染，避免改变既有签名格式。`.`, `->`, `::` 后的成员名和成员模板名保留原文；命中模板形参但角色无法确定时用 `qWarning()` 记录文件、行列和前后 token，并保守保留原文。

位置归一化同时容忍外围类形参和函数形参改名，使类内声明与类外定义得到相同的 `normalizedArgs`，不再需要对渲染后的签名重新 Tokenize。StatementBuilder 复制该签名，并通过 `declarator.qualifier` 区分：

- 空 qualifier：类内声明或自由函数，DeclarationInfo 中的模板形参全部属于当前函数。
- 非空 qualifier：类外成员定义，DeclarationInfo 末尾才是函数自身形参；外围数量由 context 中的外围类链计算。

Statement 继续保存完整 `templateParameters`，另存函数自身形参数量。合并声明和定义时保留该数量。Eval 只在函数自身后缀中绑定显式实参和默认实参。

`const T&` 与 `const value_type&` 等 alias 差异无法在 Parse 阶段可靠合并，仍由 Eval 在模板替换后比较参数类型。`@` 编号不再使该保护提前失败；这层处理不等同于把全部语义合并移回 Eval。

`using Base<T>::member` 形成的 qualified member Alias 在 Eval 中展开：先解析 Alias 左侧的基类类型，再复用基类成员查找和绑定，全部重载采用 using 声明处的可访问性。未限定的类型 Alias 不走该路径。非 const 成员优先于 const 成员，但 friend 非成员候选不再抑制 const 成员；这避免 friend 参数不匹配时错误丢失合法的 const 成员运算符。

## KISS 与范围审核

本批只修改：

- DeclarationParser 的模板上下文、参数规范文本和函数规范签名生成。
- StatementBuilder 的规范签名复制、函数自身模板形参元数据及其合并。
- `matchFunctionCandidate()` 的显式实参与默认实参绑定范围，以及 Eval 对 alias 等价声明的既有保护。
- `memberCandidates()` 的 qualified member Alias 展开和 friend/const 候选边界。
- 对应最小组件测试和真实标准库测试。

本批不修改 Tokenizer、Preprocessor、Statement 公共查询 API，不删除 Eval 的 `equivalentDeclaration()`，不重写整个重载解析。若后续要删除该函数，必须另建测试证明所有依赖场景已由 Parse 合并或 Statement 身份去重覆盖。

## 验收与 profile

正确性场景：

- 空参数包 `Function<void()>::operator()` 返回 `void`，不产生伪重载歧义。
- 类内声明加类外定义的函数模板，部分显式实参和后续默认实参绑定正确。
- 外围类模板形参与函数自身形参改名后仍能合并。
- 自由函数多级模板前缀和类内 inline 成员模板不回归。
- GCC、Clang 的 `std::function`、容器和算法场景全部通过。
- C/C++ 公开接口声明查找缺失保持为 0。

性能使用同一 Release 可执行文件，分别记录：

- `bits/stdc++.h` 完整 Parse 耗时和 Statement 数量。
- 容器、算法和 `std::function` 的逐场景 Eval 耗时与失败数。
- 双方完整 Parser 测试，以及最终 Release/Debug 全量回归。

若 Parse 归一化代价明显，再比较更精细的“外围名称规范化 + 函数自身位置规范化”方案；不得只看单次总耗时，应同时检查 Statement 数量和 Eval 是否实际受益。
