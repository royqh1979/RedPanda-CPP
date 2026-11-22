# QSynEdit 批量插入 Deferred 重解析实施结果

## 1. 实施范围

本项依据 [批量插入 Deferred 重解析设计](qsynedit-bulk-insert-deferred-design.md) 完成，父提交为 `c375eae6`，实施分支为 `document-edit-bulk-insert`。

新增 `Document::insertLinesDeferred()`，只创建空行和 Insert change，不解析临时空文本。普通 `insertLines()` 复用该入口创建行，随后仍按原规则重解析，独立 primitive 语义不变。普通多行粘贴改用 deferred，随后仍逐行 `putLine()` 写入正式内容。

正式内容和自动缩进顺序未改变：每行仍读取前一行文本和 syntax state，逐行解析并传播。没有采用预计算后一次性插入方案，因为仅预计算缩进仍为 2N，而保存整批预演 state 与 deferred 同为 N，却需要重写 lineSeq、fold-dirty、尾部收敛和 Undo/Redo 协议。

## 2. 测试与审核迭代

新增验证：

- Document deferred 插入 3 行，正式内容累计只解析 3 行，普通 `insertLines()` 仍解析插入行；
- 正式内容为空的行同样建立 state；
- C++ 多行粘贴的 Tab 缩进、括号层级和 Undo/Redo；
- 两个 profile 均改用 deferred，检查实际重解析次数。

全量测试最初发现 5 组旧信号预期包含“临时空行的 reparse”。逐个核对后，正向和 Undo/Redo 的预期都只移除了空行信号，所有正式行的刷新起点和次数仍保留。没有通过删除信号断言来掩盖漏解析。

验证结果：

- `TestDocument`：64 passed，0 failed；
- 多行粘贴及相关回归：5 passed，0 failed；
- Release 全量 CTest：6/6 passed，136.03 秒；
- Debug 全量 CTest：6/6 passed，300.85 秒。Debug 的 `test-cppparser` 在系统负载下运行 243.44 秒，但正常完成，没有跳过。

## 3. Document profile

使用既有 `TestDocumentProfile`、`miniaudio.h`、Qt 5.15.18 静态 Release 和 GCC 8.1.0，每场景 3 次采样取中位数。基线为 `d77823fa`，其 Document 插入实现与父提交相同。

单位为 ms。

| 场景 | 基线 sync | 优化后 sync | 基线重解析行数 | 优化后重解析行数 |
|---|---:|---:|---:|---:|
| Insert 1 行，plain | 0.065 | 0.066 | 2 | 1 |
| Insert 1000 行，plain | 2.429 | 2.096 | 2000 | 1000 |
| Insert 10000 行，plain | 24.221 | 21.476 | 20000 | 10000 |
| Replace 1000 行，plain | 3.003 | 2.635 | 2000 | 1000 |
| Replace 10000 行，plain | 25.419 | 23.641 | 20000 | 10000 |
| Insert 1000 行，fold | 2.576 | 2.277 | 2000 | 1000 |
| Insert 10000 行，fold | 25.669 | 22.317 | 20000 | 10000 |
| Replace 1000 行，fold | 3.096 | 2.753 | 2000 | 1000 |
| Replace 10000 行，fold | 26.506 | 25.967 | 20001 | 10000 |

正式行解析次数减半，Document sync 也下降，但不是严格的 50%，因为剩余工作还包括正式内容解析、行对象和 change 维护。fold 场景的尾部状态传播保持正确。

## 4. View profile

View 基线使用 `c375eae6` 前一轮批量布局 A/B 数据，包含相同的 pending 和多行布局实现。除 Replace 1000 行外均为 3 次有效采样中位数；Replace 1000 行因系统停顿只保留 1 次通过全部断言的有效样本，未把超时日志纳入结果。

| 场景 | 基线 sync | 优化后 sync | 基线 total | 优化后 total |
|---|---:|---:|---:|---:|
| Insert 1 行，plain | 1.583 | 2.349 | 2.875 | 3.107 |
| Insert 1000 行，plain | 4.616 | 4.592 | 50.487 | 50.045 |
| Insert 10000 行，plain | 31.494 | 26.819 | 424.978 | 420.370 |
| Replace 1000 行，plain | 7.574 | 5.288 | 50.121 | 50.410 |
| Replace 10000 行，plain | 32.555 | 28.535 | 404.555 | 432.186 |

大批量场景的 sync 进一步下降，单行插入仍有运行波动，不宣称收益。View total 由后台完整布局主导，本项不保证 total 同比下降。Replace 1000 行只有单样本，其数值仅作辅助证据，核心结论以 Document 解析计数和多样本 Document profile 为准。

## 5. 审核结论

实现达到“临时空行不解析、正式内容只解析一次”的目标，同时保持自动缩进、逐行 state、change、fold-dirty 和 Undo/Redo 语义。普通独立插入接口没有被削弱，deferred 的调用契约已写入现状设计文档。结果可合并回 `document-edit-profile`。
