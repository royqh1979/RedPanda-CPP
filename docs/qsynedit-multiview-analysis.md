# QSynEdit 多视图共享 Document：分析（讨论稿）

本文是关于"多视图共享一个 `Document`"的分析稿，供研究讨论用；**不是现状文档**
（现状与约定见 `qsynedit-design.md`）。结论与建议均未实施。

三个问题：

1. `Document` 到底拆不拆？
2. Undo/Redo 在视图间共享还是各自独立？
3. 变更通知（刷新显示、代码折叠等）怎么组织？

## 1. 出发点：拆分的真正判据是"共享性"

单视图时代，`Document` 聚了 6 项职责（设计文档 2.1），拆不拆主要是代码审美问题。
多视图目标改变了一切：**每个数据项都必须回答"它是文档的还是视图的"**，
答错的项不是难看，而是**错误**。按这个判据审计现状：

### 1.1 在 Document 里、确实应该共享的

| 数据 | 判定 |
|---|---|
| 文本内容、行序号 | 共享 ✓ |
| 语法分析结果（每行 `PSyntaxState`） | 共享 ✓（解析结果是文本的纯函数，视图无关） |
| 文件 I/O、编码、换行风格 | 共享 ✓ |
| 互斥锁 | 共享 ✓ |

### 1.2 在 Document 里、但属于视图的（多视图下是错误）

| 数据 | 问题 |
|---|---|
| `QFont`（构造函数依赖，`document.h:294`） | 字体是视图属性：两个视图可用不同字体/缩放 |
| `GlyphCalculator`、字形像素位置、行宽、`mIndexOfLongestLine`、`maxLineWidthChanged` 信号 | 全部依赖字体度量 → 依赖视图。放文档里，多视图下哪个视图的宽度是真的？ |
| （字形分段 `mGlyphStartCharList` 与字体无关，可留） | — |

**结论：职责 3（绘制缓存）中字体相关的部分必须移出。** 这正是设计文档 2.3
两步走的第一步，多视图目标把它从"可选优化"变成"前置条件"。

### 1.3 在 QSynEdit（视图）里、但可能属于文档的

| 数据 | 现状 | 问题 |
|---|---|---|
| `mUndoList`/`mRedoList`（`qsynedit.h:751-752`） | 每视图持有 | 见第 2 节，焦点问题 |
| `mModified`（`qsynedit.h:758`） | 由本视图 `UndoList::initialState()` 驱动（`qsynedit.cpp:3330`） | 跟随 undo 归属 |
| `mSyntaxer` 实例与解析驱动（`qsynedit.h:738`，同步解析，无后台线程） | 每视图各持一个解析器，结果写进共享 `Document` | 两视图重复解析同一文本、竞争写同一语法状态槽（`mMutex` 只保证不崩，不保证不浪费） |

### 1.4 在 QSynEdit 里、确实属于视图的

caret、selection、滚动位置、gutter 配置与宽度、折叠状态（`mCodeBlocks` 及各块的
collapsed 标志，`qsynedit.h:712`）、字体。这些留在视图 ✓。

## 2. Undo/Redo：共享还是独立

### 2.1 主流编辑器调查

| 编辑器/库 | 共享机制 | Undo 归属 |
|---|---|---|
| Qt `QTextDocument` | `QPlainTextEdit::setDocument`/`QTextEdit::setDocument` 共享文档 | **文档**：`QTextDocument::undo()/redo()` 是文档级 API |
| Scintilla | 文档引用计数，多窗口共享（[SCI_SETDOCPOINTER](https://scintilla.org/ScintillaDoc.html#MultipleViews)） | **文档**：undo buffer 是文档属性，save point 即 undo 栈中的位置（`SCI_EMPTYUNDOBUFFER` 作用于文档）；Notepad++ 的克隆视图因此共享 undo 历史 |
| Kate / KTextEditor | Document/View 分离 | **文档**：`KTextEditor::Document::undo()` |
| VS Code | 多个 editor 挂同一 TextModel | **model**：undo 栈绑在 model 上，分栏编辑共享 undo 历史；光标恢复是 editor 侧的事 |
| Emacs | 多 window 显示同一 buffer | **buffer**：`buffer-undo-list` 是 buffer-local |
| Vim | 多 window 显示同一 buffer | **buffer**：undo tree per-buffer |

**主流完全一致：undo 历史属于共享的 buffer/document/model，没有"每视图独立 undo"的先例。**

### 2.2 为什么每视图独立 undo 在共享文档下不成立

- 视图 A 编辑后，视图 B 的 undo 栈里所有 `CharPos` 失效。两条路都难看：
  - 作废 B 的历史 → 语义怪：B 没参与编辑却丢历史，且 B 自己的编辑也一并丢了；
  - 给 B 的每条历史做位置平移（mark 调整）→ 每条编辑路径都要向所有视图广播变换，
    复杂度不低于共享 undo，收益为零。
- redo 语义更乱：B 的 redo 栈对应的是 A 早已改掉的文档状态。
- 用户心智也不支持：用户看到的是"一个文件"，不会理解"这半屏能 undo、那半屏不能"。

### 2.3 共享 undo 对 qsynedit 的具体影响

若采纳共享 undo（UndoList/RedoList 移入 Document，或移入 Document 持有的
`EditHistory` 组件）：

- **caret/selection 快照是视图无关的纯数据**（`CaretAndSelectionInfo` 全是
  `CharPos` 缓冲区坐标），由发起视图在 `beginBlock/endBlock` 时传入，
  Document 只存数据。**栈里不记录"是哪个视图"**——Document 对视图的关系只停留在
  信号观察者层面，不存在文档 → 视图的反向依赖（也就无所谓视图关闭后的悬垂指针）。
- **光标/选区的恢复语义收窄为"恢复触发 undo 的视图"**：
  - 同视图 undo（常见情形）：快照本就是为它记的，完美恢复；
  - 跨视图 undo（B 撤销 A 的编辑）：B 的光标/选区从未被那次编辑改动，谈不上
    "恢复"；把快照当建议值套给 B，光标跳到变更处（VS Code 即此语义）；
  - "同时恢复所有视图各自的光标/选区"不实现：栈里本来就没有其它视图的快照，
    也没有主流编辑器这么做；其它视图只做按变更的位置平移/钳制；
  - 边界情况：快照来源视图已关闭——快照按建议值套给触发视图，或丢弃只钳制，
    均可。
- **GroupUndo 合并**：先沿用现有合并规则（位置连续 + 同类型），不区分来源视图
  （VS Code 在同一 model 上跨编辑器也照样合并输入，语义无问题）。若以后要
  "切换编辑视图即断组"，编辑块带一个**不透明来源 token**（只做相等比较、
  从不解引用），Document 依然不认识视图。
- **`addedUndo` 信号**变为文档级，所有视图同步 undo/redo 菜单使能与 modified 状态。
- **modified / save point** 随 undo 栈一起变为文档级（本来也该是文档级：
  文件只有一个，"未保存修改"是文件的事）。
- 搬迁风险：B 类 redo 重跑命令依赖的 `mUndoing` 抑制层（设计文档 1.6 第 3 条）、
  `shouldInsertAfterCurrentLine` 等对解析状态的依赖，都要随栈一起理清。

### 2.4 备选方案：每视图 undo + 位置调整

理论可行（CRDT 式位置变换），但如上所述复杂度更高且无先例。**不推荐。**

## 3. 变更通知：多视图下信号机制的再评估

### 3.1 单视图结论在多视图下不成立

设计文档 6.1 的结论：修改后处理（undo 记录、语法级联、刷新、折叠维护）不走信号，
走四个 chokepoint 直接调用——因为单视图下信号的唯一听众就是所有者自己，
信号和直接调用等价，直接调用可读性更强。

**多视图改变了听众数量**：编辑发起视图做完自己的收尾后，**其它视图也需要知道
文档变了**。发起者无法直接调用其它视图的内部逻辑（那是它们的私有状态），
于是"文档 → 所有视图"的变更通知信号从风格选择变成**必需品**——观察者模式在
N 个听众时才是正当的。

### 3.2 通知粒度的两个方案

**方案 A：细粒度即时信号。** Document 在 `insertLine`/`deleteLines`/`moveLine`/`putLine`
内直接发 `linesInserted/linesDeleted/lineMoved/linesChanged`——发射点与现 chokepoint
里的 emit 一一对应，语义不变；视图把折叠平移、滚动条、重绘搬进对应槽。
改动小、风险低，但中间态发射照旧（列编辑逐行 `putLine` → N 次），
槽的正确性全靠挂接准则的纪律。

**方案 B：更新边界聚合变更集。** Document 在 `begin/endUpdate` 之间累积变更区间
（ChangeSet），最外层 `endUpdate` 一次性发 `contentChanged(changeSet)`，视图在槽里
按序应用。消灭中间态发射，大文件加载天然批处理；代价是引入 ChangeSet 结构，
且要保证多行编辑路径都有 Document 级批次包裹。

### 3.3 旧 bug 解剖：onLinesChanged 列选区重吸附为什么崩

（见设计文档 6.1 的记录。）崩溃不是"信号坏"，而是三个必要条件叠加：

1. **契约错位**：`Document::changed()` 的语义是"某个文档批次结束了"
   （每条原语自包 `begin/endUpdate`，列删除 = N 个批次），而槽作者假设的是
   "一次编辑操作完成了"。信号承诺的比槽需要的弱。
2. **跨组件一致性恢复**：槽里做的"选区按光标显示 x 重新吸附"要同时读 caret、
   读文档行、写选区——三个组件的状态必须同时一致才合法。这是"修复不变式"，
   不是"维护派生数据"。
3. **中间态发射**：发射点落在编辑中段，caret 还是旧值、行已删短——
   输入不一致，修复逻辑自然算出非法坐标。

### 3.4 两类编辑后处理：能不能进槽的判据

| 类型 | 特征 | 例子 | 能否进通知槽 |
|---|---|---|---|
| **事实驱动的派生维护** | 只凭"哪几行变了"这个事实就能做；幂等；与调用时机无关 | 折叠表平移、宽度缓存失效、重绘请求、滚动条重算、非发起视图的 caret 按行差平移 | 能 |
| **跨组件一致性恢复** | 需要 caret/选区/文本/折叠**同时**处于终态才能做 | 选区按光标吸附、光标钳制到行尾、显示 x 对齐 | 永远不能 |

设计文档 6.1 挂接准则的本质就是这个二分。多视图下它还是**发起视图私事**的判据：
一致性恢复只对发起视图有意义（选区是每视图独立的），非发起视图的所有维护
都是事实驱动的。要避免旧 bug 复发，应把准则从纪律升级为两条可执行规则：

- 事实 → 派生维护：可进槽；
- 一致性恢复：禁止进槽——留在编辑操作收尾；确需挂接时另设 QSynEdit 级
  "编辑结束"信号，不复用内容通知。

### 3.5 为什么单靠 B 不够：B 完全体

**文档批次边界 ≠ 编辑操作边界。** Document 不知道编辑命令何时把 caret/选区设完；
若批次仍由原语自包，`endUpdate` 发射时 caret 依旧是旧值——中间态窗口从 N 次
缩成 1 次，但窗口还在。B 要真正消除窗口，批次必须由编辑边界驱动：

1. **`beginEditing/endEditing` 同时包裹 `Document::beginUpdate/endUpdate`**。
   `mEditingCount` 本就是计数嵌套（B 类 redo 重跑命令里的再编辑自然嵌套）；
   于是文档批次边界 == 编辑操作边界。未走 `beginEditing` 的 Document 直改路径
   （`loadFromFile` 等）有自己的批次，退化为粗粒度发射——**正确性不受影响，
   只是次数多**（优雅降级）。
2. **发射点排在静默点**：`endEditing` 末尾的现有顺序为 命令体（编辑 + caret 终态）
   → `endBlock` 拍"变更后"快照 → `rescanCodeBlocks()`（折叠全量重扫，本就推迟到
   编辑边界）→ 文档 `endUpdate` → 发 `contentChanged(changeSet)`。发射时文档静默、
   发起视图 caret 已是终态。注意 `rescanCodeBlocks` 说明"边界收尾"钩子在架构里
   已存在——B 完全体是把文档通知也挂到同一边界，不是新造概念。
3. **信号契约只携带事实**：变更区间列表，不含也不应让槽需要 caret。
4. **结构防线**：槽内可断言"文档不在更新中"（`mUpdateCount==0`）——B 下恒真，
   A 下做不到（A 在 `putLine` 内部发射，计数 ≥1）。**这是 A/B 的本质差别：
   A 只能靠纪律，B 能断言。**
5. **一致性恢复留在编辑操作收尾**（现状修复方向不变）。

**旧 bug 在 B 完全体下不可能复发（双保险）**：结构上，发射只在静默点，
"读到编辑中途的 caret"这个前提不存在；语义上，重吸附仍是发起视图私事，
规则禁止进槽。即使有人再犯，第 4 条的断言在 debug 下当场爆炸，而不是偶发崩溃。

### 3.6 建议的分层

| 层 | 机制 | 说明 |
|---|---|---|
| 文档内部级联（undo 记录、语法状态失效、宽度缓存失效） | chokepoint 直接调用 | 不变，仍在四个 `proper*` 函数内 |
| 文档 → N 个视图 | `contentChanged(changeSet)` 信号（B 完全体） | 批次由编辑边界驱动，编辑操作一次发射 |
| 视图内部（重绘、gutter 宽度、折叠映射平移、滚动修正、本视图光标平移） | 视图自己的槽函数 | 只做事实驱动的派生维护（3.4），各视图独立处理 |

### 3.7 代码折叠

- qsynedit 现状：折叠状态（`mCodeBlocks` + collapsed 标志）在视图侧 ✓，方向正确。
- Scintilla 先例（[ScintillaDoc#Folding](https://scintilla.org/ScintillaDoc.html)）：
  **fold 层级（fold levels）存在文档里共享，行可见性是视图属性**——
  "Line visibility is a property of the view rather than the document so each view
  may be displaying a different set of lines."层次与可见性各归其位。
- 对 qsynedit 的建议：fold range 的**扫描结果**可由语法状态推导（共享），
  **collapsed 集合**每视图独立（现状已如此）；行增删时各视图挂文档的行级通知，
  各自平移自己的折叠映射。`foldsRescaned` 等测试观察口保持视图本地。

### 3.8 刷新

每视图独立 invalidation：文档只发事实（哪几行变了），不碰任何视图行为；
各视图把行区间经自己的折叠映射换算成显示行后自行重绘。这也自动解决
"不同视图不同字体/缩放"的重绘差异。

## 4. 结论：拆不拆

拆的判据 = 共享性。多视图目标下答案明确：

**必须移出 Document（字体相关度量）**：`QFont` 依赖、`GlyphCalculator`、
字形像素位置、行宽、最长行跟踪 → 每视图，或每（视图 × 文档）一个 metrics cache。
这是设计文档 2.3 的第一步，也是多视图的前置条件。

**建议移入 Document（或 Document 持有的组件）**：

- UndoList/RedoList + modified/save point（第 2 节：主流一致、per-view 不成立）；
- 语法器实例与解析驱动：解析结果是共享的，解析器也应文档级单实例
  （消灭重复解析与写竞争）。注意 `shouldInsertAfterCurrentLine` 等编辑决策
  对语法状态的依赖路径不变。

**留在 Document**：文本、行序号、语法状态、I/O、锁。

**留在 QSynEdit（视图）**：caret/selection/滚动/gutter/折叠 collapsed 状态/字体。

**新增**：文档 → 视图的行级变更通知信号（第 3.2 节）。

### 主要风险

1. 锁粒度：拆分后文本锁、语法状态锁、各视图度量缓存的关系需重新设计
   （设计文档 2.2）；
2. 失效链跨对象化：现在三层失效在同一对象内就地完成，拆开后是跨对象通知；
3. undo 搬迁牵扯 `mUndoing` 抑制层（十余处 `if (!mUndoing)`），是全身性改动；
4. 行级通知信号的中间态问题：逐行 `putLine` 的列编辑会发 N 次通知，
   视图侧槽函数必须遵守 6.1 幂等准则——或者通知以块为边界（`beginUpdate/endUpdate`
   配对包住，块结束时一次性发完整变更集）。

## 5. 决策点清单（待拍板）

1. **undo 归属**：共享文档级（推荐，主流一致）/ 每视图（不推荐）；
2. **折叠状态**：每视图独立（推荐，现状+Scintilla 先例）/ 共享；
3. **字体与度量缓存**：每视图（必选，无争议）；
4. **解析器**：文档级单实例（推荐）/ 每视图重复解析（不推荐）；
5. **行级通知粒度**：逐行发（方案 A，靠纪律）/ 编辑边界聚合发（方案 B 完全体，可断言，见 3.5；倾向后者）；
6. **modified/save point**：随 undo 栈移到文档级（若 1 选共享则自然成立）。
