# std::function 表达式求值性能基线

## 1. 场景与方法

- 对照提交：`48119372 Fix std::function evaluation`。
- 工具链：GCC 15.3 + libstdc++，C++17，Qt 5 Release；直接运行 `test-cppparser`，不使用 gdb。
- 测试入口：`TestCppParserStandardLibrary::test_standard_library_function_eval`。
- 每个场景先完整解析 `<functional>` 和 `<string>`，再单独测量一次表达式求值；parse 时间不计入 eval。
- 无插桩数据为连续三次测量的中位数。详细插桩数据只用于分析调用结构，不能与无插桩耗时直接比较。

用例过滤及详细 profile 的复现方式：

```sh
REDPANDA_STDLIB_FUNCTION_CASE='identity = functionPointer' \
REDPANDA_PARSER_PROFILE=1 REDPANDA_PARSER_TRACE=1 \
./test-cppparser --suite=TestCppParserStandardLibrary \
  test_standard_library_function_eval
```

另一个场景使用 `REDPANDA_STDLIB_FUNCTION_CASE='func(std::string())'`。不设置过滤变量时运行全部场景。正式耗时对比应去掉 `REDPANDA_PARSER_PROFILE` 和 `REDPANDA_PARSER_TRACE`。

## 2. 无插桩基线

| 表达式 | 三次 eval 耗时（ms） | 中位数（ms） | 结果类型 |
| --- | ---: | ---: | --- |
| `identity = functionPointer` | 0.466、0.481、0.454 | **0.466** | `std::function<int (int)> &` |
| `func(std::string())` | 0.403、0.329、0.360 | **0.360** | `void` |

两次求值均成功。当前专项中，赋值是最慢场景；类类型实参调用次之。

## 3. 详细插桩调用结构

以下阶段耗时包含子调用，不能相加为总耗时。`parseTypeNameSegments` 为 self-time。

| 指标 | 赋值 | `std::string` 实参调用 |
| --- | ---: | ---: |
| 插桩后 eval（ms） | 1.430 | 1.059 |
| `resolveType`：ms / 次数 | 1.456 / 31 | 0.649 / 17 |
| `resolveSegments`：ms / 次数 | 0.979 / 16 | 0.326 / 7 |
| `declarationLookup`：ms / 次数 | 0.335 / 75 | 0.080 / 14 |
| `declarationBindings`：ms / 次数 | 0.226 / 22 | 0.100 / 6 |
| `selectSpecialization`：ms / 次数 | 0.046 / 19 | 0.034 / 5 |
| `parseTypeNameSegments` self-time（ms） | 0.324 | 0.267 |

## 4. 热点判断

1. **赋值的额外候选转换与约束链最突出。** `reference_wrapper<_Functor>` 候选会继续进入 `__not_same`、`is_same`、`remove_cvref`、`enable_if` 等求值。与类类型实参调用相比，赋值的类型解析次数为 31 对 17，声明查找为 75 对 14。这部分是赋值慢于普通 `operator()` 的主要原因。
2. **限定名分段解析是两个场景的共同热点。** 其 self-time 分别占插桩 `resolveType` 耗时的约 22% 和 41%。后续可优先检查：已经具有 declaration、结构化模板实参和 derivations 的 `TypeInfo`，是否仍会重复走文本 tokenizer/segment 解析。
3. **赋值返回类型解析是必要的真实工作。** 偏特化注入类名、函数签名参数包和返回引用都必须保留，不能通过跳过解析或使用名称兜底来降低耗时。
4. 当前数据没有显示排序、QSet 插入或 UI 是主要热点。原始无响应问题已由正确性修复消除；本轮数据用于后续缩小优化范围，不证明所有 `std::function` 用法都不存在性能问题。

## 5. 后续优化约束

- 若先处理低成本精确候选，仍须完整遵守重载优先级；不能在存在潜在更优候选时提前返回，也不能跳过会影响结果的约束。
- 若增加结构化类型快速路径，必须保持 declaration、lexical context、binding frame 和偏特化选择语义一致，并补充等价性与缓存命中测试。
- 每项优化均使用上述相同场景比较无插桩中位数，同时运行完整 parser 及 Release/Debug 全量回归。
