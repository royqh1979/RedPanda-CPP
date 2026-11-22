# QSynEdit Pending Layout 增量维护实施结果

## 1. 实施范围

本项依据 [Pending Layout 增量维护设计](../../design/qsynedit/qsynedit-pending-layout-incremental-design.md) 完成，基线为 `d77823fa`，实施分支为 `document-edit-pending-incremental`。

实际修改仅涉及：

- `QSynEdit` 新增 `mWrapMissingSeqs`，增量记录现存且缺少有效 layout 的行；
- `scheduleWrapLayoutJob()` 直接使用该集合，不再遍历 Document 重建 pending；
- 同步/后台缓存写入、Insert、SetLine、Delete 和全局失效维护集合；
- `TestQSyneditDisplay` 新增 missing 生命周期和空调度测试；
- 更新相关设计、Agent 流程和本结果文档。

未修改 Document、WrapLayoutJob、布局算法、painter、RowIndex、批量 SetLine 同步布局、语法分析或折叠实现。

## 2. 审核迭代

专项测试最初发现一个真实时序遗漏：回车拆分等新行可能在 Insert change 到达前已经同步布局。无条件把新行加入 missing 后，`wrapLayout()` 因缓存命中直接返回，导致同一 seq 同时处于缓存和 missing，并创建一个不必要的 Job。

最终修正为 Insert 只登记仍无有效缓存的新行。既有 `test_single_line_edit_indexes_without_job` 保留了该场景并继续检查不创建 Job，没有修改原断言来绕过问题。临时诊断输出已删除。

设计中的 Insert 时序说明已同步修正。其它审核未发现 dirty/missing 语义混用、dead seq 重复调度或修改范围扩大。

## 3. 测试验证

- `TestQSyneditDisplay`：111 passed，0 failed；
- Release 全量 CTest：6/6 passed，44.40 秒；
- Debug 全量 CTest：6/6 passed，89.01 秒。

新增测试覆盖：

- SetLine、单行 Insert、多行 Insert 后的缓存/missing 互斥不变式；
- 删除 missing 行时清理 dead seq，其它 missing 行继续完成；
- Move 保留 seq 和 layout；
- 全局失效后的集合重建与最终完成；
- missing 为空时直接完成调度且不创建 Job。

既有 abort、stale batch、RowIndex、max width、Undo/Redo、多视图和全量替换测试均通过。

## 4. Profile 方法与直接 A/B 结果

为排除不同时段机器状态影响，除既有基线外，又在 `d77823fa` 的独立 worktree 中以相同配置直接构建并测量基线。两边均使用：

- Qt 5.15.18 静态 Release，`d:/qt/mingw64` GCC 8.1.0；
- `miniaudio.h` 95844 行，offscreen，View 可见；
- 关闭 word-wrap，开启 C++ syntaxer 和代码折叠；
- 每场景 3 次采样并取中位数；pending 微型 profile 采样 5 次；
- 文件加载、初始布局及场景准备不计入编辑耗时。

单位均为 ms。直接 A/B 使用同一时期的基线结果；删除和移动另做了交错复测。

| 场景 | 基线 sync | 优化后 sync | 基线 total | 优化后 total | sync 变化 |
|---|---:|---:|---:|---:|---:|
| 中部修改 1 行，plain | 6.489 | 1.424 | 7.945 | 2.708 | -78.1% |
| 中部插入 1 行，plain | 7.995 | 1.583 | 9.543 | 2.875 | -80.2% |
| 中部插入 1000 行，plain | 49.791 | 39.981 | 50.342 | 40.497 | -19.7% |
| 中部删除 1 行，plain | 1.800 | 1.608 | 2.836 | 2.135 | 非目标路径，无稳定回退 |
| 中部移动 1 行，plain | 1.737 | 1.947 | 2.188 | 2.439 | 非目标路径，存在运行波动 |

重解析行数、change entries、changed lines 和 fold rescans 与基线一致。

删除/移动不是本项优化路径。多轮测量中，删除 sync 约 1.608–3.430 ms，移动 sync 约 1.947–3.006 ms；交错复测时删除恢复到基线附近，因此不宣称这些场景有收益，也没有证据把波动归因于新增集合。移动路径没有新增算法工作。

### Pending 收集微型 profile

`schedule_pending` 在 95844 行、layout 已全部有效时，只测量调度前的 pending 收集：

| 实现 | 原始样本 | 中位数 |
|---|---|---:|
| 基线 | 5.936、5.630、5.543、5.214、5.944 | 5.630 |
| 优化后 | 0.001、0.0003、0.0001、0.0001、0.0003 | 0.0003 |

优化后的数值接近计时器分辨率，含义是“已不再执行可观察的全文档扫描”，不是对空调用耗时的精确保证。

`syncFoldVisibility()` 未修改；基线中位数约 0.762 ms，优化后约 0.657 ms，仍属于独立的次要固定成本。

## 5. 结论与限制

本项达到设计目标：普通 SetLine 的 UI 线程 pending 收集由 O(N) 降为 O(1)，单行修改和插入的同步耗时显著下降；多行插入也消除了调度前的全文档扫描。正确性、后台结果拒绝和索引协议均通过全量回归。

额外状态为 O(M) 的 missing 集合，缓存完整时为空。全局失效仍会 O(N) 建立集合并计算全部布局，本项没有对该路径单独计时，不宣称全局布局性能收益。后台 Job 仍按原方式遍历 Document 选择 pending 行，大批量 SetLine 的 UI 同步布局成本也未改变；这些留待各自独立项目处理。
