# Parse 阶段声明/定义合并 Profile

## 场景与方法

- 基线提交：`64be3d88`（`WIP: Add standard library public API coverage`）。
- 候选分支：`parse-time-declaration-merge-profile`。
- 构建：Release，Qt 5.15.18 静态构建，GCC 8.1 构建工具，直接运行可执行文件，不使用调试器。
- GCC 标准库：GCC 15.3 / libstdc++。
- Clang 标准库：Clang 23.1.5 / libc++。
- 场景：`TestCppParserStandardLibrary::test_standard_library_parse_baseline`，完整解析 `bits/stdc++.h`。
- 基线和候选分别独立构建，各运行 9 次，比较中位数；候选同时记录 Statement 数量。

测量脚本为 `RedPandaIDE/test/resources/stdlib/compare_parse_baseline.py`，只依赖 Python 标准库。用法：

```sh
python RedPandaIDE/test/resources/stdlib/compare_parse_baseline.py \
  R:/parse-baseline-timing-gcc.log R:/parse-merge-timing-gcc.log
```

输入为多次 `test_standard_library_parse_baseline` 的日志，输出运行次数、最小/最大/中位耗时、百分比变化和 Statement 变化。

## Parse 结果

| 实现 | 基线中位数 | 候选中位数 | 变化 | 变化率 |
|---|---:|---:|---:|---:|
| GCC 15.3 | 994 ms | 960 ms | -34 ms | -3.42% |
| Clang 23.1.5 | 834 ms | 796 ms | -38 ms | -4.56% |

9 次样本：

- GCC 基线：981、991、988、997、1010、993、1008、1013、994 ms；候选：959、967、960、949、976、956、971、999、939 ms。
- Clang 基线：824、826、817、823、841、834、842、840、839 ms；候选：796、800、792、787、792、804、797、791、837 ms。

Statement 数量：

| 实现 | 基线 | 候选 | 减少 | 减少比例 |
|---|---:|---:|---:|---:|
| GCC 15.3 | 61169 | 60082 | 1087 | 1.78% |
| Clang 23.1.5 | 58945 | 56638 | 2307 | 3.91% |

候选没有增加完整 Parse 中位耗时，且减少了重复 Statement，符合采用 KISS 方案的验收条件。不同构建目录和运行批次的绝对耗时不能直接混用；本表只用于同一实验中的相对比较。

## 正确性与边界

最小测试覆盖：

- 模板形参原文与位置编号、嵌套作用域、遮蔽和 checkpoint 恢复。
- 成员名和成员模板名不被误替换，不确定时保留原文并输出 `qWarning()`。
- 类内声明与改名后的类外定义生成相同规范签名，原始类型名不变。
- 空参数包 `operator()`、函数模板部分显式实参与默认实参。
- `const T&` 与 `const value_type&` 的 Eval 等价处理。
- `using Base<T>::member` 的基类成员展开。
- friend 非成员候选与 const 成员运算符共存。

真实标准库场景覆盖 GCC、Clang 的 `std::function`、容器、算法和公开接口基准。Eval 逐场景数据只用于检查回归，不作为本方案扩大实现范围或重写重载解析的理由。

## Member Alias 展开复测

初版 qualified member Alias 展开在全容器场景触发 Clang 47 次、GCC 21 次，其中大量 `size_type`、`rebind` 等类型 Alias 返回空成员列表，属于不必要的递归类型解析。改为只在函数调用路径展开后，Clang 降为 0 次、GCC 降为 1 次。未引入 Alias 专用缓存。

`optional.value` 的端到端耗时没有显著下降，因此该修改只证明消除了不必要的 Alias 工作，不宣称解决全部 Eval 成本。`using Base<T>::get` 与 `using Base<T>::reference` 均保留最小测试。

## 最终验证

- GCC、Clang 完整 `test-cppparser` 均通过；双方 C++17/C11 声明查找缺失均为 0。
- Release 全量构建及 7 个 CTest Suite 全部通过，总耗时 57.68 秒。
- Debug 全量构建及 7 个 CTest Suite 全部通过，总耗时 132.59 秒。
- 公开接口基准开关在 Release/Debug 默认配置中均为 `OFF`。
