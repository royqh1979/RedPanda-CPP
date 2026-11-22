# QSynEdit 范围移动设计

## 1. 目标与范围

把连续 N 行移动从 N 次单行 `moveLine()` 改为一个范围级操作，消除 O(N×移动距离) 的重复重解析，以及 Move change 对 RowIndex 和折叠块的逐项维护。

本项同时保证：

- 块内行顺序和 lineSeq 不变，layout 继续按 seq 复用；
- Document 只产生一个范围 Move change，并只进行一次受影响范围重解析及必要的状态传播；
- RowIndex、折叠块、caret/selection、top anchor 和多视图保持一致；
- Undo/Redo 以同一范围反向恢复；
- 现有单行 `moveLine()` 行为作为 N=1 的兼容特例。

不修改搜索替换、自动缩进算法、布局算法或公共移动选区命令的行为。

## 2. 现状与根因

连续块移动当前由 N 次 `moveLine(from, to)` 组成。每次都会：

- 平移行对象并产生一个只有 from/to 的 Move change；
- 重解析 `[min(from,to), max(from,to)]` 及必要尾部；
- 让 View 更新 RowIndex 和折叠块。

因此 N 行移动 100 行会产生约 N×101 行解析及 N 个 change。现有 Move change 没有范围行数，无法让 View 按块处理。

## 3. 范围 change 表达

`DocumentChange` 增加 `moveLineCount`，默认值为 1。只用于 `DocumentChangeKind::Move`：

- `line`：块起始行 from；
- `count`：移动目标行 to；
- `moveLineCount`：连续行数 N。

其它 change 类型不使用该字段。`recordChange()` 增加一个仅用于 Move 的行数参数，普通调用继续默认 1，避免引入另一套 change 类型或复用字符位置。

`moveLine(from,to,parseToEnd)` 变为 `moveLines(from,to,1,parseToEnd)` 的兼容包装。

## 4. Document 范围移动

新增 `moveLines(from,to,numLines,parseToEnd)`：

1. 要求 `numLines>0`、`from>=0`、`from+numLines<=旧行数`、`0<=to<=旧行数-numLines`；from==to 时无操作；
2. 保存连续行对象，按原顺序移除并在 to 位置一次插回；
3. 保持每个 DocumentLine 和 lineSeq 对象身份，lineSeq 索引总数不变；
4. 对被移动行执行原有 fold-delta 检查；
5. 产生一个 `Move(from,to)` change，附带 `moveLineCount=numLines`；
6. 从 `min(from,to)` 重解析到 `max(from,to)+numLines`，之后仍由现有 state 收敛决定是否需要尾部传播。

移动距离和块长共同决定必要的本地范围，不再乘 N 次。parseToEnd=false 仍只解析该范围，true 继续传播到收敛。

`mReparseToLine` 使用同一映射：移动块内位置映射到 to+偏移，前向中间行减 N，后向中间行加 N；N=1 与旧规则完全一致。

## 5. 统一行号映射

Document、View、折叠和选区共用以下语义：

- 被移动块 `[from,from+N)`：`line -> to + (line-from)`；
- 前向 from<to：原 `[from+N,to]` 行减 N；
- 后向 to<from：原 `[to,from)` 行加 N；
- 其它行不变。

to 是移除块之前的插入目标行，与现有单行接口一致。该映射同时用于 `shiftPosByChange()`、`shiftAnchorByChange()` 和折叠块端点。

## 6. View 维护

### RowIndex 与 layout

一次 `eraseLines(from,N)` 和 `insertLines(to,N)`。随后按缓存恢复移动块的 segment count；因为 lineSeq/layout 不变，缓存命中时不重新计算布局。

missing 集合不需要因 Move 改变。后台 batch 仍按 seq 和记录行号验证结果。

### 折叠块

用同一映射更新每个块的 fromLine/toLine。只有端点位于被移动块内的 collapsed 块按原单行规则展开；端点平移后倒置的块移除。父子关系和 collapsed 匹配由现有 partial/full rescan 规则继续处理。

`expandCollapsedFoldsForChange()` 的受影响结束行扩展为 `max(from,to)+N-1`，覆盖后向移动的整个块。

### caret、selection、anchor 和信号

按统一映射更新 caret、selection 和 anchor，再执行现有字符位置钳制。单行 `lineMoved(from,to)` 信号保持；范围移动增加 `linesMoved(from,to,count)`，避免调用方把 N 误认为 1。

## 7. Undo/Redo

新增 `ChangeReason::MoveLines`，避免改变现有 MoveLine 的含义。记录时：

- changeStartPos/changeEndPos 保存原 from/to；
- `changeText()` 保存被移动的文本行，既保留内容，也能得到范围行数。

Undo 从 to 反向移动到 from，Redo 从 from 移动到 to，行数取 `changeText().count()`。现有单行 MoveLine 处理不变，范围操作不与普通输入合并。

## 8. 测试保护

### Document

- 前向、后向和跨整个文档范围移动，检查最终文本、块内顺序和 lineSeq；
- Move change 只有一个且 moveLineCount 正确；
- 重解析范围为 `min` 到 `max+N`，必要尾部传播不缩短；
- N=1 的文本、change 和 reparse 与旧实现一致；
- EOF、from==to、fold-dirty 和 freeze line 正确。

### View

- RowIndex 与所有移动行 layout/segment count 一致；
- caret、selection、anchor 和被移动块端点按统一映射更新；
- 折叠块范围与 full rescan 一致，collapsed 状态按原规则保留或展开；
- 多视图共享 Document 时各自索引和折叠状态正确；
- Undo/Redo 反向恢复相同范围、文本、caret 和索引；
- 现有单行移动、Move Up/Down 测试不回归。

### Profile

现有 move 矩阵改为一次 `moveLines()`，覆盖 1/10/100/1000/10000 行、plain/fold、前中后位置，比较重解析行数、change 数、Document/View sync 和 total。

## 9. 方案边界与 KISS 复审

不采用“N 个单行操作但延迟最后一次解析”，因为那仍保留 N 个 change 和逐项 View 维护，无法解决主要复杂度。也不复用 startCh/endCh 存行数，避免破坏字符坐标语义。

新增一个默认 1 的范围字段和一个范围 Undo reason，是覆盖 Document、View 和 Undo 的最小完整方案。若只在 Document 优化而不更新上述消费点，应视为实现不完整，不能提交。

## 10. 验证

先运行 Document、移动/折叠/多视图专项测试，再依次执行 Release、Debug 全量 CTest。以 `2a140d3b` 为父提交进行直接 A/B profile，重点确认 1000/10000 行移动由约 101000/1010000 行解析降为约 1010/10010 行（未计入必要的额外收敛行），并核对最终所有状态。
