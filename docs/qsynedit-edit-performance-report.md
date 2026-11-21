# QSynEdit Document / Document+View 编辑性能分析报告

## 1. 范围

本次只增加 profiling 测试并分析现有实现，没有修改 Document、View、折叠、布局或绘制的生产代码。

重点测量 Document+View 的端到端编辑成本；Document 单独数据用于区分文本/语法工作和 View 工作。测试基于 `libs/qsynedit/test/resources/miniaudio.h`，文件包含 95844 行。

## 2. 测量方法

### 2.1 编译与运行环境

- Windows 11，Qt 5.15.18 静态 Release；
- `d:/qt/mingw64` GCC 8.1.0；
- 构建目录位于 R: 内存盘；
- Qt offscreen 平台，View 在编辑矩阵中保持可见；
- 关闭 word-wrap，开启 C++ syntaxer 和代码折叠；
- 文件加载、初始语法分析、初始布局和场景准备均不计入编辑耗时。

这些数值用于比较代码路径和数量级，不等同于真实窗口、显卡和老机器上的最终体验。

### 2.2 场景矩阵

五类编辑均在一个 Document 更新批次内完成：

- `modify`：修改已有的 N 行；
- `insert`：插入 N 行并写入正式内容；
- `delete`：删除 N 行；
- `replace`：删除 N 行、插入 N 行并写入正式内容；
- `move`：把连续 N 行向下移动 100 行，保持块内行顺序。现有接口只有单行 `moveLine()`，因此该场景由 N 次单行移动组成。

每类操作覆盖 1、10、100、1000、10000 行，并区分：

- `plain`：不改变折叠分隔符；
- `fold`：增加、删除或移动 `{`/`}`，改变折叠边界。

两类负载使用相同的声明主体和相同显示宽度，只有末尾分隔符不同，避免把文本长度差异误判为折叠成本。

`modify` 另测文档前部、中部和尾部，区分编辑行成本与后续语法状态重扫成本。所有场景检查最终行数、重解析计数以及 fold-dirty 标记是否符合场景定义。

Document 全矩阵每场景采样 3 次并取中位数；View 全矩阵采样 1 次，关键场景另行采样 3 次并取中位数。`scheduleWrapLayoutJob()` 微型 profile 采样 5 次。

### 2.3 时间口径

- Document `sync`：编辑批次完成、Document 信号同步处理完毕的时间；
- View `sync`：包括 Document 工作、View 的同步折叠/索引/布局处理及任务调度；
- View `total`：继续处理事件，直到后台布局全部完成，包括 offscreen 绘制事件；
- `reparsed lines`：本次编辑中 `linesReparsed` 报告的累计工作量，同一行被多个 primitive 重复解析时会重复计数。

## 3. 关键基线

完整的 Document、Document+View 场景矩阵及微型 profile 原始样本见 [QSynEdit Document / Document+View 编辑性能基线](qsynedit-edit-performance-baseline.md)。

单位均为 ms。普通修改、插入、10000 行修改、10000 行折叠删除和 1000 行移动的 View 数据为 3 次中位数；其余 View 数据为全矩阵单次采样。

| 场景 | Document sync | View sync | View total | 重解析行数 |
|---|---:|---:|---:|---:|
| 中部修改 1 行，plain | 0.005 | 6.885 | 7.338 | 1 |
| 中部插入 1 行，plain | 0.065 | 11.353 | 11.898 | 2 |
| 中部删除 1 行，plain | 0.462 | 1.388 | 1.893 | 1 |
| 中部替换 1 行，plain | 0.516 | 6.917 | 7.364 | 2 |
| 中部移动 1 行，plain | 0.681 | 1.624 | 2.087 | 103 |
| 前部修改 1 行，fold | 119.470 | 138.145 | 138.648 | 92109 |
| 中部修改 1 行，fold | 58.463 | 75.753 | 76.231 | 47923 |
| 尾部修改 1 行，fold | 1.214 | 8.350 | 8.801 | 1001 |
| 中部修改 1000 行，plain | 1.864 | 46.008 | 46.475 | 1000 |
| 中部修改 10000 行，plain | 18.313 | 381.477 | 381.952 | 10000 |
| 中部插入 10000 行，plain | 24.221 | 394.411 | 395.832 | 20000 |
| 中部删除 10000 行，plain | 14.753 | 16.297 | 16.802 | 10000 |
| 中部删除 10000 行，fold | 14.800 | 418.260 | 418.784 | 10000 |
| 中部移动 1000 行，plain | 575.115 | 721.315 | 721.892 | 101000 |
| 中部移动 10000 行，plain | 6271.859 | 7487.583 | 7488.047 | 1010000 |

大多数场景的 `total - sync` 小于 1 ms。主要问题在编辑同步路径，而不是等待后台线程或 offscreen 绘制。

## 4. 已确认热点

### 4.1 每次 SetLine 都扫描全文档收集 pending 行

`libs/qsynedit/qsynedit/qsynedit.cpp:459` 的 `scheduleWrapLayoutJob()` 在每次需要布局时遍历全部 Document 行，通过 `getLineSeq()` 和 layout 哈希表重新构造 `pendingSeqs`。

微型 profile 在 95844 行、所有 layout 均已有效的情况下，只执行这段收集逻辑：

- 5 次结果：7.589、5.439、5.106、5.129、5.314 ms；
- 中位数：约 5.314 ms。

普通单行修改只重解析 1 行，Document 约 0.005 ms，而 View 同步耗时约 6.885 ms。关闭代码折叠后，普通单行 View 修改仍约 5.356 ms。由此可以确认，全文档 pending 收集是普通单行编辑的首要固定开销，不是语法分析或折叠重扫造成的。

### 4.2 大批量 SetLine 在 UI 线程同步布局所有修改行

`libs/qsynedit/qsynedit/qsynedit.cpp:8383` 的 SetLine 分支逐行删除缓存，并立即通过 `wrapSegmentCount()` 同步计算每行布局、更新 RowIndex。

普通/折叠负载等长后，10000 行修改的 View 同步耗时约 381–402 ms，插入和替换也约 394–416 ms；Document 部分仅约 18–26 ms。关闭折叠后，10000 行普通修改仍约 390 ms。

因此 bulk edit 的主要成本是 View 逐行同步布局及索引更新，不是 fold rescan，也不是后台任务等待。

### 4.3 含大量折叠块的删除触发昂贵的块维护

同一个删除 10000 行 fold 场景：

- 开启折叠：View sync 中位约 418.260 ms；
- 关闭折叠：View sync 中位约 37.916 ms；
- Document：约 14.800 ms。

约 380 ms 差值来自 View 折叠维护。`processFoldsOnLinesDeleted()` 在反向遍历 `mCodeBlocks` 时逐个 `QVector::remove()`，大量连续删除会产生反复移动元素的成本；之后 `partialScanCodeBlocks()` 还需多次遍历旧块列表。

普通 10000 行删除没有这部分成本，View sync 约 16.297 ms。

### 4.4 删除 N 行后会重解析后续 N 行

`libs/qsynedit/qsynedit/document.cpp:645` 的 `deleteLines()` 在删除后调用：

```cpp
reparseLines(index, index + numLines, true);
```

被删除的行已经不存在，但请求范围仍使用删除数量。plain 删除 1/10/100/1000/10000 行时，实测重解析行数恰为 1/10/100/1000/10000。10000 行 plain 删除的 Document 耗时约 14.753 ms，而普通修改 10000 行约 18.313 ms，说明这部分不是免费的边界检查。

真正需要重新验证的是删除点后当前行及其状态传播；没有必要仅因为删除了 N 行就强制解析后续 N 行。

### 4.5 插入和替换把空行、正式内容各解析一遍

插入矩阵先 `insertLines()`，再逐行 `putLine()`，与现有编辑 primitive 的组合方式一致。实测：

- 插入 N 行累计解析约 2N 行；
- 替换 N 行累计解析约 2N 行；
- 插入 10000 行解析 20000 行，Document 约 24.221 ms。

空行状态的中间结果不会成为批次的最终结果，这是一次可消除的重复工作。

### 4.6 连续行块移动是 N 次局部全扫

单行移动 100 行会解析约 103 行。N 行块移动由 N 次 `moveLine()` 组成，因此：

- 1000 行移动解析约 101000 行；
- 10000 行移动解析约 1010000 行；
- Document 中位耗时分别约 575 ms 和 6.27 s；
- View 总耗时分别约 721 ms 和 7.49 s。

关闭折叠后，1000 行 View 移动约 581 ms，说明主要成本仍是 Document 的重复语法扫描；View 的每个 Move change 又逐次遍历折叠块，进一步增加成本。

### 4.7 未配对分隔符导致必要的尾部状态传播

中部单行加入未配对 `{` 时，Document 重解析约 47923 行，约 58 ms；前部约 92109 行，约 119 ms；尾部只有约 1001 行，约 1.2 ms。

这是跨行语法状态变化的真实语义成本，不能通过“只解析修改行”来优化。View 还会把折叠配对更新到 freeze line，因此总成本更高。

### 4.8 次要固定成本

`syncFoldVisibility()` 微型 profile 每次约 0.6–1.0 ms。它值得后续检查重复调用，但相对 pending 全扫、bulk layout 和大块移动不是第一优先级。

## 5. 优化建议与优先级

### P0：消除普通编辑的全文档 pending 收集

让 dirty/missing 行集合在编辑和缓存失效时增量维护，调度 Job 时直接消费已知集合，不再遍历整个 Document 重建。

现有 `mWrapDirtySeqs`、Insert 的已知行范围和 layout 删除点已经提供了大部分信息。设计时须区分“需要拒绝旧结果”和“仍需计算”两种状态，不能简单复用一个集合而重复计算已同步完成的行。

预期收益：普通单行 View 编辑可减少约 5 ms，且文件越大收益越明显。优先补单行、单行插入并修改、后台 abort、字体/宽度全局失效和多视图测试。

### P0：为大块修改设计批量布局路径

不要在 UI 线程对所有修改行逐一同步 `wrapSegmentCount()`。建议单独评审以下方案：

- 可见行同步计算，保证当前屏和 caret 正确；
- 其余修改行进入批量后台布局；
- RowIndex 和 max width 在批次到达时统一更新，避免临时滚动范围变化。

这会触及现有“SetLine 后索引立即精确”的约定，不能直接改成异步而不设计中间状态。应先补 bulk edit、滚动位置、折叠、多视图和 Undo/Redo 测试，再实施。

### P1：修正删除后的重解析范围

删除后只需从 `index` 的当前行开始验证并传播至收敛，不应把 `numLines` 当作删除后仍需强制解析的行数。EOF 删除继续走现有空范围处理。

必须保留先删除、后 setLine 修复状态的既有协议，并覆盖普通删除、块注释/字符串跨行状态、折叠边界和 EOF。

### P1：让批量插入直接接收正式内容

提供内部批量插入路径，在创建 DocumentLine 时就写入正式文本，然后统一解析一次，避免“空行扫描 + 正式内容扫描”。

不建议仅在现有调用中随意把多个 `parseToEnd=false` 改成延迟处理，因为每个 primitive 仍负有维护局部 syntax state 的职责；需要明确批处理契约。

### P1：批量移除失效折叠块

把删除范围内的块一次性移除或单遍筛选重建，避免在循环中反复 `QVector::remove()`。保留折叠状态匹配、每个 View 独立的折叠状态以及结束行可见性规则。

A/B 数据表明该场景有约 380 ms 的直接收益空间。

### P1：增加真正的范围移动语义

连续行块移动应作为一个操作：

- 一次性移动 Document 行并保留 lineSeq/layout；
- 产生一个范围级 change；
- 对受影响区间统一重解析并传播到收敛；
- RowIndex、折叠块、caret/selection 和 Undo/Redo 按同一范围更新。

不要把当前 N 次单行 primitive 简单包装后仍逐项处理，那样无法消除 O(N×距离) 和 Move×折叠块的成本。

### P2：再优化必要的语法尾部扫描

消除上述无意义工作后，再分析 `reparseLines()` 内循环。可检查递归锁、重复 Document accessor、状态对象分配和 Syntaxer 扫描成本，但不应牺牲状态收敛和折叠正确性。

## 6. 复现方式

配置独立 profiling 构建：

```sh
cmake -S . -B /r/build-redpanda-qt5-release-edit-profile \
  -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/d/qt/qt5-static-64 -DFORCE_QT5=ON \
  -DQSYNEDIT_BUILD_PROFILE_TESTS=ON
cmake --build /r/build-redpanda-qt5-release-edit-profile \
  --target test-qsynedit --parallel
```

在 `libs/qsynedit` 测试运行目录中：

```sh
QSYNEDIT_EDIT_PROFILE=1 QSYNEDIT_EDIT_PROFILE_REPEAT=3 \
  ./test-qsynedit.exe --suite=TestDocumentProfile test_edit_matrix

QSYNEDIT_EDIT_PROFILE=1 QSYNEDIT_EDIT_PROFILE_REPEAT=1 \
  ./test-qsynedit.exe --suite=TestQSyneditDisplay test_miniaudio_edit_profile

QSYNEDIT_EDIT_PROFILE=1 \
  ./test-qsynedit.exe --suite=TestQSyneditDisplay test_miniaudio_view_overhead_profile
```

View 可用 `QSYNEDIT_EDIT_PROFILE_FOLDING=0` 做折叠开关对照。输出为 `DOCUMENT_EDIT_PROFILE`、`VIEW_EDIT_PROFILE` 和 `VIEW_OVERHEAD_PROFILE` 前缀的 CSV 风格行。原始日志的持久化汇总脚本及使用方法见 [QSynEdit 编辑性能 Profile 工具](qsynedit-edit-performance-tools.md)。

`QSYNEDIT_BUILD_PROFILE_TESTS` 默认 OFF；Syntaxer、增量布局和编辑 profile 都不会进入普通测试构建。

## 7. 限制与后续测量

- 本次没有开启 word-wrap，也没有测多 View 共享 Document 和折叠块预先 collapsed 的组合矩阵；
- View 全矩阵为单次采样，关键结论使用额外中位数采样和微型 profile 验证；
- Document 与 View 在不同进程中测量，差值用于定位数量级，不应视为严格的函数级 self-time；
- offscreen 结果不能替代真实 GUI 和老机器测试；
- 本报告只提出优化方向，未验证任何优化实现的收益或正确性。
