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
2. **内容编辑**：锚点按 marker 语义平移（O(1) 行号算术，与 caret 的 `shiftPosByChange` 同约定：Insert(line,count) 且 line ≤ mTopLine → mTopLine += count；锚点落在删除带内 → 钳到 change.line、段号归 0；SetLine 不动行号）；树同步 `insertLines`/`eraseLines`/`setSegCount`；最后 `mTopPos` 重算。**行为变化（相对 master）：视口顶部内容稳定，master 保持 row 序号。**
3. **折叠/展开**：树 `setHidden` 区间更新；锚点行被折进隐藏区时经 `nextVisibleLine`/`prevVisibleLine` 重新吸附（吸附到 fold 起始行）；`mTopPos` 重算。
4. **映射重建**（wrap 切换 / 字体变化 / resize）：保持锚点，段号钳进新映射有效范围（查布局缓存 O(1)），`mTopPos` 重算。原 `adjustTopPosToAnchor` 的"未被钳时恢复原值、被钳才回退 rowToLineSegment"双路径整体删除。
5. **后台批次到达**：树点更新各精化行；`mWrapAllComputed` 语义不变（dirty 集合判空）；`mTopPos` 重算（锚点不动，滚动条值微调）；更新滚动条上限。

### 滚动条规则

- 像素制（现状）：value = mTopPos，max = maxScrollHeight() = `visibleSegCount()*mTextHeight`，singleStep = mTextHeight，pageStep = mLinesInWindow*mTextHeight。
- **未就绪期间 max 不得钳住新值**：设值若大于当前 max，先调大 max 再设值（max 的估计值是下界：未精化行按 1 段计）。水平滚动条同理。此规则源自批 1 发现的"旧 max 钳回"bug。
- 布局未就绪期间用户的滚动操作优先；**后台分段计算就绪后，不主动恢复旧滚动位置**。本方案下这条自动成立：就绪只改变计数精化，锚点不动。

### 绘制 / 光标 / 鼠标

- 绘制从锚点逐段窗口内走步 O(W)（批 1 的 `advanceLineSegment`/`retreatLineSegment` helper 保留此用途；隐藏行跳过改用树的可见性查询/跳跃）。`y = row*mTextHeight - (mTopPos % mTextHeight)`。gutter 行号/折叠按钮只画首段，折叠竖线逐段画。
- `moveCaretVert`/PageUp/Down：窗口内段走步推进（现状 helper 够用，caret 移动距离小）。
- `ensureCaretVisible`：caret 的窗口 row = 锚点与 caret 间的可见段差（近距离走步 O(|距离|)；远距离经树两次前缀查询 O(log N)）。
- 鼠标命中：`y → 窗口 row → (line, seg, x)`：窗口内走步或 `lineSegAtVisibleRow(anchorRow + windowRow)` O(log N)。
- `DisplayCoord{x, row}`：`x` 为段内像素 x；`row` 为窗口 row（0 = 锚点）。所有鼠标相关操作逐一人工审查（含 RedPandaIDE 侧），返回/接受 DisplayCoord 的方法单元测试全覆盖（按下定位、拖拽选区、双击选词、三击选行、悬停、拖放编辑、列模式 Alt+拖拽等）。

### 后台布局计算

- 估计→精化原则不变：未精化行按 1 段计入树，`mWrapAllComputed` 就绪后全部精确。
- **空文档不进线程**：`scheduleWrapLayoutJob` 在文档为空时直接同步返回（`mWrapAllComputed` 立即就绪，测试无需等事件循环）。（已实现）
- **schedule 与 topPos 更新无顺序约束**（2026-11 分析定稿）：job 的快照输入只有 generation/contentVersion/font/glyphCalculator/wrapWidth/wrap 标志，不读视口位置；topPos/锚点更新走 `wrapLayout` 主线程同步兜底，与 worker 只共享 `mWrapLayouts`（视图是唯一写者，worker 批结果经 generation/contentVersion/dirty-seq 校验后合并）。Roy 曾担心"先设 topPos 会被旧滚动条 max 钳回"——已由 `setTopPos` 的"max 不钳新值"规则覆盖（`value > maximum` 时先抬 max 再 setValue），与 schedule 顺序无关。现有两种顺序（`setOptions` 先 schedule 后 `adjustTopPosToAnchor`；`recalcCharExtent` 经 `onSizeOrFontChanged`→`adjustTopPosToAnchor`）均正确。
- 树随编辑同步更新（`onDocumentContentChanged` 里与现有 fold 平移同处），worker 批次只做 `setSegCount` 点更新，不做结构变更。
- **worker 早停（已实现）**：`Document` 持有原子内容版本号 `mContentVersion`（批次内首次 `recordChange` 时自增；`mUpdateCount` 同为原子，供 worker 线程安全读 `inUpdate()`）；worker 构造（=schedule）时快照 `startVersion`，逐行循环的既有中止条件（`viewDead`、generation 失配）之外增加 `mDocument->inUpdate() || mDocument->contentVersion() != startVersion`——文档一旦开始更新，worker 最多再算一行即退出，**不投递任何布局结果**。**重调度链（实现时补全）**：中止的 worker 改为投递一个空布局的 `aborted` 批次（纯控制信号，不含数据，不可能过期）；`onWrapLayoutBatch` 收到后重新 `scheduleWrapLayoutJob()`——否则编辑后无人重启 sweep，后台计算永久停摆（此前 finished 批次的重调度是唯一链）。互斥锁时序保证无竞争：aborted 批次在共享锁内投递、`jobsInFlight--` 之后才由 GUI 处理，schedule 的去重检查看到的必是已递减的值。稳态打字（无 job 在飞）行为不变：不触发后台 sweep，编辑行走 lazy 计算。正确性本就有 seq/dirty 校验兜底（过期批次被 `onWrapLayoutBatch` 丢弃），早停纯粹省 CPU 并消除 mid-update 计算窗口。配套的纵深防御已实现：`getSyntaxState` 对未解析行返回空 state、`CppSyntaxer::setState` 对空 state 回退 `resetState()`。

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

1. **批 1（已完成，部分将被替代）**：段内换算 `charToSegmentCoord`/`segmentCoordToChar`、窗口内走步 `advanceLineSegment`/`retreatLineSegment`/`advanceAnchorRow`、`anchorRowForLineSegment`——**保留**（窗口内用途）。`syncAnchorToTopPos` 增量走步与两个定锚例外、`adjustTopPosToAnchor` 双路径——**将在批 C 删除/重写**。`setTopPos`/`setLeftPos` "max 不钳新值"、空文档直接就绪——保留。
2. **批 A：RowIndex 段数前缀树**。implicit treap：双和增广 + 非重叠懒标记 hide/unhide + 点更新 + 插删 + 前缀查询 + kth 可见段下降 + next/prevVisibleLine 跳跃。随机对拍测试（vs QVector 暴力参考）。
3. **批 B：映射函数树化**。`displayLineCount`/`lineSegmentToRow`/`rowToLineSegment`/`lineVisibleForWrap` 统一走树（无 wrap 模式 = segs 恒 1 的平凡情形）；折叠 toggle/rescan 对接 `syncFoldVisibility()`（整体重建式：根级 unhide + 有效区间 setHidden）；删除 `foldLineToRow`/`foldRowToLine` 线性扫描。编辑时树随 `onDocumentContentChanged` 同步（Insert/Delete 结构插删、Move 搬行、SetLine 强制同步重布局后点更新）。
4. **批 C：锚点懒推导改造**。`setTopPos`/滚动条入口走树查询；编辑/折叠/批次事件统一"锚点平移或不动 + mTopPos 树重算"；锚点删除带钳制、折叠吸附；`adjustTopPosToAnchor` 简化（保持锚点 + 段号钳 + 重算）；删除批 1 的走步同步与估计机制。
5. **批 D：绘制/caret/翻页/鼠标/invalidate**。窗口相对化 + 树查询；DisplayCoord 审计与测试全覆盖（含 IDE 侧）。
6. **批 E：收尾**。IDE 侧确认（滚动保存/恢复沿用现有像素 API）；demo 目检（滚动平滑、翻页、折叠、大文件无卡顿）；全量验证。

## 已知问题与决策记录

- **RowIndex 懒标记的陈旧性约定**：`lines`/`totalSegs` 在所有节点上始终精确；`hiddenSegs`/叶标志在带 tag 节点的**子树内部**可能陈旧。含 tag 的节点其子树一定均匀（tag⇒uniform；split/merge/setSegCount/setHidden 在下钻前一律 push），因此 next/prevVisible 可在均匀子树整跳；前缀查询与 row 下降必须**继承祖先 tag 下传**（tag1 ⇒ 隐藏和=段数和，tag2 ⇒ 隐藏贡献为 0），且**外层（祖先）tag 优先**——能在已有 tag 之下再产生的 tag 必是先打的，后打的操作下钻时已 push。
- **wrap 切换后滚动条上限窗口期**：由"max 不钳新值"规则解决（设值不再被钳丢）；另有语义约定：**后台分段计算就绪时，不主动恢复旧滚动位置**（窗口期内用户自己的操作优先，就绪后也不把位置拉回旧值）。
- **就绪后 mTopPos 精化（Roy 新想法，2026-11 记录在案，待实现/讨论）**：后台布局计算完成（`mWrapAllComputed` 转真）时，用当前锚点 `(mTopLine, mTopSegment)` 经树查询精确行号 + `mTopPos % mTextHeight` 的余数重算 `mTopPos`，然后更新滚动条（消除窗口期内 max 与 value 的累计误差）。视图不需要重绘——锚点内容未变，只有像素偏移的量化误差被修正；实现选项：paintEvent 检测"仅滚动条数值精化"跳过重绘，或直接重贴之前的 cached image；若两者都太复杂，重绘一遍也可接受（非高频事件）。待批 C 时一并定夺。
- **RowIndex 内存**：~48B/行/视图（treap 节点）。多视图各自建树（wrap 宽度/字体/折叠都是 per-view）。大文件若成问题，打包 B-tree 节点作为后续优化。
- **测试环境注意**：wrap 相关测试开 wrap 后要 `QTRY_VERIFY(mWrapAllComputed)` 等后台布局算完，否则滚动条上限停在"每行 1 段"的下界估计。
- 下划线不作断词点（`_` 是单词成分）已有测试约定；测试用等宽字体保证字形宽度确定。

## 验证

- test-qsynedit 全量零失败（含 RowIndex 对拍测试）。
- IDE 编译通过、IDE 测试通过。
- demo 目检：滚动平滑、翻页、折叠、大文件滚动无卡顿。
