# QSynEdit 删除后的重解析范围实施结果

## 1. 实施范围

本项依据 [删除后的重解析范围设计](qsynedit-delete-reparse-design.md) 完成，父提交为 `0bc11805`，实施分支为 `document-edit-delete-reparse`。

生产实现仅把 `Document::deleteLines()` 在 `parseToEnd=true` 时的调用从 `reparseLines(index, index + numLines, true)` 改为 `reparseLines(index, index + 1, true)`。

删除点后当前一行仍被强制验证；若注释、字符串、预处理或语言指令状态未收敛，现有 `reparseLines()` 会继续传播到状态稳定或 EOF。`parseToEnd=false`、EOF、`deleteLine()`、后续 `putLine()` 修复、自动缩进、fold-dirty、Undo/Redo 和公共接口均未改变。

## 2. 测试保护

新增 `test_delete_lines_reparse_scope`，覆盖：

- 普通多行删除只强制解析删除点后一行；
- 单行删除行为不变；
- EOF 删除仍发送 `INT_MAX` 结束通知；
- 删除块注释结束符后，后续行传播 comment-not-finished 到 EOF。

扩展 GAS directive 测试，验证多行删除语言模式指令后，后续行恢复正确的语法模式和寄存器前缀状态。

验证结果：

- `TestDocument`：63 passed，0 failed；
- Release 全量 CTest：6/6 passed，127.60 秒；
- Debug 全量 CTest：6/6 passed，102.71 秒。

全量回归包含自动缩进、Undo/Redo、折叠、多视图和编辑批处理测试，未发现状态收敛回归。

## 3. Profile 结果

使用既有 `TestDocumentProfile` 和 `miniaudio.h`，Qt 5.15.18 静态 Release、GCC 8.1.0，每场景 3 次采样并取中位数。基线为 `d77823fa` 的完整 Document 基线；该基线与父提交的 Document 删除实现相同。

单位均为 ms。`重解析行数` 是 `linesReparsed` 的累计工作量。

| 删除场景 | 基线 sync | 优化后 sync | 基线重解析行数 | 优化后重解析行数 |
|---|---:|---:|---:|---:|
| 1 行，plain | 0.462 | 0.471 | 1 | 1 |
| 10 行，plain | 0.520 | 0.444 | 10 | 1 |
| 100 行，plain | 0.588 | 0.555 | 100 | 1 |
| 1000 行，plain | 1.943 | 0.812 | 1000 | 1 |
| 10000 行，plain | 14.753 | 3.735 | 10000 | 1 |
| 1 行，fold | 60.067 | 62.843 | 47922 | 47922 |
| 10 行，fold | 0.507 | 0.525 | 10 | 2 |
| 100 行，fold | 0.666 | 0.456 | 100 | 1 |
| 1000 行，fold | 1.881 | 0.698 | 1000 | 1 |
| 10000 行，fold | 14.800 | 4.004 | 10000 | 1 |

普通删除 1000/10000 行的 sync 分别下降约 58.2% 和 74.7%，重解析行数不再随删除数量线性增长。单行删除基本持平。

fold 单行删除会改变未配对分隔符状态，仍需要约 47922 行传播，耗时没有下降；这证明本项没有截断必要的跨行状态传播。其它 fold 场景在状态快速收敛时也获得与普通删除相近的收益。

## 4. 审核结论

实现符合 KISS 和预定范围，没有引入新状态或修改收敛算法。测试同时验证重解析计数和最终 syntax state，未通过减少必要工作获得性能数字。自动缩进依赖的逐行 state 仍由 Document 同步维护，结果可合并回 `document-edit-profile`。
