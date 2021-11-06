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
#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QStyleHints>
#include <QMessageBox>

SynEdit::SynEdit(QWidget *parent) : QAbstractScrollArea(parent)
{
    mLastKey = 0;
    mLastKeyModifiers = Qt::NoModifier;
    mModified = false;
    mPaintLock = 0;
    mPainterLock = 0;
    mPainting = false;
    mLines = std::make_shared<SynEditStringList>(this);
    mOrigLines = mLines;
    //fPlugins := TList.Create;
    mMouseMoved = false;
    mUndoing = false;
    mLines->connect(mLines.get(), &SynEditStringList::changed, this, &SynEdit::onLinesChanged);
    mLines->connect(mLines.get(), &SynEditStringList::changing, this, &SynEdit::onLinesChanging);
    mLines->connect(mLines.get(), &SynEditStringList::cleared, this, &SynEdit::onLinesCleared);
    mLines->connect(mLines.get(), &SynEditStringList::deleted, this, &SynEdit::onLinesDeleted);
    mLines->connect(mLines.get(), &SynEditStringList::inserted, this, &SynEdit::onLinesInserted);
    mLines->connect(mLines.get(), &SynEditStringList::putted, this, &SynEdit::onLinesPutted);

#ifdef Q_OS_WIN
    mFontDummy = QFont("Consolas",12);
#elif Q_OS_LINUX
    mFontDummy = QFont("terminal",14);
#else
#error "Not supported!"
#endif
    mFontDummy.setStyleStrategy(QFont::PreferAntialias);
    setFont(mFontDummy);

    mUndoList = std::make_shared<SynEditUndoList>();
    mUndoList->connect(mUndoList.get(), &SynEditUndoList::addedUndo, this, &SynEdit::onUndoAdded);
    mOrigUndoList = mUndoList;
    mRedoList = std::make_shared<SynEditUndoList>();
    mRedoList->connect(mRedoList.get(), &SynEditUndoList::addedUndo, this, &SynEdit::onRedoAdded);
    mOrigRedoList = mRedoList;

    mCaretColor = QColorConstants::Red;
    mCaretUseTextColor = false;
    mActiveLineColor = QColorConstants::Svg::lightblue;
    mSelectedBackground = palette().color(QPalette::Highlight);
    mSelectedForeground = palette().color(QPalette::HighlightedText);

    mBookMarkOpt.connect(&mBookMarkOpt, &SynBookMarkOpt::changed, this, &SynEdit::onBookMarkOptionsChanged);
    //  fRightEdge has to be set before FontChanged is called for the first time
    mRightEdge = 80;

    mMouseWheelScrollSpeed = 1;

    mGutter.setRightOffset(21);
    mGutter.connect(&mGutter, &SynGutter::changed, this, &SynEdit::onGutterChanged);
    mGutterWidth = mGutter.realGutterWidth(charWidth());
    //ControlStyle := ControlStyle + [csOpaque, csSetCaption, csNeedsBorderPaint];
    //Height := 150;
    //Width := 200;
    this->setCursor(Qt::CursorShape::IBeamCursor);
    //TabStop := True;
    mInserting = true;
    mScrollBars = SynScrollStyle::ssBoth;
    mExtraLineSpacing = 0;

    this->setFrameShape(QFrame::Panel);
    this->setFrameShadow(QFrame::Sunken);
    this->setLineWidth(1);
    mInsertCaret = SynEditCaretType::ctVerticalLine;
    mOverwriteCaret = SynEditCaretType::ctBlock;
    mSelectionMode = SynSelectionMode::smNormal;
    mActiveSelectionMode = SynSelectionMode::smNormal;
    mReadOnly = false;

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
    mLastCaretColumn = 1;
    mCaretY = 1;
    mBlockBegin.Char = 1;
    mBlockBegin.Line = 1;
    mBlockEnd = mBlockBegin;
    mOptions = eoAutoIndent
            | eoDragDropEditing | eoEnhanceEndKey | eoTabIndent |
             eoGroupUndo | eoKeepCaretX | eoSelectWordByDblClick;

    mScrollTimer = new QTimer(this);
    mScrollTimer->setInterval(100);
    connect(mScrollTimer, &QTimer::timeout,this, &SynEdit::onScrollTimeout);

    mScrollHintColor = QColorConstants::Yellow;
    mScrollHintFormat = SynScrollHintFormat::shfTopLineOnly;

    mContentImage = std::make_shared<QImage>(clientWidth(),clientHeight(),QImage::Format_ARGB32);

    mUseCodeFolding = true;
    m_blinkTimerId = 0;
    m_blinkStatus = 0;

    synFontChanged();

    hideCaret();

    connect(horizontalScrollBar(),&QScrollBar::valueChanged,
            this, &SynEdit::onScrolled);
    connect(verticalScrollBar(),&QScrollBar::valueChanged,
            this, &SynEdit::onScrolled);
    //enable input method
    setAttribute(Qt::WA_InputMethodEnabled);

    //setMouseTracking(true);
}

int SynEdit::displayLineCount() const
{
    if (mLines->empty()) {
        return 0;
    }
    return lineToRow(mLines->count());
}

DisplayCoord SynEdit::displayXY() const
{
    return bufferToDisplayPos(caretXY());
}

int SynEdit::displayX() const
{
    return displayXY().Column;
}

int SynEdit::displayY() const
{
    return displayXY().Row;
}

BufferCoord SynEdit::caretXY() const
{
    BufferCoord result;
    result.Char = caretX();
    result.Line = caretY();
    return result;
}

int SynEdit::caretX() const
{
    return mCaretX;
}

int SynEdit::caretY() const
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
    int nMaxX;
    if (value.Line > mLines->count())
        value.Line = mLines->count();
    if (value.Line < 1) {
        // this is just to make sure if Lines stringlist should be empty
        value.Line = 1;
        if (!mOptions.testFlag(SynEditorOption::eoScrollPastEol)) {
            nMaxX = 1;
        } else {
            nMaxX = mLines->getString(value.Line-1).length()+1;
        }
    } else {
        nMaxX = mLines->getString(value.Line-1).length()+1;
    }
    value.Char = std::min(value.Char,nMaxX);
    value.Char = std::max(value.Char,1);
//    if ((value.Char > nMaxX) && (! (mOptions.testFlag(SynEditorOption::eoScrollPastEol)) ) )
//        value.Char = nMaxX;
//    if (value.Char < 1)
//        value.Char = 1;
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

void SynEdit::uncollapseAroundLine(int line)
{
    while (true) { // Open up the closed folds around the focused line until we can see the line we're looking for
      PSynEditFoldRange fold = foldHidesLine(line);
      if (fold)
          uncollapse(fold);
      else
          break;
    }
}

PSynEditFoldRange SynEdit::foldHidesLine(int line)
{
    return foldAroundLineEx(line, true, false, true);
}

void SynEdit::setInsertMode(bool value)
{
    if (mInserting != value) {
        mInserting = value;
        updateCaret();
        emit statusChanged(scInsertMode);
    }
}

bool SynEdit::insertMode() const
{
    return mInserting;
}

bool SynEdit::canUndo() const
{
    return !mReadOnly && mUndoList->CanUndo();
}

bool SynEdit::canRedo() const
{
    return !mReadOnly && mRedoList->CanUndo();
}

int SynEdit::maxScrollWidth() const
{
    if (mOptions.testFlag(eoScrollPastEol))
        return std::max(mLines->lengthOfLongestLine(),1);
    else
        return std::max(mLines->lengthOfLongestLine()-mCharsInWindow+1, 1);
}

bool SynEdit::getHighlighterAttriAtRowCol(const BufferCoord &XY, QString &Token, PSynHighlighterAttribute &Attri)
{
    SynHighlighterTokenType TmpType;
    int TmpKind, TmpStart;
    return getHighlighterAttriAtRowColEx(XY, Token, TmpType, TmpKind,TmpStart, Attri);
}

bool SynEdit::getHighlighterAttriAtRowCol(const BufferCoord &XY, QString &Token, bool &tokenFinished, SynHighlighterTokenType &TokenType, PSynHighlighterAttribute &Attri)
{
    int PosX, PosY, endPos, Start;
    QString Line;
    PosY = XY.Line - 1;
    if (mHighlighter && (PosY >= 0) && (PosY < mLines->count())) {
        Line = mLines->getString(PosY);
        if (PosY == 0) {
            mHighlighter->resetState();
        } else {
            mHighlighter->setState(mLines->ranges(PosY-1));
        }
        mHighlighter->setLine(Line, PosY);
        PosX = XY.Char;
        if ((PosX > 0) && (PosX <= Line.length())) {
            while (!mHighlighter->eol()) {
                Start = mHighlighter->getTokenPos() + 1;
                Token = mHighlighter->getToken();
                endPos = Start + Token.length()-1;
                if ((PosX >= Start) && (PosX <= endPos)) {
                    Attri = mHighlighter->getTokenAttribute();
                    if (PosX == endPos)
                        tokenFinished = mHighlighter->getTokenFinished();
                    else
                        tokenFinished = false;
                    TokenType = mHighlighter->getTokenType();
                    return true;
                }
                mHighlighter->next();
            }
        }
    }
    Token = "";
    Attri = PSynHighlighterAttribute();
    tokenFinished = false;
    return false;
}

bool SynEdit::getHighlighterAttriAtRowColEx(const BufferCoord &XY, QString &Token, SynHighlighterTokenType &TokenType, SynTokenKind &TokenKind, int &Start, PSynHighlighterAttribute &Attri)
{
    int PosX, PosY, endPos;
    QString Line;
    PosY = XY.Line - 1;
    if (mHighlighter && (PosY >= 0) && (PosY < mLines->count())) {
        Line = mLines->getString(PosY);
        if (PosY == 0) {
            mHighlighter->resetState();
        } else {
            mHighlighter->setState(mLines->ranges(PosY-1));
        }
        mHighlighter->setLine(Line, PosY);
        PosX = XY.Char;
        if ((PosX > 0) && (PosX <= Line.length())) {
            while (!mHighlighter->eol()) {
                Start = mHighlighter->getTokenPos() + 1;
                Token = mHighlighter->getToken();
                endPos = Start + Token.length()-1;
                if ((PosX >= Start) && (PosX <= endPos)) {
                    Attri = mHighlighter->getTokenAttribute();
                    TokenKind = mHighlighter->getTokenKind();
                    TokenType = mHighlighter->getTokenType();
                    return true;
                }
                mHighlighter->next();
            }
        }
    }
    Token = "";
    Attri = PSynHighlighterAttribute();
    TokenKind = 0;
    TokenType = SynHighlighterTokenType::Default;
    return false;
}

void SynEdit::beginUpdate()
{
    incPaintLock();
}

void SynEdit::endUpdate()
{
    decPaintLock();
}

BufferCoord SynEdit::getMatchingBracket()
{
    return getMatchingBracketEx(caretXY());
}

BufferCoord SynEdit::getMatchingBracketEx(BufferCoord APoint)
{
    QChar Brackets[] = {'(', ')', '[', ']', '{', '}', '<', '>'};
    QString Line;
    int i, PosX, PosY, Len;
    QChar Test, BracketInc, BracketDec;
    int NumBrackets;
    QString vDummy;
    PSynHighlighterAttribute attr;
    BufferCoord p;
    bool isCommentOrStringOrChar;
    int nBrackets = sizeof(Brackets) / sizeof(QChar);

    if (mLines->count()<1)
        return BufferCoord{0,0};
    if (!mHighlighter)
        return BufferCoord{0,0};
    // get char at caret
    PosX = std::max(APoint.Char,1);
    PosY = std::max(APoint.Line,1);
    Line = mLines->getString(APoint.Line - 1);
    if (Line.length() >= PosX ) {
        Test = Line[PosX-1];
        // is it one of the recognized brackets?
        for (i = 0; i<nBrackets; i++) {
            if (Test == Brackets[i]) {
                // this is the bracket, get the matching one and the direction
                BracketInc = Brackets[i];
                BracketDec = Brackets[i ^ 1]; // 0 -> 1, 1 -> 0, ...
                // search for the matching bracket (that is until NumBrackets = 0)
                NumBrackets = 1;
                if (i%2==1) {
                    while (true) {
                        // search until start of line
                        while (PosX > 1) {
                            PosX--;
                            Test = Line[PosX-1];
                            p.Char = PosX;
                            p.Line = PosY;
                            if ((Test == BracketInc) || (Test == BracketDec)) {
                                if (getHighlighterAttriAtRowCol(p, vDummy, attr))
                                    isCommentOrStringOrChar =
                                        (attr == mHighlighter->stringAttribute()) ||
                                            (attr == mHighlighter->commentAttribute()) ||
                                            (attr->name() == SYNS_AttrCharacter);
                                else
                                    isCommentOrStringOrChar = false;
                                if ((Test == BracketInc) && (!isCommentOrStringOrChar))
                                    NumBrackets++;
                                else if ((Test == BracketDec) && (!isCommentOrStringOrChar)) {
                                    NumBrackets--;
                                    if (NumBrackets == 0) {
                                        // matching bracket found, set caret and bail out
                                        return p;
                                    }
                                }
                            }
                        }
                        // get previous line if possible
                        if (PosY == 1)
                            break;
                        PosY--;
                        Line = mLines->getString(PosY - 1);
                        PosX = Line.length() + 1;
                    }
                } else {
                    while (true) {
                        // search until end of line
                        Len = Line.length();
                        while (PosX < Len) {
                            PosX++;
                            Test = Line[PosX-1];
                            p.Char = PosX;
                            p.Line = PosY;
                            if ((Test == BracketInc) || (Test == BracketDec)) {
                                if (getHighlighterAttriAtRowCol(p, vDummy, attr))
                                    isCommentOrStringOrChar =
                                        (attr == mHighlighter->stringAttribute()) ||
                                            (attr == mHighlighter->commentAttribute()) ||
                                            (attr->name() == SYNS_AttrCharacter);
                                else
                                    isCommentOrStringOrChar = false;
                                if ((Test == BracketInc) && (!isCommentOrStringOrChar))
                                    NumBrackets++;
                                else if ((Test == BracketDec) && (!isCommentOrStringOrChar)) {
                                    NumBrackets--;
                                    if (NumBrackets == 0) {
                                        // matching bracket found, set caret and bail out
                                        return p;
                                    }
                                }
                            }
                        }
                        // get next line if possible
                        if (PosY == mLines->count())
                            break;
                        PosY++;
                        Line = mLines->getString(PosY - 1);
                        PosX = 0;
                    }
                }
                // don't test the other brackets, we're done
                break;
            }
        }
    }
    return BufferCoord{0,0};
}

QStringList SynEdit::contents()
{
    return lines()->contents();
}

QString SynEdit::text()
{
    return lines()->text();
}

bool SynEdit::getPositionOfMouse(BufferCoord &aPos)
{
    QPoint point = QCursor::pos();
    point = mapFromGlobal(point);
    return pointToCharLine(point,aPos);
}

bool SynEdit::getLineOfMouse(int &line)
{
    QPoint point = QCursor::pos();
    point = mapFromGlobal(point);
    return pointToLine(point,line);
}

bool SynEdit::pointToCharLine(const QPoint &point, BufferCoord &coord)
{
    // Make sure it fits within the SynEdit bounds (and on the gutter)
    if ((point.x() < gutterWidth() + clientLeft())
            || (point.x()>clientWidth()+clientLeft())
            || (point.y() < clientTop())
            || (point.y() > clientTop()+clientHeight())) {
        return false;
    }

    coord = displayToBufferPos(pixelsToRowColumn(point.x(),point.y()));
    return true;
}

bool SynEdit::pointToLine(const QPoint &point, int &line)
{
    // Make sure it fits within the SynEdit bounds
    if ((point.x() < clientLeft())
            || (point.x()>clientWidth()+clientLeft())
            || (point.y() < clientTop())
            || (point.y() > clientTop()+clientHeight())) {
        return false;
    }

    BufferCoord coord = displayToBufferPos(pixelsToRowColumn(point.x(),point.y()));
    line = coord.Line;
    return true;
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
DisplayCoord SynEdit::pixelsToNearestRowColumn(int aX, int aY) const
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
      std::max(1, (int)(leftChar() + round(f))),
      std::max(1, mTopLine + (aY / mTextHeight))
    };
}

DisplayCoord SynEdit::pixelsToRowColumn(int aX, int aY) const
{
    return {
        std::max(1, mLeftChar + ((aX - mGutterWidth - 2) / mCharWidth)),
        std::max(1, mTopLine + (aY / mTextHeight))
    };
}

QPoint SynEdit::rowColumnToPixels(const DisplayCoord &coord) const
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
DisplayCoord SynEdit::bufferToDisplayPos(const BufferCoord &p) const
{
    DisplayCoord result {p.Char,p.Line};
    // Account for tabs and charColumns
    if (p.Line-1 <mLines->count())
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
BufferCoord SynEdit::displayToBufferPos(const DisplayCoord &p) const
{
    BufferCoord Result{p.Column,p.Row};
    // Account for code folding
    if (mUseCodeFolding)
        Result.Line = foldRowToLine(p.Row);
    // Account for tabs
    if (Result.Line <= mLines->count() ) {
        Result.Char = columnToChar(Result.Line,p.Column);
    }
    return Result;
}

ContentsCoord SynEdit::fromBufferCoord(const BufferCoord &p) const
{
    return createNormalizedBufferCoord(p.Char,p.Line);
}

ContentsCoord SynEdit::createNormalizedBufferCoord(int aChar, int aLine) const
{
    return ContentsCoord(this,aChar,aLine);
}

QStringList SynEdit::getContents(const ContentsCoord &pStart, const ContentsCoord &pEnd)
{
    QStringList result;
    if (mLines->count()==0)
        return result;
    if (pStart.line()>0) {
        QString s = mLines->getString(pStart.line()-1);
        result += s.mid(pStart.ch()-1);
    }
    int endLine = std::min(pEnd.line(),mLines->count());
    for (int i=pStart.line();i<endLine-1;i++) {
        result += mLines->getString(i);
    }
    if (pEnd.line()<=mLines->count()) {
        result += mLines->getString(pEnd.line()-1).mid(0,pEnd.ch()-1);
    }
    return result;
}

QString SynEdit::getJoinedContents(const ContentsCoord &pStart, const ContentsCoord &pEnd, const QString &joinStr)
{
    return getContents(pStart,pEnd).join(joinStr);
}

int SynEdit::leftSpaces(const QString &line) const
{
    int result = 0;
    if (mOptions.testFlag(eoAutoIndent)) {
        for (QChar ch:line) {
            if (ch == '\t') {
                result += mTabWidth - (result % mTabWidth);
            } else if (ch == ' ') {
                result ++;
            } else {
                break;
            }
        }
    }
    return result;
}

QString SynEdit::GetLeftSpacing(int charCount, bool wantTabs) const
{
    if (wantTabs && !mOptions.testFlag(eoTabsToSpaces)) {
        return QString(charCount / mTabWidth,'\t') + QString(charCount % mTabWidth,' ');
    } else {
        return QString(charCount,' ');
    }
}

int SynEdit::charToColumn(int aLine, int aChar) const
{
    if (aLine>=1 && aLine <= mLines->count()) {
        QString s = mLines->getString(aLine - 1);
        return charToColumn(s,aChar);
    }
    return aChar;
}

int SynEdit::charToColumn(const QString &s, int aChar) const
{
    int x = 0;
    int len = std::min(aChar-1,s.length());
    for (int i=0;i<len;i++) {
        if (s[i] == '\t')
            x+=mTabWidth - (x % mTabWidth);
        else
            x+=charColumns(s[i]);
    }
    return x+1;
}

int SynEdit::columnToChar(int aLine, int aColumn) const
{
    Q_ASSERT( (aLine <= mLines->count()) && (aLine >= 1));
    if (aLine <= mLines->count()) {
        QString s = mLines->getString(aLine - 1);
        int x = 0;
        int len = s.length();
        int i;
        for (i=0;i<len;i++) {
            if (s[i] == '\t')
                x+=mTabWidth - (x % mTabWidth);
            else
                x+=charColumns(s[i]);
            if (x>=aColumn) {
                break;
            }
        }
        return i+1;
    }
    return aColumn;
}

int SynEdit::stringColumns(const QString &line, int colsBefore) const
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

int SynEdit::getLineIndent(const QString &line) const
{
    int indents = 0;
    for (QChar ch:line) {
        switch(ch.unicode()) {
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

int SynEdit::rowToLine(int aRow) const
{
    if (mUseCodeFolding)
        return foldRowToLine(aRow);
    else
        return aRow;
    //return displayToBufferPos({1, aRow}).Line;
}

int SynEdit::lineToRow(int aLine) const
{
    return bufferToDisplayPos({1, aLine}).Row;
}

int SynEdit::foldRowToLine(int Row) const
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

int SynEdit::foldLineToRow(int Line) const
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
    if (Line<1 || (Line>mLines->count() &&
                   Line!=1) || !isVisible())
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
    invalidateLines(blockBegin().Line, blockEnd().Line);
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

bool SynEdit::selAvail() const
{
    return (mBlockBegin.Char != mBlockEnd.Char) ||
            ((mBlockBegin.Line != mBlockEnd.Line) && (mActiveSelectionMode != SynSelectionMode::smColumn));
}

QString SynEdit::wordAtCursor()
{
    return wordAtRowCol(caretXY());
}

QString SynEdit::wordAtRowCol(const BufferCoord &XY)
{
    if ((XY.Line >= 1) && (XY.Line <= mLines->count())) {
        QString line = mLines->getString(XY.Line - 1);
        int Len = line.length();
        if (Len == 0)
            return "";
        if (XY.Char<1 || XY.Char>Len)
            return "";

        int start = XY.Char - 1;
        if  ((start> 0) && !isIdentChar(line[start]))
             start--;

        if (isIdentChar(line[start])) {
            int stop = start;
            while ((stop < Len) && isIdentChar(line[stop]))
                stop++;
            while ((start-1 >=0) && isIdentChar(line[start - 1]))
                start--;
            if (stop > start)
                return line.mid(start,stop-start);
        }
    }
    return "";
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

void SynEdit::processGutterClick(QMouseEvent *event)
{
    int X = event->pos().x();
    int Y = event->pos().y();
    DisplayCoord RowColumn = pixelsToRowColumn(X, Y);
    int Line = rowToLine(RowColumn.Row);

    // Check if we clicked on a folding thing
    if (mUseCodeFolding) {
        PSynEditFoldRange FoldRange = foldStartAtLine(Line);
        if (FoldRange) {
            // See if we actually clicked on the rectangle...
            //rect.Left := Gutter.RealGutterWidth(CharWidth) - Gutter.RightOffset;
            QRect rect;
            rect.setLeft(mGutterWidth - mGutter.rightOffset());
            rect.setRight(rect.left() + mGutter.rightOffset() - 4);
            rect.setTop((RowColumn.Row - mTopLine) * mTextHeight);
            rect.setBottom(rect.top() + mTextHeight - 1);
            if (rect.contains(QPoint(X, Y))) {
                if (FoldRange->collapsed)
                    uncollapse(FoldRange);
                else
                    collapse(FoldRange);
                return;
            }
        }
    }

    // If not, check gutter marks
    if (Line>=1 && Line <= mLines->count()) {
        emit gutterClicked(event->button(),X,Y,Line);
    }
}

void SynEdit::clearUndo()
{
    mUndoList->Clear();
    mRedoList->Clear();
}

int SynEdit::findIndentsStartLine(int line, QVector<int> indents)
{
    line--;
    if (line<0 || line>=mLines->count())
        return -1;
    while (line>=1) {
        SynRangeState range = mLines->ranges(line);
        QVector<int> newIndents = range.indents.mid(range.firstIndentThisLine);
        int i = 0;
        int len = indents.length();
        while (i<len && !newIndents.isEmpty()) {
            int indent = indents[i];
            int idx = newIndents.lastIndexOf(indent);
            if (idx >=0) {
                newIndents.remove(idx,newIndents.size());
            } else {
                break;
            }
            i++;
        }
        if (i>=len) {
            return line+1;
        } else {
            indents = range.matchingIndents + indents.mid(i);
        }
        line--;
    }
    return -1;
}

BufferCoord SynEdit::getPreviousLeftBrace(int x, int y)
{
    QChar Test;
    QString vDummy;
    PSynHighlighterAttribute attr;
    BufferCoord p;
    bool isCommentOrStringOrChar;
    BufferCoord Result{0,0};
    // get char at caret
    int PosX = x-1;
    int PosY = y;
    if (PosX<1)
        PosY--;
    if (PosY<1 )
        return Result;
    QString Line = mLines->getString(PosY - 1);
    if ((PosX > Line.length()) || (PosX<1))
        PosX = Line.length();
    int numBrackets = 1;
    while (true) {
        if (Line.isEmpty()){
            PosY--;
            if (PosY<1)
                return Result;
            Line = mLines->getString(PosY - 1);
            PosX = Line.length();
            continue;
        }
        Test = Line[PosX-1];
        p.Char = PosX;
        p.Line = PosY;
        if (Test=='{' || Test == '}') {
            if (getHighlighterAttriAtRowCol(p, vDummy, attr)) {
                isCommentOrStringOrChar =
                        (attr == mHighlighter->stringAttribute()) ||
                        (attr == mHighlighter->commentAttribute()) ||
                        (attr->name() == SYNS_AttrCharacter);
            } else
                isCommentOrStringOrChar = false;
            if ((Test == '{') && (! isCommentOrStringOrChar))
                numBrackets--;
            else if ((Test == '}') && (!isCommentOrStringOrChar))
                numBrackets++;
            if (numBrackets == 0) {
                return p;
            }
        }
        PosX--;
        if (PosX<1) {
            PosY--;
            if (PosY<1)
                return Result;
            Line = mLines->getString(PosY - 1);
            PosX = Line.length();
        }
    }
}

int SynEdit::charColumns(QChar ch) const
{
    if (ch == ' ')
        return 1;
    //return std::ceil((int)(fontMetrics().horizontalAdvance(ch) * dpiFactor()) / (double)mCharWidth);
    return std::ceil((int)(fontMetrics().horizontalAdvance(ch)) / (double)mCharWidth);
}

double SynEdit::dpiFactor() const
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

bool SynEdit::isPointInSelection(const BufferCoord &Value) const
{
    BufferCoord ptBegin = blockBegin();
    BufferCoord ptEnd = blockEnd();
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

BufferCoord SynEdit::nextWordPos()
{
    return nextWordPosEx(caretXY());
}

BufferCoord SynEdit::nextWordPosEx(const BufferCoord &XY)
{
    int CX = XY.Char;
    int CY = XY.Line;
    // valid line?
    if ((CY >= 1) && (CY <= mLines->count())) {
        QString Line = mLines->getString(CY - 1);
        int LineLen = Line.length();
        if (CX >= LineLen) {
            // find first IdentChar or multibyte char in the next line
            if (CY < mLines->count()) {
                Line = mLines->getString(CY);
                CY++;
                CX=StrScanForWordChar(Line,1);
                if (CX==0)
                    CX=1;
            }
        } else {
            // find next "whitespace" if current char is an IdentChar
            if (!Line[CX-1].isSpace())
                CX = StrScanForNonWordChar(Line,CX);
            // if "whitespace" found, find the next IdentChar
            if (CX > 0)
                CX = StrScanForWordChar(Line, CX);
            // if one of those failed position at the begin of next line
            if (CX == 0) {
                if (CY < mLines->count()) {
                    Line = mLines->getString(CY);
                    CY++;
                    CX=StrScanForWordChar(Line,1);
                    if (CX==0)
                        CX=1;
                } else {
                    CX=Line.length()+1;
                }
            }
        }
    }
    return BufferCoord{CX,CY};
}

BufferCoord SynEdit::wordStart()
{
    return wordStartEx(caretXY());
}

BufferCoord SynEdit::wordStartEx(const BufferCoord &XY)
{
    int CX = XY.Char;
    int CY = XY.Line;
    // valid line?
    if ((CY >= 1) && (CY <= mLines->count())) {
        QString Line = mLines->getString(CY - 1);
        CX = std::min(CX, Line.length()+1);
        if (CX > 1) {
            if (isWordChar(Line[CX - 2]))
                CX = StrRScanForNonWordChar(Line, CX - 1) + 1;
        }
    }
    return BufferCoord{CX,CY};
}

BufferCoord SynEdit::wordEnd()
{
    return wordEndEx(caretXY());
}

BufferCoord SynEdit::wordEndEx(const BufferCoord &XY)
{
    int CX = XY.Char;
    int CY = XY.Line;
    // valid line?
    if ((CY >= 1) && (CY <= mLines->count())) {
        QString Line = mLines->getString(CY - 1);
        if (CX <= Line.length() && CX-2>=0) {
            if (isWordChar(Line[CX - 2]))
                CX = StrScanForNonWordChar(Line, CX);
            if (CX == 0)
                CX = Line.length() + 1;
        }
    }
    return BufferCoord{CX,CY};
}

BufferCoord SynEdit::prevWordPos()
{
    return prevWordPosEx(caretXY());
}

BufferCoord SynEdit::prevWordPosEx(const BufferCoord &XY)
{
    int CX = XY.Char;
    int CY = XY.Line;
    // valid line?
    if ((CY >= 1) && (CY <= mLines->count())) {
        QString Line = mLines->getString(CY - 1);
        CX = std::min(CX, Line.length());
        if (CX <= 1) {
            // find last IdentChar in the previous line
            if (CY > 1) {
                CY -- ;
                Line = mLines->getString(CY - 1);
                CX = StrRScanForWordChar(Line, Line.length())+1;
            }
        } else {
            // if previous char is a "whitespace" search for the last IdentChar
            if (Line[CX - 2].isSpace())
                CX = StrRScanForWordChar(Line, CX - 1);
            if (CX > 0) // search for the first IdentChar of this "word"
                CX = StrRScanForNonWordChar(Line, CX - 1)+1;
            if (CX == 0) {
                // find last IdentChar in the previous line
                if (CY > 1) {
                    CY -- ;
                    Line = mLines->getString(CY - 1);
                    CX = StrRScanForWordChar(Line, Line.length())+1;
                } else {
                    CX = 1;
                }
            }
        }
    }
    return BufferCoord{CX,CY};
}

void SynEdit::setSelWord()
{
    setWordBlock(caretXY());
}

void SynEdit::setWordBlock(BufferCoord Value)
{
//    if (mOptions.testFlag(eoScrollPastEol))
//        Value.Char =
//    else
//        Value.Char = std::max(Value.Char, 1);
    Value.Line = minMax(Value.Line, 1, mLines->count());
    Value.Char = std::max(Value.Char, 1);
    QString TempString = mLines->getString(Value.Line - 1); //needed for CaretX = LineLength +1
    if (Value.Char > TempString.length()) {
        internalSetCaretXY(BufferCoord{TempString.length()+1, Value.Line});
        return;
    }

    BufferCoord v_WordStart = wordStartEx(Value);
    BufferCoord v_WordEnd = wordEndEx(Value);
    if ((v_WordStart.Line == v_WordEnd.Line) && (v_WordStart.Char < v_WordEnd.Char))
        setCaretAndSelection(v_WordEnd, v_WordStart, v_WordEnd);
}

int SynEdit::calcIndentSpaces(int line, const QString& lineText, bool addIndent)
{
    if (!mHighlighter)
        return 0;
    line = std::min(line, mLines->count()+1);
    if (line<=1)
        return 0;
    // find the first non-empty preceeding line
    int startLine = line-1;
    QString s;
    while (startLine>=1) {
        s = mLines->getString(startLine-1);
        if (!s.trimmed().isEmpty()) {
            break;
        }
        startLine -- ;
    }
    int indentSpaces = 0;
    if (startLine>=1) {
        indentSpaces = leftSpaces(s);
        SynRangeState rangePreceeding = mLines->ranges(startLine-1);
        mHighlighter->setState(rangePreceeding);
        if (addIndent
                && !mHighlighter->isLastLineCommentNotFinished(rangePreceeding.state)
                && !mHighlighter->isLastLineStringNotFinished(rangePreceeding.state)) {
            mHighlighter->setLine(lineText.trimmed(),line-1);
            SynRangeState rangeAfterFirstToken = mHighlighter->getRangeState();
            QString firstToken = mHighlighter->getToken();
            PSynHighlighterAttribute attr = mHighlighter->getTokenAttribute();
            if (attr == mHighlighter->keywordAttribute()
                                  &&  lineText.endsWith(':')
                                  && (
                                  firstToken == "public" || firstToken == "private"
                                  || firstToken == "protected" || firstToken == "case")) {
                mHighlighter->setState(rangePreceeding);
                mHighlighter->setLine("}",line-1);
                rangeAfterFirstToken = mHighlighter->getRangeState();
                firstToken = mHighlighter->getToken();
                attr = mHighlighter->getTokenAttribute();
            }
            bool dontAddIndent = false;
            QVector<int> matchingIndents;
            int l;
            if (attr == mHighlighter->symbolAttribute()
                    && firstToken == '}' ) {
                matchingIndents = rangeAfterFirstToken.matchingIndents;
                dontAddIndent = true;
                l = startLine;
            } else {
                matchingIndents = rangePreceeding.matchingIndents;
                l = startLine-1;
            }

            if (!matchingIndents.isEmpty()
                    ) {
                // find the indent's start line, and use it's indent as the default indent;
                while (l>=1) {
                    SynRangeState range = mLines->ranges(l-1);
                    QVector<int> newIndents = range.indents.mid(range.firstIndentThisLine);
                    int i = 0;
                    int len = matchingIndents.length();
                    while (i<len && !newIndents.isEmpty()) {
                        int indent = matchingIndents[i];
                        int idx = newIndents.lastIndexOf(indent);
                        if (idx >=0) {
                            newIndents.remove(idx,newIndents.length()-idx);
                        } else {
                            break;
                        }
                        i++;
                    }
                    if (i>=len) {
                        // we found the where the indent started
                        if (len>0 && !range.matchingIndents.isEmpty()
                                &&
                                ( matchingIndents.back()== sitBrace
                                  || matchingIndents.back() == sitStatement
                                ) ) {
                            // but it's not a complete statement
                            matchingIndents = range.matchingIndents;
                        } else {
                            indentSpaces = leftSpaces(mLines->getString(l-1));
                            if (newIndents.length()>0)
                                indentSpaces+=mTabWidth;
                            break;
                        }
                    } else {
                        matchingIndents = range.matchingIndents + matchingIndents.mid(i);
                    }
                    l--;
                }
            }
            if (!dontAddIndent) {
                if (rangePreceeding.firstIndentThisLine < rangePreceeding.indents.length()) {
                    indentSpaces += mTabWidth;
                    dontAddIndent = true;
                }
            }

            if (!dontAddIndent && !s.isEmpty()) {
                BufferCoord coord;
                QString token;
                PSynHighlighterAttribute attr;
                coord.Line = startLine;
                coord.Char = lines()->getString(startLine-1).length();
                if (getHighlighterAttriAtRowCol(coord,token,attr)
                        && attr == mHighlighter->symbolAttribute()
                        && token == ":") {
                    indentSpaces += mTabWidth;
                    dontAddIndent = true;
                }
            }
        }
    }
    return std::max(0,indentSpaces);
}

void SynEdit::doSelectAll()
{
    BufferCoord LastPt;
    LastPt.Char = 1;
    if (mLines->empty()) {
        LastPt.Line = 1;
    } else {
        LastPt.Line = mLines->count();
        LastPt.Char = mLines->getString(LastPt.Line-1).length()+1;
    }
    setCaretAndSelection(caretXY(), BufferCoord{1, 1}, LastPt);
    // Selection should have changed...
    emit statusChanged(SynStatusChange::scSelection);
}

void SynEdit::doComment()
{
    BufferCoord origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    if (mReadOnly)
        return;
    doOnPaintTransient(SynTransientType::ttBefore);
    mUndoList->BeginBlock();
    auto action = finally([this]{
        mUndoList->EndBlock();
    });
    origBlockBegin = blockBegin();
    origBlockEnd = blockEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.Char == 1)
        endLine = std::max(origBlockBegin.Line - 1, origBlockEnd.Line - 2);
    else
        endLine = origBlockEnd.Line - 1;
    for (int i = origBlockBegin.Line - 1; i<=endLine; i++) {
        mLines->putString(i, "//" + mLines->getString(i));
        mUndoList->AddChange(SynChangeReason::crInsert,
              BufferCoord{1, i + 1},
              BufferCoord{3, i + 1},
              "", SynSelectionMode::smNormal);
    }
    // When grouping similar commands, process one comment action per undo/redo
    mUndoList->AddChange(SynChangeReason::crNothing,
                         BufferCoord{0, 0},
                         BufferCoord{0, 0},
                         "", SynSelectionMode::smNormal);
    // Move begin of selection
    if (origBlockBegin.Char > 1)
        origBlockBegin.Char+=2;
    // Move end of selection
    if (origBlockEnd.Char > 1)
        origBlockEnd.Char+=2;
    // Move caret
    if (origCaret.Char > 1)
          origCaret.Char+=2;
    setCaretAndSelection(origCaret, origBlockBegin, origBlockEnd);
}

void SynEdit::doUncomment()
{
    BufferCoord origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    QString s;
    if (mReadOnly)
        return;
    doOnPaintTransient(SynTransientType::ttBefore);
    mUndoList->BeginBlock();
    auto action = finally([this]{
        mUndoList->EndBlock();
    });
    origBlockBegin = blockBegin();
    origBlockEnd = blockEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.Char == 1)
        endLine = std::max(origBlockBegin.Line - 1, origBlockEnd.Line - 2);
    else
        endLine = origBlockEnd.Line - 1;
    for (int i = origBlockBegin.Line - 1; i<= endLine; i++) {
        s = mLines->getString(i);
        // Find // after blanks only
        int j = 0;
        while ((j+1 < s.length()) && (s[j] == '\n' || s[j] == '\t'))
            j++;
        if ((j + 1 < s.length()) && (s[j] == '/') && (s[j + 1] == '/')) {
            s.remove(j,2);
            mLines->putString(i,s);
            mUndoList->AddChange(SynChangeReason::crDelete,
                                 BufferCoord{j+1, i + 1},
                                 BufferCoord{j + 3, i + 1},
                                 "//", SynSelectionMode::smNormal);
            // Move begin of selection
            if ((i == origBlockBegin.Line - 1) && (origBlockBegin.Char > 1))
                origBlockBegin.Char-=2;
            // Move end of selection
            if ((i == origBlockEnd.Line - 1) && (origBlockEnd.Char > 1))
                origBlockEnd.Char-=2;
            // Move caret
            if ((i == origCaret.Line - 1) && (origCaret.Char > 1))
                origCaret.Char-=2;
        }
    }
    // When grouping similar commands, process one uncomment action per undo/redo
    mUndoList->AddChange(SynChangeReason::crNothing,
                         BufferCoord{0, 0},
                         BufferCoord{0, 0},
                         "", SynSelectionMode::smNormal);
    setCaretAndSelection(origCaret,origBlockBegin,origBlockEnd);
}

void SynEdit::doToggleComment()
{
    BufferCoord origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    QString s;
    bool allCommented = true;
    if (mReadOnly)
        return;
    doOnPaintTransient(SynTransientType::ttBefore);
    mUndoList->BeginBlock();
    auto action = finally([this]{
        mUndoList->EndBlock();
    });
    origBlockBegin = blockBegin();
    origBlockEnd = blockEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.Char == 1)
        endLine = std::max(origBlockBegin.Line - 1, origBlockEnd.Line - 2);
    else
        endLine = origBlockEnd.Line - 1;
    for (int i = origBlockBegin.Line - 1; i<= endLine; i++) {
        s = mLines->getString(i);
        // Find // after blanks only
        int j = 0;
        while ((j < s.length()) && (s[j] == '\n' || s[j] == '\t'))
            j++;
        if (j>= s.length())
            continue;
        if (s[j] != '/'){
            allCommented = false;
            break;
        }
        if (j+1>=s.length()) {
            allCommented = false;
            break;
        }
        if (s[j + 1] != '/') {
            allCommented = false;
            break;
        }
    }
    if (allCommented)
        doUncomment();
    else
        doComment();
}

void SynEdit::doDeleteLastChar()
{
    if (mReadOnly)
        return ;
    doOnPaintTransientEx(SynTransientType::ttBefore, true);
    auto action = finally([this]{
        ensureCursorPosVisible();
        doOnPaintTransientEx(SynTransientType::ttAfter, true);
    });

    if (selAvail()) {
        setSelectedTextEmpty();
        return;
    }
    QString Temp = lineText();
    int Len = Temp.length();
    BufferCoord Caret = caretXY();
    QString helper = "";
    if (mCaretX > Len + 1) {
//        if (mOptions.setFlag(eoSmartTabDelete)) {
//            //It's at the end of the line, move it to the length
//            if (Len > 0)
//                internalSetCaretX(Len + 1);
//            else {
//                //move it as if there were normal spaces there
//                int SpaceCount1 = mCaretX - 1;
//                int SpaceCount2 = SpaceCount1;
//                // unindent
//                if (SpaceCount1 > 0) {
//                    int BackCounter = mCaretY - 2;
//                    while (BackCounter >= 0) {
//                        SpaceCount2 = leftSpaces(mLines->getString(BackCounter));
//                        if (SpaceCount2 < SpaceCount1)
//                            break;
//                        BackCounter--;
//                    }
//                }
//                if (SpaceCount2 >= SpaceCount1)
//                    SpaceCount2 = 0;
//                setCaretX(SpaceCount2+1);
//                updateLastCaretX();
//                mStateFlags.setFlag(SynStateFlag::sfCaretChanged);
//                statusChanged(SynStatusChange::scCaretX);
//            }
//        } else {
            // only move caret one column
            internalSetCaretX(mCaretX - 1);
//        }
    } else if (mCaretX == 1) {
        // join this line with the last line if possible
        if (mCaretY > 1) {
            internalSetCaretY(mCaretY - 1);
            internalSetCaretX(mLines->getString(mCaretY - 1).length() + 1);
            mLines->deleteAt(mCaretY);
            doLinesDeleted(mCaretY+1, 1);
            if (mOptions.testFlag(eoTrimTrailingSpaces))
                Temp = TrimRight(Temp);
            setLineText(lineText() + Temp);
            helper = lineBreak(); //"/r/n"
        }
    } else {
        // delete text before the caret
        int caretColumn = charToColumn(mCaretY,mCaretX);
        int SpaceCount1 = leftSpaces(Temp);
        int SpaceCount2 = 0;
        int newCaretX;

        if (SpaceCount1 == caretColumn - 1) {
//            if (mOptions.testFlag(eoSmartTabDelete)) {
//                // unindent
//                if (SpaceCount1 > 0) {
//                    int BackCounter = mCaretY - 2;
//                    while (BackCounter >= 0) {
//                        SpaceCount2 = leftSpaces(mLines->getString(BackCounter));
//                        if (SpaceCount2 < SpaceCount1)
//                            break;
//                        BackCounter--;
//                    }
//                }
//                if (SpaceCount2 >= SpaceCount1)
//                    SpaceCount2 = 0;

//                newCaretX = columnToChar(mCaretY,SpaceCount2+1);
//                helper = Temp.mid(newCaretX - 1, mCaretX - newCaretX);
//                Temp.remove(newCaretX - 1, mCaretX - newCaretX);
//            } else {

                //how much till the next tab column
                int BackCounter = (caretColumn - 1) % mTabWidth;
                if (BackCounter == 0)
                    BackCounter = mTabWidth;
                SpaceCount2 = std::max(0,SpaceCount1 - mTabWidth);
                newCaretX = columnToChar(mCaretY,SpaceCount2+1);
                helper = Temp.mid(newCaretX - 1, mCaretX - newCaretX);
                Temp.remove(newCaretX-1,mCaretX - newCaretX);
//            }
            properSetLine(mCaretY - 1, Temp);
            setCaretX(newCaretX);
            updateLastCaretX();
            mStateFlags.setFlag(SynStateFlag::sfCaretChanged);
            emit statusChanged(SynStatusChange::scCaretX);
        } else {
            // delete char
            internalSetCaretX(mCaretX - 1);
            // Stores the previous "expanded" CaretX if the line contains tabs.
//            if (mOptions.testFlag(eoTrimTrailingSpaces) && (Len <> Length(TabBuffer)) then
//                          vTabTrim := CharIndex2CaretPos(CaretX, TabWidth, Temp);
            helper = Temp[mCaretX-1];
            Temp.remove(mCaretX-1,1);
            properSetLine(mCaretY - 1, Temp);
        }
    }
    if ((Caret.Char != mCaretX) || (Caret.Line != mCaretY)) {
        mUndoList->AddChange(SynChangeReason::crSilentDelete, caretXY(), Caret, helper,
                        SynSelectionMode::smNormal);
    }
}

void SynEdit::doDeleteCurrentChar()
{
    QString helper;
    BufferCoord Caret;
    if (!mReadOnly) {
        doOnPaintTransient(SynTransientType::ttBefore);
        if (selAvail())
            setSelectedTextEmpty();
        else {
            // Call UpdateLastCaretX. Even though the caret doesn't move, the
            // current caret position should "stick" whenever text is modified.
            updateLastCaretX();
            QString Temp = lineText();
            int Len = Temp.length();
            if (mCaretX <= Len) {
                // delete char
                helper = Temp.mid(mCaretX-1, 1);
                Caret.Char = mCaretX + 1;
                Caret.Line = mCaretY;
                Temp.remove(mCaretX-1, 1);
                properSetLine(mCaretY - 1, Temp);
            } else {
                // join line with the line after
                if (mCaretY < mLines->count()) {
                      properSetLine(mCaretY - 1, Temp + mLines->getString(mCaretY));
                      Caret.Char = 1;
                      Caret.Line = mCaretY + 1;
                      helper = lineBreak();
                      mLines->deleteAt(mCaretY);
                      if (mCaretX==1)
                          doLinesDeleted(mCaretY, 1);
                      else
                          doLinesDeleted(mCaretY + 1, 1);
                }
            }
            if ((Caret.Char != mCaretX) || (Caret.Line != mCaretY)) {
                mUndoList->AddChange(SynChangeReason::crSilentDeleteAfterCursor, caretXY(), Caret,
                      helper, SynSelectionMode::smNormal);
            }
        }
        doOnPaintTransient(SynTransientType::ttAfter);
    }
}

void SynEdit::doDeleteWord()
{
    if (mReadOnly)
        return;

    BufferCoord start = wordStart();
    BufferCoord end = wordEnd();
    deleteFromTo(start,end);
}

void SynEdit::doDeleteToEOL()
{
    if (mReadOnly)
        return;
    deleteFromTo(caretXY(),BufferCoord{lineText().length()+1,mCaretY});
}

void SynEdit::doDeleteLastWord()
{
    if (mReadOnly)
        return;
    BufferCoord start = prevWordPos();
    BufferCoord end = wordEndEx(start);
    deleteFromTo(start,end);
}

void SynEdit::doDeleteFromBOL()
{
    if (mReadOnly)
        return;
    deleteFromTo(BufferCoord{1,mCaretY},caretXY());
}

void SynEdit::doDeleteLine()
{
    if (!mReadOnly && (mLines->count() > 0)
            && ! ((mCaretY == mLines->count()) && (lineText().isEmpty()))) {
        doOnPaintTransient(SynTransientType::ttBefore);
        if (selAvail())
            setBlockBegin(caretXY());
        QString helper = lineText();
        if (mCaretY == mLines->count()) {
            mLines->putString(mCaretY - 1,"");
            mUndoList->AddChange(SynChangeReason::crSilentDeleteAfterCursor,
                                 BufferCoord{1, mCaretY},
                                 BufferCoord{helper.length() + 1, mCaretY},
                                 helper, SynSelectionMode::smNormal);
        } else {
            mLines->deleteAt(mCaretY - 1);
            helper = helper + lineBreak();
            mUndoList->AddChange(SynChangeReason::crSilentDeleteAfterCursor,
                                 BufferCoord{1, mCaretY},
                                 BufferCoord{helper.length() + 1, mCaretY},
                                 helper, SynSelectionMode::smNormal);
            doLinesDeleted(mCaretY, 1);
        }
        internalSetCaretXY(BufferCoord{1, mCaretY}); // like seen in the Delphi editor
        doOnPaintTransient(SynTransientType::ttAfter);
    }
}

void SynEdit::doDuplicateLine()
{
    if (!mReadOnly && (mLines->count() > 0)) {
        doOnPaintTransient(SynTransientType::ttBefore);
        mLines->insert(mCaretY, lineText());
        doLinesInserted(mCaretY + 1, 1);
        mUndoList->AddChange(SynChangeReason::crLineBreak,
                             caretXY(), caretXY(), "", SynSelectionMode::smNormal);
        internalSetCaretXY(BufferCoord{1, mCaretY}); // like seen in the Delphi editor
        doOnPaintTransient(SynTransientType::ttAfter);
    }
}

void SynEdit::doMoveSelUp()
{
    if (!mReadOnly && (mLines->count() > 0) && (blockBegin().Line > 1)) {
        doOnPaintTransient(SynTransientType::ttBefore);

        // Backup caret and selection
        BufferCoord OrigBlockBegin = blockBegin();
        BufferCoord OrigBlockEnd = blockEnd();

        // Delete line above selection
        QString s = mLines->getString(OrigBlockBegin.Line - 2); // before start, 0 based
        mLines->deleteAt(OrigBlockBegin.Line - 2); // before start, 0 based
        doLinesDeleted(OrigBlockBegin.Line - 1, 1); // before start, 1 based

        // Insert line below selection
        mLines->insert(OrigBlockEnd.Line - 1, s);
        doLinesInserted(OrigBlockEnd.Line, 1);

        // Restore caret and selection
        setCaretAndSelection(
                  BufferCoord{mCaretX, mCaretY - 1},
                  BufferCoord{1, OrigBlockBegin.Line - 1},
                  BufferCoord{mLines->getString(OrigBlockEnd.Line - 2).length() + 1, OrigBlockEnd.Line - 1}
        );
        // Retrieve end of line we moved up
        BufferCoord MoveDelim = BufferCoord{mLines->getString(OrigBlockEnd.Line - 1).length() + 1, OrigBlockEnd.Line};
        // Support undo, implement as drag and drop
        {
            mUndoList->BeginBlock();
            auto action = finally([this]{
                mUndoList->EndBlock();
            });
            mUndoList->AddChange(SynChangeReason::crSelection, // backup original selection
                    OrigBlockBegin,
                    OrigBlockEnd,
                    "",
                    SynSelectionMode::smNormal);
            mUndoList->AddChange(SynChangeReason::crDragDropInsert,
                    mBlockBegin, // modified
                    MoveDelim, // put at end of line me moved up
                    s + lineBreak() + selText(),
                    SynSelectionMode::smNormal);
        }
        doOnPaintTransient(SynTransientType::ttAfter);
    }
}

void SynEdit::doMoveSelDown()
{
    if (!mReadOnly && (mLines->count() > 0) && (blockEnd().Line < mLines->count())) {
        doOnPaintTransient(SynTransientType::ttBefore);
        // Backup caret and selection
        BufferCoord OrigBlockBegin = blockBegin();
        BufferCoord OrigBlockEnd = blockEnd();

        // Delete line below selection
        QString s = mLines->getString(OrigBlockEnd.Line); // after end, 0 based
        mLines->deleteAt(OrigBlockEnd.Line); // after end, 0 based
        doLinesDeleted(OrigBlockEnd.Line, 1); // before start, 1 based

        // Insert line above selection
        mLines->insert(OrigBlockBegin.Line - 1, s);
        doLinesInserted(OrigBlockBegin.Line, 1);

        // Restore caret and selection
        setCaretAndSelection(
                  BufferCoord{mCaretX, mCaretY + 1},
                  BufferCoord{1, OrigBlockBegin.Line + 1},
                  BufferCoord{mLines->getString(OrigBlockEnd.Line).length() + 1, OrigBlockEnd.Line + 1}
                    );

        // Retrieve start of line we moved down
        BufferCoord MoveDelim = BufferCoord{1, OrigBlockBegin.Line};

        // Support undo, implement as drag and drop
        {
            mUndoList->BeginBlock();
            auto action = finally([this] {
                mUndoList->EndBlock();
            });
            mUndoList->AddChange(SynChangeReason::crSelection,
                    OrigBlockBegin,
                    OrigBlockEnd,
                    "",
                    SynSelectionMode::smNormal);
            mUndoList->AddChange(SynChangeReason::crDragDropInsert,
                    MoveDelim, // put at start of line me moved down
                    mBlockEnd, // modified
                    selText() + lineBreak() + s,
                    SynSelectionMode::smNormal);
        }
        doOnPaintTransient(SynTransientType::ttAfter);
    }
}

void SynEdit::clearAll()
{
    mLines->clear();
    mMarkList.clear();
    mUndoList->Clear();
    mRedoList->Clear();
    setModified(false);
}

void SynEdit::insertLine(bool moveCaret)
{
    if (mReadOnly)
        return;
    int nLinesInserted=0;
    mUndoList->BeginBlock();
    auto action = finally([this] {
        mUndoList->EndBlock();
    });
    QString helper;
    if (selAvail()) {
        helper = selText();
        BufferCoord iUndoBegin = mBlockBegin;
        BufferCoord iUndoEnd = mBlockEnd;
        setSelTextPrimitive("");
        mUndoList->AddChange(SynChangeReason::crDelete, iUndoBegin, iUndoEnd, helper,
                      mActiveSelectionMode);
    }

    QString Temp = lineText();
    QString Temp2 = Temp;
    QString Temp3;
    PSynHighlighterAttribute Attr;

    // This is sloppy, but the Right Thing would be to track the column of markers
    // too, so they could be moved depending on whether they are after the caret...
    int InsDelta = (mCaretX == 1)?1:0;
    QString leftLineText = lineText().mid(0, mCaretX - 1);
    QString rightLineText = lineText().mid(mCaretX-1);
    bool notInComment=true;
    properSetLine(mCaretY-1,leftLineText);
    //update range stated for line mCaretY
    if (mHighlighter) {
        if (mCaretY==1) {
            mHighlighter->resetState();
        } else {
            mHighlighter->setState(mLines->ranges(mCaretY-2));
        }
        mHighlighter->setLine(leftLineText, mCaretY-1);
        mHighlighter->nextToEol();
        mLines->setRange(mCaretY-1,mHighlighter->getRangeState());
        notInComment = !mHighlighter->isLastLineCommentNotFinished(
                    mHighlighter->getRangeState().state)
                && !mHighlighter->isLastLineStringNotFinished(
                    mHighlighter->getRangeState().state);
    }
    int indentSpaces = calcIndentSpaces(mCaretY+1,
                                        rightLineText,mOptions.testFlag(eoAutoIndent)
                                        && notInComment);
    if (mOptions.testFlag(eoAutoIndent)) {
        rightLineText=TrimLeft(rightLineText);
    }
    QString indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
    mLines->insert(mCaretY, indentSpacesForRightLineText+rightLineText);
    nLinesInserted++;

    //SpaceCount1 = mLines->getString(mCaretY).length(); //???
    mUndoList->AddChange(SynChangeReason::crLineBreak, caretXY(), caretXY(), rightLineText,
              SynSelectionMode::smNormal);
    //insert new line in middle of "{" and "}"
    if (notInComment &&
            ( leftLineText.endsWith('{') && rightLineText.startsWith('}')
             )) {
        indentSpaces = calcIndentSpaces(mCaretY+1, "" , mOptions.testFlag(eoAutoIndent)
                                                               && notInComment);
        indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
        mLines->insert(mCaretY, indentSpacesForRightLineText);
        nLinesInserted++;
        mUndoList->AddChange(SynChangeReason::crLineBreak, caretXY(), caretXY(), "",
                SynSelectionMode::smNormal);
    }
    if (moveCaret)
        internalSetCaretXY(BufferCoord{indentSpacesForRightLineText.length()+1,mCaretY + 1});


//    if (Len > 0) {
//        if (Len >= mCaretX) {
//            if (mCaretX <= 1) {
//                mLines->insert(mCaretY - 1, "");
//                nLinesInserted++;
//                mUndoList->AddChange(SynChangeReason::crLineBreak, caretXY(), caretXY(), Temp2,
//                                     SynSelectionMode::smNormal);
//                if (moveCaret)
//                    internalSetCaretY(mCaretY + 1);
//            } else {
//                QString leftLineText = lineText().mid(0, mCaretX - 1);
//                QString rightLineText = lineText().mid(mCaretX-1);
//                int indentSpacesOfLeftLineText = leftSpaces(leftLineText);
//                int indentSpaces = indentSpacesOfLeftLineText;
//                bool notInComment=true;
//                properSetLine(mCaretY-1,leftLineText);
//                if (mOptions.testFlag(eoAutoIndent)) {
//                    rightLineText=TrimLeft(rightLineText);
//                }
//                if (getHighlighterAttriAtRowCol(BufferCoord{leftLineText.length(), mCaretY},
//                                                leftLineText, Attr)) {
//                    notInComment = (Attr != mHighlighter->commentAttribute());
//                }
//                leftLineText = leftLineText.trimmed();
//                if (mOptions.testFlag(eoAddIndent)) { // only add indent to source files
//                    if (notInComment) { // and outside of comments
//                        if (leftLineText.endsWith(':') || leftLineText.endsWith('{'))
//                            indentSpaces+=mTabWidth;
//                        if (rightLineText.startsWith('}'))
//                            indentSpaces-=mTabWidth;
//                    }
//                }
//                QString indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
//                mLines->insert(mCaretY, indentSpacesForRightLineText+rightLineText);
//                nLinesInserted++;

//                //SpaceCount1 = mLines->getString(mCaretY).length(); //???
//                mUndoList->AddChange(SynChangeReason::crLineBreak, caretXY(), caretXY(), rightLineText,
//                          SynSelectionMode::smNormal);
//                //insert new line in middle of "{" and "}"
//                if (notInComment && leftLineText.endsWith('{') && rightLineText.startsWith('}')) {
//                    indentSpaces = indentSpacesOfLeftLineText;
//                    indentSpaces += mTabWidth;
//                    indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
//                    mLines->insert(mCaretY, indentSpacesForRightLineText);
//                    nLinesInserted++;
//                    mUndoList->AddChange(SynChangeReason::crLineBreak, caretXY(), caretXY(), "",
//                            SynSelectionMode::smNormal);
//                }
//                if (moveCaret)
//                    internalSetCaretXY(BufferCoord{indentSpacesForRightLineText.length()+1,mCaretY + 1});
//            }
//        } else {
//            SpaceCount2 = 0;
//            int BackCounter = mCaretY;
//            if (mOptions.testFlag(eoAutoIndent)) {
//                do {
//                    BackCounter--;
//                    Temp = mLines->getString(BackCounter);
//                    SpaceCount2 = leftSpaces(Temp);
//                } while ((BackCounter != 0) && (Temp == ""));
//            }
//            mLines->insert(mCaretY, "");
//            nLinesInserted++;
//            BufferCoord Caret = caretXY();
//            if (moveCaret) {
//                QString Temp4=GetLeftSpacing(SpaceCount2,true);
//                if (SpaceCount2 > 0) {
//                }
//                if (mOptions.testFlag(eoAddIndent) && getHighlighterAttriAtRowCol(BufferCoord{Temp.length(), mCaretY},
//                          Temp, Attr)) { // only add indent to source files
//                    if (Attr != mHighlighter->commentAttribute()) { // and outside of comments
//                        Temp = Temp.trimmed();
//                        if (Temp.endsWith('{') || Temp.endsWith(':')) { // add more indent for these too
//                            Temp4=GetLeftSpacing(mTabWidth,true)+Temp4;
//                        }
//                    }
//                }
//                mLines->putString(mCaretY,Temp4); // copy previous indent
//                internalSetCaretXY(BufferCoord{Temp4.length()+1, mCaretY + 1});
//            }
//            mUndoList->AddChange(SynChangeReason::crLineBreak, Caret, Caret, "", SynSelectionMode::smNormal);
//        }
//    } else {
//        if (mLines->count() == 0)
//            mLines->add("");
//        SpaceCount2 = 0;
//        if (mOptions.testFlag(eoAutoIndent)) {
//            int BackCounter = mCaretY - 1;
//            while (BackCounter >= 0) {
//                SpaceCount2 = leftSpaces(mLines->getString(BackCounter));
//                if (mLines->getString(BackCounter).length() > 0)
//                    break;
//                BackCounter--;
//            }
//        }
//        mLines->insert(mCaretY - 1, GetLeftSpacing(SpaceCount2,true));
//        nLinesInserted++;
//        mUndoList->AddChange(SynChangeReason::crLineBreak, caretXY(), caretXY(), "",
//                             SynSelectionMode::smNormal);
//        if (moveCaret) {
//            internalSetCaretXY(BufferCoord{1, mCaretY + 1});
//        }
//    }
    doLinesInserted(mCaretY - InsDelta, nLinesInserted);
    setBlockBegin(caretXY());
    setBlockEnd(caretXY());
    ensureCursorPosVisible();
    updateLastCaretX();
}

void SynEdit::doTabKey()
{
    // Provide Visual Studio like block indenting
    if (mOptions.testFlag(eoTabIndent) && canDoBlockIndent()) {
        doBlockIndent();
        return;
    }
    int i = 0;
    {
        mUndoList->BeginBlock();
        auto action = finally([this]{
            mUndoList->EndBlock();
        });
        if (selAvail()) {
            mUndoList->AddChange(SynChangeReason::crDelete,
                                 mBlockBegin,
                                 mBlockEnd,
                                 selText(),
                                 mActiveSelectionMode);
            setSelTextPrimitive("");
        }
        BufferCoord StartOfBlock = caretXY();
        QString Spaces;
        int NewCaretX = 0;
        if (mOptions.testFlag(eoTabsToSpaces)) {
            int cols = charToColumn(mCaretY,mCaretX);
            i = tabWidth() - (cols) % mTabWidth;
            Spaces = QString(i,' ');
            NewCaretX = mCaretX + i;
        } else {
            Spaces = '\t';
            NewCaretX = mCaretX + 1;
        }

        setSelTextPrimitive(Spaces);
      // Undo is already handled in SetSelText when SelectionMode is Column
        if (mActiveSelectionMode != SynSelectionMode::smColumn) {
            mUndoList->AddChange(SynChangeReason::crInsert, StartOfBlock,
                                 caretXY(),
                                 selText(),
                                 mActiveSelectionMode);
        }
        internalSetCaretX(NewCaretX);
    }

    ensureCursorPosVisible();
}

void SynEdit::doShiftTabKey()
{
    // Provide Visual Studio like block indenting
    if (mOptions.testFlag(eoTabIndent) && canDoBlockIndent()) {
      doBlockUnindent();
      return;
    }

    //Don't un-tab if caret is not on line or is beyond line end
    if (mCaretY > mLines->count() || mCaretX > lineText().length()+1)
        return;
    //Don't un-tab if no chars before the Caret
    if (mCaretX==1)
        return;
    QString s = lineText().mid(0,mCaretX-1);
    //Only un-tab if caret is at the begin of the line
    if (!s.trimmed().isEmpty())
        return;

    int NewX = 0;
    if (s[s.length()-1] == '\t') {
        NewX= mCaretX-1;
    } else {
        int colsBefore = charToColumn(mCaretY,mCaretX)-1;
        int spacesToRemove = colsBefore % mTabWidth;
        if (spacesToRemove == 0)
            spacesToRemove = mTabWidth;
        if (spacesToRemove > colsBefore )
            spacesToRemove = colsBefore;
        NewX = mCaretX;
        while (spacesToRemove > 0 && s[NewX-2] == ' ' ) {
            NewX--;
            spacesToRemove--;
        }
    }
    // perform un-tab

    if (NewX != mCaretX) {
        BufferCoord OldCaretXY = caretXY();
        setBlockBegin(BufferCoord{NewX, mCaretY});
        setBlockEnd(caretXY());

        QString OldSelText = selText();
        setSelTextPrimitive("");

        mUndoList->AddChange(
                    SynChangeReason::crSilentDelete, BufferCoord{NewX, mCaretY},
                    OldCaretXY, OldSelText,  SynSelectionMode::smNormal);
        internalSetCaretX(NewX);
    }
}


bool SynEdit::canDoBlockIndent()
{
    BufferCoord BB;
    BufferCoord BE;

    if (selAvail()) {
        BB = blockBegin();
        BE = blockEnd();
    } else {
        BB = caretXY();
        BE = caretXY();
    }


    if (BB.Line > mLines->count() || BE.Line > mLines->count()) {
        return false;
    }

    if (mActiveSelectionMode == SynSelectionMode::smNormal) {
        QString s = mLines->getString(BB.Line-1).mid(0,BB.Char-1);
        if (!s.trimmed().isEmpty())
            return false;
        if (BE.Char>1) {
            QString s1=mLines->getString(BE.Line-1).mid(BE.Char-1);
            QString s2=mLines->getString(BE.Line-1).mid(0,BE.Char-1);
            if (!s1.trimmed().isEmpty() && !s2.trimmed().isEmpty())
                return false;
        }
    }
    if (mActiveSelectionMode == SynSelectionMode::smColumn) {
        int startCol = charToColumn(BB.Line,BB.Char);
        int endCol = charToColumn(BE.Line,BE.Char);
        for (int i = BB.Line; i<=BE.Line;i++) {
            QString line = mLines->getString(i-1);
            int startChar = columnToChar(i,startCol);
            QString s = line.mid(0,startChar-1);
            if (!s.trimmed().isEmpty())
                return false;

            int endChar = columnToChar(i,endCol);
            s=line.mid(endChar-1);
            if (!s.trimmed().isEmpty())
                return false;
        }
    }
    return true;
}

QRect SynEdit::calculateCaretRect()
{
    DisplayCoord coord = displayXY();
    if (!mInputPreeditString.isEmpty()) {
        QString sLine = lineText().left(mCaretX-1)
                + mInputPreeditString
                + lineText().mid(mCaretX-1);
        coord.Column = charToColumn(sLine,mCaretX+mInputPreeditString.length());
    }
    QPoint caretPos = rowColumnToPixels(coord);
    int caretWidth=mCharWidth;
    if (mCaretY <= mLines->count() && mCaretX <= mLines->getString(mCaretY-1).length()) {
        caretWidth = charColumns(mLines->getString(mCaretY-1)[mCaretX-1])*mCharWidth;
    }
    return QRect(caretPos.x(),caretPos.y(),caretWidth,
                  mTextHeight);
}

void SynEdit::clearAreaList(SynEditingAreaList areaList)
{
    areaList.clear();
}

void SynEdit::computeCaret(int X, int Y)
{
    DisplayCoord vCaretNearestPos = pixelsToNearestRowColumn(X, Y);
    vCaretNearestPos.Row = minMax(vCaretNearestPos.Row, 1, displayLineCount());
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
    int e,x,i;
    QString Spaces;
    SynSelectionMode OrgSelectionMode;
    BufferCoord InsertionPos;

    OrgSelectionMode = mActiveSelectionMode;
    OrgCaretPos = caretXY();
    StrToInsert = nullptr;

    auto action = finally([&,this]{
        if (BB.Char > 1)
            BB.Char += Spaces.length();
        if (BE.Char > 1)
          BE.Char+=Spaces.length();
        setCaretAndSelection(OrgCaretPos,
          BB, BE);
        setActiveSelectionMode(OrgSelectionMode);
    });
    // keep current selection detail
    if (selAvail()) {
        BB = blockBegin();
        BE = blockEnd();
    } else {
        BB = caretXY();
        BE = caretXY();
    }
    // build text to insert
    if (BE.Char == 1 && BE.Line != BB.Line) {
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
        Spaces = QString(mTabWidth,' ') ;
    } else {
        Spaces = "\t";
    }
    for (i = BB.Line; i<e;i++) {
        StrToInsert+=Spaces+lineBreak();
    }
    StrToInsert+=Spaces;

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

void SynEdit::doBlockUnindent()
{
    int LastIndent = 0;
    int FirstIndent = 0;

    BufferCoord BB,BE;
    // keep current selection detail
    if (selAvail()) {
        BB = blockBegin();
        BE = blockEnd();
    } else {
        BB = caretXY();
        BE = caretXY();
    }
    BufferCoord OrgCaretPos = caretXY();
    int x = 0;

    int e = BE.Line;
    // convert selection to complete lines
    if (BE.Char == 1)
        e = BE.Line - 1;
    // build string to delete
    QString FullStrToDelete;
    for (int i = BB.Line; i<= e;i++) {
        QString Line = mLines->getString(i - 1);
        FullStrToDelete += Line;
        if (i!=e-1)
            FullStrToDelete += lineBreak();
        if (Line.isEmpty())
            continue;
        if (Line[0]!=' ' && Line[0]!='\t')
            continue;
        int charsToDelete = 0;
        while (charsToDelete < mTabWidth &&
               charsToDelete < Line.length() &&
               Line[charsToDelete] == ' ')
            charsToDelete++;
        if (charsToDelete == 0)
            charsToDelete = 1;
        if (i==BB.Line)
            FirstIndent = charsToDelete;
        if (i==e)
            LastIndent = charsToDelete;
        if (i==OrgCaretPos.Line)
            x = charsToDelete;
        QString TempString = Line.mid(charsToDelete);
        mLines->putString(i-1,TempString);
    }
    mUndoList->AddChange(
                SynChangeReason::crUnindent, BB, BE, FullStrToDelete, mActiveSelectionMode);
  // restore selection
  //adjust the x position of orgcaretpos appropriately

    OrgCaretPos.Char -= x;
    BB.Char -= FirstIndent;
    BE.Char -= LastIndent;
    setCaretAndSelection(OrgCaretPos, BB, BE);
}

void SynEdit::doAddChar(QChar AChar)
{
    if (mReadOnly)
        return;
    if (!AChar.isPrint())
        return;
    //DoOnPaintTransient(ttBefore);
    //mCaretX will change after setSelLength;
    int oldCaretX=mCaretX;
    int oldCaretY=mCaretY;
    if ((mInserting == false) && (!selAvail())) {
        setSelLength(1);
    }

    mUndoList->BeginBlock();
    if (mOptions.testFlag(eoAutoIndent) && mHighlighter
            && (oldCaretY<=mLines->count())) {
        //unindent if ':' at end of the line
        if (AChar == ':') {
            QString line = mLines->getString(oldCaretY-1);
            if (line.length() < oldCaretX) {
                int indentSpaces = calcIndentSpaces(oldCaretY,line+":", true);
                if (indentSpaces != leftSpaces(line)) {
                    QString temp = GetLeftSpacing(indentSpaces,true) + TrimLeft(line);
                    int i = temp.length();
                    mLines->putString(oldCaretY-1,temp);
                    internalSetCaretXY(BufferCoord{i+1,oldCaretY});
                    mUndoList->AddChange(
                                SynChangeReason::crDelete,
                                BufferCoord{1, oldCaretY},
                                BufferCoord{line.length()+1, oldCaretY},
                                line,
                                SynSelectionMode::smNormal
                                );
                    mUndoList->AddChange(
                                SynChangeReason::crInsert,
                                BufferCoord{1, oldCaretY},
                                BufferCoord{temp.length()+1, oldCaretY},
                                "",
                                SynSelectionMode::smNormal
                                );
                }
            }
        }
        //unindent if '{' is after an statement like 'if' 'for'
        if (AChar == '{') {
            QString temp = mLines->getString(oldCaretY-1).mid(0,oldCaretX-1);
            // and the first nonblank char is this new {
            if (temp.trimmed().isEmpty()) {
                int indentSpaces = calcIndentSpaces(oldCaretY,"{", true);
                QString line = mLines->getString(oldCaretY-1);
                if (indentSpaces != leftSpaces(line)) {
                    QString temp = GetLeftSpacing(indentSpaces,true);
                    int i = temp.length();
                    mLines->putString(oldCaretY-1,temp);
                    internalSetCaretXY(BufferCoord{i+1,oldCaretY});
                    mUndoList->AddChange(
                                SynChangeReason::crDelete,
                                BufferCoord{1, oldCaretY},
                                BufferCoord{line.length()+1, oldCaretY},
                                line,
                                SynSelectionMode::smNormal
                                );
                    mUndoList->AddChange(
                                SynChangeReason::crInsert,
                                BufferCoord{1, oldCaretY},
                                BufferCoord{temp.length()+1, oldCaretY},
                                "",
                                SynSelectionMode::smNormal
                                );
                }
            }
        }
        // Remove TabWidth of indent of the current line when typing a }
        if (AChar == '}') {
            QString temp = mLines->getString(oldCaretY-1).mid(0,oldCaretX-1);
            // and the first nonblank char is this new }
            if (temp.trimmed().isEmpty()) {
                int indentSpaces = calcIndentSpaces(oldCaretY,"}", true);
                QString line = mLines->getString(oldCaretY-1);
                if (indentSpaces != leftSpaces(line)) {
                    QString temp = GetLeftSpacing(indentSpaces,true);
                    int i = temp.length();
                    mLines->putString(oldCaretY-1,temp);
                    internalSetCaretXY(BufferCoord{i+1,oldCaretY});
                    mUndoList->AddChange(
                                SynChangeReason::crDelete,
                                BufferCoord{1, oldCaretY},
                                BufferCoord{line.length()+1, oldCaretY},
                                line,
                                SynSelectionMode::smNormal
                                );
                    mUndoList->AddChange(
                                SynChangeReason::crInsert,
                                BufferCoord{1, oldCaretY},
                                BufferCoord{temp.length()+1, oldCaretY},
                                "",
                                SynSelectionMode::smNormal
                                );
                }
            }
        }
    }
    doSetSelText(AChar);
    mUndoList->EndBlock();

    //DoOnPaintTransient(ttAfter);
}

void SynEdit::doCutToClipboard()
{
    if (mReadOnly || !selAvail())
        return;
    mUndoList->BeginBlock();
    auto action = finally([this] {
        mUndoList->EndBlock();
    });
    internalDoCopyToClipboard(selText());
    doSetSelText("");
}

void SynEdit::doCopyToClipboard()
{
    if (!selAvail())
        return;
    bool ChangeTrim = (mActiveSelectionMode == SynSelectionMode::smColumn) &&
            mOptions.testFlag(eoTrimTrailingSpaces);
    QString sText;
    {
        auto action = finally([&,this] {
            if (ChangeTrim)
                mOptions.setFlag(eoTrimTrailingSpaces);
        });
        if (ChangeTrim)
            mOptions.setFlag(eoTrimTrailingSpaces,false);
        sText = selText();
    }
    internalDoCopyToClipboard(sText);
}

void SynEdit::internalDoCopyToClipboard(const QString &s)
{
    QClipboard* clipboard=QGuiApplication::clipboard();
    clipboard->clear();
    clipboard->setText(s);
}

void SynEdit::doPasteFromClipboard()
{
    if (mReadOnly)
        return;
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard->text().isEmpty())
        return;
    doOnPaintTransient(SynTransientType::ttBefore);
    mUndoList->BeginBlock();
    bool AddPasteEndMarker = false;
    auto action = finally([&,this] {
        if (AddPasteEndMarker)
          mUndoList->AddChange(
                      SynChangeReason::crPasteEnd,
                      blockBegin(),
                      blockEnd(),
                      "",
                      SynSelectionMode::smNormal);
        mUndoList->EndBlock();
    });
    if (!clipboard->text().isEmpty()) {
        mUndoList->AddChange(
                    SynChangeReason::crPasteBegin,
                    blockBegin(),
                    blockEnd(),
                    "",
                    SynSelectionMode::smNormal);
        AddPasteEndMarker = true;
        if (selAvail()) {
            mUndoList->AddChange(
                        SynChangeReason::crDelete,
                        blockBegin(),
                        blockEnd(),
                        selText(),
                        mActiveSelectionMode);
        } else
            setActiveSelectionMode(selectionMode());
        BufferCoord vStartOfBlock = blockBegin();
        BufferCoord vEndOfBlock = blockEnd();
        mBlockBegin = vStartOfBlock;
        mBlockEnd = vEndOfBlock;
        setSelTextPrimitive(clipboard->text());
        if (mActiveSelectionMode != SynSelectionMode::smColumn) {
            mUndoList->AddChange(
                        SynChangeReason::crPaste,
                        vStartOfBlock,
                        blockEnd(),
                        selText(),
                        mActiveSelectionMode);
        }
    }
}

void SynEdit::incPaintLock()
{
    if (mPaintLock==0) {
        onBeginFirstPaintLock();
    }
    mPaintLock ++ ;
}

void SynEdit::decPaintLock()
{
    Q_ASSERT(mPaintLock > 0);
    mPaintLock--;
    if (mPaintLock == 0 ) {
        if (mStateFlags.testFlag(SynStateFlag::sfScrollbarChanged)) {
            updateScrollbars();
            ensureCursorPosVisible();
        }
        if (mStateFlags.testFlag(SynStateFlag::sfCaretChanged))
            updateCaret();
        if (mStatusChanges!=0)
            doOnStatusChange(mStatusChanges);
        onEndFirstPaintLock();
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
    onSizeOrFontChanged(true);
}

void SynEdit::doOnPaintTransient(SynTransientType TransientType)
{
    doOnPaintTransientEx(TransientType, false);
}

void SynEdit::updateLastCaretX()
{
    mMBCSStepAside = false;
    mLastCaretColumn = displayX();
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

void SynEdit::scrollWindow(int , int )
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

void SynEdit::internalSetCaretX(int Value)
{
    internalSetCaretXY(BufferCoord{Value, mCaretY});
}

void SynEdit::internalSetCaretY(int Value)
{
    internalSetCaretXY(BufferCoord{mCaretX,Value});
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
                nMaxScroll = maxScrollWidth();
                if (nMaxScroll <= MAX_SCROLL) {
                    nMin = 1;
                    nMax = nMaxScroll;
                    nPage = mCharsInWindow;
                    nPos = mLeftChar;
                } else {
                    nMin = 0;
                    nMax = MAX_SCROLL;
                    nPage = mulDiv(MAX_SCROLL, mCharsInWindow, nMaxScroll);
                    nPos = mulDiv(MAX_SCROLL, mLeftChar, nMaxScroll);
                }
                horizontalScrollBar()->setMinimum(nMin);
                horizontalScrollBar()->setMaximum(nMax);
                horizontalScrollBar()->setPageStep(nPage);
                horizontalScrollBar()->setValue(nPos);
                horizontalScrollBar()->setSingleStep(1);
            } else
                setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);

            if (mScrollBars == SynScrollStyle::ssBoth ||  mScrollBars == SynScrollStyle::ssVertical) {
                nMaxScroll = maxScrollHeight();
                if (nMaxScroll <= MAX_SCROLL) {
                    nMin = 1;
                    nMax = std::max(1, nMaxScroll);
                    nPage = mLinesInWindow;
                    nPos = mTopLine;
                } else {
                    nMin = 0;
                    nMax = MAX_SCROLL;
                    nPage = mulDiv(MAX_SCROLL, mLinesInWindow, nMaxScroll);
                    nPos = mulDiv(MAX_SCROLL, mTopLine, nMaxScroll);
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
    invalidateRect(calculateCaretRect());
}

void SynEdit::recalcCharExtent()
{
    SynFontStyle styles[] = {SynFontStyle::fsBold, SynFontStyle::fsItalic, SynFontStyle::fsStrikeOut, SynFontStyle::fsUnderline};
    bool hasStyles[] = {false,false,false,false};
    int size = 4;
    if (mHighlighter && mHighlighter->attributes().count()>0) {
        for (const PSynHighlighterAttribute& attribute: mHighlighter->attributes()) {
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
    return Result;
}

void SynEdit::updateModifiedStatus()
{
    setModified(!mUndoList->initialState());
}

int SynEdit::scanFrom(int Index, int canStopIndex)
{
    SynRangeState iRange;
    int Result = std::max(0,Index);
    if (Result >= mLines->count())
        return Result;

    if (Result == 0) {
        mHighlighter->resetState();
    } else {
        mHighlighter->setState(mLines->ranges(Result-1));
    }
    do {
        mHighlighter->setLine(mLines->getString(Result), Result);
        mHighlighter->nextToEol();
        iRange = mHighlighter->getRangeState();
        if (Result > canStopIndex){
            if (mLines->ranges(Result).state == iRange.state
                    && mLines->ranges(Result).braceLevel == iRange.braceLevel
                    && mLines->ranges(Result).parenthesisLevel == iRange.parenthesisLevel
                    && mLines->ranges(Result).bracketLevel == iRange.bracketLevel
                    ) {
                if (mUseCodeFolding)
                    rescanFolds();
                return Result;// avoid the final Decrement
            }
        }
        mLines->setRange(Result,iRange);
        Result ++ ;
    } while (Result < mLines->count());
    Result--;
    if (mUseCodeFolding)
        rescanFolds();
    return Result;
}

void SynEdit::rescanRange(int line)
{
    if (!mHighlighter)
        return;
    line--;
    line = std::max(0,line);
    if (line >= mLines->count())
        return;

    if (line == 0) {
        mHighlighter->resetState();
    } else {
        mHighlighter->setState(mLines->ranges(line-1));
    }
    mHighlighter->setLine(mLines->getString(line), line);
    mHighlighter->nextToEol();
    SynRangeState iRange = mHighlighter->getRangeState();
    mLines->setRange(line,iRange);
}

void SynEdit::rescanRanges()
{
    if (mHighlighter && !mLines->empty()) {
        mHighlighter->resetState();
        for (int i =0;i<mLines->count();i++) {
            mHighlighter->setLine(mLines->getString(i), i);
            mHighlighter->nextToEol();
            mLines->setRange(i, mHighlighter->getRangeState());
        }
    }
    if (mUseCodeFolding)
        rescanFolds();
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

void SynEdit::collapse(PSynEditFoldRange FoldRange)
{
    FoldRange->linesCollapsed = FoldRange->toLine - FoldRange->fromLine;
    FoldRange->collapsed = true;

    // Extract caret from fold
    if ((mCaretY > FoldRange->fromLine) && (mCaretY <= FoldRange->toLine)) {
          setCaretXY(BufferCoord{mLines->getString(FoldRange->fromLine - 1).length() + 1,
                                 FoldRange->fromLine});
    }

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

void SynEdit::rescanFolds()
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
    bool useBraces = ( mCodeFolding.foldRegions.get(FoldIndex)->openSymbol == "{"
            && mCodeFolding.foldRegions.get(FoldIndex)->closeSymbol == "}");

    while (Line < mLines->count()) { // index is valid for LinesToScan and fLines
        // If there is a collapsed fold over here, skip it
        CollapsedFold = collapsedFoldStartAtLine(Line + 1); // only collapsed folds remain
        if (CollapsedFold) {
          Line = CollapsedFold->toLine;
          continue;
        }

        //we just use braceLevel
        if (useBraces) {
            // Find an opening character on this line
            CurLine = mLines->getString(Line);
            if (mLines->rightBraces(Line)>0) {
                for (int i=0; i<mLines->rightBraces(Line);i++) {
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
                }
            }
            if (mLines->leftBraces(Line)>0) {
                for (int i=0; i<mLines->leftBraces(Line);i++) {
                    // Add it to the top list of folds
                    Parent = parentFoldRanges->addByParts(
                      Parent,
                      TopFoldRanges,
                      Line + 1,
                      mCodeFolding.foldRegions.get(FoldIndex),
                      Line + 1);
                    parentFoldRanges = Parent->subFoldRanges;
                }
            }
        } else {

            // Find an opening character on this line
            CurLine = mLines->getString(Line);

            mHighlighter->setState(mLines->ranges(Line));
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

void SynEdit::doOnPaintTransientEx(SynTransientType , bool )
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
    int oldColumns=0;
    while (columns < startColumn) {
        oldColumns = columns;
        if (i>=len)
            break;
        if (s[i] == '\t')
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
    if (mCaretUseTextColor) {
        painter.setPen(mForegroundColor);
    } else {
        painter.setPen(mCaretColor);
    }
    switch(ct) {
    case SynEditCaretType::ctVerticalLine:
        painter.drawLine(rcClip.left()+1,rcClip.top(),rcClip.left()+1,rcClip.bottom());
        break;
    case SynEditCaretType::ctHorizontalLine:
        painter.drawLine(rcClip.left(),rcClip.bottom()-1,rcClip.right(),rcClip.bottom()-1);
        break;
    case SynEditCaretType::ctBlock:
        painter.fillRect(rcClip, mCaretColor);
        break;
    case SynEditCaretType::ctHalfBlock:
        QRect rc=rcClip;
        rc.setTop(rcClip.top()+rcClip.height() / 2);
        painter.fillRect(rcClip, mCaretColor);
        break;
    }
}

int SynEdit::textOffset() const
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

void SynEdit::onSizeOrFontChanged(bool bFont)
{

    if (mCharWidth != 0) {
        mCharsInWindow = std::max(clientWidth() - mGutterWidth - 2, 0) / mCharWidth;
        mLinesInWindow = clientHeight() / mTextHeight;
        bool scrollBarChangedSettings = mStateFlags.testFlag(SynStateFlag::sfScrollbarChanged);
        if (bFont) {
            if (mGutter.showLineNumbers())
                onGutterChanged();
            else
                updateScrollbars();
            mStateFlags.setFlag(SynStateFlag::sfCaretChanged,false);
            invalidate();
        } else
            updateScrollbars();
        mStateFlags.setFlag(SynStateFlag::sfScrollbarChanged,scrollBarChangedSettings);
        //if (!mOptions.testFlag(SynEditorOption::eoScrollPastEol))
        setLeftChar(mLeftChar);
        //if (!mOptions.testFlag(SynEditorOption::eoScrollPastEof))
        setTopLine(mTopLine);
    }
}

void SynEdit::onChanged()
{
    emit changed();
}

void SynEdit::onScrolled(int)
{
    mLeftChar = horizontalScrollBar()->value();
    mTopLine = verticalScrollBar()->value();
    invalidate();
}

const QColor &SynEdit::backgroundColor() const
{
    return mBackgroundColor;
}

void SynEdit::setBackgroundColor(const QColor &newBackgroundColor)
{
    mBackgroundColor = newBackgroundColor;
}

const QColor &SynEdit::foregroundColor() const
{
    return mForegroundColor;
}

void SynEdit::setForegroundColor(const QColor &newForegroundColor)
{
    mForegroundColor = newForegroundColor;
}

int SynEdit::mouseWheelScrollSpeed() const
{
    return mMouseWheelScrollSpeed;
}

void SynEdit::setMouseWheelScrollSpeed(int newMouseWheelScrollSpeed)
{
    mMouseWheelScrollSpeed = newMouseWheelScrollSpeed;
}

const PSynHighlighterAttribute &SynEdit::rainbowAttr3() const
{
    return mRainbowAttr3;
}

const PSynHighlighterAttribute &SynEdit::rainbowAttr2() const
{
    return mRainbowAttr2;
}

const PSynHighlighterAttribute &SynEdit::rainbowAttr1() const
{
    return mRainbowAttr1;
}

const PSynHighlighterAttribute &SynEdit::rainbowAttr0() const
{
    return mRainbowAttr0;
}

bool SynEdit::caretUseTextColor() const
{
    return mCaretUseTextColor;
}

void SynEdit::setCaretUseTextColor(bool newCaretUseTextColor)
{
    mCaretUseTextColor = newCaretUseTextColor;
}

const QColor &SynEdit::rightEdgeColor() const
{
    return mRightEdgeColor;
}

void SynEdit::setRightEdgeColor(const QColor &newRightEdgeColor)
{
    if (newRightEdgeColor!=mRightEdgeColor) {
        mRightEdgeColor = newRightEdgeColor;
    }
}

int SynEdit::rightEdge() const
{
    return mRightEdge;
}

void SynEdit::setRightEdge(int newRightEdge)
{
    if (mRightEdge != newRightEdge) {
        mRightEdge = newRightEdge;
        invalidate();
    }
}

const QColor &SynEdit::selectedBackground() const
{
    return mSelectedBackground;
}

void SynEdit::setSelectedBackground(const QColor &newSelectedBackground)
{
    mSelectedBackground = newSelectedBackground;
}

const QColor &SynEdit::selectedForeground() const
{
    return mSelectedForeground;
}

void SynEdit::setSelectedForeground(const QColor &newSelectedForeground)
{
    mSelectedForeground = newSelectedForeground;
}

int SynEdit::textHeight() const
{
    return mTextHeight;
}

bool SynEdit::readOnly() const
{
    return mReadOnly;
}

void SynEdit::setReadOnly(bool readOnly)
{
    if (mReadOnly != readOnly) {
        mReadOnly = readOnly;
        emit statusChanged(scReadOnly);
    }
}

SynGutter& SynEdit::gutter()
{
    return mGutter;
}

SynEditCaretType SynEdit::insertCaret() const
{
    return mInsertCaret;
}

void SynEdit::setInsertCaret(const SynEditCaretType &insertCaret)
{
    mInsertCaret = insertCaret;
}

SynEditCaretType SynEdit::overwriteCaret() const
{
    return mOverwriteCaret;
}

void SynEdit::setOverwriteCaret(const SynEditCaretType &overwriteCaret)
{
    mOverwriteCaret = overwriteCaret;
}

QColor SynEdit::activeLineColor() const
{
    return mActiveLineColor;
}

void SynEdit::setActiveLineColor(const QColor &activeLineColor)
{
    if (mActiveLineColor!=activeLineColor) {
        mActiveLineColor = activeLineColor;
        invalidateLine(mCaretY);
    }
}

QColor SynEdit::caretColor() const
{
    return mCaretColor;
}

void SynEdit::setCaretColor(const QColor &caretColor)
{
    mCaretColor = caretColor;
}

int SynEdit::tabWidth() const
{
    return mTabWidth;
}

void SynEdit::setTabWidth(int tabWidth)
{
    if (tabWidth!=mTabWidth) {
        mTabWidth = tabWidth;
        mLines->resetColumns();
        invalidate();
    }
}

SynEditorOptions SynEdit::getOptions() const
{
    return mOptions;
}

void SynEdit::setOptions(const SynEditorOptions &Value)
{
    if (Value != mOptions) {
        //bool bSetDrag = mOptions.testFlag(eoDropFiles) != Value.testFlag(eoDropFiles);
        //if  (!mOptions.testFlag(eoScrollPastEol))
        setLeftChar(mLeftChar);
        //if (!mOptions.testFlag(eoScrollPastEof))
        setTopLine(mTopLine);

        bool bUpdateAll = Value.testFlag(eoShowSpecialChars) != mOptions.testFlag(eoShowSpecialChars);
        if (!bUpdateAll)
            bUpdateAll = Value.testFlag(eoShowRainbowColor) != mOptions.testFlag(eoShowRainbowColor);
        //bool bUpdateScroll = (Options * ScrollOptions)<>(Value * ScrollOptions);
        bool bUpdateScroll = true;
        mOptions = Value;

        // constrain caret position to MaxScrollWidth if eoScrollPastEol is enabled
        internalSetCaretXY(caretXY());
        if (mOptions.testFlag(eoScrollPastEol)) {
            BufferCoord vTempBlockBegin = blockBegin();
            BufferCoord vTempBlockEnd = blockEnd();
            setBlockBegin(vTempBlockBegin);
            setBlockEnd(vTempBlockEnd);
        }
        updateScrollbars();
      // (un)register HWND as drop target
//      if bSetDrag and not (csDesigning in ComponentState) and HandleAllocated then
//        DragAcceptFiles(Handle, (eoDropFiles in fOptions));
        if (bUpdateAll)
            invalidate();
        if (bUpdateScroll)
            updateScrollbars();
    }
}

void SynEdit::doAddStr(const QString &s)
{
    if (!selAvail() && !mInserting) {
        BufferCoord BB = caretXY();
        BufferCoord BE = BB;
        BE.Char = BB.Char + s.length();
        setCaretAndSelection(caretXY(),BB,BE);
    }
    doSetSelText(s);
}

void SynEdit::doUndo()
{
    if (mReadOnly)
        return;

    //Remove Group Break;
    if (mUndoList->LastChangeReason() ==  SynChangeReason::crGroupBreak) {
        int OldBlockNumber = mRedoList->blockChangeNumber();
        auto action = finally([&,this]{
           mRedoList->setBlockChangeNumber(OldBlockNumber);
        });
        PSynEditUndoItem Item = mUndoList->PopItem();
        mRedoList->setBlockChangeNumber(Item->changeNumber());
        mRedoList->AddGroupBreak();
    }

    SynChangeReason  FLastChange = mUndoList->LastChangeReason();
    bool FAutoComplete = (FLastChange == SynChangeReason::crAutoCompleteEnd);
    bool FPasteAction = (FLastChange == SynChangeReason::crPasteEnd);
    bool FSpecial1 = (FLastChange == SynChangeReason::crSpecial1End);
    bool FSpecial2 = (FLastChange == SynChangeReason::crSpecial2End);
    bool FKeepGoing;

    PSynEditUndoItem Item = mUndoList->PeekItem();
    if (Item) {
        int OldChangeNumber = Item->changeNumber();
        int SaveChangeNumber = mRedoList->blockChangeNumber();
        mRedoList->setBlockChangeNumber(Item->changeNumber());

        {
            auto action = finally([&,this] {
               mRedoList->setBlockChangeNumber(SaveChangeNumber);
            });
            do {
                doUndoItem();
                Item = mUndoList->PeekItem();
                if (!Item)
                    FKeepGoing = false;
                else {
                    if (FAutoComplete)
                        FKeepGoing = (mUndoList->LastChangeReason() != SynChangeReason::crAutoCompleteBegin);
                    else if (FPasteAction)
                        FKeepGoing = (mUndoList->LastChangeReason() != SynChangeReason::crPasteBegin);
                    else if (FSpecial1)
                        FKeepGoing = (mUndoList->LastChangeReason() != SynChangeReason::crSpecial1Begin);
                    else if (FSpecial2)
                        FKeepGoing = (mUndoList->LastChangeReason() != SynChangeReason::crSpecial2Begin);
                    else if (Item->changeNumber() == OldChangeNumber)
                        FKeepGoing = true;
                    else {
                        FKeepGoing = (mOptions.testFlag(eoGroupUndo) &&
                            (FLastChange == Item->changeReason()) &&
                            ! (FLastChange == SynChangeReason::crIndent
                               || FLastChange == SynChangeReason::crUnindent));
                    }
                    FLastChange = Item->changeReason();
                }
            } while (FKeepGoing);
            //we need to eat the last command since it does nothing and also update modified status...
            if ((FAutoComplete && (mUndoList->LastChangeReason() == SynChangeReason::crAutoCompleteBegin)) ||
                    (FPasteAction && (mUndoList->LastChangeReason() == SynChangeReason::crPasteBegin)) ||
                    (FSpecial1 && (mUndoList->LastChangeReason() == SynChangeReason::crSpecial1Begin)) ||
                    (FSpecial2 && (mUndoList->LastChangeReason() == SynChangeReason::crSpecial2Begin))) {
                doUndoItem();
                updateModifiedStatus();
            }
        }
    }
}

void SynEdit::doUndoItem()
{
    mUndoing = true;
    bool ChangeScrollPastEol = ! mOptions.testFlag(eoScrollPastEol);

    PSynEditUndoItem Item = mUndoList->PopItem();
    if (Item) {
        setActiveSelectionMode(Item->changeSelMode());
        incPaintLock();
        auto action = finally([&,this]{
            mUndoing = false;
            if (ChangeScrollPastEol)
                mOptions.setFlag(eoScrollPastEol,false);
            decPaintLock();
        });
        mOptions.setFlag(eoScrollPastEol);
        switch(Item->changeReason()) {
        case SynChangeReason::crCaret:
            mRedoList->AddChange(
                        Item->changeReason(),
                        caretXY(),
                        caretXY(), "",
                        mActiveSelectionMode);
            internalSetCaretXY(Item->changeStartPos());
            break;
        case SynChangeReason::crSelection:
            mRedoList->AddChange(
                        Item->changeReason(),
                        blockBegin(),
                        blockEnd(),
                        "",
                        mActiveSelectionMode);
            setCaretAndSelection(caretXY(), Item->changeStartPos(), Item->changeEndPos());
            break;
        case SynChangeReason::crInsert:
        case SynChangeReason::crPaste:
        case SynChangeReason::crDragDropInsert: {
            setCaretAndSelection(
                        Item->changeStartPos(),
                        Item->changeStartPos(),
                        Item->changeEndPos());
            QString TmpStr = selText();
            setSelTextPrimitiveEx(
                        Item->changeSelMode(),
                        Item->changeStr(),
                        false);
            mRedoList->AddChange(
                        Item->changeReason(),
                        Item->changeStartPos(),
                        Item->changeEndPos(),
                        TmpStr,
                        Item->changeSelMode());
            internalSetCaretXY(Item->changeStartPos());
            break;
        }
        case SynChangeReason::crDeleteAfterCursor:
        case SynChangeReason::crDelete:
        case SynChangeReason::crSilentDelete:
        case SynChangeReason::crSilentDeleteAfterCursor:
        case SynChangeReason::crDeleteAll: {
            // If there's no selection, we have to set
            // the Caret's position manualy.
            BufferCoord TmpPos;
            if (Item->changeSelMode() == SynSelectionMode::smColumn) {
                TmpPos = BufferCoord{
                    std::min(Item->changeStartPos().Char, Item->changeEndPos().Char),
                    std::min(Item->changeStartPos().Line, Item->changeEndPos().Line)};
            } else {
                TmpPos = BufferCoord{
                        minBufferCoord(
                            Item->changeStartPos(),
                            Item->changeEndPos())};
            }
            if ( (Item->changeReason() == SynChangeReason::crDeleteAfterCursor
                  || Item->changeReason() == SynChangeReason::crSilentDeleteAfterCursor)
                 && (TmpPos.Line > mLines->count())) {
                internalSetCaretXY(BufferCoord{1, mLines->count()});
                mLines->add("");
            }
            setCaretXY(TmpPos);
            setSelTextPrimitiveEx(
                        Item->changeSelMode(),
                        Item->changeStr(),
                        false);
            if ( (Item->changeReason() == SynChangeReason::crDeleteAfterCursor
                  || Item->changeReason() == SynChangeReason::crSilentDeleteAfterCursor)) {
                TmpPos = Item->changeStartPos();
                internalSetCaretXY(TmpPos);
            } else {
                TmpPos = Item->changeEndPos();
                setCaretAndSelection(TmpPos,
                                     Item->changeStartPos(),
                                     Item->changeEndPos());
            }
            mRedoList->AddChange(
                        Item->changeReason(),
                        Item->changeStartPos(),
                        Item->changeEndPos(),
                        "",
                        Item->changeSelMode());
            if (Item->changeReason() == SynChangeReason::crDeleteAll) {
                internalSetCaretXY(BufferCoord{1, 1});
                mBlockEnd = BufferCoord{1, 1};
            }
            ensureCursorPosVisible();
            break;
        }
        case SynChangeReason::crLineBreak:
            // If there's no selection, we have to set
            // the Caret's position manualy.
            internalSetCaretXY(Item->changeStartPos());
            if (mCaretY > 0) {
                QString TmpStr = mLines->getString(mCaretY - 1);
                if ( (mCaretX > TmpStr.length() + 1) && (leftSpaces(Item->changeStr()) == 0))
                    TmpStr = TmpStr + QString(mCaretX - 1 - TmpStr.length(), ' ');
                properSetLine(mCaretY - 1, TmpStr + Item->changeStr());
                mLines->deleteAt(mCaretY);
                doLinesDeleted(mCaretY, 1);
            }
            mRedoList->AddChange(
                        Item->changeReason(),
                        Item->changeStartPos(),
                        Item->changeEndPos(),
                        "",
                        Item->changeSelMode());
            break;
        case SynChangeReason::crIndent:
            // restore the selection
            setCaretAndSelection(
                        Item->changeEndPos(),
                        Item->changeStartPos(),
                        Item->changeEndPos());
            mRedoList->AddChange(
                        Item->changeReason(),
                        Item->changeStartPos(),
                        Item->changeEndPos(),
                        Item->changeStr(),
                        Item->changeSelMode());
            break;
        case SynChangeReason::crUnindent:
            // reinsert the string
            if (Item->changeSelMode()!= SynSelectionMode::smColumn)
                insertBlock(BufferCoord{1, Item->changeStartPos().Line},
                            BufferCoord{1, Item->changeEndPos().Line},
                            Item->changeStr(),
                            false);
            else {
                int BeginX = std::min(Item->changeStartPos().Char,
                                      Item->changeEndPos().Char);
                insertBlock(BufferCoord{BeginX, Item->changeStartPos().Line},
                            BufferCoord{BeginX, Item->changeEndPos().Line},
                            Item->changeStr(), false);
            }
            setCaretAndSelection(
                        Item->changeStartPos(),
                        Item->changeStartPos(),
                        Item->changeEndPos());
            mRedoList->AddChange(
                        Item->changeReason(),
                        Item->changeStartPos(),
                        Item->changeEndPos(),
                        Item->changeStr(),
                        Item->changeSelMode());
            break;
        default:
            break;
        }
    }
}

void SynEdit::doRedo()
{
    if (mReadOnly)
        return;

    SynChangeReason FLastChange = mRedoList->LastChangeReason();
    bool FAutoComplete = (FLastChange == SynChangeReason::crAutoCompleteBegin);
    bool FPasteAction = (FLastChange == SynChangeReason::crPasteBegin);
    bool FSpecial1 = (FLastChange == SynChangeReason::crSpecial1Begin);
    bool FSpecial2 = (FLastChange == SynChangeReason::crSpecial2Begin);

    PSynEditUndoItem Item = mRedoList->PeekItem();
    if (!Item)
        return;
    int OldChangeNumber = Item->changeNumber();
    int SaveChangeNumber = mUndoList->blockChangeNumber();
    mUndoList->setBlockChangeNumber(Item->changeNumber());
    {
        auto action = finally([&,this]{
            mUndoList->setBlockChangeNumber(SaveChangeNumber);
        });
        bool FKeepGoing;
        do {
          doRedoItem();
          Item = mRedoList->PeekItem();
          if (!Item)
              FKeepGoing = false;
          else {
            if (FAutoComplete)
              FKeepGoing = (mRedoList->LastChangeReason() != SynChangeReason::crAutoCompleteEnd);
            if (FPasteAction)
              FKeepGoing = (mRedoList->LastChangeReason() != SynChangeReason::crPasteEnd);
            if (FSpecial1)
              FKeepGoing = (mRedoList->LastChangeReason() != SynChangeReason::crSpecial1End);
            if (FSpecial2)
              FKeepGoing = (mRedoList->LastChangeReason() != SynChangeReason::crSpecial2End);
            else if (Item->changeNumber() == OldChangeNumber)
                FKeepGoing = true;
            else {
                FKeepGoing = (mOptions.testFlag(eoGroupUndo) &&
                (FLastChange == Item->changeReason()) &&
                !(FLastChange == SynChangeReason::crIndent
                        || FLastChange == SynChangeReason::crUnindent));
            }
            FLastChange = Item->changeReason();
          }
        } while (FKeepGoing);

        //we need to eat the last command since it does nothing and also update modified status...
        if ((FAutoComplete && (mRedoList->LastChangeReason() == SynChangeReason::crAutoCompleteEnd)) ||
          (FPasteAction && (mRedoList->LastChangeReason() == SynChangeReason::crPasteEnd)) ||
          (FSpecial1 && (mRedoList->LastChangeReason() == SynChangeReason::crSpecial1End)) ||
          (FSpecial2 && (mRedoList->LastChangeReason() == SynChangeReason::crSpecial2End))) {
            doRedoItem();
            updateModifiedStatus();
        }
    }
    //Remove Group Break
    if (mRedoList->LastChangeReason() == SynChangeReason::crGroupBreak) {
        int OldBlockNumber = mUndoList->blockChangeNumber();
        Item = mRedoList->PopItem();
        {
            auto action2=finally([&,this]{
                mUndoList->setBlockChangeNumber(OldBlockNumber);
            });
            mUndoList->setBlockChangeNumber(Item->changeNumber());
            mUndoList->AddGroupBreak();
        }
        updateModifiedStatus();
    }
}

void SynEdit::doRedoItem()
{
    bool ChangeScrollPastEol = ! mOptions.testFlag(eoScrollPastEol);
    PSynEditUndoItem Item = mRedoList->PopItem();
    if (Item) {
        setActiveSelectionMode(Item->changeSelMode());
        incPaintLock();
        mOptions.setFlag(eoScrollPastEol);
        mUndoList->setInsideRedo(true);
        auto action = finally([&,this]{
            mUndoList->setInsideRedo(false);
            if (ChangeScrollPastEol)
                mOptions.setFlag(eoScrollPastEol,false);
            decPaintLock();
        });
        switch(Item->changeReason()) {
        case SynChangeReason::crCaret:
            mUndoList->AddChange(
                        Item->changeReason(),
                        caretXY(),
                        caretXY(),
                        "",
                        mActiveSelectionMode);
            internalSetCaretXY(Item->changeStartPos());
            break;
        case SynChangeReason::crSelection:
            mUndoList->AddChange(
                        Item->changeReason(),
                        blockBegin(),
                        blockEnd(),
                        "",
                        mActiveSelectionMode);
            setCaretAndSelection(
                        caretXY(),
                        Item->changeStartPos(),
                        Item->changeEndPos());
            break;
        case SynChangeReason::crInsert:
        case SynChangeReason::crPaste:
        case SynChangeReason::crDragDropInsert:
            setCaretAndSelection(
                        Item->changeStartPos(),
                        Item->changeStartPos(),
                        Item->changeStartPos());
            setSelTextPrimitiveEx(Item->changeSelMode(), Item->changeStr(), false);
            internalSetCaretXY(Item->changeEndPos());
            mUndoList->AddChange(Item->changeReason(),
                                 Item->changeStartPos(),
                                 Item->changeEndPos(),
                                 selText(),
                                 Item->changeSelMode());
            if (Item->changeReason() == SynChangeReason::crDragDropInsert) {
                setCaretAndSelection(Item->changeStartPos(),
                                     Item->changeStartPos(),
                                     Item->changeEndPos());
            }
            break;
        case SynChangeReason::crDeleteAfterCursor:
        case SynChangeReason::crSilentDeleteAfterCursor: {
            setCaretAndSelection(Item->changeStartPos(), Item->changeStartPos(),
                                 Item->changeEndPos());
            QString TempString = selText();
            setSelTextPrimitiveEx(Item->changeSelMode(),
                                  Item->changeStr(),false);
            mUndoList->AddChange(Item->changeReason(), Item->changeStartPos(),
                                 Item->changeEndPos(), TempString, Item->changeSelMode());
            internalSetCaretXY(Item->changeEndPos());
            break;
        }
        case SynChangeReason::crDelete:
        case SynChangeReason::crSilentDelete: {
            setCaretAndSelection(Item->changeStartPos(), Item->changeStartPos(),
                                 Item->changeEndPos());
            QString TempString = selText();
            setSelTextPrimitiveEx(Item->changeSelMode(), Item->changeStr(),false);
            mUndoList->AddChange(Item->changeReason(), Item->changeStartPos(),
                                 Item->changeEndPos(),TempString,
                                 Item->changeSelMode());
            internalSetCaretXY(Item->changeStartPos());
            break;
        };
        case SynChangeReason::crLineBreak: {
            BufferCoord CaretPt = Item->changeStartPos();
            setCaretAndSelection(CaretPt, CaretPt, CaretPt);
            commandProcessor(SynEditorCommand::ecLineBreak);
            break;
        }
        case SynChangeReason::crIndent:
            setCaretAndSelection(
                        Item->changeEndPos(),
                        Item->changeStartPos(),
                        Item->changeEndPos());
            mUndoList->AddChange(Item->changeReason(), Item->changeStartPos(),
                        Item->changeEndPos(), Item->changeStr(), Item->changeSelMode());
            break;
        case SynChangeReason::crUnindent: {
            // Delete string
            QString StrToDelete = Item->changeStr();
            internalSetCaretY(Item->changeStartPos().Line);
            int BeginX = 1;
            if (Item->changeSelMode() == SynSelectionMode::smColumn)
                BeginX = std::min(Item->changeStartPos().Char, Item->changeEndPos().Char);
            int Run = 0;
            int Len;
            QString TempString;
            do {
                Run = GetEOL(StrToDelete,Run);
                if (Run != 0) {
                    Len = Run;
                    if (Len > 0) {
                        TempString = mLines->getString(mCaretY - 1);
                        TempString.remove(BeginX-1,Len);
                        mLines->putString(mCaretY-1,TempString);
                    }
                } else
                    Len = 0;
                if (StrToDelete[Run] == '\r') {
                    Run++;
                    if (StrToDelete[Run] == '\n')
                      Run++;
                    mCaretY++;
                }
            } while (Run<StrToDelete.length());
            if (Item->changeSelMode() == SynSelectionMode::smColumn) {
                setCaretAndSelection(Item->changeStartPos(), Item->changeStartPos(),
                    Item->changeEndPos());
            } else {
                // restore selection
                BufferCoord CaretPt;
                if (mOptions.testFlag(eoTabsToSpaces))
                    CaretPt.Char = Item->changeStartPos().Char - mTabWidth;
                else
                    CaretPt.Char = Item->changeStartPos().Char - 1;
                CaretPt.Line = Item->changeStartPos().Line;
                setCaretAndSelection(CaretPt, CaretPt,
                    BufferCoord{Item->changeEndPos().Char - Len,
                                Item->changeEndPos().Line});
            }
            mUndoList->AddChange(Item->changeReason(), Item->changeStartPos(),
                                 Item->changeEndPos(), Item->changeStr(), Item->changeSelMode());
            break;
        }
        default:
            break;
        }
    }
}

void SynEdit::doZoomIn()
{
    QFont newFont = font();
    int size = newFont.pointSize();
    size++;
    newFont.setPointSize(size);
    setFont(newFont);
}

void SynEdit::doZoomOut()
{
    QFont newFont = font();
    int size = newFont.pointSize();
    size--;
    if (size<2)
        size = 2;
    newFont.setPointSize(size);
    setFont(newFont);
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

QString SynEdit::selText()
{
    if (!selAvail()) {
        return "";
    } else {
        int ColFrom = blockBegin().Char;
        int First = blockBegin().Line - 1;
        //
        int ColTo = blockEnd().Char;
        int Last = blockEnd().Line - 1;
        switch(mActiveSelectionMode) {
        case SynSelectionMode::smNormal:
            if (First == Last)
                return  mLines->getString(First).mid(ColFrom-1, ColTo - ColFrom);
            else {
                QString result = mLines->getString(First).mid(ColFrom-1);
                result+= lineBreak();
                for (int i = First + 1; i<=Last - 1; i++) {
                    result += mLines->getString(i);
                    result+=lineBreak();
                }
                result += mLines->getString(Last).leftRef(ColTo-1);
                return result;
            }
        case SynSelectionMode::smColumn:
        {
              First = blockBegin().Line-1;
              ColFrom = charToColumn(blockBegin().Line, blockBegin().Char);
              Last = blockEnd().Line - 1;
              ColTo = charToColumn(blockEnd().Line, blockEnd().Char);
              if (ColFrom > ColTo)
                  std::swap(ColFrom, ColTo);
              QString result;
              for (int i = First; i <= Last; i++) {
                  int l = columnToChar(i,ColFrom);
                  int r = columnToChar(i,ColTo-1);
                  QString s = mLines->getString(i);
                  result += s.mid(l-1,r-l);
                  result+=lineBreak();
              }
              return result;
        }
        case SynSelectionMode::smLine:
        {
            QString result;
            // If block selection includes LastLine,
            // line break code(s) of the last line will not be added.
            for (int i= First; i<=Last - 1;i++) {
                result += mLines->getString(i);
                result+=lineBreak();
            }
            result += mLines->getString(Last);
            if (Last < mLines->count() - 1)
                result+=lineBreak();
            return result;
        }
        }
    }
    return "";
}

QString SynEdit::lineBreak()
{
    return mLines->lineBreak();
}

bool SynEdit::useCodeFolding() const
{
    return mUseCodeFolding;
}

void SynEdit::setUseCodeFolding(bool value)
{
    if (mUseCodeFolding!=value) {
        mUseCodeFolding = value;
    }
}

SynEditCodeFolding &SynEdit::codeFolding()
{
    return mCodeFolding;
}

QString SynEdit::lineText() const
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
        rescanRanges();
    }
    onSizeOrFontChanged(true);
    invalidate();
}

const PSynEditStringList& SynEdit::lines() const
{
    return mLines;
}

bool SynEdit::empty()
{
    return mLines->empty();
}

void SynEdit::commandProcessor(SynEditorCommand Command, QChar AChar, void *pData)
{
    // first the program event handler gets a chance to process the command
    onProcessCommand(Command, AChar, pData);
    if (Command != SynEditorCommand::ecNone)
        ExecuteCommand(Command, AChar, pData);
    onCommandProcessed(Command, AChar, pData);
}

void SynEdit::moveCaretHorz(int DX, bool isSelection)
{
    BufferCoord ptO = caretXY();
    BufferCoord ptDst = ptO;
    QString s = lineText();
    int nLineLen = s.length();
    // only moving or selecting one char can change the line
    //bool bChangeY = !mOptions.testFlag(SynEditorOption::eoScrollPastEol);
    bool bChangeY=true;
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
    moveCaretAndSelection(mBlockBegin, ptDst, isSelection);
}

void SynEdit::moveCaretVert(int DY, bool isSelection)
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
            ptDst.Column = mLastCaretColumn;
    }
    BufferCoord vDstLineChar = displayToBufferPos(ptDst);
    int SaveLastCaretX = mLastCaretColumn;
    bool NewStepAside = mMBCSStepAside;

    // set caret and block begin / end
    incPaintLock();
    moveCaretAndSelection(mBlockBegin, vDstLineChar, isSelection);
    decPaintLock();

    // Set fMBCSStepAside and restore fLastCaretX after moving caret, since
    // UpdateLastCaretX, called by SetCaretXYEx, changes them. This is the one
    // case where we don't want that.
    mMBCSStepAside = NewStepAside;
    mLastCaretColumn = SaveLastCaretX;
}

void SynEdit::moveCaretAndSelection(const BufferCoord &ptBefore, const BufferCoord &ptAfter, bool isSelection)
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

void SynEdit::moveCaretToLineStart(bool isSelection)
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
    moveCaretAndSelection(caretXY(), BufferCoord{newX, mCaretY}, isSelection);
}

void SynEdit::moveCaretToLineEnd(bool isSelection)
{
    int vNewX;
    if (mOptions.testFlag(SynEditorOption::eoEnhanceEndKey)) {
        QString vText = lineText();
        int vLastNonBlank = vText.length()-1;
        int vMinX = 0;
        while ((vLastNonBlank >= vMinX) && (vText[vLastNonBlank] == ' ' || vText[vLastNonBlank] =='\t'))
            vLastNonBlank--;
        vLastNonBlank++;
        vNewX = mCaretX;
        if (vNewX > vLastNonBlank)
            vNewX = vText.length() + 1;
        else
            vNewX = vLastNonBlank + 1;
    } else
        vNewX = lineText().length() + 1;

    moveCaretAndSelection(caretXY(), BufferCoord{vNewX, mCaretY}, isSelection);
}

void SynEdit::setSelectedTextEmpty()
{
    BufferCoord vUndoBegin = mBlockBegin;
    BufferCoord vUndoEnd = mBlockEnd;
    QString vSelText = selText();
    setSelTextPrimitive("");
    if ((vUndoBegin.Line < vUndoEnd.Line) || (
        (vUndoBegin.Line == vUndoEnd.Line) && (vUndoBegin.Char < vUndoEnd.Char))) {
        mUndoList->AddChange(SynChangeReason::crDelete, vUndoBegin, vUndoEnd, vSelText,
            mActiveSelectionMode);
    } else {
        mUndoList->AddChange(SynChangeReason::crDeleteAfterCursor, vUndoBegin, vUndoEnd, vSelText,
            mActiveSelectionMode);
    }
}

void SynEdit::setSelTextPrimitive(const QString &aValue)
{
    setSelTextPrimitiveEx(mActiveSelectionMode, aValue, true);
}

void SynEdit::setSelTextPrimitiveEx(SynSelectionMode PasteMode, const QString &Value, bool AddToUndoList)
{
    incPaintLock();
    mLines->beginUpdate();
    auto action = finally([this] {
        mLines->endUpdate();
        decPaintLock();
    });
    BufferCoord BB = blockBegin();
    BufferCoord BE = blockEnd();
    if (selAvail()) {
        deleteSelection(BB,BE);
        internalSetCaretXY(BB);
    }
    if (!Value.isEmpty()) {
        insertText(Value,PasteMode,AddToUndoList);
    }
    if (mCaretY < 1)
        internalSetCaretY(1);
}

void SynEdit::doSetSelText(const QString &Value)
{
    mUndoList->BeginBlock();
    auto action = finally([this]{
        mUndoList->EndBlock();
    });
    if (selAvail()) {
      mUndoList->AddChange(
                  SynChangeReason::crDelete, mBlockBegin, mBlockEnd,
                  selText(), mActiveSelectionMode);
    } else
        setActiveSelectionMode(selectionMode());
    BufferCoord StartOfBlock = blockBegin();
    BufferCoord EndOfBlock = blockEnd();
    mBlockBegin = StartOfBlock;
    mBlockEnd = EndOfBlock;
    setSelTextPrimitive(Value);
    if (!Value.isEmpty() && (mActiveSelectionMode !=SynSelectionMode::smColumn))
        mUndoList->AddChange(
                    SynChangeReason::crInsert,
                    StartOfBlock,
                    blockEnd(), "",
                    mActiveSelectionMode);
}

int SynEdit::searchReplace(const QString &sSearch, const QString &sReplace, SynSearchOptions sOptions, PSynSearchBase searchEngine,
                    SynSearchMathedProc matchedCallback)
{
    if (!searchEngine)
        return 0;

    // can't search for or replace an empty string
    if (sSearch.isEmpty()) {
        return 0;
    }
    int result = 0;
    // get the text range to search in, ignore the "Search in selection only"
    // option if nothing is selected
    bool bBackward = sOptions.testFlag(ssoBackwards);
    bool bFromCursor = !sOptions.testFlag(ssoEntireScope);
    BufferCoord ptCurrent;
    BufferCoord ptStart;
    BufferCoord ptEnd;
    if (!selAvail())
        sOptions.setFlag(ssoSelectedOnly,false);
    if (sOptions.testFlag(ssoSelectedOnly)) {
        ptStart = blockBegin();
        ptEnd = blockEnd();
        // search the whole line in the line selection mode
        if (mActiveSelectionMode == SynSelectionMode::smLine) {
            ptStart.Char = 1;
            ptEnd.Char = mLines->getString(ptEnd.Line - 1).length();
        } else if (mActiveSelectionMode == SynSelectionMode::smColumn) {
            // make sure the start column is smaller than the end column
            if (ptStart.Char > ptEnd.Char)
                std::swap(ptStart.Char,ptEnd.Char);
        }
        // ignore the cursor position when searching in the selection
        if (bBackward) {
            ptCurrent = ptEnd;
        } else {
            ptCurrent = ptStart;
        }
    } else {
        ptStart.Char = 1;
        ptStart.Line = 1;
        ptEnd.Line = mLines->count();
        ptEnd.Char = mLines->getString(ptEnd.Line - 1).length();
        if (bFromCursor) {
            if (bBackward)
                ptEnd = caretXY();
            else
                ptStart = caretXY();
        }
        if (bBackward)
            ptCurrent = ptEnd;
        else
            ptCurrent = ptStart;
    }
    // initialize the search engine
    searchEngine->setOptions(sOptions);
    searchEngine->setPattern(sSearch);
    // search while the current search position is inside of the search range
    bool dobatchReplace = false;
    doOnPaintTransient(SynTransientType::ttBefore);
    {
        auto action = finally([&,this]{
            if (dobatchReplace) {
                decPaintLock();
                mUndoList->EndBlock();
            }
            doOnPaintTransient(SynTransientType::ttAfter);
        });
        int i;
        // If it's a search only we can leave the procedure now.
        SynSearchAction searchAction = SynSearchAction::Exit;
        while ((ptCurrent.Line >= ptStart.Line) && (ptCurrent.Line <= ptEnd.Line)) {
            int nInLine = searchEngine->findAll(mLines->getString(ptCurrent.Line - 1));
            int iResultOffset = 0;
            if (bBackward)
                i = searchEngine->resultCount()-1;
            else
                i = 0;
            // Operate on all results in this line.
            while (nInLine > 0) {
                // An occurrence may have been replaced with a text of different length
                int nFound = searchEngine->result(i) + 1 + iResultOffset;
                int nSearchLen = searchEngine->length(i);
                int nReplaceLen = 0;
                if (bBackward)
                    i--;
                else
                    i++;
                nInLine--;
                // Is the search result entirely in the search range?
                bool isInValidSearchRange = true;
                int first = nFound;
                int last = nFound + nSearchLen;
                if ((mActiveSelectionMode == SynSelectionMode::smNormal)
                        || !sOptions.testFlag(ssoSelectedOnly)) {
                    if (((ptCurrent.Line == ptStart.Line) && (first < ptStart.Char)) ||
                            ((ptCurrent.Line == ptEnd.Line) && (last > ptEnd.Char)))
                        isInValidSearchRange = false;
                } else if (mActiveSelectionMode == SynSelectionMode::smColumn) {
                    // solves bug in search/replace when smColumn mode active and no selection
                    isInValidSearchRange = ((first >= ptStart.Char) && (last <= ptEnd.Char))
                            || (ptEnd.Char - ptStart.Char < 1);
                }
                if (!isInValidSearchRange)
                    continue;
                result++;
                // Select the text, so the user can see it in the OnReplaceText event
                // handler or as the search result.
                ptCurrent.Char = nFound;
                setBlockBegin(ptCurrent);

                //Be sure to use the Ex version of CursorPos so that it appears in the middle if necessary
                setCaretXYEx(false, BufferCoord{1, ptCurrent.Line});
                ensureCursorPosVisibleEx(true);
                ptCurrent.Char += nSearchLen;
                setBlockEnd(ptCurrent);
                //internalSetCaretXY(ptCurrent);
                if (bBackward)
                    internalSetCaretXY(blockBegin());
                else
                    internalSetCaretXY(ptCurrent);

                QString replaceText = searchEngine->replace(selText(), sReplace);
                if (matchedCallback && !dobatchReplace) {
                    searchAction = matchedCallback(sSearch,replaceText,ptCurrent.Line,
                                    nFound,nSearchLen);
                }
                if (searchAction==SynSearchAction::Exit) {
                    return result;
                } else if (searchAction == SynSearchAction::Skip) {
                    continue;
                } else if (searchAction == SynSearchAction::Replace
                           || searchAction == SynSearchAction::ReplaceAll) {
                    if (!dobatchReplace &&
                            (searchAction == SynSearchAction::ReplaceAll) ){
                        incPaintLock();
                        mUndoList->BeginBlock();
                        dobatchReplace = true;
                    }
                    doSetSelText(replaceText);
                    nReplaceLen = caretX() - nFound;
                    // fix the caret position and the remaining results
                    if (!bBackward) {
                        internalSetCaretX(nFound + nReplaceLen);
                        if ((nSearchLen != nReplaceLen)) {
                            iResultOffset += nReplaceLen - nSearchLen;
                            if ((mActiveSelectionMode != SynSelectionMode::smColumn) && (caretY() == ptEnd.Line)) {
                                ptEnd.Char+=nReplaceLen - nSearchLen;
                                setBlockEnd(ptEnd);
                            }
                        }
                    }
                }
            }
            // search next / previous line
            if (bBackward)
                ptCurrent.Line--;
            else
                ptCurrent.Line++;
        }
    }
    return result;
}

void SynEdit::doLinesDeleted(int firstLine, int count)
{
    emit linesDeleted(firstLine, count);
//    // gutter marks
//    for i := 0 to Marks.Count - 1 do begin
//      if Marks[i].Line >= FirstLine + Count then
//        Marks[i].Line := Marks[i].Line - Count
//      else if Marks[i].Line > FirstLine then
//        Marks[i].Line := FirstLine;
//    end;
//    // plugins
//    if fPlugins <> nil then begin
//      for i := 0 to fPlugins.Count - 1 do
//        TSynEditPlugin(fPlugins[i]).LinesDeleted(FirstLine, Count);
    //    end;
}

void SynEdit::doLinesInserted(int firstLine, int count)
{
    emit linesInserted(firstLine, count);
//    // gutter marks
//    for i := 0 to Marks.Count - 1 do begin
//      if Marks[i].Line >= FirstLine then
//        Marks[i].Line := Marks[i].Line + Count;
//    end;
//    // plugins
//    if fPlugins <> nil then begin
//      for i := 0 to fPlugins.Count - 1 do
//        TSynEditPlugin(fPlugins[i]).LinesInserted(FirstLine, Count);
//    end;
}

void SynEdit::properSetLine(int ALine, const QString &ALineText)
{
    if (mOptions.testFlag(eoTrimTrailingSpaces))
        mLines->putString(ALine,TrimRight(ALineText));
    else
        mLines->putString(ALine,ALineText);
}

void SynEdit::deleteSelection(const BufferCoord &BB, const BufferCoord &BE)
{
    bool UpdateMarks = false;
    int MarkOffset = 0;
    switch(mActiveSelectionMode) {
    case SynSelectionMode::smNormal:
        if (mLines->count() > 0) {
            // Create a string that contains everything on the first line up
            // to the selection mark, and everything on the last line after
            // the selection mark.
            QString TempString = mLines->getString(BB.Line - 1).mid(0, BB.Char - 1)
                + mLines->getString(BE.Line - 1).mid(BE.Char-1);
            // Delete all lines in the selection range.
            mLines->deleteLines(BB.Line, BE.Line - BB.Line);
            properSetLine(BB.Line-1,TempString);
            UpdateMarks = true;
            internalSetCaretXY(BB);
        }
        break;
    case SynSelectionMode::smColumn:
    {
        int First = BB.Line-1;
        int ColFrom = charToColumn(BB.Line, BB.Char);
        int Last = BE.Line - 1;
        int ColTo = charToColumn(BE.Line, BE.Char);
        if (ColFrom > ColTo)
            std::swap(ColFrom, ColTo);
        QString result;
        for (int i = First; i <= Last; i++) {
            int l = columnToChar(i,ColFrom);
            int r = columnToChar(i,ColTo-1);
            QString s = mLines->getString(i);
            s.remove(l-1,r-l);
            properSetLine(i,s);
        }
        // Lines never get deleted completely, so keep caret at end.
        internalSetCaretXY(BB);
        // Column deletion never removes a line entirely, so no mark
        // updating is needed here.
        break;
    }
    case SynSelectionMode::smLine:
        if (BE.Line == mLines->count()) {
            mLines->putString(BE.Line - 1,"");
            mLines->deleteLines(BB.Line-1,BE.Line-BB.Line);
        } else {
            mLines->deleteLines(BB.Line-1,BE.Line-BB.Line+1);
        }
        // smLine deletion always resets to first column.
        internalSetCaretXY(BufferCoord{1, BB.Line});
        UpdateMarks = true;
        MarkOffset = 1;
        break;
    }
    // Update marks
    if (UpdateMarks)
        doLinesDeleted(BB.Line, BE.Line - BB.Line + MarkOffset);
}

void SynEdit::insertText(const QString &Value, SynSelectionMode PasteMode,bool AddToUndoList)
{
    if (Value.isEmpty())
        return;

    int StartLine = mCaretY;
    int StartCol = mCaretX;
    int InsertedLines = 0;
    switch(PasteMode){
    case SynSelectionMode::smNormal:
        InsertedLines = insertTextByNormalMode(Value);
        break;
    case SynSelectionMode::smColumn:
        InsertedLines = insertTextByColumnMode(Value,AddToUndoList);
        break;
    case SynSelectionMode::smLine:
        InsertedLines = insertTextByLineMode(Value);
        break;
    }
    // We delete selected based on the current selection mode, but paste
    // what's on the clipboard according to what it was when copied.
    // Update marks
    if (InsertedLines > 0) {
        if ((PasteMode == SynSelectionMode::smNormal) && (StartCol > 1))
            StartLine++;
        doLinesInserted(StartLine, InsertedLines);
    }
    ensureCursorPosVisible();
}

int SynEdit::insertTextByNormalMode(const QString &Value)
{
    QString sLeftSide;
    QString sRightSide;
    QString Str;
    int Start;
    int P;
    bool bChangeScroll;
//    int SpaceCount;
    int Result = 0;
    sLeftSide = lineText().mid(0, mCaretX - 1);
    if (mCaretX - 1 > sLeftSide.length()) {
        if (StringIsBlank(sLeftSide))
            sLeftSide = GetLeftSpacing(displayX() - 1, true);
        else
            sLeftSide += QString(mCaretX - 1 - sLeftSide.length(),' ');
    }
    sRightSide = lineText().mid(mCaretX-1);
//    if (mUndoing) {
//        SpaceCount = 0;
//    } else {
//        SpaceCount = leftSpaces(sLeftSide);
//    }
    int caretY=mCaretY;
    // step1: insert the first line of Value into current line
    Start = 0;
    P = GetEOL(Value,Start);
    if (P<Value.length()) {
        QString s = TrimLeft(Value.mid(0, P - Start));
        if (sLeftSide.isEmpty()) {
            sLeftSide = GetLeftSpacing(calcIndentSpaces(caretY,s,true),true);
        }
        Str = sLeftSide + s;
        properSetLine(caretY - 1, Str);
        mLines->insertLines(caretY, CountLines(Value,P));
    } else {
        Str = sLeftSide + Value + sRightSide;
        properSetLine(caretY - 1, Str);
    }
    rescanRange(caretY);
    // step2: insert remaining lines of Value
    while (P < Value.length()) {
        if (Value[P] == '\r')
            P++;
        if (Value[P] == '\n')
            P++;
        caretY++;
        mStatusChanges.setFlag(SynStatusChange::scCaretY);
        Start = P;
        P = GetEOL(Value,Start);
        if (P == Start) {
          if (P<Value.length())
              Str = GetLeftSpacing(calcIndentSpaces(caretY,"",true),true);
          else
              Str = sRightSide;
        } else {
            Str = Value.mid(Start, P-Start);
            if (P>=Value.length())
                Str += sRightSide;
            if (mOptions.testFlag(eoAutoIndent)) {
                int indentSpaces = calcIndentSpaces(caretY,Str,true);
                Str = GetLeftSpacing(indentSpaces,true)+TrimLeft(Str);
            }
        }
        properSetLine(caretY - 1, Str);
        rescanRange(caretY);
        Result++;
    }
    bChangeScroll = !mOptions.testFlag(eoScrollPastEol);
    mOptions.setFlag(eoScrollPastEol);
    auto action = finally([&,this]{
        if (bChangeScroll)
            mOptions.setFlag(eoScrollPastEol,false);
    });
    if (mOptions.testFlag(eoTrimTrailingSpaces) && (sRightSide == "")) {
          internalSetCaretXY(BufferCoord{lineText().length()+1,caretY});
    } else
        internalSetCaretXY(BufferCoord{Str.length() - sRightSide.length()+1,caretY});
    return Result;
}

int SynEdit::insertTextByColumnMode(const QString &Value, bool AddToUndoList)
{
    QString Str;
    QString TempString;
    int Start;
    int P;
    int Len;
    int InsertCol;
    BufferCoord  LineBreakPos;
    int Result = 0;
    // Insert string at current position
    InsertCol = charToColumn(mCaretY,mCaretX);
    Start = 0;
    do {
        P = GetEOL(Value,Start);
        if (P != Start) {
            Str = Value.mid(0,P-Start);
//          Move(Start^, Str[1], P - Start);
            if (mCaretY > mLines->count()) {
                Result++;
                TempString = QString(InsertCol - 1,' ') + Str;
                mLines->add("");
                if (AddToUndoList) {
                    LineBreakPos.Line = mCaretY - 1;
                    LineBreakPos.Char = mLines->getString(mCaretY - 2).length() + 1;
                    mUndoList->AddChange(SynChangeReason::crLineBreak,
                                     LineBreakPos,
                                     LineBreakPos,
                                     "", SynSelectionMode::smNormal);
                }
            } else {
                TempString = mLines->getString(mCaretY - 1);
                Len = stringColumns(TempString,0);
                if (Len < InsertCol) {
                    TempString = TempString + QString(InsertCol - Len - 1,' ') + Str;
                } else {
                    int insertPos = charToColumn(TempString,InsertCol);
                    TempString.insert(insertPos-1,Str);
                }
            }
            properSetLine(mCaretY - 1, TempString);
            // Add undo change here from PasteFromClipboard
            if (AddToUndoList) {
                mUndoList->AddChange(SynChangeReason::crPaste, BufferCoord{mCaretX, mCaretY},
                    BufferCoord{mCaretX + (P - Start), mCaretY}, "", mActiveSelectionMode);
            }
        }
        if (P<Value.length() && ((Value[P]=='\r') || (Value[P]=='\n'))) {
            P++;
            if (P<Value.length() && Value[P]=='\n')
                P++;
            mCaretY++;
            mStatusChanges.setFlag(SynStatusChange::scCaretY);
        }
        Start = P;
    } while (P<Value.length());
    mCaretX+=Str.length();
    mStatusChanges.setFlag(SynStatusChange::scCaretX);
    return Result;
}

int SynEdit::insertTextByLineMode(const QString &Value)
{
    int Start;
    int P;
    QString Str;
    int Result = 0;
    mCaretX = 1;
    emit statusChanged(SynStatusChange::scCaretX);
    // Insert string before current line
    Start = 0;
    do {
        P = GetEOL(Value,Start);
        if (P != Start)
            Str = Value.mid(Start,P - Start);
        else
            Str = "";
        if ((mCaretY == mLines->count()) || mInserting) {
            mLines->insert(mCaretY - 1, "");
            Result++;
        }
        properSetLine(mCaretY - 1, Str);
        mCaretY++;
        mStatusChanges.setFlag(SynStatusChange::scCaretY);
        if (P<Value.length() && Value[P]=='\r')
            P++;
        if (P<Value.length() && Value[P]=='\n')
            P++;
        Start = P;
    } while (P<Value.length());
    return Result;
}

void SynEdit::deleteFromTo(const BufferCoord &start, const BufferCoord &end)
{
    if (mReadOnly)
        return;
    doOnPaintTransient(SynTransientType::ttBefore);
    if ((start.Char != end.Char) || (start.Line != end.Line)) {
        setBlockBegin(start);
        setBlockEnd(end);
        setActiveSelectionMode(SynSelectionMode::smNormal);
        QString helper = selText();
        setSelTextPrimitive("");
        mUndoList->AddChange(SynChangeReason::crSilentDeleteAfterCursor, start, end,
                helper, SynSelectionMode::smNormal);
        internalSetCaretXY(start);
    }
    doOnPaintTransient(SynTransientType::ttAfter);
}

bool SynEdit::onGetSpecialLineColors(int, QColor &, QColor &)
{
    return false;
}

void SynEdit::onGetEditingAreas(int, SynEditingAreaList &)
{

}

void SynEdit::onGutterGetText(int , QString &)
{

}

void SynEdit::onGutterPaint(QPainter &, int , int , int )
{

}

void SynEdit::onPaint(QPainter &)
{

}

void SynEdit::onPreparePaintHighlightToken(int , int , const QString &,
                                           PSynHighlighterAttribute , SynFontStyles &, QColor &, QColor &)
{

}

void SynEdit::onProcessCommand(SynEditorCommand , QChar , void *)
{

}

void SynEdit::onCommandProcessed(SynEditorCommand , QChar , void *)
{

}

void SynEdit::ExecuteCommand(SynEditorCommand Command, QChar AChar, void *pData)
{
    incPaintLock();
    auto action=finally([this] {
        decPaintLock();
    });
    switch(Command) {
    //horizontal caret movement or selection
    case SynEditorCommand::ecLeft:
    case SynEditorCommand::ecSelLeft:
        moveCaretHorz(-1, Command == SynEditorCommand::ecSelLeft);
        break;
    case SynEditorCommand::ecRight:
    case SynEditorCommand::ecSelRight:
        moveCaretHorz(1, Command == SynEditorCommand::ecSelRight);
        break;
    case SynEditorCommand::ecPageLeft:
    case SynEditorCommand::ecSelPageLeft:
        moveCaretHorz(-mCharsInWindow, Command == SynEditorCommand::ecSelPageLeft);
        break;
    case SynEditorCommand::ecPageRight:
    case SynEditorCommand::ecSelPageRight:
        moveCaretHorz(mCharsInWindow, Command == SynEditorCommand::ecSelPageRight);
        break;
    case SynEditorCommand::ecLineStart:
    case SynEditorCommand::ecSelLineStart:
        moveCaretToLineStart(Command == SynEditorCommand::ecSelLineStart);
        break;
    case SynEditorCommand::ecLineEnd:
    case SynEditorCommand::ecSelLineEnd:
        moveCaretToLineEnd(Command == SynEditorCommand::ecSelLineEnd);
        break;
    // vertical caret movement or selection
    case SynEditorCommand::ecUp:
    case SynEditorCommand::ecSelUp:
        moveCaretVert(-1, Command == SynEditorCommand::ecSelUp);
        break;
    case SynEditorCommand::ecDown:
    case SynEditorCommand::ecSelDown:
        moveCaretVert(1, Command == SynEditorCommand::ecSelDown);
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
        moveCaretVert(counter, Command == SynEditorCommand::ecSelPageUp || Command == SynEditorCommand::ecSelPageDown);
        break;
    }
    case SynEditorCommand::ecPageTop:
    case SynEditorCommand::ecSelPageTop:
        moveCaretVert(mTopLine-mCaretY, Command == SynEditorCommand::ecSelPageTop);
        break;
    case SynEditorCommand::ecPageBottom:
    case SynEditorCommand::ecSelPageBottom:
        moveCaretVert(mTopLine+mLinesInWindow-1-mCaretY, Command == SynEditorCommand::ecSelPageBottom);
        break;
    case SynEditorCommand::ecEditorStart:
    case SynEditorCommand::ecSelEditorStart:
        moveCaretVert(1-mCaretY, Command == SynEditorCommand::ecSelEditorStart);
        moveCaretToLineStart(Command == SynEditorCommand::ecSelEditorStart);
        break;
    case SynEditorCommand::ecEditorEnd:
    case SynEditorCommand::ecSelEditorEnd:
        if (!mLines->empty()) {
            moveCaretVert(mLines->count()-mCaretY, Command == SynEditorCommand::ecSelEditorEnd);
            moveCaretToLineEnd(Command == SynEditorCommand::ecSelEditorStart);
        }
        break;
    // goto special line / column position
    case SynEditorCommand::ecGotoXY:
    case SynEditorCommand::ecSelGotoXY:
        if (pData)
            moveCaretAndSelection(caretXY(), *((BufferCoord *)(pData)), Command == SynEditorCommand::ecSelGotoXY);
        break;
    // word selection
    case SynEditorCommand::ecWordLeft:
    case SynEditorCommand::ecSelWordLeft:
    {
        BufferCoord CaretNew = prevWordPos();
        moveCaretAndSelection(caretXY(), CaretNew, Command == SynEditorCommand::ecSelWordLeft);
        break;
    }
    case SynEditorCommand::ecWordRight:
    case SynEditorCommand::ecSelWordRight:
    {
        BufferCoord CaretNew = nextWordPos();
        moveCaretAndSelection(caretXY(), CaretNew, Command == SynEditorCommand::ecSelWordRight);
        break;
    }
    case SynEditorCommand::ecSelWord:
        setSelWord();
        break;
    case SynEditorCommand::ecSelectAll:
        doSelectAll();
        break;
    case SynEditorCommand::ecDeleteLastChar:
        doDeleteLastChar();
        break;
    case SynEditorCommand::ecDeleteChar:
        doDeleteCurrentChar();
        break;
    case SynEditorCommand::ecDeleteWord:
        doDeleteWord();
        break;
    case SynEditorCommand::ecDeleteEOL:
        doDeleteToEOL();
        break;
    case SynEditorCommand::ecDeleteLastWord:
        doDeleteLastWord();
        break;
    case SynEditorCommand::ecDeleteBOL:
        doDeleteFromBOL();
        break;
    case SynEditorCommand::ecDeleteLine:
        doDeleteLine();
        break;
    case SynEditorCommand::ecDuplicateLine:
        doDuplicateLine();
        break;
    case SynEditorCommand::ecMoveSelUp:
        doMoveSelUp();
        break;
    case SynEditorCommand::ecMoveSelDown:
        doMoveSelDown();
        break;
    case SynEditorCommand::ecClearAll:
        clearAll();
        break;
    case SynEditorCommand::ecInsertLine:
    case SynEditorCommand::ecLineBreak:
        insertLine(Command == SynEditorCommand::ecLineBreak);
        break;
    case SynEditorCommand::ecTab:
        doTabKey();
        break;
    case SynEditorCommand::ecShiftTab:
        doShiftTabKey();
        break;
    case SynEditorCommand::ecChar:
        doAddChar(AChar);
        break;
    case SynEditorCommand::ecInsertMode:
        if (!mReadOnly)
            setInsertMode(true);
        break;
    case SynEditorCommand::ecOverwriteMode:
        if (!mReadOnly)
            setInsertMode(false);
        break;
    case SynEditorCommand::ecToggleMode:
        if (!mReadOnly) {
            setInsertMode(!mInserting);
        }
        break;
    case SynEditorCommand::ecCut:
        if (!mReadOnly && selAvail())
            doCutToClipboard();
        break;
    case SynEditorCommand::ecCopy:
        if (selAvail())
            doCopyToClipboard();
        break;
    case SynEditorCommand::ecPaste:
        if (!mReadOnly)
            doPasteFromClipboard();
        break;
    case SynEditorCommand::ecImeStr:
    case SynEditorCommand::ecString:
        if (!mReadOnly)
            doAddStr(*((QString*)pData));
        break;
    case SynEditorCommand::ecUndo:
        if (!mReadOnly)
            doUndo();
        break;
    case SynEditorCommand::ecRedo:
        if (!mReadOnly)
            doRedo();
        break;
    case SynEditorCommand::ecZoomIn:
        doZoomIn();
        break;
    case SynEditorCommand::ecZoomOut:
        doZoomOut();
        break;
    case SynEditorCommand::ecComment:
        doComment();
        break;
    case SynEditorCommand::ecUncomment:
        doUncomment();
        break;
    case SynEditorCommand::ecToggleComment:
        doToggleComment();
        break;
    default:
        break;
    }

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

//        ecUpperCase,
//          ecLowerCase,
//          ecToggleCase,
//          ecTitleCase,
//          ecUpperCaseBlock,
//          ecLowerCaseBlock,
//          ecToggleCaseBlock:
//          if not ReadOnly then
//            DoCaseChange(Command);

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
//  {$ENDIF}
//      end;
//    finally
//      DecPaintLock;
//    end;
//  end;

}

void SynEdit::onEndFirstPaintLock()
{

}

void SynEdit::onBeginFirstPaintLock()
{

}

bool SynEdit::isIdentChar(const QChar &ch)
{
    if (mHighlighter) {
        return mHighlighter->isIdentChar(ch);
    } else {
        if (ch == '_') {
            return true;
        }
        if ((ch>='0') && (ch <= '9')) {
            return true;
        }
        if ((ch>='a') && (ch <= 'z')) {
            return true;
        }
        if ((ch>='A') && (ch <= 'Z')) {
            return true;
        }
        return false;
    }
}

void SynEdit::setRainbowAttrs(const PSynHighlighterAttribute &attr0, const PSynHighlighterAttribute &attr1, const PSynHighlighterAttribute &attr2, const PSynHighlighterAttribute &attr3)
{
    mRainbowAttr0 = attr0;
    mRainbowAttr1 = attr1;
    mRainbowAttr2 = attr2;
    mRainbowAttr3 = attr3;
}

void SynEdit::updateMouseCursor(){
    QPoint p = mapFromGlobal(cursor().pos());
    if (p.y() >= clientHeight() || p.x()>= clientWidth()) {
        setCursor(Qt::ArrowCursor);
    } else if (p.x() > mGutterWidth) {
        setCursor(Qt::IBeamCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
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
    QRect rcCaret = calculateCaretRect();

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
        nL1 = minMax(mTopLine + rcClip.top() / mTextHeight, mTopLine, displayLineCount());
        nL2 = minMax(mTopLine + (rcClip.bottom() + mTextHeight - 1) / mTextHeight, 1, displayLineCount());

        //qDebug()<<"Paint:"<<nL1<<nL2<<nC1<<nC2;

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

    onSizeOrFontChanged(false);
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
    case QEvent::KeyPress:{
        QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
        {
            // process tab key presse event
            keyPressEvent(keyEvent);
            return true;
        }
    }
        break;
    case QEvent::FontChange:
        synFontChanged();
        break;
    case QEvent::MouseMove: {
        updateMouseCursor();
        break;
    }
    default:
        break;
    }
    return QAbstractScrollArea::event(event);
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
        commandProcessor(cmd,QChar(),nullptr);
        event->accept();
    } else if (!event->text().isEmpty()) {
        QChar c = event->text().at(0);
        if (c=='\t' || c.isPrint()) {
            commandProcessor(SynEditorCommand::ecChar,c,nullptr);
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
    mMouseMoved = false;
//    BufferCoord TmpBegin = mBlockBegin;
//    BufferCoord TmpEnd = mBlockEnd;
    Qt::MouseButton button = event->button();
    int X=event->pos().x();
    int Y=event->pos().y();

    QAbstractScrollArea::mousePressEvent(event);

    //fKbdHandler.ExecuteMouseDown(Self, Button, Shift, X, Y);

    if (button == Qt::RightButton) {
        if (mOptions.testFlag(eoRightMouseMovesCursor) &&
                ( (selAvail() && ! isPointInSelection(displayToBufferPos(pixelsToRowColumn(X, Y))))
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
            mMouseDownPos = event->pos();
        }
        computeCaret(X,Y);
        //I couldn't track down why, but sometimes (and definitely not all the time)
        //the block positioning is lost.  This makes sure that the block is
        //maintained in case they started a drag operation on the block
//        setBlockBegin(TmpBegin);
//        setBlockEnd(TmpEnd);

        setMouseTracking(true);
        //if mousedown occurred in selected block begin drag operation
        mStateFlags.setFlag(SynStateFlag::sfWaitForDragging,false);
        if (bWasSel && mOptions.testFlag(eoDragDropEditing) && (X >= mGutterWidth + 2)
                && (mSelectionMode == SynSelectionMode::smNormal) && isPointInSelection(displayToBufferPos(pixelsToRowColumn(X, Y))) ) {
          bStartDrag = true;
        }
        if (bStartDrag) {
            mStateFlags.setFlag(SynStateFlag::sfWaitForDragging);
        } else {
            if (event->modifiers() == Qt::ShiftModifier) {
                //BlockBegin and BlockEnd are restored to their original position in the
                //code from above and SetBlockEnd will take care of proper invalidation
                setBlockEnd(caretXY());
            } else if (mOptions.testFlag(eoAltSetsColumnMode) &&
                     (mActiveSelectionMode != SynSelectionMode::smLine)) {
                if (event->modifiers() == Qt::AltModifier)
                    setSelectionMode(SynSelectionMode::smColumn);
                else
                    setSelectionMode(SynSelectionMode::smNormal);
                //Selection mode must be set before calling SetBlockBegin
                setBlockBegin(caretXY());
            }
        }
    }
}

void SynEdit::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseReleaseEvent(event);
    int X=event->pos().x();
    int Y=event->pos().y();

    if (!mMouseMoved && (X < mGutterWidth + 2)) {
        processGutterClick(event);
    }

    mScrollTimer->stop();
//    if ((button = ) and (Shift = [ssRight]) and Assigned(PopupMenu) then
//      exit;
    setMouseTracking(false);

    if (mStateFlags.testFlag(SynStateFlag::sfWaitForDragging) &&
            !mStateFlags.testFlag(SynStateFlag::sfDblClicked)) {
        computeCaret(X, Y);
        if (! (event->modifiers() & Qt::ShiftModifier))
            setBlockBegin(caretXY());
        setBlockEnd(caretXY());
        mStateFlags.setFlag(SynStateFlag::sfWaitForDragging, false);
    }
    mStateFlags.setFlag(SynStateFlag::sfDblClicked,false);
}

void SynEdit::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseMoveEvent(event);
    mMouseMoved = true;
    Qt::MouseButtons buttons = event->buttons();
    int X=event->pos().x();
    int Y=event->pos().y();
//    if (!hasMouseTracking())
//        return;

    if ((mStateFlags.testFlag(SynStateFlag::sfWaitForDragging))) {
        if ( ( event->pos() - mMouseDownPos).manhattanLength()>=QApplication::startDragDistance()) {
            mStateFlags.setFlag(SynStateFlag::sfWaitForDragging);
            //BeginDrag(false);
        }
//    } else if ((buttons == Qt::LeftButton) && (X > mGutterWidth)) {
    } else if ((buttons == Qt::LeftButton)) {
      // should we begin scrolling?
      computeScroll(X, Y);
      DisplayCoord P = pixelsToNearestRowColumn(X, Y);
      P.Row = minMax(P.Row, 1, displayLineCount());
      if (mScrollDeltaX != 0)
          P.Column = displayX();
      if (mScrollDeltaY != 0)
          P.Row = displayY();
      internalSetCaretXY(displayToBufferPos(P));
      setBlockEnd(caretXY());
    } else if (buttons == Qt::NoButton) {
        updateMouseCursor();
    }
}

void SynEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseDoubleClickEvent(event);
    QPoint ptMouse = event->pos();
    if (ptMouse.x() >= mGutterWidth + 2) {
      if (!mOptions.testFlag(eoNoSelection))
          setWordBlock(caretXY());
      mStateFlags.setFlag(SynStateFlag::sfDblClicked);
      //MouseCapture := FALSE;
    }
}

void SynEdit::inputMethodEvent(QInputMethodEvent *event)
{
//    qDebug()<<event->replacementStart()<<":"<<event->replacementLength()<<" - "
//           << event->preeditString()<<" - "<<event->commitString();

    QString oldString = mInputPreeditString;
    mInputPreeditString = event->preeditString();
    if (oldString!=mInputPreeditString) {
        invalidateLine(mCaretY);
    }
    QString s = event->commitString();
    if (!s.isEmpty()) {
        commandProcessor(SynEditorCommand::ecImeStr,QChar(),&s);
//        for (QChar ch:s) {
//            CommandProcessor(SynEditorCommand::ecChar,ch);
//        }
    }
}

void SynEdit::leaveEvent(QEvent *)
{
    setCursor(Qt::ArrowCursor);
}

void SynEdit::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y()>0) {
        verticalScrollBar()->setValue(verticalScrollBar()->value()-mMouseWheelScrollSpeed);
        event->accept();
        return;
    } else if (event->angleDelta().y()<0) {
        verticalScrollBar()->setValue(verticalScrollBar()->value()+mMouseWheelScrollSpeed);
        event->accept();
        return;
    }
    QAbstractScrollArea::wheelEvent(event);
}

bool SynEdit::viewportEvent(QEvent * event)
{
//    switch (event->type()) {
//        case QEvent::Resize:
//            sizeOrFontChanged(false);
//        break;
//    }
    return QAbstractScrollArea::viewportEvent(event);
}

int SynEdit::maxScrollHeight() const
{
    if (mOptions.testFlag(eoScrollPastEof))
        return std::max(displayLineCount(),1);
    else
        return std::max(displayLineCount()-mLinesInWindow+1, 1);
}

bool SynEdit::modified() const
{
    return mModified;
}

void SynEdit::setModified(bool Value)
{
    if (Value) {
        mLastModifyTime = QDateTime::currentDateTime();
        emit statusChanged(SynStatusChange::scModified);
    }
    if (Value != mModified) {
        mModified = Value;
        if (mOptions.testFlag(SynEditorOption::eoGroupUndo) && (!Value) && mUndoList->CanUndo())
            mUndoList->AddGroupBreak();
        mUndoList->setInitialState(!Value);
        emit statusChanged(SynStatusChange::scModifyChanged);
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
        onSizeOrFontChanged(false);
        invalidate();
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

void SynEdit::onBookMarkOptionsChanged()
{
    invalidateGutter();
}

void SynEdit::onLinesChanged()
{
    SynSelectionMode vOldMode;
    mStateFlags.setFlag(SynStateFlag::sfLinesChanging, false);

    updateScrollbars();
    vOldMode = mActiveSelectionMode;
    setBlockBegin(caretXY());
    mActiveSelectionMode = vOldMode;
    if (mInvalidateRect.width()==0)
        invalidate();
    else
        invalidateRect(mInvalidateRect);
    mInvalidateRect = {0,0,0,0};
    if (mGutter.showLineNumbers() && (mGutter.autoSize()))
        mGutter.autoSizeDigitCount(mLines->count());
    //if (!mOptions.testFlag(SynEditorOption::eoScrollPastEof))
    setTopLine(mTopLine);
}

void SynEdit::onLinesChanging()
{
    mStateFlags.setFlag(SynStateFlag::sfLinesChanging);
}

void SynEdit::onLinesCleared()
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

void SynEdit::onLinesDeleted(int index, int count)
{
    if (mUseCodeFolding)
        foldOnListDeleted(index + 1, count);
    if (mHighlighter && mLines->count() > 0)
        scanFrom(index, index+1);
    invalidateLines(index + 1, INT_MAX);
    invalidateGutterLines(index + 1, INT_MAX);
}

void SynEdit::onLinesInserted(int index, int count)
{
    if (mUseCodeFolding)
        foldOnListInserted(index + 1, count);
    if (mHighlighter && mLines->count() > 0) {
//        int vLastScan = index;
//        do {
          scanFrom(index, index+count);
//            vLastScan++;
//        } while (vLastScan < index + count) ;
    }
    invalidateLines(index + 1, INT_MAX);
    invalidateGutterLines(index + 1, INT_MAX);
}

void SynEdit::onLinesPutted(int index, int count)
{
    int vEndLine = index + 1;
    if (mHighlighter) {
        vEndLine = std::max(vEndLine, scanFrom(index, index+count) + 1);
        // If this editor is chained then the real owner of text buffer will probably
        // have already parsed the changes, so ScanFrom will return immediately.
        if (mLines != mOrigLines)
            vEndLine = INT_MAX;
    }
    invalidateLines(index + 1, vEndLine);
}

void SynEdit::onUndoAdded()
{
    updateModifiedStatus();

    // we have to clear the redo information, since adding undo info removes
    // the necessary context to undo earlier edit actions
    if (! mUndoList->insideRedo() &&
            mUndoList->PeekItem() && (mUndoList->PeekItem()->changeReason()!=SynChangeReason::crGroupBreak))
        mRedoList->Clear();
    if (mUndoList->blockCount() == 0 )
        onChanged();
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
    if ((mBlockEnd.Line < mBlockBegin.Line)
      || ((mBlockEnd.Line == mBlockBegin.Line) && (mBlockEnd.Char < mBlockBegin.Char)))
        return mBlockBegin;
    else
        return mBlockEnd;

}

void SynEdit::setBlockEnd(BufferCoord Value)
{
    setActiveSelectionMode(mSelectionMode);
    if (!mOptions.testFlag(eoNoSelection)) {
        Value.Line = minMax(Value.Line, 1, mLines->count());
        Value.Char = minMax(Value.Char, 1, mLines->lengthOfLongestLine()+1);
        if (mActiveSelectionMode == SynSelectionMode::smNormal) {
          if (Value.Line >= 1 && Value.Line <= mLines->count())
              Value.Char = std::min(Value.Char, mLines->getString(Value.Line - 1).length() + 1);
          else
              Value.Char = 1;
        }
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

void SynEdit::setSelLength(int Value)
{
    if (mBlockBegin.Line>mLines->count() || mBlockBegin.Line<=0)
        return;

    if (Value >= 0) {
        int y = mBlockBegin.Line;
        int ch = mBlockBegin.Char;
        int x = ch + Value;
        QString line;
        while (y<=mLines->count()) {
            line = mLines->getString(y-1);
            if (x <= line.length()+2) {
                if (x==line.length()+2)
                    x = line.length()+1;
                break;
            }
            x -= line.length()+2;
            y ++;
        }
        if (y>mLines->count()) {
            y = mLines->count();
            x = mLines->getString(y-1).length()+1;
        }
        BufferCoord iNewEnd{x,y};
        setCaretAndSelection(iNewEnd, mBlockBegin, iNewEnd);
    } else {
        int y = mBlockBegin.Line;
        int ch = mBlockBegin.Char;
        int x = ch + Value;
        QString line;
        while (y>=1) {
            if (x>=0) {
                if (x==0)
                    x = 1;
                break;
            }
            y--;
            line = mLines->getString(y-1);
            x += line.length()+2;
        }
        if (y>mLines->count()) {
            y = mLines->count();
            x = mLines->getString(y-1).length()+1;
        }
        BufferCoord iNewStart{x,y};
        setCaretAndSelection(iNewStart, iNewStart, mBlockBegin);
    }
}

void SynEdit::setSelText(const QString &text)
{
    doSetSelText(text);
}

BufferCoord SynEdit::blockBegin() const
{
    if ((mBlockEnd.Line < mBlockBegin.Line)
      || ((mBlockEnd.Line == mBlockBegin.Line) && (mBlockEnd.Char < mBlockBegin.Char)))
        return mBlockEnd;
    else
        return mBlockBegin;
}

void SynEdit::setBlockBegin(BufferCoord value)
{
    int nInval1, nInval2;
    bool SelChanged;
    setActiveSelectionMode(mSelectionMode);
    value.Char = minMax(value.Char, 1, mLines->lengthOfLongestLine()+1);
    value.Line = minMax(value.Line, 1, mLines->count());
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
    //int MaxVal;
    //QRect iTextArea;
    Value = std::min(Value,maxScrollWidth());
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
    Value = std::min(Value,maxScrollHeight());
//    if (mOptions.testFlag(SynEditorOption::eoScrollPastEof))
//        Value = std::min(Value, displayLineCount());
//    else
//        Value = std::min(Value, displayLineCount() - mLinesInWindow + 1);
    Value = std::max(Value, 1);
    if (Value != mTopLine) {
        verticalScrollBar()->setValue(Value);
        setStatusChanged(SynStatusChange::scTopLine);
    }
}

void SynEdit::onRedoAdded()
{
    updateModifiedStatus();

    if (mRedoList->blockCount() == 0 )
        onChanged();
}

void SynEdit::onGutterChanged()
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

void SynEdit::onScrollTimeout()
{
    QPoint iMousePos;
    DisplayCoord C;
    int X, Y;

    iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    C = pixelsToRowColumn(iMousePos.x(), iMousePos.y());
    C.Row = minMax(C.Row, 1, displayLineCount());
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
        C.Row = minMax(Y, 1, displayLineCount());
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
//        if (mouseCapture())
        setBlockEnd(caretXY());
    }
    computeScroll(iMousePos.x(), iMousePos.y());
}
