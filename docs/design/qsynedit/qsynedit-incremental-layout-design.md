# QSynEdit 增量布局与 Max Width 更新设计

## 目标与范围

解决大文件（如 `miniaudio.h`）中插入或修改一行时，后台重新计算整个文档布局，以及水平滚动条随临时 max width 多次变化的问题。

本次只修改：

- `WrapLayoutJob` 的行选择；
- QSynEdit 对 dirty/missing 行的调度；
- max width 的缓存读取、失效和滚动条通知时机。

不修改文本布局算法、语法分析、RowIndex、绘制器、Document 数据结构或其它滚动条逻辑。

## 现状与根因

### 普通编辑触发全量布局

`SetLine` 会删除修改行的 layout 缓存并设置 `mWrapAllComputed=false`。`WrapLayoutJob::run()` 随后无条件遍历全部 Document 行。

`mWrapDirtySeqs` 当前只用于拒绝过期结果，没有用于选择计算对象。因此单行修改也承担全文档布局成本。

### max width 与临时结果耦合

最长行由 `mMaxLineWidthSeq` 标识。缓存写入点 `noteLineWidthKnown()` 会采纳临时较长行并请求滚动条更新；后台 sweep 完成后又调用 `recomputeMaxLineWidthAndNotify()`。

`recomputeMaxLineWidthAndNotify()` 通过 `wrapLayout(i)` 读取宽度。该接口在缓存缺失时会同步计算布局，使 max width 查询与布局计算隐式耦合。

### 通知过频

后台按 256 行分批返回。临时 max width 每次变化都可能通知 scroller，造成水平滚动条长度逐步变化。

## 设计原则

- 普通编辑只计算缓存缺失行，已经同步算出的修改行不得重复计算；
- max width 只读取有效 layout 缓存，绝不触发布局计算；
- 非最长行只与当前 max width 比较，不扫描其它行；
- 最长行失效后，允许扫描缓存重新找最大值，但不重算 layout；
- max width 无效时不通知 scroller；得到有效值后最多通知一次；
- 不引入堆、有序宽度索引或新的布局/max-width 类。

## 增量布局调度

### 待计算集合

`mWrapMissingSeqs` 增量记录当前 Document 中缺少有效 layout 的行 seq。全局失效时一次建立全集；Insert 只登记尚无缓存的新行，SetLine 在删除旧缓存时登记，同步或后台写入有效缓存时移除，Delete 清理已不存在的 seq。`scheduleWrapLayoutJob()` 直接复制该集合作为 `pendingSeqs`，不再逐行扫描 Document。

`mWrapDirtySeqs` 不直接进入待计算集合。现有 `SetLine` 已经同步重算修改行并写入有效缓存；dirty 集合继续只负责拒绝更早任务返回的过期结果，不能据此再算一次。`mWrapAllComputed` 由 missing 是否为空决定。

初次加载或全局失效时集合覆盖所有未布局行，普通 SetLine 后若没有其它缺失行则集合为空，不创建新 Job。

### Job 接口

给 `WrapLayoutJob` 增加一个可选的 `QSet<size_t> pendingSeqs` 参数和对应成员。空集合保留原有“计算全部行”的含义，以兼容现有直接构造 Job 的代码和测试；调度端在集合为空时直接完成，不创建空 Job。

`run()` 在计算一行前检查：集合非空且不包含当前 seq 时直接跳过。文档锁、内容版本检查、批大小、abort 和结果发布逻辑保持不变。

### Insert/Delete/SetLine

- Insert：完整 edit 只插入一行时，立即同步计算新行 layout 并写入缓存；多行插入将未缓存的新行交给后台处理。插入并修改同一批发生时，按各自的单行/多行规则处理。任何插入都不因此重算其它有效行；
- SetLine：单行沿用 dirty 标记和同步精确布局，不启动重复的后台计算；多行使用 dirty/missing 标记和 RowIndex 的 1 段估计，由现有查询与后台 Job 精化；
- Delete：不产生布局计算需求，死缓存继续惰性清理，不起 Job；
- Move：seq 不变，不产生布局计算需求。

单行插入或插入并修改后，若没有其它缺失行，不创建 Job。任务 abort 后按最新缓存状态重新生成 `pendingSeqs`，已被接受的有效 layout 不重算。现有按 contentVersion 拒绝过期结果的逻辑不变。

## Max Width 状态与更新

### 状态含义

沿用现有字段，不建立新状态机：

- `mMaxLineWidthSeq`：当前最长行或失效后的候选行；0 表示尚无候选；
- `mMaxLineWidthMaybeStale=true`：当前候选不是可靠最终值，`maxLineWidth()` 返回 -1；
- `mMaxLineWidthMaybeStale=false`：当前 max width 有效；
- 新增一个布尔值 `mMaxLineWidthUpdatePending`：只表示有效后是否需要通知 scroller，不参与宽度计算。

现有 `noteMaxLineWidthDirty()` 改为只设置 `mMaxLineWidthUpdatePending`，不再立即发送滚动条事件。

### 非最长行修改或新行

新 layout 写入缓存时：

- `width <= 当前有效 max width`：不改变 max width；
- `width > 当前有效 max width`：更新 `mMaxLineWidthSeq`，保持其有效状态，并设置 update pending。

不扫描其它行。即使新最长行已经确定，也等本次增量布局完成后统一通知。

### 当前最长行修改或删除

- SetLine 删除最长行缓存前：清除 `mMaxLineWidthSeq`，设置 stale 和 update pending；
- Delete 后：通过 `findLineBySeq(mMaxLineWidthSeq)` 判断最长行是否已不存在；若是，执行同样失效处理；
- 失效期间不刷新 scroller，保留其原 maximum、pageStep 和当前位置；
- 当前同步修改及所有 missing 行布局完成后，从缓存重新找最长行。

Delete 不需要知道被删行的旧序号，只需检查当前记录的 seq 是否仍存在于 Document。

### 全局失效

字体、wrap 宽度等全局失效沿用现有全缓存清除。max width 同时失效，待所有 layout 就绪后重新计算并通知一次。

## 从缓存重找最长行

将 `recomputeMaxLineWidthAndNotify()` 改为只读算法：

1. 遍历 Document 中的行 seq；
2. 通过 `mWrapLayouts.constFind(seq)` 查缓存；
3. 不调用 `wrapLayout()`，缺行时标记尚未完整，不做最终通知；
4. 缓存完整时选出最终最长行，清除 stale；
5. update pending 且 max width 有效时，调用一次 `updateHScrollBarLater()` 并清除 pending。

在增量 batch 完成、以及不发生布局计算的 Delete 后调用同一完成处理。部分 batch 到达时不做最终重找，也不通知 scroller。

## 通知时序

普通 SetLine：

1. 标记 dirty，必要时使 max width 失效；
2. 同步计算修改行一次，更新缓存候选，但不发送临时滚动条事件；
3. 若仍有其它 missing 行，后台只处理这些行；否则不创建 Job；
4. 当前修改完成且缓存有效后，重找最长行（仅在必要时）或接受比较得到的新最长行；
5. 最多发送一次 scroller 更新。

Delete 最长行且没有待计算布局时，可直接在当前缓存中重找并通知一次，不创建空 Job。

## 修改范围

预计只涉及：

- `wraplayoutjob.h/.cpp`：可选 pending 集合及跳过逻辑；
- `qsynedit.h/.cpp`：调度集合、Insert/Delete 的少量失效处理、max width 缓存读取与通知；
- `test_qsynedit_display.*`：增量 Job 和 max width 通知测试；
- 本设计文档及必要的现状说明。

不改 `computeWrapLayoutForLine()`、painter、RowIndex 和公共文本编辑接口。

## 测试方案

### 增量 Job

- 100 行文档只指定两个 seq，Job 只返回这两行的 layout；
- 普通 SetLine 只同步计算修改行一次，不创建重复的后台 Job；
- 单行插入及插入并修改只各同步计算一次，不创建 Job；多行插入只后台处理缺失行；
- abort 后已接受的有效行不重算；
- 初次全量布局、字体/宽度全局失效仍覆盖所有行。

### Max width

- 非最长行变长：只比较当前最大值，完成后一次通知；
- 非最长行变短：max width 和 scroller 不变；
- 最长行变短：失效期间无 scroller 更新，完成后一次通知；
- 删除最长行：无空 Job，完成后一次通知；
- 插入空行：max width 不变，无滚动条变化；
- 缓存不完整时不以临时值刷新 scroller；
- Undo/Redo、Move 和多视图共享 Document 行为不回归。

### 编辑后的索引一致性

每类编辑都同时检查 layout 缓存、RowIndex 和 lineSeq，而不只检查最终文本：

- 单行插入及插入并修改：新行缓存和 segment count 同步正确，`lineToRow`/`rowToLine` 与 `wrapLayout()` 一致，不创建 Job；
- 多行插入：后台结果按 seq 更新正确行，已移动的旧行号不会被误写；
- 删除一行、多行及最长行：RowIndex 同步删除，后续行映射正确，死缓存不参与 max width；
- Move：原 seq 和 layout 被复用，erase + insert 后索引指向正确；
- SetLine：只同步计算一次，segment count 与缓存一致；
- 对以上操作分别执行 Undo/Redo，再次核对索引、缓存和 max width；
- 多视图共享 Document 时，各视图的 RowIndex 都正确更新；
- 已有 `TestRowIndex` 继续负责索引算法本身，本新增集成测试负责编辑操作与索引的联动。

### 验证

先运行 `TestQSyneditDisplay` 和 QSynEdit 全量测试，再按要求执行 Release 全量 CTest、Debug 全量 CTest。使用 `miniaudio.h` 重复插入空行，确认后台计算行数和 scroller 更新次数，而不只观察最终 maximum。

## KISS 复审结论

本方案不需要新的布局框架、宽度索引或复杂状态机。核心改动只有“给 Job 一个行集合”和“把 max width 通知推迟到有效后”。`mMaxLineWidthMaybeStale` 继续表达结果可信度，新增的 update pending 只承担通知去重，语义不重叠。

不建议本次引入堆或有序 map：它们只在最长行频繁变短时减少 O(N) 缓存读取，却会要求每次缓存写入、删除和失效都维护额外索引，修改面和出错风险明显更大。
