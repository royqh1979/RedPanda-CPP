# QSynEdit 全局映射与自动换行：段数前缀树 + 懒推导锚点（设计计划）

> 状态：方向调整稿（2026-11）。分支 `wordwrap-anchor-model`。本文档随代码同步更新。
> 原"锚点增量走步模型"批 1 已完成并测试（见 git 历史）；经效率与维护性比选后改为本方案，批 1 的增量走步机制大部分被树查询替代（保留与删除清单见实现路径）。

## 背景

自动换行已合并进 master（段布局/映射/绘制/滚动/caret/鼠标/后台计算/测试/菜单）。剩余的核心问题是**全局 row↔(line, segment) 映射的成本与维护方式**：

- 现状实现：`foldLineToRow`/`foldRowToLine` 对全部 fold 块线性扫描（O(F)，F=fold 块数），wrap 模式的 `displayLineCount`/`rowToLineSegment`/`lineSegmentToRow` 逐行循环（O(N·F)），且被滚动、绘制、滚动条更新等热路径调用。
- 上一版"全局 row 前缀和/全量映射"在大文件下绘制 O(n²)；尝试改"窗口 row（锚点相对）"引发坐标语义回归，已回退。
- 曾计划"锚点增量走步模型"（批 1 已完成）：靠事件驱动增量维护 `(mTopLine, mTopSegment)` 与 `mTopPos` 的一致，避开全局扫描。经比选（见"方向比选记录"）改为：**用高效数据结构把全局映射本身变廉价，锚点语义保留但改为懒推导，不做增量维护。**

## 模型

### 核心数据结构：段数前缀树（RowIndex，per-view）

每行一个叶子的增广平衡树。叶子存 `segs`（段数，≥1；后台未精化时为 cache-or-1 估计值）和 `hidden` 标志；内部节点存子树双和 `(totalSegs, hiddenSegs)`，可见段数 = 两者之差。

支持的操作（全部 O(log N)）：

- `insertLines(at, k)` / `eraseLines(at, k)` — 编辑插删行；
- `setSegCount(line, n)` — 点更新（后台布局批次精化、SetLine 后强制同步重布局）；
- `setHidden(from, to, on)` — 区间懒标记。调用方保证只传**有效折叠区间**（`collapsed && !parentCollapsed()`），有效区间互不重叠，因此懒标记不会叠加，idempotent；
- `prefixVisibleSegs(line)` — line 之前的可见段数 = S(l) − H(l)（S=所有行段数和，H=隐藏行段数和；两者是同一"每行段数"数组上的前缀和，由同一结构一次回答）；
- `lineSegAtVisibleRow(row)` — row→(line, seg) 树上二分下降；
- `nextVisibleLine(line)` / `prevVisibleLine(line)` — 带折叠区跳跃（跨过折了 5 万行的块只需 O(log N)，不走步）；
- `visibleSegCount()` — 根节点 O(1)，即 `displayLineCount()`。

实现：v1 用 implicit treap（约 200 行，正确性靠**随机操作序列 vs QVector 暴力参考实现的对拍测试**保证）。内存 ~48B/行/视图，记录在案；若大文件内存成为问题，后续可打包成 B-tree 节点（~4-8B/行开销）作为第二级优化。

### 折叠可见性：不落 per-line 存储，由树派生

`hidden` 在 RowIndex 内（区间懒标记），两种模式统一：**无 wrap 模式就是"所有行 segs≡1"的平凡情形，不再使用独立的数据结构**。可见性谓词 = 树点查询；row 映射 = 树前缀和；折叠区跳跃 O(log N)（整跳隐藏子树）。

曾考虑为无 wrap 模式单独维护"有效折叠区间表"（C 级，省内存），最终否决：分开的唯一收益是内存（~48B/行/视图），而代价是两套隐藏前缀语义实现、两条同步路径、两套测试面（易漂移的平行机制），且内存问题本就由第二级优化（B-tree 打包叶子）解决。统一后映射函数不再按模式分叉。

折叠同步点：`rescanCodeBlocks` 末尾 + collapse/uncollapse/collapseAll 切换处，调用 `syncFoldVisibility()`——**整体重建式**：树上一个根级 unhide 全清（O(log N)）+ 对每个有效折叠区间（`collapsed && !parentCollapsed()`，互不重叠）`setHidden(fromLine+1, toLine, true)`（O(C log N)）。幂等、事实驱动，无需增量维护或差异比对。

### 权威量与派生量：锚点语义保留，懒推导替代增量维护

- `(mTopLine, mTopSegment)` + 亚像素偏移（`mTopPos % mTextHeight`）是 top 状态的**权威量**——保留"视口顶部内容稳定"的既定语义。
- `mTopPos` 是**派生缓存**，不变式：`mTopPos = (prefixVisibleSegs(mTopLine) + mTopSegment) * mTextHeight + subPixel`。
- **维护规则：任何改变锚点或映射的事件之后，`mTopPos` 由锚点经树一次 O(log N) 查询重算。** 不做像素增量估计、不做就绪自愈校准——树在任何时刻给出当前计数下的精确答案，计数本身是估计值时结果自然随精化收敛（锚点不动 → 内容稳定，像素值吸收精化，滚动条位置随之微调——这正是既定语义）。
- `mTopRow` 不存，由 `mTopPos / mTextHeight` 派生。
- 两种模式都保留像素平滑滚动；亚像素偏移只来自平滑滚动，其它情况 `mTopPos` 总是 `mTextHeight` 的整数倍。

### 事件处理规则

1. **滚动**（`setTopPos`/滚动条入口）：value 钳到 `[0, maxScrollHeight()]`；`row = value/mTextHeight`、`sub = value%mTextHeight`；`(line, seg) = lineSegAtVisibleRow(min(row, visibleSegCount()-1))`。O(log N) 精确查询，**无走步、无"走不满"边界约定、无直接定锚例外**（row=0 与文档尾由下降/钳制自然得到）。
2. **内容编辑**：锚点按 marker 语义平移（O(1) 行号算术，与 caret 的 `shiftPosByChange` 同约定：Insert(line,count) 且 line ≤ mTopLine → mTopLine += count；锚点落在删除带内 → 钳到 change.line、段号归 0；SetLine 不动行号，但**锚点行被 SetLine 缩短时段号钳进新段数范围**）；树同步 `insertLines`/`eraseLines`/`setSegCount`；最后 `mTopPos` 重算。**行为变化（相对 master）：视口顶部内容稳定，master 保持 row 序号。**
3. **折叠/展开**：树 `setHidden` 区间更新；锚点行被折进隐藏区时经 `nextVisibleLine`/`prevVisibleLine` 重新吸附（吸附到 fold 起始行，**段号保留并钳进该行段数**，不归零）；`mTopPos` 重算。
4. **映射重建**（wrap 切换 / 字体变化 / resize）：保持锚点，段号钳进新映射有效范围（查布局缓存 O(1)），**亚像素余数清零**（行高变化后旧余数单位失效；映射重建本就是视觉断点，吸附到段边界），`mTopPos` 重算。原 `adjustTopPosToAnchor` 的"未被钳时恢复原值、被钳才回退 rowToLineSegment"双路径整体删除。
5. **后台批次到达**：树点更新各精化行；`mWrapAllComputed` 语义不变（dirty 集合判空）；`mTopPos` 重算（锚点不动，滚动条值微调）；更新滚动条上限。

### 滚动条规则

- 像素制（现状）：value = mTopPos，max = maxScrollHeight() = `visibleSegCount()*mTextHeight`，singleStep = mTextHeight，pageStep = mLinesInWindow*mTextHeight。
- **未就绪期间 max 不得钳住新值**：设值若大于当前 max，先调大 max 再设值（max 的估计值是下界：未精化行按 1 段计）。水平滚动条同理。此规则源自批 1 发现的"旧 max 钳回"bug。
- 布局未就绪期间用户的滚动操作优先；**后台分段计算就绪后，不主动恢复旧滚动位置**。本方案下这条自动成立：就绪只改变计数精化，锚点不动。

### 绘制 / 光标 / 鼠标

- **走步优先原则（2026-11 定稿，回到批 1 的原始决定）**：凡逐位置推进的路径（绘制各循环、caret 垂直移动/翻页、PageUp/Down 的 gap 保持、invalidate 系列、isCaretVisible、ensureCaretVisible、折叠标记线）都从锚点 `advance/retreatLineSegment` 走步或 `windowRowOfLineSegment`（带窗口封顶）计算——**走步沿途强制逐行精算，任何后台进度下都精确**；树查询（O(log N)）只留给一次性定位（鼠标命中、锚点↔像素换算、滚动条范围）。此前"树查询便宜故可替代走步"的分析只算了性能账，漏了估计期映射是暂定值这一正确性维度（Roy 纠正）。
- **例外：ensureCaretVisible 改为从 caret 侧 retreat 判可见（2026-11，Roy 定）**：批次内视图的 RowIndex 尚未追上刚插入的行（行数要等 `contentChanged` 才更新），从锚点向 caret 前进（advance）会因 `nextVisibleLine` 不认识新行而失败，把 caret 误判为"窗口下方"而错误滚动（实测：末行回车时 caret 落到新行 → 误判 → retreat 撞文档头 → 视口被错误拉回顶部，反而掩盖了当时的锚点平移 bug）。retreat 从 caret 向锚点走，只经过索引已覆盖的行（`prevVisibleLine` 对越界行自动钳到索引末尾），批次内外都精确。"在窗口上方"用 (line, segment) 与锚点的字典序直接比较（锚点恒为可见位置）。注意：isCaretVisible 目前无调用者（死方法），未改。
- `VisualPos{x, row}`（原 DisplayCoord）：**文档大画布坐标**——`x` 画布绝对像素（wrap 下逐段归零=段内相对，no-wrap 即行绝对），`row` 全局显示行（含窗口外）。所有鼠标相关操作逐一人工审查（含 RedPandaIDE 侧），返回/接受 VisualPos 的方法单元测试全覆盖（按下定位、拖拽选区、双击选词、三击选行、悬停、拖放编辑、列模式 Alt+拖拽等）。
- **鼠标路径测试边界（批 D' 审计结论）**：点击定位/双击选词/Alt 进列模式/gutter 折叠钮已有事件级测试（QTest 合成事件）。**拖拽选区与拖放编辑无法 headless 覆盖**：它们由 20ms 滚动定时器驱动（`computeScroll`→`doMouseScroll`）且读真实光标 `QCursor::pos()`，合成事件无法注入位置——这两条路径由坐标层 roundtrip 测试（pixelsToGlyphPos/displayToBufferPos）+ demo 目检兜底。为此 `computeCaret` 已改为吃事件坐标（`computeCaret(const QPoint&)`）而非重读真实光标，点击类路径全部可测。
- painter 的选区比较在 (line, segment) 空间进行（`SelectionEndpoint`），不做全局 row 换算。

### 后台布局计算

- 估计→精化原则不变：未精化行按 1 段计入树，`mWrapAllComputed` 就绪后全部精确。
- **空文档不进线程**：`scheduleWrapLayoutJob` 在文档为空时直接同步返回（`mWrapAllComputed` 立即就绪，测试无需等事件循环）。（已实现）
- **schedule 与 topPos 更新无顺序约束**（2026-11 分析定稿）：job 的快照输入只有 generation/contentVersion/font/glyphCalculator/wrapWidth/wrap 标志，不读视口位置；topPos/锚点更新走 `wrapLayout` 主线程同步兜底，与 worker 只共享 `mWrapLayouts`（视图是唯一写者，worker 批结果经 generation/contentVersion/dirty-seq 校验后合并）。Roy 曾担心"先设 topPos 会被旧滚动条 max 钳回"——已由 `setTopPos` 的"max 不钳新值"规则覆盖（`value > maximum` 时先抬 max 再 setValue），与 schedule 顺序无关。现有两种顺序（`setOptions` 先 schedule 后 `adjustTopPosToAnchor`；`recalcCharExtent` 经 `onSizeOrFontChanged`→`adjustTopPosToAnchor`）均正确。
- 树随编辑同步更新（`onDocumentContentChanged` 里与现有 fold 平移同处），worker 批次只做 `setSegCount` 点更新，不做结构变更。
- **worker 早停（已实现）**：`Document` 持有原子内容版本号 `mContentVersion`（批次内首次 `recordChange` 时自增；`mUpdateCount` 同为原子，供 worker 线程安全读 `inUpdate()`）；worker 构造（=schedule）时快照 `startVersion`，逐行循环的既有中止条件（`viewDead`、generation 失配）之外增加 `mDocument->inUpdate() || mDocument->contentVersion() != startVersion`——文档一旦开始更新，worker 最多再算一行即退出，**不投递任何布局结果**。**重调度链（实现时补全）**：中止的 worker 改为投递一个空布局的 `aborted` 批次（纯控制信号，不含数据，不可能过期）；`onWrapLayoutBatch` 收到后重新 `scheduleWrapLayoutJob()`——否则编辑后无人重启 sweep，后台计算永久停摆（此前 finished 批次的重调度是唯一链）。互斥锁时序保证无竞争：aborted 批次在共享锁内投递、`jobsInFlight--` 之后才由 GUI 处理，schedule 的去重检查看到的必是已递减的值。稳态打字（无 job 在飞）行为不变：不触发后台 sweep，编辑行走 lazy 计算。正确性本就有 seq/dirty 校验兜底（过期批次被 `onWrapLayoutBatch` 丢弃），早停纯粹省 CPU 并消除 mid-update 计算窗口。配套的纵深防御已实现：`getSyntaxState` 对未解析行返回空 state、`CppSyntaxer::setState` 对空 state 回退 `resetState()`。
- **估计期内映射重置把锚点打到文档尾（已修，2026-11）**：映射重置（`invalidateAllLineMetrics`：resize/wrap 切换/字体/syntaxer）把树段数回退为 1段/行估计 → `displayLineCount` 掉到目标下界 → `maxScrollHeight` 缩小 → `doUpdateVScrollbar` 的 max 收缩钳制把 mTopPos 钳到下界估计的底部、锚点回推到文档尾（测试现场：ready 时 displayLineCount 1200、row(50,0)=300，重置后 displayLineCount 212、topLine 199、topPos 5572）。修法二合一：①`doUpdateVScrollbar` 在**未就绪时不钳 mTopPos**（`maxValue = max(maxScrollHeight(), mTopPos)`，估计只增不减语义在滚动条侧成立）；②`invalidateAllLineMetrics` 末尾 `recalcTopPosFromAnchor()`——重置后立刻按新映射从锚点重算 mTopPos，像素与树重新一致。回归测试 `test_dnd_scroll_churn`（wrap + 后台 sweep + 滚动 + 重置后绘制完整）。**另：caret 垂直移动改走步**——`moveCaretVert`/翻页在估计期按 1段/行映射会过冲（长行未展开）；改从 caret 的 (line, segment) 直接 advance/retreat，沿途强制精算，永远精确且不依赖全局 row。
- **paintLines 逐段重复解析整行（已完成，2026-11，bcb21543）**：改为跨段续扫——同一行一次 paint 只扫一遍，扫描状态（行/文本/tokenLeft/累积器/glyph 列表）在行循环外延续，换行或 IME preedit 改文本才重扫；跨界 token（如整行注释单 token）靠"累积器画完不消费"两段各画一半。关键实现点：提前跳出改为消费前判断（`tokenLeft >= clipRight` 即停），否则起点在下一段的 token 会丢。大文件滚动条拖拽粒度受范围/高度比限制（新老版本一致），属固有行为，不改。
- **宽度 0 时的布局缓存污染（已修，2026-11）**：文件在 widget 尚无有效宽度时载入（打开即算布局），`computeWrapLayoutForLine` 旧的 `wrapWidth<=0` 早退返回**空 glyphStartXList**——但 glyph 位置与宽度无关、painter 任何模式都需要；带毒缓存导致整行不绘制（.s/.txt 打开大片空白；.cpp 因解析慢、首绘时缓存未中毒而幸免；切语法器触发 `invalidateAllLineMetrics` 清缓存所以"修好"）。修复：仅空行早退；宽度无效时照算 glyph 位置、段结构退化为单段估计（wrap 模式下随后 resize 会全量失效重算，no-wrap 下位置本就与宽度无关）。回归测试 `test_layout_with_zero_wrap_width`。

### 无 wrap 模式

每行 1 段（segment 恒 0），与 wrap 模式共用同一 RowIndex（segs 恒 1）——不建独立结构。像素平滑滚动保留。

### 水平方向

保持现状（`currentLeftCol = mLeftPos/mCharWidth` 按平均字宽保持列序号）：startCh（内容）保持方案在"参照行很短"时会弹回 0，失败模式更糟；水平滚动是视口全局像素量，startCh 只是逐行概念，没有良定义的全局语义。wrap 模式 `mLeftPos` 恒 0。

## 方向比选记录（2026-11，Roy 参与讨论定稿）

**候选路线：**

1. **段数前缀树 + 懒推导锚点（本方案）**：全局映射 O(log N)，锚点语义保留但零增量维护。
2. 第一级数组（连续段数数组 + 运行总和 + 区间表）+ 维持锚点增量走步：白名单罕见路径 O(N) 扫；维护面最大（编辑/折叠/精化/重建四类事件的像素增量估计与自愈）。
3. 第一级数组 + 像素权威 mTopPos-only：最简单，但放弃"视口内容稳定"语义（后台精化时固定像素对应的内容漂移——Roy 之前明确否决过）。

**效率对比**（N=10 万行，W≈50 窗口行；结论：有了前缀和结构，mTopPos-only 效率与锚点同级，效率不构成继续做增量维护的理由）：

- 绘制定顶行：锚点走步 O(W) = 树查询 O(log N)+O(W) ≈ 数组扫 O(N)~0.1ms；
- 滚动：锚点 O(|delta|)（大拖拽最坏 O(N)）vs 树 O(log N)（**消除最坏情况**）；
- displayLineCount：树/运行总和 O(1)；
- 编辑：树 O(K log N) vs 锚点 O(1)+估计自愈机制（复杂度在代码不在运行）。

**关键洞察：树不是替代锚点语义，是干掉锚点的维护成本。** (line,seg) 权威、内容稳定的语义零代价保留（需要像素时树 O(log N) 反查），批 2~4 计划中的像素增量估计、就绪自愈、走步例外路径整体不需要了。

**折叠可见性表示的比选（flags vs 区间表/树）：**

- 讨论中淘汰的论点（避免走回头路）：①"flags 有同步点/漂移风险"——不成立，flags 可随 rescanCodeBlocks 同趟整体重建，同步点与派生查询相同；②"flags 每次编辑 O(N) 重算"——不成立，可增量平移 + rescan 后只补差异。
- 有效结论：row 映射需要隐藏行数前缀和，前缀和要廉价活过编辑（折叠/展开隐藏 k 行的区间后所有后缀 ±k）就必须游程压缩为"每个有效折叠区间一条"——flags 的映射半边必然收敛到区间形态；带双和的段数前缀树单结构同时回答谓词与映射，并支持折叠区 O(log N) 跳跃，flags 跨折叠区只能逐行走步。
- Fenwick 树出局：不支持下标插删（编辑插删行要 O(N) 移位）。
- 若 profile 证明谓词热点，可从树再派生 flags 做局部备忘，不破坏架构。
- 后续决定（2026-11）：两模式统一用树，**否决**为无 wrap 模式单独建区间表（唯一收益是内存，代价是两套平行机制；内存由 B-tree 打包解决）。

## 全局映射 API 的新定位

原"尽量不用 + 白名单登记"原则**随树落地放宽**：映射查询 O(log N) 后可在任意路径使用。但仍优先窗口相对走步（绘制等逐行顺序消费方持游标 O(1) 摊还，比逐行树查询更省）。`invalidateLine`/`invalidateLines` 等仍应改锚点相对窗口 row（避免长距离查询），并补折行高度。

## 实现路径

1. **批 1（已完成，部分已被批 C 替代）**：段内换算 `charToSegmentCoord`/`segmentCoordToChar`、窗口内走步 `advanceLineSegment`/`retreatLineSegment`、`anchorRowForLineSegment`——**保留**（窗口内用途）。`advanceAnchorRow`、`syncAnchorToTopPos` 增量走步与两个定锚例外、`adjustTopPosToAnchor` 双路径——**已在批 C 删除/重写**。`setTopPos`/`setLeftPos` "max 不钳新值"、空文档直接就绪——保留。
2. **批 A：RowIndex 段数前缀树**。implicit treap：双和增广 + 非重叠懒标记 hide/unhide + 点更新 + 插删 + 前缀查询 + kth 可见段下降 + next/prevVisibleLine 跳跃。随机对拍测试（vs QVector 暴力参考）。
3. **批 B：映射函数树化（已完成）**。`displayLineCount`/`lineSegmentToRow`/`rowToLineSegment`/`lineVisibleForWrap`/`lineToRow`/`rowToLine` 统一走树（无 wrap 模式 = segs 恒 1 的平凡情形）；折叠 toggle/rescan/`setUseCodeFolding` 对接 `syncFoldVisibility()`（整体重建式：根级 unhide + 有效区间 setHidden，范围 = `(fromLine, toLine]`）；`foldLineToRow`/`foldRowToLine` 线性扫描已删除；`advance`/`retreatLineSegment` 折叠区跳跃改用树的 `next`/`prevVisibleLine`。编辑时树随 `onDocumentContentChanged` 同步（Insert/Delete 结构插删、Move 搬行后从缓存恢复段数、SetLine 强制同步重布局后点更新）；`wrapLayout` 同步兜底计算与 worker 批次（携带 seq→计算时行号，应用时 `getLineSeq(line)==seq` 校验）做点更新；`invalidateAllLineMetrics` 用 `resetSegCounts()` 整体回估计。实现要点：①批次进行中树未同步（文档已变），查询端一律按**树的** count 钳制（`lineVisibleForWrap`、`wrapLayout` 守卫；前缀/下降查询自带钳制），Insert 同步从缓存恢复批次中提前算出的段数；②`adjustTopPosToAnchor` 的锚点行号必须立即精确（视口内容不能跳），暂用同步走步强制计算锚点之前各行而非树估计——批 C 重写锚点推导时再统一；③SetLine 强制同步重布局改变了过期拒绝的可见语义：被编辑行在编辑时即刻重新入缓存，过期批次表现为"不覆盖新布局"（测试已相应更新）。
4. **批 C：锚点懒推导改造（已完成）**。`setTopPos`/`onVScrolled` 走树查询推锚点（`deriveAnchorFromTopPos`）；编辑/折叠/批次事件统一"锚点平移或不动 + `recalcTopPosFromAnchor` 树重算"；批次到达时树点更新与 mTopPos 重算成对（防漂移论证见"已知问题与决策记录"）；锚点删除带钳制（`shiftAnchorByChange`，镜像 `shiftPosByChange` 约定）、折叠吸附（`syncFoldVisibility` 末尾统一做）；`adjustTopPosToAnchor` 简化为"钳锚点 + 清零余数 + setTopPos"；`syncAnchorToTopPos`/`advanceAnchorRow` 已删除。实现要点：①程序性滚动条同步一律 `QSignalBlocker`，`onVScrolled` 只服务用户滚动；②`doUpdateVScrollbar` 显式处理 max 收缩（block 信号会吞掉 Qt 的自动钳制）；③**`resetEditState`（loadContent/doClearAll/setDocument）置 `mAnchorResetPending`**——整文档替换不是编辑，锚点不得被其变更集 marker 平移（否则载入即滚到底部）；④批次路径放"重算不超 max"的 `Q_ASSERT`（证明见决策记录），编辑路径由重算 helper 统一钳制。新增测试：`test_edit_shifts_anchor_marker_style`（含滚动条用户路径）、`test_setline_clamps_anchor_segment`、`test_collapse_adsorbs_anchor`、`test_wrap_batch_recomputes_toppos`。注意：collapse 会在折叠后 `ensureCaretVisible`——caret 不在折叠区但不可见时视口仍会被拉向 caret，吸附语义只在 caret 可见时成立（测试已按此设计）。⑤**对外滚动定位 API 改为锚点制**：新增 `topLine()`/`topSegment()`/`setTopLineSegment(line, seg)`（越界钳制、折叠行吸附到 fold 起始行）；`setTopPos`/`topPos` 降为**私有**像素级入口（滚动条/平滑滚动/undo LeftTop 用）。动机：像素是派生量，sweep 未就绪时按估计映射反推的 (line,seg) 是错的且会被锚点语义固化——跨会话持久化必须存锚点。IDE 侧布局保存/恢复（mainwindow/project/editor 的 replaceContent/searchinfiledialog 等）已全部改存 line+segment，不做旧配置兼容（缺 key 按 0）。
5. **批 D（已缩水为 D'，2026-11 定）**：窗口相对化整体**取消**——其动机（全局映射太贵）已被批 B 的树消除（O(log N) 查询可在任意路径使用），现状语义自洽、测试全过，切换只有回归风险没有收益。保留项：**VisualPos（原 DisplayCoord，Roy 改名）与鼠标路径的审计 + 测试补齐**。语义定稿：VisualPos 是**文档大画布**坐标——x 为画布绝对像素（wrap 下逐段归零=段内相对，no-wrap 即行绝对），row 为全局显示行（含窗口外）；`types.h` 注释已按此写明。
6. **批 E：收尾**。IDE 侧确认（滚动保存/恢复已改锚点制 `topLine`/`topSegment`/`setTopLineSegment`）；demo 目检（滚动平滑、翻页、折叠、大文件无卡顿）；全量验证。

## 已知问题与决策记录

- **RowIndex 懒标记的陈旧性约定**：`lines`/`totalSegs` 在所有节点上始终精确；`hiddenSegs`/叶标志在带 tag 节点的**子树内部**可能陈旧。含 tag 的节点其子树一定均匀（tag⇒uniform；split/merge/setSegCount/setHidden 在下钻前一律 push），因此 next/prevVisible 可在均匀子树整跳；前缀查询与 row 下降必须**继承祖先 tag 下传**（tag1 ⇒ 隐藏和=段数和，tag2 ⇒ 隐藏贡献为 0），且**外层（祖先）tag 优先**——能在已有 tag 之下再产生的 tag 必是先打的，后打的操作下钻时已 push。
- **wrap 切换后滚动条上限窗口期**：由"max 不钳新值"规则解决（设值不再被钳丢）；另有语义约定：**后台分段计算就绪时，不主动恢复旧滚动位置**（窗口期内用户自己的操作优先，就绪后也不把位置拉回旧值）。
- **后台批次到达时 mTopPos 随树重算（2026-11 已定，方案一）**：Roy 提出的"就绪时按锚点精化 mTopPos"想法经分析落实为更一般的规则——**树的点更新与 mTopPos 重算必须成对发生，否则内容漂移**：批次精化了锚点上方行的段数后，`paintEvent` 的 `firstRow = mTopPos / mTextHeight` 拿旧像素值查已精化的树会落到锚点之前的行，视口内容在扫描期间向上漂移。因此每个批次到达都做：树点更新 → 锚点不动，按 `mTopPos = (prefixVisibleSegs(mTopLine) + mTopSegment) * mTextHeight + (mTopPos % mTextHeight)` 重算 → 更新滚动条。就绪翻转无特殊逻辑（只是最后一个批次）。**不需要重绘**：重算后 `firstRow` 精确落回锚点、亚像素余数保留，前后两帧像素逐点相同，唯一变化是滚动条 value/max。
- **程序性滚动条同步一律用 QSignalBlocker，onVScrolled 无守卫（2026-11 批 C 复查修正）**：最初设想在 `onVScrolled` 加 `mTopPos == value` 相等守卫来免重绘——有洞：`setTopPos` 自己不调 `invalidate()`，依赖 `setValue`→`onVScrolled`→`invalidate()`，守卫会吞掉这次重绘。定稿：`setTopPos`/`doUpdateVScrollbar`/批次路径的程序性 `setValue` 一律 `QSignalBlocker` 阻断，各自显式负责重绘；`onVScrolled` 只服务用户滚动（mTopPos = value → 树查询推锚点 → invalidate）。
- **max 收缩路径的显式钳制（批 C 复查补充）**：block 信号后，Qt"setMaximum < value 时自动钳 value"的机制被吞，`mTopPos` 可能残留越界。`doUpdateVScrollbar` 显式处理：`mTopPos > maxScrollHeight()` → 钳 mTopPos + 树回推锚点 + 重绘（内容下滑填尾，同 master 语义）。**钳制不变式**：批次路径（精化只增段数）可证 `newTopPos ≤ max`（锚点行号增量 ≤ 总量增量），放 `Q_ASSERT` 验证；编辑路径锚点下方行 SetLine 缩短时 max 可缩、锚点行号不变，**可能越界**，故统一由重算 helper 钳制，断言只放批次路径。
- **undo 的 LeftTop 记录像素值**：`addLeftTopToUndo` 存的是 `mTopPos` 像素，估计期保存、精化后恢复（或反之）语义有窗口期漂移——记录在案，不改（低概率、无正确性风险）。
- **RowIndex 内存**：~48B/行/视图（treap 节点）。多视图各自建树（wrap 宽度/字体/折叠都是 per-view）。大文件若成问题，打包 B-tree 节点作为后续优化。
- **测试环境注意**：wrap 相关测试开 wrap 后要 `QTRY_VERIFY(mWrapAllComputed)` 等后台布局算完，否则滚动条上限停在"每行 1 段"的下界估计。
- 下划线不作断词点（`_` 是单词成分）已有测试约定；测试用等宽字体保证字形宽度确定。

## 验证

- test-qsynedit 全量零失败（含 RowIndex 对拍测试）。
- IDE 编译通过、IDE 测试通过。
- demo 目检：滚动平滑、翻页、折叠、大文件滚动无卡顿。
