# QSynEdit 重解析内循环优化实施结果

## 1. 实施范围

本项依据 [重解析内循环优化设计](../../design/qsynedit/qsynedit-reparse-loop-design.md) 完成，父提交为 `feb6632b`，实施分支为 `document-edit-reparse-loop`。

采用内外接口分层：

- `getLine()`、`getLineSeq()`、`getSyntaxState()`、`setSyntaxState()` 保持对外加锁语义；
- `doGetLine()`、`doGetLineSeq()`、`doGetSyntaxState()`、`doSetSyntaxState()` 供 Document 内部已经持锁的路径使用；
- `reparseLines()` 改用内部接口，不再递归加锁。

非法行号和未解析行仍返回与原实现一致的空文本、0 或新的空 `SyntaxState`。没有直接在内循环中散落 `DocumentLine` 访问，也没有改变收敛算法、getState、锁生命周期或自动缩进。

该拆分使锁上下文在接口上清晰可见：`do + 对外接口名` 的对应关系简单，适合限制在 Document 内部已持锁路径使用，不应扩散成公共无锁访问习惯。

## 2. 测试验证

- `TestDocument`：65 passed，0 failed；
- Release 全量 CTest：6/6 passed，114.02 秒；
- Debug 全量 CTest：6/6 passed，141.54 秒。

Debug 初次运行时 `test-qsynedit` 出现一次未复现失败；直接重跑及随后全量 CTest 均通过，没有修改测试或忽略失败。

现有 reparse、GAS cascade、fold-dirty、deferred 插入、范围移动、自动缩进、Undo/Redo 和多视图测试均通过，重解析行数与通知范围未改变。

## 3. 直接 A/B profile

基线为父提交 `feb6632b` 的独立 worktree，与实现后使用相同 Qt 5.15.18 静态 Release、GCC 8.1.0 和 `miniaudio.h`。每场景采样 5 次并取中位数。单位 ms。

| 场景 | 基线 sync | 优化后 sync | 变化 |
|---|---:|---:|---:|
| 修改 10000 行，plain | 20.713 | 17.653 | -14.8% |
| 前部修改 1 行，fold | 122.691 | 109.377 | -10.9% |
| 中部修改 1 行，fold | 64.317 | 58.206 | -9.5% |
| 尾部修改 1 行，fold | 1.347 | 1.173 | -12.9% |

所有目标场景均为正向收益，重解析行数完全一致。收益包括减少递归锁开销，不包含跳过任何必要状态传播。

## 4. 审核结论

内外接口拆分在可读性、职责边界和性能上都成立。对外调用者不需要了解锁上下文，内部已持锁路径则有明确且一一对应的 `do*` 接口，空状态处理集中且不重复。实现范围严格符合设计，结果可合并回 `document-edit-profile`。
