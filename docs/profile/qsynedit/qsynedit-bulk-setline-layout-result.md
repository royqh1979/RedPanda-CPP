# QSynEdit 大批量 SetLine 布局实施结果

## 1. 实施范围与场景边界

本项依据 [大批量 SetLine 布局设计](../../design/qsynedit/qsynedit-bulk-setline-layout-design.md) 完成，父提交为 `ef3a5ce8`，实施分支为 `document-edit-bulk-layout`。

生产实现只修改 `onDocumentContentChanged()` 的 SetLine 分支：

- 单行 SetLine 继续同步计算精确布局；
- 多行 SetLine 将修改行 RowIndex 置为 1 段估计，使用现有 missing、按需同步和后台 Job 精化；
- dirty、contentVersion、max width、caret、滚动和通知协议不变。

场景边界按真实编辑语义确定：多行 SetLine 是删除/插入/替换（包括复制粘贴）过程中的内部 change，不存在独立的“多行修改”操作。收益结论只采用单行修改及 Insert/Replace 场景，不使用人工构造的多行 modify profile 作为收益证据。

未修改 Document、Undo/Redo、WrapLayoutJob、RowIndex 算法、painter、syntaxer、折叠或公共编辑接口。自动缩进仍由 `Document::putLine()` 逐行调用 `reparseLines()` 并维护 syntax state，本项只延迟 View 显示布局，没有延迟自动缩进所需的文本和语法状态。

## 2. 测试保护与审核迭代

新增和扩展测试覆盖：

- 不可见区域多行 SetLine 的 missing、1 段估计和后台 Job；
- 显式布局查询后的精确缓存与 RowIndex；
- 最终 line/row 映射、max width 及最长行变长/变短；
- 非 word-wrap 下的水平滚动通知；
- 两个 View 共享 Document 时的独立后台精化。

审核中修正了测试本身的三类场景错误：把可见区按需同步误认为必须创建后台 Job、在 word-wrap 下等待水平滚动通知，以及忽略等宽最长行和未全部替换最长行。生产实现没有为这些测试假设增加补丁。多行 max-width 通知被放入既有非 word-wrap 测试中，临时诊断输出均已删除。

## 3. 测试验证

- `TestQSyneditDisplay`：112 passed，0 failed；
- Release 全量 CTest：6/6 passed，140.86 秒；
- Debug 全量 CTest：6/6 passed，130.16 秒。

全量回归包含自动缩进、Undo/Redo、stale/abort batch、多视图、折叠和绘制测试。两种构建的全部测试均通过。

## 4. Profile 方法与结果

基线使用父提交 `ef3a5ce8` 的独立 worktree，与实现后代码采用相同配置：

- Qt 5.15.18 静态 Release，`d:/qt/mingw64` GCC 8.1.0；
- `miniaudio.h` 95844 行，offscreen，View 可见；
- 关闭 word-wrap，开启 C++ syntaxer 和代码折叠；
- 每场景 3 次有效采样并取中位数；初始等待超时或明显系统停顿的日志不纳入结果；
- 文件加载、初始布局和场景准备不计入编辑耗时。

单位为 ms。Insert/Replace 是实际批量路径，单行修改是无回退对照。

| 场景 | 基线 sync | 优化后 sync | sync 变化 | 基线 total | 优化后 total |
|---|---:|---:|---:|---:|---:|
| 修改 1 行，plain | 1.177 | 1.164 | 基本持平 | 1.768 | 1.747 |
| Insert 1000 行，plain | 40.936 | 4.616 | -88.7% | 41.549 | 50.487 |
| Insert 10000 行，plain | 396.981 | 31.494 | -92.1% | 399.180 | 424.978 |
| Replace 1000 行，plain | 40.800 | 7.574 | -81.4% | 41.367 | 50.121 |
| Replace 10000 行，plain | 397.523 | 32.555 | -91.8% | 397.995 | 404.555 |

重解析行数、change entries、changed lines 和 fold rescans 与基线一致。

## 5. 收益解释与限制

本项达到主要目标：真实批量 Insert/Replace 的 UI 同步耗时显著下降，不再由“同步布局全部修改行”主导。单行修改没有可观察回退。

total 是“编辑同步 + 后台布局全部完成”的时间，不保证下降。1000 行 Insert/Replace 的 total 增加约 9 ms，10000 行 Insert 增加约 26 ms；Replace 10000 行基本持平。当前后台 Job 仍遍历 Document 来寻找 pending 行，并增加线程和结果排队成本，因此把同步工作移到后台不等于消除总工作量。本项明确不以 total 全面下降为结论。

后续若要降低 total，应单独评估 WrapLayoutJob 的 pending 行定位方式，不能在本项中顺手扩大范围。自动缩进逐行 syntax state、必要的跨行 reparse 和后台最终布局都不能为追求 sync 数字而省略。
