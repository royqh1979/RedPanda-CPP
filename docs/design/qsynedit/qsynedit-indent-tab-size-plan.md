# QSynEdit 缩进宽度与制表符宽度拆分设计

> 状态：设计稿，尚未实施。目标默认值：`indentSize = 4`，`tabSize = 8`。

## 1. 目标

当前 `QSynEdit::tabSize()` 同时控制自动缩进步长、块缩进、Tab 输入、逻辑制表位和像素显示宽度，无法表达“每级缩进 4 列，但正文制表符按 8 列显示”。

拆分为两个独立的视图设置：

- `indentSize`：行首缩进区的 Tab 宽度及缩进步长，默认 4；
- `tabSize`：首个非空白字符之后的制表符宽度，默认 8。

本功能只改变编辑器内的显示和编辑语义，不改变已有文件内容，也不把设置写入 Document。无论是否开启 `TabsToSpaces`，切换该选项或修改 `indentSize`、`tabSize` 都不会自动重写文档中的 Tab 或空格；`TabsToSpaces` 只影响后续编辑操作。两个视图共享同一文档时，可以各自使用不同字体、`indentSize` 和 `tabSize`。

## 2. 精确定义

### 2.1 行首缩进区

行首缩进区是从行首开始的、仅由空格和 Tab 组成的最大前缀。首个非空白字符是缩进区边界；全空白行的行尾也是边界。

逻辑列、像素位置和编辑操作必须显式跟踪“当前 glyph 是否仍在缩进区”，不能只根据 glyph 是 Tab 就假定使用同一种宽度。

### 2.2 缩进区中的 Tab

缩进区中的每个 Tab 固定增加 `indentSize` 个逻辑列，不使用“前进到下一个 `tabSize` 制表位”的规则。

例如 `indentSize=4` 时：

- `"\t"` 的宽度为 4；
- `"\t\t"` 的宽度为 8；
- `" \t"` 的宽度为 5；
- `"\t \t"` 的宽度为 9。

显示时对应固定增加 `indentSize * spaceWidth()` 像素。等宽字体下逻辑列与视觉列一致；非等宽字体下仍分别计算，不从像素位置反推逻辑列。

### 2.3 正文中的 Tab

首个非空白字符之后的 Tab 前进到下一个 `tabSize` 制表位：

```text
count = tabSize - logicalColumn % tabSize
```

显示时前进到下一个 `tabSize * spaceWidth()` 像素制表位，且宽度必须为正。

### 2.4 编辑操作

- 不使用 `TabsToSpaces` 时，按 Tab 直接写入一个 `\t`。
- 使用 `TabsToSpaces` 时：
  - caret 位于缩进区，按 `indentSize` 补到下一个缩进边界；
  - caret 位于正文，按 `tabSize` 补到下一个正文制表位；
  - 已在边界时插入完整一级宽度，而不是 0 个空格。
- 自动缩进每级增加 `indentSize` 个逻辑列。
- 块缩进：不使用 `TabsToSpaces` 时插入一个 Tab，否则插入 `indentSize` 个空格。
- 块反缩进：删除行首一个 Tab，或最多 `indentSize` 个连续空格。
- Backspace/Delete 仅在 `TabsToSpaces` 开启时按边界成组删除：缩进区使用 `indentSize`，正文使用 `tabSize`；非边界仍删除单个 glyph。
- 缩进参考线的层级距离使用 `indentSize`。

不根据 caret 当前列把块缩进换算成若干 Tab；块缩进始终是独立的一层操作。

### 2.5 自动生成缩进文本

自动缩进先计算目标逻辑列，再生成行首缩进文本：

- 使用 `TabsToSpaces` 时全部生成空格；
- 不使用 `TabsToSpaces` 时，每个完整 `indentSize` 层级生成一个 Tab，余数生成空格。

例如 `indentSize=4`、`tabSize=8` 时，目标第 4 列生成一个 Tab，目标第 8 列生成两个 Tab，目标第 10 列生成两个 Tab 和两个空格。这里不使用 `tabSize` 决定自动缩进字符数量。

## 3. 设置与兼容性

### 3.1 QSynEdit API

新增：

```cpp
int indentSize() const;
void setIndentSize(int size);
```

`setIndentSize()` 与 `setTabSize()` 一样，需要使所有行布局、最大行宽、caret/鼠标命中相关缓存失效并触发重绘。`tabSize()` 保留原 API，但语义收窄为正文制表符宽度。

`GlyphCalculator` 同时持有两个值。它仍归视图所有，并随 `WrapLayoutJob` 的值对象快照传入后台布局计算；Document 和 DocumentLine 不新增任何字体或宽度状态。

### 3.2 IDE 设置

`EditorSettings` 新增 `indentSize`，使用配置键 `indent_size`，默认 4。现有 `tab_width` 默认改为 8。

加载旧配置时：

- 已存在的 `tab_width` 原样读取，保留用户原有正文显示宽度；
- 缺少 `indent_size` 时使用 4；
- `Editor::applySettings()` 分别调用 `setIndentSize()` 和 `setTabSize()`。

设置页提供两个独立输入项：

- `Indent Size`，默认 4；
- `Tab Width`，默认 8。

新增界面文本纳入 Qt 翻译更新。

### 3.3 导出

HTML/RTF 导出目前使用独立的 exporter 构造参数，不属于 QSynEdit 视图布局。第一阶段不改变 exporter API，也不宣称导出结果复现行首/正文两种 Tab 宽度；若后续需要所见即所得导出，再单独扩展 exporter。

## 4. 实现路径

### 4.1 GlyphCalculator 与布局

当前 `GlyphCalculator::glyphWidth()` 只接收 glyph 和像素左位置，无法知道 glyph 是否在缩进区。需要让调用方显式传入缩进区状态，或在布局扫描中传递一个小的宽度上下文；不得用默认值掩盖遗漏。

布局扫描同时维护：

- 当前是否仍处于行首缩进区；
- 当前逻辑列（仅供编辑列计算使用）；
- 当前像素 X（仅供视图显示使用）。

缩进区 Tab 使用 `indentSize`，正文 Tab 使用 `tabSize`。以下路径必须采用同一判定：

- `calcGlyphPositionList()`；
- `updateGlyphStartPositionList()`；
- `stringWidth()`；
- word-wrap 的逐 glyph 布局循环；
- caret、鼠标命中、列编辑和拖选所使用的行宽及 glyph 位置。

`calcLogicalColumn()` 改为同时接收两种宽度，并按相同缩进区规则计算。编辑代码不得读取像素位置来决定空格数量。

### 4.2 自动缩进与缩进参考线

以下位置由 `tabSize()` 改为 `indentSize()`：

- `CppFormatter` 增加一层缩进的计算；
- `QSynEdit::leftSpaces()` 对行首 Tab 的计宽；
- `QSynEdit::getLineIndent()`；
- `QSynEdit::genSpaces()` 生成缩进文本的规则；
- painter 的缩进参考线步长。

`leftSpaces()` 只用于行首缩进区，因此不引入正文制表位规则。

### 4.3 Tab 输入与删除

`doTabKey()` 根据 caret 是否在缩进区选择本级的 `stepSize`：

```text
stepSize = inIndentArea ? indentSize : tabSize
count = stepSize - logicalColumn % stepSize
```

Backspace/Delete 使用相同的区域判定，但只计算编辑逻辑列，不读取 `tabWidth()` 或 `charToGlyphLeft()`。

`doBlockIndent()` 和 `doBlockUnindent()` 按第 2.4 节改为 `indentSize`。保留现有多行、单行、选区终点位于下一行行首等处理。

## 5. 不变式

1. 同一行、同一视图设置下，painter、caret 和鼠标命中使用同一份 glyph 宽度结果。
2. 同一文档在多个视图中可以显示不同宽度，但编辑生成的文本不依赖当前视图的像素度量。
3. 行首缩进区与正文的判定只扫描一次，不因调用路径不同而产生不同边界。
4. `indentSize` 不改变正文 Tab 的宽度，`tabSize` 不改变自动缩进步长。
5. 设置变化后，旧布局缓存不得继续参与绘制或命中计算；后台旧代际批次继续按现有 generation 规则丢弃。
6. 所有宽度均大于 0；非法设置由 UI 和 setter 约束在正数范围内。

## 6. 测试计划

### 6.1 默认值与设置映射

- QSynEdit 默认 `indentSize=4`、`tabSize=8`。
- EditorSettings 新配置采用上述默认值。
- 旧配置含 `tab_width` 时保留其值，缺少 `indent_size` 时回退 4。
- `applySettings()` 后两个 QSynEdit 属性分别等于设置页值。

### 6.2 逻辑列与显示

至少覆盖 `indentSize=4`、`tabSize=8` 以及二者相等的对照组：

- 纯 Tab、纯空格、空格与 Tab 混合的行首缩进；
- 首个非空白字符前/后的 Tab；
- 全空白行和空行；
- CJK、组合附加符号、emoji 前后的 Tab；
- 等宽与非等宽字体；
- 修改任一设置后，布局、行宽、caret 和鼠标命中同步更新。

### 6.3 编辑矩阵

两种宽度不相等，并分别以二者相等作为对照：

- 缩进区和正文分别按 Tab，覆盖边界、边界前后及已有 Tab；
- `TabsToSpaces` 开/关；
- 自动缩进多级嵌套；
- 单行/多行块缩进与反缩进；
- 缩进区/正文的 Backspace/Delete；
- Undo/Redo 后文本和 caret 正确；
- word-wrap、列编辑、鼠标拖选不受错误宽度影响。

现有 Tab 测试在需要旧“两种宽度相同”语义时，必须显式同时设置 `indentSize` 和 `tabSize`，不能依靠新默认值维持旧结果。

### 6.4 回归

先运行 Release 全量 CTest，再运行 Debug 全量 CTest。重点检查 QSynEdit 的布局、编辑、字体无关逻辑列和多视图测试。

## 7. 非目标

- 不根据文件内容自动检测或覆盖用户的两种宽度；
- 不引入项目级、语言级或 `.editorconfig` 设置；
- 不统一修改已有文件中的 Tab/空格；
- 不在本阶段扩展 exporter、QConsole 或代码格式化器的独立配置。
