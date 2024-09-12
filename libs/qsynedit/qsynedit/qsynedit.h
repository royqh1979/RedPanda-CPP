/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef QSYNEDIT_H
#define QSYNEDIT_H

#include <QAbstractScrollArea>
#include <QCursor>
#include <QDateTime>
#include <QFrame>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include "gutter.h"
#include "codefolding.h"
#include "types.h"
#include "keystrokes.h"
#include "searcher/baseseacher.h"
#include "formatter/formatter.h"

namespace QSynedit {

enum class ScrollStyle {
    None, OnlyHorizontal, OnlyVertical, Both
};

enum class EditCaretType {
    VerticalLine=0, HorizontalLine=1, HalfBlock=2, Block=3
};

enum class StatusChange {
    None = 0,
    AllCleared = 0x0001,
    CaretX = 0x0002,
    CaretY = 0x0004,
    LeftPos = 0x0008,
    TopPos = 0x0010,
    InsertMode = 0x0020,
    ModifyChanged = 0x0040,
    Selection = 0x0080,
    ReadOnly = 0x0100,
    OpenFile = 0x0200,
    Modified = 0x0400
};

Q_DECLARE_FLAGS(StatusChanges, StatusChange)
Q_DECLARE_OPERATORS_FOR_FLAGS(StatusChanges)

enum class StateFlag  {
    HScrollbarChanged             = 0x0001,
    VScrollbarChanged             = 0x0002,
    DblClicked                    = 0x0004,
    WaitForDragging               = 0x0008,
    EnsureCaretVisible            = 0x0010,
    EnsureCaretVisibleForceMiddle = 0x0020,
};
Q_DECLARE_FLAGS(StateFlags,StateFlag)

Q_DECLARE_OPERATORS_FOR_FLAGS(StateFlags)

enum class EditorOption {
    AltSetsColumnMode =     0x00000001, //Holding down the Alt Key will put the selection mode into columnar format
    AutoIndent =            0x00000002, //Will auto calculate the indent when input
    LigatureSupport =       0x00000004, //Support ligaures in fonts like fira code
    DragDropEditing =       0x00000008, //Allows you to select a block of text and drag it within the document to another location
    DropFiles =             0x00000010, //Allows the editor accept OLE file drops
    EnhanceHomeKey =        0x00000020, //enhances home key positioning, similar to visual studio
    EnhanceEndKey =         0x00000040, //enhances End key positioning, similar to JDeveloper
    GroupUndo =             0x00000080, //When undoing/redoing actions, handle all continous changes of the same kind in one call instead undoing/redoing each command separately
    HalfPageScroll =        0x00000100, //When scrolling with page-up and page-down commands, only scroll a half page at a time
    AutoHideScrollbars =    0x00000200, //if enabled, then the scrollbars will only show when necessary.  If you have ScrollPastEOL, then it the horizontal bar will always be there (it uses MaxLength instead)
    KeepCaretX =            0x00000400 , //When moving through lines w/o Cursor Past EOL, keeps the X position of the cursor
    RightMouseMovesCursor=  0x00000800, //When clicking with the right mouse for a popup menu, move the cursor to that location
//    ScrollByOneLess =       0x00001000, //Forces scrolling to be one less
    ScrollPastEof =         0x00002000, //Allows the cursor to go past the end of file marker
    ScrollPastEol =         0x00004000, //Allows the cursor to go past the last character into the white space at the end of a line
    InvertMouseScroll =     0x00008000, //Shows the special Characters
//  SpecialLineDefaultFg = 0x00010000, //disables the foreground text color override when using the OnSpecialLineColor event
    TabIndent =             0x00020000, //When active <Tab> and <Shift><Tab> act as block indent, unindent when text is selected
    TabsToSpaces =          0x00040000, //Converts a tab character to a specified number of space characters
    ShowRainbowColor    =   0x00080000,
    SelectWordByDblClick =  0x00100000,
    ShowLeadingSpaces =     0x00200000,
    ShowTrailingSpaces =    0x00400000,
    ShowInnerSpaces =       0x00800000,
    ShowLineBreaks =        0x01000000,
    ForceMonospace =        0x02000000,
};

Q_DECLARE_FLAGS(EditorOptions, EditorOption)

Q_DECLARE_OPERATORS_FOR_FLAGS(EditorOptions)

enum class SearchAction {
    Replace,
    ReplaceAll,
    ReplaceAndExit,
    Skip,
    Exit
};

/*
using SynPaintTransientProc = std::function<void(const QPaintDevice& paintDevice,
        SynTransientType transientType)>;
        */
// using ProcessCommandProc = std::function<void(EditCommand& command, QChar& AChar, void* data)>;
// using MouseCursorProc = std::function<void(const BufferCoord& aLineCharPos, QCursor &  aCursor)>;
// using PaintProc = std::function<void(const QPaintDevice& paintDevice )>;
using SearchMathedProc = std::function<SearchAction(const QString& sSearch,
    const QString& sReplace, int Line, int ch, int wordLen)>;
using SearchConfirmAroundProc = std::function<bool ()>;

struct GlyphPostionsListCache {
    QString str;
    QList<int> glyphCharList;
    QList<int> glyphPositionList;
    int strWidth;
};


class QSynEdit;
using PSynEdit = std::shared_ptr<QSynEdit>;

class TokenAttribute;
using PTokenAttribute = std::shared_ptr<TokenAttribute>;
class Document;
using PDocument = std::shared_ptr<Document>;
struct SyntaxState;
class Syntaxer;
using PSyntaxer = std::shared_ptr<Syntaxer>;
class UndoList;
class RedoList;
using PUndoList = std::shared_ptr<UndoList>;
using PRedoList = std::shared_ptr<RedoList>;

class QSynEdit : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit QSynEdit(QWidget* parent=nullptr);
    QSynEdit(const QSynEdit&)=delete;
    QSynEdit& operator=(const QSynEdit&)=delete;


    int lineCount() const;
    /**
     * Returns how many rows are there in the editor
     * @return
     */
    int displayLineCount() const;

    /**
     * @brief displayX
     * @return
     */
    DisplayCoord displayXY() const;
    int displayX() const;
    int displayY() const;
    BufferCoord caretXY() const;
    int caretX() const;
    int caretY() const;

    void invalidateGutter();
    void invalidateGutterLine(int aLine);
    void invalidateGutterLines(int FirstLine, int LastLine);

    int yposToRow(int y) const {
        return std::max(1, ((y + mTopPos) / mTextHeight) + 1 );
    }

    DisplayCoord pixelsToNearestGlyphPos(int aX, int aY) const;
    DisplayCoord pixelsToGlyphPos(int aX, int aY) const;
    QPoint displayCoordToPixels(const DisplayCoord& coord) const;
    DisplayCoord bufferToDisplayPos(const BufferCoord& p) const;
    BufferCoord displayToBufferPos(const DisplayCoord& p) const;

    //normalized buffer coord operations
//    ContentsCoord fromBufferCoord(const BufferCoord& p) const;
//    ContentsCoord createNormalizedBufferCoord(int aChar,int aLine) const;
//    QStringList getContents(const ContentsCoord& pStart,const ContentsCoord& pEnd);
//    QString getJoinedContents(const ContentsCoord& pStart,const ContentsCoord& pEnd, const QString& joinStr);

    int leftSpaces(const QString& line) const;
    QString GetLeftSpacing(int charCount,bool wantTabs) const;
    int charToGlyphLeft(int line, int charPos) const;
    int charToGlyphLeft(int line, const QString& s, int charPos) const;
    //int charToColumn(const QString& s, int aChar) const;
    int xposToGlyphStartChar(int line, int xpos) const;
    int xposToGlyphStartChar(int line, const QString& s, int xpos) const;
    int xposToGlyphLeft(int line, int xpos) const;
    //int xposToGlyphRight(int line, int xpos) const;
    int stringWidth(const QString& line, int left) const;
    int getLineIndent(const QString& line) const;
    int rowToLine(int aRow) const;
    int lineToRow(int aLine) const;
    int foldRowToLine(int row) const;
    int foldLineToRow(int line) const;
    void setDefaultKeystrokes();
    void setExtraKeystrokes();
    void invalidateLine(int line);
    void invalidateLines(int firstLine, int lastLine);
    void invalidateSelection();
    void invalidateRect(const QRect& rect);
    void invalidate();
    bool selAvail() const;
    bool colSelAvail() const;
    QString wordAtCursor();
    QString wordAtRowCol(const BufferCoord& XY);

    QChar charAt(const BufferCoord& pos);
    QChar nextNonSpaceChar(int line, int ch);
    QChar lastNonSpaceChar(int line, int ch);

    bool isPointInSelection(const BufferCoord& Value) const;
    BufferCoord nextWordPos();
    BufferCoord nextWordPosEx(const BufferCoord& XY);
    BufferCoord wordStart();
    BufferCoord wordStartEx(const BufferCoord& XY);
    BufferCoord wordEnd();
    BufferCoord wordEndEx(const BufferCoord& XY);
    BufferCoord prevWordPos();
    BufferCoord prevWordPosEx(const BufferCoord& XY);

    //Caret
    void showCaret();
    void hideCaret();
    void setCaretX(int value);
    void setCaretY(int value);
    void setCaretXY(const BufferCoord& value);
    void setCaretXYCentered(const BufferCoord& value);
    void setCaretAndSelection(const BufferCoord& ptCaret,
                              const BufferCoord& ptSelBegin,
                              const BufferCoord& ptSelEnd);

    bool inputMethodOn();

    void collapseAll();
    void unCollpaseAll();
    void uncollapseAroundLine(int line);
    PCodeFoldingRange foldHidesLine(int line);
    void setSelLength(int Value);
    void setSelText(const QString& text);

    void replaceLine(int line, const QString& lineText);
    int searchReplace(const QString& sSearch, const QString& sReplace, SearchOptions options,
               PSynSearchBase searchEngine,  SearchMathedProc matchedCallback = nullptr,
                      SearchConfirmAroundProc confirmAroundCallback = nullptr);

    int maxScrollWidth() const;
    int maxScrollHeight() const;

    bool getTokenAttriAtRowCol(const BufferCoord& pos, QString& token,
      PTokenAttribute& attri);
    bool getTokenAttriAtRowCol(const BufferCoord& pos, QString& token,
      PTokenAttribute& attri, SyntaxState &syntaxState);
    bool getTokenAttriAtRowColEx(const BufferCoord& pos, QString& token,
      int &start, PTokenAttribute& attri);

    void addGroupBreak();
    void beginEditing();
    void endEditing();
    void beginSetting();
    void endSetting();
    void addCaretToUndo();
    void addLeftTopToUndo();
    void addSelectionToUndo();
    void trimTrailingSpaces() {
        processCommand(EditCommand::TrimTrailingSpaces);
    }

    //Commands
    virtual void cutToClipboard() { processCommand(EditCommand::Cut);}
    virtual void copyToClipboard() { processCommand(EditCommand::Copy);}
    virtual void pasteFromClipboard() { processCommand(EditCommand::Paste);}
    virtual void undo()  { processCommand(EditCommand::Undo);}
    virtual void redo()  { processCommand(EditCommand::Redo);}
    virtual void zoomIn()  { processCommand(EditCommand::ZoomIn);}
    virtual void zoomOut()  { processCommand(EditCommand::ZoomOut);}
    virtual void selectAll() {
        processCommand(EditCommand::SelectAll);
    }
    virtual void selectWord() {
        processCommand(EditCommand::SelWord);
    }
    virtual void tab() { processCommand(EditCommand::Tab);}
    virtual void shifttab() { processCommand(EditCommand::ShiftTab);}
    virtual void toggleComment() { processCommand(EditCommand::ToggleComment);}
    virtual void toggleBlockComment() { processCommand(EditCommand::ToggleBlockComment);}
    virtual void matchBracket() { processCommand(EditCommand::MatchBracket);}
    virtual void moveSelUp(){ processCommand(EditCommand::MoveSelUp);}
    virtual void moveSelDown(){ processCommand(EditCommand::MoveSelDown);}

    virtual BufferCoord getMatchingBracket();
    virtual BufferCoord getMatchingBracketEx(BufferCoord APoint);

    QStringList contents();
    QString text();

    bool getPositionOfMouse(BufferCoord& aPos);
    bool getLineOfMouse(int& line);
    bool pointToCharLine(const QPoint& point, BufferCoord& coord);
    bool pointToLine(const QPoint& point, int& line);
    bool isIdentChar(const QChar& ch);
    bool isIdentStartChar(const QChar& ch);

    void setRainbowAttrs(const PTokenAttribute &attr0,
                         const PTokenAttribute &attr1,
                         const PTokenAttribute &attr2,
                         const PTokenAttribute &attr3);

    void updateMouseCursor();

    bool isCaretVisible();

    // setter && getters
    int topPos() const;
    void setTopPos(int value);

    int linesInWindow() const;

    int leftPos() const;
    void setLeftPos(int value);

    BufferCoord blockBegin() const;
    BufferCoord blockEnd() const;
    int selectionBeginLine() const;
    int selectionEndLine() const;

    void clearSelection();
    void setBlockBegin(BufferCoord value);
    void setBlockEnd(BufferCoord Value);

    SelectionMode activeSelectionMode() const;
    void setActiveSelectionMode(const SelectionMode &Value);

    int charWidth() const {
        return mCharWidth;
    }

    int viewWidth() const{
        return clientWidth() - mGutterWidth - 2;
    }

    int gutterWidth() const;
    void setGutterWidth(int value);

    bool modified() const;
    void setModified(bool Value);

    PSyntaxer syntaxer() const;
    void setSyntaxer(const PSyntaxer &syntaxer);

    bool useCodeFolding() const;
    void setUseCodeFolding(bool value);

    CodeFoldingOptions & codeFolding();

    QString displayLineText();
    QString lineText() const;
    QString lineText(int line) const;
    void setLineText(const QString s);

    const PDocument& document() const;
    bool empty();

    QString selText() const;
    int selCount() const;

    QStringList getContent(BufferCoord startPos, BufferCoord endPos, SelectionMode mode) const;
    void reparseDocument();

    QString lineBreak() const;

    EditorOptions getOptions() const;
    void setOptions(const EditorOptions &Value);

    int tabSize() const;
    void setTabSize(int tabSize);
    int tabWidth() const;

    QColor caretColor() const;
    void setCaretColor(const QColor &caretColor);

    QColor activeLineColor() const;
    void setActiveLineColor(const QColor &activeLineColor);

    EditCaretType overwriteCaret() const;
    void setOverwriteCaret(const EditCaretType &overwriteCaret);

    EditCaretType insertCaret() const;
    void setInsertCaret(const EditCaretType &insertCaret);

    Gutter& gutter();

    bool readOnly() const;
    void setReadOnly(bool readOnly);

    void setInsertMode(bool value);
    bool insertMode() const;

    bool canUndo() const;
    bool canRedo() const;

    int textHeight() const;

    const QColor &selectedForeground() const;
    void setSelectedForeground(const QColor &newSelectedForeground);

    const QColor &selectedBackground() const;
    void setSelectedBackground(const QColor &newSelectedBackground);

    int rightEdge() const;
    void setRightEdge(int newRightEdge);

    const QColor &rightEdgeColor() const;
    void setRightEdgeColor(const QColor &newRightEdgeColor);

    bool caretUseTextColor() const;
    void setCaretUseTextColor(bool newCaretUseTextColor);

    const PTokenAttribute &rainbowAttr0() const;

    const PTokenAttribute &rainbowAttr1() const;

    const PTokenAttribute &rainbowAttr2() const;

    const PTokenAttribute &rainbowAttr3() const;

    int mouseWheelScrollSpeed() const;
    void setMouseWheelScrollSpeed(int newMouseWheelScrollSpeed);

    const QColor &foregroundColor() const;
    void setForegroundColor(const QColor &newForegroundColor);

    const QColor &backgroundColor() const;
    void setBackgroundColor(const QColor &newBackgroundColor);

    bool isEmpty();

    int mouseSelectionScrollSpeed() const;
    void setMouseSelectionScrollSpeed(int newMouseSelectionScrollSpeed);

    ScrollStyle scrollBars() const;
    void setScrollBars(ScrollStyle newScrollBars);

    double lineSpacingFactor() const;
    void setLineSpacingFactor(double newLineSpacingFactor);

    const QDateTime &lastModifyTime() const;

    const PFormatter &formatter() const;
    void setFormatter(const PFormatter &newFormatter);
signals:
    void linesDeleted(int FirstLine, int Count);
    void linesInserted(int FirstLine, int Count);
    void changed();
    void gutterClicked(Qt::MouseButton button, int x, int y, int line);
    void statusChanged(StatusChanges changes);
    void fontChanged();
    void tabSizeChanged();
protected:
    virtual bool onGetSpecialLineColors(int Line,
         QColor& foreground, QColor& backgroundColor) ;
    virtual void onGetEditingAreas(int Line, EditingAreaList& areaList);
    virtual void onGutterGetText(int aLine, QString& aText);
    virtual void onGutterPaint(QPainter& painter, int aLine, int X, int Y);
    virtual void onPaint(QPainter& painter);
    virtual void onPreparePaintHighlightToken(int line,
            int aChar, const QString& token, PTokenAttribute attr,
            FontStyles& style, QColor& foreground, QColor& background);
    virtual void onProcessCommand(EditCommand command, QChar car, void * pData);
    virtual void onCommandProcessed(EditCommand command, QChar car, void * pData);
    virtual void executeCommand(EditCommand command, QChar ch, void * pData);
protected:
    void replaceAll(const QString& text);
    int clientWidth() const;
    int clientHeight() const;
    int clientTop() const;
    int clientLeft() const;
    QRect clientRect() const;
    void doSelectLine();
    void incPaintLock();
    void decPaintLock();
    SyntaxState calcSyntaxStateAtLine(int line, const QString &newLineText);
    void processCommand(EditCommand Command, QChar AChar = QChar(), void * pData = nullptr);

private:
    int calcLineAlignedTopPos(int currentValue, bool passFirstLine);
    void ensureLineAlignedWithTop(void);
    BufferCoord ensureBufferCoordValid(const BufferCoord& coord);
    void beginEditingWithoutUndo();
    void endEditingWithoutUndo();
    void clearAreaList(EditingAreaList areaList);
    void computeCaret();
    void computeScroll(bool isDragging);

    void synFontChanged();

    void doSetSelText(const QString& value);

    void updateLastCaretX();
    void ensureCaretVisible();
    void ensureCaretVisibleEx(bool ForceToMiddle);
    void scrollWindow(int dx,int dy);
    void setCaretDisplayXY(const DisplayCoord& aPos, bool ensureCaretVisible = true);
    void internalSetCaretXY(BufferCoord value, bool ensureVisible = true);
    void internalSetCaretX(int value);
    void internalSetCaretY(int value);
    void setStatusChanged(StatusChanges changes);
    void doOnStatusChange(StatusChanges changes);
    void updateHScrollbar();
    void doUpdateHScrollbar();
    void updateVScrollbar();
    void doUpdateVScrollbar();
    void updateCaret();
    void recalcCharExtent();
    void updateModifiedStatus();
    int reparseLines(int startLine, int endLine, bool needRescanFolds = true,  bool toDocumentEnd = true);
    //void reparseLine(int line);
    void uncollapse(PCodeFoldingRange FoldRange);
    void collapse(PCodeFoldingRange FoldRange);

    void foldOnLinesInserted(int Line, int Count);
    void foldOnLinesDeleted(int Line, int Count);
    void foldOnListCleared();
    void rescanFolds(); // rescan for folds
    void rescanForFoldRanges();
    void scanForFoldRanges(PCodeFoldingRanges topFoldRanges);
    void findSubFoldRange(PCodeFoldingRanges topFoldRanges,PCodeFoldingRanges& parentFoldRanges, PCodeFoldingRange Parent);
    PCodeFoldingRange collapsedFoldStartAtLine(int Line);
    void initializeCaret();
    PCodeFoldingRange foldStartAtLine(int Line) const;
    bool foldCollapsedBetween(int startLine, int endLine) const;
    //QString substringByColumns(const QString& s, int startColumn, int& colLen);
    PCodeFoldingRange foldAroundLine(int line);
    PCodeFoldingRange foldAroundLineEx(int line, bool wantCollapsed, bool acceptFromLine, bool acceptToLine);
    PCodeFoldingRange checkFoldRange(PCodeFoldingRanges foldRangesToCheck,int line, bool wantCollapsed, bool AcceptFromLine, bool AcceptToLine);
    PCodeFoldingRange foldEndAtLine(int line);
    void paintCaret(QPainter& painter, const QRect rcClip);
    int textOffset() const;
    EditCommand TranslateKeyCode(int key, Qt::KeyboardModifiers modifiers);
    /**
     * Move the caret to right DX columns
     * @param DX
     * @param SelectionCommand
     */
    void moveCaretHorz(int deltaX, bool isSelection);
    void moveCaretVert(int deltaY, bool isSelection);
    void moveCaretAndSelection(const BufferCoord& ptBefore, const BufferCoord& ptAfter,
                               bool isSelection, bool ensureCaretVisible = true);
    void moveCaretToLineStart(bool isSelection);
    void moveCaretToLineEnd(bool isSelection, bool ensureCaretVisible = true);
    void doGotoBlockStart(bool isSelection);
    void doGotoBlockEnd(bool isSelection);
    void doGotoEditorStart(bool isSelection);
    void doGotoEditorEnd(bool isSelection);
    void setSelectedTextEmpty();
    void setSelTextPrimitive(const QStringList& text);
    void setSelTextPrimitiveEx(SelectionMode PasteMode,
                               const QStringList& text);
    void doLinesDeleted(int FirstLine, int Count);
    void doLinesInserted(int FirstLine, int Count);
    void properSetLine(int ALine, const QString& ALineText, bool notify = true);

    //primitive edit operations
    void doDeleteText(BufferCoord startPos, BufferCoord endPos, SelectionMode mode);
    void doInsertText(const BufferCoord& pos, const QStringList& text, SelectionMode mode, int startLine, int endLine);
    int doInsertTextByNormalMode(const BufferCoord& pos, const QStringList& text, BufferCoord &newPos);
    int doInsertTextByColumnMode(const BufferCoord& pos, const QStringList& text, int startLine, int endLine);

    void doTrimTrailingSpaces();
    void deleteFromTo(const BufferCoord& start, const BufferCoord& end);
    void setSelWord();
    void setWordBlock(BufferCoord value);

    void doExpandSelection(const BufferCoord& pos);
    void doShrinkSelection(const BufferCoord& pos);


    int calcIndentSpaces(int line, const QString& lineText, bool addIndent);

    void processGutterClick(QMouseEvent* event);

    void clearUndo();
    BufferCoord getPreviousLeftBrace(int x,int y);
    bool canDoBlockIndent();

    QRect calculateCaretRect() const;
    QRect calculateInputCaretRect() const;

    //Commands
    void doDeleteLastChar();
    void doDeleteCurrentChar();
    void doDeleteWord();
    void doDeleteToEOL();
    void doDeleteToWordStart();
    void doDeleteToWordEnd();
    void doDeleteFromBOL();
    void doDeleteLine();

    void doDuplicateLine();
    void doMoveSelUp();
    void doMoveSelDown();
    void clearAll();
    void insertLine(bool moveCaret);
    void doTabKey();
    void doShiftTabKey();
    void doBlockIndent();
    void doBlockUnindent();
    void doAddChar(const QChar& ch);
    void doCutToClipboard();
    void doCopyToClipboard();
    void internalDoCopyToClipboard(const QString& s);
    void doPasteFromClipboard();
    void doAddStr(const QString& s);
    void doUndo();
    void doUndoItem();
    void doRedo();
    void doRedoItem();
    void doZoomIn();
    void doZoomOut();
    void doSelectAll();
    void doComment();
    void doUncomment();
    void doToggleComment();
    void doToggleBlockComment();
    void doMouseScroll(bool isDragging, int scrollX, int scrollY);

    QString getDisplayStringAtLine(int line) const;


private slots:
    void onMaxLineWidthChanged();
    void updateHScrollBarLater();
    void onBookMarkOptionsChanged();
    void onGutterChanged();
    void onLinesChanged();
    void onLinesChanging();
    void onLinesCleared();
    void onLinesDeleted(int line, int count);
    void onLinesInserted(int line, int count);
    void onLinesPutted(int line);
    //void onRedoAdded();
    void onScrollTimeout();
    void onDraggingScrollTimeout();
    void onUndoAdded();
    void onSizeOrFontChanged();
    void onChanged();
    void onHScrolled(int value);
    void onVScrolled(int value);

private:
    std::shared_ptr<QImage> mContentImage;
    PCodeFoldingRanges mAllFoldRanges;
    CodeFoldingOptions mCodeFolding;
    int mEditingCount;
    bool mUseCodeFolding;
    bool  mAlwaysShowCaret;
    BufferCoord mBlockBegin;
    BufferCoord mBlockEnd;
    int mCaretX;
    int mLastCaretColumn;
    int mCaretY;
    int mCharWidth;
    QFont mFontDummy;
    bool mMouseMoved;
    QPoint mMouseOrigin;

    bool mInserting;
    PDocument mDocument;
    int mLinesInWindow;
    int mLeftPos;
    int mPaintLock; // lock counter for internal calculations
    bool mReadOnly;
    int mRightEdge;
    QColor mRightEdgeColor;
    ScrollStyle mScrollBars;
    int mTextHeight;
    int mTopPos;
    PSyntaxer mSyntaxer;
    QColor mSelectedForeground;
    QColor mSelectedBackground;
    QColor mForegroundColor;
    QColor mBackgroundColor;
    QColor mCaretColor;
    PTokenAttribute mRainbowAttr0;
    PTokenAttribute mRainbowAttr1;
    PTokenAttribute mRainbowAttr2;
    PTokenAttribute mRainbowAttr3;

    bool mCaretUseTextColor;
    QColor mActiveLineColor;
    PUndoList mUndoList;
    PRedoList mRedoList;
    QPoint mMouseDownPos;
    EditCaretType mOverwriteCaret;
    EditCaretType  mInsertCaret;
    QPoint mCaretOffset;
    EditKeyStrokes mKeyStrokes;
    bool mModified;
    QDateTime mLastModifyTime;
    double mLineSpacingFactor;
    SelectionMode mActiveSelectionMode; //mode of the active selection
    bool mWantReturns;
    bool mWantTabs;
    Gutter mGutter;
    StateFlags mStateFlags;
    EditorOptions mOptions;
    StatusChanges  mStatusChanges;
    int mLastKey;
    Qt::KeyboardModifiers mLastKeyModifiers;
    QTimer*  mScrollTimer;

    PSynEdit  fChainedEditor;

    int mPaintTransientLock;
    bool mIsScrolling;
    int mOptionLock; // lock counter to prevent recalculate glyph widths while change settings;
    bool mUndoing;
    int mGutterWidth;
    //caret blink related
    int m_blinkTimerId;
    int m_blinkStatus;

    QCursor mDefaultCursor;

    QString mInputPreeditString;

    int mMouseWheelScrollSpeed;
    int mMouseSelectionScrollSpeed;

    BufferCoord mDragCaretSave;
    BufferCoord mDragSelBeginSave;
    BufferCoord mDragSelEndSave;
    bool mDropped;
    int mWheelAccumulatedDeltaX;
    int mWheelAccumulatedDeltaY;

    PFormatter mFormatter;
    GlyphPostionsListCache mGlyphPostionCacheForInputMethod;

friend class QSynEditPainter;

// QWidget interface
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    bool event(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

// QAbstractScrollArea interface
protected:
    bool viewportEvent(QEvent * event) override;

    // QWidget interface
public:
    QVariant inputMethodQuery(Qt::InputMethodQuery property) const override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
};

}
#endif // QSYNEDIT_H
