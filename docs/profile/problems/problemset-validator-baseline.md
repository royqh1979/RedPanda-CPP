# 试题集期望输出比对性能基线

## 环境与方法

- 日期：2026-11-22
- 平台：Windows，MinGW GCC 8.1.0，Qt 5.15.18 static Release
- 基线：`master` 的 `b7a79e93` 加 `test-problemset` 测试脚手架；生产验证器尚未修改。
- 数据：运行时生成的 `line N value\n`，外部期望文件为 UTF-8；实际输出与期望完全相同，或仅最后一行不同。
- 工具：`test-problemset` 中由 `REDPANDA_BUILD_PROBLEMSET_PROFILE_TESTS` 控制的 Qt Test 基准，不计入样本生成和写文件时间。
- 直接运行 Release 可执行文件，不使用调试器。下表取后两轮的一致结果；首轮大样本只迭代一次且受运行环境波动影响，不用于判断趋势。

运行命令：

```sh
cmake -S . -B R:/build-redpanda-qt5-release \
    -DREDPANDA_BUILD_PROBLEMSET_PROFILE_TESTS=ON
cmake --build R:/build-redpanda-qt5-release --target test-problemset -j 8
R:/build-redpanda-qt5-release/RedPandaIDE/test-problemset.exe \
    --suite=TestProblemCaseValidator test_large_expected_file_profile
```

类型 0 为精确比对，类型 1 为忽略行首尾空白，类型 2 为忽略所有空白。单位为毫秒。

| 行数 | 类型 | 完全相同 | 最后一行不同 |
|---:|---:|---:|---:|
| 10,000 | 0 | 2.28–2.44 | 2.25–2.28 |
| 10,000 | 1 | 2.41 | 2.00–2.13 |
| 10,000 | 2 | 7.50–7.63 | 7.75–8.00 |
| 100,000 | 0 | 16.5–18 | 73–75 |
| 100,000 | 1 | 81 | 79–82 |
| 100,000 | 2 | 281 | 273–287 |
| 500,000 | 0 | 473–502 | 450–458 |
| 500,000 | 1 | 495–507 | 480–496 |
| 500,000 | 2 | 1538–1545 | 1490–1529 |

## 结论与限制

- 大文件成本随规模明显增长；“忽略所有空白”因每行构造两份字符串列表而最贵。
- 当前测试只测 `ProblemCaseValidator::validate()`，不包含进程输出采集、MainWindow 展示和 UI 阻塞时间。
- 500,000 行每轮只迭代一次，适合发现数量级变化，不适合证明少量百分比收益。
- 后续优化必须使用相同样本和配置复测，并同时验证差异行、行数和编码语义。
