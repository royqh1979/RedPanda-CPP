# QSynEdit 现有缩进与制表符设计

> 状态：现行设计。仔细比较后，现有设计已经够用，暂时没有修改的必要。`qsynedit-indent-tab-size-plan.md` 与 `qsynedit-indent-always-use-spaces-plan.md` 两个修改方案仅作为设计留痕备用，均不实施。

## 1. 设置与职责

现有设计只有一个 `tabSize`，默认值为 4，以及一个 `TabsToSpaces` 选项，默认关闭。

`tabSize` 同时用于：

- Tab 的像素显示宽度；
- 行首自动缩进步长；
- 块缩进与块反缩进步长；
- 开启 `TabsToSpaces` 时的 Tab 输入转换；
- 开启 `TabsToSpaces` 时空格的成组删除边界；
- 缩进参考线的层级距离。

`GlyphCalculator` 和 `tabSize` 属于视图，Document 不保存字体或宽度设置。修改选项或宽度不会自动重写已有文件内容，只影响后续编辑和显示。

## 2. Tab 显示

所有 Tab 使用同一套像素制表位规则，不区分行首和正文：

```text
tabWidth = tabSize * spaceWidth()
glyphWidth = tabWidth - currentPixelX % tabWidth
```

因此 Tab 从当前实际像素位置前进到下一个制表位，宽度始终为正。正文已有字符会影响 Tab 的实际像素宽度；非等宽字体下不需要用固定数量空格模拟该行为。

painter、caret、鼠标命中和布局计算复用 glyph 位置。这里的像素位置只服务显示和命中，不作为编辑时的逻辑列。

## 3. Tab 输入

关闭 `TabsToSpaces` 时，按 Tab 插入一个 `\t`。

开启时，按 Tab 插入空格并前进到下一个 `tabSize` 边界；已经在边界时插入完整一级。该行为同时适用于行首和正文，是当前选项的全局转换语义。

若满足块缩进条件，Tab 优先执行块缩进，而不是在 caret 处插入空白。

## 4. 缩进编辑

自动缩进只扫描参考行的行首空白前缀：

- 空格增加一个缩进单位；
- Tab 前进到下一个 `tabSize` 边界；
- 首个非空白字符结束扫描。

生成缩进文本时：

- 开启 `TabsToSpaces`：全部生成空格；
- 关闭：每个完整 `tabSize` 生成一个 Tab，余数生成空格。

块缩进增加一级：开启选项时插入 `tabSize` 个空格，关闭时插入一个 Tab。块反缩进删除行首一个 Tab，或最多 `tabSize` 个连续空格。

缩进参考线按 `tabSize` 分层。现有 `getLineIndent()` 是行首空白计量，不是正文显示逻辑列。

## 5. Backspace 与 Delete

只有开启 `TabsToSpaces` 且删除对象是空格时，才检查是否位于 `tabSize` 边界：

- Backspace 在边界上向前成组删除连续空格，最多一级；
- Delete 在边界上向后成组删除连续空格，最多一级；
- 非边界或实际 Tab 按普通单 glyph 删除。

该规则目前同样适用于行首和正文，与全局 `TabsToSpaces` 语义一致。

## 6. 逻辑列的有限用途

现有 `calcLogicalColumn()` 只用于上述 Tab 输入及空格成组删除，不参与 painter 的像素布局。它可以处理正文中的宽字符，但得数只表示编辑用的制表边界，不能用来推导 glyph 像素位置。

行首自动缩进使用独立空白前缀计算，不依赖正文逻辑列。

## 7. 保留现状的结论

当前默认 `tabSize = 4`，已经同时满足默认缩进和 Tab 显示需求。现有全局 `TabsToSpaces` 行为定义明确，并已有覆盖测试；目前没有足够强的实际需求证明需要拆分两种宽度或收窄选项语义。

两个备选方案分别记录了“行首/正文双宽度”和“只让缩进始终使用空格”的修改思路，但都会引入额外的行为迁移、布局状态和回归成本，因此暂不作为待实施计划。后续只有出现明确需求时，才重新评估并选择其中一个方案。
