# QSynEdit Pending Layout 增量维护设计

## 1. 目标与范围

消除 `QSynEdit::scheduleWrapLayoutJob()` 在普通编辑后遍历整个 Document、逐行查询 lineSeq 和 layout 缓存来重建 `pendingSeqs` 的固定成本。

本项只修改 View 侧待计算行集合的维护与调度：

- 新增现存且缺少有效 layout 的行 seq 集合；
- 在现有缓存写入、缓存失效和 Document change 处理点增量维护该集合；
- 调度时直接使用集合，不再扫描 Document；
- 保持现有同步布局、后台 Job、过期结果拒绝、max width 和 RowIndex 协议不变。

明确不改：

- `WrapLayoutJob::run()` 对 Document 的遍历方式；
- `computeWrapLayoutForLine()`、painter、syntaxer 和 Document；
- 大批量 SetLine 的同步布局路径；
- 删除后的语法重解析范围、折叠块维护和范围移动；
- 公共编辑接口。

## 2. 现状与根因

普通 SetLine 已经同步计算修改行并写回有效 layout，但仍设置 `mWrapAllComputed=false`。随后的 `scheduleWrapLayoutJob()` 为确认是否还有缺失行，遍历全部 Document 行并构造 `pendingSeqs`。对于缓存完整的单行修改，最终结果通常是不创建 Job，但 UI 线程已经承担 O(N) 扫描。

`mWrapDirtySeqs` 不能替代待计算集合。它记录行的内容版本，用于拒绝编辑前启动的 Job 所返回的过期结果；具有 dirty 标记的行可能已经完成同步布局并持有有效缓存。把 dirty 直接解释为 missing 会导致重复计算或错误接受过期结果。

## 3. 方案选择

### 方案 A：独立维护 missing 集合（采用）

新增 `mWrapMissingSeqs`，只表达“当前 Document 中已知缺少有效 layout 的行 seq”。全局失效时从 Document 一次建立全集；普通 Insert/SetLine 和缓存写入点只做集合同步增删。调度直接复制该集合作为 Job 的 `pendingSeqs`。

普通单行 SetLine 的调度变为 O(1)，单行插入为 O(1)，多行插入需要 O(K) 登记 K 个新行。全局失效仍需 O(N) 建集合，但该路径本来就清除全部缓存、重置 RowIndex 并计算全部行，不增加复杂度量级。

### 方案 B：扩展 dirty 集合并增加全量标志

复用 `mWrapDirtySeqs` 表示 missing，再用额外布尔值区分初次加载/全局失效。该方案字段较少，但会让同一集合同时承担内容版本拒绝和待计算选择两种语义。Insert、同步 SetLine、过期 batch 和已完成 batch 的清理条件不同，容易重复计算或提前标记完成，不符合 KISS，不作为实验实现。

### 方案 C：仅在 SetLine 后跳过调度

如果修改行已同步布局，直接把 `mWrapAllComputed` 设回 true 可以避开常见扫描，但无法证明其它行没有 missing，也不能正确处理初次布局、全局失效和 Job abort 后的混合状态。它只掩盖常见路径，未建立完整不变式，不作为实验实现。

因此本项只有方案 A 值得进入实现和 profile；没有为语义不完整或实质重复的方案额外建立实验分支。

## 4. 状态不变式

在 View 的调度入口和 Document change 处理完成后：

1. `mWrapMissingSeqs` 中的每个 seq 都属于当前 Document；
2. 当前 Document 的任一行若没有有效 `mWrapLayouts` 缓存，其 seq 必须在 `mWrapMissingSeqs` 中；
3. 已写入有效缓存的 seq 不得留在 `mWrapMissingSeqs`；
4. `mWrapDirtySeqs` 继续只负责按内容版本拒绝过期 batch，不参与待计算行选择；
5. `mWrapAllComputed=true` 表示当前 Document 没有 missing 行，而不是仅表示 dirty 集合为空。

后台计算期间允许 Job 持有集合快照。编辑发生后仍由现有 generation/contentVersion 机制拒绝旧结果；新一次调度使用最新 missing 集合。

## 5. 各维护点

### 全局失效

`invalidateAllLineMetrics()` 清除 layout 后，遍历当前 Document 一次，把所有现存行 seq 放入 `mWrapMissingSeqs`，再按现有逻辑调度。空 Document 不产生集合并直接完成。

### 同步布局写入

`wrapLayout()` 在同步计算并插入缓存后，从 missing 删除该 seq。除此之外不改变 RowIndex 和 max width 更新顺序。

### SetLine

删除旧缓存时把 seq 加入 missing，再执行现有 dirty 版本登记和同步 `wrapSegmentCount()`。同步写回缓存后，该 seq 自动从 missing 删除。若其它行没有 missing，调度发现集合为空，不创建 Job。

### Insert

处理 Insert change 时只登记仍无有效缓存的新行 seq。新行可能在 Insert change 到达前已经同步布局，例如回车拆分触发的中间查询；这种行不得再加入 missing。单行插入的现有同步布局会移除对应 missing；多行插入将未同步计算的 seq 留给后台。Insert+SetLine 仍遵循现有两次 change 的处理顺序，不合并操作。

### Delete

Delete 不删除其它现存行的缓存，但已删除 seq 不得留在 missing。处理 Delete 时仅在 missing 非空的情况下遍历该集合，通过 `findLineBySeq()` 移除已不存在的项。普通缓存完整时集合为空，删除不增加扫描。

### Move

lineSeq 和 layout 随行保留，不修改 missing。

### 后台 batch

接受有效 layout 时写入缓存并删除对应 missing。过期结果被拒绝时不改变现有同步缓存，也不仅凭 batch 把行标记为完成。

finished batch 根据最新 missing 是否为空更新 `mWrapAllComputed`。dead dirty seq 的清理保持原逻辑；不再用 dirty 是否为空决定是否继续调度。

### 空调度

`mWrapAllComputed=false` 但 missing 为空时，直接设置完成并执行现有 max width 完成处理，不扫描 Document、不创建空 Job。

## 6. 复杂度与内存

- 普通 SetLine 调度：由 O(N) Document 扫描降为 O(1) 集合查询；
- 单行 Insert：O(1) 集合增删；
- K 行 Insert：O(K) 登记，替代原来的 O(N) 调度扫描；
- Delete：仅 missing 非空时为 O(M)，M 为 missing 数；缓存完整时为 O(1)；
- 全局失效：O(N)，与原全量布局同量级；
- 额外内存：O(M)，缓存完整时集合为空。

后台 Job 仍按现有方式扫描 Document 来选择 pending 行，本项不把 UI 线程优化扩大成 Job 重构。

## 7. 测试保护

在 `TestQSyneditDisplay` 中补充调度与状态测试，并保留现有 RowIndex、max width、stale batch、abort、多视图和 Undo/Redo 测试：

1. 全量加载后 missing 覆盖所有未布局行，完成后为空；
2. 普通 SetLine 同步写回缓存，missing 为空且不创建 Job；
3. 单行 Insert、Insert+SetLine 不创建 Job，missing 为空；
4. 多行 Insert 只把未缓存新行交给后台，完成后缓存与 missing 一致；
5. 删除 missing 行会移除其 seq，不创建针对 dead seq 的重复 Job，其它 missing 行仍能完成；
6. Move 保留 seq、layout 和 missing 状态；
7. 全局字体/宽度失效重新建立完整 missing，最终所有行得到新 layout；
8. aborted/stale batch 后只计算仍缺失的行，已接受或同步计算的布局不重复计算；
9. `mWrapAllComputed=false`、missing 为空且缓存完整时，空调度直接完成且不创建 Job；
10. 对以上关键场景继续检查 RowIndex、max width 和最终文本，已有 Undo/Redo、多视图测试不得丢失。

## 8. 验证与收益评估

先运行 `TestQSyneditDisplay` 和 QSynEdit 测试，再依次运行 Release、Debug 全量 CTest。

使用已提交的 `QSYNEDIT_EDIT_PROFILE` 和 `VIEW_OVERHEAD_PROFILE`，在相同 Release 环境、相同 `miniaudio.h` 负载下对比 `d77823fa` 基线：

- 普通中部 SetLine 的 View sync/total；
- 单行 Insert、Delete、Move，确认无功能或明显性能回退；
- `schedule_pending` 微型 profile；
- 初次全量布局与多行 Insert，确认全局路径和新增集合维护成本。

记录原始样本、中位数、重解析行数和 Job 行为。若收益不足或正确性测试暴露状态缺口，先在本项内审核迭代，不通过扩大修改范围追求数字。

## 9. KISS 与范围复审

本方案只增加一个含义单一的集合并复用现有 Job `pendingSeqs` 接口，不引入新类、新线程协议或第二套布局路径。全局失效时显式建立全集比增加“全部 missing”特殊模式更直观，也避免空集合同时表示“全部”和“没有”的歧义。

修改应限于 `qsynedit.h/.cpp`、`test_qsynedit_display.*`、必要的设计/结果文档及 `AGENTS.md` 文档索引。出现需要改 Document、WrapLayoutJob 或批量布局才能解决的问题时，应停止并重新评审范围，而不是顺手扩大本项。
