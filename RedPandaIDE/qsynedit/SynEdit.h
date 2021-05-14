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
    ctVerticalLine, ctHorizontalLine, ctHalfBlock, ctBlock
};

enum class SynStatusChange {
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
  eoAutoIndent = 0x00000002, //Will indent the caret on new lines with the same amount of leading white space as the preceding line
  eoAddIndent = 0x00000004, //Will add one tab width of indent when typing { and :, and remove the same amount when typing }
  eoAutoSizeMaxScrollWidth = 0x00000008, //Automatically resizes the MaxScrollWidth property when inserting text
  //eoDisableScrollArrows = 0x00000010 , //Disables the scroll bar arrow buttons when you can't scroll in that direction any more
  eoDragDropEditing = 0x00000020, //Allows you to select a block of text and drag it within the document to another location
  eoDropFiles = 0x00000040, //Allows the editor accept OLE file drops
  eoEnhanceHomeKey = 0x00000080, //enhances home key positioning, similar to visual studio
  eoEnhanceEndKey = 0x00000100, //enhances End key positioning, similar to JDeveloper
  eoGroupUndo = 0x00000200, //When undoing/redoing actions, handle all continous changes of the same kind in one call instead undoing/redoing each command separately
  eoHalfPageScroll = 0x00000400, //When scrolling with page-up and page-down commands, only scroll a half page at a time
  eoHideShowScrollbars = 0x00000800, //if enabled, then the scrollbars will only show when necessary.  If you have ScrollPastEOL, then it the horizontal bar will always be there (it uses MaxLength instead)
  eoKeepCaretX = 0x00001000 , //When moving through lines w/o Cursor Past EOL, keeps the X position of the cursor
  eoNoCaret = 0x00002000, //Makes it so the caret is never visible
  eoNoSelection = 0x00004000, //Disables selecting text
  eoRightMouseMovesCursor = 0x00008000, //When clicking with the right mouse for a popup menu, move the cursor to that location
  eoScrollByOneLess = 0x00010000, //Forces scrolling to be one less
  eoScrollHintFollows = 0x00020000, //The scroll hint follows the mouse when scrolling vertically
  eoScrollPastEof = 0x00040000, //Allows the cursor to go past the end of file marker
  eoScrollPastEol = 0x00080000, //Allows the cursor to go past the last character into the white space at the end of a line
  eoShowScrollHint = 0x00100000, //Shows a hint of the visible line numbers when scrolling vertically
  eoShowSpecialChars = 0x00200000, //Shows the special Characters
  eoSmartTabDelete = 0x00400000, //similar to Smart Tabs, but when you delete characters
  eoSmartTabs = 0x00800000, //When tabbing, the cursor will go to the next non-white space character of the previous line
  eoSpecialLineDefaultFg = 0x01000000, //disables the foreground text color override when using the OnSpecialLineColor event
  eoTabIndent = 0x02000000, //When active <Tab> and <Shift><Tab> act as block indent, unindent when text is selected
  eoTabsToSpaces = 0x04000000, //Converts a tab character to a specified number of space characters
  eoShowRainbowColor = 0x08000000,
  eoTrimTrailingSpaces = 0x10000000 //Spaces at the end of lines will be trimmed and not saved
};

Q_DECLARE_FLAGS(SynEditorOptions, SynEditorOption)

Q_DECLARE_OPERATORS_FOR_FLAGS(SynEditorOptions)

enum class SynReplaceAction {
    raCancel, raSkip, raReplace, raReplaceAll
};

struct SynEditingArea {
    int beginX;
    int endX;
    QColor color;
};


using PSynEditingArea = std::shared_ptr<SynEditingArea>;
using SynEditingAreaList = QList<PSynEditingArea>;
enum class SynEditingAreaType {
  eatRectangleBorder,
  eatWaveUnderLine,
  eatUnderLine
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
using SynReplaceTextProc = std::function<void(const QString& ASearch, const QString& AReplace,
    int Line, int Column, int wordLen, SynReplaceAction& action)>;
using SynSpecialLineColorsProc = std::function<void(int Line,
    bool& Special, QColor& foreground, QColor& backgroundColor)>;
using SynEditingAreasProc = std::function<void(int Line, SynEditingAreaList& areaList,
            QColor& borderColor,SynEditingAreaType& areaType)>;
using SynGutterGetTextProc = std::function<void(int aLine, QString& aText)>;
using SynTGutterPaintProc = std::function<void(int aLine, int X, int Y)>;

class SynEdit;
using PSynEdit = std::shared_ptr<SynEdit>;

class SynEdit : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit SynEdit(QWidget *parent = nullptr);

    int displayLineCount();
    DisplayCoord displayXY();
    int displayX();
    int displayY();
    BufferCoord caretXY();
    int caretX();
    int caretY();

    void setCaretX(int value);
    void setCaretY(int value);
    void setCaretXY(const BufferCoord& value);
    void setCaretXYEx(bool CallEnsureCursorPos, BufferCoord value);
    void setCaretXYCentered(bool ForceToMiddle, const BufferCoord& value);

    void invalidateGutter();
    void invalidateGutterLine(int aLine);
    void invalidateGutterLines(int FirstLine, int LastLine);
    DisplayCoord pixelsToNearestRowColumn(int aX, int aY);
    DisplayCoord pixelsToRowColumn(int aX, int aY);
    DisplayCoord bufferToDisplayPos(const BufferCoord& p);
    BufferCoord displayToBufferPos(const DisplayCoord& p);
    int rowToLine(int aRow);
    int lineToRow(int aLine);
    int foldRowToLine(int Row);
    int foldLineToRow(int Line);
    void setDefaultKeystrokes();
    void invalidateLine(int Line);
    void invalidateLines(int FirstLine, int LastLine);
    void invalidateSelection();
    void invalidateRect(const QRect& rect);
    void invalidate();
    void lockPainter();
    void unlockPainter();
    bool selAvail();
    void setCaretAndSelection(const BufferCoord& ptCaret,
                              const BufferCoord& ptBefore,
                              const BufferCoord& ptAfter);
    void clearUndo();

    int topLine() const;
    void setTopLine(int value);

    int linesInWindow() const;

    int leftChar() const;
    void setLeftChar(int Value);

    BufferCoord blockBegin() const;
    void setBlockBegin(BufferCoord value);

    BufferCoord blockEnd() const;
    void setBlockEnd(BufferCoord Value);

    SynSelectionMode activeSelectionMode() const;
    void setActiveSelectionMode(const SynSelectionMode &Value);

    int charsInWindow() const;

    int charWidth() const;

    int gutterWidth() const;
    void setGutterWidth(int value);

    bool modified() const;
    void setModified(bool Value);

    int maxScrollWidth() const;
    void setMaxScrollWidth(int Value);

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
    void GutterClicked(Qt::MouseButton button, int x, int y, int line, PSynEditMark mark);
    void ImeInputed(const QString& s);

    void contextHelp(const QString& word);

    void scrolled(SynScrollBarKind ScrollBar);
    void statusChanged(SynStatusChanges changes);

private:
    void clearAreaList(SynEditingAreaList areaList);
    void computeCaret(int X, int Y);
    void computeScroll(int X, int Y);
    void doBlockIndent();

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
    void uncollapse(PSynEditFoldRange FoldRange);
    void foldOnListInserted(int Line, int Count);
    void foldOnListDeleted(int Line, int Count);
    void foldOnListCleared();
    void rescan(); // rescan for folds
    void rescanForFoldRanges();
    void scanForFoldRanges(PSynEditFoldRanges TopFoldRanges);
    int lineHasChar(int Line, int startChar, QChar character, const QString& highlighterAttrName);
    void findSubFoldRange(PSynEditFoldRanges TopFoldRanges,int FoldIndex, PSynEditFoldRange Parent);
    PSynEditFoldRange collapsedFoldStartAtLine(int Line);
    void setSelTextPrimitiveEx(SynSelectionMode PasteMode,
                               const QString& Value, bool AddToUndoList);
    void doOnPaintTransientEx(SynTransientType TransientType, bool Lock);
    void initializeCaret();

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

private:
    SynEditFoldRanges mAllFoldRanges;
    SynEditCodeFolding mCodeFolding;
    bool mUseCodeFolding;
    bool  mAlwaysShowCaret;
    BufferCoord mBlockBegin;
    BufferCoord mBlockEnd;
    int mCaretX;
    int mLastCaretX;
    int mCaretY;
    int mCharsInWindow;
    int mCharWidth;
    QFont mFontDummy;
    QColor mColor;
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
    int mMaxScrollWidth;
    int mPaintLock; // lock counter for internal calculations
    bool mReadOnly;
    int mRightEdge;
    QColor mRightEdgeColor;
    QColor mScrollHintColor;
    SynScrollHintFormat mScrollHintFormat;
    SynScrollStyle mScrollBars;
    int mTextHeight;
    int mTextOffset;
    int mTopLine;
    PSynHighlighter mHighlighter;
    QColor mSelectedForeground;
    QColor mSelectedBackground;
    QColor mActiveLineColor;
    PSynEditUndoList mUndoList;
    PSynEditUndoList mRedoList;
    SynEditMarkList  mBookMarks;
    int mMouseDownX;
    int mMouseDownY;
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

    bool mShowSpecChar;
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

    SynReplaceTextProc mOnReplaceText;
    SynSpecialLineColorsProc mOnSpecialLineColors;
    SynEditingAreasProc mOnEditingAreas;
    SynGutterGetTextProc  mOnGutterGetText;
    SynTGutterPaintProc mOnGutterPaint;
    int mGutterWidth;

    //caret blink related
    int m_blinkTimerId;
    int m_bliknStatus;


    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event) override;

friend class SynEditTextPainter;
};

#endif // SYNEDIT_H
