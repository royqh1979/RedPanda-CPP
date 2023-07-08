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
#include "document.h"
#include "keystrokes.h"
#include "searcher/baseseacher.h"
#include "formatter/formatter.h"

namespace QSynedit {

enum class ScrollStyle {
    ssNone, ssHorizontal, ssVertical, ssBoth
};

enum class EditCaretType {
    ctVerticalLine=0, ctHorizontalLine=1, ctHalfBlock=2, ctBlock=3
};

enum StatusChange {
    scNone = 0,
    scAll = 0x0001,
    scCaretX = 0x0002,
    scCaretY = 0x0004,
    scLeftChar = 0x0008,
    scTopLine = 0x0010,
    scInsertMode = 0x0020,
    scModifyChanged = 0x0040,
    scSelection = 0x0080,
    scReadOnly = 0x0100,
    scOpenFile = 0x0200,
    scModified = 0x0400
};

Q_DECLARE_FLAGS(StatusChanges, StatusChange)
Q_DECLARE_OPERATORS_FOR_FLAGS(StatusChanges)

enum class StateFlag  {
    sfCaretChanged = 0x0001,
    sfScrollbarChanged = 0x0002,
    sfLinesChanging = 0x0004,
    sfIgnoreNextChar = 0x0008,
    sfCaretVisible = 0x0010,
    sfDblClicked = 0x0020,
    sfWaitForDragging = 0x0040
};

Q_DECLARE_FLAGS(StateFlags,StateFlag)

Q_DECLARE_OPERATORS_FOR_FLAGS(StateFlags)

enum EditorOption {
  eoAltSetsColumnMode =     0x00000001, //Holding down the Alt Key will put the selection mode into columnar format
  eoAutoIndent =            0x00000002, //Will auto calculate the indent when input
  eoLigatureSupport =       0x00000004, //Support ligaures in fonts like fira code
  eoDragDropEditing =       0x00000008, //Allows you to select a block of text and drag it within the document to another location
  eoDropFiles =             0x00000010, //Allows the editor accept OLE file drops
  eoEnhanceHomeKey =        0x00000020, //enhances home key positioning, similar to visual studio
  eoEnhanceEndKey =         0x00000040, //enhances End key positioning, similar to JDeveloper
  eoGroupUndo =             0x00000080, //When undoing/redoing actions, handle all continous changes of the same kind in one call instead undoing/redoing each command separately
  eoHalfPageScroll =        0x00000100, //When scrolling with page-up and page-down commands, only scroll a half page at a time
  eoHideShowScrollbars =    0x00000200, //if enabled, then the scrollbars will only show when necessary.  If you have ScrollPastEOL, then it the horizontal bar will always be there (it uses MaxLength instead)
  eoKeepCaretX =            0x00000400 , //When moving through lines w/o Cursor Past EOL, keeps the X position of the cursor
  eoRightMouseMovesCursor=  0x00000800, //When clicking with the right mouse for a popup menu, move the cursor to that location
  eoScrollByOneLess =       0x00001000, //Forces scrolling to be one less
  eoScrollPastEof =         0x00002000, //Allows the cursor to go past the end of file marker
  eoScrollPastEol =         0x00004000, //Allows the cursor to go past the last character into the white space at the end of a line
//  eoShowSpecialChars =      0x00008000, //Shows the special Characters
//  eoSpecialLineDefaultFg = 0x00010000, //disables the foreground text color override when using the OnSpecialLineColor event
  eoTabIndent =             0x00020000, //When active <Tab> and <Shift><Tab> act as block indent, unindent when text is selected
  eoTabsToSpaces =          0x00040000, //Converts a tab character to a specified number of space characters
  eoShowRainbowColor    =   0x00080000,
  eoSelectWordByDblClick=   0x00100000,
  eoShowLeadingSpaces =   0x00200000,
  eoShowTrailingSpaces   =   0x00400000,
  eoShowInnerSpaces=   0x00800000,
  eoShowLineBreaks      =   0x01000000,
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



enum class TransientType {
    ttBefore, ttAfter
};

/*
using SynPaintTransientProc = std::function<void(const QPaintDevice& paintDevice,
        SynTransientType transientType)>;
        */
using ProcessCommandProc = std::function<void(EditCommand& command, QChar& AChar, void* data)>;
using MouseCursorProc = std::function<void(const BufferCoord& aLineCharPos, QCursor &  aCursor)>;
using PaintProc = std::function<void(const QPaintDevice& paintDevice )>;
using SearchMathedProc = std::function<SearchAction(const QString& sSearch,
    const QString& sReplace, int Line, int ch, int wordLen)>;
using SearchConfirmAroundProc = std::function<bool ()>;

class QSynEdit;
using PSynEdit = std::shared_ptr<QSynEdit>;

class QSynEdit : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit QSynEdit(QWidget* parent=nullptr);
    QSynEdit(const QSynEdit&)=delete;
    QSynEdit& operator=(const QSynEdit&)=delete;
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
    DisplayCoord pixelsToNearestRowColumn(int aX, int aY) const;
    DisplayCoord pixelsToRowColumn(int aX, int aY) const;
    QPoint rowColumnToPixels(const DisplayCoord& coord) const;
    DisplayCoord bufferToDisplayPos(const BufferCoord& p) const;
    BufferCoord displayToBufferPos(const DisplayCoord& p) const;

    //normalized buffer coord operations
//    ContentsCoord fromBufferCoord(const BufferCoord& p) const;
//    ContentsCoord createNormalizedBufferCoord(int aChar,int aLine) const;
//    QStringList getContents(const ContentsCoord& pStart,const ContentsCoord& pEnd);
//    QString getJoinedContents(const ContentsCoord& pStart,const ContentsCoord& pEnd, const QString& joinStr);

    int leftSpaces(const QString& line) const;
    QString GetLeftSpacing(int charCount,bool wantTabs) const;
    int charToColumn(int aLine, int aChar) const;
    int charToColumn(const QString& s, int aChar) const;
    int columnToChar(int aLine, int aColumn) const;
    int stringColumns(const QString& line, int colsBefore) const;
    int getLineIndent(const QString& line) const;
    int rowToLine(int aRow) const;
    int lineToRow(int aLine) const;
    int foldRowToLine(int Row) const;
    int foldLineToRow(int Line) const;
    void setDefaultKeystrokes();
    void setExtraKeystrokes();
    void invalidateLine(int Line);
    void invalidateLines(int FirstLine, int LastLine);
    void invalidateSelection();
    void invalidateRect(const QRect& rect);
    void invalidate();
    void lockPainter();
    void unlockPainter();
    bool selAvail() const;
    bool colSelAvail() const;
    QString wordAtCursor();
    QString wordAtRowCol(const BufferCoord& XY);

    QChar charAt(const BufferCoord& pos);
    QChar nextNonSpaceChar(int line, int ch);
    QChar lastNonSpaceChar(int line, int ch);
    int charColumns(QChar ch) const;

    bool isPointInSelection(const BufferCoord& Value) const;
    BufferCoord nextWordPos();
    BufferCoord nextWordPosEx(const BufferCoord& XY);
    BufferCoord wordStart();
    BufferCoord wordStartEx(const BufferCoord& XY);
    BufferCoord wordEnd();
    BufferCoord wordEndEx(const BufferCoord& XY);
    BufferCoord prevWordPos();
    BufferCoord prevWordPosEx(const BufferCoord& XY);

    void processCommand(EditCommand Command, QChar AChar = QChar(), void * pData = nullptr);
    //Caret
    void showCaret();
    void hideCaret();
    void setCaretX(int value);
    void setCaretY(int value);
    void setCaretXY(const BufferCoord& value);
    void setCaretXYEx(bool CallEnsureCursorPosVisible, BufferCoord value);
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
      bool& tokenFinished, PTokenAttribute& attri);
    bool getTokenAttriAtRowColEx(const BufferCoord& pos, QString& token,
      int &start, PTokenAttribute& attri);

    void addGroupBreak();
    void beginEditing();
    void endEditing();
    void addCaretToUndo();
    void addLeftTopToUndo();
    void addSelectionToUndo();
    void replaceAll(const QString& text) {
        mUndoList->addChange(ChangeReason::Selection,mBlockBegin,mBlockEnd,QStringList(), activeSelectionMode());
        selectAll();
        setSelText(text);
    }
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

    void setRainbowAttrs(const PTokenAttribute &attr0,
                         const PTokenAttribute &attr1,
                         const PTokenAttribute &attr2,
                         const PTokenAttribute &attr3);

    void updateMouseCursor();

    bool isCaretVisible();

    // setter && getters
    int topLine() const;
    void setTopLine(int value);

    int linesInWindow() const;

    int leftChar() const;
    void setLeftChar(int Value);

    BufferCoord blockBegin() const;
    BufferCoord blockEnd() const;

    void clearSelection();
    void setBlockBegin(BufferCoord value);
    void setBlockEnd(BufferCoord Value);

    SelectionMode activeSelectionMode() const;
    void setActiveSelectionMode(const SelectionMode &Value);

    int charsInWindow() const;

    int charWidth() const;

    void setUndoLimit(int size);
    void setUndoMemoryUsage(int size);

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
    void setLineText(const QString s);

    const PDocument& document() const;
    bool empty();

    QString selText() const;

    QStringList getContent(BufferCoord startPos, BufferCoord endPos, SelectionMode mode) const;

    QString lineBreak() const;

    EditorOptions getOptions() const;
    void setOptions(const EditorOptions &Value);

    int tabWidth() const {    return mDocument->tabWidth(); }
    void setTabWidth(int tabWidth);

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

signals:
    void linesDeleted(int FirstLine, int Count);
    void linesInserted(int FirstLine, int Count);

    void changed();

//    void chainUndoAdded();
//    void chainRedoAdded();
//    void chainLinesChanging();
//    void chainLinesChanged();
//    void chainListCleared();

//    void chainListDeleted(int Index, int Count);
//    void chainListInserted(int Index, int Count);
//    void chainListPutted(int Index, int Count);

//    void filesDropped(int X,int Y, const QStringList& AFiles);
    void gutterClicked(Qt::MouseButton button, int x, int y, int line);
//    void imeInputed(const QString& s);

//    void contextHelp(const QString& word);

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
    virtual void onEndFirstPaintLock();
    virtual void onBeginFirstPaintLock();

protected:
    void doSelectLine();
private:
    BufferCoord ensureBufferCoordValid(const BufferCoord& coord);
    void beginEditingWithoutUndo();
    void endEditingWithoutUndo();
    void clearAreaList(EditingAreaList areaList);
    void computeCaret();
    void computeScroll(bool isDragging);

    void incPaintLock();
    void decPaintLock();
    int clientWidth();
    int clientHeight();
    int clientTop();
    int clientLeft();
    QRect clientRect();
    void synFontChanged();

    void doSetSelText(const QString& value);

    void updateLastCaretX();
    void ensureCursorPosVisible();
    void ensureCursorPosVisibleEx(bool ForceToMiddle);
    void scrollWindow(int dx,int dy);
    void setInternalDisplayXY(const DisplayCoord& aPos);
    void internalSetCaretXY(const BufferCoord& Value);
    void internalSetCaretX(int Value);
    void internalSetCaretY(int Value);
    void setStatusChanged(StatusChanges changes);
    void doOnStatusChange(StatusChanges changes);
    void updateScrollbars();
    void updateCaret();
    void recalcCharExtent();
    QString expandAtWideGlyphs(const QString& S);
    void updateModifiedStatus();
    void scanFrom(int index);
    void reparseLine(int line);
    void reparseDocument();
    void uncollapse(PCodeFoldingRange FoldRange);
    void collapse(PCodeFoldingRange FoldRange);

    void foldOnListInserted(int Line, int Count);
    void foldOnListDeleted(int Line, int Count);
    void foldOnListCleared();
    void rescanFolds(); // rescan for folds
    void rescanForFoldRanges();
    void scanForFoldRanges(PCodeFoldingRanges topFoldRanges);
    int lineHasChar(int Line, int startChar, QChar character, const QString& tokenAttrName);
    void findSubFoldRange(PCodeFoldingRanges topFoldRanges,PCodeFoldingRanges& parentFoldRanges, PCodeFoldingRange Parent);
    PCodeFoldingRange collapsedFoldStartAtLine(int Line);
    void initializeCaret();
    PCodeFoldingRange foldStartAtLine(int Line) const;
    bool foldCollapsedBetween(int startLine, int endLine) const;
    QString substringByColumns(const QString& s, int startColumn, int& colLen);
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
    void moveCaretHorz(int DX, bool isSelection);
    void moveCaretVert(int DY, bool isSelection);
    void moveCaretAndSelection(const BufferCoord& ptBefore, const BufferCoord& ptAfter,
                               bool isSelection);
    void moveCaretToLineStart(bool isSelection);
    void moveCaretToLineEnd(bool isSelection);
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
    int doInsertTextByColumnMode(const QStringList& text, int startLine, int endLine);
    int doInsertTextByLineMode(const BufferCoord& pos, const QStringList& text, BufferCoord &newPos);

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
    void doMouseScroll(bool isDragging);

    QString getDisplayStringAtLine(int line) const;


private slots:
    void onBookMarkOptionsChanged();
    void onGutterChanged();
    void onLinesChanged();
    void onLinesChanging();
    void onLinesCleared();
    void onLinesDeleted(int index, int count);
    void onLinesInserted(int index, int count);
    void onLinesPutted(int index, int count);
    //void onRedoAdded();
    void onScrollTimeout();
    void onDraggingScrollTimeout();
    void onUndoAdded();
    void onSizeOrFontChanged(bool bFont);
    void onChanged();
    void onScrolled(int value);

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
    int mCharsInWindow;
    int mCharWidth;
    QFont mFontDummy;
    QFont mFontForNonAscii;
    bool mMouseMoved;
    QPoint mMouseOrigin;

    bool mInserting;
    bool mPainting;
    PDocument mDocument;
    int mLinesInWindow;
    int mLeftChar;
    int mPaintLock; // lock counter for internal calculations
    bool mReadOnly;
    int mRightEdge;
    QColor mRightEdgeColor;
    ScrollStyle mScrollBars;
    int mTextHeight;
    int mTopLine;
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
    QRect mInvalidateRect;
    StateFlags mStateFlags;
    EditorOptions mOptions;
    StatusChanges  mStatusChanges;
    int mLastKey;
    Qt::KeyboardModifiers mLastKeyModifiers;
    //fSearchEngine: TSynEditSearchCustom;
    //fHookedCommandHandlers: TList;
    //fKbdHandler: TSynEditKbdHandler;
    //  fFocusList: TList;
    //  fPlugins: TList;
    QTimer*  mScrollTimer;
    int mScrollDeltaX;
    int mScrollDeltaY;

    PSynEdit  fChainedEditor;

    int mPaintTransientLock;
    bool mIsScrolling;
    int mPainterLock; // lock counter to prevent repaint while painting
    bool mUndoing;
    // event handlers
    ProcessCommandProc mOnCommandProcessed;
    MouseCursorProc mOnMouseCursor;
    PaintProc mOnPaint;
//    SynPreparePaintHighlightTokenProc mOnPaintHighlightToken;
    ProcessCommandProc mOnProcessingCommand;
    ProcessCommandProc mOnProcessingUserCommand;

//    SynSpecialLineColorsProc mOnSpecialLineColors;
//    SynEditingAreasProc mOnEditingAreas;
//    SynGutterGetTextProc  mOnGutterGetText;
//    SynTGutterPaintProc mOnGutterPaint;
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

    // QWidget interface
    const QFont &fontForNonAscii() const;
    void setFontForNonAscii(const QFont &newFontForNonAscii);

    int mouseSelectionScrollSpeed() const;
    void setMouseSelectionScrollSpeed(int newMouseSelectionScrollSpeed);

    ScrollStyle scrollBars() const;
    void setScrollBars(ScrollStyle newScrollBars);

    double lineSpacingFactor() const;
    void setLineSpacingFactor(double newLineSpacingFactor);

    const QDateTime &lastModifyTime() const;

    const PFormatter &formatter() const;
    void setFormatter(const PFormatter &newFormatter);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
};

}
#endif // QSYNEDIT_H
