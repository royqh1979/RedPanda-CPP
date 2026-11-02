# QSynEdit 设计文档

本文档记录 QSynEdit（`libs/qsynedit/`）当前实现的设计与关键决策依据，内容以代码为准，随实现演进而更新。只写现状与约定；具体修改历史见 git log，不记入本文档。

## 目录

- [1. 撤销/重做（Undo/Redo）](#1-撤销重做undoredo)
- [2. 文档模型](#2-文档模型)
  - [2.5 多视图支持现状（进行中）](#25-多视图支持现状进行中)
- [3. 坐标体系](#3-坐标体系)
- 4. 语法高亮（待补）
- 5. 代码折叠（待补）
- [6. 信号机制](#6-信号机制)
- [7. 文件编码与 BOM](#7-文件编码与-bom)
- [8. 选区模型](#8-选区模型)

## 1. 撤销/重做（Undo/Redo）

> 归属：`UndoList`/`RedoList` 实例由 **Document** 持有（多视图共享同一份历史；
> 各视图经 `QSynEdit::mUndoList`/`mRedoList` 别名访问）。新编辑使 redo 栈失效的清理
> 由 Document 自己完成（`addedUndo` 时按 `insideRedo`/栈顶类型判定）；undo/redo 操作
> 完成后由触发视图调 `Document::notifyUndoRedoStateChanged()`，所有视图据此同步
> modified。块级 caret/选区快照是视图无关的纯数据，恢复时套用到触发 undo/redo 的
> 视图（详见 2.5 与 `qsynedit-multiview-analysis.md` 2.3）。

### 1.1 数据结构

**`UndoItem`**（`document.h:700`）——一次变更的记录，**immutable**（构造后只读）：

- `changeReason`（`ChangeReason`，`document.h:669`）：变更类型，决定 undo/redo 的逆操作方式；
- `changeStartPos` / `changeEndPos`（`CharPos`）：变更区间；
- `changeText`（`QStringList`）：变更相关文本（含义随 reason 而变）；
- `changeNumber`：分组编号，同一次逻辑操作（块）内的多个变更共享同一编号；
- `changeSelMode`（`SelectionMode`）：变更发生时的选择模式（普通/列）。

**派生类扩展**：需要额外字段的变更类型通过派生携带（基类只有虚析构，item 经
`shared_ptr` 在 undo/redo 栈间原样往返），避免所有 item 分摊内存。现有：

- **`ColumnDeleteUndoItem`**（`ChangeReason::ColumnDelete`）：列删除的撤销记录。
  列删除的带边缘在**每行独立吸附**字形边界，这些左缘位置在删除后无法从带位置
  重新推导（非等宽字形下信息已丢失），因此在删除时把每行左缘字符索引
  （`perLineLeftChars`，与 `changeText` 平行）记入 item；undo 按记录逐行精确回插，
  redo 仍在恢复后的文档上按带重放（该路径本就精确）。

**`CaretAndSelectionInfo`**（`document.h:690`）——光标与选区快照，按 `changeNumber` 成对（变更前/变更后）保存。

### 1.2 UndoList 的职责

`UndoList`（`document.h:755`，实现 `document.cpp:1325-1565`）不只是栈，还承担：

- **变更编号分配**：`mNextChangeNumber` 递增分配，`beginBlock` 时取号（`document.cpp:1488`）；编号是 undo/redo 分组归并的唯一依据；
- **嵌套分组块**：`beginBlock`/`endBlock` 用 `mBlockLock` 计数支持嵌套；块开始时记录"变更前"光标快照，块结束且栈非空时记录"变更后"快照并发出 `addedUndo` 信号（`document.cpp:1449`）；
- **`addedUndo` 信号**：驱动 UI 更新"可撤销"状态；发射条件依赖块状态和 `mLastRestoredItemChangeNumber` 去重（`document.cpp:1380-1390`）；
- **保存点追踪**：`mInitialChangeNumber`/`initialState()`/`setInitialState()`，用于"文档未修改"判定（`document.cpp:1544`）；
- **`insideRedo` 标志**：标记当前处于 redo 过程中（见 1.5）；
- **`maxUndoActions`/`mFullUndoImposible`**：声明了深度限制，但裁剪逻辑未实现（半成品，见 1.7）。

### 1.3 RedoList 的职责

`RedoList`（`document.h:822`，实现 `document.cpp:1630-1722`）是**被动**的：

- 一个变更项栈（peek/pop/clear/lastChangeReason）；
- 按 `changeNumber` 存取 caret/selection 快照（`mCaretInfoBeforeChange`/`mCaretInfoAfterChange`）。

它不分配编号、不分组、不发信号、不追踪保存点——这些都在 UndoList 侧。

### 1.4 协作流程

1. **编辑时**：`QSynEdit::beginEditing`/`endEditing`（`qsynedit.cpp:1751,1763`）包装编辑操作，内部调 `mUndoList->beginBlock/endBlock`；各编辑命令经 `addChangeToUndo`（`qsynedit.cpp:326`）记录 `UndoItem`。
2. **undo 时**（`doUndo`，`qsynedit.cpp:4163`）：先弹出连续的 `GroupBreak` 项搬入 redo 栈；然后按 `changeNumber` 归并执行（同编号视为一次逻辑操作；`GroupUndo` 选项开启时还会归并相邻同类型变更）；每个 `doUndoItem` 把 redo 所需数据 push 进 `mRedoList`；结束后把 caret/selection 快照从 undo 侧搬到 redo 侧，并按"变更前"快照恢复光标与选区。
3. **redo 时**（`doRedo`，`qsynedit.cpp:4483`）：镜像过程——`GroupBreak` 搬回 undo 栈（`restoreChange`），按编号归并执行 `doRedoItem`，快照搬回 undo 侧，按"变更后"快照恢复光标与选区。
4. **新编辑发生时**：redo 栈清空。

### 1.5 编辑操作的三种实现类型

`doUndoItem`（`qsynedit.cpp:4225`）和 `doRedoItem`（`qsynedit.cpp:4545`）各有一个按 `ChangeReason` 的 switch，每个分支手写逆逻辑。按 redo 的实现策略分三类：

**A 类：数据回放型（undo/redo 对称）**

| 操作 | undo | redo |
|---|---|---|
| `Input` | 共享 `doUndoRedoInput`（`qsynedit.cpp:1721`）：用存储的旧文本逆向替换，被替换文本存进 redo 项 | 同一 helper，方向相反 |
| `Insert` | 删除区间，删掉的文本存进 redo 项 | `doInsertText` 回放存储文本 |
| `Delete`/`DeleteChar`/`DeletePreviousChar` | `doInsertText` 回插存储文本 | `doDeleteText(start,end)` |
| `ReplaceLine` | 换回旧行文本（undo 时把当前行文本存进 redo 项） | 换回另一份 |
| `MoveLine` | from/to 行号互换回放 | 同数据正向回放 |
| `InsertLine` | 空文档首行特例，对称 | 对称 |

**A 类变体：`ColumnDelete`（undo 数据回放 + redo 命令重放）**

- undo：按 `ColumnDeleteUndoItem::perLineLeftChars` 逐行精确回插删除文本
  （`qsynedit.cpp:4414-4435`），item 经 `addRedo(PUndoItem)` **原样**入 redo 栈；
- redo：重放 `doDeleteText` 按列带重新删除（`qsynedit.cpp:4686-4692`，恢复后的文档上
  该路径本就精确），item 经 `restoreChange(PUndoItem)` 原样搬回 undo 栈。

item 在两栈间原样往返（不经字段拷贝），`perLineLeftChars` 因此对整个 undo/redo
往返周期可用（见 1.1）。

**B 类：命令重跑型（redo 不是回放数据，而是重跑编辑命令本身）**

- `LineBreak` → redo 重跑 `doBreakLine()`（`qsynedit.cpp:4699`）
- `MergeWithNextLine`/`MergeWithPrevLine` → redo 重跑 `doMergeWithNextLine()`/`doMergeWithPrevLine()`
- `DuplicateLine` → redo 重跑 `doDuplicateCurrentLine()`

这些 redo **完全不用存储的文本**，其正确性依赖：

1. 相同文档状态下命令执行的确定性；
2. `mUndoing`/`mUndoList->setInsideRedo` 标志对重复记录的抑制——重跑会走正常编辑路径，靠 `qsynedit.cpp:324` 等十余处 `if (!mUndoing)` 避免再次入栈；
3. undo 侧拆分行时的 `shouldInsertAfterCurrentLine`（`qsynedit.cpp:1411`）依赖**语法解析状态**（`calcSyntaxStateAtLine`）——undo 的结果受解析器状态影响。

**C 类：状态项（单项数据不自完备）**

`Caret`/`Selection`/`LeftTop`：undo 时存进 redo 的是原 item 数据而非当前状态；`doRedoItem` 的 Caret/Selection 分支未做恢复（代码中相关行被注释，`qsynedit.cpp:4571,4597`）；`LeftTop` 的 undo/redo 分支设置的是同一个值。单项层面 redo 不完备，正确性靠块级 caret/selection 快照兜底（`doUndo`/`doRedo` 末尾的块级恢复）。

> 注：`LeftTop` 项把滚动像素偏移（`leftPos()`/`topPos()`，均为像素值）存入 `changeStartPos` 的 `ch/line` 字段（`qsynedit.cpp:4252-4266`）。这是**有意复用**——`UndoItem` 只有 `CharPos` 可承载位置信息，undo/redo 两侧约定一致、信息能完整往返；属风格问题而非误用。`CharPos` 字段含义随 `ChangeReason` 而变的多义性详见 3.4。

### 1.6 为什么不能合并 UndoList/RedoList

1. **UndoItem 不是自完备的变更记录**：B 类操作的 redo 依赖重跑命令而非数据回放，C 类依赖块级快照兜底——无法抽象成"统一变更记录 + 方向"的对称模型。
2. **职责不对称**：编号分配、分组块、`addedUndo` 信号、保存点、`insideRedo` 都在 UndoList 侧；RedoList 是被动的。合并会把这些不对称职责塞进一个类，语义反而更模糊。
3. **抑制层脆弱**：B 类 redo 重跑命令时，正确性依赖贯穿十余处编辑路径的 `mUndoing` 抑制；任何合并/重写都必须先理清这层，风险高。

可做的收敛仅限于**存储层**：两个类重复的"栈 + caret/selection info 两张映射"可提取为内部共享组件（如 `ChangeStack`），公共接口与上述语义保持不变。

### 1.7 已知疑点（记录待讨论，未改动行为）

- `UndoList::addChange` 中 `Q_ASSERT(inBlock())` 与紧随的 `if (reason != GroupBreak && !inBlock())` 分支自相矛盾（`document.cpp:1342-1350`）——release 下后者为死分支，说明逻辑经历过演化；
- `maxUndoActions`/`mFullUndoImposible`：有接口和标志，未见裁剪实现，是半成品；
- `doUndoItem` 的 `Delete*` 分支中判断了 `ChangeReason::MergeWithNextLine`（`qsynedit.cpp:4400-4401`），但该分支按 switch 不可能到达（MergeWithNextLine 有独立分支），疑为历史遗留。

## 2. 文档模型

### 2.1 Document 的职责清单（现状审计）

`Document`（`document.h:290`，实现 `document.cpp`）实际承担 **6 项职责**：

1. **文本内容**：行存储与编辑（`insertLine`/`deleteLines`/`moveLine`/`putLine`/`setText`/`setContents` 等）、空文档不变式（`ensureHasLine`，`document.cpp:1100`）、行序号（`lineSeq`/`findLineBySeq`/`mLineSeqIndice`）、批量更新（`beginUpdate`/`endUpdate`）。
2. **语法分析结果**：每行 `PSyntaxState` 的存取（`getSyntaxState`/`setSyntaxState`），以及其字段的访问器——`parenthesisLevel`/`bracketLevel`/`braceLevel`/`blockLevel`/`blockStarted`/`blockEnded` 全部直接读 `syntaxState()` 的字段（`document.cpp:52-131`）。注意语法状态不只服务高亮：`shouldInsertAfterCurrentLine`（`qsynedit.cpp:1411`）用它做编辑决策，因此它是"模型+编辑"共享的数据。
3. **绘制缓存**：字形分段（`mGlyphStartCharList`，**与字体无关**）、字形像素位置与行宽（`mGlyphStartPositionList`/`mWidth`/`mIsTempWidth`，**与字体有关**）、`GlyphCalculator`（持有 `QFontMetrics`、tabSize、forceMonospace，`document.h:205`）、最长行跟踪（`mIndexOfLongestLine` 与 `maxLineWidthChanged` 信号）。`Document` 构造函数因此依赖 `QFont`（`document.h:294`）。
4. **文件 I/O 与编码**：`loadFromFile`/`saveToFile`、UTF-16/32 BOM 处理（`loadUTF16BOMFile` 等，`document.cpp:525-567`）、编码探测（`tryLoadFileByEncoding`）、换行风格（`NewlineType`/`lineBreak`）、`appendNewLineAtEOF`。
5. **线程同步**：`QRecursiveMutex mMutex` 横切保护上述全部职责（后台解析线程会读语法状态）。
6. **变更通知**：`changed`/`changing`/`maxLineWidthChanged` 信号。

撤销/重做的栈归 Document 持有（多视图共享，见 2.5）；视图经 `QSynEdit::mUndoList`/`mRedoList`
别名访问同一份实例，见第 1 章。

### 2.2 职责间的耦合点

- **失效级联**：文本修改 → 字形分段与宽度缓存失效（`invalidateWidth`）→ 最长行重估；语法状态由重解析异步更新。目前三层失效在同一对象内就地完成，拆开后需设计跨对象的失效通知链。
- **API 面**：`lineWidth`/`maxLineWidth`/`stringWidth`/`glyph*`/`fontMetrics`/`setFont` 挂在 `Document` 上，被 `QSynEdit`（约 10 处）使用，且 `QSynEditPainter` 是 `Document` 的 friend 直接访问内部（`document.h:666`、`painter.cpp:793`）。
- **锁粒度**：拆分后文本锁、语法状态锁、度量缓存锁的关系需要重新设计，否则易死锁或丢保护。

### 2.3 拆分思路（讨论稿）

自然的拆分边界：

- **TextBuffer**：纯文本行 + 行序号 + 变更通知——不知道字体、不知道语法；
- **SyntaxStateStore**：每行 `PSyntaxState` 的存取，随高亮/解析管线走；需先理清 `shouldInsertAfterCurrentLine` 等编辑决策的取值路径；
- **LineMetricsCache**：字形像素位置、行宽、最长行，移到视图侧，监听文本变更失效。

注意：`mGlyphStartCharList`（字形分段）与字体无关，可留在文本侧或独立成纯函数模块；只有"位置/宽度"必须跟字体走。

务实的走法是分两步：先移出职责 3 中字体相关的部分（位置/宽度/GlyphCalculator/最长行），字形分段留下；验证后再议语法状态与文件 I/O 的归属。每步的失效链与调用点迁移都可控。

> 历史背景：该拆分是公认的大改动，因此一直未动；本文档先固化职责清单与耦合点，作为动手前的事实基线。

### 2.4 注意事项：测试与字体度量的耦合

- 光标上下移动保持**显示 x 像素位置**，再按目标行的字形宽度换算回字符位置——编辑行为的结果依赖字体度量；
- 为此测试**自带两个机器生成的确定性字体**（`libs/qsynedit/test/fonts/`，由 `gen_test_font.py` 用 fonttools 生成，CMake 复制到测试工作目录）：
  - `qsynedit-test-mono.ttf`：等宽，ASCII 半角 512 单位、CJK/emoji 全角 1024 单位（恒 2×半角）、ZWJ 序列零宽；
  - `qsynedit-test-prop.ttf`：非等宽，宽度按码位确定性生成，用于覆盖 `forceMonospace=false` 的渲染路径；
  - `installTestFonts()`（`test_utils.cpp:75-85`）懒安装注入；`defaultMonoFont()`（`test_utils.cpp:87-91`）返回 `"QSynedit Test Mono"`，`testProportionalFont()`（`test_utils.cpp:93-97`）返回 `"QSynedit Test Prop"`；`TestQSyneditInfra::test_fonts` 验证字体真实安装，字体缺失时尽早失败；
- 即使自带字体，跨平台字体引擎取整仍可能有 ±1 像素差异，宽度断言应尽量运行时现算期望值，不写死像素；
- **运行方式**：Windows 桌面环境跑测试**不要**设 `QT_QPA_PLATFORM=offscreen`（offscreen 仅用于 Linux 无头 CI）；运行前需将 Qt bin 与 mingw64/bin 加入 `PATH`。

自带字体解除了对系统字体环境（字体缺失、回退字体度量随 QPA 平台变化）的依赖；
但"字形分段（字体无关）与宽度度量（字体相关）混在 Document 中"（见 2.1 职责 3）
意味着编辑行为测试仍与字体度量耦合，只是度量现已可复现。

### 2.5 多视图支持现状（进行中）

目标：多个 QSynEdit 视图共享一个 `Document`（分析见 `qsynedit-multiview-analysis.md`）。
分支 `qsynedit-multiview` 上的进展：

- **`QSynEdit::setDocument(PDocument)`**：视图可挂接到（共享的）文档；挂接时复位
  视图态（caret/选区到文档头、滚动归零、折叠重建、modified 从共享 undo 状态同步），
  undo/redo 历史随文档继承。`document()` 返回非 const `PDocument` 以便共享。
- **视图侧维护由 `Document::contentChanged` 驱动**（见第 6 章）：任何视图发起的编辑，
  所有挂接视图各自完成折叠平移/重扫、滚动条修正与重绘。语法状态共享（同一语法器
  类型下结果一致），折叠扫描只读共享状态，安全。
- **undo/redo 栈归 Document 持有（共享）**：视图经 `mUndoList`/`mRedoList` 别名访问
  同一份栈；undo/redo 由触发视图发起，块级 caret/选区快照套用到触发视图（分析文档
  2.3 的语义）；`Document::undoRedoStateChanged` 让全部视图同步 modified；
  新编辑使 redo 栈失效的清理由 Document 自己完成。
- **每视图独立**：caret、选区、滚动、gutter、折叠 collapsed 状态、readOnly。
- **已知限制（后续阶段解除）**：
  - 非发起视图的 caret 目前**不随编辑平移**（只在越界时由其它路径钳制）；
    marker 式平移待做；
  - Document 持有字体与行宽度量缓存——共享文档的视图必须用同一字体；
  - 语法器实例每视图一个，多视图下重复解析同一文本（结果一致但浪费）。
- 单元测试：`test_shared_document_basics`（含跨视图 undo/redo 两轮往返）/
  `test_shared_document_fold_shift`。

## 3. 坐标体系

### 3.1 坐标种类与基数约定

| 坐标 | 载体 | 基数 | 说明 |
|---|---|---|---|
| 缓冲区字符位置 | `CharPos{ch, line}`（`types.h:75`） | 均 **0 基** | 核心约定；`fileBegin()=CharPos{0,0}` |
| 文档行 | `Document` 全部行接口 | **0 基** | `QSynEdit::validLine`: `line>=0 && line<count()`（`qsynedit.cpp:489`） |
| 显示位置 | `DisplayCoord{x, row}`（`types.h:105`） | 均 **0 基**（x 像素、row 显示行） | `displayLineCount()` 返回**行数**而非末行索引，转换边界处注意 |
| 滚动偏移 | `leftPos()`/`topPos()` | 像素 | `setTopPos` 按 `maxScrollHeight()` 钳制并驱动滚动条（`qsynedit.cpp:7164`） |
| Gutter 行号显示 | `Gutter::formatLineNumber` | 入参 1 基 | 调用方 `painter.cpp:148` 传 `line+1`，函数内再加 `lineNumberStart-1`（`gutter.cpp:113`）——换算分散在两侧，自洽但易误解 |
| 解析器行号 | token 的 `line` 等 | **0 基** | `Syntaxer::setLine` 直接接收 0 基缓冲区行号（`syntaxer/cpp.cpp:1734`；调用点如 `qsynedit.cpp:5032`），与 `CharPos.line` 一致，跨模块传递无需换算 |
| 导出回调 | `Exporter::FormatTokenHandler(line, ch)` | 均 **0 基** | line 为缓冲区行号，ch 为 token 首字符索引（`exporter/exporter.h:41`） |

转换路径：`bufferToDisplayPos`/`displayToBufferPos`（折叠映射 + char↔像素换算）、`lineToRow`/`rowToLine`（0 基，无折叠时 `line==row`，折叠时经 `foldRowToLine`/`foldLineToRow` 映射）、`yposToRow`（像素→0 基显示行）。

### 3.2 命名约定

- `CharPos` 变量/参数命名为 `pos`/`xxxPos`（`startPos`/`endPos`/`blockBegin`/`blockEnd` 等）；`coord` 一词专指 `DisplayCoord`；
- caret 访问器与 `CharPos{ch,line}` 对齐：`caretCh()`/`caretLine()`/`caretPos()`、`setCaretCh()`/`setCaretLine()`/`setCaretPos()`/`setCaretPosCentered()`，成员 `mCaretCh`/`mCaretLine`；`displayX()`/`displayY()`/`displayXY()` 返回 `DisplayCoord`（成员本就是 `x`/`row`），用 X/Y 命名；
- `mLastCaretX` 存的是 `displayX()`（glyph 显示列），不是缓冲区 ch；
- `getWordAtPosition`（RedPandaIDE 侧）的 `p.ch` 是 caret 缝隙索引，语义契约见其声明处注释（`editor.h`）；
- 禁用匈牙利前缀（`v`/`pt`/`i` 等）和 PascalCase 局部变量。

### 3.3 已知不一致（确认的问题）

- `document.h:545` 注释称 `glyphStartPostion` 返回 "column (starting from 1)"，实现实为 **0 基**（`calcGlyphPositionList` 从 0 累积，`document.cpp:965-983`）——注释错误；`document.h:581` 的 `@param column (starting from 1)` 是同一类错误；
- `charToGlyphIndex` 的文档注释（`document.h:570`）仍写参数名 `charIdx`，声明实参已统一为 `charPos`（`document.h:573`）——注释滞后；
- `RedPandaIDE/src/editor.cpp:2405` 的注释 `//BufferCoord::Char starts with 1` 是旧 SynEdit（1 基）时代残留，当前 ch 为 0 基，代码本身正确但注释误导。

### 3.4 CharPos 的多义使用（有意复用，非误用）

`UndoItem` 只有 `CharPos` 字段承载位置信息，其含义随 `ChangeReason` 而变：

- 编辑类项（A/B 类）：缓冲区变更区间 `[changeStartPos, changeEndPos)`；
- `Caret` 项：光标位置；
- `Selection` 项：选区起止；
- `LeftTop` 项：**滚动像素偏移**（`ch`←`leftPos()`、`line`←`topPos()`，均为像素，`qsynedit.cpp:4252-4266`）。

undo/redo 两侧约定一致、信息完整往返，故属风格问题而非误用；但 C 类项的正确性另需块级快照兜底（见 1.5）。改进方向是拆分类型（如独立 `ScrollPos`）或加注释，而非改行为。

### 3.5 改进方向

- 修正 3.3 中的错误/过时注释；
- 中期：引入强类型坐标（`CharIndex`/`BufferLine`/`DisplayRow`/像素类型）；gutter 行号换算收敛到单侧。

## 4. 语法高亮（待补）

## 5. 代码折叠（待补）

## 6. 信号机制

### 6.1 设计约定

修改内容后的处理分两层：

- **文档内部级联**（undo 记录、语法状态级联重算）由四个 chokepoint 直接调用完成：
  `properSetLine` / `properInsertLine(s)` / `properDeleteLines` / `properMoveLine`；
- **视图侧维护**（折叠映射平移、滚动条修正、重绘、对外结构信号转发）由
  `Document::contentChanged(DocumentChangeSet)` 信号驱动，QSynEdit 在槽
  `onDocumentContentChanged`（`qsynedit.cpp`）中完成。多视图下每个视图各自挂接、
  各自维护（见 `qsynedit-multiview-analysis.md`）。

**批次与发射时机**：`QSynEdit::beginEditing/endEditing` 同时包裹
`Document::beginUpdate/endUpdate`（两者都计数嵌套），文档批次边界 == 编辑操作边界。
`endEditing` 的顺序：`endBlock` 拍"变更后"快照 → 文档 `endUpdate`（发
`contentChanged`；槽内先按 changeSet 做折叠增量平移，再 `rescanCodeBlocks`
全量重扫——重扫按平移后的行号回贴 collapsed 状态——然后滚动条修正与重绘）
→ `endInternalChanges`。发射时文档静默、发起视图 caret 已是终态。
未走 `beginEditing` 的 Document 直改路径（如测试直接调 document 方法）
按原语自身批次发射，粒度退化为逐原语但语义不变。

**ChangeSet 累积规则**（`Document::recordChange`，`document.cpp`）：

- 原语在批次内按序记录 `(kind, line, count)`（`DocumentChangeKind`：
  `SetLine`/`Insert`/`Delete`/`Move`，Move 的 count 字段承载目标行）；
- 只允许**同类相邻**合并：Insert/SetLine 相邻（`last.line+last.count==line`）、
  Delete 同位置（`last.line==line`）；Move 不合并；**不做跨类合并或抵销**——
  序列必须忠实反映原语实际做了什么，消费者可朴素按序回放；
- DEBUG 断言：记录时按变更后状态检查范围合法；`endUpdate` 断言批次计数配对；
  槽内 `Q_ASSERT(!inUpdate())`（发射必在静默点，中间态发射在结构上不可能）；
- `putLine` 内容未变时不记录（空写不产生伪变更）；
- `Document::clear()` 语义：删除前 n-1 行并把保留的最后一行置空
  （Delete(n-1) + SetLine），而非全删后插新行——外部观察者看到
  `linesDeleted(0, n-1)`，末行上的断点等标记得以保留。

**信号挂接两条铁律**（旧"挂接准则"的可执行版）：

1. 槽里只允许**事实驱动的派生维护**：幂等、只凭"哪几行变了"就能做
   （折叠平移、滚动条、重绘、非发起视图的光标按行差平移）；
2. **跨组件一致性恢复禁止进槽**（选区按光标吸附、光标钳制等需要 caret/选区/文本
   同时处于终态的操作）——由编辑操作自己在收尾完成。

历史教训：`onLinesChanged` 曾在 `changed` 槽里对列选区做重吸附，编辑中间态下
光标仍是旧值、行可能已删短，导致坐标断言崩溃。

### 6.2 信号清单与用途

**Document → 视图（上行通知，听众 = 挂接该文档的所有视图）：**

- `contentChanged(DocumentChangeSet)`：批次内行级变更的有序、合并列表，
  最外层 `endUpdate` 发射一次。QSynEdit 槽据此做折叠平移、滚动条修正、重绘，
  并**转发**对外结构信号（见下）。
- `changing()` / `changed()`：批次边界；QSynEdit 据此做批量收尾
  （gutter autosize、topPos 钳制、`endInternalChanges` 聚合刷新）。
  注意：编辑被 `beginEditing/endEditing` 包裹后，编辑期间该信号**每次编辑一次**，
  不再是逐原语 N 次。不做跨组件一致性恢复（见 6.1 铁律 2）。
- `maxLineWidthChanged()`：行宽重算后发出，用于更新水平滚动条
  （依赖字体度量，多视图归属待度量拆分时处理）。

**UndoList / Gutter → QSynEdit（上行通知）：**

- `UndoList::addedUndo()`：QSynEdit 据此更新 modified 状态并清空 redo 栈。
- `Gutter::changed()`：Gutter 各属性 setter 经 `setChanged()` 发出，是标准的属性变更通知。

注意 gutter 宽度有**两个触发源**：Gutter 自身属性（信号路径）和编辑器字体/尺寸变化
（`onSizeOrFontChanged` 直接调用 `onGutterChanged`，因为宽度计算依赖编辑器字体度量，
Gutter 自己感知不到）。两条路径不是冗余。

**QSynEdit → 外部观察者（正当的观察者模式，保留）：**

- `linesInserted(firstLine, count)` / `linesDeleted(firstLine, count)` / `lineMoved(from, to)`：
  行级结构变化，由 `onDocumentContentChanged` 从 changeSet 逐项转发；
  相邻变更已按 6.1 的合并规则聚合（如 undo 一次删除相邻两行 = 一个
  `linesDeleted(line, 2)`）。RedPandaIDE 的 EditorManager 依赖它们平移断点行号，
  语义必须精确。
- `statusChanged(StatusChanges)`：caret/selection/modified/滚动位置等状态位的聚合通知，
  带 `mStatusChanges` 合并机制（一次编辑聚合后只发一次）。用于状态栏更新等。
- `changed()`：内容被修改过（如设置对话框标记"已编辑"）。与
  `statusChanged(StatusChange::ModifyChanged)` 语义重叠，前者是简单的"改过"标记，
  后者携带具体状态位。
- `gutterClicked(button, x, y, line)`：gutter 鼠标点击的语义化转发（用于切换断点）。

**行内容修改（不增删行）没有对外行级信号，这是有意为之**：
EditorManager 等外部观察者只需跟踪行号结构变化；行内容的变化通过重绘和语法状态体现。

**测试专用（`QSYNEDIT_TEST` 下才有）：**

- `linesReparesd(start, count)`（拼写保持现状）：`reparseLines`/`reparseDocument` 的观察口，
  用于测试断言"被修改的行有被重解析/刷新"。
- `foldsRescaned()`：折叠重扫描的观察口。

### 6.3 测试断言约定

信号测试不断言发射次数（合并、级联会导致次数随实现细节变化），而是断言**覆盖性**：
被修改的行 ⊆ 通知区间的并集；结构信号（linesInserted/Deleted/Moved）因语义精确，
可断言精确的 (line, count)——注意同一批次的相邻变更已合并（一个信号而非 N 个）。
更本质的断言是修改行及下游行的语法状态内容正确。
Document 层的 `contentChanged` 契约（批次边界、合并规则、clear 语义、空写不通知）
由 `TestDocument::test_content_changed` 锁定。

## 7. 文件编码与 BOM

### 7.1 写出侧约定

- `TextEncoder` 恒不写 BOM：Qt5 下默认带 `IgnoreHeader` 转换标志
  （state 版 `QTextCodec::fromUnicode` 默认每次调用都写 BOM，与 QString 版行为不一致，须显式抑制）；
  Qt6 下用 `Stateless` 构造，LE 变体本就不写 BOM。
- `ENCODING_UTF16` / `ENCODING_UTF32` 写出**无 BOM** 的小端文件；
  `ENCODING_UTF16_BOM` / `ENCODING_UTF32_BOM` = 显式写入 BOM 字节 + 相同内容。
  BOM 由 `Document::saveToFile` 显式写出，不依赖 codec 默认行为。
- 全 ASCII 内容保存后 `realEncoding` 报告为 `ENCODING_ASCII`。
- 换行符按 `Document::newlineType` 写出，每行（含末行）追加换行。

### 7.2 读入侧约定

- 自动检测顺序：UTF-8 BOM → UTF-32 BOM → UTF-16 BOM → UTF-8 试行 →
  失败后回退系统默认编码 → 按 locale 猜测。回退路径的结果依赖系统区域设置。
- 含 `\0` 字节的行按二进制文件拒绝（`BinaryFileError`），故无 BOM 的 UTF-16/32 无法自动检测。

### 7.3 已知疑点

- Qt6 下显式 `UTF-16` 解码无 BOM 内容的默认端序未验证（Qt5 实测按小端处理，与写出侧自洽）。

## 8. 选区模型

### 8.1 两个核心谓词：状态与内容

选区有两个语义不同的谓词，**禁止混用**：

- **选中状态 `selAvail()`**：锚点（selection begin）与活动端（selection end）是否分离，
  即 begin != end。这是纯状态事实，与选区形状、覆盖文本无关。
  列模式下零宽选区（每行选中 0 个字符）也是真实存在的选区：锚点真实存在，
  Shift+方向键必须从锚点扩展。
- **选区内容 `hasSelContent()`**：选区是否覆盖文本，是派生事实。
  Normal 模式下与 `selAvail()` 等价；Column 模式下额外要求选区显示宽度非零
  （begin/end 不同行但显示 x 相同即为零宽，无内容）。

关系：hasSelContent = selAvail ∧ 内容非空。Normal 模式两者等价（历史上因此混用），
分化是列模式引入的。

### 8.2 使用规则

- 问"**是否处于选中状态**"用 `selAvail()`：建立/保留锚点、判断鼠标是否点在选区内、
  重绘选区、菜单"有选区"使能。
- 问"**操作会不会动文本**"用 `hasSelContent()`：删除前是否需扩展零宽选区、
  复制/剪切无内容时退化为操作当前行、`selText()`/`selContent()`/`selCount()` 的返回、
  `doSetSelText` 是否先删除选区。

把 `hasSelContent()` 当状态用会出错（零宽列选区被当作"没选"，锚点丢失）；
把 `selAvail()` 当内容用基本无害（对空内容的删除是 no-op），但语义不符处仍应按规则改写。

### 8.3 推论：列光标

`selAvail() && !hasSelContent()` 在列模式下等价于"每行一个光标、每行选中 0 字符"，
即**列光标**状态。Backspace/Delete 对零宽列选区"先扩一个字符再删"
（`doDeletePrevChar`/`doDeleteCurrentChar` 列分支）就是这一语义的体现；
列模式输入（见 8.6）同样以该状态为入口条件。

**列删除/列插入后保持列模式**：`doDeleteText`/`doInsertText` 的列分支结束后
只设置光标与选区、不触碰选择模式（不能用会重置为 Normal 的 `setCaretAndSelection`），
使列删除留下零宽列选区（列光标），连续 Backspace 等操作保持多光标语义。

### 8.4 `setCaretAndSelection` 与选择模式

`setCaretAndSelection` **不改变选择模式**。历史上它曾无条件重置为 Normal——
2023-03 为修"列模式下格式化（astyle）把文档弄乱"而加（`7a8a239d`），但修在了
API 层而非格式化调用点，此后在列删除/列插入/undo-redo 恢复三处造成模式被意外冲掉。
现行约定：

- 需要 **Normal** 选区的调用方，在调用前显式 `setActiveSelectionMode(SelectionMode::Normal)`
  （IDE 侧 3 处、qsynedit 内列可达的 Normal 语义操作均如此）；
- 需要**保模式**的代码（列编辑、undo/redo 块级恢复）使用内部 setter 序列
  （`internalSetCaretPos`/`internalSetSelBegin`/`internalSetSelEnd`），不经过
  `setCaretAndSelection`（其 `ensureCharPosValid` 还会按当前模式钳制 ch，对行尾之外的
  列位置不忠实）；
- undo/redo 的块级恢复（`caretAndSelBeforeChange`/`caretAndSelAfterChange`）完整恢复
  caret、选区和**模式**：undo 回到编辑前状态，redo 回到编辑后状态。

### 8.5 列模式粘贴语义

**内容分配**：剪贴板单行 → 每行插同一字符串；剪贴板多行 → 选区第 i 行插第 i 行，
选区更高时重复剪贴板末行。目标行短于列带左缘时，先补空格对齐到列再插入。
（剪贴板更高时多余行**丢弃**——现状保留；VS/Emacs 是插入新行，是否跟进待讨论。）

**粘贴后的模式去留**：列选区由 begin/end 两点定义，"列光标"必须两端显示 x 相等，
因此"列光标位于每行粘贴内容之后"只在各行粘贴内容**显示宽度相等**时才可表示：

- 等宽（单行广播、整齐数据）：**保持列模式**，列光标落在粘贴内容之后，
  可继续多行输入（多光标编辑的核心体验）；
- 不等宽：对齐的列光标不存在，**退出列模式**，Normal 光标落在光标行粘贴内容之后。

### 8.6 列模式输入语义

列模式下输入一个字符串 = **对每行执行相同的替换**，逐行位置各自吸附：

- 有内容的列选区：删除带内容（记 `ColumnDeleteUndoItem`），再在带左缘每行插入该字符串；
- 零宽列选区（列光标）：**插入模式**在光标列每行插入该字符串；
  **覆写模式**先把选区扩展覆盖光标下的字形（等效于每行覆写），再按有内容处理
  （行尾不够覆写时自然退化为追加）；
- 输入后：每行插入的是同一字符串，显示宽度恒相等（8.5 的规则），故**保持列模式**，
  列光标落在插入文本之后，可继续输入——这是列光标的核心场景（多光标输入）。

实现与 undo：与列粘贴同路（`doSetSelText` → 删除记 `ColumnDeleteUndoItem`、
插入记逐行 `Insert` item），一次输入是一个 undo 块。
已知限制：连续输入的多个字符**不合并**（Normal 模式的单词级 GroupUndo 合并不覆盖
列输入），每个字符是独立的 undo 步骤；如需合并另行设计。
