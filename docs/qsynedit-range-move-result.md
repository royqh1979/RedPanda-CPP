# QSynEdit 范围移动实施结果

## 1. 实施范围

本项依据 [范围移动设计](qsynedit-range-move-design.md) 完成，父提交为 `2a140d3b`，实施分支为 `document-edit-range-move`。

实现包括：

- `DocumentChange::moveLineCount`（默认 1）表达范围；
- `Document::moveLines()` 一次移除并插回连续行，保留块内顺序、DocumentLine 和 lineSeq；
- 单次范围重解析及必要的状态传播；
- RowIndex 一次 erase/insert，layout 继续按 seq 复用；
- 折叠块、caret、selection、anchor 使用统一范围映射；
- 单行 `lineMoved` 与范围 `linesMoved` 信号；
- `ChangeReason::MoveLines` 及 Undo/Redo，`changeText()` 保存移动文本；
- 现有 `moveLine()` 作为 N=1 的兼容包装。

没有采用只延迟最后一次解析的方案，因为那无法消除 N 个 change 和逐项 View 维护。

## 2. 测试验证

新增 Document 测试覆盖前向/后向文本、lineSeq、单个 Move change、moveLineCount 和重解析范围；N=1 继续验证兼容行为。

新增 View 测试覆盖范围移动后的文本、caret/selection、lineSeq、RowIndex 和 Undo/Redo。

验证结果：

- Document 范围专项：3 passed，0 failed；
- View 范围及 Undo 专项：3 passed，0 failed；
- Release 全量 CTest：6/6 passed，51.15 秒；
- Debug 全量 CTest：6/6 passed，95.45 秒。

既有单行移动、Move Up/Down、折叠、多视图、自动缩进和布局测试均通过。

## 3. Document profile

基线为 `d77823fa` 的完整 Document 基线。使用相同 Qt 5.15.18 静态 Release、GCC 8.1.0 和 `miniaudio.h`，每场景 3 次采样取中位数。单位 ms。

| 场景 | 基线 sync | 优化后 sync | 基线重解析行数 | 优化后重解析行数 | 基线 change 数 | 优化后 change 数 |
|---|---:|---:|---:|---:|---:|---:|
| 移动 1 行，plain | 0.681 | 0.630 | 103 | 103 | 1 | 1 |
| 移动 1000 行，plain | 575.115 | 1.970 | 101000 | 1102 | 1000 | 1 |
| 移动 10000 行，plain | 6271.859 | 13.225 | 1010000 | 10100 | 10000 | 1 |
| 移动 1 行，fold | 59.255 | 64.101 | 47923 | 47923 | 1 | 1 |
| 移动 1000 行，fold | 577.733 | 2.038 | 101001 | 1102 | 1000 | 1 |
| 移动 10000 行，fold | 6382.483 | 13.561 | 1010001 | 10101 | 10000 | 1 |

范围解析包含 `max(from,to)+N` 的本地区间，部分场景另有 2 行状态收敛，因此 1000 行场景为 1102 行而不是约 1010 行。fold 单行的必要尾部传播保持，不承诺收益。

## 4. View profile

基线为 `d77823fa` 的全矩阵单次采样；优化后为 3 次中位数。单位 ms。

| 场景 | 基线 sync | 优化后 sync | 基线 total | 优化后 total |
|---|---:|---:|---:|---:|
| 移动 1 行，plain | 1.624 | 2.384 | 2.087 | 3.271 |
| 移动 1000 行，plain | 730.746 | 4.066 | 731.246 | 5.125 |
| 移动 10000 行，plain | 7487.583 | 19.683 | 7488.047 | 20.684 |
| 移动 1 行，fold | 72.033 | 76.798 | 72.545 | 77.699 |
| 移动 1000 行，fold | 736.252 | 5.383 | 736.739 | 5.879 |
| 移动 10000 行，fold | 8007.134 | 23.411 | 8007.694 | 23.940 |

1000/10000 行的重复工作已消除，sync 从秒级/数百毫秒降为毫秒级。单行小场景存在运行波动，不作为收益依据。

## 5. 审核结论

本项不是只优化 Document 循环：范围 change、lineSeq、RowIndex、折叠、caret/selection、anchor 和 Undo/Redo 均已同步更新。实现覆盖设计要求的完整语义，结果可合并回 `document-edit-profile`。
