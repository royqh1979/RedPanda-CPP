#ifndef SYNEDIT_H
#define SYNEDIT_H

#include <QAbstractScrollArea>
#include <QCursor>
#include <QDateTime>
#include <QFrame>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include "MiscClasses.h"
#include "CodeFolding.h"
#include "Types.h"
#include "TextBuffer.h"
#include "KeyStrokes.h"
#include "SearchBase.h"

enum class SynFontSmoothMethod {
    None, AntiAlias, ClearType
};


enum class SynScrollHintFormat {
    shfTopLineOnly, shfTopToBottom
};

enum class SynScrollStyle {
    ssNone, ssHorizontal, ssVertical, ssBoth
};

enum class SynEditCaretType {
    ctVerticalLine=0, ctHorizontalLine=1, ctHalfBlock=2, ctBlock=3
};

enum SynStatusChange {
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

Q_DECLARE_FLAGS(SynStatusChanges, SynStatusChange)
Q_DECLARE_OPERATORS_FOR_FLAGS(SynStatusChanges)

enum class SynStateFlag  {
    sfCaretChanged = 0x0001,
    sfScrollbarChanged = 0x0002,
    sfLinesChanging = 0x0004,
    sfIgnoreNextChar = 0x0008,
    sfCaretVisible = 0x0010,
    sfDblClicked = 0x0020,
    sfWaitForDragging = 0x0040
};

Q_DECLARE_FLAGS(SynStateFlags,SynStateFlag)

Q_DECLARE_OPERATORS_FOR_FLAGS(SynStateFlags)

enum SynEditorOption {
  eoAltSetsColumnMode = 0x00000001, //Holding down the Alt Key will put the selection mode into columnar format
  eoAutoIndent =        0x00000002, //Will auto calculate the indent when input
//  eoAddIndent =         0x00000004, //Will add one tab width of indent when typing { and :, and remove the same amount when typing }
  eoDragDropEditing =   0x00000008, //Allows you to select a block of text and drag it within the document to another location
  eoDropFiles =         0x00000010, //Allows the editor accept OLE file drops
  eoEnhanceHomeKey =    0x00000020, //enhances home key positioning, similar to visual studio
  eoEnhanceEndKey =     0x00000040, //enhances End key positioning, similar to JDeveloper
  eoGroupUndo =         0x00000080, //When undoing/redoing actions, handle all continous changes of the same kind in one call instead undoing/redoing each command separately
  eoHalfPageScroll =    0x00000100, //When scrolling with page-up and page-down commands, only scroll a half page at a time
  eoHideShowScrollbars =0x00000200, //if enabled, then the scrollbars will only show when necessary.  If you have ScrollPastEOL, then it the horizontal bar will always be there (it uses MaxLength instead)
  eoKeepCaretX =        0x00000400 , //When moving through lines w/o Cursor Past EOL, keeps the X position of the cursor
  eoRightMouseMovesCursor= 0x00000800, //When clicking with the right mouse for a popup menu, move the cursor to that location
  eoScrollByOneLess =   0x00001000, //Forces scrolling to be one less
  eoScrollPastEof =     0x00002000, //Allows the cursor to go past the end of file marker
  eoScrollPastEol =     0x00004000, //Allows the cursor to go past the last character into the white space at the end of a line
  eoShowSpecialChars =  0x00008000, //Shows the special Characters
//  eoSpecialLineDefaultFg = 0x00010000, //disables the foreground text color override when using the OnSpecialLineColor event
  eoTabIndent =         0x00020000, //When active <Tab> and <Shift><Tab> act as block indent, unindent when text is selected
  eoTabsToSpaces =      0x00040000, //Converts a tab character to a specified number of space characters
  eoShowRainbowColor =  0x00080000,
  eoTrimTrailingSpaces =0x00100000, //Spaces at the end of lines will be trimmed and not saved
  eoSelectWordByDblClick=0x00200000,
  eoNoSelection =       0x00400000, //Disables selecting text
    //eoAutoSizeMaxScrollWidth = 0x00000008, //Automatically resizes the MaxScrollWidth property when inserting text
    //eoDisableScrollArrows = 0x00000010 , //Disables the scroll bar arrow buttons when you can't scroll in that direction any more
    //  eoScrollHintFollows = 0x00020000, //The scroll hint follows the mouse when scrolling vertically
    //  eoShowScrollHint = 0x00100000, //Shows a hint of the visible line numbers when scrolling vertically
    //  eoSmartTabDelete = 0x00400000, //similar to Smart Tabs, but when you delete characters
    //  eoSmartTabs = 0x00800000, //When tabbing, the cursor will go to the next non-white space character of the previous line
    // eoNoCaret =           0x00000800, //Makes it so the caret is never visible
};

Q_DECLARE_FLAGS(SynEditorOptions, SynEditorOption)

Q_DECLARE_OPERATORS_FOR_FLAGS(SynEditorOptions)

enum class SynSearchAction {
    Replace,
    ReplaceAll,
    Skip,
    Exit
};



enum class SynTransientType {
    ttBefore, ttAfter
};

enum class SynScrollBarKind {
    sbHorizontal, sbVertical
};

/*
using SynPaintTransientProc = std::function<void(const QPaintDevice& paintDevice,
        SynTransientType transientType)>;
        */
using SynPlaceMarkProc = std::function<void(PSynEditMark& Mark)>;
using SynProcessCommandProc = std::function<void(SynEditorCommand& command, QChar& AChar, void* data)>;
using SynMouseCursorProc = std::function<void(const BufferCoord& aLineCharPos, QCursor &  aCursor)>;
using SynPaintProc = std::function<void(const QPaintDevice& paintDevice )>;
//using SynPreparePaintHighlightTokenProc = std::function<void(int row,
//        int column, const QString& token, PSynHighlighterAttribute attr,
//        SynFontStyles& style, QColor& foreground, QColor& background)>;
using SynSearchMathedProc = std::function<SynSearchAction(const QString& sSearch,
    const QString& sReplace, int Line, int ch, int wordLen)>;

class SynEdit;
using PSynEdit = std::shared_ptr<SynEdit>;

class SynEdit : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit SynEdit(QWidget* parent=nullptr);
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
    ContentsCoord fromBufferCoord(const BufferCoord& p) const;
    ContentsCoord createNormalizedBufferCoord(int aChar,int aLine) const;
    QStringList getContents(const ContentsCoord& pStart,const ContentsCoord& pEnd);
    QString getJoinedContents(const ContentsCoord& pStart,const ContentsCoord& pEnd, const QString& joinStr);

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
    void invalidateLine(int Line);
    void invalidateLines(int FirstLine, int LastLine);
    void invalidateSelection();
    void invalidateRect(const QRect& rect);
    void invalidate();
    void lockPainter();
    void unlockPainter();
    bool selAvail() const;
    QString wordAtCursor();
    QString wordAtRowCol(const BufferCoord& XY);

    int charColumns(QChar ch) const;
    double dpiFactor() const;
    bool isPointInSelection(const BufferCoord& Value) const;
    BufferCoord nextWordPos();
    BufferCoord nextWordPosEx(const BufferCoord& XY);
    BufferCoord wordStart();
    BufferCoord wordStartEx(const BufferCoord& XY);
    BufferCoord wordEnd();
    BufferCoord wordEndEx(const BufferCoord& XY);
    BufferCoord prevWordPos();
    BufferCoord prevWordPosEx(const BufferCoord& XY);
    void commandProcessor(SynEditorCommand Command, QChar AChar = QChar(), void * pData = nullptr);
    //Caret
    void showCaret();
    void hideCaret();
    void setCaretX(int value);
    void setCaretY(int value);
    void setCaretXY(const BufferCoord& value);
    void setCaretXYEx(bool CallEnsureCursorPos, BufferCoord value);
    void setCaretXYCentered(bool ForceToMiddle, const BufferCoord& value);
    void setCaretAndSelection(const BufferCoord& ptCaret,
                              const BufferCoord& ptBefore,
                              const BufferCoord& ptAfter);

    void uncollapseAroundLine(int line);
    PSynEditFoldRange foldHidesLine(int line);
    void setSelLength(int Value);
    void setSelText(const QString& text);

    int searchReplace(const QString& sSearch, const QString& sReplace, SynSearchOptions options,
               PSynSearchBase searchEngine,  SynSearchMathedProc matchedCallback = nullptr);

    int maxScrollWidth() const;
    int maxScrollHeight() const;

    bool getHighlighterAttriAtRowCol(const BufferCoord& XY, QString& Token,
      PSynHighlighterAttribute& Attri);
    bool getHighlighterAttriAtRowCol(const BufferCoord& XY, QString& Token,
      bool& tokenFinished, SynHighlighterTokenType& TokenType,
      PSynHighlighterAttribute& Attri);
    bool getHighlighterAttriAtRowColEx(const BufferCoord& XY, QString& Token,
      SynHighlighterTokenType& TokenType, SynTokenKind &TokenKind, int &Start,
      PSynHighlighterAttribute& Attri);

    //Commands
    virtual void cutToClipboard() { commandProcessor(SynEditorCommand::ecCut);}
    virtual void copyToClipboard() { commandProcessor(SynEditorCommand::ecCopy);}
    virtual void pasteFromClipboard() { commandProcessor(SynEditorCommand::ecPaste);}
    virtual void undo()  { commandProcessor(SynEditorCommand::ecUndo);}
    virtual void redo()  { commandProcessor(SynEditorCommand::ecRedo);}
    virtual void zoomIn()  { commandProcessor(SynEditorCommand::ecZoomIn);}
    virtual void zoomOut()  { commandProcessor(SynEditorCommand::ecZoomOut);}
    virtual void selectAll() {  commandProcessor(SynEditorCommand::ecSelectAll);}
    virtual void tab() { commandProcessor(SynEditorCommand::ecTab);}
    virtual void shifttab() { commandProcessor(SynEditorCommand::ecShiftTab);}
    virtual void toggleComment() { commandProcessor(SynEditorCommand::ecToggleComment);}

    virtual void beginUpdate();
    virtual void endUpdate();
    virtual BufferCoord getMatchingBracket();
    virtual BufferCoord getMatchingBracketEx(BufferCoord APoint);

    QStringList contents();
    QString text();

    bool getPositionOfMouse(BufferCoord& aPos);
    bool getLineOfMouse(int& line);
    bool pointToCharLine(const QPoint& point, BufferCoord& coord);
    bool pointToLine(const QPoint& point, int& line);
    bool isIdentChar(const QChar& ch);

    void setRainbowAttrs(const PSynHighlighterAttribute &attr0,
                         const PSynHighlighterAttribute &attr1,
                         const PSynHighlighterAttribute &attr2,
                         const PSynHighlighterAttribute &attr3);

    void updateMouseCursor();
// setter && getters
    int topLine() const;
    void setTopLine(int value);

    int linesInWindow() const;

    int leftChar() const;
    void setLeftChar(int Value);

    BufferCoord blockBegin() const;
    BufferCoord blockEnd() const;
    void setBlockBegin(BufferCoord value);
    void setBlockEnd(BufferCoord Value);

    SynSelectionMode activeSelectionMode() const;
    void setActiveSelectionMode(const SynSelectionMode &Value);

    int charsInWindow() const;

    int charWidth() const;

    int gutterWidth() const;
    void setGutterWidth(int value);

    bool modified() const;
    void setModified(bool Value);

    PSynHighlighter highlighter() const;
    void setHighlighter(const PSynHighlighter &highlighter);

    bool useCodeFolding() const;
    void setUseCodeFolding(bool value);

    SynEditCodeFolding & codeFolding();

    QString lineText() const;
    void setLineText(const QString s);

    const PSynEditStringList& lines() const;
    bool empty();

    SynSelectionMode selectionMode() const;
    void setSelectionMode(SynSelectionMode value);

    QString selText();

    QString lineBreak();

    SynEditorOptions getOptions() const;
    void setOptions(const SynEditorOptions &Value);

    int tabWidth() const;
    void setTabWidth(int tabWidth);

    QColor caretColor() const;
    void setCaretColor(const QColor &caretColor);

    QColor activeLineColor() const;
    void setActiveLineColor(const QColor &activeLineColor);

    SynEditCaretType overwriteCaret() const;
    void setOverwriteCaret(const SynEditCaretType &overwriteCaret);

    SynEditCaretType insertCaret() const;
    void setInsertCaret(const SynEditCaretType &insertCaret);

    SynGutter& gutter();

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

    const PSynHighlighterAttribute &rainbowAttr0() const;

    const PSynHighlighterAttribute &rainbowAttr1() const;

    const PSynHighlighterAttribute &rainbowAttr2() const;

    const PSynHighlighterAttribute &rainbowAttr3() const;

    int mouseWheelScrollSpeed() const;
    void setMouseWheelScrollSpeed(int newMouseWheelScrollSpeed);

    const QColor &foregroundColor() const;
    void setForegroundColor(const QColor &newForegroundColor);

    const QColor &backgroundColor() const;
    void setBackgroundColor(const QColor &newBackgroundColor);

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

//    void scrolled(SynScrollBarKind ScrollBar);
    void statusChanged(SynStatusChanges changes);

    void fontChanged();
    void tabSizeChanged();

protected:
    virtual bool onGetSpecialLineColors(int Line,
         QColor& foreground, QColor& backgroundColor) ;
    virtual void onGetEditingAreas(int Line, SynEditingAreaList& areaList);
    virtual void onGutterGetText(int aLine, QString& aText);
    virtual void onGutterPaint(QPainter& painter, int aLine, int X, int Y);
    virtual void onPaint(QPainter& painter);
    virtual void onPreparePaintHighlightToken(int line,
            int aChar, const QString& token, PSynHighlighterAttribute attr,
            SynFontStyles& style, QColor& foreground, QColor& background);
    virtual void onProcessCommand(SynEditorCommand Command, QChar AChar, void * pData);
    virtual void onCommandProcessed(SynEditorCommand Command, QChar AChar, void * pData);
    virtual void ExecuteCommand(SynEditorCommand Command, QChar AChar, void * pData);
    virtual void onEndFirstPaintLock();
    virtual void onBeginFirstPaintLock();

private:
    void clearAreaList(SynEditingAreaList areaList);
    void computeCaret(int X, int Y);
    void computeScroll(int X, int Y);

    void incPaintLock();
    void decPaintLock();
    bool mouseCapture();
    int clientWidth();
    int clientHeight();
    int clientTop();
    int clientLeft();
    QRect clientRect();
    void synFontChanged();
    void doOnPaintTransient(SynTransientType TransientType);
    void doSetSelText(const QString& Value);

    void updateLastCaretX();
    void ensureCursorPosVisible();
    void ensureCursorPosVisibleEx(bool ForceToMiddle);
    void scrollWindow(int dx,int dy);
    void setInternalDisplayXY(const DisplayCoord& aPos);
    void internalSetCaretXY(const BufferCoord& Value);
    void internalSetCaretX(int Value);
    void internalSetCaretY(int Value);
    void setStatusChanged(SynStatusChanges changes);
    void doOnStatusChange(SynStatusChanges changes);
    void insertBlock(const BufferCoord& BB, const BufferCoord& BE, const QString& ChangeStr,
                     bool AddToUndoList);
    void updateScrollbars();
    void updateCaret();
    void recalcCharExtent();
    QString expandAtWideGlyphs(const QString& S);
    void updateModifiedStatus();
    int scanFrom(int Index, int canStopIndex);
    void rescanRange(int line);
    void rescanRanges();
    void uncollapse(PSynEditFoldRange FoldRange);
    void collapse(PSynEditFoldRange FoldRange);

    void foldOnListInserted(int Line, int Count);
    void foldOnListDeleted(int Line, int Count);
    void foldOnListCleared();
    void rescanFolds(); // rescan for folds
    void rescanForFoldRanges();
    void scanForFoldRanges(PSynEditFoldRanges TopFoldRanges);
    int lineHasChar(int Line, int startChar, QChar character, const QString& highlighterAttrName);
    void findSubFoldRange(PSynEditFoldRanges TopFoldRanges,int FoldIndex,PSynEditFoldRanges& parentFoldRanges, PSynEditFoldRange Parent);
    PSynEditFoldRange collapsedFoldStartAtLine(int Line);
    void doOnPaintTransientEx(SynTransientType TransientType, bool Lock);
    void initializeCaret();
    PSynEditFoldRange foldStartAtLine(int Line);
    QString substringByColumns(const QString& s, int startColumn, int& colLen);
    PSynEditFoldRange foldAroundLine(int Line);
    PSynEditFoldRange foldAroundLineEx(int Line, bool WantCollapsed, bool AcceptFromLine, bool AcceptToLine);
    PSynEditFoldRange checkFoldRange(SynEditFoldRanges* FoldRangeToCheck,int Line, bool WantCollapsed, bool AcceptFromLine, bool AcceptToLine);
    PSynEditFoldRange foldEndAtLine(int Line);
    void paintCaret(QPainter& painter, const QRect rcClip);
    int textOffset() const;
    SynEditorCommand TranslateKeyCode(int key, Qt::KeyboardModifiers modifiers);
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
    void setSelectedTextEmpty();
    void setSelTextPrimitive(const QString& aValue);
    void setSelTextPrimitiveEx(SynSelectionMode PasteMode,
                               const QString& Value, bool AddToUndoList);
    void doLinesDeleted(int FirstLine, int Count);
    void doLinesInserted(int FirstLine, int Count);
    void properSetLine(int ALine, const QString& ALineText);
    void deleteSelection(const BufferCoord& BB, const BufferCoord& BE);
    void insertText(const QString& Value, SynSelectionMode PasteMode,bool AddToUndoList);
    int insertTextByNormalMode(const QString& Value);
    int insertTextByColumnMode(const QString& Value,bool AddToUndoList);
    int insertTextByLineMode(const QString& Value);
    void deleteFromTo(const BufferCoord& start, const BufferCoord& end);
    void setSelWord();
    void setWordBlock(BufferCoord Value);

    int calcIndentSpaces(int line, const QString& lineText, bool addIndent);


    void processGutterClick(QMouseEvent* event);

    void clearUndo();
    int findIndentsStartLine(int line, QVector<int> indents);
    BufferCoord getPreviousLeftBrace(int x,int y);
    bool canDoBlockIndent();

    QRect calculateCaretRect();

    //Commands
    void doDeleteLastChar();
    void doDeleteCurrentChar();
    void doDeleteWord();
    void doDeleteToEOL();
    void doDeleteLastWord();
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
    void doAddChar(QChar AChar);
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


private slots:
    void onBookMarkOptionsChanged();
    void onGutterChanged();
    void onLinesChanged();
    void onLinesChanging();
    void onLinesCleared();
    void onLinesDeleted(int index, int count);
    void onLinesInserted(int index, int count);
    void onLinesPutted(int index, int count);
    void onRedoAdded();
    void onScrollTimeout();
    void onUndoAdded();
    void onSizeOrFontChanged(bool bFont);
    void onChanged();
    void onScrolled(int value);

private:
    std::shared_ptr<QImage> mContentImage;
    SynEditFoldRanges mAllFoldRanges;
    SynEditCodeFolding mCodeFolding;
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
    SynFontSmoothMethod mFontSmoothing;
    bool mMouseMoved;
    /* IME input */
    int mImeCount;
    bool mMBCSStepAside;
    /* end of IME input */
    bool mInserting;
    bool mPainting;
    PSynEditStringList mLines;
    PSynEditStringList mOrigLines;
    PSynEditUndoList mOrigUndoList;
    PSynEditUndoList mOrigRedoList;
    int mLinesInWindow;
    int mLeftChar;
    int mPaintLock; // lock counter for internal calculations
    bool mReadOnly;
    int mRightEdge;
    QColor mRightEdgeColor;
    QColor mScrollHintColor;
    SynScrollHintFormat mScrollHintFormat;
    SynScrollStyle mScrollBars;
    int mTextHeight;
    int mTopLine;
    PSynHighlighter mHighlighter;
    QColor mSelectedForeground;
    QColor mSelectedBackground;
    QColor mForegroundColor;
    QColor mBackgroundColor;
    QColor mCaretColor;
    PSynHighlighterAttribute mRainbowAttr0;
    PSynHighlighterAttribute mRainbowAttr1;
    PSynHighlighterAttribute mRainbowAttr2;
    PSynHighlighterAttribute mRainbowAttr3;

    bool mCaretUseTextColor;
    QColor mActiveLineColor;
    PSynEditUndoList mUndoList;
    PSynEditUndoList mRedoList;
    SynEditMarkList  mBookMarks;
    QPoint mMouseDownPos;
    SynBookMarkOpt mBookMarkOpt;
    bool mHideSelection;
    int mMouseWheelAccumulator;
    SynEditCaretType mOverwriteCaret;
    SynEditCaretType  mInsertCaret;
    QPoint mCaretOffset;
    SynEditKeyStrokes mKeyStrokes;
    bool mModified;
    QDateTime mLastModifyTime;
    SynEditMarkList mMarkList;
    int mExtraLineSpacing;
    SynSelectionMode mSelectionMode;
    SynSelectionMode mActiveSelectionMode; //mode of the active selection
    bool mWantReturns;
    bool mWantTabs;
    SynGutter mGutter;
    int mTabWidth;
    QRect mInvalidateRect;
    SynStateFlags mStateFlags;
    SynEditorOptions mOptions;
    SynStatusChanges  mStatusChanges;
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
    SynPlaceMarkProc mOnClearMark;
    SynProcessCommandProc mOnCommandProcessed;
    SynMouseCursorProc mOnMouseCursor;
    SynPaintProc mOnPaint;
//    SynPreparePaintHighlightTokenProc mOnPaintHighlightToken;
    SynPlaceMarkProc mOnPlaceMark;
    SynProcessCommandProc mOnProcessingCommand;
    SynProcessCommandProc mOnProcessingUserCommand;

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

friend class SynEditTextPainter;

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
};

#endif // SYNEDIT_H
