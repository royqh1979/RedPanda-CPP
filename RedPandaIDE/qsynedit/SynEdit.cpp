#include "SynEdit.h"
#include <QApplication>
#include <QFontMetrics>
#include <algorithm>

SynEdit::SynEdit(QWidget *parent, Qt::WindowFlags f) : QFrame(parent,f)
{
    mPaintLock = 0;
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

void SynEdit::setCaretXY(const BufferCoord &value)
{
    setCaretXYCentered(false,value);
}

void SynEdit::setCaretXYEx(bool CallEnsureCursorPos, BufferCoord value)
{
    bool vTriggerPaint=true; //how to test it?

    if (vTriggerPaint)
        doOnPaintTransient(SynTransientType::ttBefore);
    int nMaxX = maxScrollWidth() + 1;
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
    if selAvail() then
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
            update(rcInval);
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
        FirstLine = std::max(FirstLine, topLine());
        LastLine = std::min(LastLine, topLine() + linesInWindow());
        // any line visible?
        if (LastLine >= FirstLine) {
            rcInval = {clientLeft(), clientTop()+mTextHeight * (FirstLine - topLine()),
                       mGutterWidth, mTextHeight * (LastLine - topLine() + 1)};
            if (mStateFlags.testFlag(SynStateFlag::sfLinesChanging)) {
                mInvalidateRect =  mInvalidateRect.united(rcInval);
            } else {
                update(rcInval);
            }
        }
    }
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
        BB = blockBegin();
        BE = blockEnd();
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
            mUndoList.AddChange(SynChangeReason::crIndent,
              {BB.Char + Spaces.length(), BB.Line},
              {BE.Char + Spaces.length(), BE.Line},
              "", SynSelectionMode::smColumn);
            //adjust the x position of orgcaretpos appropriately
            OrgCaretPos.Char = X;
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
    return frameRect().width()-2*frameWidth();
}

int SynEdit::clientHeight()
{
    return frameRect().height()-2*frameWidth();
}

int SynEdit::clientTop()
{
    return frameRect().top()+frameWidth();
}

int SynEdit::clientLeft()
{
    return frameRect().left()+frameWidth();
}

QRect SynEdit::clientRect()
{
    return QRect(frameRect().left()+frameWidth(),frameRect().top()+frameWidth(), frameRect().width()-2*frameWidth(), frameRect().height()-2*frameWidth());
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

    updateScrollBars();
    vOldMode = mActiveSelectionMode;
    setBlockBegin(caretXY());
    mActiveSelectionMode = vOldMode;
    update(mInvalidateRect);
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
    update();
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

void SynEdit::linesPutted(int index, int count)
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
          setTopLine(topLine() + mScrollDeltaY * linesInWindow());
        else
          setTopLine(topLine()) + mScrollDeltaY);
        Y = topLine();
        if (mScrollDeltaY > 0)  // scrolling down?
            Y+=linesInWindow() - 1;
        C.Row = MinMax(Y, 1, displayLineCount());
    }
    BufferCoord vCaret = displayToBufferPos(C);
    if ((caretX() <> vCaret.Char) || (caretY() != vCaret.Line)) {
        // changes to line / column in one go
        incPaintLock();
        auto action = finally([this]{
            decPaintLock();
        });
        setInternalCaretXY(vCaret);

          // if MouseCapture is True we're changing selection. otherwise we're dragging
        if (mouseCapture())
            setBlockEnd(caretXY());
    }
    computeScroll(iMousePos.x(), iMousePos.y());
}
