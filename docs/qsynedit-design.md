# QSynEdit 设计文档

本文档记录 QSynEdit（`libs/qsynedit/`）当前实现的设计与关键决策依据，内容以代码为准，随实现演进而更新。

## 目录

- [1. 撤销/重做（Undo/Redo）](#1-撤销重做undoredo)
- [2. 文档模型](#2-文档模型)
- [3. 坐标体系](#3-坐标体系)
- 4. 语法高亮（待补）
- 5. 代码折叠（待补）

## 1. 撤销/重做（Undo/Redo）

### 1.1 数据结构

**`UndoItem`**（`document.h:701`）——一次变更的记录：

- `changeReason`（`ChangeReason`，`document.h:671`）：变更类型，决定 undo/redo 的逆操作方式；
- `changeStartPos` / `changeEndPos`（`CharPos`）：变更区间；
- `changeText`（`QStringList`）：变更相关文本（含义随 reason 而变）；
- `changeNumber`：分组编号，同一次逻辑操作（块）内的多个变更共享同一编号；
- `changeSelMode`（`SelectionMode`）：变更发生时的选择模式（普通/列）。

**`CaretAndSelectionInfo`**（`document.h:691`）——光标与选区快照，按 `changeNumber` 成对（变更前/变更后）保存。

### 1.2 UndoList 的职责

`UndoList`（`document.h:727`，实现 `document.cpp:1343-1568`）不只是栈，还承担：

- **变更编号分配**：`mNextChangeNumber` 递增分配，`beginBlock` 时取号（`document.cpp:1489`）；编号是 undo/redo 分组归并的唯一依据；
- **嵌套分组块**：`beginBlock`/`endBlock` 用 `mBlockLock` 计数支持嵌套；块开始时记录"变更前"光标快照，块结束且栈非空时记录"变更后"快照并发出 `addedUndo` 信号（`document.cpp:1435`）；
- **`addedUndo` 信号**：驱动 UI 更新"可撤销"状态；发射条件依赖块状态和 `mLastRestoredItemChangeNumber` 去重（`document.cpp:1389`）；
- **保存点追踪**：`mInitialChangeNumber`/`initialState()`/`setInitialState()`，用于"文档未修改"判定（`document.cpp:1538`）；
- **`insideRedo` 标志**：标记当前处于 redo 过程中（见 1.5）；
- **`maxUndoActions`/`mFullUndoImposible`**：声明了深度限制，但裁剪逻辑未实现（半成品，见 1.7）。

### 1.3 RedoList 的职责

`RedoList`（`document.h:791`，实现 `document.cpp:1616-1708`）是**被动**的：

- 一个变更项栈（peek/pop/clear/lastChangeReason）；
- 按 `changeNumber` 存取 caret/selection 快照（`mCaretInfoBeforeChange`/`mCaretInfoAfterChange`）。

它不分配编号、不分组、不发信号、不追踪保存点——这些都在 UndoList 侧。

### 1.4 协作流程

1. **编辑时**：`QSynEdit::beginEditing`/`endEditing`（`qsynedit.cpp:1789,1800`）包装编辑操作，内部调 `mUndoList->beginBlock/endBlock`；各编辑命令经 `addChangeToUndo`（`qsynedit.cpp:348`）记录 `UndoItem`。
2. **undo 时**（`doUndo`，`qsynedit.cpp:4218`）：先弹出连续的 `GroupBreak` 项搬入 redo 栈；然后按 `changeNumber` 归并执行（同编号视为一次逻辑操作；`GroupUndo` 选项开启时还会归并相邻同类型变更）；每个 `doUndoItem` 把 redo 所需数据 push 进 `mRedoList`；结束后把 caret/selection 快照从 undo 侧搬到 redo 侧，并按"变更前"快照恢复光标与选区。
3. **redo 时**（`doRedo`，`qsynedit.cpp:4515`）：镜像过程——`GroupBreak` 搬回 undo 栈（`restoreChange`），按编号归并执行 `doRedoItem`，快照搬回 undo 侧，按"变更后"快照恢复光标与选区。
4. **新编辑发生时**：redo 栈清空。

### 1.5 编辑操作的三种实现类型

`doUndoItem`（`qsynedit.cpp:4273`）和 `doRedoItem`（`qsynedit.cpp:4571`）各有一个按 `ChangeReason` 的 switch，每个分支手写逆逻辑。按 redo 的实现策略分三类：

**A 类：数据回放型（undo/redo 对称）**

| 操作 | undo | redo |
|---|---|---|
| `Input` | 共享 `doUndoRedoInput`（`qsynedit.cpp:1753`）：用存储的旧文本逆向替换，被替换文本存进 redo 项 | 同一 helper，方向相反 |
| `Insert` | 删除区间，删掉的文本存进 redo 项 | `doInsertText` 回放存储文本 |
| `Delete`/`DeleteChar`/`DeletePreviousChar` | `doInsertText` 回插存储文本 | `doDeleteText(start,end)` |
| `ReplaceLine` | 换回旧行文本（undo 时把当前行文本存进 redo 项） | 换回另一份 |
| `MoveLine` | from/to 行号互换回放 | 同数据正向回放 |
| `InsertLine` | 空文档首行特例，对称 | 对称 |

**B 类：命令重跑型（redo 不是回放数据，而是重跑编辑命令本身）**

- `LineBreak` → redo 重跑 `doBreakLine()`（`qsynedit.cpp:4718`）
- `MergeWithNextLine`/`MergeWithPrevLine` → redo 重跑 `doMergeWithNextLine()`/`doMergeWithPrevLine()`
- `DuplicateLine` → redo 重跑 `doDuplicateCurrentLine()`

这些 redo **完全不用存储的文本**，其正确性依赖：

1. 相同文档状态下命令执行的确定性；
2. `mUndoing`/`mUndoList->setInsideRedo` 标志对重复记录的抑制——重跑会走正常编辑路径，靠 `qsynedit.cpp:350` 等十余处 `if (!mUndoing)` 避免再次入栈；
3. undo 侧拆分行时的 `shouldInsertAfterCurrentLine`（`qsynedit.cpp:1446`）依赖**语法解析状态**（`calcSyntaxStateAtLine`）——undo 的结果受解析器状态影响。

**C 类：状态项（单项数据不自完备）**

`Caret`/`Selection`/`LeftTop`：undo 时存进 redo 的是原 item 数据而非当前状态；`doRedoItem` 的 Caret/Selection 分支未做恢复（代码中相关行被注释，`qsynedit.cpp:4597,4623`）；`LeftTop` 的 undo/redo 分支设置的是同一个值。单项层面 redo 不完备，正确性靠块级 caret/selection 快照兜底（`doUndo`/`doRedo` 末尾的 `setCaretAndSelection(endInfo)`）。

> 注：`LeftTop` 项把滚动像素偏移（`leftPos()`/`topPos()`，均为像素值）存入 `changeStartPos` 的 `ch/line` 字段（`qsynedit.cpp:4300-4313`）。这是**有意复用**——`UndoItem` 只有 `CharPos` 可承载位置信息，undo/redo 两侧约定一致、信息能完整往返；属风格问题而非误用。`CharPos` 字段含义随 `ChangeReason` 而变的多义性详见 3.3。

### 1.6 为什么不能合并 UndoList/RedoList

1. **UndoItem 不是自完备的变更记录**：B 类操作的 redo 依赖重跑命令而非数据回放，C 类依赖块级快照兜底——无法抽象成"统一变更记录 + 方向"的对称模型。
2. **职责不对称**：编号分配、分组块、`addedUndo` 信号、保存点、`insideRedo` 都在 UndoList 侧；RedoList 是被动的。合并会把这些不对称职责塞进一个类，语义反而更模糊。
3. **抑制层脆弱**：B 类 redo 重跑命令时，正确性依赖贯穿十余处编辑路径的 `mUndoing` 抑制；任何合并/重写都必须先理清这层，风险高。

可做的收敛仅限于**存储层**：两个类重复的"栈 + caret/selection info 两张映射"可提取为内部共享组件（如 `ChangeStack`），公共接口与上述语义保持不变。

### 1.7 已知疑点（记录待讨论，未改动行为）

- `UndoList::addChange` 中 `Q_ASSERT(inBlock())` 与紧随的 `if (reason != GroupBreak && !inBlock())` 分支自相矛盾（`document.cpp:1360-1369`）——release 下后者为死分支，说明逻辑经历过演化；
- `maxUndoActions`/`mFullUndoImposible`：有接口和标志，未见裁剪实现，是半成品；
- 列模式 `Delete`：undo 做 char↔xpos 换算（`qsynedit.cpp:4445-4451`），redo 直接用原区间（`qsynedit.cpp:4706`），宽字符/tab 场景下可能不对称；
- `doUndoItem` 的 `Delete*` 分支中判断了 `ChangeReason::MergeWithNextLine`（`qsynedit.cpp:4455-4456`），但该分支按 switch 不可能到达（MergeWithNextLine 有独立分支），疑为历史遗留。

## 2. 文档模型

### 2.1 Document 的职责清单（现状审计）

`Document`（`document.h:290`，实现 `document.cpp`）实际承担 **6 项职责**：

1. **文本内容**：行存储与编辑（`insertLine`/`deleteLines`/`moveLine`/`putLine`/`setText`/`setContents` 等）、空文档不变式（`ensureHasLine`，`document.cpp:1118`）、行序号（`lineSeq`/`findLineBySeq`/`mLineSeqIndice`）、批量更新（`beginUpdate`/`endUpdate`）。
2. **语法分析结果**：每行 `PSyntaxState` 的存取（`getSyntaxState`/`setSyntaxState`），以及其字段的访问器——`parenthesisLevel`/`bracketLevel`/`braceLevel`/`blockLevel`/`blockStarted`/`blockEnded` 全部直接读 `syntaxState()` 的字段（`document.cpp:52-131`）。注意语法状态不只服务高亮：`shouldInsertAfterCurrentLine`（`qsynedit.cpp:1446`）用它做编辑决策，因此它是"模型+编辑"共享的数据。
3. **绘制缓存**：字形分段（`mGlyphStartCharList`，**与字体无关**）、字形像素位置与行宽（`mGlyphStartPositionList`/`mWidth`/`mIsTempWidth`，**与字体有关**）、`GlyphCalculator`（持有 `QFontMetrics`、tabSize、forceMonospace，`document.h:205`）、最长行跟踪（`mIndexOfLongestLine` 与 `maxLineWidthChanged` 信号）。`Document` 构造函数因此依赖 `QFont`（`document.h:294`）。
4. **文件 I/O 与编码**：`loadFromFile`/`saveToFile`、UTF-16/32 BOM 处理（`loadUTF16BOMFile` 等，`document.cpp:525-567`）、编码探测（`tryLoadFileByEncoding`）、换行风格（`NewlineType`/`lineBreak`）、`appendNewLineAtEOF`。
5. **线程同步**：`QRecursiveMutex mMutex` 横切保护上述全部职责（后台解析线程会读语法状态）。
6. **变更通知**：`changed`/`changing`/`maxLineWidthChanged` 信号。

撤销/重做**不在** Document 内——`UndoList`/`RedoList` 由 `QSynEdit` 持有（`qsynedit.cpp:79-82`），见第 1 章。

### 2.2 职责间的耦合点

- **失效级联**：文本修改 → 字形分段与宽度缓存失效（`invalidateWidth`）→ 最长行重估；语法状态由重解析异步更新。目前三层失效在同一对象内就地完成，拆开后需设计跨对象的失效通知链。
- **API 面**：`lineWidth`/`maxLineWidth`/`stringWidth`/`glyph*`/`fontMetrics`/`setFont` 挂在 `Document` 上，被 `QSynEdit`（约 10 处）使用，且 `QSynEditPainter` 是 `Document` 的 friend 直接访问内部（`document.h:668`、`painter.cpp:793`）。
- **锁粒度**：拆分后文本锁、语法状态锁、度量缓存锁的关系需要重新设计，否则易死锁或丢保护。

### 2.3 拆分思路（讨论稿）

自然的拆分边界：

- **TextBuffer**：纯文本行 + 行序号 + 变更通知——不知道字体、不知道语法；
- **SyntaxStateStore**：每行 `PSyntaxState` 的存取，随高亮/解析管线走；需先理清 `shouldInsertAfterCurrentLine` 等编辑决策的取值路径；
- **LineMetricsCache**：字形像素位置、行宽、最长行，移到视图侧，监听文本变更失效。

注意：`mGlyphStartCharList`（字形分段）与字体无关，可留在文本侧或独立成纯函数模块；只有"位置/宽度"必须跟字体走。

务实的走法是分两步：先移出职责 3 中字体相关的部分（位置/宽度/GlyphCalculator/最长行），字形分段留下；验证后再议语法状态与文件 I/O 的归属。每步的失效链与调用点迁移都可控。

> 历史背景：该拆分是公认的大改动，因此一直未动；本文档先固化职责清单与耦合点，作为动手前的事实基线。

### 2.4 实证案例：字体度量平台差异导致测试失败（2026-04 记录）

**现象**：`TestQSyneditEmoji::test_move_caret_y("Up in the middle of emoji")`（`test_qsynedit_emoji.cpp:235`）在 Windows 上以 `QT_QPA_PLATFORM=offscreen` 运行时失败——光标从 emoji 字形中间（`{3,1}`）按 ↑，期望吸附到字形边界 `{2,0}`，实际落在 `{4,0}`。同一二进制在 `QT_QPA_PLATFORM=windows` 下全部通过；CI（Linux headless offscreen）也通过。

**根因链**：

1. 光标上下移动保持**显示 x 像素位置**，再按目标行的字形宽度换算回字符位置——行为结果依赖字体度量；
2. 测试通过 `defaultMonoFont()`（`test_utils.cpp:66-83`）按 OS 硬编码字体名（Windows 为 Consolas），其 `exactMatch()` 结果依赖当前 QPA 平台的字体数据库；
3. 关键：Consolas **不含 emoji 字形**，emoji 由系统回退字体渲染（Windows 为 Segoe UI Emoji），`QFontMetrics` 宽度走平台回退链——不同 QPA 平台（offscreen vs windows）下度量不同；
4. 因此用例期望实质是把**某个特定环境的字体度量**硬编码成了断言，环境一变就脆断（本机 offscreen 失败、本机 windows 通过、Linux CI offscreen 通过，三者度量两两不同只是恰好后两者凑对）。

**结论与启示**：

- 这是"字形分段（字体无关）与宽度度量（字体相关）混在 Document 中"的直接代价：编辑行为测试被字体环境绑架；
- 该用例测的是"编辑逻辑 + 字体度量"混合行为，应改造为：断言与字体解耦（运行时测量期望，或只断言字形边界吸附关系）、或用捆绑字体注入（`QFontDatabase::addApplicationFont`）使度量可复现、或环境探测 + `QSKIP`；
- **运行方式备忘**：Windows 桌面环境跑测试**不要**设 `QT_QPA_PLATFORM=offscreen`（offscreen 仅用于 Linux 无头 CI）；且运行前需将 `Qt bin` 与 `mingw64/bin` 加入 `PATH`。

## 3. 坐标体系

### 3.1 坐标种类与基数约定

| 坐标 | 载体 | 基数 | 说明 |
|---|---|---|---|
| 缓冲区字符位置 | `CharPos{ch, line}`（`types.h:75`） | 均 **0 基** | 核心约定；`fileBegin()=CharPos{0,0}` |
| 文档行 | `Document` 全部行接口 | **0 基** | `QSynEdit::validLine`: `line>=0 && line<count()`（`qsynedit.cpp:515`） |
| 显示位置 | `DisplayCoord{x, row}`（`types.h:105`） | 均 **0 基**（x 像素、row 显示行） | 2026-04 起统一为全 0 基（原 row 为 1 基，见 3.6） |
| 滚动偏移 | `leftPos()`/`topPos()` | 像素 | `setTopPos` 按 `maxScrollHeight()` 钳制并驱动滚动条（`qsynedit.cpp:7141`） |
| Gutter 行号显示 | `Gutter::formatLineNumber` | 入参 1 基 | 调用方 `painter.cpp:148` 传 `line+1`，函数内再加 `lineNumberStart-1`（`gutter.cpp:115`）——换算分散在两侧，自洽但易误解 |
| 解析器行号 | `Statement->line` 等 | **0 基** | tokenizer `mCurrentLine` 从 0 起（`cpptokenizer.cpp:61`），与 `CharPos.line` 一致，跨模块传递无需换算 |

转换路径：`bufferToDisplayPos`/`displayToBufferPos`（折叠映射 + char↔像素换算）、`lineToRow`/`rowToLine`（0 基，无折叠时 `line==row`，折叠时经 `foldRowToLine`/`foldLineToRow` 映射）、`yposToRow`（像素→0 基显示行）。

### 3.2 已知不一致（确认的问题）

- `document.h:545` 注释称 `glyphStartPostion` 返回 "column (starting from 1)"，实现实为 **0 基**（`calcGlyphPositionList` 从 0 累积，`document.cpp:983`）——注释错误；
- `Document::charToGlyphIndex` 声明参数名 `charIdx`（`document.h:573`），同族接口多用 `charPos`，命名不统一；
- `RedPandaIDE/src/editor.cpp:2405` 的注释 `//BufferCoord::Char starts with 1` 是旧 SynEdit（1 基）时代残留，当前 ch 为 0 基，代码本身正确但注释误导。

> 已解决（2026-04）：`getLineIndent`/`leftSpaces` 等 QString 类型的 `line` 参数/变量已统一改名为 `lineText`（含 `getEOL` 的 `Line` 参数，共 14 处），消除了"行号/行文本"的命名歧义。

### 3.3 CharPos 的多义使用（有意复用，非误用）

`UndoItem` 只有 `CharPos` 字段承载位置信息，其含义随 `ChangeReason` 而变：

- 编辑类项（A/B 类）：缓冲区变更区间 `[changeStartPos, changeEndPos)`；
- `Caret` 项：光标位置；
- `Selection` 项：选区起止；
- `LeftTop` 项：**滚动像素偏移**（`ch`←`leftPos()`、`line`←`topPos()`，均为像素，`qsynedit.cpp:4300-4313`）。

undo/redo 两侧约定一致、信息完整往返，故属风格问题而非误用；但 C 类项的正确性另需块级快照兜底（见 1.5）。改进方向是拆分类型（如独立 `ScrollPos`）或加注释，而非改行为。

### 3.4 审计发现的问题及处理结果（2026-04 已修复）

坐标审计发现三项问题，已全部验证并修复（修复后 `test-qsynedit` 全部通过）：

- **`setLeftPos` 忽略新值（真 bug，已修复）**：无滚动条且 `maxLineWidth()>0` 时，else 分支钳制的是旧 `mLeftPos` 自身，`value` 未赋入——导致 `ScrollStyle::None` 下 `ensureCaretVisible` 的水平滚动失效。修复：`std::max(mLeftPos, 0)` 改为 `std::max(value, 0)`。
- **`xposToGlyphStartChar(line, newStr, xpos)` 忽略 `newStr`（死分支陷阱，已修复）**：else 分支原本用旧行文本计算字形位置；修复后改用 `newStr`。修复前唯一调用路径（折叠行点击定位）下结果凑巧正确（越界钳制到行尾），但参数名不副实，是未来调用者的陷阱。
- **`uncollapseAroundLines` 条件恒假（死代码，已删除）**：条件 `pFold->toLine <= endLine && endLine <= pFold->toLine` 恒等价于 `toLine == endLine`，与函数意图不符；全仓库无调用者，已从 `qsynedit.h/.cpp` 整体删除。

### 3.5 改进方向

- 修正 3.2 中的错误/过时注释；
- 中期：引入强类型坐标（`CharIndex`/`BufferLine`/`DisplayRow`/像素类型）；gutter 行号换算收敛到单侧。

### 3.6 变更记录：DisplayCoord 全 0 基化（2026-04，3.6 版本）

**背景**：3.4 及之前 `CharPos` 为 ch 0 基 / line 1 基，易出错；3.5 已将 `CharPos` 统一为全 0 基。本次将 `DisplayCoord`（仅 QSynedit 内部使用）的 `row` 从 1 基统一为 0 基，`x`（像素）维持 0 基不变。

**改动要点**：

- 5 个转换函数改 0 基语义：`yposToRow`（去 `+1`）、`rowToLine`/`lineToRow`（无折叠时 `line==row`）、`foldRowToLine`/`foldLineToRow`（平移调整）；
- 行→像素换算全部改为 `row * mTextHeight`（`displayCoordToPixels`、`invalidateLine(s)`、`invalidateGutterLines`、`processGutterClick`、`ensureCaretVisible`、painter 的 5 处）；
- 行范围钳制统一为 `[0, displayLineCount()-1]`（`moveCaretVert`、`computeScroll`、paintEvent 的 nL1/nL2）；
- **`displayLineCount()` 保持"行数"语义**：0 基化后原实现 `lineToRow(count-1)` 会变成"末行索引"，已显式 `+1` 保持值不变——这是本次重构最易踩的陷阱；
- `moveCaretVert` 的"已到首行仍按选择键上移则跳到文件头"特殊分支，判定由 `row==1` 改为 `row==0`（漏改会导致跳到文件尾，行为测试当场抓获）；
- `displayToBufferPos` 边界顺手统一为 `< count`。

**验证**：先补行为测试（`TestQSyneditDisplay`，13 用例，只断言重构前后都成立的往返/行差/端到端不变量）钉住现状；重构后 `test-qsynedit` 全部 374 用例通过（Windows 原生平台），RedPandaIDE 主程序构建通过。

## 4. 语法高亮（待补）

## 5. 代码折叠（待补）
