# QSynEdit 大批量 SetLine 布局设计

## 1. 目标与范围

消除大批量连续 SetLine 时，UI 线程在 `onDocumentContentChanged()` 中逐行同步调用 `wrapSegmentCount()`、立即计算全部修改行布局的固定成本。

目标是缩短编辑同步路径，让非立即需要的行沿用现有 missing + 后台布局机制；不是减少最终必须完成的布局总量，也不是改写布局算法。

本项只修改连续 SetLine change 的 View 处理：

- 单行 SetLine 保持现有同步精确布局和通常不创建 Job 的行为；
- 多行 SetLine 不再无条件逐行同步布局；
- 修改行进入现有 `mWrapMissingSeqs`，RowIndex 使用已有的 1 段估计；
- 现有同步查询、后台 batch、max width 和滚动位置协议负责后续精化。

明确不改 Document、Undo/Redo、WrapLayoutJob、RowIndex 算法、布局算法、painter、syntaxer、折叠、Insert/Delete/Move 和公共编辑接口。

自动缩进依赖 Document 的逐行文本、syntax state 及其向后传播，不依赖 View 的 wrap layout。`Document::putLine()` 仍逐行调用 `reparseLines()`，自动缩进需要的前行 state 和文本查询不得延后或省略。本项延迟的只是 View 显示布局，不能用 missing/后台 layout 代替自动缩进所需的逐行语法状态。

## 2. 现状与根因

连续修改在一个 Document change 中报告为 `SetLine(line, count)`。当前实现对每一行执行：

1. 删除旧 layout，登记 missing 和 dirty；
2. 设置 `mWrapAllComputed=false`；
3. 立即调用 `wrapSegmentCount()`，同步计算该行完整布局并更新 RowIndex。

因此 N 行修改在编辑信号处理期间就完成 N 次布局。`scheduleWrapLayoutJob()` 虽然已不再扫描全 Document，但此时所有修改行已有缓存，通常没有后台工作可做。10000 行场景的主要成本仍发生在 UI 线程。

## 3. 可复用的中间状态协议

现有全局失效和后台布局已经定义了不完整布局下的行为：

- missing 行没有有效缓存；
- RowIndex 对 missing 行使用 1 段估计；
- `wrapLayout()` 被绘制、caret 或滚动查询需要时同步计算，并精确更新对应行；
- 后台 Job 按 seq 返回布局，View 接受后精确更新 RowIndex；
- 布局未完成时垂直滚动范围是下界，不允许把当前 top 位置错误收缩；
- max width 只读取有效缓存，必要时等布局完成后重新确定并通知。

多行 SetLine 可以直接复用该协议，不需要第二套“批量布局中间状态”。

## 4. 方案选择

### 方案 A：多行先估计，按需同步，其余后台（采用）

单行 SetLine 保留现有同步路径。多行 SetLine 对每行执行既有缓存失效和 dirty/missing 登记，并把 RowIndex 的 segment count 置为 1；不再主动调用 `wrapSegmentCount()`。

后续行为完全沿用现有接口：

- 当前 top、重绘结束行、caret 或 painter 真正需要某行时，由现有同步查询计算；
- 其它行由后台 Job 计算；
- 每个有效结果按 seq 精化缓存和 RowIndex；
- 全部 missing 清除后执行现有完成处理。

该方案只改变多行分支，不改变任何查询接口的语义。

### 方案 B：预先遍历可见区并主动同步可见行

在 change 处理中先计算可见行范围，再主动同步这些行，其余后台。它可以让可见行布局时机更明确，但需要额外遍历和可见区判定，而且现有 top、invalidate 和 painter 查询本来就会按需计算相同的行。两套可见性策略还会增加折叠、估计映射和多视图场景的边界条件。

方案 B 没有独立于方案 A 的新正确性机制，预期只是重复现有按需行为，不符合 KISS，不建立额外实验分支。

### 方案 C：保留旧 segment count 直到后台返回

该方案虽然省去 RowIndex 置 1 的工作，但旧 segment count 对应旧文本，可能大于真实值，既不是安全下界也不能用于正确定位。它会产生错误滚动范围和 caret 映射，不是可接受的备选实现。

## 5. 单行与多行契约

### 单行 SetLine

保持现有行为不变：同步计算修改行、更新精确缓存和 RowIndex；若没有其它 missing 行，不创建 Job。普通打字和现有单行 profile 不应因本项回退。

### 多行 SetLine

对每个修改行：

1. 处理当前最长行失效，规则不变；
2. 删除旧 layout；
3. 登记 missing 和 dirty，规则不变；
4. 将 `mRowIndex` 对应行设为 1 段估计；
5. 设置 `mWrapAllComputed=false`，但不主动计算布局。

change 处理完成后仍执行现有 caret/selection、fold、anchor、invalidate 和 schedule 流程。部分行可能在这些流程中被同步计算，这是现有按需语义，不应额外禁止或重复计算。

## 6. RowIndex、caret 与滚动

多行修改后、后台完成前，允许 RowIndex 是“已按需计算行的精确值 + 其余行的 1 段估计”。这与全局失效后的状态一致。

- top anchor 若位于修改行，现有 `recalcTopPosFromAnchor()` 会按需取得其布局并重新定位；
- caret/selection 查询需要修改行时，通过 `wrapLayout()` 得到精确值；
- painter 需要显示修改行时逐行按需计算；
- 后台接受结果时仍验证 seq 和原行号，避免把结果写到已移动的行；
- `mWrapAllComputed=false` 时继续沿用滚动范围下界和 top 防收缩规则；
- 完成后所有 segment count 必须与实际 layout 一致。

不把 RowIndex 改为异步接口，也不引入“未知 segment count”的新枚举或特殊值。

## 7. Max width 与通知

现有规则不变：

- 修改当前最长行时立即使 max width 失效，期间不发送临时水平滚动范围；
- 非最长行的有效 layout 返回后与当前最大值比较；
- 同步或后台写入缓存都可能更新候选；
- 全部 missing 清除后，必要时从有效缓存重新确定最长行并完成通知。

本项不保证 total 时间下降，因为全部布局工作仍然存在；主要收益是 UI 同步编辑时间下降。水平滚动通知次数不得因分批返回而增加。

## 8. 测试保护

保留现有单行、索引、abort、stale batch、max width、多视图和 Undo/Redo 测试，并补充：

1. 多行 SetLine 返回时不主动布局全部修改行，存在 missing 且后台 Job 已调度；
2. missing 行 RowIndex 为 1 段，已按需计算行与缓存精确值一致；
3. top 或 caret 位于修改区域时，同步查询得到正确布局，最终映射一致；
4. drain 后所有修改行缓存、segment count、line/row 映射和最终文本正确；
5. 多行变长、变短及包含当前最长行时，max width 与水平滚动通知正确；
6. 多视图共享 Document 时，各 View 独立 missing、同步查询和后台精化均正确；
7. Undo/Redo 再次经过多行 SetLine 后，最终缓存、索引和文本一致；
8. 单行 SetLine 继续同步完成且不创建重复 Job；
9. 已有 aborted/stale batch、全量失效、Insert/Delete/Move 测试不得回归。

## 9. 验证与收益评估

先运行 `TestQSyneditDisplay` 和 QSynEdit 测试，再依次运行 Release、Debug 全量 CTest。

在相同 Release 环境对比本项父提交与实现后的 `miniaudio.h` profile。真实多行编辑只按删除/插入/替换（包括复制粘贴）理解；多行 SetLine 是这些操作内部的 change，不存在独立的“多行修改”场景，修改操作只按单行评估：

- Insert/Replace 1000、10000 行，确认共享 SetLine 分支的同步路径收益；
- 修改 1 行，确认单行路径无回退；
- 重解析行数、change 数量、fold rescans、最终索引和 Job 完成状态。

重点评价 sync 收益与 total 成本，不把后台等待包装成总工作量减少。若 total 明显上升，需要分析同步查询与后台是否重复计算，不能只看 sync 数字。

## 10. KISS 与范围复审

实现只需在多行 SetLine 分支把“立即精确计算”改为“1 段估计”，其它机制均已存在并通过测试。无需可见区计算器、新 Job 类型、新 RowIndex 状态或异步公共接口。

生产改动应限于 `onDocumentContentChanged()` 的 SetLine 分支；测试限于 `test_qsynedit_display.*`，另更新设计、结果、现状文档和 NEWS。若必须修改 WrapLayoutJob、RowIndex 或 Document 才能维持协议，应停止并重新评审，而不是扩大本项。
