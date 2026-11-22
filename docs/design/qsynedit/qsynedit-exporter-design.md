# HTML / RTF 导出与带格式复制设计

## 应用边界

当前有两个用户场景：

1. Editor 带格式复制：实际只生成 HTML 格式和纯文本两种剪贴板数据；RTF 不用于复制。
2. 完整导出为文件：Editor 使用 HTMLExporter 或 RTFExporter 导出整个文档并保存。

打印使用 QtSupportedHtmlExporter，不属于本轮 HTMLExporter/RTFExporter 修改范围。

## 当前导出约定

- 每次范围导出先清空上一次结果；空文档、空范围、反向范围和空指针产生空输出。
- 原始行号与选区重编号分别计算，均遵守 0/1 起始设置。
- HTML 使用 DOCTYPE、转义后的标题/字体属性和合并的 text-decoration。正文以 `white-space: pre`、普通空格和实际换行保留内容，Tab 按导出器设置展开，不使用逐空格 `&nbsp;` 或额外的 `<br>`。
- RTF 使用标准 `\uN?` Unicode 控制字和 `\uc1`，输出内容可由纯 ASCII 表示；字体表不绑定中文代码页，不写死美国英语默认语言。纯 ASCII token 直接输出，避免额外分配。
- 文档背景由显式背景设置决定，不被 Space token 背景覆盖；Space 背景只影响对应 token。
- HTML CSS 名称和 RTF 颜色表在新导出开始时重置；RTF 背景色进入统一颜色索引，不重复追加。颜色索引以 `QColor::rgb()` 为 key，不能使用表示 HSV 亮度的 `QColor::value()`。
- 文件编码失败、打开失败和写入失败均抛出 `FileError`。

## 测试分层

### QSynEdit 导出器

在现有 `test-qsynedit` 中新增 `TestHTMLExporter` 和 `TestRTFExporter`，直接使用真实 Document 和 Syntaxer，不经过编辑器或系统剪贴板。

共同覆盖：

- 整篇文档、单行范围、跨行范围、token 首尾被截断的范围；
- 空文档、空范围、反向范围、越界范围和重复使用同一导出器；
- 实际输出与内联文本的边界、CRLF/LF、Tab、Unicode 和格式保留字符；
- 文件保存与 buffer 一致，无法打开文件时抛出错误；
- 样式、颜色、标题和换行；
- 大文档输出尺寸和导出耗时。

HTML 另外覆盖完整文档与 clipboard fragment、转义、CSS 样式合并、行号原始值/重编号及 0/1 起始。RTF 另外覆盖反斜杠和花括号、颜色表、字体样式及 Unicode 表示。

### Editor 数据准备

`Editor::copyAsHTML()` 只负责取得数据并交给系统剪贴板。将数据准备提取为私有 `createCopyMimeData()`，由现有友元 `TestEditorGeneral` 直接测试：

- 有选区时同时生成正确的 `text/plain` 和 `text/html`；
- 无选区时的当前行回退仍由 `copyAsHTML()` 处理，数据准备函数只读取已有选区；
- 纯文本语法器只生成纯文本；
- HTML fragment 标记、标题、颜色和行号设置来自 Editor；
- 不依赖系统剪贴板，不在 CI 中竞争剪贴板所有权。

`exportAsHTML()`、`exportAsRTF()` 通过临时文件验证完整文档和 Editor 设置的真实调用路径。剪贴板最终调用保持很薄，不把系统剪贴板可读性当作导出正确性的判据。

## 兼容性与尺寸原则

先通过测试确认问题，再做针对性修改：

- HTML 文本、标题和属性必须正确转义；同一元素的多个 text-decoration 不得互相覆盖。
- 行号应明确区分原始文档行号与选区重编号，并遵守 0/1 起始设置。
- 空范围或无效输入不得残留上一次导出内容。
- RTF 控制字、花括号、反斜杠、标题和非 ASCII 字符必须能被常见阅读器识别；文件写入或编码失败必须报告。
- HTML 使用 CSS 保留空白时，必须同时检查实际纯文本和换行，避免 `pre` 与 `<br>` 叠加产生双倍换行。
- 尺寸优化优先减少无意义的逐字符实体、重复样式和跨次导出残留，不以牺牲显示内容或兼容性为代价。

## 测量与实施

先用原实现记录 10,000/100,000 行 C++ 文档的 HTML/RTF 字节数和 Release 导出时间，再逐项修改。正确性场景不随实现删除。每项优化后复测输出内容、文件大小和性能，最后执行完整 Release/Debug 回归。

## 逐行语义高亮

QSynEdit Exporter 与 Painter 共用逐行语义高亮流程，不再由 Exporter 对每个 identifier 单独提取表达式和调用 `findStatementOf()`：

- Exporter 负责用导出 Syntaxer 对一整行分词，组装 `PaintHighlightTokenList`，每行调用一次高亮回调，再按各 token 最终的前景色、背景色和字体样式序列化；范围裁切只影响输出文本，不改变整行表达式分析的坐标。
- Editor 的语义高亮核心同时服务绘制与导出：整行提取表达式、批量求值、statement 种类到颜色的映射只保留一条实现。
- 绘制路径可以继续叠加鼠标悬停、当前单词和匹配括号等临时状态；导出路径不得带入这些状态。
- 导出使用独立颜色方案时，高亮核心接收该方案的 Syntaxer 和 statement 颜色表，不能误用 Editor 当前颜色表。使用 Editor 配色时，导出与绘制的稳定语义颜色应一致。
- `PaintHighlightToken::ch` 始终是完整源文本行中的 0 起始字符位置。选区从 token 中间开始时，只裁切输出字符串，不移动后续 token 的坐标。
- HTML 的 CSS 样式键必须包含最终前景色、背景色和字体样式，不能只使用 Syntaxer 属性名；同一个 Identifier 属性经语义分析后会对应多种 statement 颜色。

性能对比使用真实 Editor、CppParser 和 `miniaudio.h`，分别记录 HTML 与 RTF 的冷/热 `mIdCache` 导出耗时；同时保留无 Parser 的 QSynEdit 大文档基线，用于区分通用序列化开销与语义求值收益。
