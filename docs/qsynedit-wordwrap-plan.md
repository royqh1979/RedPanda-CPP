# QSynEdit 自动换行：锚点增量模型（设计计划）

> 状态：实现中（分支 `wordwrap-anchor-model`）。本文档随代码同步更新。

## 背景

自动换行已合并进 master（段布局/映射/绘制/滚动/caret/鼠标/后台计算/测试/菜单）。上一版"全局 row 前缀和/全量映射"在大文件下绘制有 O(n²) 性能问题；尝试改成"窗口 row（锚点相对）"引发大量坐标语义回归，已回退。本计划是性能优化的最终方案。

实现过程中 `test_wrap_toggle_preserves_anchor` 暴露了"滚动条 max 旧值钳回"问题（wrap 切换后 max 停在非 wrap 时的 84，`setTopPos(112)` 被钳回并经 `onVScrolled` 把锚点拉错），由此确立了"max 不钳新值"规则（见下）。

## 模型

### 核心原则：锚点是 top 状态的权威表示

- `mTopLine`/`mTopSegment`（视口顶部的行、段）是**权威量**；`mTopPos`（像素）是派生量。像素值只有两部分有实际意义：`mTopPos % mTextHeight`（行内亚像素偏移，平滑滚动用）和 delta row（增量走步步数）。
- `mTopRow` 不单独存，由 `mTopPos / mTextHeight` 派生。
- 内部主路径用 `setTopLineSeg(line, seg[, subPixelOffset])`；`setTopPos` 退化为外部接口（IDE 会话恢复等）和滚动条入口。
- 两种模式都保留像素平滑滚动；亚像素偏移只来自平滑滚动，其它情况 `mTopPos` 总是 `mTextHeight` 的整数倍。

### 不变式与同步时机

不变式：`globalRow(anchor) == mTopPos / mTextHeight` 始终成立，但靠**事件驱动增量维护**，不做使用时校验（懒同步不可行：由锚点反推 mTopPos 需全局映射 O(n)，用到才算会卡）。

四类同步事件：

1. **滚动**（`setTopPos`/`onVScrolled`）：增量走步（见下）。
2. **内容编辑**：锚点按 marker 语义平移（同 caret 逻辑），`mTopPos` 按受影响段数增量调整。
3. **折叠/展开**：锚点行被折进去时重新吸附到最近可见行；`mTopPos` 增量调整。
4. **映射重建**（wrap 切换 / 字体变化 / resize）：`adjustTopPosToAnchor`——保持锚点（段号钳进新映射有效范围），由 `lineSegmentToRow(anchor)` 反推 `mTopPos`，行对齐丢弃亚像素偏移。

"重算滚动条上限"是消费方，不承担同步职责。

### setTopPos 规则

- **永远只做增量相对计算**：value 先钳到 `[0, maxScrollHeight()]`（下界估计，安全：≤ 真实 max），再按 delta row 从锚点走步。
- 走不满 delta = 锚点已停在文档首/尾边界段 = 被钳后的正确答案。**无全局换算回退**——滚动路径一处全局换算都不剩。
- 仅两个例外允许直接定锚：
  - `value == 0` → anchor = (0, 0)；
  - `mWrapAllComputed` 且 `value == maxScrollHeight()`（短路原则：先判廉价标志，避免未就绪时白算 max）→ anchor = 文档最后一个可见段。
- 例外判定用现算的 `maxScrollHeight()`，**不读 `scrollbar->maximum()`**（未就绪时它可能被撑大或滞后，不可靠）。

### 滚动条规则

- 已是像素制（现状）：value = mTopPos，max = maxScrollHeight()，singleStep = mTextHeight，pageStep = mLinesInWindow*mTextHeight。
- **未就绪期间 max 不得钳住新值**：`!mWrapAllComputed` 时，设值若大于当前 max，先调大 max 再设值；就绪后 max 固定为精确值。水平滚动条同理（最长行宽度也是估计值）。
- 布局未就绪期间用户的滚动操作优先；**后台分段计算就绪后，不主动恢复旧滚动位置**（即 max 固定为精确值时，也不把用户当前位置拉回切换前的旧位置）。

### 绘制

- 从锚点逐段推进；`y = row*mTextHeight - (mTopPos % mTextHeight)`。
- gutter 行号/折叠按钮只画首段，折叠竖线逐段画。

### 光标 / 翻页 / ensureCaretVisible（不走全局 row）

- `moveCaretVert`：光标当前 (line, seg, x) → 按 deltaY 段推进 → 新 (line, seg) → 由 x 算 char。
- PageUp/Down 同理（段推进）。
- `ensureCaretVisible` 用锚点相对**窗口 row**：
  - `r = anchorRowForLineSegment(caretLine, caretSeg)`（O(|距离|)，caret 一般在视口附近）；
  - `r < 0` → 锚点上移 |r| 段，`mTopPos -= |r|*mTextHeight`；
  - `r >= mLinesInWindow` → 锚点下移 `r - mLinesInWindow + 1` 段，`mTopPos` 同步增量；
  - 否则不动；`forceToMiddle` 变体：锚点走到 caret 上方 `(mLinesInWindow-1)/2` 段处；
  - 亚像素偏移原样保留；滚动条位置就是增量维护的 mTopPos 本身，无需额外设置。

### 鼠标 / DisplayCoord

- `DisplayCoord{x, row}`：`x` 为段内像素 x；`row` 为相对锚点的**窗口 row**（0 = 锚点）。
- DisplayCoord 的主要用途是鼠标定位（caret 也走它：`displayXY()`/`bufferToDisplayPos` 用于绘制光标和 ensureCaretVisible）。
- 所有鼠标相关操作逐一人工审查，**包括 RedPandaIDE 侧调用**；返回值或参数为 DisplayCoord 的方法单元测试必须全覆盖，使用它的代码段尽量覆盖（按下定位、拖拽选区、双击选词、三击选行、悬停、拖放编辑、列模式 Alt+拖拽等）。

### 后台布局计算

- **空文档不进线程**：`scheduleWrapLayoutJob` 在文档为空时直接同步计算返回（`mWrapAllComputed` 立即就绪，测试无需等事件循环）。
- **顺序约束：先 schedule，再更新 topPos**（映射重建类操作里）。*待 Roy 澄清动机：目前理解是 job 快照 generation/内容版本在先、主线程再强制同步布局，靠 seq/version 校验天然安全（现有顺序已满足）；若动机是 worker 按视口位置排优先级，则"先 schedule"会让 job 拿到旧视口位置。*

### 边界契约（advance/retreat）

- 单步分开：`advanceLineSegment`/`retreatLineSegment` 返回 bool，到达文档首/尾返回 false 不动；输入越界先钳到 [0, count-1]；隐藏（折叠）行跳过。
- 多步合一：`advanceAnchorRow(带符号 delta)`，负值走 retreat，返回实际移动的行数。
- `scrollToRow(globalRow)` = `setTopPos(row*mTextHeight)`，行对齐。
- 走步会强制沿途行同步布局，永远面对真实段数。

### 无 wrap 模式

每行 1 段，同一套模型（segment 恒 0），像素平滑滚动保留。

### 映射重建事件的细节决策（Roy 定）

- **保持视口顶部内容稳定**，不保持像素/row 序号（保持序号会让视口内容跳变）。
- `setTopPos` 内部的增量走步在跨映射时无意义（delta 混淆新旧映射）：未被钳时直接恢复锚点原值；被钳到滚动范围边界时才回退全局 `rowToLineSegment`。
- `recalcCharExtent` 里"强制写回越界 topPos"的旧补丁已删除——越界交给 `setTopPos` 正常钳制（字体缩小后旧位置可能超出新范围）。
- **水平方向保持现状**（`currentLeftCol = mLeftPos/mCharWidth` 按平均字宽保持列序号）：startCh（内容）保持方案在"参照行很短"时会弹回 0，失败模式更糟；水平滚动是视口全局像素量，startCh 只是逐行概念，没有良定义的全局语义。wrap 模式 `mLeftPos` 恒 0。
- **代价**：wrap 模式下 `lineSegmentToRow` 强制同步布局锚点之前的所有行（O(锚点行数)）。切 wrap/改字体是罕见操作，可接受；resize 频率高，需单独评估（见批 3）。

## 全局映射使用白名单

`rowToLine`/`lineToRow`/`rowToLineSegment`/`lineSegmentToRow` 尽量不用；保留下来的每一处逐个人工过，登记在此：

- `adjustTopPosToAnchor`（映射重建，合法）。
- `displayLineCount`/`maxScrollHeight`（滚动条范围，估计+后台补全）。
- 远距离跳转（Go to Line 等，走步 O(|距离|) 的替代——待评估，可能保留全量换算）。
- `invalidateLine`/`invalidateLines` 目前用 lineToRow 算刷新区域，应改锚点相对 row 并补折行高度（批 6）。
- 其余 ~44 处使用点在批 5 审计后逐个填入（保留/改写/删除）。

## 实现路径

1. **批 1（已完成）**：辅助函数 + 锚点成员 + 同步机制。
   - 段内换算 `charToSegmentCoord`/`segmentCoordToChar`、走步 `advanceLineSegment`/`retreatLineSegment`/`advanceAnchorRow`、锚点成员、`scrollToRow`、`anchorRowForLineSegment`。
   - `syncAnchorToTopPos`：增量走步 + 两个直接定锚例外（`value==0`；`mWrapAllComputed` 且 `value==maxScrollHeight()`），无全局换算回退。
   - 滚动条 max 不钳新值规则（`setTopPos`/`setLeftPos`：设值超过当前 max 时先调大 max）。
   - `adjustTopPosToAnchor`（wrap 切换 + `recalcCharExtent`）。
   - 空文档时 `scheduleWrapLayoutJob` 同步直接返回（`mWrapAllComputed` 立即就绪）。
   - 测试：test_toppos_anchor_sync / test_scroll_to_row / test_wrap_toggle_preserves_anchor / test_anchor_row_for_line_segment 全绿。
2. **批 2：内容编辑/折叠时的锚点增量维护**。onLinesInserted/Deleted/Changed 的 marker 平移 + mTopPos 像素增量；collapse/uncollapse 时锚点隐藏吸附。配套测试。
3. **批 3：绘制从锚点推进**（painter.cpp paintLines/paintGutter；displayCoordToPixels 新公式）。含 `resizeEvent` 的映射重建处理——调用频率高，O(锚点行数) 的同步布局成本需评估（可能要限制为仅 wrap 模式 + 阈值）。
4. **批 4：光标/翻页/ensureCaretVisible 窗口 row 化**。bufferToDisplayPos/displayToBufferPos、moveCaretVert、PageUp/Down。
5. **批 5：DisplayCoord 全审计 + 鼠标路径测试全覆盖**（qsynedit + IDE editor.cpp/mainwindow.cpp/project.cpp）。
6. **批 6：invalidate*/刷新区域锚点相对化 + 折行高度**。
7. **批 7：收尾**。IDE 侧确认（滚动保存/恢复沿用现有像素 API，无改动）；demo 目检（滚动平滑、翻页、折叠、大文件无卡顿）；全量验证。

## 已知问题与决策记录

- **wrap 切换后滚动条上限窗口期**：由"max 不钳新值"规则解决（设值不再被钳丢）；另有语义约定：**后台分段计算就绪时，不主动恢复旧滚动位置**（窗口期内用户自己的操作优先，就绪后也不把位置拉回旧值）。
- **测试环境注意**：wrap 相关测试开 wrap 后要 `QTRY_VERIFY(mWrapAllComputed)` 等后台布局算完，否则滚动条上限停在"每行 1 段"的下界估计。

## 验证

- test-qsynedit 全量零失败。
- IDE 编译通过、IDE 测试通过。
- demo 目检：滚动平滑、翻页、折叠、大文件滚动无卡顿。
