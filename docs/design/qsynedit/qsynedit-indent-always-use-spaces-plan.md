# QSynEdit 缩进使用空格与双制表宽度设计

> 状态：备选设计稿，尚未实施。与 `qsynedit-indent-tab-size-plan.md` 并列；不修改原方案。目标默认值：`indentSize = 4`，`tabSize = 4`。

## 1. 核心思路

当前 `TabsToSpaces` 把任意位置的 Tab 输入都转换为空格，但正文中的空格不能自然代替 Tab：Tab 根据当前像素位置前进到下一个制表位，固定数量的空格在非等宽字体或不同前缀下不一定到达相同位置。

本方案先把该选项收窄为 **Indent Always Use Spaces**：只有以行首缩进为目的的操作才根据选项生成空格或 Tab，正文中的 Tab 输入始终写入真正的 `\t`。随后再把缩进宽度与正文制表宽度拆开：

- `indentSize`：行首缩进编辑步长，以及行首缩进区 Tab 的显示宽度；
- `tabSize`：首个非空白字符之后的 Tab 显示宽度；
- `IndentAlwaysUseSpaces`：只控制缩进操作生成空格还是 Tab，不控制正文 Tab 输入。

不引入通用逻辑列。编辑只分析 Document line 的空白前缀；绘制只使用实际像素位置和“是否仍在行首缩进区”的状态。

## 2. 精确定义

### 2.1 Document line 与行首缩进区

“行”始终指 Document line，不是显示 row，也不是 word-wrap segment。

行首缩进区是从 Document line 的 `ch == 0` 开始，仅由空格和 Tab 组成的最大前缀。首个非空白字符是边界；全空白行的行尾也是边界。

软换行不会建立新的行首。若 `<Tab>foo<Tab>bar` 在第二个 Tab 前折行，第二个 Tab 虽然位于显示段开头，仍属于正文 Tab，因为它在源 Document line 中位于 `foo` 之后。

### 2.2 Tab 输入

- caret 位于行首缩进区：
  - 开启 `IndentAlwaysUseSpaces` 时，插入空格并前进到下一个 `indentSize` 缩进位；已经在边界时插入完整一级；
  - 关闭时插入一个 `\t`。
- caret 位于正文时，无论选项是否开启，都插入一个 `\t`。
- 有选区且执行块缩进时，按块缩进规则处理，不把正文中的每个 Tab 字符转换为空格。

行首缩进位只需根据空白前缀计算。空格累计缩进余数，已有 Tab 表示前进到完整缩进位；不需要也不允许扫描正文字符来计算逻辑列。第一阶段尚未引入 `indentSize` 时，沿用现有 `tabSize` 作为唯一缩进步长。

### 2.3 自动缩进与块缩进

- 自动缩进每一级使用 `indentSize`；只分析参考行的空白前缀，不引入正文逻辑列。
- 可以保留参考行缩进前缀时直接复制，再追加或删除一级，避免无必要的 Tab/空格换算。
- 块缩进始终增加一级：开启选项时插入 `indentSize` 个空格，关闭时插入一个 Tab。
- 块反缩进删除行首一个 Tab，或最多 `indentSize` 个连续空格。
- 缩进参考线的层级距离使用 `indentSize`。

### 2.4 Backspace 与 Delete

只有开启 `IndentAlwaysUseSpaces` 且操作位于行首缩进区时，才按 `indentSize` 缩进位成组处理空格：

- Backspace 最多删除到上一个缩进位；
- Delete 最多删除到下一个缩进位；
- 空格不足一级时删除存在的连续空格；
- 遇到实际 Tab 时仍只删除一个 Tab。

正文中的空格始终按普通 glyph 删除，不再根据制表位成组删除。关闭选项时保持单 glyph 删除；块反缩进仍按上一节处理。

### 2.5 Tab 显示

布局扫描只维护源 Document line 的像素 X 和缩进区状态：

- 行首缩进区中的 Tab 使用 `indentSize * spaceWidth()` 作为像素缩进间距，从实际像素位置前进到下一个缩进位；
- 正文中的 Tab 使用 `tabSize * spaceWidth()` 作为像素制表间距，从实际像素位置前进到下一个制表位；
- 两种宽度都必须大于 0；
- 空格不会结束缩进区，首个非空白字符才会结束；
- 不从像素位置反推编辑所需的空格数量，不引入 CJK、组合字符或 emoji 的逻辑列。

软换行段不得重新初始化缩进区状态，也不得把段首当作新的 Document line 起点。布局、caret、鼠标命中、列编辑和拖选必须复用同一份源行 glyph 位置结果，避免因窗口宽度变化使 Tab 宽度反复变化。

### 2.6 设置变化与已有文本

无论选项是否开启，修改 `IndentAlwaysUseSpaces`、`indentSize` 或 `tabSize` 都不会自动改写文档中的 Tab 或空格。它们只影响后续编辑及当前视图的显示。

两个视图共享同一 Document 时，可以各自使用不同字体、`indentSize` 和 `tabSize`。Document 与 DocumentLine 不保存这些视图设置。

## 3. 设置与 API

### 3.1 第一阶段：收窄现有选项

QSynEdit 的 `EditorOption::TabsToSpaces` 改名为 `EditorOption::IndentAlwaysUseSpaces`，所有调用点同步改成缩进语义，不保留一个名称仍表示全局转换的旧接口。

IDE 设置同步改为 `indentAlwaysUseSpaces`。设置页文案表达“缩进始终使用空格”，不再暗示正文 Tab 也会转换。旧配置 `tab_to_spaces` 作为新选项的兼容来源读取；新配置可使用 `indent_always_use_spaces`。正文 Tab 不再转换是有意的行为变化，实施时记入 NEWS。

此阶段仍只有现有 `tabSize`，默认 4，不改变 Tab 显示规则。

### 3.2 第二阶段：拆分宽度

新增：

```cpp
int indentSize() const;
void setIndentSize(int size);
```

- QSynEdit 默认 `indentSize = 4`、`tabSize = 4`；
- IDE 新配置 `indent_size` 默认 4；
- 旧配置中的 `tab_width` 原样保留，缺少 `indent_size` 时回退 4；
- `Editor::applySettings()` 分别应用两个值；
- 设置页提供独立的 Indent Size 与 Tab Width，默认值均为 4。

`setIndentSize()` 和 `setTabSize()` 都必须失效相关布局、最大行宽、caret/鼠标命中缓存并触发重绘。`GlyphCalculator` 持有两个值，并随后台布局任务的值对象快照传递；旧 generation 批次继续按现有规则丢弃。

HTML/RTF exporter 目前使用独立构造参数，不在本方案中扩展，也不宣称导出复现两种 Tab 宽度。

## 4. 实施阶段

### P1：Indent Always Use Spaces 行为改造

- 重命名现有选项及 IDE 设置，完成旧配置兼容；
- 只修改 Tab 输入和 Backspace/Delete 的作用范围：空格转换及成组删除仅限行首缩进区；
- 正文 Tab 始终插入 `\t`，正文空格恢复普通单 glyph 删除；
- 自动缩进和块缩进保持现有单宽度行为；
- 删除不再需要的正文 `calcLogicalColumn()` 使用，行首只保留空白前缀计算；
- 暂不增加 `indentSize`，暂不改变 GlyphCalculator 和 painter 的显示算法。

### P2：拆分 indentSize 与 tabSize

- 增加两个设置及 IDE 映射，默认值均为 4；
- 自动缩进、块缩进/反缩进、行首 Tab 编辑和缩进参考线改用 `indentSize`；
- GlyphCalculator、普通布局和 word-wrap 布局传递源行缩进区状态；
- 行首 Tab 使用 `indentSize`，正文 Tab 使用 `tabSize`，全部基于实际像素位置；
- 统一 caret、鼠标命中、列编辑和拖选的 glyph 位置来源，不引入通用逻辑列。

### P3：回归与兼容性验收

- 先运行 Release 全量 CTest，再运行 Debug 全量 CTest；
- 对比默认设置下的旧编辑和显示结果；
- 检查旧配置迁移、NEWS、翻译及多视图行为；
- 只有测试证明需要时才调整实现，不扩大到自动检测项目缩进风格。

## 5. 测试计划

### 5.1 P1 编辑行为矩阵

在多个 `tabSize` 下分别覆盖选项开/关：

- 行首、缩进边界、边界前后及全空白行按 Tab；
- 正文 identifier、运算符、字符串之后及行尾按 Tab，开启选项时仍插入一个 `\t`；
- 单行/多行块缩进与反缩进；
- 行首空格的 Backspace/Delete，覆盖边界、差 1 至 `tabSize - 1` 个空格、空格不足一级及混合 Tab；
- 正文空格逐个删除；
- Undo/Redo 后文本、caret 和选区正确。

### 5.2 P2 显示与编辑矩阵

至少使用 `indentSize=4、tabSize=8`、`indentSize=8、tabSize=4` 和二者均为 4 的对照组：

- 纯 Tab、纯空格、空格与 Tab 混合的行首缩进；
- 首个非空白字符前后的 Tab；
- 非等宽字体、CJK、组合附加符号和 emoji 前后的正文 Tab；
- 空行、全空白行、修改任一设置后的缓存失效；
- 软换行恰好发生在正文 Tab 前时，该 Tab 仍使用 `tabSize`；
- caret、鼠标命中、列编辑和拖选与 painter 使用一致位置；
- 两个视图使用不同宽度时互不污染布局。

### 5.3 默认兼容性

- 新默认 `indentSize=4、tabSize=4` 下，行首和正文 Tab 显示保持旧默认结果；
- 旧 `tab_width` 不丢失，缺失的 `indent_size` 回退 4；
- 旧 `tab_to_spaces` 能迁移为新选项；
- 明确验证并记录唯一有意行为变化：开启选项后，正文 Tab 不再转换为空格。

## 6. 非目标

- 不引入通用逻辑列，也不让编辑行为依赖正文字体像素宽度；
- 不根据文件内容自动检测或覆盖缩进风格；
- 不引入项目级、语言级或 `.editorconfig` 设置；
- 不统一重写已有文件中的 Tab 和空格；
- 不在本方案中扩展 exporter、QConsole 或独立代码格式化器配置。
