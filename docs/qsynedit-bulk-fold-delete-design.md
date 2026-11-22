# QSynEdit 折叠块批量删除设计

## 1. 目标与范围

消除大量折叠块随多行删除逐项调用 `QVector::remove()` 造成的反复元素搬移。

本项只修改 `QSynEdit::processFoldsOnLinesDeleted()` 的块列表维护方式：

- 保留现有 opener/closure 删除、范围平移、collapsed 清除和空块判断规则；
- 把反向遍历、逐项删除改为正向单遍筛选、一次替换块列表；
- 保持块对象、父子关系、顺序和后续 `partialScanCodeBlocks()` 输入语义不变。

不修改折叠扫描、显示、结束行可见性、自动缩进、Document、Undo/Redo、Insert/Move 或公共编辑接口。

## 2. 现状与根因

当前实现反向遍历 `mCodeBlocks`，在 opener 被删除或范围无效时执行 `mCodeBlocks.remove(i)`。连续删除大量块时，每次 remove 都可能搬移后续元素，总成本接近 O(B²)，B 为块数。

删除逻辑本身不需要反向遍历：每个块的保留与否只由当前块和删除范围决定，不依赖其它被删除块。

## 3. 方案选择

### 方案 A：正向单遍筛选（采用）

创建 `keptBlocks`，预留原块数，按原顺序遍历：

- opener 在删除范围内：不保留；
- closure 在删除范围内：保留，并按原规则展开；
- opener 在删除范围之后：保留并平移；
- closure 在删除范围之后：保留并平移，或按原规则丢弃无效块；
- 其它情况：保留原对象。

遍历结束后一次 `mCodeBlocks = keptBlocks`。分支条件和执行顺序与原逻辑一致，collapsed 状态变化继续统一触发原有滚动和重绘处理。

时间复杂度降为 O(B)，额外空间为 O(B) 的 shared_ptr 指针数组，不复制 CodeBlock 对象。

### 方案 B：使用标准库 remove_if 擦除

语义上与方案 A 相同，但 Qt 容器和现有代码风格下，显式筛选更直观，也便于保持每个分支完全对应。无需为同一算法另建实验分支。

### 方案 C：延后到 partial scan 全量重建

直接跳过删除维护会丢失 collapse 状态匹配、链块初始栈和 before-start 保留块，不能作为等价优化。只优化 partial scan 而不修正删除维护也无法消除当前 O(B²) 热点。

## 4. 必须保持的语义

- opener 被删除的块必须移除；
- 仅 closure 被删除的块保留，供 partial scan 重新确定范围；
- 删除范围后的块按原规则平移；
- collapsed 块在范围变化时按原规则展开；整块删除后保留下来的同范围 collapsed 状态继续由 partial scan 恢复；
- `subBlocks`、parent weak_ptr 和 shared_ptr 对象身份不被复制或重建；
- 列表顺序保持原顺序；
- 每个 View 仍独立维护自己的 `mCodeBlocks`，Document 和其它 View 的折叠状态不受影响；
- 结束行后缀可见性、占位符、caret 和 Undo/Redo 行为不变。

## 5. 测试保护

保留现有整块删除、fold shift、partial/full scan、共享 View 和 Undo/Redo 测试，并补充多块删除：

- 多个独立块与嵌套块同时删除；
- 删除范围包含前块的 closure、多个完整块和后块 opener 前的内容；
- 删除前后均有 collapsed 块，验证保留状态；
- 将删除后的 `foldRangesForTest()` 与 `fullRescanCodeBlocksForTest()` 结果比较；
- 两个 View 的 collapsed 状态仍各自独立。

使用 10000 行 fold 删除 profile 验证耗时下降，并比较普通删除、fold 开关和最终 fold ranges，防止以少做必要扫描换性能。

## 6. 验证与收益评估

先运行 `TestQSyneditDisplay` 和 QSynEdit 测试，再依次运行 Release、Debug 全量 CTest。

以 `cf005070` 为父提交，对比 10000 行 fold 删除的 Document sync、View sync/total、fold rescans 和最终块列表；普通 10000 行删除作为非目标路径对照。重点验证开启折叠时由 QVector 逐项 remove 造成的主要成本是否消失。

## 7. KISS 与范围复审

本项只改变一个函数内的列表维护方式，不需要新容器、索引、状态机或折叠扫描改动。正向筛选与原分支一一对应，是范围最小且容易验证等价性的实现。
