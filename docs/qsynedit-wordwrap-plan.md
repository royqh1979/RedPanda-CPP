# QSynEdit 自动换行：锚点增量模型（设计计划）

> 状态：计划稿（2026-04 讨论定稿），实现前回退到 `fea3d970` 干净基线。

## 背景

自动换行已合并进 master（段布局/映射/绘制/滚动/caret/鼠标/后台计算/测试/菜单）。上一版"全局 row 前缀和/全量映射"在大文件下绘制有 O(n²) 性能问题；尝试改成"窗口 row（锚点相对）"引发大量坐标语义回归，已回退。本计划是性能优化的最终方案。

## 模型

### 滚动状态（三个都存，始终一致）

- `mTopPos`（像素）：垂直滚动偏移，**两种模式都是像素平滑滚动**。
- `mTopLine` / `mTopSegment`：锚点（视口顶部的 (行, 段)）。
- `mTopRow` 不单独存，由 `mTopPos / mTextHeight` 得出（锚点的全局 display row）。

`setTopPos(value)` 写 `mTopPos` 后立即由 `topRow` 推出 `mTopLine`/`mTopSegment`，**三者始终一致**。

### 坐标语义

- `rowToLine`/`lineToRow`：保持**全局 display row** 语义（不变）。
- `DisplayCoord{x, row}`：`x` 为段内像素 x；`row` 为**相对锚点的窗口 row**（0 = 锚点）。
- `displayCoordToPixels`：`y = row * mTextHeight - (mTopPos - mTopRow*mTextHeight)`。

### 全局 row ↔ (line, seg) 转换只在 `setTopPos` 里出现一次

- 滚动：`mTopPos` 变（像素）→ `topRow = mTopPos/mTextHeight` → 按差值 Δ 推进锚点（增量，无全局映射）。
- 绘制：直接用锚点逐段推进，无全局映射。
- `displayLineCount()`：**仅用于垂直滚动条上限**（估计+后台补全），绘制/光标/翻页不用它（边界靠 advance/retreat 失败探测）。
- 内容变化：锚点行 marker 式平移（同 caret 逻辑），`mTopRow` 按受影响行段数增量调整，`mTopPos = mTopRow*mTextHeight`——都不用全局映射。

### 绘制

paint 开头由 `mTopPos` 算锚点一次，之后 `advanceLineSegment` 逐段推进。Gutter/paintLines 都从锚点逐段推进。折叠竖线逐段画，按钮/行号只首段。

### 滚动（像素平滑 + 增量锚点）

新 `mTopPos` → 新 `topRow` → 与旧 `topRow` 求差 Δ → 由锚点 `advanceAnchorRow(Δ)` 推进到新 (line, seg)。

### 光标/翻页（不走全局 row）

- `moveCaretVert`：光标当前 (line, seg, x) → 按 deltaY 段推进 → 新 (line, seg) → 由 x 算 char。
- PageUp/Down 同理，锚点经 `scrollToRow`（全局 row）或增量推进同步。

### 边界契约（advance/retreat）

- 单步分开：`advanceLineSegment`/`retreatLineSegment` 返回 bool，到达文档首/尾返回 false 不动；输入越界先钳到 [0, count-1]；断言结果行/段合法。
- 多步合一：`advanceAnchorRow(带符号 delta)`，负值走 retreat。
- `scrollToRow(globalRow)`：目标全局 row 钳下界（>=0），按与 `mTopRow` 的差值推进锚点，边界靠 advance/retreat 失败自然停住，不用 displayLineCount 钳制。

### 无 wrap 模式

每行 1 段，同一套模型（segment 恒 0），像素平滑滚动保留。

## 待逐项审计（实现前的必读项）

- **DisplayCoord 使用点逐条检查**（qsynedit.cpp/h、painter.cpp、IDE editor.cpp/mainwindow.cpp/project.cpp）：row 现在语义变化（窗口 row vs 全局 row），每处都要确认用对。
- **lineToRow/rowToLine 使用点逐条检查**：约 44 处。特别注意 `invalidateLine`/`invalidateLines` 用 lineToRow 算屏幕刷新区域——这些**不需要全局转换**（用锚点相对 row 即可），且目前**没考虑 wrap**（行高按单段算，折行后刷新区域会缺）。
- 审计时区分三类：显示位置换算（caret/鼠标/选区）、刷新区域计算（invalidate*）、滚动条范围（displayLineCount）。

## 实现批次

1. qsynedit.h/cpp：锚点成员 + advance/retreat/advanceAnchorRow/scrollToRow/anchorRowForLineSegment/globalRowForLineSegment。
2. 绘制（painter.cpp）：paintLines/paintGutter 从锚点推进；y = row*mTextHeight - (mTopPos - mTopRow*mTextHeight)；折叠竖线逐段、按钮/行号只首段。
3. 光标：bufferToDisplayPos/displayToBufferPos 用窗口 row 语义；moveCaretVert/PageUp/Down 用段推进。
4. 滚动条：像素（value = mTopPos），max = (displayLineCount()-1)*mTextHeight。
5. setTopPos/topPos 保留像素语义给外部（IDE 会话/恢复）。
6. IDE（editor.cpp/mainwindow.cpp/project.cpp）：滚动保存/恢复用现有像素 API，不新增。
7. 测试：回退到 fea3d970 全绿基线 + 锚点/推进/边界的单元测试。

## 验证

- test-qsynedit 全量零失败。
- IDE 编译通过、IDE 测试通过。
- demo 目检：滚动平滑、翻页、折叠、大文件滚动无卡顿。
