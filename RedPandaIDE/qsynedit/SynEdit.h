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
    scModified = 0x0040,
    scSelection = 0x0080,
    scReadOnly = 0x0100,
    scOpenFile = 0x0200
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
  eoAutoIndent =        0x00000002, //Will indent the caret on new lines with the same amount of leading white space as the preceding line
  eoAddIndent =         0x00000004, //Will add one tab width of indent when typing { and :, and remove the same amount when typing }
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
  eoSpecialLineDefaultFg = 0x00010000, //disables the foreground text color override when using the OnSpecialLineColor event
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
using SynPreparePaintHighlightTokenProc = std::function<void(int row,
        int column, const QString& token, PSynHighlighterAttribute attr,
        SynFontStyles& style, QColor& foreground, QColor& background)>;
using SynSearchMathedProc = std::function<SynSearchAction(const QString& sSearch,
    const QString& sReplace, int Line, int ch, int wordLen)>;

class SynEdit;
using PSynEdit = std::shared_ptr<SynEdit>;

class SynEdit : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit SynEdit(QWidget *parent = nullptr);

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
    QPoint RowColumnToPixels(const DisplayCoord& coord) const;
    DisplayCoord bufferToDisplayPos(const BufferCoord& p) const;
    BufferCoord displayToBufferPos(const DisplayCoord& p) const;
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
    QString WordAtCursor();
    QString WordAtRowCol(const BufferCoord& XY);

    int charColumns(QChar ch) const;
    double dpiFactor() const;
    bool IsPointInSelection(const BufferCoord& Value) const;
    BufferCoord NextWordPos();
    BufferCoord NextWordPosEx(const BufferCoord& XY);
    BufferCoord WordStart();
    BufferCoord WordStartEx(const BufferCoord& XY);
    BufferCoord WordEnd();
    BufferCoord WordEndEx(const BufferCoord& XY);
    BufferCoord PrevWordPos();
    BufferCoord PrevWordPosEx(const BufferCoord& XY);
    void CommandProcessor(SynEditorCommand Command, QChar AChar = QChar(), void * pData = nullptr);
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
    void setSelText(const QString& Value);

    int searchReplace(const QString& sSearch, const QString& sReplace, SynSearchOptions options,
               PSynSearchBase searchEngine,  SynSearchMathedProc matchedCallback = nullptr);

    int maxScrollWidth() const;
    int maxScrollHeight() const;

    bool GetHighlighterAttriAtRowCol(const BufferCoord& XY, QString& Token,
      PSynHighlighterAttribute& Attri);
    bool GetHighlighterAttriAtRowCol(const BufferCoord& XY, QString& Token,
      bool& tokenFinished, SynHighlighterTokenType& TokenType,
      PSynHighlighterAttribute& Attri);
    bool GetHighlighterAttriAtRowColEx(const BufferCoord& XY, QString& Token,
      SynHighlighterTokenType& TokenType, SynTokenKind &TokenKind, int &Start,
      PSynHighlighterAttribute& Attri);

    //Commands
    virtual void cutToClipboard() { CommandProcessor(SynEditorCommand::ecCut);}
    virtual void copyToClipboard() { CommandProcessor(SynEditorCommand::ecCopy);}
    virtual void pasteFromClipboard() { CommandProcessor(SynEditorCommand::ecPaste);}
    virtual void undo()  { CommandProcessor(SynEditorCommand::ecUndo);}
    virtual void redo()  { CommandProcessor(SynEditorCommand::ecRedo);}
    virtual void zoomIn()  { CommandProcessor(SynEditorCommand::ecZoomIn);}
    virtual void zoomOut()  { CommandProcessor(SynEditorCommand::ecZoomOut);}
    virtual void selectAll() {  CommandProcessor(SynEditorCommand::ecSelectAll);}
    virtual void tab() { CommandProcessor(SynEditorCommand::ecTab);}
    virtual void untab() { CommandProcessor(SynEditorCommand::ecShiftTab);}
    virtual void toggleComment() { CommandProcessor(SynEditorCommand::ecToggleComment);}

    virtual void beginUpdate();
    virtual void endUpdate();
    virtual BufferCoord getMatchingBracket();
    virtual BufferCoord getMatchingBracketEx(BufferCoord APoint);


// setter && getters
    int topLine() const;
    void setTopLine(int value);

    int linesInWindow() const;

    int leftChar() const;
    void setLeftChar(int Value);

    BufferCoord blockBegin() const;
    BufferCoord blockEnd() const;

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

    QString lineText();
    void setLineText(const QString s);

    PSynEditStringList lines() const;
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

signals:
    void Changed();

    void ChainUndoAdded();
    void ChainRedoAdded();
    void ChainLinesChanging();
    void ChainLinesChanged();
    void ChainListCleared();

    void ChainListDeleted(int Index, int Count);
    void ChainListInserted(int Index, int Count);
    void ChainListPutted(int Index, int Count);

    void FilesDropped(int X,int Y, const QStringList& AFiles);
    void gutterClicked(Qt::MouseButton button, int x, int y, int line);
    void ImeInputed(const QString& s);

    void contextHelp(const QString& word);

    void scrolled(SynScrollBarKind ScrollBar);
    void statusChanged(SynStatusChanges changes);

    void fontChanged();
    void tabSizeChanged();

protected:
    bool isIdentChar(const QChar& ch);

protected:
    virtual bool onGetSpecialLineColors(int Line,
         QColor& foreground, QColor& backgroundColor) ;
    virtual void onGetEditingAreas(int Line, SynEditingAreaList& areaList);
    virtual void onGutterGetText(int aLine, QString& aText);
    virtual void onGutterPaint(QPainter& painter, int aLine, int X, int Y);
    virtual void onPaint(QPainter& painter);
    virtual void onProcessCommand(SynEditorCommand Command, QChar AChar, void * pData);
    virtual void onCommandProcessed(SynEditorCommand Command, QChar AChar, void * pData);
    virtual void ExecuteCommand(SynEditorCommand Command, QChar AChar, void * pData);

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
    int scanFrom(int Index);
    void scanRanges();
    void uncollapse(PSynEditFoldRange FoldRange);
    void collapse(PSynEditFoldRange FoldRange);

    void foldOnListInserted(int Line, int Count);
    void foldOnListDeleted(int Line, int Count);
    void foldOnListCleared();
    void rescan(); // rescan for folds
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
    void MoveCaretHorz(int DX, bool isSelection);
    void MoveCaretVert(int DY, bool isSelection);
    void MoveCaretAndSelection(const BufferCoord& ptBefore, const BufferCoord& ptAfter,
                               bool isSelection);
    void MoveCaretToLineStart(bool isSelection);
    void MoveCaretToLineEnd(bool isSelection);
    void SetSelectedTextEmpty();
    void SetSelTextPrimitive(const QString& aValue);
    void SetSelTextPrimitiveEx(SynSelectionMode PasteMode,
                               const QString& Value, bool AddToUndoList);
    void DoLinesDeleted(int FirstLine, int Count);
    void DoLinesInserted(int FirstLine, int Count);
    void ProperSetLine(int ALine, const QString& ALineText);
    void DeleteSelection(const BufferCoord& BB, const BufferCoord& BE);
    void InsertText(const QString& Value, SynSelectionMode PasteMode,bool AddToUndoList);
    int InsertTextByNormalMode(const QString& Value);
    int InsertTextByColumnMode(const QString& Value,bool AddToUndoList);
    int InsertTextByLineMode(const QString& Value);
    void DeleteFromTo(const BufferCoord& start, const BufferCoord& end);
    void SetSelWord();
    void SetWordBlock(BufferCoord Value);


    void processGutterClick(QMouseEvent* event);

    void clearUndo();
    BufferCoord GetPreviousLeftBracket(int x,int y);
    bool CanDoBlockIndent();

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

private:
    void setBlockBegin(BufferCoord value);
    void setBlockEnd(BufferCoord Value);
    void setSelLength(int Value);

private slots:
    void bookMarkOptionsChanged();
    void gutterChanged();
    void linesChanged();
    void linesChanging();
    void linesCleared();
    void linesDeleted(int index, int count);
    void linesInserted(int index, int count);
    void linesPutted(int index, int count);
    void redoAdded();
    void scrollTimerHandler();
    void undoAdded();
    void sizeOrFontChanged(bool bFont);
    void doChange();
    void doScrolled(int value);

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
    QColor mCaretColor;
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
    SynPreparePaintHighlightTokenProc mOnPaintHighlightToken;
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
