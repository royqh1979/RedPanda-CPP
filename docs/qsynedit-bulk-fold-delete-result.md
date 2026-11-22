# QSynEdit 折叠块批量删除实施结果

## 1. 实施范围

本项依据 [折叠块批量删除设计](qsynedit-bulk-fold-delete-design.md) 完成，父提交为 `cf005070`，实施分支为 `document-edit-bulk-fold-delete`。

`processFoldsOnLinesDeleted()` 已从反向遍历并逐项 `QVector::remove()` 改为正向单遍筛选 `keptBlocks`，遍历结束后一次替换列表。

opener/closure 删除、范围平移、空块判断、collapsed 清除、块对象身份、父子关系、列表顺序和后续 partial scan 语义均保持不变。额外空间仅为 O(B) 的 shared_ptr 指针数组，没有复制 CodeBlock 对象，也没有修改 Document、折叠显示、自动缩进、Undo/Redo 或公共接口。

## 2. 测试验证

新增 `test_fold_bulk_delete_matches_full_rescan`：

- 40 个函数块，多个 collapsed 块；
- 删除前块 closure 和连续多个完整块；
- 验证删除后剩余 collapsed 块的行号与状态；
- 比较 `foldRangesForTest()` 与 `fullRescanCodeBlocksForTest()` 的完整结果。

验证结果：

- 多块删除专项：3 passed，0 failed；
- Release 全量 CTest：6/6 passed，77.77 秒；
- Debug 全量 CTest：6/6 passed，198.59 秒。

既有整块删除、fold shift、共享 View、Undo/Redo、结束行后缀和 caret 测试均通过。

## 3. Profile 方法与结果

直接基线为父提交 `cf005070` 的独立 worktree，与实现后使用相同的 Qt 5.15.18 静态 Release、GCC 8.1.0、`miniaudio.h` 和 offscreen 配置。每个优化后场景采样 3 次并取中位数。

基线 fold-on 因系统停顿只保留 1 次通过全部断言的有效样本；其它基线场景为 3 次中位数。超时日志未纳入结果。

单位为 ms，场景均为删除 10000 行。

| 场景 | 基线 sync | 优化后 sync | 基线 total | 优化后 total |
|---|---:|---:|---:|---:|
| fold 负载，开启折叠 | 397.964 | 37.609 | 398.942 | 38.279 |
| fold 负载，关闭折叠 | 28.552 | 34.363 | 28.672 | 34.478 |
| plain 负载，开启折叠 | 28.465 | 27.300 | 28.986 | 27.832 |

开启折叠时 sync 下降约 90.5%，原先由逐项 `QVector::remove()` 主导的成本基本消除。关闭折叠时该函数提前返回，不经过新列表算法；其数值波动不能解释为算法回退。plain 删除基本持平。

重解析行数在两种实现中均为 1，change entries/changed lines 和 fold rescans 一致。Document 的删除重解析行为未由本项修改。

## 4. 审核结论

实现符合 KISS 和范围约束，没有通过跳过折叠扫描或减少必要状态维护获得收益。partial 与 full rescan 的一致性、各 View 独立折叠状态和 Undo/Redo 均得到验证，结果可合并回 `document-edit-profile`。
