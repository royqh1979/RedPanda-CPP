#include "SynEdit.h"
#include <QApplication>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>
#include <QScrollBar>
#include <QPaintEvent>
#include <QPainter>
#include <QTimerEvent>
#include "highlighter/base.h"
#include "Constants.h"
#include "TextPainter.h"
#include <QDebug>
#include <QPaintEvent>

SynEdit::SynEdit(QWidget *parent) : QAbstractScrollArea(parent)
{
    qDebug()<<"init SynEdit:";
    mPaintLock = 0;
    mPainterLock = 0;
    mPainting = false;
    mLines = std::make_shared<SynEditStringList>(this);
    qDebug()<<"init SynEdit: 1";
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
    qDebug()<<"init SynEdit: 2";

#ifdef Q_OS_WIN
    mFontDummy = QFont("Consolas",12);
#elif Q_OS_LINUX
    mFontDummy = QFont("terminal",14);
#else
#error "Not supported!"
#endif
    mFontDummy.setStyleStrategy(QFont::PreferAntialias);
    setFont(mFontDummy);
    qDebug()<<"init SynEdit:3";

    mUndoList = std::make_shared<SynEditUndoList>();
    mUndoList->connect(mUndoList.get(), &SynEditUndoList::addedUndo, this, &SynEdit::undoAdded);
    mOrigUndoList = mUndoList;
    mRedoList = std::make_shared<SynEditUndoList>();
    mRedoList->connect(mRedoList.get(), &SynEditUndoList::addedUndo, this, &SynEdit::redoAdded);
    mOrigRedoList = mRedoList;
    qDebug()<<"init SynEdit: 4";

    mCaretColor = QColorConstants::Red;
    mActiveLineColor = QColorConstants::Svg::lightblue;
    mSelectedBackground = QColorConstants::Svg::lightgray;
    mSelectedForeground = palette().color(QPalette::Text);

    mBookMarkOpt.connect(&mBookMarkOpt, &SynBookMarkOpt::changed, this, &SynEdit::bookMarkOptionsChanged);
    //  fRightEdge has to be set before FontChanged is called for the first time
    mRightEdge = 80;
    qDebug()<<"init SynEdit: 5";

    mGutter.setRightOffset(21);
    mGutter.connect(&mGutter, &SynGutter::changed, this, &SynEdit::gutterChanged);
    mGutterWidth = mGutter.width();
    //ControlStyle := ControlStyle + [csOpaque, csSetCaption, csNeedsBorderPaint];
    //Height := 150;
    //Width := 200;
    this->setCursor(Qt::CursorShape::IBeamCursor);
    //TabStop := True;
    mInserting = true;
    mMaxScrollWidth = 1024;
    mScrollBars = SynScrollStyle::ssBoth;
    mExtraLineSpacing = 0;
    qDebug()<<"init SynEdit: 6";

    this->setFrameShape(QFrame::Panel);
    this->setFrameShadow(QFrame::Sunken);
    this->setLineWidth(1);
    mInsertCaret = SynEditCaretType::ctVerticalLine;
    mOverwriteCaret = SynEditCaretType::ctBlock;
    mSelectionMode = SynSelectionMode::smNormal;
    mActiveSelectionMode = SynSelectionMode::smNormal;
    qDebug()<<"init SynEdit: 7";

    //stop qt to auto fill background
    setAutoFillBackground(false);
    //fFocusList := TList.Create;
    //fKbdHandler := TSynEditKbdHandler.Create;
    //fMarkList.OnChange := MarkListChange;
    qDebug()<<"init SynEdit: 7-1";
    setDefaultKeystrokes();
    qDebug()<<"init SynEdit: 7-2";
    mRightEdgeColor = QColorConstants::Svg::silver;
    qDebug()<<"init SynEdit: 8";

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
            eoShowScrollHint | eoSmartTabs | eoTabsToSpaces |
            eoSmartTabDelete| eoGroupUndo | eoKeepCaretX;
    qDebug()<<"init SynEdit: 9";

    mScrollTimer = new QTimer(this);
    mScrollTimer->setInterval(100);
    connect(mScrollTimer, &QTimer::timeout,this, &SynEdit::scrollTimerHandler);

    mScrollHintColor = QColorConstants::Yellow;
    mScrollHintFormat = SynScrollHintFormat::shfTopLineOnly;

    mContentImage = std::make_shared<QImage>(clientWidth(),clientHeight(),QImage::Format_ARGB32);

    mUseCodeFolding = true;
    m_blinkTimerId = 0;
    m_blinkStatus = 0;
    qDebug()<<"init SynEdit: 10";

    synFontChanged();
    qDebug()<<"init SynEdit: done";

    showCaret();

    connect(horizontalScrollBar(),&QScrollBar::valueChanged,
            this, &SynEdit::doScrolled);
    connect(verticalScrollBar(),&QScrollBar::valueChanged,
            this, &SynEdit::doScrolled);
}

int SynEdit::displayLineCount()
{
    if (mLines->empty()) {
        return 0;
    }
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
    qDebug()<<"new Value"<<value.Line<<value.Char;
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
            nMaxX = mLines->getString(value.Line-1).length()+1;
    }
    if ((value.Char > nMaxX) && (! (mOptions.testFlag(SynEditorOption::eoScrollPastEol)) ) )
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
            invalidateLine(mCaretY);
        }
        if (mCaretY != value.Line) {
            int oldCaretY = mCaretY;
            mCaretY = value.Line;
            if (mActiveLineColor.isValid()) {
                invalidateLine(mCaretY);
                invalidateLine(oldCaretY);
            }
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
        rcInval = QRect(0, 0, mGutterWidth, clientHeight());
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
            rcInval = {0, mTextHeight * (FirstLine - mTopLine),
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

QPoint SynEdit::RowColumnToPixels(const DisplayCoord &coord)
{
    QPoint result;
    result.setX((coord.Column - 1) * mCharWidth + textOffset());
    result.setY((coord.Row - mTopLine) * mTextHeight);
    return result;
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

        while (x < p.Column && i<s.length()) {
            if (i < l && s[i] == '\t')
                x += mTabWidth - (x % mTabWidth);
            else
                x += charColumns(s[i]);
            i++;
        }
        if (i==0) {
            i=1;
        }
        Result.Char = i;
    }
    return Result;
}

int SynEdit::charToColumn(int aLine, int aChar)
{
    if (aLine <= mLines->count()) {
        QString s = mLines->getString(aLine - 1);
        int l = s.length();
        int x = 0;
        int len = std::min(aChar-1,s.length());
        for (int i=0;i<len;i++) {
            if (i<=l && s[i] == '\t')
                x+=mTabWidth - (x % mTabWidth);
            else
                x+=charColumns(s[i]);
        }
        return x+1;
    }
    qDebug()<<"Line outof range"<<aLine<<aChar;
    throw BaseError(SynEdit::tr("Line %1 is out of range").arg(aLine));
}

int SynEdit::stringColumns(const QString &line, int colsBefore)
{
    int columns = colsBefore;
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
    return columns-colsBefore;
}

int SynEdit::getLineIndent(const QString &line)
{
    int indents = 0;
    for (QChar ch:line) {
        switch(ch.digitValue()) {
        case '\t':
            indents+=mTabWidth;
            break;
        case ' ':
            indents+=1;
            break;
        default:
            return indents;
        }
    }
    return indents;
}

int SynEdit::rowToLine(int aRow)
{
    if (mUseCodeFolding)
        return foldRowToLine(aRow);
    else
        return aRow;
    //return displayToBufferPos({1, aRow}).Line;
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
        rcInval = { mGutterWidth,
                    mTextHeight * (Line - mTopLine),
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
    if (ch == ' ')
        return 1;
    //return std::ceil((int)(fontMetrics().horizontalAdvance(ch) * dpiFactor()) / (double)mCharWidth);
    return std::ceil((int)(fontMetrics().horizontalAdvance(ch)) / (double)mCharWidth);
}

double SynEdit::dpiFactor()
{
    return fontMetrics().fontDpi() / 96.0;
}

void SynEdit::showCaret()
{
    if (m_blinkTimerId==0)
        m_blinkTimerId = startTimer(500);
}

void SynEdit::hideCaret()
{
    if (m_blinkTimerId!=0) {
        killTimer(m_blinkTimerId);
        m_blinkTimerId = 0;
        m_blinkStatus = 0;
        updateCaret();
    }
}

bool SynEdit::IsPointInSelection(const BufferCoord &Value)
{
    BufferCoord ptBegin = mBlockBegin;
    BufferCoord ptEnd = mBlockEnd;
    if ((Value.Line >= ptBegin.Line) && (Value.Line <= ptEnd.Line) &&
            ((ptBegin.Line != ptEnd.Line) || (ptBegin.Char != ptEnd.Char))) {
        if (mActiveSelectionMode == SynSelectionMode::smLine)
            return true;
        else if (mActiveSelectionMode == SynSelectionMode::smColumn) {
            if (ptBegin.Char > ptEnd.Char)
                return (Value.Char >= ptEnd.Char) && (Value.Char < ptBegin.Char);
            else if (ptBegin.Char < ptEnd.Char)
                return (Value.Char >= ptBegin.Char) && (Value.Char < ptEnd.Char);
            else
                return false;
        } else
            return ((Value.Line > ptBegin.Line) || (Value.Char >= ptBegin.Char)) &&
      ((Value.Line < ptEnd.Line) || (Value.Char < ptEnd.Char));
    } else
        return false;
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
        mScrollDeltaX = (X - iScrollBounds.left()) / mCharWidth - 1;
    else if (X >= iScrollBounds.right())
        mScrollDeltaX = (X - iScrollBounds.right()) / mCharWidth + 1;
    else
        mScrollDeltaX = 0;

    if (Y < iScrollBounds.top())
        mScrollDeltaY = (Y - iScrollBounds.top()) / mTextHeight - 1;
    else if (Y >= iScrollBounds.bottom())
        mScrollDeltaY = (Y - iScrollBounds.bottom()) / mTextHeight + 1;
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
    return QRect(0,0, clientWidth(), clientHeight());
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
//    int nx = horizontalScrollBar()->value()+dx;
//    int ny = verticalScrollBar()->value()+dy;
//    nx = std::min(std::max(horizontalScrollBar()->minimum(),nx),horizontalScrollBar()->maximum());
//    ny = std::min(std::max(verticalScrollBar()->minimum(),ny),verticalScrollBar()->maximum());
//    horizontalScrollBar()->setValue(nx);
//    verticalScrollBar()->setValue(ny);

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
                nMaxScroll = std::max(mLines->lengthOfLongestLine(), 1);
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
    DisplayCoord coord = displayXY();
    QPoint caretPos = RowColumnToPixels(coord);
    int caretWidth=mCharWidth;
    if (mCaretY <= mLines->count() && mCaretX <= mLines->getString(mCaretY-1).length()) {
        caretWidth = charColumns(mLines->getString(mCaretY-1)[mCaretX-1])*mCharWidth;
    }
    QRect rcCaret(caretPos.x(),caretPos.y(),caretWidth,
                  mTextHeight);
    invalidateRect(rcCaret);
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
    //mCharWidth = mCharWidth * dpiFactor();
    //mTextHeight = mTextHeight * dpiFactor();
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

int SynEdit::scanRanges()
{
    if (mHighlighter && !mLines->empty()) {
        mHighlighter->resetState();
        for (int i =0;i<mLines->count();i++) {
            mHighlighter->setLine(mLines->getString(i), i);
            qDebug()<<i<<mLines->getString(i);
            mHighlighter->nextToEol();
            mLines->setRange(i, mHighlighter->getRangeState());
            mLines->setParenthesisLevel(i, mHighlighter->getParenthesisLevel());
            mLines->setBracketLevel(i, mHighlighter->getBracketLevel());
            mLines->setBraceLevel(i, mHighlighter->getBraceLevel());
        }
        qDebug()<<"finished.";
    }
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
    PSynEditFoldRanges parentFoldRanges = TopFoldRanges;
      // Recursively scan for folds (all types)
      for (int i= 0 ; i< mCodeFolding.foldRegions.count() ; i++ ) {
          findSubFoldRange(TopFoldRanges, i,parentFoldRanges,PSynEditFoldRange());
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

void SynEdit::findSubFoldRange(PSynEditFoldRanges TopFoldRanges, int FoldIndex,PSynEditFoldRanges& parentFoldRanges, PSynEditFoldRange Parent)
{
    PSynEditFoldRange  CollapsedFold;
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
                    Parent = parentFoldRanges->addByParts(
                      Parent,
                      TopFoldRanges,
                      Line + 1,
                      mCodeFolding.foldRegions.get(FoldIndex),
                      Line + 1);
                    parentFoldRanges = Parent->subFoldRanges;

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
                      if (!Parent) {
                          parentFoldRanges = TopFoldRanges;
                      } else {
                          parentFoldRanges = Parent->subFoldRanges;
                      }
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
    //showCaret();
}

PSynEditFoldRange SynEdit::foldStartAtLine(int Line)
{
    for (int i = 0; i<mAllFoldRanges.count();i++) {
        PSynEditFoldRange range = mAllFoldRanges[i];
        if (range->fromLine == Line ){
            return range;
        } else if (range->fromLine>Line)
            break; // sorted by line. don't bother scanning further
    }
    return PSynEditFoldRange();
}

QString SynEdit::substringByColumns(const QString &s, int startColumn, int &colLen)
{

    int len = s.length();
    int columns = 0;
    int i = 0;
    int oldColumns;
    while (columns < startColumn) {
        oldColumns = columns;
        if (i < len && s[i] == '\t')
            columns += mTabWidth - (columns % mTabWidth);
        else
            columns += charColumns(s[i]);
        i++;
    }
    QString result;
    if (i>=len) {
        colLen = 0;
        return result;
    }
    if (colLen>result.capacity()) {
        result.resize(colLen);
    }
    int j=0;
    if (i>0) {
        result[0]=s[i-1];
        j++;
    }
    while (i<len && columns<startColumn+colLen) {
        result[j]=s[i];
        if (i < len && s[i] == '\t')
            columns += mTabWidth - (columns % mTabWidth);
        else
            columns += charColumns(s[i]);
        i++;
        j++;
    }
    result.resize(j);
    colLen = columns-oldColumns;
    return result;
}

PSynEditFoldRange SynEdit::foldAroundLine(int Line)
{
    return foldAroundLineEx(Line,false,false,false);
}

PSynEditFoldRange SynEdit::foldAroundLineEx(int Line, bool WantCollapsed, bool AcceptFromLine, bool AcceptToLine)
{
    // Check global list
    PSynEditFoldRange Result = checkFoldRange(&mAllFoldRanges, Line, WantCollapsed, AcceptFromLine, AcceptToLine);

    // Found an item in the top level list?
    if (Result) {
        while (true) {
            PSynEditFoldRange ResultChild = checkFoldRange(Result->subFoldRanges.get(), Line, WantCollapsed, AcceptFromLine, AcceptToLine);
            if (!ResultChild)
                break;
            Result = ResultChild; // repeat for this one
        }
    }
    return Result;
}

PSynEditFoldRange SynEdit::checkFoldRange(SynEditFoldRanges *FoldRangeToCheck, int Line, bool WantCollapsed, bool AcceptFromLine, bool AcceptToLine)
{
    for (int i = 0; i< FoldRangeToCheck->count(); i++) {
        PSynEditFoldRange range = (*FoldRangeToCheck)[i];
        if (((range->fromLine < Line) || ((range->fromLine <= Line) && AcceptFromLine)) &&
          ((range->toLine > Line) || ((range->toLine >= Line) && AcceptToLine))) {
            if (range->collapsed == WantCollapsed) {
                return range;
            }
        }
    }
    return PSynEditFoldRange();
}

PSynEditFoldRange SynEdit::foldEndAtLine(int Line)
{
    for (int i = 0; i<mAllFoldRanges.count();i++) {
        PSynEditFoldRange range = mAllFoldRanges[i];
        if (range->toLine == Line ){
            return range;
        } else if (range->fromLine>Line)
            break; // sorted by line. don't bother scanning further
    }
    return PSynEditFoldRange();
}

void SynEdit::paintCaret(QPainter &painter, const QRect rcClip)
{
    if (m_blinkStatus!=1)
        return;
    painter.setClipRect(rcClip);
    SynEditCaretType ct;
    if (this->mInserting) {
        ct = mInsertCaret;
    } else {
        ct =mOverwriteCaret;
    }
    painter.setPen(QColorConstants::Svg::red);
    switch(ct) {
    case SynEditCaretType::ctVerticalLine:
        painter.drawLine(rcClip.left()+1,rcClip.top(),rcClip.left()+1,rcClip.bottom());
        break;
    case SynEditCaretType::ctHorizontalLine:
        painter.drawLine(rcClip.left(),rcClip.bottom()-1,rcClip.right(),rcClip.bottom()-1);
        break;
    case SynEditCaretType::ctBlock:
        painter.fillRect(rcClip, QColorConstants::Svg::red);
        break;
    case SynEditCaretType::ctHalfBlock:
        QRect rc=rcClip;
        rc.setTop(rcClip.top()+rcClip.height() / 2);
        painter.fillRect(rcClip, QColorConstants::Svg::red);
        break;
    }
}

int SynEdit::textOffset()
{
    return mGutterWidth + 2 - (mLeftChar-1)*mCharWidth;
}

SynEditorCommand SynEdit::TranslateKeyCode(int key, Qt::KeyboardModifiers modifiers)
{
    PSynEditKeyStroke keyStroke = mKeyStrokes.findKeycode2(mLastKey,mLastKeyModifiers,
                                                           key, modifiers);
    SynEditorCommand cmd=SynEditorCommand::ecNone;
    if (keyStroke)
        cmd = keyStroke->command();
    else {
        keyStroke = mKeyStrokes.findKeycode(key,modifiers);
        if (keyStroke)
            cmd = keyStroke->command();
    }
    if (cmd == SynEditorCommand::ecNone) {
        mLastKey = key;
        mLastKeyModifiers = modifiers;
    } else {
        mLastKey = 0;
        mLastKeyModifiers = Qt::NoModifier;
    }
    return cmd;
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

void SynEdit::doScrolled(int)
{
    mLeftChar = horizontalScrollBar()->value();
    mTopLine = verticalScrollBar()->value();
    invalidate();
}

SynSelectionMode SynEdit::selectionMode() const
{
    return mSelectionMode;
}

void SynEdit::setSelectionMode(SynSelectionMode value)
{
    if (mSelectionMode!=value) {
        mSelectionMode = value;
        setActiveSelectionMode(value);
    }
}

bool SynEdit::useCodeFolding() const
{
    return mUseCodeFolding;
}

void SynEdit::setUseCodeFolding(bool value)
{
    if (mUseCodeFolding!=value) {
        mUseCodeFolding = value;
        rescan();
    }
}

QString SynEdit::lineText()
{
    if (mCaretY >= 1 && mCaretY <= mLines->count())
        return mLines->getString(mCaretY - 1);
    else
        return QString();
}

void SynEdit::setLineText(const QString s)
{
    if (mCaretY >= 1 && mCaretY <= mLines->count())
        mLines->putString(mCaretY-1,s);
}

PSynHighlighter SynEdit::highlighter() const
{
    return mHighlighter;
}

void SynEdit::setHighlighter(const PSynHighlighter &highlighter)
{
    PSynHighlighter oldHighlighter= mHighlighter;
    mHighlighter = highlighter;
    if (oldHighlighter && mHighlighter &&
            oldHighlighter->language() == highlighter->language()) {
    } else {
        recalcCharExtent();
        mLines->beginUpdate();
        auto action=finally([this]{
            mLines->endUpdate();
        });
        scanRanges();
    }
    sizeOrFontChanged(true);
    invalidate();
}

PSynEditStringList SynEdit::lines() const
{
    return mLines;
}

int SynEdit::tabWidth() const
{
    return mTabWidth;
}

bool SynEdit::empty()
{
    return mLines->empty();
}

void SynEdit::CommandProcessor(SynEditorCommand Command, QChar AChar, void *pData)
{
    // first the program event handler gets a chance to process the command
    onProcessCommand(Command, AChar, pData);
    if (Command != SynEditorCommand::ecNone)
        ExecuteCommand(Command, AChar, pData);
    onCommandProcessed(Command, AChar, pData);
}

void SynEdit::MoveCaretHorz(int DX, bool isSelection)
{
    BufferCoord ptO = caretXY();
    BufferCoord ptDst = ptO;
    QString s = lineText();
    int nLineLen = s.length();
    // only moving or selecting one char can change the line
    bool bChangeY = !mOptions.testFlag(SynEditorOption::eoScrollPastEol);
    if (bChangeY && (DX == -1) && (ptO.Char == 1) && (ptO.Line > 1)) {
        // end of previous line
        ptDst.Line--;
        ptDst.Char = mLines->getString(ptDst.Line - 1).length() + 1;
    } else if (bChangeY && (DX == 1) && (ptO.Char > nLineLen) && (ptO.Line < mLines->count())) {
        // start of next line
        ptDst.Line++;
        ptDst.Char=1;
    } else {
        ptDst.Char = std::max(1, ptDst.Char + DX);
        // don't go past last char when ScrollPastEol option not set
        if ((DX > 0) && bChangeY)
          ptDst.Char = std::min(ptDst.Char, nLineLen + 1);
    }
    // set caret and block begin / end
    MoveCaretAndSelection(mBlockBegin, ptDst, isSelection);
}

void SynEdit::MoveCaretVert(int DY, bool isSelection)
{
    DisplayCoord ptO = displayXY();
    DisplayCoord ptDst = ptO;

    ptDst.Row+=DY;
    if (DY >= 0) {
        if (rowToLine(ptDst.Row) > mLines->count())
            ptDst.Row = std::max(1, displayLineCount());
    } else {
        if (ptDst.Row < 1)
            ptDst.Row = 1;
    }

    if (ptO.Row != ptDst.Row) {
        if (mOptions.testFlag(eoKeepCaretX))
            ptDst.Column = mLastCaretX;
    }
    BufferCoord vDstLineChar = displayToBufferPos(ptDst);
    int SaveLastCaretX = mLastCaretX;
    bool NewStepAside = mMBCSStepAside;

    // set caret and block begin / end
    incPaintLock();
    MoveCaretAndSelection(mBlockBegin, vDstLineChar, isSelection);
    decPaintLock();

    // Set fMBCSStepAside and restore fLastCaretX after moving caret, since
    // UpdateLastCaretX, called by SetCaretXYEx, changes them. This is the one
    // case where we don't want that.
    mMBCSStepAside = NewStepAside;
    mLastCaretX = SaveLastCaretX;
}

void SynEdit::MoveCaretAndSelection(const BufferCoord &ptBefore, const BufferCoord &ptAfter, bool isSelection)
{
    if (mOptions.testFlag(SynEditorOption::eoGroupUndo) && mUndoList->CanUndo())
      mUndoList->AddGroupBreak();

    incPaintLock();
    if (isSelection) {
      if (!selAvail())
        setBlockBegin(ptBefore);
      setBlockEnd(ptAfter);
    } else
      setBlockBegin(ptAfter);
    internalSetCaretXY(ptAfter);
    decPaintLock();
}

void SynEdit::MoveCaretToLineStart(bool isSelection)
{
    int newX;
    // home key enhancement
    if (mOptions.testFlag(SynEditorOption::eoEnhanceHomeKey) && (lineToRow(mCaretY) == displayY())) {
        QString s = mLines->getString(mCaretY - 1);

        int first_nonblank = 0;
        int vMaxX = s.length();
        while ((first_nonblank < vMaxX) && (s[first_nonblank] == ' ' || s[first_nonblank] == '\t')) {
            first_nonblank++;
        }

        newX = mCaretX;

        if (newX > first_nonblank)
            newX = first_nonblank;
        else
            newX = 1;
    } else
        newX = 1;
    MoveCaretAndSelection(caretXY(), BufferCoord{newX, mCaretY}, isSelection);
}

void SynEdit::MoveCaretToLineEnd(bool isSelection)
{
    int vNewX;
    if (mOptions.testFlag(SynEditorOption::eoEnhanceEndKey)) {
        QString vText = lineText();
        int vLastNonBlank = vText.length();
        int vMinX = 0;
        while ((vLastNonBlank > vMinX) && (vText[vLastNonBlank] == ' ' || vText[vLastNonBlank] =='\t'))
            vLastNonBlank--;
        vNewX = mCaretX;
        if (vNewX > vLastNonBlank)
            vNewX = vText.length() + 1;
        else
            vNewX = vLastNonBlank + 1;
    } else
        vNewX = lineText().length() + 1;

    MoveCaretAndSelection(caretXY(), BufferCoord{vNewX, mCaretY}, isSelection);
}

bool SynEdit::onGetSpecialLineColors(int, QColor &, QColor &)
{
    return false;
}

void SynEdit::onGetEditingAreas(int, SynEditingAreaList &)
{

}

void SynEdit::onGutterGetText(int aLine, QString &aText)
{

}

void SynEdit::onGutterPaint(QPainter &painter, int aLine, int X, int Y)
{

}

void SynEdit::onPaint(QPainter &)
{

}

void SynEdit::onProcessCommand(SynEditorCommand Command, QChar AChar, void *pData)
{

}

void SynEdit::onCommandProcessed(SynEditorCommand Command, QChar AChar, void *pData)
{

}

void SynEdit::ExecuteCommand(SynEditorCommand Command, QChar AChar, void *pData)
{
    switch(Command) {
    //horizontal caret movement or selection
    case SynEditorCommand::ecLeft:
    case SynEditorCommand::ecSelLeft:
        MoveCaretHorz(-1, Command == SynEditorCommand::ecSelLeft);
        break;
    case SynEditorCommand::ecRight:
    case SynEditorCommand::ecSelRight:
        MoveCaretHorz(1, Command == SynEditorCommand::ecSelRight);
        break;
    case SynEditorCommand::ecPageLeft:
    case SynEditorCommand::ecSelPageLeft:
        MoveCaretHorz(-mCharsInWindow, Command == SynEditorCommand::ecSelPageLeft);
        break;
    case SynEditorCommand::ecPageRight:
    case SynEditorCommand::ecSelPageRight:
        MoveCaretHorz(mCharsInWindow, Command == SynEditorCommand::ecSelPageRight);
        break;
    case SynEditorCommand::ecLineStart:
    case SynEditorCommand::ecSelLineStart:
        MoveCaretToLineStart(Command == SynEditorCommand::ecSelLineStart);
        break;
    case SynEditorCommand::ecLineEnd:
    case SynEditorCommand::ecSelLineEnd:
        MoveCaretToLineEnd(Command == SynEditorCommand::ecSelLineEnd);
        break;
    // vertical caret movement or selection
    case SynEditorCommand::ecUp:
    case SynEditorCommand::ecSelUp:
        MoveCaretVert(-1, Command == SynEditorCommand::ecSelUp);
        break;
    case SynEditorCommand::ecDown:
    case SynEditorCommand::ecSelDown:
        MoveCaretVert(1, Command == SynEditorCommand::ecSelDown);
        break;
    case SynEditorCommand::ecPageUp:
    case SynEditorCommand::ecSelPageUp:
    case SynEditorCommand::ecPageDown:
    case SynEditorCommand::ecSelPageDown:
    {
        int counter = mLinesInWindow;
        if (mOptions.testFlag(eoHalfPageScroll))
            counter /= 2;
        if (mOptions.testFlag(eoScrollByOneLess)) {
            counter -=1;
        }
        if (counter<0)
            break;
        if (Command == SynEditorCommand::ecPageUp || Command == SynEditorCommand::ecSelPageUp) {
            counter = -counter;
        }
        MoveCaretVert(counter, Command == SynEditorCommand::ecSelPageUp || Command == SynEditorCommand::ecSelPageDown);
        break;
    }
    case SynEditorCommand::ecPageTop:
    case SynEditorCommand::ecSelPageTop:
        MoveCaretVert(mTopLine-mCaretY, Command == SynEditorCommand::ecSelPageTop);
        break;
    case SynEditorCommand::ecPageBottom:
    case SynEditorCommand::ecSelPageBottom:
        MoveCaretVert(mTopLine+mLinesInWindow-1-mCaretY, Command == SynEditorCommand::ecSelPageBottom);
        break;
    case SynEditorCommand::ecEditorTop:
    case SynEditorCommand::ecSelEditorTop:
        MoveCaretVert(1-mCaretY, Command == SynEditorCommand::ecSelEditorTop);
        break;
    case SynEditorCommand::ecEditorBottom:
    case SynEditorCommand::ecSelEditorBottom:
        if (!mLines->empty())
            MoveCaretVert(mLines->count()-mCaretY, Command == SynEditorCommand::ecSelEditorBottom);
        break;
    // goto special line / column position
    case SynEditorCommand::ecGotoXY:
    case SynEditorCommand::ecSelGotoXY:
        if (pData)
            MoveCaretAndSelection(caretXY(), *((BufferCoord *)(pData)), Command == SynEditorCommand::ecSelGotoXY);
    }

//    procedure SetSelectedTextEmpty;
//    var
//      vSelText: string;
//      vUndoBegin, vUndoEnd: TBufferCoord;
//    begin
//      vUndoBegin := fBlockBegin;
//      vUndoEnd := fBlockEnd;
//      vSelText := SelText;
//      SetSelTextPrimitive('');
//      if (vUndoBegin.Line < vUndoEnd.Line) or (
//        (vUndoBegin.Line = vUndoEnd.Line) and (vUndoBegin.Char < vUndoEnd.Char)) then begin
//        fUndoList.AddChange(crDelete, vUndoBegin, vUndoEnd, vSelText,
//          fActiveSelectionMode);
//      end else begin
//        fUndoList.AddChange(crDeleteAfterCursor, vUndoBegin, vUndoEnd, vSelText,
//          fActiveSelectionMode);
//      end;
//    end;

//    procedure ForceCaretX(aCaretX: integer);
//    var
//      vRestoreScroll: boolean;
//    begin
//      vRestoreScroll := not (eoScrollPastEol in fOptions);
//      Include(fOptions, eoScrollPastEol);
//      try
//        InternalCaretX := aCaretX;
//      finally
//        if vRestoreScroll then
//          Exclude(fOptions, eoScrollPastEol);
//      end;
//    end;

//  var
//    CX: Integer;
//    Len: Integer;
//    Temp: string;
//    Temp2: string;
//    Temp3: AnsiString;
//    helper: string;
//    TabBuffer: string;
//    SpaceBuffer: string;
//    SpaceCount1: Integer;
//    SpaceCount2: Integer;
//    BackCounter: Integer;
//    OrigBlockBegin: TBufferCoord;
//    OrigBlockEnd: TBufferCoord;
//    OrigCaret: TBufferCoord;
//    MoveDelim: TBufferCoord;
//    BeginIndex: integer;
//    EndIndex: integer;
//    StartOfBlock: TBufferCoord;
//    CurrentBracketPos, MatchBracketPos: TBufferCoord;
//    bChangeScroll: boolean;
//    moveBkm: boolean;
//    WP: TBufferCoord;
//    Caret: TBufferCoord;
//    CaretNew: TBufferCoord;
//    i, j: integer;
//  {$IFDEF SYN_MBCSSUPPORT}
//    s: string;
//  {$ENDIF}
//    counter: Integer;
//    InsDelta: integer;
//    iUndoBegin, iUndoEnd: TBufferCoord;
//    vCaretRow: integer;
//    vTabTrim: integer;
//    Attr: TSynHighlighterAttributes;
//    StartPos: Integer;
//    EndPos: Integer;
//    tempStr : AnsiString;
//    nLinesInserted: integer;
//  begin
//    IncPaintLock;
//    try
//      case Command of
//        // goto special line / column position
//        ecGotoXY, ecSelGotoXY:
//          if Assigned(Data) then begin
//            MoveCaretAndSelection(CaretXY, TBufferCoord(Data^), Command = ecSelGotoXY);
//            Update;
//          end;
//        // word selection
//        ecWordLeft, ecSelWordLeft: begin
//            CaretNew := PrevWordPos;
//            MoveCaretAndSelection(CaretXY, CaretNew, Command = ecSelWordLeft);
//          end;
//        ecWordRight, ecSelWordRight: begin
//            CaretNew := NextWordPos;
//            MoveCaretAndSelection(CaretXY, CaretNew, Command = ecSelWordRight);
//          end;
//        ecSelWord: begin
//            SetSelWord;
//          end;
//        ecSelectAll: begin
//            SelectAll;
//          end;
//        ecDeleteLastChar:
//          if not ReadOnly then begin
//            DoOnPaintTransientEx(ttBefore, true);
//            try
//              if SelAvail then
//                SetSelectedTextEmpty
//              else begin
//                Temp := LineText;
//                TabBuffer := Lines.ExpandedStrings[CaretY - 1];
//                Len := Length(Temp);
//                Caret := CaretXY;
//                vTabTrim := 0;
//                if CaretX > Len + 1 then begin
//                  helper := '';
//                  if eoSmartTabDelete in fOptions then begin
//                    //It's at the end of the line, move it to the length
//                    if Len > 0 then
//                      InternalCaretX := Len + 1
//                    else begin
//                      //move it as if there were normal spaces there
//                      SpaceCount1 := CaretX - 1;
//                      SpaceCount2 := 0;
//                      // unindent
//                      if SpaceCount1 > 0 then begin
//                        BackCounter := CaretY - 2;
//                        //It's better not to have if statement inside loop
//                        if (eoTrimTrailingSpaces in Options) and (Len = 0) then
//                          while BackCounter >= 0 do begin
//                            SpaceCount2 := LeftSpacesEx(Lines[BackCounter], True);
//                            if SpaceCount2 < SpaceCount1 then
//                              break;
//                            Dec(BackCounter);
//                          end else
//                          while BackCounter >= 0 do begin
//                            SpaceCount2 := LeftSpaces(Lines[BackCounter]);
//                            if SpaceCount2 < SpaceCount1 then
//                              break;
//                            Dec(BackCounter);
//                          end;
//                        if (BackCounter = -1) and (SpaceCount2 > SpaceCount1) then
//                          SpaceCount2 := 0;
//                      end;
//                      if SpaceCount2 = SpaceCount1 then
//                        SpaceCount2 := 0;
//                      fCaretX := fCaretX - (SpaceCount1 - SpaceCount2);
//                      UpdateLastCaretX;
//                      fStateFlags := fStateFlags + [sfCaretChanged];
//                      StatusChanged([scCaretX]);
//                    end;
//                  end else begin
//                    // only move caret one column
//                    InternalCaretX := CaretX - 1;
//                  end;
//                end else if CaretX = 1 then begin
//                  // join this line with the last line if possible
//                  if CaretY > 1 then begin
//                    InternalCaretY := CaretY - 1;
//                    InternalCaretX := Length(Lines[CaretY - 1]) + 1;
//                    Lines.Delete(CaretY);
//                    DoLinesDeleted(CaretY+1, 1);
//                    if eoTrimTrailingSpaces in Options then
//                      Temp := TrimTrailingSpaces(Temp);

//                    LineText := LineText + Temp;
//                    helper := #13#10;
//                  end;
//                end else begin
//                  // delete text before the caret
//                  SpaceCount1 := LeftSpaces(Temp);
//                  SpaceCount2 := 0;
//                  if (Temp[CaretX - 1] <= #32) and (SpaceCount1 = CaretX - 1) then begin
//                    if eoSmartTabDelete in fOptions then begin
//                      // unindent
//                      if SpaceCount1 > 0 then begin
//                        BackCounter := CaretY - 2;
//                        while BackCounter >= 0 do begin
//                          SpaceCount2 := LeftSpaces(Lines[BackCounter]);
//                          if SpaceCount2 < SpaceCount1 then
//                            break;
//                          Dec(BackCounter);
//                        end;
//                        if (BackCounter = -1) and (SpaceCount2 > SpaceCount1) then
//                          SpaceCount2 := 0;
//                      end;
//                      if SpaceCount2 = SpaceCount1 then
//                        SpaceCount2 := 0;
//                      helper := Copy(Temp, 1, SpaceCount1 - SpaceCount2);
//                      Delete(Temp, 1, SpaceCount1 - SpaceCount2);
//                    end else begin
//                      SpaceCount2 := SpaceCount1;
//                      //how much till the next tab column
//                      BackCounter := (DisplayX - 1) mod FTabWidth;
//                      if BackCounter = 0 then
//                        BackCounter := FTabWidth;

//                      SpaceCount1 := 0;
//                      CX := DisplayX - BackCounter;
//                      while (SpaceCount1 < FTabWidth) and
//                        (SpaceCount1 < BackCounter) and
//                        (TabBuffer[CX] <> #9) do begin
//                        Inc(SpaceCount1);
//                        Inc(CX);
//                      end;
//  {$IFOPT R+}
//                      // Avoids an exception when compiled with $R+.
//                      // 'CX' can be 'Length(TabBuffer)+1', which isn't an AV and evaluates
//                      //to #0. But when compiled with $R+, Delphi raises an Exception.
//                      if CX <= Length(TabBuffer) then
//  {$ENDIF}
//                        if TabBuffer[CX] = #9 then
//                          SpaceCount1 := SpaceCount1 + 1;

//                      if SpaceCount2 = SpaceCount1 then begin
//                        helper := Copy(Temp, 1, SpaceCount1);
//                        Delete(Temp, 1, SpaceCount1);
//                      end else begin
//                        helper := Copy(Temp, SpaceCount2 - SpaceCount1 + 1, SpaceCount1);
//                        Delete(Temp, SpaceCount2 - SpaceCount1 + 1, SpaceCount1);
//                      end;
//                      SpaceCount2 := 0;
//                    end;
//                    ProperSetLine(CaretY - 1, Temp);
//                    fCaretX := fCaretX - (SpaceCount1 - SpaceCount2);
//                    UpdateLastCaretX;
//                    fStateFlags := fStateFlags + [sfCaretChanged];
//                    StatusChanged([scCaretX]);
//                  end else begin
//                    // delete char
//                    counter := 1;
//  {$IFDEF SYN_MBCSSUPPORT}
//                    if (CaretX >= 3) and (ByteType(Temp, CaretX - 2) = mbLeadByte) then
//                      Inc(counter);
//  {$ENDIF}
//                    InternalCaretX := CaretX - counter;
//                    // Stores the previous "expanded" CaretX if the line contains tabs.
//                    if (eoTrimTrailingSpaces in Options) and (Len <> Length(TabBuffer)) then
//                      vTabTrim := CharIndex2CaretPos(CaretX, TabWidth, Temp);
//                    helper := Copy(Temp, CaretX, counter);
//                    Delete(Temp, CaretX, counter);
//                    ProperSetLine(CaretY - 1, Temp);
//                    // Calculates a delta to CaretX to compensate for trimmed tabs.
//                    if vTabTrim <> 0 then
//                      if Length(Temp) <> Length(LineText) then
//                        Dec(vTabTrim, CharIndex2CaretPos(CaretX, TabWidth, LineText))
//                      else
//                        vTabTrim := 0;
//                  end;
//                end;
//                if (Caret.Char <> CaretX) or (Caret.Line <> CaretY) then begin
//                  fUndoList.AddChange(crSilentDelete, CaretXY, Caret, helper,
//                    smNormal);
//                  if vTabTrim <> 0 then
//                    ForceCaretX(CaretX + vTabTrim);
//                end;
//              end;
//              EnsureCursorPosVisible;
//            finally
//              DoOnPaintTransientEx(ttAfter, true);
//            end;
//          end;
//        ecDeleteChar:
//          if not ReadOnly then begin
//            DoOnPaintTransient(ttBefore);

//            if SelAvail then
//              SetSelectedTextEmpty
//            else begin
//              // Call UpdateLastCaretX. Even though the caret doesn't move, the
//              // current caret position should "stick" whenever text is modified.
//              UpdateLastCaretX;
//              Temp := LineText;
//              Len := Length(Temp);
//              if CaretX <= Len then begin
//                // delete char
//                counter := 1;
//  {$IFDEF SYN_MBCSSUPPORT}
//                if ByteType(Temp, CaretX) = mbLeadByte then
//                  Inc(counter);
//  {$ENDIF}
//                helper := Copy(Temp, CaretX, counter);
//                Caret.Char := CaretX + counter;
//                Caret.Line := CaretY;
//                Delete(Temp, CaretX, counter);
//                ProperSetLine(CaretY - 1, Temp);
//              end else begin
//                // join line with the line after
//                if CaretY < Lines.Count then begin
//                  helper := StringOfChar(#32, CaretX - 1 - Len);
//                  ProperSetLine(CaretY - 1, Temp + helper + Lines[CaretY]);
//                  Caret.Char := 1;
//                  Caret.Line := CaretY + 1;
//                  helper := #13#10;
//                  Lines.Delete(CaretY);
//                  if CaretX=1 then
//                    DoLinesDeleted(CaretY, 1)
//                  else
//                    DoLinesDeleted(CaretY + 1, 1);
//                end;
//              end;
//              if (Caret.Char <> CaretX) or (Caret.Line <> CaretY) then begin
//                fUndoList.AddChange(crSilentDeleteAfterCursor, CaretXY, Caret,
//                  helper, smNormal);
//              end;
//            end;
//            DoOnPaintTransient(ttAfter);
//          end;
//        ecDeleteWord, ecDeleteEOL:
//          if not ReadOnly then begin
//            DoOnPaintTransient(ttBefore);
//            Len := Length(LineText);
//            if Command = ecDeleteWord then begin
//              WP := WordEnd;
//              Temp := LineText;
//              if (WP.Char < CaretX) or ((WP.Char = CaretX) and (WP.Line < fLines.Count)) then begin
//                if WP.Char > Len then begin
//                  Inc(WP.Line);
//                  WP.Char := 1;
//                  Temp := Lines[WP.Line - 1];
//                end else if Temp[WP.Char] <> #32 then
//                  Inc(WP.Char);
//              end;
//  {$IFOPT R+}
//              Temp := Temp + #0;
//  {$ENDIF}
//              if Temp <> '' then
//                while Temp[WP.Char] = #32 do
//                  Inc(WP.Char);
//            end else begin
//              WP.Char := Len + 1;
//              WP.Line := CaretY;
//            end;
//            if (WP.Char <> CaretX) or (WP.Line <> CaretY) then begin
//              SetBlockBegin(CaretXY);
//              SetBlockEnd(WP);
//              ActiveSelectionMode := smNormal;
//              helper := SelText;
//              SetSelTextPrimitive(StringOfChar(' ', CaretX - BlockBegin.Char));
//              fUndoList.AddChange(crSilentDeleteAfterCursor, CaretXY, WP,
//                helper, smNormal);
//              InternalCaretXY := CaretXY;
//            end;
//            DoOnPaintTransient(ttAfter);
//          end;
//        ecDeleteLastWord, ecDeleteBOL:
//          if not ReadOnly then begin
//            DoOnPaintTransient(ttBefore);
//            if Command = ecDeleteLastWord then
//              WP := PrevWordPos
//            else begin
//              WP.Char := 1;
//              WP.Line := CaretY;
//            end;
//            if (WP.Char <> CaretX) or (WP.Line <> CaretY) then begin
//              SetBlockBegin(CaretXY);
//              SetBlockEnd(WP);
//              ActiveSelectionMode := smNormal;
//              helper := SelText;
//              SetSelTextPrimitive('');
//              fUndoList.AddChange(crSilentDelete, WP, CaretXY, helper,
//                smNormal);
//              InternalCaretXY := WP;
//            end;
//            DoOnPaintTransient(ttAfter);
//          end;
//        ecDeleteLine:
//          if not ReadOnly and (Lines.Count > 0) and not ((CaretY = Lines.Count) and (Length(Lines[CaretY - 1]) =
//            0)) then begin
//            DoOnPaintTransient(ttBefore);
//            if SelAvail then
//              SetBlockBegin(CaretXY);
//            helper := LineText;
//            if CaretY = Lines.Count then begin
//              Lines[CaretY - 1] := '';
//              fUndoList.AddChange(crSilentDeleteAfterCursor, BufferCoord(1, CaretY),
//                BufferCoord(Length(helper) + 1, CaretY), helper, smNormal);
//            end else begin
//              Lines.Delete(CaretY - 1);
//              helper := helper + #13#10;
//              fUndoList.AddChange(crSilentDeleteAfterCursor, BufferCoord(1, CaretY),
//                BufferCoord(1, CaretY + 1), helper, smNormal);
//              DoLinesDeleted(CaretY, 1);
//            end;
//            InternalCaretXY := BufferCoord(1, CaretY); // like seen in the Delphi editor
//            DoOnPaintTransient(ttAfter);
//          end;
//        ecDuplicateLine:
//          if not ReadOnly and (Lines.Count > 0) then begin
//            DoOnPaintTransient(ttBefore);
//            Lines.Insert(CaretY, Lines[CaretY - 1]);
//            DoLinesInserted(CaretY + 1, 1);
//            fUndoList.AddChange(crLineBreak, CaretXY, CaretXY, '', smNormal);
//            InternalCaretXY := BufferCoord(1, CaretY); // like seen in the Delphi editor
//            DoOnPaintTransient(ttAfter);
//          end;
//        ecMoveSelUp:
//          if not ReadOnly and (Lines.Count > 0) and (BlockBegin.Line > 1) then begin
//            DoOnPaintTransient(ttBefore);

//            // Backup caret and selection
//            OrigBlockBegin := BlockBegin;
//            OrigBlockEnd := BlockEnd;

//            // Delete line above selection
//            s := Lines[OrigBlockBegin.Line - 2]; // before start, 0 based
//            Lines.Delete(OrigBlockBegin.Line - 2); // before start, 0 based
//            DoLinesDeleted(OrigBlockBegin.Line - 1, 1); // before start, 1 based

//            // Insert line below selection
//            Lines.Insert(OrigBlockEnd.Line - 1, S);
//            DoLinesInserted(OrigBlockEnd.Line, 1);

//            // Restore caret and selection
//            SetCaretAndSelection(
//              BufferCoord(CaretX, CaretY - 1),
//              BufferCoord(1, OrigBlockBegin.Line - 1), // put start of selection at start of top line
//              BufferCoord(Length(Lines[OrigBlockEnd.Line - 2]) + 1, OrigBlockEnd.Line - 1));
//            // put end of selection at end of top line

//        // Retrieve end of line we moved up
//            MoveDelim := BufferCoord(Length(Lines[OrigBlockEnd.Line - 1]) + 1, OrigBlockEnd.Line);

//            // Support undo, implement as drag and drop
//            fUndoList.BeginBlock;
//            try
//              fUndoList.AddChange(crSelection, // backup original selection
//                OrigBlockBegin,
//                OrigBlockEnd,
//                '',
//                smNormal);
//              fUndoList.AddChange(crDragDropInsert,
//                BlockBegin, // modified
//                MoveDelim, // put at end of line me moved up
//                S + #13#10 + SelText,
//                smNormal);
//            finally
//              fUndoList.EndBlock;
//            end;

//            DoOnPaintTransient(ttAfter);
//          end;
//        ecMoveSelDown:
//          if not ReadOnly and (Lines.Count > 0) and (BlockEnd.Line < Lines.Count) then begin
//            DoOnPaintTransient(ttBefore);

//            // Backup caret and selection
//            OrigBlockBegin := BlockBegin;
//            OrigBlockEnd := BlockEnd;

//            // Delete line below selection
//            s := Lines[OrigBlockEnd.Line]; // after end, 0 based
//            Lines.Delete(OrigBlockEnd.Line); // after end, 0 based
//            DoLinesDeleted(OrigBlockEnd.Line, 1); // before start, 1 based

//            // Insert line above selection
//            Lines.Insert(OrigBlockBegin.Line - 1, S);
//            DoLinesInserted(OrigBlockBegin.Line, 1);

//            // Restore caret and selection
//            SetCaretAndSelection(
//              BufferCoord(CaretX, CaretY + 1),
//              BufferCoord(1, OrigBlockBegin.Line + 1),
//              BufferCoord(Length(Lines[OrigBlockEnd.Line]) + 1, OrigBlockEnd.Line + 1));

//            // Retrieve start of line we moved down
//            MoveDelim := BufferCoord(1, OrigBlockBegin.Line);

//            // Support undo, implement as drag and drop
//            fUndoList.BeginBlock;
//            try
//              fUndoList.AddChange(crSelection,
//                OrigBlockBegin,
//                OrigBlockEnd,
//                '',
//                smNormal);
//              fUndoList.AddChange(crDragDropInsert,
//                MoveDelim, // put at start of line me moved down
//                BlockEnd, // modified
//                SelText + #13#10 + S,
//                smNormal);
//            finally
//              fUndoList.EndBlock;
//            end;

//            DoOnPaintTransient(ttAfter);
//          end;
//        ecClearAll: begin
//            if not ReadOnly then
//              ClearAll;
//          end;
//        ecInsertLine,
//          ecLineBreak:
//          if not ReadOnly then begin
//            nLinesInserted:=0;
//            UndoList.BeginBlock;
//            try
//              if SelAvail then begin
//                helper := SelText;
//                iUndoBegin := fBlockBegin;
//                iUndoEnd := fBlockEnd;
//                SetSelTextPrimitive('');
//                fUndoList.AddChange(crDelete, iUndoBegin, iUndoEnd, helper,
//                  fActiveSelectionMode);
//              end;
//              Temp := LineText;
//              Temp2 := Temp;
//              // This is sloppy, but the Right Thing would be to track the column of markers
//              // too, so they could be moved depending on whether they are after the caret...
//              InsDelta := Ord(CaretX = 1);
//              Len := Length(Temp);
//              if Len > 0 then begin
//                if Len >= CaretX then begin
//                  if CaretX > 1 then begin

//                    Temp:= Copy(LineText, 1, CaretX - 1);
//                    Temp3:=Temp;
//                    SpaceCount1 := LeftSpacesEx(Temp, true);
//                    Delete(Temp2, 1, CaretX - 1);
//                    ProperSetLine(CaretY-1,Temp);
//                    Lines.Insert(CaretY, GetLeftSpacing(SpaceCount1, true));
//                    inc(nLinesInserted);
//                    if (eoAddIndent in Options) and GetHighlighterAttriAtRowCol(BufferCoord(Length(Temp3), CaretY),
//                      Temp3, Attr) then begin // only add indent to source files
//                      if Attr <> Highlighter.CommentAttribute then begin // and outside of comments
//                        if (Temp[Length(Temp)] =':')
//                          or (
//                            (Temp[Length(Temp)] ='{')
//                            and ((Length(Temp2)<=0) or (Temp2[1]<>'}'))
//                          )then begin // add more indent for these too
//                          if not (eoTabsToSpaces in Options) then begin
//                            Lines[CaretY] := Lines[CaretY] + TSynTabChar;
//                          end else begin
//                            Lines[CaretY] := Lines[CaretY] + StringOfChar(' ', TabWidth);
//                          end;
//                        end;
//                      end;
//                    end;
//                    SpaceCount1 := Length(Lines[CaretY]);
//                    Lines[CaretY] := Lines[CaretY] + Temp2;
//                    fUndoList.AddChange(crLineBreak, CaretXY, CaretXY, Temp2,
//                      smNormal);

//                    if (Length(Temp)>0) and (Temp[Length(Temp)] = '{') and (Length(Temp2)>0) and (Temp2[1]='}') then begin
//                      Lines.Insert(CaretY, GetLeftSpacing(LeftSpacesEx(Temp, true), true));
//                      inc(nLinesInserted);
//                      if (eoAddIndent in Options) then begin;
//                        if not (eoTabsToSpaces in Options) then begin
//                          Lines[CaretY] := Lines[CaretY] + TSynTabChar;
//                        end else begin
//                          Lines[CaretY] := Lines[CaretY] + StringOfChar(' ', TabWidth);
//                        end;
//                      end;
//                      fUndoList.AddChange(crLineBreak, CaretXY, CaretXY, '',
//                        smNormal);
//                      if Command = ecLineBreak then
//                        InternalCaretXY := BufferCoord(
//                          Length(Lines[CaretY])+1,
//                          CaretY + 1);
//                    end else begin
//                      if Command = ecLineBreak then
//                        InternalCaretXY := BufferCoord(
//                          SpaceCount1+1,
//                          CaretY + 1);
//                    end;
//                  end else begin
//                    Lines.Insert(CaretY - 1, '');
//                    inc(nLinesInserted);
//                    fUndoList.AddChange(crLineBreak, CaretXY, CaretXY, Temp2,
//                      smNormal);
//                    if Command = ecLineBreak then
//                      InternalCaretY := CaretY + 1;
//                  end;
//                end else begin
//                  SpaceCount2 := 0;
//                  BackCounter := CaretY;
//                  if eoAutoIndent in Options then begin
//                    repeat
//                      Dec(BackCounter);
//                      Temp := Lines[BackCounter];
//                      SpaceCount2 := LeftSpaces(Temp);
//                    until (BackCounter = 0) or (Temp <> '');
//                  end;
//                  Lines.Insert(CaretY, '');
//                  inc(nLinesInserted);
//                  Caret := CaretXY;
//                  if Command = ecLineBreak then begin
//                    if SpaceCount2 > 0 then begin
//                      Lines[CaretY] := Copy(Lines[BackCounter], 1, SpaceCount2); // copy previous indent
//                    end;
//                    if (eoAddIndent in Options) and GetHighlighterAttriAtRowCol(BufferCoord(Length(Temp), CaretY),
//                      Temp,
//                      Attr) then begin // only add indent to source files
//                      if Attr <> Highlighter.CommentAttribute then begin // and outside of comments
//                        if Temp[Length(Temp)] in ['{', ':'] then begin // add more indent for these too
//                          if not (eoTabsToSpaces in Options) then begin
//                            Lines[CaretY] := Lines[CaretY] + TSynTabChar;
//                            Inc(SpaceCount2, 1);
//                          end else begin
//                            Lines[CaretY] := Lines[CaretY] + StringOfChar(' ', TabWidth);
//                            Inc(SpaceCount2, TabWidth); // update caret counter
//                          end;
//                        end;
//                      end;
//                    end;
//                    InternalCaretXY := BufferCoord(SpaceCount2 + 1, CaretY + 1);
//                  end;
//                  fUndoList.AddChange(crLineBreak, Caret, Caret, '', smNormal);
//                end;
//              end else begin
//                if fLines.Count = 0 then
//                  fLines.Add('');

//                SpaceCount2 := 0;
//                if eoAutoIndent in Options then begin
//                  BackCounter := CaretY - 1;
//                  while BackCounter >= 0 do begin
//                    SpaceCount2 := LeftSpacesEx(Lines[BackCounter], True);
//                    if Length(Lines[BackCounter]) > 0 then
//                      break;
//                    dec(BackCounter);
//                  end;
//                end;
//                Lines.Insert(CaretY - 1, '');
//                inc(nLinesInserted);
//                fUndoList.AddChange(crLineBreak, CaretXY, CaretXY, '', smNormal);
//                if Command = ecLineBreak then begin
//                  InternalCaretXY := BufferCoord(SpaceCount2 + 1, CaretY + 1);
//                end;
//              end;
//              DoLinesInserted(CaretY - InsDelta, nLinesInserted);
//              BlockBegin := CaretXY;
//              BlockEnd := CaretXY;
//              EnsureCursorPosVisible;
//              UpdateLastCaretX;
//            finally
//              UndoList.EndBlock;
//            end;
//          end;
//        ecTab:
//          if not ReadOnly then
//            DoTabKey;
//        ecShiftTab:
//          if not ReadOnly then
//            DoShiftTabKey;
//        ecComment:
//          DoComment;
//        ecUnComment:
//          DoUncomment;
//        ecToggleComment:
//          if not ReadOnly then begin
//            OrigBlockBegin := BlockBegin;
//            OrigBlockEnd := BlockEnd;

//            BeginIndex := OrigBlockBegin.Line - 1;
//            // Ignore the last line the cursor is placed on
//            if (OrigBlockEnd.Char = 1) and (OrigBlockBegin.Line < OrigBlockEnd.Line) then
//              EndIndex := max(0, OrigBlockEnd.Line - 2)
//            else
//              EndIndex := OrigBlockEnd.Line - 1;

//            // if everything is commented, then uncomment
//            for I := BeginIndex to EndIndex do begin
//              if Pos('//', TrimLeft(fLines[i])) <> 1 then begin // not fully commented
//                DoComment; // comment everything
//                Exit;
//              end;
//            end;
//            DoUncomment;
//          end;
//        ecCommentInline: // toggle inline comment
//          if not ReadOnly and SelAvail then begin
//            Temp := SelText;

//            // Check if the selection starts with /* after blanks
//            StartPos := -1;
//            I := 1;
//            while I <= Length(Temp) do begin
//              if Temp[I] in [#9, #32] then
//                Inc(I)
//              else if ((I + 1) <= Length(Temp)) and (Temp[i] = '/') and (Temp[i + 1] = '*') then begin
//                StartPos := I;
//                break;
//              end else
//                break;
//            end;

//            // Check if the selection ends with /* after blanks
//            EndPos := -1;
//            if StartPos <> -1 then begin
//              I := Length(Temp);
//              while I > 0 do begin
//                if Temp[I] in [#9, #32] then
//                  Dec(I)
//                else if ((I - 1) > 0) and (Temp[i] = '/') and (Temp[i - 1] = '*') then begin
//                  EndPos := I;
//                  break;
//                end else
//                  break;
//              end;
//            end;

//            // Keep selection
//            OrigBlockBegin := BlockBegin;
//            OrigBlockEnd := BlockEnd;

//            // Toggle based on current comment status
//            if (StartPos <> -1) and (EndPos <> -1) then begin
//              SelText := Copy(SelText, StartPos + 2, EndPos - StartPos - 3);
//              BlockBegin := OrigBlockBegin;
//              BlockEnd := BufferCoord(OrigBlockEnd.Char - 4, OrigBlockEnd.Line);
//            end else begin
//              SelText := '/*' + SelText + '*/';
//              BlockBegin := BufferCoord(OrigBlockBegin.Char, OrigBlockBegin.Line);
//              BlockEnd := BufferCoord(OrigBlockEnd.Char + 4, OrigBlockEnd.Line);
//            end;
//          end;
//        ecMatchBracket:
//          FindMatchingBracket;
//        ecChar:
//          // #127 is Ctrl + Backspace, #32 is space
//          if not ReadOnly and (AChar >= #32) and (AChar <> #127) then begin
//            //DoOnPaintTransient(ttBefore);
//            if (InsertMode = False) and (not SelAvail) then begin
//              SelLength := 1;
//            end;

//            if eoAddIndent in Options then begin

//              // Remove TabWidth of indent of the current line when typing a }
//              if AChar in ['}'] then begin
//                temp := Copy(Lines[CaretY-1],1,CaretX-1);
//                // and the first nonblank char is this new }
//                if TrimLeft(temp) = '' then begin
//                  MatchBracketPos := GetPreviousLeftBracket(CaretX, CaretY);
//                  if (MatchBracketPos.Line > 0) then begin
//                    i := 1;
//                    while (i<=Length(Lines[MatchBracketPos.Line-1])) do begin
//                      if  not (Lines[MatchBracketPos.Line-1][i] in [#9,#32]) then
//                        break;
//                      inc(i);
//                    end;
//                    temp := Copy(Lines[MatchBracketPos.Line-1], 1, i-1)
//                      + Copy(Lines[CaretY - 1],CaretX,MaxInt);
//                    Lines[CaretY - 1] := temp;
//                    InternalCaretXY := BufferCoord(i, CaretY);
//                  end;
//                end;
//              end;
//            end;

//            SelText := AChar;

//            //DoOnPaintTransient(ttAfter);
//          end;
//        ecUpperCase,
//          ecLowerCase,
//          ecToggleCase,
//          ecTitleCase,
//          ecUpperCaseBlock,
//          ecLowerCaseBlock,
//          ecToggleCaseBlock:
//          if not ReadOnly then
//            DoCaseChange(Command);
//        ecUndo: begin
//            if not ReadOnly then
//              Undo;
//          end;
//        ecRedo: begin
//            if not ReadOnly then
//              Redo;
//          end;
//        ecGotoMarker0..ecGotoMarker9: begin
//            if BookMarkOptions.EnableKeys then
//              GotoBookMark(Command - ecGotoMarker0);
//          end;
//        ecSetMarker0..ecSetMarker9: begin
//            if BookMarkOptions.EnableKeys then begin
//              CX := Command - ecSetMarker0;
//              if Assigned(Data) then
//                Caret := TBufferCoord(Data^)
//              else
//                Caret := CaretXY;
//              if assigned(fBookMarks[CX]) then begin
//                moveBkm := (fBookMarks[CX].Line <> Caret.Line);
//                ClearBookMark(CX);
//                if moveBkm then
//                  SetBookMark(CX, Caret.Char, Caret.Line);
//              end else
//                SetBookMark(CX, Caret.Char, Caret.Line);
//            end; // if BookMarkOptions.EnableKeys
//          end;
//        ecCut: begin
//            if (not ReadOnly) and SelAvail then
//              CutToClipboard;
//          end;
//        ecCopy: begin
//            CopyToClipboard;
//          end;
//        ecPaste: begin
//            if not ReadOnly then
//              PasteFromClipboard;
//          end;
//        ecScrollUp, ecScrollDown: begin
//            vCaretRow := DisplayY;
//            if (vCaretRow < TopLine) or (vCaretRow >= TopLine + LinesInWindow) then
//              // If the caret is not in view then, like the Delphi editor, move
//              // it in view and do nothing else
//              EnsureCursorPosVisible
//            else begin
//              if Command = ecScrollUp then begin
//                TopLine := TopLine - 1;
//                if vCaretRow > TopLine + LinesInWindow - 1 then
//                  MoveCaretVert((TopLine + LinesInWindow - 1) - vCaretRow, False);
//              end else begin
//                TopLine := TopLine + 1;
//                if vCaretRow < TopLine then
//                  MoveCaretVert(TopLine - vCaretRow, False);
//              end;
//              EnsureCursorPosVisible;
//              Update;
//            end;
//          end;
//        ecScrollLeft: begin
//            LeftChar := LeftChar - 1;
//            // todo: The following code was commented out because it is not MBCS or hard-tab safe.
//            //if CaretX > LeftChar + CharsInWindow then
//            //  InternalCaretX := LeftChar + CharsInWindow;
//            Update;
//          end;
//        ecScrollRight: begin
//            LeftChar := LeftChar + 1;
//            // todo: The following code was commented out because it is not MBCS or hard-tab safe.
//            //if CaretX < LeftChar then
//            //  InternalCaretX := LeftChar;
//            Update;
//          end;
//        ecInsertMode: begin
//            InsertMode := TRUE;
//          end;
//        ecOverwriteMode: begin
//            InsertMode := FALSE;
//          end;
//        ecToggleMode: begin
//            InsertMode := not InsertMode;
//          end;
//        ecBlockIndent:
//          if not ReadOnly then
//            DoBlockIndent;
//        ecBlockUnindent:
//          if not ReadOnly then
//            DoBlockUnindent;
//        ecNormalSelect:
//          SelectionMode := smNormal;
//        ecColumnSelect:
//          SelectionMode := smColumn;
//        ecLineSelect:
//          SelectionMode := smLine;
//        ecContextHelp: begin
//            if Assigned(fOnContextHelp) then
//              fOnContextHelp(self, WordAtCursor);
//          end;
//  {$IFDEF SYN_MBCSSUPPORT}
//        ecImeStr: begin;
//          if not ReadOnly then begin
//            SetString(s, PChar(Data), StrLen(Data));
//            if SelAvail then begin
//            {
//              BeginUndoBlock;
//              try
//                fUndoList.AddChange(crDelete, fBlockBegin, fBlockEnd, selText,
//                  smNormal);
//                StartOfBlock := fBlockBegin;
//                SetSelTextPrimitive(s);
//                fUndoList.AddChange(crInsert, fBlockBegin, fBlockEnd, s,
//                  smNormal);
//              finally
//                EndUndoBlock;
//              end;
//            }
//              SetSelTextExternal(s);
//              InvalidateGutterLines(-1, -1);
//            end else begin
//              Temp := LineText;
//              Len := Length(Temp);
//              if Len < (CaretX-1) then
//                Temp := Temp + StringOfChar(#32, (CaretX-1) - Len);
//              bChangeScroll := not (eoScrollPastEol in fOptions);
//              try
//                if bChangeScroll then
//                  Include(fOptions, eoScrollPastEol);
//                StartOfBlock := CaretXY;
//                // Processing of case character covers on LeadByte.
//                Len := Length(s);
//                if not fInserting then begin
//                  i := (CaretX + Len);
//                  if (ByteType(Temp, i) = mbTrailByte) then begin
//                    s := s + Temp[i - 1];
//                    helper := Copy(Temp, CaretX, Len - 1);
//                  end else
//                    helper := Copy(Temp, CaretX, Len);
//                  Delete(Temp, CaretX, Len);
//                end;
//                Insert(s, Temp, CaretX);
//                InternalCaretX := (CaretX + Len);
//                ProperSetLine(CaretY - 1, Temp);
//                if fInserting then
//                  helper := '';
//                fUndoList.AddChange(crInsert, StartOfBlock, CaretXY, helper,
//                  smNormal);
//                if CaretX >= LeftChar + fCharsInWindow then
//                  LeftChar := LeftChar + min(25, fCharsInWindow - 1);
//              finally
//                if bChangeScroll then
//                  Exclude(fOptions, eoScrollPastEol);
//              end;
//            end;
//          end;
//            if assigned(fOnImeInput) then
//              fOnImeInput(self,s);
//          end;
//  {$ENDIF}
//      end;
//    finally
//      DecPaintLock;
//    end;
//  end;

}

void SynEdit::paintEvent(QPaintEvent *event)
{
    if (mPainterLock>0)
        return;
    if (mPainting)
        return;
    mPainting = true;
    auto action = finally([&,this] {
        mPainting = false;
    });

    // Now paint everything while the caret is hidden.
    QPainter painter(viewport());
    //Get the invalidated rect.
    QRect rcClip = event->rect();
    DisplayCoord coord = displayXY();
    QPoint caretPos = RowColumnToPixels(coord);
    int caretWidth=mCharWidth;
    if (mCaretY <= mLines->count() && mCaretX <= mLines->getString(mCaretY-1).length()) {
        caretWidth = charColumns(mLines->getString(mCaretY-1)[mCaretX-1])*mCharWidth;
    }
    QRect rcCaret(caretPos.x(),caretPos.y(),caretWidth,
                  mTextHeight);

    if (rcCaret == rcClip) {
        // only update caret
        // calculate the needed invalid area for caret
        //qDebug()<<"update caret"<<rcCaret;
        painter.drawImage(rcCaret,*mContentImage,rcCaret);
    } else {
        QRect rcDraw;
        int nL1, nL2, nC1, nC2;
        // Compute the invalid area in lines / columns.
        // columns
        nC1 = mLeftChar;
        if (rcClip.left() > mGutterWidth + 2 )
            nC1 += (rcClip.left() - mGutterWidth - 2 ) / mCharWidth;
        nC2 = mLeftChar +
          (rcClip.right() - mGutterWidth - 2 + mCharWidth - 1) / mCharWidth;
        // lines
        nL1 = MinMax(mTopLine + rcClip.top() / mTextHeight, mTopLine, displayLineCount());
        nL2 = MinMax(mTopLine + (rcClip.bottom() + mTextHeight - 1) / mTextHeight, 1, displayLineCount());

        qDebug()<<"Paint:"<<nL1<<nL2<<nC1<<nC2;

        QPainter cachePainter(mContentImage.get());
        cachePainter.setFont(font());
        SynEditTextPainter textPainter(this, &cachePainter,
                                       nL1,nL2,nC1,nC2);
        // First paint paint the text area if it was (partly) invalidated.
        if (rcClip.right() > mGutterWidth ) {
            rcDraw = rcClip;
            rcDraw.setLeft( std::max(rcDraw.left(), mGutterWidth));
            textPainter.paintTextLines(rcDraw);
        }

        // Then the gutter area if it was (partly) invalidated.
        if (rcClip.left() < mGutterWidth) {
            rcDraw = rcClip;
            rcDraw.setRight(mGutterWidth-1);
            textPainter.paintGutter(rcDraw);
        }

        //PluginsAfterPaint(Canvas, rcClip, nL1, nL2);
        // If there is a custom paint handler call it.
        onPaint(painter);
        doOnPaintTransient(SynTransientType::ttAfter);
        painter.drawImage(rcClip,*mContentImage,rcClip);
    }
    paintCaret(painter, rcCaret);
}

void SynEdit::resizeEvent(QResizeEvent *)
{
    //resize the cache image
    std::shared_ptr<QImage> image = std::make_shared<QImage>(clientWidth(),clientHeight(),
                                                            QImage::Format_ARGB32);
    QRect newRect = image->rect().intersected(mContentImage->rect());

    QPainter painter(image.get());

    painter.drawImage(newRect,*mContentImage);

    mContentImage = image;

    sizeOrFontChanged(false);
}

void SynEdit::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_blinkTimerId) {
        m_blinkStatus = 1- m_blinkStatus;
        updateCaret();
    }
}

bool SynEdit::event(QEvent *event)
{
    switch(event->type()) {
        case QEvent::FontChange:
        synFontChanged();
        break;
    }
    QAbstractScrollArea::event(event);
}

void SynEdit::focusInEvent(QFocusEvent *)
{
    showCaret();
}

void SynEdit::focusOutEvent(QFocusEvent *)
{
    hideCaret();
}

void SynEdit::keyPressEvent(QKeyEvent *event)
{
    SynEditorCommand cmd=TranslateKeyCode(event->key(),event->modifiers());
    if (cmd!=SynEditorCommand::ecNone) {
        CommandProcessor(cmd,QChar(),nullptr);
        event->accept();
    } else if (!event->text().isEmpty()) {
        QChar c = event->text().at(0);
        if (c=='\t' || c.isPrint()) {
            CommandProcessor(SynEditorCommand::ecChar,c,nullptr);
            event->accept();
        }
    }
    if (!event->isAccepted()) {
        QAbstractScrollArea::keyPressEvent(event);
    }
}

void SynEdit::mousePressEvent(QMouseEvent *event)
{
    bool bWasSel = false;
    bool bStartDrag = false;
    bool mMouseMoved = false;
    BufferCoord TmpBegin = mBlockBegin;
    BufferCoord TmpEnd = mBlockEnd;
    Qt::MouseButton button = event->button();
    int X=event->pos().x();
    int Y=event->pos().y();

    if (button == Qt::LeftButton) {
        if (selAvail()) {
            //remember selection state, as it will be cleared later
            bWasSel = true;
            mMouseDownX = X;
            mMouseDownY = Y;
        }
    }
    QAbstractScrollArea::mousePressEvent(event);

    //fKbdHandler.ExecuteMouseDown(Self, Button, Shift, X, Y);

    if (button == Qt::RightButton) {
        if (mOptions.testFlag(eoRightMouseMovesCursor) &&
                ( (selAvail() && ! IsPointInSelection(displayToBufferPos(pixelsToRowColumn(X, Y))))
                  || ! selAvail())) {
            invalidateSelection();
            mBlockEnd=mBlockBegin;
            computeCaret(X,Y);
        }else {
            return;
        }
    } else if (button == Qt::LeftButton) {
        if (selAvail()) {
            //remember selection state, as it will be cleared later
            bWasSel = true;
            mMouseDownX = X;
            mMouseDownY = Y;
        }
        computeCaret(X,Y);
        //I couldn't track down why, but sometimes (and definitely not all the time)
        //the block positioning is lost.  This makes sure that the block is
        //maintained in case they started a drag operation on the block
        mBlockBegin = TmpBegin;
        mBlockEnd = TmpEnd;

        setMouseTracking(true);
        //if mousedown occurred in selected block begin drag operation
        mStateFlags.setFlag(SynStateFlag::sfWaitForDragging,false);
        if (bWasSel && mOptions.testFlag(eoDragDropEditing) && (X >= mGutterWidth + 2)
                && (mSelectionMode == SynSelectionMode::smNormal) && IsPointInSelection(displayToBufferPos(pixelsToRowColumn(X, Y))) ) {
          bStartDrag = true;
        }
        if (bStartDrag) {
            mStateFlags.setFlag(SynStateFlag::sfWaitForDragging);
        } else {
            if (!mStateFlags.testFlag(SynStateFlag::sfDblClicked)) {
                if (event->modifiers() == Qt::ShiftModifier)
                    //BlockBegin and BlockEnd are restored to their original position in the
                    //code from above and SetBlockEnd will take care of proper invalidation
                    setBlockEnd(caretXY());
                else if (mOptions.testFlag(eoAltSetsColumnMode) &&
                         (mActiveSelectionMode != SynSelectionMode::smLine)) {
                    if (event->modifiers() == Qt::AltModifier)
                        setSelectionMode(SynSelectionMode::smColumn);
                    else
                        setSelectionMode(SynSelectionMode::smNormal);
                }
                //Selection mode must be set before calling SetBlockBegin
                setBlockBegin(caretXY());
             }
        }
    }
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
    //QRect iTextArea;
    MaxVal = mLines->lengthOfLongestLine();
    if (mOptions.testFlag(SynEditorOption::eoScrollPastEol)) {
        Value = std::min(Value,MaxVal);
    } else {
        Value = std::min(Value,MaxVal-mCharsInWindow+1);
    }
    if (Value != mLeftChar) {
        horizontalScrollBar()->setValue(Value);
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
        //updateScrollbars();
        verticalScrollBar()->setValue(Value);
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
