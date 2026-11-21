# QSynEdit Document / Document+View 编辑性能基线

## 1. 基线说明

本文持久记录优化前的完整 profile 基线，供后续优化做同场景对比。热点分析和优化建议见 [QSynEdit Document / Document+View 编辑性能分析报告](qsynedit-edit-performance-report.md)。

- 基线代码：`master` 的 `678821c2`，仅加入 profile 测试，没有修改生产实现；
- 记录日期：2026-11-22；
- 测试负载：`libs/qsynedit/test/resources/miniaudio.h`，95844 行；
- 环境：Windows 11，Qt 5.15.18 静态 Release，`d:/qt/mingw64` GCC 8.1.0，R: 内存盘，Qt offscreen；
- View 可见，关闭 word-wrap，开启 C++ syntaxer；除折叠开关对照外均开启代码折叠；
- 文件加载、初始语法分析、初始布局和场景准备不计入编辑耗时。

这些数值用于比较代码路径和数量级，不等同于真实窗口、显卡和老机器上的最终体验。后续对比应使用相同负载、构建类型、环境变量、采样次数和时间口径；环境变化时应另建基线，不直接覆盖本文数据。

## 2. 场景与字段

五类编辑均在一个 Document 更新批次内完成：

- `modify`：修改已有的 N 行；
- `insert`：插入 N 行并写入正式内容；
- `delete`：删除 N 行；
- `replace`：删除 N 行、插入 N 行并写入正式内容；
- `move`：把连续 N 行向下移动 100 行，保持块内行顺序。现有接口只有单行 `moveLine()`，因此由 N 次单行移动组成。

`plain` 不改变折叠分隔符，`fold` 增加、删除或移动 `{`/`}`。两类负载使用相同的声明主体和显示宽度，仅末尾分隔符不同。`begin`、`middle`、`end` 分别表示文档前部、中部和尾部。

- `sync`：编辑及同步信号处理完成的时间；
- `total`：继续处理事件直到后台布局完成的时间，包含 offscreen 绘制事件；Document 的 `total` 等于 `sync`；
- `reparsed lines`：`linesReparsed` 报告的累计工作量，同一行被多个 primitive 重复解析时重复计数；
- `change entries`：本次 Document 更新产生的 change 数量；
- `changed lines`：各 change 报告行数的累计值；
- `fold rescans`：View 发起的 fold rescan 次数。

除特别说明外，时间单位均为 ms。

## 3. Document 全矩阵基线

每个场景采样 3 次，表中取中位数。

| 操作 | 行数 | 负载 | 位置 | sync | 重解析行数 | change entries | changed lines |
|---|---:|---|---|---:|---:|---:|---:|
| modify | 1 | plain | begin | 0.005 | 1 | 1 | 1 |
| modify | 1 | plain | middle | 0.005 | 1 | 1 | 1 |
| modify | 1 | plain | end | 0.005 | 1 | 1 | 1 |
| modify | 1 | fold | begin | 119.470 | 92109 | 1 | 1 |
| modify | 1 | fold | middle | 58.463 | 47923 | 1 | 1 |
| modify | 1 | fold | end | 1.214 | 1001 | 1 | 1 |
| modify | 10 | plain | middle | 0.026 | 10 | 1 | 10 |
| modify | 10 | fold | middle | 0.030 | 12 | 1 | 10 |
| modify | 100 | plain | middle | 0.177 | 100 | 1 | 100 |
| modify | 100 | fold | middle | 0.196 | 101 | 1 | 100 |
| modify | 1000 | plain | begin | 1.698 | 1000 | 1 | 1000 |
| modify | 1000 | plain | middle | 1.864 | 1000 | 1 | 1000 |
| modify | 1000 | plain | end | 1.697 | 1000 | 1 | 1000 |
| modify | 1000 | fold | begin | 1.832 | 1001 | 1 | 1000 |
| modify | 1000 | fold | middle | 1.984 | 1000 | 1 | 1000 |
| modify | 1000 | fold | end | 1.932 | 1000 | 1 | 1000 |
| modify | 10000 | plain | middle | 18.313 | 10000 | 1 | 10000 |
| modify | 10000 | fold | middle | 19.460 | 10001 | 1 | 10000 |
| insert | 1 | plain | middle | 0.065 | 2 | 2 | 2 |
| insert | 1 | fold | middle | 58.287 | 47925 | 2 | 2 |
| insert | 10 | plain | middle | 0.083 | 20 | 2 | 20 |
| insert | 10 | fold | middle | 0.079 | 20 | 2 | 20 |
| insert | 100 | plain | middle | 0.272 | 201 | 2 | 200 |
| insert | 100 | fold | middle | 0.292 | 201 | 2 | 200 |
| insert | 1000 | plain | middle | 2.429 | 2000 | 2 | 2000 |
| insert | 1000 | fold | middle | 2.576 | 2000 | 2 | 2000 |
| insert | 10000 | plain | middle | 24.221 | 20000 | 2 | 20000 |
| insert | 10000 | fold | middle | 25.669 | 20000 | 2 | 20000 |
| delete | 1 | plain | middle | 0.462 | 1 | 1 | 1 |
| delete | 1 | fold | middle | 60.067 | 47922 | 1 | 1 |
| delete | 10 | plain | middle | 0.520 | 10 | 1 | 10 |
| delete | 10 | fold | middle | 0.507 | 10 | 1 | 10 |
| delete | 100 | plain | middle | 0.588 | 100 | 1 | 100 |
| delete | 100 | fold | middle | 0.666 | 100 | 1 | 100 |
| delete | 1000 | plain | middle | 1.943 | 1000 | 1 | 1000 |
| delete | 1000 | fold | middle | 1.881 | 1000 | 1 | 1000 |
| delete | 10000 | plain | middle | 14.753 | 10000 | 1 | 10000 |
| delete | 10000 | fold | middle | 14.800 | 10000 | 1 | 10000 |
| replace | 1 | plain | middle | 0.516 | 2 | 3 | 3 |
| replace | 1 | fold | middle | 59.468 | 47924 | 3 | 3 |
| replace | 10 | plain | middle | 0.481 | 20 | 3 | 30 |
| replace | 10 | fold | middle | 0.490 | 22 | 3 | 30 |
| replace | 100 | plain | middle | 0.697 | 200 | 3 | 300 |
| replace | 100 | fold | middle | 0.719 | 201 | 3 | 300 |
| replace | 1000 | plain | middle | 3.003 | 2000 | 3 | 3000 |
| replace | 1000 | fold | middle | 3.096 | 2000 | 3 | 3000 |
| replace | 10000 | plain | middle | 25.419 | 20000 | 3 | 30000 |
| replace | 10000 | fold | middle | 26.506 | 20001 | 3 | 30000 |
| move | 1 | plain | middle | 0.681 | 103 | 1 | 1 |
| move | 1 | fold | middle | 59.255 | 47923 | 1 | 1 |
| move | 10 | plain | middle | 5.934 | 1010 | 10 | 10 |
| move | 10 | fold | middle | 5.889 | 1011 | 10 | 10 |
| move | 100 | plain | middle | 59.394 | 10100 | 100 | 100 |
| move | 100 | fold | middle | 58.994 | 10101 | 100 | 100 |
| move | 1000 | plain | middle | 575.115 | 101000 | 1000 | 1000 |
| move | 1000 | fold | middle | 577.733 | 101001 | 1000 | 1000 |
| move | 10000 | plain | middle | 6271.859 | 1010000 | 10000 | 10000 |
| move | 10000 | fold | middle | 6382.483 | 1010001 | 10000 | 10000 |

## 4. Document+View 全矩阵基线

每个场景采样 1 次。关键场景的 3 次中位数和折叠开关对照见后续章节，不能用本章的单次结果替代那些中位数。

| 操作 | 行数 | 负载 | 位置 | sync | total | 重解析行数 | change entries | changed lines | fold rescans |
|---|---:|---|---|---:|---:|---:|---:|---:|---:|
| modify | 1 | plain | begin | 7.796 | 8.286 | 1 | 1 | 1 | 0 |
| modify | 1 | plain | middle | 6.157 | 6.623 | 1 | 1 | 1 | 0 |
| modify | 1 | plain | end | 7.154 | 7.657 | 1 | 1 | 1 | 0 |
| modify | 1 | fold | begin | 138.145 | 138.648 | 92109 | 1 | 1 | 1 |
| modify | 1 | fold | middle | 75.753 | 76.231 | 47923 | 1 | 1 | 1 |
| modify | 1 | fold | end | 8.350 | 8.801 | 1001 | 1 | 1 | 1 |
| modify | 10 | plain | middle | 6.443 | 6.919 | 10 | 1 | 10 | 0 |
| modify | 10 | fold | middle | 8.406 | 8.898 | 12 | 1 | 10 | 1 |
| modify | 100 | plain | middle | 9.997 | 10.571 | 100 | 1 | 100 | 0 |
| modify | 100 | fold | middle | 11.288 | 11.791 | 101 | 1 | 100 | 1 |
| modify | 1000 | plain | begin | 45.565 | 46.040 | 1000 | 1 | 1000 | 0 |
| modify | 1000 | plain | middle | 46.008 | 46.475 | 1000 | 1 | 1000 | 0 |
| modify | 1000 | plain | end | 44.934 | 45.389 | 1000 | 1 | 1000 | 0 |
| modify | 1000 | fold | begin | 47.293 | 47.746 | 1001 | 1 | 1000 | 1 |
| modify | 1000 | fold | middle | 46.287 | 46.773 | 1000 | 1 | 1000 | 1 |
| modify | 1000 | fold | end | 47.309 | 47.767 | 1000 | 1 | 1000 | 1 |
| modify | 10000 | plain | middle | 387.530 | 388.015 | 10000 | 1 | 10000 | 0 |
| modify | 10000 | fold | middle | 402.128 | 402.721 | 10001 | 1 | 10000 | 1 |
| insert | 1 | plain | middle | 10.805 | 11.371 | 2 | 2 | 2 | 0 |
| insert | 1 | fold | middle | 83.630 | 84.172 | 47925 | 2 | 2 | 1 |
| insert | 10 | plain | middle | 7.431 | 8.021 | 20 | 2 | 20 | 0 |
| insert | 10 | fold | middle | 12.032 | 12.553 | 20 | 2 | 20 | 1 |
| insert | 100 | plain | middle | 11.167 | 12.170 | 201 | 2 | 200 | 0 |
| insert | 100 | fold | middle | 12.387 | 12.945 | 201 | 2 | 200 | 1 |
| insert | 1000 | plain | middle | 51.395 | 51.940 | 2000 | 2 | 2000 | 0 |
| insert | 1000 | fold | middle | 48.497 | 49.022 | 2000 | 2 | 2000 | 1 |
| insert | 10000 | plain | middle | 394.411 | 395.832 | 20000 | 2 | 20000 | 0 |
| insert | 10000 | fold | middle | 416.649 | 418.057 | 20000 | 2 | 20000 | 1 |
| delete | 1 | plain | middle | 1.388 | 1.893 | 1 | 1 | 1 | 0 |
| delete | 1 | fold | middle | 72.717 | 73.290 | 47922 | 1 | 1 | 1 |
| delete | 10 | plain | middle | 1.195 | 1.710 | 10 | 1 | 10 | 0 |
| delete | 10 | fold | middle | 2.619 | 3.124 | 10 | 1 | 10 | 1 |
| delete | 100 | plain | middle | 1.351 | 1.870 | 100 | 1 | 100 | 0 |
| delete | 100 | fold | middle | 6.413 | 6.945 | 100 | 1 | 100 | 1 |
| delete | 1000 | plain | middle | 2.753 | 3.330 | 1000 | 1 | 1000 | 0 |
| delete | 1000 | fold | middle | 44.917 | 45.472 | 1000 | 1 | 1000 | 1 |
| delete | 10000 | plain | middle | 16.298 | 16.802 | 10000 | 1 | 10000 | 0 |
| delete | 10000 | fold | middle | 395.237 | 395.750 | 10000 | 1 | 10000 | 1 |
| replace | 1 | plain | middle | 6.917 | 7.364 | 2 | 3 | 3 | 0 |
| replace | 1 | fold | middle | 78.499 | 78.999 | 47924 | 3 | 3 | 1 |
| replace | 10 | plain | middle | 7.140 | 7.622 | 20 | 3 | 30 | 0 |
| replace | 10 | fold | middle | 8.616 | 9.106 | 22 | 3 | 30 | 1 |
| replace | 100 | plain | middle | 11.030 | 11.501 | 200 | 3 | 300 | 0 |
| replace | 100 | fold | middle | 12.565 | 13.085 | 201 | 3 | 300 | 1 |
| replace | 1000 | plain | middle | 47.461 | 47.941 | 2000 | 3 | 3000 | 0 |
| replace | 1000 | fold | middle | 49.174 | 49.665 | 2000 | 3 | 3000 | 1 |
| replace | 10000 | plain | middle | 407.933 | 408.432 | 20000 | 3 | 30000 | 0 |
| replace | 10000 | fold | middle | 416.091 | 416.678 | 20001 | 3 | 30000 | 1 |
| move | 1 | plain | middle | 1.624 | 2.087 | 103 | 1 | 1 | 0 |
| move | 1 | fold | middle | 72.033 | 72.545 | 47923 | 1 | 1 | 1 |
| move | 10 | plain | middle | 8.026 | 8.522 | 1010 | 10 | 10 | 0 |
| move | 10 | fold | middle | 9.347 | 9.811 | 1011 | 10 | 10 | 1 |
| move | 100 | plain | middle | 75.778 | 76.288 | 10100 | 100 | 100 | 0 |
| move | 100 | fold | middle | 76.428 | 77.330 | 10101 | 100 | 100 | 1 |
| move | 1000 | plain | middle | 730.746 | 731.246 | 101000 | 1000 | 1000 | 0 |
| move | 1000 | fold | middle | 736.252 | 736.739 | 101001 | 1000 | 1000 | 1 |
| move | 10000 | plain | middle | 7487.583 | 7488.047 | 1010000 | 10000 | 10000 | 0 |
| move | 10000 | fold | middle | 8007.134 | 8007.694 | 1010001 | 10000 | 10000 | 1 |

## 5. View 关键场景中位数与折叠开关对照

每个场景、每种折叠设置分别采样 3 次并取中位数。位置均为 `middle`。

| 操作 | 行数 | 负载 | 折叠 | sync | total | 重解析行数 | fold rescans |
|---|---:|---|---|---:|---:|---:|---:|
| modify | 1 | plain | 开 | 6.885 | 7.338 | 1 | 0 |
| modify | 1 | plain | 关 | 5.356 | 5.609 | 1 | 0 |
| insert | 1 | plain | 开 | 11.353 | 11.898 | 2 | 0 |
| insert | 1 | plain | 关 | 8.175 | 8.446 | 2 | 0 |
| modify | 10000 | plain | 开 | 381.477 | 381.952 | 10000 | 0 |
| modify | 10000 | plain | 关 | 389.956 | 390.167 | 10000 | 0 |
| delete | 10000 | fold | 开 | 418.260 | 418.784 | 10000 | 1 |
| delete | 10000 | fold | 关 | 37.916 | 38.200 | 10000 | 0 |
| move | 1000 | plain | 开 | 721.315 | 721.892 | 101000 | 0 |
| move | 1000 | plain | 关 | 581.253 | 581.541 | 101000 | 0 |

折叠关闭时，`fold` 仍表示负载中的文本包含折叠分隔符变化，只是 View 不执行代码折叠维护。

## 6. View 固定成本微型 profile

两项均在 95844 行、layout 已全部有效的 Document+View 上测量，各执行 5 次。

| 测量项 | 第 1 次 | 第 2 次 | 第 3 次 | 第 4 次 | 第 5 次 | 中位数 |
|---|---:|---:|---:|---:|---:|---:|
| `scheduleWrapLayoutJob()` 的 pending 行收集 | 7.589 | 5.439 | 5.106 | 5.129 | 5.314 | 5.314 |
| `syncFoldVisibility()` | 0.902 | 0.638 | 0.593 | 0.955 | 0.970 | 0.902 |

微型 profile 只测量指定代码段，不包含完整编辑、后台布局或绘制。

## 7. 复现与后续对比

构建开关、运行命令和 CSV 输出前缀见分析报告第 6 节；日志汇总脚本及其输入输出说明见 [QSynEdit 编辑性能 Profile 工具](qsynedit-edit-performance-tools.md)。原始输出前缀为：

- `DOCUMENT_EDIT_PROFILE`：Document 全矩阵；
- `VIEW_EDIT_PROFILE`：View 全矩阵、关键场景及折叠开关对照；
- `VIEW_OVERHEAD_PROFILE`：View 固定成本微型 profile。

`QSYNEDIT_BUILD_PROFILE_TESTS` 默认 OFF。全矩阵使用 `QSYNEDIT_EDIT_PROFILE_REPEAT=3`（Document）或 `1`（View），关键场景使用 `3`，微型 profile 固定采样 `5` 次；View 可用 `QSYNEDIT_EDIT_PROFILE_FOLDING=0` 关闭折叠。

后续优化结果应同时记录优化前基线版本、优化后提交、环境差异、每次原始样本和中位数。除了耗时，还应核对重解析行数、change 数量、fold rescan 次数及正确性测试，避免以减少必要工作为代价获得虚假的性能收益。
