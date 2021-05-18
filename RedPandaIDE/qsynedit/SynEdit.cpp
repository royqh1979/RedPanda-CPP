#include "SynEdit.h"
#include <QApplication>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>
#include <QScrollBar>
#include <QPaintEvent>
#include "highlighter/base.h"
#include "Constants.h"

SynEdit::SynEdit(QWidget *parent) : QAbstractScrollArea(parent)
{
    mPaintLock = 0;
    mPainterLock = 0;
    mPainting = false;
    mLines = std::make_shared<SynEditStringList>();
    mOrigLines = mLines;
    //fPlugins := TList.Create;
    mMouseMoved = false;
    mUndoing = false;
    mLines->connect(mLines.get(), &SynEditStringList::changed, this, &SynEdit::linesChanged);
    mLines->connect(mLines.get(), &SynEditStringList::changing, this, &SynEdit::linesChanging);
    mLines->connect(mLines.get(), &SynEditStringList::cleared, this, &SynEdit::linesCleared);
    mLines->connect(mLines.get(), &SynEditStringList::deleted, this, &SynEdit::linesDeleted);
    mLines->connect(mLines.get(), &SynEditStringList::inserted, this, &SynEdit::linesInserted);
    mLines->connect(mLines.get(), &SynEditStringList::putted, this, &SynEdit::linesPutted);
#ifdef Q_OS_WIN
    mFontDummy = QFont("Consolas",10);
#elif Q_OS_LINUX
    mFontDummy = QFont("terminal",14);
#else
#error "Not supported!"
#endif
    mUndoList = std::make_shared<SynEditUndoList>();
    mUndoList->connect(mUndoList.get(), &SynEditUndoList::addedUndo, this, &SynEdit::undoAdded);
    mOrigUndoList = mUndoList;
    mRedoList = std::make_shared<SynEditUndoList>();
    mRedoList->connect(mRedoList.get(), &SynEditUndoList::addedUndo, this, &SynEdit::redoAdded);
    mOrigRedoList = mRedoList;
    //DoubleBuffered = false;
    mActiveLineColor = QColor();
    mSelectedBackground = QColor();
    mSelectedForeground = QColor();

    mBookMarkOpt.connect(&mBookMarkOpt, &SynBookMarkOpt::changed, this, &SynEdit::bookMarkOptionsChanged);
    //  fRightEdge has to be set before FontChanged is called for the first time
    mRightEdge = 80;
    mGutter.setRightOffset(21);
    mGutter.connect(&mGutter, &SynGutter::changed, this, &SynEdit::gutterChanged);
    mGutterWidth = mGutter.width();
    mTextOffset = mGutterWidth + 2;
    //ControlStyle := ControlStyle + [csOpaque, csSetCaption, csNeedsBorderPaint];
    //Height := 150;
    //Width := 200;
    this->setCursor(Qt::CursorShape::IBeamCursor);
    //TabStop := True;
    mInserting = true;
    mMaxScrollWidth = 1024;
    mScrollBars = SynScrollStyle::ssBoth;
    this->setFrameShape(QFrame::Panel);
    this->setFrameShadow(QFrame::Sunken);
    this->setLineWidth(1);
    mInsertCaret = SynEditCaretType::ctVerticalLine;
    mOverwriteCaret = SynEditCaretType::ctBlock;
    mSelectionMode = SynSelectionMode::smNormal;
    mActiveSelectionMode = SynSelectionMode::smNormal;
    //stop qt to auto fill background
    setAutoFillBackground(false);
    //fFocusList := TList.Create;
    //fKbdHandler := TSynEditKbdHandler.Create;
    //fMarkList.OnChange := MarkListChange;
    setDefaultKeystrokes();
    mRightEdgeColor = QColorConstants::Svg::silver;
    /* IME input */
    mImeCount = 0;
    mMBCSStepAside = false;
    /* end of IME input */
    mWantReturns = true;
    mWantTabs = false;
    mTabWidth = 4;
    mLeftChar = 1;
    mTopLine = 1;
    mCaretX = 1;
    mLastCaretX = 1;
    mCaretY = 1;
    mBlockBegin.Char = 1;
    mBlockBegin.Line = 1;
    mBlockEnd = mBlockBegin;
    mOptions = eoAutoIndent | eoDragDropEditing | eoEnhanceEndKey |
            eoScrollPastEol | eoShowScrollHint | eoSmartTabs | eoTabsToSpaces |
            eoSmartTabDelete| eoGroupUndo;
    mScrollTimer = new QTimer(this);
    mScrollTimer->setInterval(100);
    connect(mScrollTimer, &QTimer::timeout,this, &SynEdit::scrollTimerHandler);

    mScrollHintColor = QColorConstants::Yellow;
    mScrollHintFormat = SynScrollHintFormat::shfTopLineOnly;

    synFontChanged();
}

int SynEdit::displayLineCount()
{
    return lineToRow(mLines->count());
}

DisplayCoord SynEdit::displayXY()
{
    return bufferToDisplayPos(caretXY());
}

int SynEdit::displayX()
{
    return displayXY().Column;
}

int SynEdit::displayY()
{
    return displayXY().Row;
}

BufferCoord SynEdit::caretXY()
{
    BufferCoord result;
    result.Char = caretX();
    result.Line = caretY();
    return result;
}

int SynEdit::caretX()
{
    return mCaretX;
}

int SynEdit::caretY()
{
    return mCaretY;
}

void SynEdit::setCaretX(int value)
{
    setCaretXY({value,mCaretY});
}

void SynEdit::setCaretY(int value)
{
    setCaretXY({mCaretX,value});
}

void SynEdit::setCaretXY(const BufferCoord &value)
{
    setCaretXYCentered(false,value);
}

void SynEdit::setCaretXYEx(bool CallEnsureCursorPos, BufferCoord value)
{
    bool vTriggerPaint=true; //how to test it?

    if (vTriggerPaint)
        doOnPaintTransient(SynTransientType::ttBefore);
    int nMaxX = mMaxScrollWidth + 1;
    if (value.Line > mLines->count())
        value.Line = mLines->count();
    if (value.Line < 1) {
        // this is just to make sure if Lines stringlist should be empty
        value.Line = 1;
        if (!mOptions.testFlag(SynEditorOption::eoScrollPastEol)) {
            nMaxX = 1;
        }
    } else {
        if (!mOptions.testFlag(SynEditorOption::eoScrollPastEol))
            nMaxX = mLines->getString(value.Line-1).length();
    }
    if ((value.Char > nMaxX) && (! (mOptions.testFlag(SynEditorOption::eoScrollPastEol)) ||
      !(mOptions.testFlag(SynEditorOption::eoAutoSizeMaxScrollWidth))) )
        value.Char = nMaxX;
    if (value.Char < 1)
        value.Char = 1;
    if ((value.Char != mCaretX) || (value.Line != mCaretY)) {
        incPaintLock();
        auto action = finally([this]{
            decPaintLock();
        });
        // simply include the flags, fPaintLock is > 0
        if (mCaretX != value.Char) {
            mCaretX = value.Char;
            mStatusChanges.setFlag(SynStatusChange::scCaretX);
        }
        if (mCaretY != value.Line) {
            if (!mActiveLineColor.isValid()) {
                invalidateLine(value.Line);
                invalidateLine(mCaretY);
            }
            mCaretY = value.Line;
            mStatusChanges.setFlag(SynStatusChange::scCaretY);
        }
        // Call UpdateLastCaretX before DecPaintLock because the event handler it
        // calls could raise an exception, and we don't want fLastCaretX to be
        // left in an undefined state if that happens.
        updateLastCaretX();
        if (CallEnsureCursorPos)
            ensureCursorPosVisible();
        mStateFlags.setFlag(SynStateFlag::sfCaretChanged);
        mStateFlags.setFlag(SynStateFlag::sfScrollbarChanged);
    } else {
      // Also call UpdateLastCaretX if the caret didn't move. Apps don't know
      // anything about fLastCaretX and they shouldn't need to. So, to avoid any
      // unwanted surprises, always update fLastCaretX whenever CaretXY is
      // assigned to.
      // Note to SynEdit developers: If this is undesirable in some obscure
      // case, just save the value of fLastCaretX before assigning to CaretXY and
      // restore it afterward as appropriate.
      updateLastCaretX();
    }
    if (vTriggerPaint)
      doOnPaintTransient(SynTransientType::ttAfter);

}

void SynEdit::setCaretXYCentered(bool ForceToMiddle, const BufferCoord &value)
{
    incPaintLock();
    auto action = finally([this] {
        decPaintLock();
    });
    mStatusChanges.setFlag(SynStatusChange::scSelection);
    setCaretXYEx(ForceToMiddle,value);
    if (selAvail())
        invalidateSelection();
    mBlockBegin.Char = mCaretX;
    mBlockBegin.Line = mCaretY;
    mBlockEnd = mBlockBegin;
    if (ForceToMiddle)
        ensureCursorPosVisibleEx(true); // but here after block has been set

}

void SynEdit::invalidateGutter()
{
    invalidateGutterLines(-1, -1);
}

void SynEdit::invalidateGutterLine(int aLine)
{
    if ((aLine < 1) || (aLine > mLines->count()))
        return;

    invalidateGutterLines(aLine, aLine);
}

void SynEdit::invalidateGutterLines(int FirstLine, int LastLine)
{
    QRect rcInval;
    if (!isVisible())
        return;
    if (FirstLine == -1 && LastLine == -1) {
        rcInval = QRect(clientLeft(), clientTop(), mGutterWidth, clientHeight());
        if (mStateFlags.testFlag(SynStateFlag::sfLinesChanging))
            mInvalidateRect = mInvalidateRect.united(rcInval);
        else
            invalidateRect(rcInval);
    } else {
        // find the visible lines first
        if (LastLine < FirstLine)
            std::swap(LastLine, FirstLine);
        if (mUseCodeFolding) {
            FirstLine = lineToRow(FirstLine);
            if (LastLine <= mLines->count())
              LastLine = lineToRow(LastLine);
            else
              LastLine = INT_MAX;
        }
        FirstLine = std::max(FirstLine, mTopLine);
        LastLine = std::min(LastLine, mTopLine + mLinesInWindow);
        // any line visible?
        if (LastLine >= FirstLine) {
            rcInval = {clientLeft(), clientTop()+mTextHeight * (FirstLine - mTopLine),
                       mGutterWidth, mTextHeight * (LastLine - mTopLine + 1)};
            if (mStateFlags.testFlag(SynStateFlag::sfLinesChanging)) {
                mInvalidateRect =  mInvalidateRect.united(rcInval);
            } else {
                invalidateRect(rcInval);
            }
        }
    }
}

/**
 * @brief Convert point on the edit (x,y) to (row,column)
 * @param aX
 * @param aY
 * @return
 */
DisplayCoord SynEdit::pixelsToNearestRowColumn(int aX, int aY)
{
    // Result is in display coordinates
    float f;
    f = (aX - mGutterWidth - 2.0) / mCharWidth;
    // don't return a partially visible last line
    if (aY >= mLinesInWindow * mTextHeight) {
        aY = mLinesInWindow * mTextHeight - 1;
        if (aY < 0)
            aY = 0;
    }
    return {
      .Column = std::max(1, (int)(leftChar() + round(f))),
      .Row = std::max(1, mTopLine + (aY / mTextHeight))
    };
}

DisplayCoord SynEdit::pixelsToRowColumn(int aX, int aY)
{
    return {
        .Column = std::max(1, mLeftChar + ((aX - mGutterWidth - 2) / mCharWidth)),
        .Row = std::max(1, mTopLine + (aY / mTextHeight))
    };
}

/**
 * @brief takes a position in the text and transforms it into
 *  the row and column it appears to be on the screen
 * @param p
 * @return
 */
DisplayCoord SynEdit::bufferToDisplayPos(const BufferCoord &p)
{
    DisplayCoord result {p.Char,p.Line};
    // Account for tabs and charColumns
    result.Column = charToColumn(p.Line,p.Char);
    // Account for code folding
    if (mUseCodeFolding)
        result.Row = foldLineToRow(result.Row);
    return result;
}

/**
 * @brief takes a position on screen and transfrom it into position of text
 * @param p
 * @return
 */
BufferCoord SynEdit::displayToBufferPos(const DisplayCoord &p)
{
    BufferCoord Result{p.Column,p.Row};
    // Account for code folding
    if (mUseCodeFolding)
        Result.Line = foldRowToLine(Result.Line);
    // Account for tabs
    if (Result.Line <= mLines->count() ) {
        QString s = mLines->getString(Result.Line - 1);
        int l = s.length();
        int x = 0;
        int i = 0;

        while (x < p.Column) {
            if (i < l && s[i] == '\t')
                x += mTabWidth - (x % mTabWidth);
            else
                x += charColumns(s[i]);
            i++;
        }
        Result.Char = i;
    }
}

int SynEdit::charToColumn(int aLine, int aChar)
{
    if (aLine < mLines->count()) {
        QString s = mLines->getString(aLine - 1);
        int l = s.length();
        int x = 0;
        for (int i=0;i<aChar-1;i++) {
            if (i<=l && s[i] == '\t')
                x+=mTabWidth - (x % mTabWidth);
            else
                x+=charColumns(s[i]);
        }
        return x+1;
    }
    throw BaseError(SynEdit::tr("Line %1 is out of range").arg(aLine));
}

int SynEdit::stringColumns(const QString &line)
{
    int columns = 0;
    if (!line.isEmpty()) {
        int charCols;
        for (int i=0;i<line.length();i++) {
            QChar ch = line[i];
            if (ch == '\t') {
                charCols = mTabWidth - columns % mTabWidth;
            } else {
                charCols = charColumns(ch);
            }
            columns+=charCols;
        }
    }
    return columns;
}

int SynEdit::rowToLine(int aRow)
{
    return displayToBufferPos({1, aRow}).Line;
}

int SynEdit::lineToRow(int aLine)
{
    return bufferToDisplayPos({1, aLine}).Row;
}

int SynEdit::foldRowToLine(int Row)
{
    int result = Row;
    for (int i=0;i<mAllFoldRanges.count();i++) {
        PSynEditFoldRange range = mAllFoldRanges[i];
        if (range->collapsed && !range->parentCollapsed() && range->fromLine < result) {
            result += range->linesCollapsed;
        }
    }
    return result;
}

int SynEdit::foldLineToRow(int Line)
{
    int result = Line;
    for (int i=mAllFoldRanges.count()-1;i>=0;i--) {
        PSynEditFoldRange range =mAllFoldRanges[i];
        if (range->collapsed && !range->parentCollapsed()) {
            // Line is found after fold
            if (range->toLine < Line)
                result -= range->linesCollapsed;
            // Inside fold
            else if (range->fromLine < Line && Line <= range->toLine)
                result -= Line - range->fromLine;
        }
    }
    return result;
}

void SynEdit::setDefaultKeystrokes()
{
    mKeyStrokes.resetDefaults();
}

void SynEdit::invalidateLine(int Line)
{
    QRect rcInval;
    if (mPainterLock >0)
        return;
    if (Line<1 || Line>mLines->count() || !isVisible())
        return;

    // invalidate text area of this line
    if (mUseCodeFolding)
        Line = foldLineToRow(Line);
    if (Line >= mTopLine && Line <= mTopLine + mLinesInWindow) {
        rcInval = { clientLeft() + mGutterWidth,
                    clientTop() + mTextHeight * (Line - mTopLine),
                    clientWidth(),
                    mTextHeight};
        if (mStateFlags.testFlag(SynStateFlag::sfLinesChanging))
            mInvalidateRect = mInvalidateRect.united(rcInval);
        else
            invalidateRect(rcInval);
    }
}

void SynEdit::invalidateLines(int FirstLine, int LastLine)
{
    if (mPainterLock>0)
        return;

    if (!isVisible())
        return;
    if (FirstLine == -1 && LastLine == -1) {
        QRect rcInval = clientRect();
        rcInval.setLeft(rcInval.left()+mGutterWidth);
        if (mStateFlags.testFlag(SynStateFlag::sfLinesChanging)) {
            mInvalidateRect = mInvalidateRect.united(rcInval);
        } else {
            invalidateRect(rcInval);
        }
    } else {
        FirstLine = std::max(FirstLine, 1);
        LastLine = std::max(LastLine, 1);
        // find the visible lines first
        if (LastLine < FirstLine)
            std::swap(LastLine, FirstLine);

        if (LastLine >= mLines->count())
          LastLine = INT_MAX; // paint empty space beyond last line

        if (mUseCodeFolding) {
          FirstLine = lineToRow(FirstLine);
          // Could avoid this conversion if (First = Last) and
          // (Length < CharsInWindow) but the dependency isn't worth IMO.
          if (LastLine < mLines->count())
              LastLine = lineToRow(LastLine + 1) - 1;
        }

        // mTopLine is in display coordinates, so FirstLine and LastLine must be
        // converted previously.
        FirstLine = std::max(FirstLine, mTopLine);
        LastLine = std::min(LastLine, mTopLine + mLinesInWindow);

        // any line visible?
        if (LastLine >= FirstLine) {
            QRect rcInval = {
                clientLeft()+mGutterWidth,
                mTextHeight * (FirstLine - mTopLine),
                clientWidth(), mTextHeight * (LastLine - mTopLine + 1)
            };
            if (mStateFlags.testFlag(SynStateFlag::sfLinesChanging))
                mInvalidateRect = mInvalidateRect.united(rcInval);
            else
                invalidateRect(rcInval);
        }
    }
}

void SynEdit::invalidateSelection()
{
    if (mPainterLock>0)
        return;
    invalidateLines(mBlockBegin.Line, mBlockEnd.Line);
}

void SynEdit::invalidateRect(const QRect &rect)
{
    if (mPainterLock>0)
        return;
    viewport()->update(rect);
}

void SynEdit::invalidate()
{
    if (mPainterLock>0)
        return;
    viewport()->update();
}

void SynEdit::lockPainter()
{
    mPainterLock++;
}

void SynEdit::unlockPainter()
{
    Q_ASSERT(mPainterLock>0);
    mPainterLock--;
}

bool SynEdit::selAvail()
{
    return (mBlockBegin.Char != mBlockEnd.Char) ||
            ((mBlockBegin.Line != mBlockEnd.Line) && (mActiveSelectionMode != SynSelectionMode::smColumn));
}

void SynEdit::setCaretAndSelection(const BufferCoord &ptCaret, const BufferCoord &ptBefore, const BufferCoord &ptAfter)
{
    SynSelectionMode vOldMode = mActiveSelectionMode;
    incPaintLock();
    auto action = finally([this,vOldMode]{
        mActiveSelectionMode = vOldMode;
        decPaintLock();
    });
    internalSetCaretXY(ptCaret);
    setBlockBegin(ptBefore);
    setBlockEnd(ptAfter);
}

void SynEdit::clearUndo()
{
    mUndoList->Clear();
    mRedoList->Clear();
}

int SynEdit::charColumns(QChar ch)
{
    return std::ceil(fontMetrics().horizontalAdvance(ch) * dpiFactor() / mCharWidth);
}

double SynEdit::dpiFactor()
{
    return fontMetrics().fontDpi() / 96.0;
}

void SynEdit::clearAreaList(SynEditingAreaList areaList)
{
    areaList.clear();
}

void SynEdit::computeCaret(int X, int Y)
{
    DisplayCoord vCaretNearestPos = pixelsToNearestRowColumn(X, Y);
    vCaretNearestPos.Row = MinMax(vCaretNearestPos.Row, 1, displayLineCount());
    setInternalDisplayXY(vCaretNearestPos);
}

void SynEdit::computeScroll(int X, int Y)
{
    QRect iScrollBounds; // relative to the client area
    // don't scroll if dragging text from other control
//      if (not MouseCapture) and (not Dragging) then begin
//        fScrollTimer.Enabled := False;
//        Exit;
//      end;

    iScrollBounds = QRect(mGutterWidth+this->frameWidth(), this->frameWidth(), mCharsInWindow * mCharWidth,
        mLinesInWindow * mTextHeight);

    if (X < iScrollBounds.left())
        mScrollDeltaX = (X - iScrollBounds.left()) % mCharWidth - 1;
    else if (X >= iScrollBounds.right())
        mScrollDeltaX = (X - iScrollBounds.right()) % mCharWidth + 1;
    else
        mScrollDeltaX = 0;

    if (Y < iScrollBounds.top())
        mScrollDeltaY = (Y - iScrollBounds.top()) % mTextHeight - 1;
    else if (Y >= iScrollBounds.bottom())
        mScrollDeltaY = (Y - iScrollBounds.bottom()) % mTextHeight + 1;
      else
        mScrollDeltaY = 0;

    if (mScrollDeltaX!=0 || mScrollDeltaY!=0)
        mScrollTimer->start();
}

void SynEdit::doBlockIndent()
{
    BufferCoord  OrgCaretPos;
    BufferCoord  BB, BE;
    QString StrToInsert;
    int Run;
    int e,x,i,InsertStrLen;
    QString Spaces;
    SynSelectionMode OrgSelectionMode;
    BufferCoord InsertionPos;

    OrgSelectionMode = mActiveSelectionMode;
    OrgCaretPos = caretXY();
    StrToInsert = nullptr;
    if (selAvail()) {
        auto action = finally([&,this]{
            if (BE.Char > 1)
              BE.Char+=Spaces.length();
            setCaretAndSelection(OrgCaretPos,
              {BB.Char + Spaces.length(), BB.Line}, BE);
            setActiveSelectionMode(OrgSelectionMode);
        });
        // keep current selection detail
        BB = mBlockBegin;
        BE = mBlockEnd;
        // build text to insert
        if (BE.Char == 1) {
            e = BE.Line - 1;
            x = 1;
        } else {
            e = BE.Line;
            if (mOptions.testFlag(SynEditorOption::eoTabsToSpaces))
              x = caretX() + mTabWidth;
            else
              x = caretX() + 1;
        }
        if (mOptions.testFlag(eoTabsToSpaces)) {
            InsertStrLen = (mTabWidth + 2) * (e - BB.Line) + mTabWidth + 1;
            //               chars per line * lines-1    + last line + null char
            StrToInsert.resize(InsertStrLen);
            Run = 0;
            Spaces = QString(mTabWidth,' ') ;
        } else {
            InsertStrLen = 3 * (e - BB.Line) + 2;
            //         #9#13#10 * lines-1 + (last line's #9 + null char)
            StrToInsert.resize(InsertStrLen);
            Run = 0;
            Spaces = "\t";
        }
        for (i = BB.Line; i<e;i++) {
            StrToInsert.replace(Run,Spaces.length()+2,Spaces+"\r\n");
            Run+=Spaces.length()+2;
        }
        StrToInsert.replace(Run,Spaces.length(),Spaces);

        {
            mUndoList->BeginBlock();
            auto action2=finally([this]{
                mUndoList->EndBlock();
            });
            InsertionPos.Line = BB.Line;
            if (mActiveSelectionMode == SynSelectionMode::smColumn)
              InsertionPos.Char = std::min(BB.Char, BE.Char);
            else
              InsertionPos.Char = 1;
            insertBlock(InsertionPos, InsertionPos, StrToInsert, true);
            mUndoList->AddChange(SynChangeReason::crIndent, BB, BE, "", SynSelectionMode::smColumn);
            //We need to save the position of the end block for redo
            mUndoList->AddChange(SynChangeReason::crIndent,
              {BB.Char + Spaces.length(), BB.Line},
              {BE.Char + Spaces.length(), BE.Line},
              "", SynSelectionMode::smColumn);
            //adjust the x position of orgcaretpos appropriately
            OrgCaretPos.Char = x;
        }
    }

}

void SynEdit::incPaintLock()
{
    mPaintLock ++ ;
}

void SynEdit::decPaintLock()
{
    Q_ASSERT(mPaintLock > 0);
    mPaintLock--;
    if (mPaintLock == 0 ) {
        if (mStateFlags.testFlag(SynStateFlag::sfScrollbarChanged))
            updateScrollbars();
        if (mStateFlags.testFlag(SynStateFlag::sfCaretChanged))
            updateCaret();
        if (mStatusChanges!=0)
            doOnStatusChange(mStatusChanges);
    }
}

bool SynEdit::mouseCapture()
{
    return hasMouseTracking();
}

int SynEdit::clientWidth()
{
    return viewport()->size().width();
}

int SynEdit::clientHeight()
{
    return viewport()->size().height();
}

int SynEdit::clientTop()
{
    return 0;
}

int SynEdit::clientLeft()
{
    return 0;
}

QRect SynEdit::clientRect()
{
    return QRect(clientLeft(),clientTop(), clientWidth(), clientHeight());
}

void SynEdit::synFontChanged()
{
    recalcCharExtent();
    sizeOrFontChanged(true);
}

void SynEdit::doOnPaintTransient(SynTransientType TransientType)
{
    doOnPaintTransientEx(TransientType, false);
}

void SynEdit::updateLastCaretX()
{
    mMBCSStepAside = false;
    mLastCaretX = displayX();
}

void SynEdit::ensureCursorPosVisible()
{
    ensureCursorPosVisibleEx(false);
}

void SynEdit::ensureCursorPosVisibleEx(bool ForceToMiddle)
{
    incPaintLock();
    auto action = finally([this]{
        decPaintLock();
    });
    // Make sure X is visible
    int VisibleX = displayX();
    if (VisibleX < leftChar())
        setLeftChar(VisibleX);
    else if (VisibleX >= mCharsInWindow + leftChar() && mCharsInWindow > 0)
        setLeftChar(VisibleX - mCharsInWindow + 1);
    else
        setLeftChar(leftChar());
    // Make sure Y is visible
    int vCaretRow = displayY();
    if (ForceToMiddle) {
        if (vCaretRow < mTopLine || vCaretRow>(mTopLine + (mLinesInWindow - 1)))
            setTopLine( vCaretRow - (mLinesInWindow - 1) / 2);
    } else {
        if (vCaretRow < mTopLine)
          setTopLine(vCaretRow);
        else if (vCaretRow > mTopLine + (mLinesInWindow - 1) && mLinesInWindow > 0)
          setTopLine(vCaretRow - (mLinesInWindow - 1));
        else
          setTopLine(mTopLine);
    }
}

void SynEdit::scrollWindow(int dx, int dy)
{
    int nx = horizontalScrollBar()->value()+dx;
    int ny = verticalScrollBar()->value()+dy;
    nx = std::min(std::max(horizontalScrollBar()->minimum(),nx),horizontalScrollBar()->maximum());
    ny = std::min(std::max(verticalScrollBar()->minimum(),ny),verticalScrollBar()->maximum());
    horizontalScrollBar()->setValue(nx);
    verticalScrollBar()->setValue(ny);

}

void SynEdit::setInternalDisplayXY(const DisplayCoord &aPos)
{
    incPaintLock();
    internalSetCaretXY(displayToBufferPos(aPos));
    decPaintLock();
    updateLastCaretX();
}

void SynEdit::internalSetCaretXY(const BufferCoord &Value)
{
    setCaretXYEx(true, Value);
}

void SynEdit::setStatusChanged(SynStatusChanges changes)
{
    mStatusChanges = mStatusChanges | changes;
    if (mPaintLock == 0)
        doOnStatusChange(mStatusChanges);
}

void SynEdit::doOnStatusChange(SynStatusChanges)
{
    emit statusChanged(mStatusChanges);
    mStatusChanges = SynStatusChange::scNone;
}

void SynEdit::insertBlock(const BufferCoord &BB, const BufferCoord &BE, const QString &ChangeStr, bool AddToUndoList)
{
    setCaretAndSelection(BB, BB, BE);
    setActiveSelectionMode(SynSelectionMode::smColumn);
    setSelTextPrimitiveEx(SynSelectionMode::smColumn, ChangeStr, AddToUndoList);
    setStatusChanged(SynStatusChange::scSelection);
}

void SynEdit::updateScrollbars()
{
    int nMaxScroll;
    int nMin,nMax,nPage,nPos;
    if (mPaintLock!=0) {
        mStateFlags.setFlag(SynStateFlag::sfScrollbarChanged);
    } else {
        mStateFlags.setFlag(SynStateFlag::sfScrollbarChanged,false);
        if (mScrollBars != SynScrollStyle::ssNone) {
            if (mOptions.testFlag(eoHideShowScrollbars)) {
                setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
                setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
            } else {
                setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
                setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
            }
            if (mScrollBars == SynScrollStyle::ssBoth ||  mScrollBars == SynScrollStyle::ssHorizontal) {
                if (mOptions.testFlag(eoScrollPastEol))
                    nMaxScroll = mMaxScrollWidth;
                else
                    nMaxScroll = std::max(mLines->lengthOfLongestLine(), 1);
                if (nMaxScroll <= MAX_SCROLL) {
                    nMin = 1;
                    nMax = nMaxScroll;
                    nPage = mCharsInWindow;
                    nPos = mLeftChar;
                } else {
                    nMin = 0;
                    nMax = MAX_SCROLL;
                    nPage = MulDiv(MAX_SCROLL, mCharsInWindow, nMaxScroll);
                    nPos = MulDiv(MAX_SCROLL, mLeftChar, nMaxScroll);
                }
                horizontalScrollBar()->setMinimum(nMin);
                horizontalScrollBar()->setMaximum(nMax);
                horizontalScrollBar()->setPageStep(nPage);
                horizontalScrollBar()->setValue(nPos);
                horizontalScrollBar()->setSingleStep(1);
            } else
                setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);

            if (mScrollBars == SynScrollStyle::ssBoth ||  mScrollBars == SynScrollStyle::ssVertical) {
                nMaxScroll = displayLineCount();
                if (mOptions.testFlag(eoScrollPastEof))
                    nMaxScroll+=mLinesInWindow - 1;
                if (nMaxScroll <= MAX_SCROLL) {
                    nMin = 1;
                    nMax = std::max(1, nMaxScroll);
                    nPage = mLinesInWindow;
                    nPos = mTopLine;
                } else {
                    nMin = 0;
                    nMax = MAX_SCROLL;
                    nPage = MulDiv(MAX_SCROLL, mLinesInWindow, nMaxScroll);
                    nPos = MulDiv(MAX_SCROLL, mTopLine, nMaxScroll);
                }
                verticalScrollBar()->setMinimum(nMin);
                verticalScrollBar()->setMaximum(nMax);
                verticalScrollBar()->setPageStep(nPage);
                verticalScrollBar()->setValue(nPos);
                verticalScrollBar()->setSingleStep(1);
            } else
                setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        } else {
            setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
            setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        }
    }
}

void SynEdit::updateCaret()
{
    mStateFlags.setFlag(SynStateFlag::sfCaretChanged,false);
    //do nothing, because we will draw cursor in the paintEvent
}

void SynEdit::recalcCharExtent()
{
    SynFontStyle styles[] = {SynFontStyle::fsBold, SynFontStyle::fsItalic, SynFontStyle::fsStrikeOut, SynFontStyle::fsUnderline};
    bool hasStyles[] = {false,false,false,false};
    int size = 4;
    if (mHighlighter && mHighlighter->attributes().count()>0) {
        for (PSynHighlighterAttribute attribute: mHighlighter->attributes().values()) {
            for (int i=0;i<size;i++) {
                if (attribute->styles().testFlag(styles[i]))
                    hasStyles[i] = true;
            }
        }
    } else {
        hasStyles[0] = font().bold();
        hasStyles[1] = font().italic();
        hasStyles[2] = font().strikeOut();
        hasStyles[3] = font().underline();
    }

    mTextHeight  = 0;
    mCharWidth = 0;
    mTextHeight = fontMetrics().lineSpacing();
    mCharWidth = fontMetrics().horizontalAdvance("M");
    if (hasStyles[0]) { // has bold font
        QFont f = font();
        f.setBold(true);
        QFontMetrics fm(f);
        if (fm.lineSpacing()>mTextHeight)
            mTextHeight=fm.lineSpacing();
        if (fm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = fm.horizontalAdvance("M");
    }
    if (hasStyles[1]) { // has strike out font
        QFont f = font();
        f.setItalic(true);
        QFontMetrics fm(f);
        if (fm.lineSpacing()>mTextHeight)
            mTextHeight=fm.lineSpacing();
        if (fm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = fm.horizontalAdvance("M");
    }
    if (hasStyles[2]) { // has strikeout
        QFont f = font();
        f.setStrikeOut(true);
        QFontMetrics fm(f);
        if (fm.lineSpacing()>mTextHeight)
            mTextHeight=fm.lineSpacing();
        if (fm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = fm.horizontalAdvance("M");
    }
    if (hasStyles[3]) { // has underline
        QFont f = font();
        f.setUnderline(true);
        QFontMetrics fm(f);
        if (fm.lineSpacing()>mTextHeight)
            mTextHeight=fm.lineSpacing();
        if (fm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = fm.horizontalAdvance("M");
    }
    mTextHeight += mExtraLineSpacing;
}

QString SynEdit::expandAtWideGlyphs(const QString &S)
{
    QString Result(S.length()*2); // speed improvement
    int  j = 0;
    for (int i=0;i<S.length();i++) {
        int CountOfAvgGlyphs = ceil(fontMetrics().horizontalAdvance(S[i])/(double)mCharWidth);
        if (j+CountOfAvgGlyphs>=Result.length())
            Result.resize(Result.length()+128);
        // insert CountOfAvgGlyphs filling chars
        while (CountOfAvgGlyphs>1) {
            Result[j]=QChar(0xE000);
            j++;
            CountOfAvgGlyphs--;
        }
        Result[j]=S[i];
        j++;
    }
    Result.resize(j);
}

void SynEdit::updateModifiedStatus()
{
    setModified(!mUndoList->initialState());
}

int SynEdit::scanFrom(int Index)
{
    SynRangeState iRange;
    int Result = Index;
    if (Result >= mLines->count())
        return Result;

    if (Result == 0) {
        mHighlighter->resetState();
    } else {
        mHighlighter->setState(mLines->ranges(Result-1),
                               mLines->braceLevels(Result-1),
                               mLines->bracketLevels(Result-1),
                               mLines->parenthesisLevels(Result-1));
    }
    do {
        mHighlighter->setLine(mLines->getString(Result), Result);
        mHighlighter->nextToEol();
        iRange = mHighlighter->getRangeState();
        {
            if (mLines->ranges(Result).state == iRange.state)
                return Result;// avoid the final Decrement
        }
        mLines->setRange(Result,iRange);
        mLines->setParenthesisLevel(Result,mHighlighter->getParenthesisLevel());
        mLines->setBraceLevel(Result,mHighlighter->getBraceLevel());
        mLines->setBracketLevel(Result,mHighlighter->getBracketLevel());
        Result ++ ;
    } while (Result < mLines->count());
    Result--;
    return Result;
}

void SynEdit::uncollapse(PSynEditFoldRange FoldRange)
{
    FoldRange->linesCollapsed = 0;
    FoldRange->collapsed = false;

    // Redraw the collapsed line
    invalidateLines(FoldRange->fromLine, INT_MAX);

    // Redraw fold mark
    invalidateGutterLines(FoldRange->fromLine, INT_MAX);
}

void SynEdit::foldOnListInserted(int Line, int Count)
{
    // Delete collapsed inside selection
    for (int i = mAllFoldRanges.count()-1;i>=0;i--) {
        PSynEditFoldRange range = mAllFoldRanges[i];
        if (range->collapsed || range->parentCollapsed()){
            if (range->fromLine == Line - 1) // insertion starts at fold line
                uncollapse(range);
            else if (range->fromLine >= Line) // insertion of count lines above FromLine
                range->move(Count);
        }
    }
}

void SynEdit::foldOnListDeleted(int Line, int Count)
{
    // Delete collapsed inside selection
    for (int i = mAllFoldRanges.count()-1;i>=0;i--) {
        PSynEditFoldRange range = mAllFoldRanges[i];
        if (range->collapsed || range->parentCollapsed()){
            if (range->fromLine == Line && Count == 1)  // open up because we are messing with the starting line
                uncollapse(range);
            else if (range->fromLine >= Line - 1 && range->fromLine < Line + Count) // delete inside affectec area
                mAllFoldRanges.remove(i);
            else if (range->fromLine >= Line + Count) // Move after affected area
                range->move(-Count);
        }
    }

}

void SynEdit::foldOnListCleared()
{
    mAllFoldRanges.clear();
}

void SynEdit::rescan()
{
    if (!mUseCodeFolding)
        return;
    rescanForFoldRanges();
    invalidateGutter();
}

static void null_deleter(SynEditFoldRanges *) {}

void SynEdit::rescanForFoldRanges()
{
    // Delete all uncollapsed folds
    for (int i=mAllFoldRanges.count()-1;i>=0;i--) {
        PSynEditFoldRange range =mAllFoldRanges[i];
        if (!range->collapsed && !range->parentCollapsed())
            mAllFoldRanges.remove(i);
    }

    // Did we leave any collapsed folds and are we viewing a code file?
    if (mAllFoldRanges.count() > 0) {
        // Add folds to a separate list
        PSynEditFoldRanges TemporaryAllFoldRanges = std::make_shared<SynEditFoldRanges>();
        scanForFoldRanges(TemporaryAllFoldRanges);

        // Combine new with old folds, preserve parent order
        for (int i = 0; i< TemporaryAllFoldRanges->count();i++) {
            for (int j = 0; j< mAllFoldRanges.count() - 1;j++) {
                if (TemporaryAllFoldRanges->range(i)->fromLine < mAllFoldRanges[j]->fromLine) {
                    mAllFoldRanges.insert(j, TemporaryAllFoldRanges->range(i));
                    break;
                }
                // If we can't prepend #i anywhere, just dump it at the end
                if (j == mAllFoldRanges.count() - 1)
                    mAllFoldRanges.add(TemporaryAllFoldRanges->range(i));
            }
        }

    } else {
        // We ended up with no folds after deleting, just pass standard data...
        PSynEditFoldRanges temp(&mAllFoldRanges, null_deleter);
        scanForFoldRanges(temp);
    }
}

void SynEdit::scanForFoldRanges(PSynEditFoldRanges TopFoldRanges)
{
      // Recursively scan for folds (all types)
      for (int i= 0 ; i< mCodeFolding.foldRegions.count() ; i++ ) {
          findSubFoldRange(TopFoldRanges, i,PSynEditFoldRange());
      }
}

//this func should only be used in findSubFoldRange
int SynEdit::lineHasChar(int Line, int startChar, QChar character, const QString& highlighterAttrName) {
    QString CurLine = mLines->getString(Line);
    if (!mHighlighter){
        for (int i=startChar; i<CurLine.length();i++) {
            if (CurLine[i]==character) {
                return i;
            }
        }

    } else {
        /*
        mHighlighter->setState(mLines->ranges(Line),
                               mLines->braceLevels(Line),
                               mLines->bracketLevels(Line),
                               mLines->parenthesisLevels(Line));
        mHighlighter->setLine(CurLine,Line);
        */
        QString token;
        while (!mHighlighter->eol()) {
            token = mHighlighter->getToken();
            PSynHighlighterAttribute attr = mHighlighter->getTokenAttribute();
            if (token == character && attr->name()==highlighterAttrName)
                return mHighlighter->getTokenPos();
            mHighlighter->next();
        }
    }
    return -1;
}

void SynEdit::findSubFoldRange(PSynEditFoldRanges TopFoldRanges, int FoldIndex, PSynEditFoldRange Parent)
{
    PSynEditFoldRange  CollapsedFold;
    PSynEditFoldRanges ParentFoldRanges;
    ParentFoldRanges = TopFoldRanges;
    int Line = 0;
    QString CurLine;

    if (!mHighlighter)
        return;
    while (Line < mLines->count()) { // index is valid for LinesToScan and fLines
        // If there is a collapsed fold over here, skip it
        CollapsedFold = collapsedFoldStartAtLine(Line + 1); // only collapsed folds remain
        if (CollapsedFold) {
          Line = CollapsedFold->toLine;
          continue;
        }

        // Find an opening character on this line
        CurLine = mLines->getString(Line);

        mHighlighter->setState(mLines->ranges(Line),
                               mLines->braceLevels(Line),
                               mLines->bracketLevels(Line),
                               mLines->parenthesisLevels(Line));
        mHighlighter->setLine(CurLine,Line);

        QString token;
        int pos;
        while (!mHighlighter->eol()) {
            token = mHighlighter->getToken();
            pos = mHighlighter->getTokenPos()+token.length();
            PSynHighlighterAttribute attr = mHighlighter->getTokenAttribute();
            // We've found a starting character and it have proper highlighting (ignore stuff inside comments...)
            if (token == mCodeFolding.foldRegions.get(FoldIndex)->openSymbol && attr->name()==mCodeFolding.foldRegions.get(FoldIndex)->highlight) {
                // And ignore lines with both opening and closing chars in them
                if (lineHasChar(Line,pos,mCodeFolding.foldRegions.get(FoldIndex)->closeSymbol,
                                mCodeFolding.foldRegions.get(FoldIndex)->highlight)<0) {
                    // Add it to the top list of folds
                    Parent = ParentFoldRanges->addByParts(
                      Parent,
                      TopFoldRanges,
                      Line + 1,
                      mCodeFolding.foldRegions.get(FoldIndex),
                      Line + 1);
                    ParentFoldRanges = Parent->subFoldRanges;

                    // Skip until a newline
                    break;
                }

            } else if (token == mCodeFolding.foldRegions.get(FoldIndex)->closeSymbol && attr->name()==mCodeFolding.foldRegions.get(FoldIndex)->highlight) {
                // And ignore lines with both opening and closing chars in them
                if (lineHasChar(Line,pos,mCodeFolding.foldRegions.get(FoldIndex)->openSymbol,
                                mCodeFolding.foldRegions.get(FoldIndex)->highlight)<0) {
                    // Stop the recursion if we find a closing char, and return to our parent
                    if (Parent) {
                      Parent->toLine = Line + 1;
                      Parent = Parent->parent;
                    }

                    // Skip until a newline
                    break;
                }
            }
            mHighlighter->next();
        }
        Line++;
    }
}

PSynEditFoldRange SynEdit::collapsedFoldStartAtLine(int Line)
{
    for (int i = 0; i< mAllFoldRanges.count() - 1; i++ ) {
        if (mAllFoldRanges[i]->fromLine == Line && mAllFoldRanges[i]->collapsed) {
            return mAllFoldRanges[i];
        } else if (mAllFoldRanges[i]->fromLine > Line) {
            break; // sorted by line. don't bother scanning further
        }
    }
    return PSynEditFoldRange();
}

void SynEdit::setSelTextPrimitiveEx(SynSelectionMode PasteMode, const QString &Value, bool AddToUndoList)
{
    //todo
}

void SynEdit::doOnPaintTransientEx(SynTransientType TransientType, bool Lock)
{
    //todo: we can't draw to canvas outside paintEvent
}

void SynEdit::initializeCaret()
{
    //todo:
}

void SynEdit::sizeOrFontChanged(bool bFont)
{
    if (mCharWidth != 0) {
        mCharsInWindow = std::max(clientWidth() - mGutterWidth - 2, 0) / mCharWidth;
        mLinesInWindow = clientHeight() / mTextHeight;
        if (bFont) {
            if (mGutter.showLineNumbers())
                gutterChanged();
            else
                updateScrollbars();
            initializeCaret();
            mStateFlags.setFlag(SynStateFlag::sfCaretChanged,false);
            invalidate();
        } else
            updateScrollbars();
        mStateFlags.setFlag(SynStateFlag::sfScrollbarChanged,false);
        if (!mOptions.testFlag(SynEditorOption::eoScrollPastEol))
            setLeftChar(mLeftChar);
        if (!mOptions.testFlag(SynEditorOption::eoScrollPastEof))
            setTopLine(mTopLine);
    }
}

void SynEdit::doChange()
{
    emit Changed();
}

int SynEdit::tabWidth() const
{
    return mTabWidth;
}

void SynEdit::paintEvent(QPaintEvent *event)
{
    if (mPainterLock>0)
        return;
    if (mPainting)
        return;
    mPainting = true;
    QRect rcClip, rcDraw;
    int nL1, nL2, nC1, nC2;
    // Get the invalidated rect. Compute the invalid area in lines / columns.
    rcClip = event->rect();
    // columns
    nC1 = mLeftChar;
    if (rcClip.left() > mGutterWidth + 2 + clientLeft())
        nC1 += (rcClip.left() - mGutterWidth - 2 - clientLeft()) % mCharWidth;
    nC2 = mLeftChar +
      (rcClip.right() - mGutterWidth - 2 - clientLeft() + mCharWidth - 1) % mCharWidth;
    // lines
    nL1 = MinMax(mTopLine + rcClip.top() % mTextHeight, mTopLine, displayLineCount());
    nL2 = MinMax(mTopLine + (rcClip.bottom() + mTextHeight - 1) % mTextHeight, 1, displayLineCount());

    // Now paint everything while the caret is hidden.
    hideCaret();
    auto action = finally([this] {
        updateCaret();
        mPainting = false;
    });

    // First paint paint the text area if it was (partly) invalidated.
    if (rcClip.right() > mGutterWidth + clientLeft()) {
        rcDraw = rcClip;
        rcDraw.setLeft( std::max(rcDraw.left(), mGutterWidth + clientLeft()));
        paintTextLines(rcDraw, nL1, nL2, nC1, nC2);
    }

    // Then the gutter area if it was (partly) invalidated.
    if (rcClip.left() < clientLeft() + mGutterWidth) {
        rcDraw = rcClip;
        rcDraw.setRight(clientLeft() + mGutterWidth);
        paintGutter(rcDraw, nL1, nL2);
    }

      //PluginsAfterPaint(Canvas, rcClip, nL1, nL2);
      // If there is a custom paint handler call it.
    doOnPaint();
    doOnPaintTransient(SynTransientType:: ttAfter);
}

int SynEdit::maxScrollWidth() const
{
    return mMaxScrollWidth;
}

void SynEdit::setMaxScrollWidth(int Value)
{
    Value = MinMax(Value, 1, INT_MAX - 1);
    if (mMaxScrollWidth != Value) {
        mMaxScrollWidth = Value;
        if (mOptions.testFlag(SynEditorOption::eoScrollPastEol))
                updateScrollbars();
    }
}

bool SynEdit::modified() const
{
    return mModified;
}

void SynEdit::setModified(bool Value)
{
    if (Value)
        mLastModifyTime = QDateTime::currentDateTime();
    if (Value != mModified) {
        mModified = Value;
        if (mOptions.testFlag(SynEditorOption::eoGroupUndo) && (!Value) && mUndoList->CanUndo())
            mUndoList->AddGroupBreak();
        mUndoList->setInitialState(!Value);
        statusChanged(SynStatusChange::scModified);
    }
}

int SynEdit::gutterWidth() const
{
    return mGutterWidth;
}

void SynEdit::setGutterWidth(int Value)
{
    Value = std::max(Value, 0);
    if (mGutterWidth != Value) {
        mGutterWidth = Value;
        mTextOffset = mGutterWidth + 2 - (mLeftChar - 1) * mCharWidth;
        sizeOrFontChanged(false);
    }
}

int SynEdit::charWidth() const
{
    return mCharWidth;
}

int SynEdit::charsInWindow() const
{
    return mCharsInWindow;
}

void SynEdit::bookMarkOptionsChanged()
{
    invalidateGutter();
}

void SynEdit::linesChanged()
{
    SynSelectionMode vOldMode;
    mStateFlags.setFlag(SynStateFlag::sfLinesChanging, false);
    if (mUseCodeFolding)
        rescan();

    updateScrollbars();
    vOldMode = mActiveSelectionMode;
    setBlockBegin(caretXY());
    mActiveSelectionMode = vOldMode;
    invalidateRect(mInvalidateRect);
    mInvalidateRect = {0,0,0,0};

    if (mGutter.showLineNumbers() && (mGutter.autoSize()))
        mGutter.autoSizeDigitCount(mLines->count());
    if (!mOptions.testFlag(SynEditorOption::eoScrollPastEof))
        setTopLine(mTopLine);
}

void SynEdit::linesChanging()
{
    mStateFlags.setFlag(SynStateFlag::sfLinesChanging);
}

void SynEdit::linesCleared()
{
    if (mUseCodeFolding)
        foldOnListCleared();
    clearUndo();
    // invalidate the *whole* client area
    mInvalidateRect={0,0,0,0};
    invalidate();
    // set caret and selected block to start of text
    setCaretXY({1,1});
    // scroll to start of text
    setTopLine(1);
    setLeftChar(1);
    mStatusChanges.setFlag(SynStatusChange::scAll);
}

void SynEdit::linesDeleted(int index, int count)
{
    if (mUseCodeFolding)
        foldOnListDeleted(index + 1, count);
    if (mHighlighter && mLines->count() > 0)
        scanFrom(index);
    invalidateLines(index + 1, INT_MAX);
    invalidateGutterLines(index + 1, INT_MAX);
}

void SynEdit::linesInserted(int index, int count)
{
    if (mUseCodeFolding)
        foldOnListInserted(index + 1, count);
    if (mHighlighter && mLines->count() > 0) {
        int vLastScan = index;
        do {
            vLastScan = scanFrom(vLastScan);
            vLastScan++;
        } while (vLastScan < index + count) ;
    }
    invalidateLines(index + 1, INT_MAX);
    invalidateGutterLines(index + 1, INT_MAX);
    if (mOptions.setFlag(SynEditorOption::eoAutoSizeMaxScrollWidth)) {
        int L = mLines->expandedStringLength(index);
        if (L > mMaxScrollWidth)
          setMaxScrollWidth(L);
    }
}

void SynEdit::linesPutted(int index, int)
{
    int vEndLine = index + 1;
    if (mHighlighter) {
        vEndLine = std::max(vEndLine, scanFrom(index) + 1);
        // If this editor is chained then the real owner of text buffer will probably
        // have already parsed the changes, so ScanFrom will return immediately.
        if (mLines != mOrigLines)
            vEndLine = INT_MAX;
    }
    invalidateLines(index + 1, vEndLine);

    if (mOptions.setFlag(SynEditorOption::eoAutoSizeMaxScrollWidth)) {
        int L = mLines->expandedStringLength(index);
        if (L > mMaxScrollWidth)
          setMaxScrollWidth(L);
    }
}

void SynEdit::undoAdded()
{
    updateModifiedStatus();

    // we have to clear the redo information, since adding undo info removes
    // the necessary context to undo earlier edit actions
    if (! mUndoList->insideRedo() &&
            mUndoList->PeekItem() && (mUndoList->PeekItem()->changeReason()!=SynChangeReason::crGroupBreak))
        mRedoList->Clear();
    if (mUndoList->blockCount() == 0 )
        doChange();
}

SynSelectionMode SynEdit::activeSelectionMode() const
{
    return mActiveSelectionMode;
}

void SynEdit::setActiveSelectionMode(const SynSelectionMode &Value)
{
    if (mActiveSelectionMode != Value) {
        if (selAvail())
            invalidateSelection();
        mActiveSelectionMode = Value;
        if (selAvail())
            invalidateSelection();
        setStatusChanged(SynStatusChange::scSelection);
    }
}

BufferCoord SynEdit::blockEnd() const
{
    return mBlockEnd;
}

void SynEdit::setBlockEnd(BufferCoord Value)
{
    setActiveSelectionMode(mSelectionMode);
    if (!mOptions.testFlag(eoNoSelection)) {
        if (mOptions.testFlag(eoScrollPastEol))
            Value.Char = MinMax(Value.Char, 1, mMaxScrollWidth + 1);
        else
            Value.Char = std::max(Value.Char, 1);
        Value.Line = MinMax(Value.Line, 1, mLines->count());
        if (mActiveSelectionMode == SynSelectionMode::smNormal) {
          if (Value.Line >= 1 && Value.Line <= mLines->count())
              Value.Char = std::min(Value.Char, mLines->getString(Value.Line - 1).length() + 1);
          else
              Value.Char = 1;
        }
        if (Value.Char != mBlockEnd.Char || Value.Line != mBlockEnd.Line) {
            if (Value.Char != mBlockEnd.Char || Value.Line != mBlockEnd.Line) {
                if (mActiveSelectionMode == SynSelectionMode::smColumn && Value.Char != mBlockEnd.Char) {
                    invalidateLines(
                                std::min(mBlockBegin.Line, std::min(mBlockEnd.Line, Value.Line)),
                                std::max(mBlockBegin.Line, std::max(mBlockEnd.Line, Value.Line)));
                    mBlockEnd = Value;
                } else {
                    int nLine = mBlockEnd.Line;
                    mBlockEnd = Value;
                    if (mActiveSelectionMode != SynSelectionMode::smColumn || mBlockBegin.Char != mBlockEnd.Char)
                        invalidateLines(nLine, mBlockEnd.Line);
                }
                setStatusChanged(SynStatusChange::scSelection);
            }
        }
    }
}

BufferCoord SynEdit::blockBegin() const
{
    return mBlockBegin;
}

void SynEdit::setBlockBegin(BufferCoord value)
{
    int nInval1, nInval2;
    bool SelChanged;
    setActiveSelectionMode(mSelectionMode);
    if (mOptions.testFlag(SynEditorOption::eoScrollPastEol))
        value.Char = MinMax(value.Char, 1, mMaxScrollWidth + 1);
    else
        value.Char = std::max(value.Char, 1);
    value.Line = MinMax(value.Line, 1, mLines->count());
    if (mActiveSelectionMode == SynSelectionMode::smNormal) {
        if (value.Line >= 1 && value.Line <= mLines->count())
            value.Char = std::min(value.Char, mLines->getString(value.Line - 1).length() + 1);
        else
            value.Char = 1;
    }
    if (selAvail()) {
        if (mBlockBegin.Line < mBlockEnd.Line) {
            nInval1 = std::min(value.Line, mBlockBegin.Line);
            nInval2 = std::max(value.Line, mBlockEnd.Line);
        } else {
            nInval1 = std::min(value.Line, mBlockEnd.Line);
            nInval2 = std::max(value.Line, mBlockBegin.Line);
        };
        mBlockBegin = value;
        mBlockEnd = value;
        invalidateLines(nInval1, nInval2);
        SelChanged = true;
    } else {
        SelChanged =
          (mBlockBegin.Char != value.Char) || (mBlockBegin.Line != value.Line) ||
          (mBlockEnd.Char != value.Char) || (mBlockEnd.Line != value.Line);
        mBlockBegin = value;
        mBlockEnd = value;
    }
    if (SelChanged)
        setStatusChanged(SynStatusChange::scSelection);
}

int SynEdit::leftChar() const
{
    return mLeftChar;
}

void SynEdit::setLeftChar(int Value)
{
    int MaxVal;
    int iDelta;
    //QRect iTextArea;
    if (mOptions.testFlag(SynEditorOption::eoScrollPastEol)) {
        if (mOptions.testFlag(SynEditorOption::eoAutoSizeMaxScrollWidth))
            MaxVal = INT_MAX - mCharsInWindow;
        else
            MaxVal = mMaxScrollWidth - mCharsInWindow + 1;
    } else {
        MaxVal = mLines->lengthOfLongestLine();
        if (MaxVal > mCharsInWindow)
            MaxVal = MaxVal - mCharsInWindow + 1;
        else
            MaxVal = 1;
    }
    Value = MinMax(Value, 1, MaxVal);
    if (Value != mLeftChar) {
        iDelta = mLeftChar - Value;
        mLeftChar = Value;
        mTextOffset = mGutterWidth + 2 - (mLeftChar - 1) * mCharWidth;
        if (std::abs(iDelta) < mCharsInWindow) {
//          iTextArea = clientRect();
//          iTextArea.setLeft(iTextArea.left() + mGutterWidth + 2);
          scrollWindow(iDelta * mCharWidth, 0);
        } else {
            invalidateLines(-1, -1);
        }
        if ( (mOptions & (SynEditorOption::eoAutoSizeMaxScrollWidth | SynEditorOption::eoScrollPastEol))
             &&
          (mMaxScrollWidth < mLeftChar + mCharsInWindow)) {
            setMaxScrollWidth(mLeftChar + mCharsInWindow);
        } else
          updateScrollbars();
        setStatusChanged(SynStatusChange::scLeftChar);
    }

}

int SynEdit::linesInWindow() const
{
    return mLinesInWindow;
}

int SynEdit::topLine() const
{
    return mTopLine;
}

void SynEdit::setTopLine(int Value)
{
    if (mOptions.testFlag(SynEditorOption::eoScrollPastEof))
        Value = std::min(Value, displayLineCount());
    else
        Value = std::min(Value, displayLineCount() - mLinesInWindow + 1);
    Value = std::max(Value, 1);
    if (Value != mTopLine) {
        int Delta = mTopLine - Value;
        mTopLine = Value;
        if (mPainterLock == 0) {
            if (std::abs(Delta) < mLinesInWindow) {
                scrollWindow(0, mTextHeight * Delta);
            } else {
                invalidate();
            }
        }
        updateScrollbars();
        setStatusChanged(SynStatusChange::scTopLine);
    }
}

void SynEdit::redoAdded()
{
    updateModifiedStatus();

    if (mRedoList->blockCount() == 0 )
        doChange();
}

void SynEdit::gutterChanged()
{
    if (mGutter.showLineNumbers() && mGutter.autoSize())
        mGutter.autoSizeDigitCount(mLines->count());
    int nW;
    if (mGutter.useFontStyle()) {
        QFontMetrics fm=QFontMetrics(mGutter.font());
        nW = mGutter.realGutterWidth(fm.averageCharWidth());
    } else {
        nW = mGutter.realGutterWidth(mCharWidth);
    }
    if (nW == mGutterWidth)
        invalidateGutter();
    else
        setGutterWidth(nW);
}

void SynEdit::scrollTimerHandler()
{
    QPoint iMousePos;
    DisplayCoord C;
    int X, Y;

    iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    C = pixelsToRowColumn(iMousePos.x(), iMousePos.y());
    C.Row = MinMax(C.Row, 1, displayLineCount());
    if (mScrollDeltaX != 0) {
        setLeftChar(leftChar() + mScrollDeltaX);
        X = leftChar();
        if (mScrollDeltaX > 0) // scrolling right?
            X+=charsInWindow();
        C.Column = X;
    }
    if (mScrollDeltaY != 0) {
        if (QApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier))
          setTopLine(mTopLine + mScrollDeltaY * mLinesInWindow);
        else
          setTopLine(mTopLine + mScrollDeltaY);
        Y = mTopLine;
        if (mScrollDeltaY > 0)  // scrolling down?
            Y+=mLinesInWindow - 1;
        C.Row = MinMax(Y, 1, displayLineCount());
    }
    BufferCoord vCaret = displayToBufferPos(C);
    if ((caretX() != vCaret.Char) || (caretY() != vCaret.Line)) {
        // changes to line / column in one go
        incPaintLock();
        auto action = finally([this]{
            decPaintLock();
        });
        internalSetCaretXY(vCaret);

          // if MouseCapture is True we're changing selection. otherwise we're dragging
        if (mouseCapture())
            setBlockEnd(caretXY());
    }
    computeScroll(iMousePos.x(), iMousePos.y());
}
