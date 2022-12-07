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
#include "SynEdit.h"
#include "highlighter/cpp.h"
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
#include <QDrag>
#include <QMimeData>
#include <QDesktopWidget>
#include <QTextEdit>
#include <QMimeData>

namespace QSynedit {
SynEdit::SynEdit(QWidget *parent) : QAbstractScrollArea(parent),
    mDropped(false)
{
    mCharWidth=1;
    mTextHeight = 1;
    mLastKey = 0;
    mLastKeyModifiers = Qt::NoModifier;
    mModified = false;
    mPaintLock = 0;
    mPainterLock = 0;
    mPainting = false;
#ifdef Q_OS_WIN
    mFontDummy = QFont("Consolas",12);
#elif defined(Q_OS_LINUX)
    mFontDummy = QFont("terminal",14);
#elif defined(Q_OS_MACOS)
    mFontDummy = QFont("Menlo", 14);
#else
#error "Not supported!"
#endif
    mFontDummy.setStyleStrategy(QFont::PreferAntialias);
    mDocument = std::make_shared<Document>(mFontDummy, mFontDummy, this);
    //fPlugins := TList.Create;
    mMouseMoved = false;
    mUndoing = false;
    mDocument->connect(mDocument.get(), &Document::changed, this, &SynEdit::onLinesChanged);
    mDocument->connect(mDocument.get(), &Document::changing, this, &SynEdit::onLinesChanging);
    mDocument->connect(mDocument.get(), &Document::cleared, this, &SynEdit::onLinesCleared);
    mDocument->connect(mDocument.get(), &Document::deleted, this, &SynEdit::onLinesDeleted);
    mDocument->connect(mDocument.get(), &Document::inserted, this, &SynEdit::onLinesInserted);
    mDocument->connect(mDocument.get(), &Document::putted, this, &SynEdit::onLinesPutted);

    mGutterWidth = 0;
    mScrollBars = ScrollStyle::ssBoth;

    mUndoList = std::make_shared<UndoList>();
    mUndoList->connect(mUndoList.get(), &UndoList::addedUndo, this, &SynEdit::onUndoAdded);
    mRedoList = std::make_shared<RedoList>();
//    mRedoList->connect(mRedoList.get(), &SynEditUndoList::addedUndo, this, &SynEdit::onRedoAdded);

    mForegroundColor=palette().color(QPalette::Text);
    mBackgroundColor=palette().color(QPalette::Base);
    mCaretColor = Qt::red;
    mCaretUseTextColor = false;
    mActiveLineColor = Qt::blue;
    mSelectedBackground = palette().color(QPalette::Highlight);
    mSelectedForeground = palette().color(QPalette::HighlightedText);

    //  fRightEdge has to be set before FontChanged is called for the first time
    mRightEdge = 80;

    mMouseWheelScrollSpeed = 3;
    mMouseSelectionScrollSpeed = 1;

    mGutter.setRightOffset(21);
    mGutter.connect(&mGutter, &Gutter::changed, this, &SynEdit::onGutterChanged);
    mGutterWidth = mGutter.realGutterWidth(charWidth());
    //ControlStyle := ControlStyle + [csOpaque, csSetCaption, csNeedsBorderPaint];
    //Height := 150;
    //Width := 200;
    this->setCursor(Qt::CursorShape::IBeamCursor);
    //TabStop := True;
    mInserting = true;
    mExtraLineSpacing = 0;

    this->setFrameShape(QFrame::Panel);
    this->setFrameShadow(QFrame::Sunken);
    this->setLineWidth(1);
    mInsertCaret = EditCaretType::ctVerticalLine;
    mOverwriteCaret = EditCaretType::ctBlock;
    mSelectionMode = SelectionMode::Normal;
    mActiveSelectionMode = SelectionMode::Normal;
    mReadOnly = false;

    //stop qt to auto fill background
    setAutoFillBackground(false);
    //fFocusList := TList.Create;
    //fKbdHandler := TSynEditKbdHandler.Create;
    //fMarkList.OnChange := MarkListChange;
    setDefaultKeystrokes();
    mRightEdgeColor = Qt::lightGray;

    mWantReturns = true;
    mWantTabs = false;
    mLeftChar = 1;
    mTopLine = 1;
    mCaretX = 1;
    mLastCaretColumn = 1;
    mCaretY = 1;
    mBlockBegin.ch = 1;
    mBlockBegin.line = 1;
    mBlockEnd = mBlockBegin;
    mOptions = eoAutoIndent
            | eoDragDropEditing | eoEnhanceEndKey | eoTabIndent |
             eoGroupUndo | eoKeepCaretX | eoSelectWordByDblClick
            | eoHideShowScrollbars ;

    mScrollTimer = new QTimer(this);
    //mScrollTimer->setInterval(100);
    connect(mScrollTimer, &QTimer::timeout,this, &SynEdit::onScrollTimeout);

    qreal dpr=devicePixelRatioF();
    mContentImage = std::make_shared<QImage>(clientWidth()*dpr,clientHeight()*dpr,QImage::Format_ARGB32);
    mContentImage->setDevicePixelRatio(dpr);

    mUseCodeFolding = true;
    m_blinkTimerId = 0;
    m_blinkStatus = 0;

    hideCaret();

    connect(horizontalScrollBar(),&QScrollBar::valueChanged,
            this, &SynEdit::onScrolled);
    connect(verticalScrollBar(),&QScrollBar::valueChanged,
            this, &SynEdit::onScrolled);
    //enable input method
    setAttribute(Qt::WA_InputMethodEnabled);

    //setMouseTracking(true);
    setAcceptDrops(true);

    setFont(mFontDummy);
    setFontForNonAscii(mFontDummy);
}

int SynEdit::displayLineCount() const
{
    if (mDocument->empty()) {
        return 0;
    }
    return lineToRow(mDocument->count());
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
    result.ch = caretX();
    result.line = caretY();
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
    setBlockBegin(value);
    setBlockEnd(value);
    setCaretXYEx(true,value);
}

void SynEdit::setCaretXYEx(bool CallEnsureCursorPosVisible, BufferCoord value)
{
    int nMaxX;
    if (value.line > mDocument->count())
        value.line = mDocument->count();
    if (mActiveSelectionMode!=SelectionMode::Column) {
        if (value.line < 1) {
            // this is just to make sure if Lines stringlist should be empty
            value.line = 1;
            if (!mOptions.testFlag(EditorOption::eoScrollPastEol)) {
                nMaxX = 1;
            } else {
                nMaxX = getDisplayStringAtLine(value.line).length()+1;
            }
        } else {
            nMaxX = getDisplayStringAtLine(value.line).length()+1;
        }
        value.ch = std::min(value.ch,nMaxX);
    }
    value.ch = std::max(value.ch,1);
//    if ((value.Char > nMaxX) && (! (mOptions.testFlag(SynEditorOption::eoScrollPastEol)) ) )
//        value.Char = nMaxX;
//    if (value.Char < 1)
//        value.Char = 1;
    if ((value.ch != mCaretX) || (value.line != mCaretY)) {
        incPaintLock();
        auto action = finally([this]{
            decPaintLock();
        });
        // simply include the flags, fPaintLock is > 0
        if (mCaretX != value.ch) {
            mCaretX = value.ch;
            mStatusChanges.setFlag(StatusChange::scCaretX);
            invalidateLine(mCaretY);
        }
        if (mCaretY != value.line) {
            int oldCaretY = mCaretY;
            mCaretY = value.line;
            invalidateLine(mCaretY);
            invalidateGutterLine(mCaretY);
            invalidateLine(oldCaretY);
            invalidateGutterLine(oldCaretY);
            mStatusChanges.setFlag(StatusChange::scCaretY);
        }
        // Call UpdateLastCaretX before DecPaintLock because the event handler it
        // calls could raise an exception, and we don't want fLastCaretX to be
        // left in an undefined state if that happens.
        updateLastCaretX();
        if (CallEnsureCursorPosVisible)
            ensureCursorPosVisible();
        mStateFlags.setFlag(StateFlag::sfCaretChanged);
        mStateFlags.setFlag(StateFlag::sfScrollbarChanged);
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

}

void SynEdit::setCaretXYCentered(const BufferCoord &value)
{
    incPaintLock();
    auto action = finally([this] {
        decPaintLock();
    });
    mStatusChanges.setFlag(StatusChange::scSelection);
    setCaretXYEx(false,value);
    if (selAvail())
        invalidateSelection();
    mBlockBegin.ch = mCaretX;
    mBlockBegin.line = mCaretY;
    mBlockEnd = mBlockBegin;
    ensureCursorPosVisibleEx(true); // but here after block has been set
}

void SynEdit::uncollapseAroundLine(int line)
{
    while (true) { // Open up the closed folds around the focused line until we can see the line we're looking for
      PCodeFoldingRange fold = foldHidesLine(line);
      if (fold)
          uncollapse(fold);
      else
          break;
    }
}

PCodeFoldingRange SynEdit::foldHidesLine(int line)
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
    return !mReadOnly && mUndoList->canUndo();
}

bool SynEdit::canRedo() const
{
    return !mReadOnly && mRedoList->canRedo();
}

int SynEdit::maxScrollWidth() const
{
    int maxLen = mDocument->lengthOfLongestLine();
    if (highlighter())
        maxLen = maxLen+stringColumns(highlighter()->foldString(),maxLen);
    if (mOptions.testFlag(eoScrollPastEol))
        return std::max(maxLen ,1);
    else
        return std::max(maxLen-mCharsInWindow+1, 1);
}

bool SynEdit::getHighlighterAttriAtRowCol(const BufferCoord &pos, QString &token, PHighlighterAttribute &attri)
{
    int tmpStart;
    return getHighlighterAttriAtRowColEx(pos, token, tmpStart, attri);
}

bool SynEdit::getHighlighterAttriAtRowCol(const BufferCoord &pos, QString &token, bool &tokenFinished, PHighlighterAttribute &attri)
{
    int posX, posY, endPos, start;
    QString line;
    posY = pos.line - 1;
    if (mHighlighter && (posY >= 0) && (posY < mDocument->count())) {
        line = mDocument->getString(posY);
        if (posY == 0) {
            mHighlighter->resetState();
        } else {
            mHighlighter->setState(mDocument->ranges(posY-1));
        }
        mHighlighter->setLine(line, posY);
        posX = pos.ch;
        if ((posX > 0) && (posX <= line.length())) {
            while (!mHighlighter->eol()) {
                start = mHighlighter->getTokenPos() + 1;
                token = mHighlighter->getToken();
                endPos = start + token.length()-1;
                if ((posX >= start) && (posX <= endPos)) {
                    attri = mHighlighter->getTokenAttribute();
                    if (posX == endPos)
                        tokenFinished = mHighlighter->getTokenFinished();
                    else
                        tokenFinished = false;
                    return true;
                }
                mHighlighter->next();
            }
        }
    }
    token = "";
    attri = PHighlighterAttribute();
    tokenFinished = false;
    return false;
}

bool SynEdit::getHighlighterAttriAtRowColEx(const BufferCoord &pos, QString &token, int &start, PHighlighterAttribute &attri)
{
    int posX, posY, endPos;
    QString line;
    posY = pos.line - 1;
    if (mHighlighter && (posY >= 0) && (posY < mDocument->count())) {
        line = mDocument->getString(posY);
        if (posY == 0) {
            mHighlighter->resetState();
        } else {
            mHighlighter->setState(mDocument->ranges(posY-1));
        }
        mHighlighter->setLine(line, posY);
        posX = pos.ch;
        if ((posX > 0) && (posX <= line.length())) {
            while (!mHighlighter->eol()) {
                start = mHighlighter->getTokenPos() + 1;
                token = mHighlighter->getToken();
                endPos = start + token.length()-1;
                if ((posX >= start) && (posX <= endPos)) {
                    attri = mHighlighter->getTokenAttribute();
                    return true;
                }
                mHighlighter->next();
            }
        }
    }
    token = "";
    attri = PHighlighterAttribute();
    return false;
}

void SynEdit::beginUndoBlock()
{
    mUndoList->beginBlock();
}

void SynEdit::endUndoBlock()
{
    mUndoList->endBlock();
}

void SynEdit::addCaretToUndo()
{
    BufferCoord p=caretXY();
    mUndoList->addChange(ChangeReason::Caret,p,p,QStringList(), mActiveSelectionMode);
}

void SynEdit::addLeftTopToUndo()
{
    BufferCoord p;
    p.ch = leftChar();
    p.line = topLine();
    mUndoList->addChange(ChangeReason::LeftTop,p,p,QStringList(), mActiveSelectionMode);
}

void SynEdit::addSelectionToUndo()
{
    mUndoList->addChange(ChangeReason::Selection,mBlockBegin,
                         mBlockEnd,QStringList(),mActiveSelectionMode);
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
    PHighlighterAttribute attr;
    BufferCoord p;
    bool isCommentOrStringOrChar;
    int nBrackets = sizeof(Brackets) / sizeof(QChar);

    if (mDocument->count()<1)
        return BufferCoord{0,0};
    // get char at caret
    PosX = std::max(APoint.ch,1);
    PosY = std::max(APoint.line,1);
    Line = mDocument->getString(APoint.line - 1);
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
                            p.ch = PosX;
                            p.line = PosY;
                            if ((Test == BracketInc) || (Test == BracketDec)) {
                                isCommentOrStringOrChar = false;
                                if (getHighlighterAttriAtRowCol(p, vDummy, attr))
                                    isCommentOrStringOrChar =
                                        (attr->tokenType() == TokenType::String) ||
                                            (attr->tokenType() == TokenType::Comment) ||
                                            (attr->tokenType() == TokenType::Character);
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
                        Line = mDocument->getString(PosY - 1);
                        PosX = Line.length() + 1;
                    }
                } else {
                    while (true) {
                        // search until end of line
                        Len = Line.length();
                        while (PosX < Len) {
                            PosX++;
                            Test = Line[PosX-1];
                            p.ch = PosX;
                            p.line = PosY;
                            if ((Test == BracketInc) || (Test == BracketDec)) {
                                isCommentOrStringOrChar = false;
                                if (getHighlighterAttriAtRowCol(p, vDummy, attr))
                                    isCommentOrStringOrChar =
                                        (attr->tokenType() == TokenType::String) ||
                                            (attr->tokenType() == TokenType::Comment) ||
                                            (attr->tokenType() == TokenType::Character);
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
                        if (PosY == mDocument->count())
                            break;
                        PosY++;
                        Line = mDocument->getString(PosY - 1);
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
    return document()->contents();
}

QString SynEdit::text()
{
    return document()->text();
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

    coord = displayToBufferPos(pixelsToNearestRowColumn(point.x(),point.y()));
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

    BufferCoord coord = displayToBufferPos(pixelsToNearestRowColumn(point.x(),point.y()));
    line = coord.line;
    return true;
}

void SynEdit::invalidateGutter()
{
    invalidateGutterLines(-1, -1);
}

void SynEdit::invalidateGutterLine(int aLine)
{
    if ((aLine < 1) || (aLine > mDocument->count()))
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
        if (mStateFlags.testFlag(StateFlag::sfLinesChanging))
            mInvalidateRect = mInvalidateRect.united(rcInval);
        else
            invalidateRect(rcInval);
    } else {
        // find the visible lines first
        if (LastLine < FirstLine)
            std::swap(LastLine, FirstLine);
        if (mUseCodeFolding) {
            FirstLine = lineToRow(FirstLine);
            if (LastLine <= mDocument->count())
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
            if (mStateFlags.testFlag(StateFlag::sfLinesChanging)) {
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
    return {
        std::max(1, (int)(mLeftChar + round((aX - mGutterWidth - 2.0) / mCharWidth))),
        std::max(1, mTopLine + (aY / mTextHeight))
    };
}

DisplayCoord SynEdit::pixelsToRowColumn(int aX, int aY) const
{
    return {
        std::max(1, (int)(mLeftChar + (aX - mGutterWidth - 2.0) / mCharWidth)),
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
    DisplayCoord result {p.ch,p.line};
    // Account for tabs and charColumns
    if (p.line-1 <mDocument->count())
        result.Column = charToColumn(p.line,p.ch);
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
        Result.line = foldRowToLine(p.Row);
    // Account for tabs
    if (Result.line <= mDocument->count() ) {
        Result.ch = columnToChar(Result.line,p.Column);
    }
    return Result;
}

//ContentsCoord SynEdit::fromBufferCoord(const BufferCoord &p) const
//{
//    return createNormalizedBufferCoord(p.Char,p.Line);
//}

//ContentsCoord SynEdit::createNormalizedBufferCoord(int aChar, int aLine) const
//{
//    return ContentsCoord(this,aChar,aLine);
//}

//QStringList SynEdit::getContents(const ContentsCoord &pStart, const ContentsCoord &pEnd)
//{
//    QStringList result;
//    if (mDocument->count()==0)
//        return result;
//    if (pStart.line()>0) {
//        QString s = mDocument->getString(pStart.line()-1);
//        result += s.mid(pStart.ch()-1);
//    }
//    int endLine = std::min(pEnd.line(),mDocument->count());
//    for (int i=pStart.line();i<endLine-1;i++) {
//        result += mDocument->getString(i);
//    }
//    if (pEnd.line()<=mDocument->count()) {
//        result += mDocument->getString(pEnd.line()-1).mid(0,pEnd.ch()-1);
//    }
//    return result;
//}

//QString SynEdit::getJoinedContents(const ContentsCoord &pStart, const ContentsCoord &pEnd, const QString &joinStr)
//{
//    return getContents(pStart,pEnd).join(joinStr);
//}

int SynEdit::leftSpaces(const QString &line) const
{
    int result = 0;
    if (mOptions.testFlag(eoAutoIndent)) {
        for (QChar ch:line) {
            if (ch == '\t') {
                result += tabWidth() - (result % tabWidth());
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
    if (wantTabs && !mOptions.testFlag(eoTabsToSpaces) && tabWidth()>0) {
        return QString(charCount / tabWidth(),'\t') + QString(charCount % tabWidth(),' ');
    } else {
        return QString(charCount,' ');
    }
}

int SynEdit::charToColumn(int aLine, int aChar) const
{
    if (aLine>=1 && aLine <= mDocument->count()) {
        QString s = getDisplayStringAtLine(aLine);
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
            x+=tabWidth() - (x % tabWidth());
        else
            x+=charColumns(s[i]);
    }
    return x+1;
}

int SynEdit::columnToChar(int aLine, int aColumn) const
{
    Q_ASSERT( (aLine <= mDocument->count()) && (aLine >= 1));
    if (aLine <= mDocument->count()) {
        QString s = getDisplayStringAtLine(aLine);
        int x = 0;
        int len = s.length();
        int i;
        for (i=0;i<len;i++) {
            if (s[i] == '\t')
                x+=tabWidth() - (x % tabWidth());
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
    return mDocument->stringColumns(line,colsBefore);
}

int SynEdit::getLineIndent(const QString &line) const
{
    int indents = 0;
    for (QChar ch:line) {
        switch(ch.unicode()) {
        case '\t':
            indents+=tabWidth();
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
        PCodeFoldingRange range = mAllFoldRanges[i];
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
        PCodeFoldingRange range =mAllFoldRanges[i];
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

void SynEdit::setExtraKeystrokes()
{
    mKeyStrokes.setExtraKeyStrokes();
}

void SynEdit::invalidateLine(int Line)
{
    QRect rcInval;
    if (mPainterLock >0)
        return;
    if (Line<1 || (Line>mDocument->count() &&
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
        if (mStateFlags.testFlag(StateFlag::sfLinesChanging))
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
        if (mStateFlags.testFlag(StateFlag::sfLinesChanging)) {
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

        if (LastLine >= mDocument->count())
          LastLine = INT_MAX; // paint empty space beyond last line

        if (mUseCodeFolding) {
          FirstLine = lineToRow(FirstLine);
          // Could avoid this conversion if (First = Last) and
          // (Length < CharsInWindow) but the dependency isn't worth IMO.
          if (LastLine < mDocument->count())
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
            if (mStateFlags.testFlag(StateFlag::sfLinesChanging))
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
    invalidateLines(blockBegin().line, blockEnd().line);
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
    if (mBlockBegin.ch == mBlockEnd.ch && mBlockBegin.line == mBlockEnd.line)
        return false;
    // start line != end line  or start char != end char
    if (mActiveSelectionMode==SelectionMode::Column) {
        if (mBlockBegin.line != mBlockEnd.line) {
            DisplayCoord coordBegin = bufferToDisplayPos(mBlockBegin);
            DisplayCoord coordEnd = bufferToDisplayPos(mBlockEnd);
            return coordBegin.Column!=coordEnd.Column;
        } else
            return true;
    }
    return true;
}

bool SynEdit::colSelAvail() const
{
    if (mActiveSelectionMode != SelectionMode::Column)
        return false;
    if (mBlockBegin.ch == mBlockEnd.ch && mBlockBegin.line == mBlockEnd.line)
        return false;
    if (mBlockBegin.line == mBlockEnd.line && mBlockBegin.ch!=mBlockEnd.ch)
        return true;
    DisplayCoord coordBegin = bufferToDisplayPos(mBlockBegin);
    DisplayCoord coordEnd = bufferToDisplayPos(mBlockEnd);
    return coordBegin.Column!=coordEnd.Column;
}

QString SynEdit::wordAtCursor()
{
    return wordAtRowCol(caretXY());
}

QString SynEdit::wordAtRowCol(const BufferCoord &pos)
{
    if ((pos.line >= 1) && (pos.line <= mDocument->count())) {
        QString line = mDocument->getString(pos.line - 1);
        int len = line.length();
        if (len == 0)
            return "";
        if (pos.ch<1 || pos.ch>len)
            return "";

        int start = pos.ch - 1;
        if  ((start> 0) && !isIdentChar(line[start]))
             start--;

        if (isIdentChar(line[start])) {
            int stop = start;
            while ((stop < len) && isIdentChar(line[stop]))
                stop++;
            while ((start-1 >=0) && isIdentChar(line[start - 1]))
                start--;
            if (stop > start)
                return line.mid(start,stop-start);
        }
    }
    return "";
}

QChar SynEdit::charAt(const BufferCoord &pos)
{
    if ((pos.line >= 1) && (pos.line <= mDocument->count())) {
        QString line = mDocument->getString(pos.line-1);
        int len = line.length();
        if (len == 0)
            return QChar(0);
        if (pos.ch<1 || pos.ch>len)
            return QChar(0);
        return line[pos.ch-1];
    }
    return QChar(0);
}

QChar SynEdit::nextNonSpaceChar(int line, int ch)
{
    if (ch<0)
        return QChar();
    QString s = mDocument->getString(line);
    if (s.isEmpty())
        return QChar();
    int x=ch;
    while (x<s.length()) {
        QChar ch = s[x];
        if (!ch.isSpace())
            return ch;
        x++;
    }
    return QChar();
}

QChar SynEdit::lastNonSpaceChar(int line, int ch)
{
    if (line>=mDocument->count())
        return QChar();
    QString s = mDocument->getString(line);
    int x = std::min(ch-1,s.length()-1);
    while (line>=0) {
        while (x>=0) {
            QChar c = s[x];
            if (!c.isSpace())
                return c;
            x--;
        }
        line--;
        if (line>=0) {
            s = mDocument->getString(line);
            x = s.length()-1;
        }
    }
    return QChar();
}

void SynEdit::setCaretAndSelection(const BufferCoord &ptCaret, const BufferCoord &ptSelBegin, const BufferCoord &ptSelEnd)
{
    incPaintLock();
    internalSetCaretXY(ptCaret);
    setBlockBegin(ptSelBegin);
    setBlockEnd(ptSelEnd);
    decPaintLock();
}

bool SynEdit::inputMethodOn()
{
    return !mInputPreeditString.isEmpty();
}

void SynEdit::collapseAll()
{
    incPaintLock();
    for (int i = mAllFoldRanges.count()-1;i>=0;i--){
        collapse(mAllFoldRanges[i]);
    }
    decPaintLock();
}

void SynEdit::unCollpaseAll()
{
    incPaintLock();
    for (int i = mAllFoldRanges.count()-1;i>=0;i--){
        uncollapse(mAllFoldRanges[i]);
    }
    decPaintLock();
}

void SynEdit::processGutterClick(QMouseEvent *event)
{
    int X = event->pos().x();
    int Y = event->pos().y();
    DisplayCoord RowColumn = pixelsToNearestRowColumn(X, Y);
    int Line = rowToLine(RowColumn.Row);

    // Check if we clicked on a folding thing
    if (mUseCodeFolding) {
        PCodeFoldingRange foldRange = foldStartAtLine(Line);
        if (foldRange) {
            // See if we actually clicked on the rectangle...
            //rect.Left := Gutter.RealGutterWidth(CharWidth) - Gutter.RightOffset;
            QRect rect;
            rect.setLeft(mGutterWidth - mGutter.rightOffset());
            rect.setRight(rect.left() + mGutter.rightOffset() - 4);
            rect.setTop((RowColumn.Row - mTopLine) * mTextHeight);
            rect.setBottom(rect.top() + mTextHeight - 1);
            if (rect.contains(QPoint(X, Y))) {
                if (foldRange->collapsed)
                    uncollapse(foldRange);
                else
                    collapse(foldRange);
                return;
            }
        }
    }

    // If not, check gutter marks
    if (Line>=1 && Line <= mDocument->count()) {
        emit gutterClicked(event->button(),X,Y,Line);
    }
}

void SynEdit::clearUndo()
{
    mUndoList->clear();
    mRedoList->clear();
}

int SynEdit::findIndentsStartLine(int line, QVector<int> indents)
{
    line--;
    if (line<0 || line>=mDocument->count())
        return -1;
    while (line>=1) {
        HighlighterState range = mDocument->ranges(line);
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
    PHighlighterAttribute attr;
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
    QString Line = mDocument->getString(PosY - 1);
    if ((PosX > Line.length()) || (PosX<1))
        PosX = Line.length();
    int numBrackets = 1;
    while (true) {
        if (Line.isEmpty()){
            PosY--;
            if (PosY<1)
                return Result;
            Line = mDocument->getString(PosY - 1);
            PosX = Line.length();
            continue;
        }
        Test = Line[PosX-1];
        p.ch = PosX;
        p.line = PosY;
        if (Test=='{' || Test == '}') {
            if (getHighlighterAttriAtRowCol(p, vDummy, attr)) {
                isCommentOrStringOrChar =
                        (attr->tokenType() == TokenType::String) ||
                        (attr->tokenType() == TokenType::Comment) ||
                        (attr->tokenType() == TokenType::Character);
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
            Line = mDocument->getString(PosY - 1);
            PosX = Line.length();
        }
    }
}

int SynEdit::charColumns(QChar ch) const
{
    return mDocument->charColumns(ch);
}

void SynEdit::showCaret()
{
    if (m_blinkTimerId==0)
        m_blinkTimerId = startTimer(500);
    m_blinkStatus = 1;
    updateCaret();
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
    if ((Value.line >= ptBegin.line) && (Value.line <= ptEnd.line) &&
            ((ptBegin.line != ptEnd.line) || (ptBegin.ch != ptEnd.ch))) {
        if (mActiveSelectionMode == SelectionMode::Line)
            return true;
        else if (mActiveSelectionMode == SelectionMode::Column) {
            if (ptBegin.ch > ptEnd.ch)
                return (Value.ch >= ptEnd.ch) && (Value.ch < ptBegin.ch);
            else if (ptBegin.ch < ptEnd.ch)
                return (Value.ch >= ptBegin.ch) && (Value.ch < ptEnd.ch);
            else
                return false;
        } else
            return ((Value.line > ptBegin.line) || (Value.ch >= ptBegin.ch)) &&
      ((Value.line < ptEnd.line) || (Value.ch < ptEnd.ch));
    } else
        return false;
}

BufferCoord SynEdit::nextWordPos()
{
    return nextWordPosEx(caretXY());
}

BufferCoord SynEdit::nextWordPosEx(const BufferCoord &XY)
{
    int CX = XY.ch;
    int CY = XY.line;
    // valid line?
    if ((CY >= 1) && (CY <= mDocument->count())) {
        QString Line = mDocument->getString(CY - 1);
        int LineLen = Line.length();
        if (CX >= LineLen) {
            // find first IdentChar or multibyte char in the next line
            if (CY < mDocument->count()) {
                Line = mDocument->getString(CY);
                CY++;
                CX=findWordChar(Line,1);
                if (CX==0)
                    CX=1;
            }
        } else {
            // find next "whitespace" if current char is an IdentChar
            if (!Line[CX-1].isSpace())
                CX = findNonWordChar(Line,CX);
            // if "whitespace" found, find the next IdentChar
            if (CX > 0)
                CX = findWordChar(Line, CX);
            // if one of those failed position at the begin of next line
            if (CX == 0) {
                if (CY < mDocument->count()) {
                    Line = mDocument->getString(CY);
                    CY++;
                    CX=findWordChar(Line,1);
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
    int CX = XY.ch;
    int CY = XY.line;
    // valid line?
    if ((CY >= 1) && (CY <= mDocument->count())) {
        QString Line = mDocument->getString(CY - 1);
        CX = std::min(CX, Line.length()+1);
        if (CX > 1) {
            if (isWordChar(Line[CX - 2]))
                CX = findLastNonWordChar(Line, CX - 1) + 1;
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
    int CX = XY.ch;
    int CY = XY.line;
    // valid line?
    if ((CY >= 1) && (CY <= mDocument->count())) {
        QString Line = mDocument->getString(CY - 1);
        if (CX <= Line.length() && CX-1>=0) {
            if (isWordChar(Line[CX - 1]))
                CX = findNonWordChar(Line, CX);
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
    int CX = XY.ch;
    int CY = XY.line;
    // valid line?
    if ((CY >= 1) && (CY <= mDocument->count())) {
        QString Line = mDocument->getString(CY - 1);
        CX = std::min(CX, Line.length());
        if (CX <= 1) {
            // find last IdentChar in the previous line
            if (CY > 1) {
                CY -- ;
                Line = mDocument->getString(CY - 1);
                CX = findLastWordChar(Line, Line.length())+1;
            }
        } else {
            // if previous char is a "whitespace" search for the last IdentChar
            if (!isWordChar(Line[CX - 2]))
                CX = findLastWordChar(Line, CX - 1);
            if (CX > 0) // search for the first IdentChar of this "word"
                CX = findLastNonWordChar(Line, CX - 1)+1;
            if (CX == 0) {
                // find last IdentChar in the previous line
                if (CY > 1) {
                    CY -- ;
                    Line = mDocument->getString(CY - 1);
                    CX = findLastWordChar(Line, Line.length())+1;
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

void SynEdit::setWordBlock(BufferCoord value)
{
//    if (mOptions.testFlag(eoScrollPastEol))
//        Value.Char =
//    else
//        Value.Char = std::max(Value.Char, 1);
    value.line = minMax(value.line, 1, mDocument->count());
    value.ch = std::max(value.ch, 1);
    QString TempString = mDocument->getString(value.line - 1); //needed for CaretX = LineLength +1
    if (value.ch > TempString.length()) {
        internalSetCaretXY(BufferCoord{TempString.length()+1, value.line});
        return;
    }

    BufferCoord vWordStart = wordStartEx(value);
    BufferCoord vWordEnd = wordEndEx(value);
    if ((vWordStart.line == vWordEnd.line) && (vWordStart.ch < vWordEnd.ch))
        setCaretAndSelection(vWordEnd, vWordStart, vWordEnd);
}

void SynEdit::doExpandSelection(const BufferCoord &pos)
{
    if (selAvail()) {
        //todo
    } else {
        setWordBlock(pos);
    }
}

void SynEdit::doShrinkSelection(const BufferCoord &/*pos*/)
{
    //todo
}

int SynEdit::findCommentStartLine(int searchStartLine)
{
    int commentStartLine = searchStartLine;
    HighlighterState range;
    while (commentStartLine>=1) {
        range = mDocument->ranges(commentStartLine-1);
        if (!mHighlighter->isLastLineCommentNotFinished(range.state)){
            commentStartLine++;
            break;
        }
        if (!range.matchingIndents.isEmpty()
                || range.firstIndentThisLine<range.indents.length())
            break;
        commentStartLine--;
    }
    if (commentStartLine<1)
        commentStartLine = 1;
    return commentStartLine;
}

int SynEdit::calcIndentSpaces(int line, const QString& lineText, bool addIndent)
{
    if (!mHighlighter)
        return 0;
    line = std::min(line, mDocument->count()+1);
    if (line<=1)
        return 0;
    // find the first non-empty preceeding line
    int startLine = line-1;
    QString startLineText;
    while (startLine>=1) {
        startLineText = mDocument->getString(startLine-1);
        if (!startLineText.startsWith('#') && !startLineText.trimmed().isEmpty()) {
            break;
        }
        startLine -- ;
    }
    int indentSpaces = 0;
    if (startLine>=1) {
        //calculate the indents of last statement;
        indentSpaces = leftSpaces(startLineText);
        HighlighterState rangePreceeding = mDocument->ranges(startLine-1);
        mHighlighter->setState(rangePreceeding);
        if (addIndent) {
//            QString trimmedS = s.trimmed();
            QString trimmedLineText = lineText.trimmed();
            mHighlighter->setLine(trimmedLineText,line-1);
            int statePrePre;
            if (startLine>1) {
                statePrePre = mDocument->ranges(startLine-2).state;
            } else {
                statePrePre = 0;
            }
            HighlighterState rangeAfterFirstToken = mHighlighter->getState();
            QString firstToken = mHighlighter->getToken();
            PHighlighterAttribute attr = mHighlighter->getTokenAttribute();
            if (attr->tokenType() == TokenType::Keyword
                                  &&  lineText.endsWith(':')
                                  && (
                                  firstToken == "public" || firstToken == "private"
                                  || firstToken == "protected" || firstToken == "case"
                                  || firstToken == "default"
                        )) {
                // public: private: protecte: case: should indents like it's parent statement
                mHighlighter->setState(rangePreceeding);
                mHighlighter->setLine("}",line-1);
                rangeAfterFirstToken = mHighlighter->getState();
                firstToken = mHighlighter->getToken();
                attr = mHighlighter->getTokenAttribute();
            }
            bool indentAdded = false;
            int additionIndent = 0;
            QVector<int> matchingIndents;
            int l;
            if (attr->tokenType() == TokenType::Operator
                    && (firstToken == '}')) {
                // current line starts with '}', we should consider it to calc indents
                matchingIndents = rangeAfterFirstToken.matchingIndents;
                indentAdded = true;
                l = startLine;
            } else if (attr->tokenType() == TokenType::Operator
                       && (firstToken == '{')
                       && (rangePreceeding.getLastIndent()==sitStatement)) {
                // current line starts with '{' and last statement not finished, we should consider it to calc indents
                matchingIndents = rangeAfterFirstToken.matchingIndents;
                indentAdded = true;
                l = startLine;
            } else if (mHighlighter->language() == HighlighterLanguage::Cpp
                       && trimmedLineText.startsWith('#')
                       && attr == ((CppHighlighter *)mHighlighter.get())->preprocessorAttribute()) {
                indentAdded = true;
                indentSpaces=0;
                l=0;
            } else if (mHighlighter->language() == HighlighterLanguage::Cpp
                       && mHighlighter->isLastLineCommentNotFinished(rangePreceeding.state)
                       ) {
                // last line is a not finished comment,
                if  (trimmedLineText.startsWith("*")) {
                    // this line start with "* "
                    // it means this line is a docstring, should indents according to
                    // the line the comment beginning , and add 1 additional space
                    additionIndent = 1;
                    int commentStartLine = findCommentStartLine(startLine-1);
                    HighlighterState range;
                    indentSpaces = leftSpaces(mDocument->getString(commentStartLine-1));
                    range = mDocument->ranges(commentStartLine-1);
                    matchingIndents = range.matchingIndents;
                    indentAdded = true;
                    l = commentStartLine;
                } else {
                    //indents according to the beginning of the comment and 2 additional space
                    additionIndent = 0;
                    int commentStartLine = findCommentStartLine(startLine-1);
                    HighlighterState range;
                    indentSpaces = leftSpaces(mDocument->getString(commentStartLine-1))+2;
                    range = mDocument->ranges(commentStartLine-1);
                    matchingIndents = range.matchingIndents;
                    indentAdded = true;
                    l = startLine;
                }
            } else if ( mHighlighter->isLastLineCommentNotFinished(statePrePre)
                        && rangePreceeding.matchingIndents.isEmpty()
                        && rangePreceeding.firstIndentThisLine>=rangePreceeding.indents.length()
                        && !mHighlighter->isLastLineCommentNotFinished(rangePreceeding.state)) {
                // the preceeding line is the end of comment
                // we should use the indents of the start line of the comment
                int commentStartLine = findCommentStartLine(startLine-2);
                HighlighterState range;
                indentSpaces = leftSpaces(mDocument->getString(commentStartLine-1));
                range = mDocument->ranges(commentStartLine-1);
                matchingIndents = range.matchingIndents;
                indentAdded = true;
                l = commentStartLine;
            } else {
                // we just use infos till preceeding line's end to calc indents
                matchingIndents = rangePreceeding.matchingIndents;
                l = startLine-1;
            }

            if (!matchingIndents.isEmpty()
                    ) {
                // find the indent's start line, and use it's indent as the default indent;
                while (l>=1) {
                    HighlighterState range = mDocument->ranges(l-1);
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
                            indentSpaces = leftSpaces(mDocument->getString(l-1));
                            if (newIndents.length()>0)
                                indentSpaces+=tabWidth();
                            break;
                        }
                    } else {
                        matchingIndents = range.matchingIndents + matchingIndents.mid(i);
                    }
                    l--;
                }
            }
            if (!indentAdded) {
                if (rangePreceeding.firstIndentThisLine < rangePreceeding.indents.length()) {
                    indentSpaces += tabWidth();
                    indentAdded = true;
                }
            }

            if (!indentAdded && !startLineText.isEmpty()) {
                BufferCoord coord;
                QString token;
                PHighlighterAttribute attr;
                coord.line = startLine;
                coord.ch = document()->getString(startLine-1).length();
                if (getHighlighterAttriAtRowCol(coord,token,attr)
                        && attr->tokenType() == QSynedit::TokenType::Operator
                        && token == ":") {
                    indentSpaces += tabWidth();
                    indentAdded = true;
                }
            }
            indentSpaces += additionIndent;
        }
    }
    return std::max(0,indentSpaces);
}

void SynEdit::doSelectAll()
{
    BufferCoord LastPt;
    LastPt.ch = 1;
    if (mDocument->empty()) {
        LastPt.line = 1;
    } else {
        LastPt.line = mDocument->count();
        LastPt.ch = mDocument->getString(LastPt.line-1).length()+1;
    }
    setCaretAndSelection(caretXY(), BufferCoord{1, 1}, LastPt);
    // Selection should have changed...
    emit statusChanged(StatusChange::scSelection);
}

void SynEdit::doComment()
{
    BufferCoord origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    if (mReadOnly)
        return;
    mUndoList->beginBlock();
    auto action = finally([this]{
        mUndoList->endBlock();
    });
    origBlockBegin = blockBegin();
    origBlockEnd = blockEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.ch == 1)
        endLine = std::max(origBlockBegin.line - 1, origBlockEnd.line - 2);
    else
        endLine = origBlockEnd.line - 1;
    for (int i = origBlockBegin.line - 1; i<=endLine; i++) {
        mDocument->putString(i, "//" + mDocument->getString(i));
        mUndoList->addChange(ChangeReason::Insert,
              BufferCoord{1, i + 1},
              BufferCoord{3, i + 1},
              QStringList(), SelectionMode::Normal);
    }
    // When grouping similar commands, process one comment action per undo/redo
    mUndoList->addGroupBreak();
    // Move begin of selection
    if (origBlockBegin.ch > 1)
        origBlockBegin.ch+=2;
    // Move end of selection
    if (origBlockEnd.ch > 1)
        origBlockEnd.ch+=2;
    // Move caret
    if (origCaret.ch > 1)
          origCaret.ch+=2;
    setCaretAndSelection(origCaret, origBlockBegin, origBlockEnd);
}

void SynEdit::doUncomment()
{
    BufferCoord origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    QString s;
    QStringList changeText;
    changeText.append("//");
    if (mReadOnly)
        return;
    mUndoList->beginBlock();
    auto action = finally([this]{
        mUndoList->endBlock();
    });
    origBlockBegin = blockBegin();
    origBlockEnd = blockEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.ch == 1)
        endLine = std::max(origBlockBegin.line - 1, origBlockEnd.line - 2);
    else
        endLine = origBlockEnd.line - 1;
    for (int i = origBlockBegin.line - 1; i<= endLine; i++) {
        s = mDocument->getString(i);
        // Find // after blanks only
        int j = 0;
        while ((j+1 < s.length()) && (s[j] == '\n' || s[j] == '\t'))
            j++;
        if ((j + 1 < s.length()) && (s[j] == '/') && (s[j + 1] == '/')) {
            s.remove(j,2);
            mDocument->putString(i,s);
            mUndoList->addChange(ChangeReason::Delete,
                                 BufferCoord{j+1, i + 1},
                                 BufferCoord{j + 3, i + 1},
                                 changeText, SelectionMode::Normal);
            // Move begin of selection
            if ((i == origBlockBegin.line - 1) && (origBlockBegin.ch > 1))
                origBlockBegin.ch-=2;
            // Move end of selection
            if ((i == origBlockEnd.line - 1) && (origBlockEnd.ch > 1))
                origBlockEnd.ch-=2;
            // Move caret
            if ((i == origCaret.line - 1) && (origCaret.ch > 1))
                origCaret.ch-=2;
        }
    }
    // When grouping similar commands, process one uncomment action per undo/redo
    mUndoList->addGroupBreak();
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
    mUndoList->beginBlock();
    auto action = finally([this]{
        mUndoList->endBlock();
    });
    origBlockBegin = blockBegin();
    origBlockEnd = blockEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.ch == 1)
        endLine = std::max(origBlockBegin.line - 1, origBlockEnd.line - 2);
    else
        endLine = origBlockEnd.line - 1;
    for (int i = origBlockBegin.line - 1; i<= endLine; i++) {
        s = mDocument->getString(i);
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

void SynEdit::doToggleBlockComment()
{
    QString s;
    if (mReadOnly)
        return;

    QString text=selText().trimmed();
    if (text.length()>4 && text.startsWith("/*") && text.endsWith("*/")) {
        QString newText=selText();
        int pos = newText.indexOf("/*");
        if (pos>=0) {
            newText.remove(pos,2);
        }
        pos = newText.lastIndexOf("*/");
        if (pos>=0) {
            newText.remove(pos,2);
        }
        setSelText(newText);
    } else {
        QString newText="/*"+selText()+"*/";
        setSelText(newText);
    }

}

void SynEdit::doMouseScroll(bool isDragging)
{
    if (mDropped) {
        mDropped=false;
        return;
    }
    if (!hasFocus())
        return;
    Qt::MouseButtons buttons = qApp->mouseButtons();
    if (!buttons.testFlag(Qt::LeftButton))
        return;
    QPoint iMousePos;
    DisplayCoord C;
    int X, Y;

    iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    C = pixelsToNearestRowColumn(iMousePos.x(), iMousePos.y());
    C.Row = minMax(C.Row, 1, displayLineCount());
    if (mScrollDeltaX != 0) {
        setLeftChar(leftChar() + mScrollDeltaX * mMouseSelectionScrollSpeed);
        X = leftChar();
        if (mScrollDeltaX > 0) // scrolling right?
            X+=charsInWindow();
        C.Column = X;
    }
    if (mScrollDeltaY != 0) {
        //qDebug()<<mScrollDeltaY;
        if (QApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier))
          setTopLine(mTopLine + mScrollDeltaY * mLinesInWindow);
        else
          setTopLine(mTopLine + mScrollDeltaY * mMouseSelectionScrollSpeed);
        Y = mTopLine;
        if (mScrollDeltaY > 0)  // scrolling down?
            Y+=mLinesInWindow - 1;
        C.Row = minMax(Y, 1, displayLineCount());
    }
    BufferCoord vCaret = displayToBufferPos(C);
    if ((caretX() != vCaret.ch) || (caretY() != vCaret.line)) {
        // changes to line / column in one go
        incPaintLock();
        auto action = finally([this]{
            decPaintLock();
        });
        internalSetCaretXY(vCaret);

        // if MouseCapture is True we're changing selection. otherwise we're dragging
        if (isDragging) {
            setBlockBegin(mDragSelBeginSave);
            setBlockEnd(mDragSelEndSave);
        } else
            setBlockEnd(caretXY());

        if (mOptions.testFlag(EditorOption::eoGroupUndo))
            mUndoList->addGroupBreak();
    }
    if (isDragging) {
        mScrollTimer->singleShot(20,this,&SynEdit::onDraggingScrollTimeout);
    } else  {
        mScrollTimer->singleShot(20,this,&SynEdit::onScrollTimeout);
    }

//    computeScroll(isDragging);
}

QString SynEdit::getDisplayStringAtLine(int line) const
{
    QString s = mDocument->getString(line-1);
    PCodeFoldingRange foldRange = foldStartAtLine(line);
    if ((foldRange) && foldRange->collapsed) {
        return s+highlighter()->foldString();
    }
    return s;
}

void SynEdit::doDeleteLastChar()
{
    if (mReadOnly)
        return ;
    auto action = finally([this]{
        ensureCursorPosVisible();
    });

    if (mActiveSelectionMode==SelectionMode::Column) {
        BufferCoord start=blockBegin();
        BufferCoord end=blockEnd();
        if (!selAvail()) {
            start.ch--;
            setBlockBegin(start);
            setBlockEnd(end);
        }
        setSelectedTextEmpty();
        return;
    }
    if (selAvail()) {
        setSelectedTextEmpty();
        return;
    }
    bool shouldAddGroupBreak=false;
    QString Temp = lineText();
    int Len = Temp.length();
    BufferCoord Caret = caretXY();
    QStringList helper;
    if (mCaretX > Len + 1) {
        // only move caret one column
        return;
    } else if (mCaretX == 1) {
        // join this line with the last line if possible
        if (mCaretY > 1) {
            internalSetCaretY(mCaretY - 1);
            internalSetCaretX(mDocument->getString(mCaretY - 1).length() + 1);
            mDocument->deleteAt(mCaretY);
            doLinesDeleted(mCaretY+1, 1);
            if (mOptions.testFlag(eoTrimTrailingSpaces))
                Temp = trimRight(Temp);
            setLineText(lineText() + Temp);
            helper.append("");
            helper.append("");
            shouldAddGroupBreak=true;
        }
    } else {
        // delete text before the caret
        int caretColumn = charToColumn(mCaretY,mCaretX);
        int SpaceCount1 = leftSpaces(Temp);
        int SpaceCount2 = 0;
        int newCaretX;

        if (SpaceCount1 == caretColumn - 1) {
                //how much till the next tab column
                int BackCounter = (caretColumn - 1) % tabWidth();
                if (BackCounter == 0)
                    BackCounter = tabWidth();
                SpaceCount2 = std::max(0,SpaceCount1 - tabWidth());
                newCaretX = columnToChar(mCaretY,SpaceCount2+1);
                helper.append(Temp.mid(newCaretX - 1, mCaretX - newCaretX));
                Temp.remove(newCaretX-1,mCaretX - newCaretX);
            properSetLine(mCaretY - 1, Temp);
            internalSetCaretX(newCaretX);
        } else {
            // delete char
            internalSetCaretX(mCaretX - 1);
            QChar ch=Temp[mCaretX-1];
            if (ch==' ' || ch=='\t')
                shouldAddGroupBreak=true;
            helper.append(QString(ch));
            Temp.remove(mCaretX-1,1);
            properSetLine(mCaretY - 1, Temp);
        }
    }
    if ((Caret.ch != mCaretX) || (Caret.line != mCaretY)) {
        mUndoList->addChange(ChangeReason::Delete, caretXY(), Caret, helper,
                        mActiveSelectionMode);
        if (shouldAddGroupBreak)
            mUndoList->addGroupBreak();
    }
}

void SynEdit::doDeleteCurrentChar()
{
    QStringList helper;
    BufferCoord Caret;
    if (mReadOnly) {
        return;
    }
    auto action = finally([this]{
        ensureCursorPosVisible();
    });

    if (mActiveSelectionMode==SelectionMode::Column) {
        BufferCoord start=blockBegin();
        BufferCoord end=blockEnd();
        if (!selAvail()) {
            end.ch++;
            setBlockBegin(start);
            setBlockEnd(end);
        }
        setSelectedTextEmpty();
        return;
    }
    if (selAvail())
        setSelectedTextEmpty();
    else {
        bool shouldAddGroupBreak=false;
        // Call UpdateLastCaretX. Even though the caret doesn't move, the
        // current caret position should "stick" whenever text is modified.
        updateLastCaretX();
        QString Temp = lineText();
        int Len = Temp.length();
        if (mCaretX>Len+1) {
            return;
        } else if (mCaretX <= Len) {
            QChar ch = Temp[mCaretX-1];
            if (ch==' ' || ch=='\t')
                shouldAddGroupBreak=true;
            // delete char
            helper.append(QString(ch));
            Caret.ch = mCaretX + 1;
            Caret.line = mCaretY;
            Temp.remove(mCaretX-1, 1);
            properSetLine(mCaretY - 1, Temp);
        } else {
            // join line with the line after
            if (mCaretY < mDocument->count()) {
                shouldAddGroupBreak=true;
                properSetLine(mCaretY - 1, Temp + mDocument->getString(mCaretY));
                Caret.ch = 1;
                Caret.line = mCaretY + 1;
                helper.append("");
                helper.append("");
                mDocument->deleteAt(mCaretY);
                if (mCaretX==1)
                    doLinesDeleted(mCaretY, 1);
                else
                    doLinesDeleted(mCaretY + 1, 1);
            }
        }
        if ((Caret.ch != mCaretX) || (Caret.line != mCaretY)) {
            mUndoList->addChange(ChangeReason::Delete, caretXY(), Caret,
                  helper, mActiveSelectionMode);
            if (shouldAddGroupBreak)
                mUndoList->addGroupBreak();
        }
    }
}

void SynEdit::doDeleteWord()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    BufferCoord start = wordStart();
    BufferCoord end = wordEnd();
    deleteFromTo(start,end);
}

void SynEdit::doDeleteToEOL()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    deleteFromTo(caretXY(),BufferCoord{lineText().length()+1,mCaretY});
}

void SynEdit::doDeleteToWordStart()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    BufferCoord start = wordStart();
    BufferCoord end = caretXY();
    if (start==end) {
        start = prevWordPos();
    }
    deleteFromTo(start,end);
}

void SynEdit::doDeleteToWordEnd()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    BufferCoord start = caretXY();
    BufferCoord end = wordEnd();
    if (start == end) {
        end = wordEndEx(nextWordPos());
    }
    deleteFromTo(start,end);
}

void SynEdit::doDeleteFromBOL()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    deleteFromTo(BufferCoord{1,mCaretY},caretXY());
}

void SynEdit::doDeleteLine()
{
    if (!mReadOnly && (mDocument->count() > 0)) {
        PCodeFoldingRange foldRange=foldStartAtLine(mCaretY);
        if (foldRange && foldRange->collapsed)
            return;
        mUndoList->beginBlock();
        addCaretToUndo();
        addSelectionToUndo();
        if (selAvail())
            setBlockBegin(caretXY());
        QStringList helper(lineText());
        if (mCaretY == mDocument->count()) {
            if (mDocument->count()==1) {
                mDocument->putString(mCaretY - 1,"");
                mUndoList->addChange(ChangeReason::Delete,
                                     BufferCoord{1, mCaretY},
                                     BufferCoord{helper.length() + 1, mCaretY},
                                     helper, SelectionMode::Normal);
            } else {
                QString s = mDocument->getString(mCaretY-2);
                mDocument->deleteAt(mCaretY - 1);
                helper.insert(0,"");
                mUndoList->addChange(ChangeReason::Delete,
                                     BufferCoord{s.length()+1, mCaretY-1},
                                     BufferCoord{helper.length() + 1, mCaretY},
                                     helper, SelectionMode::Normal);
                doLinesDeleted(mCaretY, 1);
                mCaretY--;
            }
        } else {
            mDocument->deleteAt(mCaretY - 1);
            helper.append("");
            mUndoList->addChange(ChangeReason::Delete,
                                 BufferCoord{1, mCaretY},
                                 BufferCoord{helper.length() + 1, mCaretY},
                                 helper, SelectionMode::Normal);
            doLinesDeleted(mCaretY, 1);
        }
        mUndoList->endBlock();
        internalSetCaretXY(BufferCoord{1, mCaretY}); // like seen in the Delphi editor
    }
}

void SynEdit::doSelecteLine()
{
    setBlockBegin(BufferCoord{1,mCaretY});
    if (mCaretY==mDocument->count())
        setBlockEnd(BufferCoord{lineText().length()+1,mCaretY});
    else
        setBlockEnd(BufferCoord{1,mCaretY+1});
}

void SynEdit::doDuplicateLine()
{
    if (!mReadOnly && (mDocument->count() > 0)) {
        PCodeFoldingRange foldRange=foldStartAtLine(mCaretY);
        if (foldRange && foldRange->collapsed)
            return;
        QString s = lineText();
        mDocument->insert(mCaretY, lineText());
        doLinesInserted(mCaretY + 1, 1);
        mUndoList->beginBlock();
        addCaretToUndo();
        mUndoList->addChange(ChangeReason::LineBreak,
                             BufferCoord{s.length()+1,mCaretY},
                             BufferCoord{s.length()+1,mCaretY}, QStringList(), SelectionMode::Normal);
        mUndoList->addChange(ChangeReason::Insert,
                             BufferCoord{1,mCaretY+1},
                             BufferCoord{s.length()+1,mCaretY+1}, QStringList(), SelectionMode::Normal);
        mUndoList->endBlock();
        internalSetCaretXY(BufferCoord{1, mCaretY}); // like seen in the Delphi editor
    }
}

void SynEdit::doMoveSelUp()
{
    if (mActiveSelectionMode == SelectionMode::Column)
        return;
    if (!mReadOnly && (mDocument->count() > 0) && (blockBegin().line > 1)) {
        if (!mUndoing) {
            mUndoList->beginBlock();
            addCaretToUndo();
            addSelectionToUndo();
        }
        BufferCoord origBlockBegin = blockBegin();
        BufferCoord origBlockEnd = blockEnd();
        PCodeFoldingRange foldRange=foldStartAtLine(origBlockEnd.line);
        if (foldRange && foldRange->collapsed)
            return;
//        for (int line=origBlockBegin.Line;line<=origBlockEnd.Line;line++) {
//            PSynEditFoldRange foldRange=foldStartAtLine(line);
//            if (foldRange && foldRange->collapsed)
//                return;
//        }

        // Delete line above selection
        QString s = mDocument->getString(origBlockBegin.line - 2); // before start, 0 based
        mDocument->deleteAt(origBlockBegin.line - 2); // before start, 0 based
        doLinesDeleted(origBlockBegin.line - 1, 1); // before start, 1 based

        // Insert line below selection
        mDocument->insert(origBlockEnd.line - 1, s);
        doLinesInserted(origBlockEnd.line, 1);
        // Restore caret and selection
        setCaretAndSelection(
                  BufferCoord{mCaretX, origBlockBegin.line - 1},
                  BufferCoord{origBlockBegin.ch, origBlockBegin.line - 1},
                  BufferCoord{origBlockEnd.ch, origBlockEnd.line - 1}
        );
        if (!mUndoing) {
            mUndoList->addChange(ChangeReason::MoveSelectionUp,
                    origBlockBegin,
                    origBlockEnd,
                    QStringList(),
                    SelectionMode::Normal);
            mUndoList->endBlock();
        }
    }
}

void SynEdit::doMoveSelDown()
{
    if (mActiveSelectionMode == SelectionMode::Column)
        return;
    if (!mReadOnly && (mDocument->count() > 0) && (blockEnd().line < mDocument->count())) {
        if (!mUndoing) {
            mUndoList->beginBlock();
            addCaretToUndo();
            addSelectionToUndo();
        }
        BufferCoord origBlockBegin = blockBegin();
        BufferCoord origBlockEnd = blockEnd();

        PCodeFoldingRange foldRange=foldStartAtLine(origBlockEnd.line);
        if (foldRange && foldRange->collapsed)
            return;

        // Delete line below selection
        QString s = mDocument->getString(origBlockEnd.line); // after end, 0 based
        mDocument->deleteAt(origBlockEnd.line); // after end, 0 based
        doLinesDeleted(origBlockEnd.line, 1); // before start, 1 based

        // Insert line above selection
        mDocument->insert(origBlockBegin.line - 1, s);
        doLinesInserted(origBlockBegin.line, 1);

        // Restore caret and selection
        setCaretAndSelection(
                  BufferCoord{mCaretX, origBlockEnd.line + 1},
                  BufferCoord{origBlockBegin.ch, origBlockBegin.line + 1},
                  BufferCoord{origBlockEnd.ch, origBlockEnd.line + 1}
                    );

        if (!mUndoing) {
            mUndoList->addChange(ChangeReason::MoveSelectionDown,
                    origBlockBegin,
                    origBlockEnd,
                    QStringList(),
                    SelectionMode::Normal);
            mUndoList->endBlock();
        }

    }
}

void SynEdit::clearAll()
{
    mDocument->clear();
    mUndoList->clear();
    mRedoList->clear();
    setModified(false);
}

void SynEdit::insertLine(bool moveCaret)
{
    if (mReadOnly)
        return;
    int nLinesInserted=0;
    if (!mUndoing)
        mUndoList->beginBlock();
    auto action = finally([this] {
        if (!mUndoing)
            mUndoList->endBlock();
    });
    QString helper;
    if (selAvail()) {
        helper = selText();
        setSelectedTextEmpty();
    }

    QString Temp = lineText();

    if (mCaretX>lineText().length()+1) {
        PCodeFoldingRange foldRange = foldStartAtLine(mCaretY);
        if ((foldRange) && foldRange->collapsed) {
            QString s = Temp+highlighter()->foldString();
            if (mCaretX > s.length()) {
                if (!mUndoing) {
                    addCaretToUndo();
                    addSelectionToUndo();
                }
                mCaretY=foldRange->toLine;
                if (mCaretY>mDocument->count()) {
                    mCaretY=mDocument->count();
                }
                Temp = lineText();
                mCaretX=Temp.length()+1;
            }
        }
    }

    QString Temp2 = Temp;
    QString Temp3;
    PHighlighterAttribute Attr;

    // This is sloppy, but the Right Thing would be to track the column of markers
    // too, so they could be moved depending on whether they are after the caret...
    int InsDelta = (mCaretX == 1)?1:0;
    QString leftLineText = lineText().mid(0, mCaretX - 1);
    QString rightLineText = lineText().mid(mCaretX-1);
    if (!mUndoing)
        mUndoList->addChange(ChangeReason::LineBreak, caretXY(), caretXY(), QStringList(rightLineText),
              SelectionMode::Normal);
    bool notInComment=true;
    properSetLine(mCaretY-1,leftLineText);
    //update range stated for line mCaretY
    if (mHighlighter) {
        if (mCaretY==1) {
            mHighlighter->resetState();
        } else {
            mHighlighter->setState(mDocument->ranges(mCaretY-2));
        }
        mHighlighter->setLine(leftLineText, mCaretY-1);
        mHighlighter->nextToEol();
        mDocument->setRange(mCaretY-1,mHighlighter->getState());
        notInComment = !mHighlighter->isLastLineCommentNotFinished(
                    mHighlighter->getState().state)
                && !mHighlighter->isLastLineStringNotFinished(
                    mHighlighter->getState().state);
    }
    int indentSpaces = 0;
    if (mOptions.testFlag(eoAutoIndent)) {
        rightLineText=trimLeft(rightLineText);
        indentSpaces = calcIndentSpaces(mCaretY+1,
                                        rightLineText,mOptions.testFlag(eoAutoIndent)
                                            );
    }
    QString indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
    mDocument->insert(mCaretY, indentSpacesForRightLineText+rightLineText);
    nLinesInserted++;

    if (!mUndoing) {
        //insert new line in middle of "/*" and "*/"
        if (!notInComment &&
                ( leftLineText.endsWith("/*") && rightLineText.startsWith("*/")
                 )) {
            indentSpaces = calcIndentSpaces(mCaretY+1, "" , mOptions.testFlag(eoAutoIndent));
            indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
            mDocument->insert(mCaretY, indentSpacesForRightLineText);
            nLinesInserted++;
            mUndoList->addChange(ChangeReason::LineBreak, caretXY(), caretXY(), QStringList(),
                    SelectionMode::Normal);
        }
        //insert new line in middle of "{" and "}"
        if (notInComment &&
                ( leftLineText.endsWith('{') && rightLineText.startsWith('}')
                 )) {
            indentSpaces = calcIndentSpaces(mCaretY+1, "" , mOptions.testFlag(eoAutoIndent)
                                                                   && notInComment);
            indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
            mDocument->insert(mCaretY, indentSpacesForRightLineText);
            nLinesInserted++;
            mUndoList->addChange(ChangeReason::LineBreak, caretXY(), caretXY(), QStringList(),
                    SelectionMode::Normal);
        }
    }
    if (moveCaret)
        internalSetCaretXY(BufferCoord{indentSpacesForRightLineText.length()+1,mCaretY + 1});

    doLinesInserted(mCaretY - InsDelta, nLinesInserted);
    setBlockBegin(caretXY());
    setBlockEnd(caretXY());
    ensureCursorPosVisible();
    updateLastCaretX();
}

void SynEdit::doTabKey()
{
    if (mActiveSelectionMode == SelectionMode::Column) {
        doAddChar('\t');
        return;
    }
    // Provide Visual Studio like block indenting
    if (mOptions.testFlag(eoTabIndent) && canDoBlockIndent()) {
        doBlockIndent();
        return;
    }
    mUndoList->beginBlock();
    if (selAvail()) {
        setSelectedTextEmpty();
    }
    QString Spaces;
    if (mOptions.testFlag(eoTabsToSpaces)) {
        int cols = charToColumn(mCaretY,mCaretX);
        int i = tabWidth() - (cols) % tabWidth();
        Spaces = QString(i,' ');
    } else {
        Spaces = '\t';
    }
    setSelTextPrimitive(QStringList(Spaces));
    mUndoList->endBlock();
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
    if (mCaretY > mDocument->count() || mCaretX > lineText().length()+1)
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
        int spacesToRemove = colsBefore % tabWidth();
        if (spacesToRemove == 0)
            spacesToRemove = tabWidth();
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
        doDeleteText(BufferCoord{NewX, mCaretY},caretXY(),mActiveSelectionMode);
        internalSetCaretX(NewX);
    }
}


bool SynEdit::canDoBlockIndent()
{
    BufferCoord BB;
    BufferCoord BE;

    if (selAvail()) {
//        BB = blockBegin();
//        BE = blockEnd();
        return true;
    } else {
        BB = caretXY();
        BE = caretXY();
    }


    if (BB.line > mDocument->count() || BE.line > mDocument->count()) {
        return false;
    }

    if (mActiveSelectionMode == SelectionMode::Normal) {
        QString s = mDocument->getString(BB.line-1).mid(0,BB.ch-1);
        if (!s.trimmed().isEmpty())
            return false;
        if (BE.ch>1) {
            QString s1=mDocument->getString(BE.line-1).mid(BE.ch-1);
            QString s2=mDocument->getString(BE.line-1).mid(0,BE.ch-1);
            if (!s1.trimmed().isEmpty() && !s2.trimmed().isEmpty())
                return false;
        }
    }
    if (mActiveSelectionMode == SelectionMode::Column) {
        int startCol = charToColumn(BB.line,BB.ch);
        int endCol = charToColumn(BE.line,BE.ch);
        for (int i = BB.line; i<=BE.line;i++) {
            QString line = mDocument->getString(i-1);
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

QRect SynEdit::calculateCaretRect() const
{
    DisplayCoord coord = displayXY();
    if (!mInputPreeditString.isEmpty()) {
        QString sLine = lineText().left(mCaretX-1)
                + mInputPreeditString
                + lineText().mid(mCaretX-1);
        coord.Column = charToColumn(sLine,mCaretX+mInputPreeditString.length());
    }
    int rows=1;
    if (mActiveSelectionMode == SelectionMode::Column) {
        int startRow = lineToRow(std::min(blockBegin().line, blockEnd().line));
        int endRow = lineToRow(std::max(blockBegin().line, blockEnd().line));
        coord.Row = startRow;
        rows = endRow-startRow+1;
    }
    QPoint caretPos = rowColumnToPixels(coord);
    int caretWidth=mCharWidth;
    if (mCaretY <= mDocument->count() && mCaretX <= mDocument->getString(mCaretY-1).length()) {
        caretWidth = charColumns(getDisplayStringAtLine(mCaretY)[mCaretX-1])*mCharWidth;
    }
    if (mActiveSelectionMode == SelectionMode::Column) {
        return QRect(caretPos.x(),caretPos.y(),caretWidth,
                     mTextHeight*(rows));
    } else {
        return QRect(caretPos.x(),caretPos.y(),caretWidth,
                     mTextHeight);
    }
}

QRect SynEdit::calculateInputCaretRect() const
{
    DisplayCoord coord = displayXY();
    QPoint caretPos = rowColumnToPixels(coord);
    int caretWidth=mCharWidth;
    if (mCaretY <= mDocument->count() && mCaretX <= mDocument->getString(mCaretY-1).length()) {
        caretWidth = charColumns(mDocument->getString(mCaretY-1)[mCaretX-1])*mCharWidth;
    }
    return QRect(caretPos.x(),caretPos.y(),caretWidth,
                 mTextHeight);
}

void SynEdit::clearAreaList(EditingAreaList areaList)
{
    areaList.clear();
}

void SynEdit::computeCaret()
{
    QPoint iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    int X=iMousePos.x();
    int Y=iMousePos.y();

    DisplayCoord vCaretNearestPos = pixelsToNearestRowColumn(X, Y);
    vCaretNearestPos.Row = minMax(vCaretNearestPos.Row, 1, displayLineCount());
    setInternalDisplayXY(vCaretNearestPos);
}

void SynEdit::computeScroll(bool isDragging)
{
    QPoint iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    int X=iMousePos.x();
    int Y=iMousePos.y();

    QRect iScrollBounds; // relative to the client area
    int dispX=2,dispY = 2;
//    if (isDragging) {
//        dispX = mCharWidth / 2 -1;
//        dispY = mTextHeight/ 2 -1;
//    }
    int left = mGutterWidth+frameWidth()+dispX;
    int top = frameWidth()+dispY;
    iScrollBounds = QRect(left,
                          top,
                          clientWidth()-left-dispX,
                          clientHeight()-top-dispY);

    if (X < iScrollBounds.left())
        mScrollDeltaX = (X - iScrollBounds.left()) / mCharWidth - 1;
    else if (X >= iScrollBounds.right())
        mScrollDeltaX = (X - iScrollBounds.right()) / mCharWidth + 1;
    else
        mScrollDeltaX = 0;

//    if (isDragging && (X<0 || X>clientRect().width())) {
//        mScrollDeltaX = 0;
//    }

    if (Y < iScrollBounds.top())
        mScrollDeltaY = (Y - iScrollBounds.top()) / mTextHeight - 1;
    else if (Y >= iScrollBounds.bottom())
        mScrollDeltaY = (Y - iScrollBounds.bottom()) / mTextHeight + 1;
    else
        mScrollDeltaY = 0;

//    if (isDragging && (Y<0 || Y>clientRect().height())) {
//        mScrollDeltaY = 0;
//    }


//    if (mScrollDeltaX!=0 || mScrollDeltaY!=0) {
    doMouseScroll(isDragging);
//    }
}

void SynEdit::doBlockIndent()
{
    BufferCoord  oldCaretPos;
    BufferCoord  BB, BE;
    QStringList strToInsert;
    int e,x,i;
    QString spaces;

    oldCaretPos = caretXY();

    // keep current selection detail
    if (selAvail()) {
        BB = blockBegin();
        BE = blockEnd();
    } else {
        BB = caretXY();
        BE = caretXY();
    }
    // build text to insert
    if (BE.ch == 1 && BE.line != BB.line) {
        e = BE.line - 1;
        x = 1;
    } else {
        e = BE.line;
        if (mOptions.testFlag(EditorOption::eoTabsToSpaces))
          x = caretX() + tabWidth();
        else
          x = caretX() + 1;
    }
    if (mOptions.testFlag(eoTabsToSpaces)) {
        spaces = QString(tabWidth(),' ') ;
    } else {
        spaces = "\t";
    }
//    for (i = BB.line; i<e;i++) {
//        strToInsert.append(spaces);
//    }
//    strToInsert.append(spaces);
    mUndoList->beginBlock();
    mUndoList->addChange(ChangeReason::Caret, oldCaretPos, oldCaretPos,QStringList(), activeSelectionMode());
    mUndoList->addChange(ChangeReason::Selection,mBlockBegin,mBlockEnd,QStringList(), activeSelectionMode());
    int ch;
    if (mActiveSelectionMode == SelectionMode::Column)
      ch = std::min(BB.ch, BE.ch);
    else
      ch = 1;
    for (i = BB.line; i<=e;i++) {
        if (i>mDocument->count())
            break;
        QString line=mDocument->getString(i-1);
        if (ch>line.length()) {
            mUndoList->addChange(
                        ChangeReason::Insert,
                        BufferCoord{line.length(), i},
                        BufferCoord{line.length()+spaces.length(), i},
                        QStringList(),
                        SelectionMode::Normal);
            line+=spaces;
        } else {
            mUndoList->addChange(
                        ChangeReason::Insert,
                        BufferCoord{ch, i},
                        BufferCoord{ch+spaces.length(), i},
                        QStringList(),
                        SelectionMode::Normal);
            line = line.left(ch-1)+spaces+line.mid(ch-1);
        }
        properSetLine(i-1,line);
    }
    //adjust caret and selection
    oldCaretPos.ch = x;
    if (BB.ch > 1)
        BB.ch += spaces.length();
    if (BE.ch > 1)
      BE.ch+=spaces.length();
    setCaretAndSelection(oldCaretPos,
      BB, BE);
    mUndoList->endBlock();
}

void SynEdit::doBlockUnindent()
{
    int lastIndent = 0;
    int firstIndent = 0;

    BufferCoord BB,BE;
    // keep current selection detail
    if (selAvail()) {
        BB = blockBegin();
        BE = blockEnd();
    } else {
        BB = caretXY();
        BE = caretXY();
    }
    BufferCoord oldCaretPos = caretXY();
    int x = 0;
    mUndoList->beginBlock();
    mUndoList->addChange(ChangeReason::Caret, oldCaretPos, oldCaretPos,QStringList(), activeSelectionMode());
    mUndoList->addChange(ChangeReason::Selection,mBlockBegin,mBlockEnd,QStringList(), activeSelectionMode());

    int e = BE.line;
    // convert selection to complete lines
    if (BE.ch == 1)
        e = BE.line - 1;
    // build string to delete
    for (int i = BB.line; i<= e;i++) {
        QString line = mDocument->getString(i - 1);
        if (line.isEmpty())
            continue;
        if (line[0]!=' ' && line[0]!='\t')
            continue;
        int charsToDelete = 0;
        while (charsToDelete < tabWidth() &&
               charsToDelete < line.length() &&
               line[charsToDelete] == ' ')
            charsToDelete++;
        if (charsToDelete == 0)
            charsToDelete = 1;
        if (i==BB.line)
            firstIndent = charsToDelete;
        if (i==e)
            lastIndent = charsToDelete;
        if (i==oldCaretPos.line)
            x = charsToDelete;
        QString tempString = line.mid(charsToDelete);
        mDocument->putString(i-1,tempString);
        mUndoList->addChange(ChangeReason::Delete,
                             BufferCoord{1,i},
                             BufferCoord{charsToDelete+1,i},
                             QStringList(line.left(charsToDelete)),
                             SelectionMode::Normal);
    }
  // restore selection
  //adjust the x position of orgcaretpos appropriately

    oldCaretPos.ch -= x;
    BB.ch -= firstIndent;
    BE.ch -= lastIndent;
    setCaretAndSelection(oldCaretPos, BB, BE);
    mUndoList->endBlock();
}

void SynEdit::doAddChar(QChar AChar)
{
    if (mReadOnly)
        return;
    if (!AChar.isPrint() && AChar!='\t')
        return;
    //DoOnPaintTransient(ttBefore);
    //mCaretX will change after setSelLength;
    if (mInserting == false && !selAvail()) {
        switch(mActiveSelectionMode) {
        case SelectionMode::Column: {
            //we can't use blockBegin()/blockEnd()
            BufferCoord start=mBlockBegin;
            BufferCoord end=mBlockEnd;
            if (start.line > end.line )
                std::swap(start,end);
            start.ch++; // make sure we select a whole char in the start line
            setBlockBegin(start);
            setBlockEnd(end);
        }
            break;
        case SelectionMode::Line:
            //do nothing;
            break;
        default:
            setSelLength(1);
        }
    }

    if (isIdentChar(AChar)) {
        doSetSelText(AChar);
    } else if (AChar.isSpace()) {
        // break group undo chain
        mUndoList->addGroupBreak();
        doSetSelText(AChar);
        // break group undo chain
//        if (mActiveSelectionMode!=SynSelectionMode::smColumn)
//            mUndoList->AddChange(SynChangeReason::crNothing,
//                                 BufferCoord{0, 0},
//                                 BufferCoord{0, 0},
//                                 "", SynSelectionMode::smNormal);
    } else {
        mUndoList->beginBlock();
        doSetSelText(AChar);
        int oldCaretX=mCaretX-1;
        int oldCaretY=mCaretY;
        // auto
        if (mActiveSelectionMode==SelectionMode::Normal
                && mOptions.testFlag(eoAutoIndent)
                && mHighlighter
                && mHighlighter->language() == HighlighterLanguage::Cpp
                && (oldCaretY<=mDocument->count()) ) {

            //unindent if ':' at end of the line
            if (AChar == ':') {
                QString line = mDocument->getString(oldCaretY-1);
                if (line.length() <= oldCaretX) {
                    int indentSpaces = calcIndentSpaces(oldCaretY,line+":", true);
                    if (indentSpaces != leftSpaces(line)) {
                        QString newLine = GetLeftSpacing(indentSpaces,true) + trimLeft(line);
                        mDocument->putString(oldCaretY-1,newLine);
                        internalSetCaretXY(BufferCoord{newLine.length()+2,oldCaretY});
                        setBlockBegin(caretXY());
                        setBlockEnd(caretXY());
                        mUndoList->addChange(
                                    ChangeReason::Delete,
                                    BufferCoord{1, oldCaretY},
                                    BufferCoord{line.length()+1, oldCaretY},
                                    QStringList(line),
                                    SelectionMode::Normal
                                    );
                        mUndoList->addChange(
                                    ChangeReason::Insert,
                                    BufferCoord{1, oldCaretY},
                                    BufferCoord{newLine.length()+1, oldCaretY},
                                    QStringList(),
                                    SelectionMode::Normal
                                    );
                    }
                }
            } else if (AChar == '*') {
                QString line = mDocument->getString(oldCaretY-1);
                if (line.length() <= oldCaretX) {
                    int indentSpaces = calcIndentSpaces(oldCaretY,line+"*", true);
                    if (indentSpaces != leftSpaces(line)) {
                        QString newLine = GetLeftSpacing(indentSpaces,true) + trimLeft(line);
                        mDocument->putString(oldCaretY-1,newLine);
                        internalSetCaretXY(BufferCoord{newLine.length()+2,oldCaretY});
                        setBlockBegin(caretXY());
                        setBlockEnd(caretXY());
                        mUndoList->addChange(
                                    ChangeReason::Delete,
                                    BufferCoord{1, oldCaretY},
                                    BufferCoord{line.length()+1, oldCaretY},
                                    QStringList(line),
                                    SelectionMode::Normal
                                    );
                        mUndoList->addChange(
                                    ChangeReason::Insert,
                                    BufferCoord{1, oldCaretY},
                                    BufferCoord{newLine.length()+1, oldCaretY},
                                    QStringList(),
                                    SelectionMode::Normal
                                    );
                    }
                }
            } else if (AChar == '{' || AChar == '}' || AChar == '#') {
                //Reindent line when add '{' '}' and '#' at the beginning
                QString left = mDocument->getString(oldCaretY-1).mid(0,oldCaretX-1);
                // and the first nonblank char is this new {
                if (left.trimmed().isEmpty()) {
                    int indentSpaces = calcIndentSpaces(oldCaretY,AChar, true);
                    if (indentSpaces != leftSpaces(left)) {
                        QString right = mDocument->getString(oldCaretY-1).mid(oldCaretX-1);
                        QString newLeft = GetLeftSpacing(indentSpaces,true);
                        mDocument->putString(oldCaretY-1,newLeft+right);
                        BufferCoord newCaretPos =  BufferCoord{newLeft.length()+2,oldCaretY};
                        internalSetCaretXY(newCaretPos);
                        setBlockBegin(caretXY());
                        setBlockEnd(caretXY());
                        mUndoList->addChange(
                                    ChangeReason::Delete,
                                    BufferCoord{1, oldCaretY},
                                    BufferCoord{left.length()+1, oldCaretY},
                                    QStringList(left),
                                    SelectionMode::Normal
                                    );
                        mUndoList->addChange(
                                    ChangeReason::Insert,
                                    BufferCoord{1, oldCaretY},
                                    BufferCoord{newLeft.length()+1, oldCaretY},
                                    QStringList(""),
                                    SelectionMode::Normal
                                    );

                    }
                }
            }
        }
        mUndoList->endBlock();
    }
    //DoOnPaintTransient(ttAfter);
}

void SynEdit::doCutToClipboard()
{
    if (mReadOnly)
        return;
    mUndoList->beginBlock();
    addCaretToUndo();
    addSelectionToUndo();
    if (!selAvail()) {
        doSelecteLine();
    }
    internalDoCopyToClipboard(selText());
    setSelectedTextEmpty();
    mUndoList->endBlock();
    mUndoList->addGroupBreak();
}

void SynEdit::doCopyToClipboard()
{
    bool selected=selAvail();
    if (!selected)
        doSelecteLine();
    bool ChangeTrim = (mActiveSelectionMode == SelectionMode::Column) &&
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
    if (!selected) {
        setBlockBegin(caretXY());
        setBlockEnd(caretXY());
    }
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
    QString text = clipboard->text();
    if (text.isEmpty())
        return;
    //correctly handle spaces copied from wechat
//    text.replace(QChar(0x00A0),QChar(0x0020));
    mUndoList->beginBlock();
//    if (selAvail()) {
//        mUndoList->AddChange(
//                    SynChangeReason::crDelete,
//                    mBlockBegin,
//                    mBlockEnd,
//                    selText(),
//                    mActiveSelectionMode);
//    }
//        } else if (!colSelAvail())
//            setActiveSelectionMode(selectionMode());
    BufferCoord vStartOfBlock = blockBegin();
    BufferCoord vEndOfBlock = blockEnd();
    mBlockBegin = vStartOfBlock;
    mBlockEnd = vEndOfBlock;
//    qDebug()<<textToLines(text);
    setSelTextPrimitive(splitStrings(text));
    mUndoList->endBlock();
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
        if (mStateFlags.testFlag(StateFlag::sfScrollbarChanged)) {
            updateScrollbars();
            ensureCursorPosVisible();
        }
        if (mStateFlags.testFlag(StateFlag::sfCaretChanged))
            updateCaret();
        if (mStatusChanges!=0)
            doOnStatusChange(mStatusChanges);
        onEndFirstPaintLock();
    }
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


void SynEdit::updateLastCaretX()
{
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

void SynEdit::scrollWindow(int dx, int dy)
{
    int nx = horizontalScrollBar()->value()+dx;
    int ny = verticalScrollBar()->value()+dy;
    horizontalScrollBar()->setValue(nx);
    verticalScrollBar()->setValue(ny);
}

void SynEdit::setInternalDisplayXY(const DisplayCoord &aPos)
{
    incPaintLock();
    internalSetCaretXY(displayToBufferPos(aPos));
    decPaintLock();
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

void SynEdit::setStatusChanged(StatusChanges changes)
{
    mStatusChanges = mStatusChanges | changes;
    if (mPaintLock == 0)
        doOnStatusChange(mStatusChanges);
}

void SynEdit::doOnStatusChange(StatusChanges)
{
    if (mStatusChanges.testFlag(StatusChange::scCaretX)
            || mStatusChanges.testFlag(StatusChange::scCaretY)) {
        qApp->inputMethod()->update(Qt::ImCursorPosition);
    }
    emit statusChanged(mStatusChanges);
    mStatusChanges = StatusChange::scNone;
}

void SynEdit::updateScrollbars()
{
    int nMaxScroll;
    int nMin,nMax,nPage,nPos;
    if (mPaintLock!=0) {
        mStateFlags.setFlag(StateFlag::sfScrollbarChanged);
    } else {
        mStateFlags.setFlag(StateFlag::sfScrollbarChanged,false);
        if (mScrollBars != ScrollStyle::ssNone) {
            if (mOptions.testFlag(eoHideShowScrollbars)) {
                setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
                setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
            } else {
                setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
                setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
            }
            if (mScrollBars == ScrollStyle::ssBoth ||  mScrollBars == ScrollStyle::ssHorizontal) {
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

            if (mScrollBars == ScrollStyle::ssBoth ||  mScrollBars == ScrollStyle::ssVertical) {
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
    mStateFlags.setFlag(StateFlag::sfCaretChanged,false);
    invalidateRect(calculateCaretRect());
}

void SynEdit::recalcCharExtent()
{
    FontStyle styles[] = {FontStyle::fsBold, FontStyle::fsItalic, FontStyle::fsStrikeOut, FontStyle::fsUnderline};
    bool hasStyles[] = {false,false,false,false};
    int size = 4;
    if (mHighlighter && mHighlighter->attributes().count()>0) {
        for (const PHighlighterAttribute& attribute: mHighlighter->attributes()) {
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
    QFontMetrics fm(font());
    QFontMetrics fm2(font());
    mTextHeight = std::max(fm.lineSpacing(),fm2.lineSpacing());
    mCharWidth = fm.horizontalAdvance("M");

    if (hasStyles[0]) { // has bold font
        QFont f = font();
        f.setBold(true);
        QFontMetrics fm(f);
        QFont f2 = font();
        f2.setBold(true);
        QFontMetrics fm2(f);
        if (fm.lineSpacing()>mTextHeight)
            mTextHeight=fm.lineSpacing();
        if (fm2.lineSpacing()>mTextHeight)
            mTextHeight=fm2.lineSpacing();
        if (fm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = fm.horizontalAdvance("M");
    }
    if (hasStyles[1]) { // has strike out font
        QFont f = font();
        f.setItalic(true);
        QFontMetrics fm(f);
        QFont f2 = font();
        f2.setItalic(true);
        QFontMetrics fm2(f);
        if (fm.lineSpacing()>mTextHeight)
            mTextHeight=fm.lineSpacing();
        if (fm2.lineSpacing()>mTextHeight)
            mTextHeight=fm2.lineSpacing();
        if (fm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = fm.horizontalAdvance("M");
    }
    if (hasStyles[2]) { // has strikeout
        QFont f = font();
        f.setStrikeOut(true);
        QFontMetrics fm(f);
        QFont f2 = font();
        f2.setStrikeOut(true);
        QFontMetrics fm2(f);
        if (fm.lineSpacing()>mTextHeight)
            mTextHeight=fm.lineSpacing();
        if (fm2.lineSpacing()>mTextHeight)
            mTextHeight=fm2.lineSpacing();
        if (fm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = fm.horizontalAdvance("M");
    }
    if (hasStyles[3]) { // has underline
        QFont f = font();
        f.setUnderline(true);
        QFontMetrics fm(f);
        QFont f2 = font();
        f2.setUnderline(true);
        QFontMetrics fm2(f);
        if (fm.lineSpacing()>mTextHeight)
            mTextHeight=fm.lineSpacing();
        if (fm2.lineSpacing()>mTextHeight)
            mTextHeight=fm2.lineSpacing();
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
    return Result;
}

void SynEdit::updateModifiedStatus()
{
    bool oldModified = mModified;
    mModified = !mUndoList->initialState();
    setModified(mModified);
//    qDebug()<<mModified<<oldModified;
    if (oldModified!=mModified)
        emit statusChanged(StatusChange::scModifyChanged);
}

int SynEdit::scanFrom(int Index, int canStopIndex)
{
    HighlighterState iRange;
    int Result = std::max(0,Index);
    if (Result >= mDocument->count())
        return Result;

    if (Result == 0) {
        mHighlighter->resetState();
    } else {
        mHighlighter->setState(mDocument->ranges(Result-1));
    }
    do {
        mHighlighter->setLine(mDocument->getString(Result), Result);
        mHighlighter->nextToEol();
        iRange = mHighlighter->getState();
        if (Result > canStopIndex){
            if (mDocument->ranges(Result).state == iRange.state
                    && mDocument->ranges(Result).braceLevel == iRange.braceLevel
                    && mDocument->ranges(Result).parenthesisLevel == iRange.parenthesisLevel
                    && mDocument->ranges(Result).bracketLevel == iRange.bracketLevel
                    ) {
                if (mUseCodeFolding)
                    rescanFolds();
                return Result;// avoid the final Decrement
            }
        }
        mDocument->setRange(Result,iRange);
        Result ++ ;
    } while (Result < mDocument->count());
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
    if (line >= mDocument->count())
        return;

    if (line == 0) {
        mHighlighter->resetState();
    } else {
        mHighlighter->setState(mDocument->ranges(line-1));
    }
    mHighlighter->setLine(mDocument->getString(line), line);
    mHighlighter->nextToEol();
    HighlighterState iRange = mHighlighter->getState();
    mDocument->setRange(line,iRange);
}

void SynEdit::rescanRanges()
{
    if (mHighlighter && !mDocument->empty()) {
        mHighlighter->resetState();
        for (int i =0;i<mDocument->count();i++) {
            mHighlighter->setLine(mDocument->getString(i), i);
            mHighlighter->nextToEol();
            mDocument->setRange(i, mHighlighter->getState());
        }
    }
    if (mUseCodeFolding)
        rescanFolds();
}

void SynEdit::uncollapse(PCodeFoldingRange FoldRange)
{
    FoldRange->linesCollapsed = 0;
    FoldRange->collapsed = false;

    // Redraw the collapsed line
    invalidateLines(FoldRange->fromLine, INT_MAX);

    // Redraw fold mark
    invalidateGutterLines(FoldRange->fromLine, INT_MAX);
    updateScrollbars();
}

void SynEdit::collapse(PCodeFoldingRange FoldRange)
{
    FoldRange->linesCollapsed = FoldRange->toLine - FoldRange->fromLine;
    FoldRange->collapsed = true;

    // Extract caret from fold
    if ((mCaretY > FoldRange->fromLine) && (mCaretY <= FoldRange->toLine)) {
          setCaretXY(BufferCoord{mDocument->getString(FoldRange->fromLine - 1).length() + 1,
                                 FoldRange->fromLine});
    }

    // Redraw the collapsed line
    invalidateLines(FoldRange->fromLine, INT_MAX);

    // Redraw fold mark
    invalidateGutterLines(FoldRange->fromLine, INT_MAX);

    updateScrollbars();
}

void SynEdit::foldOnListInserted(int Line, int Count)
{
    // Delete collapsed inside selection
    for (int i = mAllFoldRanges.count()-1;i>=0;i--) {
        PCodeFoldingRange range = mAllFoldRanges[i];
        if (range->fromLine == Line - 1) {// insertion starts at fold line
            if (range->collapsed)
                uncollapse(range);
        } else if (range->fromLine >= Line) // insertion of count lines above FromLine
            range->move(Count);
    }
}

void SynEdit::foldOnListDeleted(int Line, int Count)
{
    // Delete collapsed inside selection
    for (int i = mAllFoldRanges.count()-1;i>=0;i--) {
        PCodeFoldingRange range = mAllFoldRanges[i];
        if (range->fromLine == Line && Count == 1)  {// open up because we are messing with the starting line
            if (range->collapsed)
                uncollapse(range);
        } else if (range->fromLine >= Line - 1 && range->fromLine < Line + Count) // delete inside affectec area
            mAllFoldRanges.remove(i);
        else if (range->fromLine >= Line + Count) // Move after affected area
            range->move(-Count);

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

static void null_deleter(CodeFoldingRanges *) {}

void SynEdit::rescanForFoldRanges()
{
    // Delete all uncollapsed folds
//    for (int i=mAllFoldRanges.count()-1;i>=0;i--) {
//        PSynEditFoldRange range =mAllFoldRanges[i];
//        if (!range->collapsed && !range->parentCollapsed())
//            mAllFoldRanges.remove(i);
//    }

    // Did we leave any collapsed folds and are we viewing a code file?
    if (mAllFoldRanges.count() > 0) {
        CodeFoldingRanges ranges=mAllFoldRanges;
        mAllFoldRanges.clear();
        // Add folds to a separate list
        PCodeFoldingRanges temporaryAllFoldRanges = std::make_shared<CodeFoldingRanges>();
        scanForFoldRanges(temporaryAllFoldRanges);

        // Combine new with old folds, preserve parent order
        for (int i = 0; i< temporaryAllFoldRanges->count();i++) {
            PCodeFoldingRange tempFoldRange=temporaryAllFoldRanges->range(i);
            int j=0;
            while (j <ranges.count()) {
                PCodeFoldingRange foldRange = ranges[j];
                //qDebug()<<TemporaryAllFoldRanges->range(i)->fromLine<<ranges[j]->fromLine;
                if (tempFoldRange->fromLine == foldRange->fromLine
                        && tempFoldRange->toLine == foldRange->toLine) {
                    //qDebug()<<"-"<<foldRange->fromLine;
                    mAllFoldRanges.add(foldRange);
                    break;
                }
                j++;
            }
            if (j>=ranges.count()) {
                //qDebug()<<"--"<<tempFoldRange->fromLine;
                mAllFoldRanges.add(tempFoldRange);
            }
        }

    } else {

        // We ended up with no folds after deleting, just pass standard data...
        PCodeFoldingRanges temp(&mAllFoldRanges, null_deleter);
        scanForFoldRanges(temp);
    }
}

void SynEdit::scanForFoldRanges(PCodeFoldingRanges TopFoldRanges)
{
    PCodeFoldingRanges parentFoldRanges = TopFoldRanges;
    // Recursively scan for folds (all types)
    for (int i= 0 ; i< mCodeFolding.foldRegions.count() ; i++ ) {
        findSubFoldRange(TopFoldRanges, i,parentFoldRanges,PCodeFoldingRange());
    }
}

//this func should only be used in findSubFoldRange
int SynEdit::lineHasChar(int Line, int startChar, QChar character, const QString& highlighterAttrName) {
    QString CurLine = mDocument->getString(Line);
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
            PHighlighterAttribute attr = mHighlighter->getTokenAttribute();
            if (token == character && attr->name()==highlighterAttrName)
                return mHighlighter->getTokenPos();
            mHighlighter->next();
        }
    }
    return -1;
}

void SynEdit::findSubFoldRange(PCodeFoldingRanges TopFoldRanges, int FoldIndex,PCodeFoldingRanges& parentFoldRanges, PCodeFoldingRange Parent)
{
    PCodeFoldingRange  CollapsedFold;
    int Line = 0;
    QString CurLine;
    if (!mHighlighter)
        return;
    bool useBraces = ( mCodeFolding.foldRegions.get(FoldIndex)->openSymbol == "{"
            && mCodeFolding.foldRegions.get(FoldIndex)->closeSymbol == "}");

    while (Line < mDocument->count()) { // index is valid for LinesToScan and fLines
        // If there is a collapsed fold over here, skip it
        CollapsedFold = collapsedFoldStartAtLine(Line + 1); // only collapsed folds remain
        if (CollapsedFold) {
          Line = CollapsedFold->toLine;
          continue;
        }

        //we just use braceLevel
        if (useBraces) {
            // Find an opening character on this line
            CurLine = mDocument->getString(Line);
            if (mDocument->rightBraces(Line)>0) {
                for (int i=0; i<mDocument->rightBraces(Line);i++) {
                    // Stop the recursion if we find a closing char, and return to our parent
                    if (Parent) {
                      Parent->toLine = Line + 1;
                      Parent = Parent->parent.lock();
                      if (!Parent) {
                          parentFoldRanges = TopFoldRanges;
                      } else {
                          parentFoldRanges = Parent->subFoldRanges;
                      }
                    }
                }
            }
            if (mDocument->leftBraces(Line)>0) {
                for (int i=0; i<mDocument->leftBraces(Line);i++) {
                    // Add it to the top list of folds
                    Parent = parentFoldRanges->addByParts(
                      Parent,
                      TopFoldRanges,
                      Line + 1,
                      Line + 1);
                    parentFoldRanges = Parent->subFoldRanges;
                }
            }
        } else {

            // Find an opening character on this line
            CurLine = mDocument->getString(Line);

            mHighlighter->setState(mDocument->ranges(Line));
            mHighlighter->setLine(CurLine,Line);

            QString token;
            int pos;
            while (!mHighlighter->eol()) {
                token = mHighlighter->getToken();
                pos = mHighlighter->getTokenPos()+token.length();
                PHighlighterAttribute attr = mHighlighter->getTokenAttribute();
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
                          Parent = Parent->parent.lock();
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

PCodeFoldingRange SynEdit::collapsedFoldStartAtLine(int Line)
{
    for (int i = 0; i< mAllFoldRanges.count() - 1; i++ ) {
        if (mAllFoldRanges[i]->fromLine == Line && mAllFoldRanges[i]->collapsed) {
            return mAllFoldRanges[i];
        } else if (mAllFoldRanges[i]->fromLine > Line) {
            break; // sorted by line. don't bother scanning further
        }
    }
    return PCodeFoldingRange();
}

void SynEdit::initializeCaret()
{
    //showCaret();
}

PCodeFoldingRange SynEdit::foldStartAtLine(int Line) const
{
    for (int i = 0; i<mAllFoldRanges.count();i++) {
        PCodeFoldingRange range = mAllFoldRanges[i];
        if (range->fromLine == Line ){
            return range;
        } else if (range->fromLine>Line)
            break; // sorted by line. don't bother scanning further
    }
    return PCodeFoldingRange();
}

bool SynEdit::foldCollapsedBetween(int startLine, int endLine) const
{
    for (int i = 0; i<mAllFoldRanges.count();i++) {
        PCodeFoldingRange range = mAllFoldRanges[i];
        if (startLine >=range->fromLine && range->fromLine<=endLine
                && (range->collapsed || range->parentCollapsed())){
            return true;
        } else if (range->fromLine>endLine)
            break; // sorted by line. don't bother scanning further
    }
    return false;
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
            columns += tabWidth() - (columns % tabWidth());
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
            columns += tabWidth() - (columns % tabWidth());
        else
            columns += charColumns(s[i]);
        i++;
        j++;
    }
    result.resize(j);
    colLen = columns-oldColumns;
    return result;
}

PCodeFoldingRange SynEdit::foldAroundLine(int Line)
{
    return foldAroundLineEx(Line,false,false,false);
}

PCodeFoldingRange SynEdit::foldAroundLineEx(int Line, bool WantCollapsed, bool AcceptFromLine, bool AcceptToLine)
{
    // Check global list
    PCodeFoldingRange Result = checkFoldRange(&mAllFoldRanges, Line, WantCollapsed, AcceptFromLine, AcceptToLine);

    // Found an item in the top level list?
    if (Result) {
        while (true) {
            PCodeFoldingRange ResultChild = checkFoldRange(Result->subFoldRanges.get(), Line, WantCollapsed, AcceptFromLine, AcceptToLine);
            if (!ResultChild)
                break;
            Result = ResultChild; // repeat for this one
        }
    }
    return Result;
}

PCodeFoldingRange SynEdit::checkFoldRange(CodeFoldingRanges *FoldRangeToCheck, int Line, bool WantCollapsed, bool AcceptFromLine, bool AcceptToLine)
{
    for (int i = 0; i< FoldRangeToCheck->count(); i++) {
        PCodeFoldingRange range = (*FoldRangeToCheck)[i];
        if (((range->fromLine < Line) || ((range->fromLine <= Line) && AcceptFromLine)) &&
          ((range->toLine > Line) || ((range->toLine >= Line) && AcceptToLine))) {
            if (range->collapsed == WantCollapsed) {
                return range;
            }
        }
    }
    return PCodeFoldingRange();
}

PCodeFoldingRange SynEdit::foldEndAtLine(int Line)
{
    for (int i = 0; i<mAllFoldRanges.count();i++) {
        PCodeFoldingRange range = mAllFoldRanges[i];
        if (range->toLine == Line ){
            return range;
        } else if (range->fromLine>Line)
            break; // sorted by line. don't bother scanning further
    }
    return PCodeFoldingRange();
}

void SynEdit::paintCaret(QPainter &painter, const QRect rcClip)
{
    if (m_blinkStatus!=1)
        return;
    painter.setClipRect(rcClip);
    EditCaretType ct;
    if (this->mInserting) {
        ct = mInsertCaret;
    } else {
        ct =mOverwriteCaret;
    }
    QColor caretColor;
    if (mCaretUseTextColor) {
        caretColor = mForegroundColor;
    } else {
        caretColor = mCaretColor;
    }
    switch(ct) {
    case EditCaretType::ctVerticalLine: {
        QRect caretRC;
        int size = std::max(1, mTextHeight/15);
        caretRC.setLeft(rcClip.left()+1);
        caretRC.setTop(rcClip.top());
        caretRC.setBottom(rcClip.bottom());
        caretRC.setRight(rcClip.left()+1+size);
        painter.fillRect(caretRC,caretColor);
        break;
    }
    case EditCaretType::ctHorizontalLine: {
        QRect caretRC;
        int size = std::max(1,mTextHeight/15);
        caretRC.setLeft(rcClip.left());
        caretRC.setTop(rcClip.bottom()-1-size);
        caretRC.setBottom(rcClip.bottom()-1);
        caretRC.setRight(rcClip.right());
        painter.fillRect(caretRC,caretColor);
        break;
    }
    case EditCaretType::ctBlock:
        painter.fillRect(rcClip, caretColor);
        break;
    case EditCaretType::ctHalfBlock:
        QRect rc=rcClip;
        rc.setTop(rcClip.top()+rcClip.height() / 2);
        painter.fillRect(rcClip, caretColor);
        break;
    }
}

int SynEdit::textOffset() const
{
    return mGutterWidth + 2 - (mLeftChar-1)*mCharWidth;
}

EditCommand SynEdit::TranslateKeyCode(int key, Qt::KeyboardModifiers modifiers)
{
    PEditKeyStroke keyStroke = mKeyStrokes.findKeycode2(mLastKey,mLastKeyModifiers,
                                                           key, modifiers);
    EditCommand cmd=EditCommand::ecNone;
    if (keyStroke)
        cmd = keyStroke->command();
    else {
        keyStroke = mKeyStrokes.findKeycode(key,modifiers);
        if (keyStroke)
            cmd = keyStroke->command();
    }
    if (cmd == EditCommand::ecNone) {
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
        bool scrollBarChangedSettings = mStateFlags.testFlag(StateFlag::sfScrollbarChanged);
        if (bFont) {
            if (mGutter.showLineNumbers())
                onGutterChanged();
            else
                updateScrollbars();
            mStateFlags.setFlag(StateFlag::sfCaretChanged,false);
            invalidate();
        } else
            updateScrollbars();
        mStateFlags.setFlag(StateFlag::sfScrollbarChanged,scrollBarChangedSettings);
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

ScrollStyle SynEdit::scrollBars() const
{
    return mScrollBars;
}

void SynEdit::setScrollBars(ScrollStyle newScrollBars)
{
    mScrollBars = newScrollBars;
}

int SynEdit::mouseSelectionScrollSpeed() const
{
    return mMouseSelectionScrollSpeed;
}

void SynEdit::setMouseSelectionScrollSpeed(int newMouseSelectionScrollSpeed)
{
    mMouseSelectionScrollSpeed = newMouseSelectionScrollSpeed;
}

const QFont &SynEdit::fontForNonAscii() const
{
    return mFontForNonAscii;
}

void SynEdit::setFontForNonAscii(const QFont &newFontForNonAscii)
{
    mFontForNonAscii = newFontForNonAscii;
    mFontForNonAscii.setStyleStrategy(QFont::PreferAntialias);
    if (mDocument)
        mDocument->setFontMetrics(font(),mFontForNonAscii);
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

const PHighlighterAttribute &SynEdit::rainbowAttr3() const
{
    return mRainbowAttr3;
}

const PHighlighterAttribute &SynEdit::rainbowAttr2() const
{
    return mRainbowAttr2;
}

const PHighlighterAttribute &SynEdit::rainbowAttr1() const
{
    return mRainbowAttr1;
}

const PHighlighterAttribute &SynEdit::rainbowAttr0() const
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

Gutter& SynEdit::gutter()
{
    return mGutter;
}

EditCaretType SynEdit::insertCaret() const
{
    return mInsertCaret;
}

void SynEdit::setInsertCaret(const EditCaretType &insertCaret)
{
    mInsertCaret = insertCaret;
}

EditCaretType SynEdit::overwriteCaret() const
{
    return mOverwriteCaret;
}

void SynEdit::setOverwriteCaret(const EditCaretType &overwriteCaret)
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

void SynEdit::setTabWidth(int newTabWidth)
{
    if (newTabWidth!=tabWidth()) {
        mDocument->setTabWidth(newTabWidth);
        invalidate();
    }
}

EditorOptions SynEdit::getOptions() const
{
    return mOptions;
}

void SynEdit::setOptions(const EditorOptions &Value)
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
    if (mInserting == false && !selAvail()) {
        switch(mActiveSelectionMode) {
        case SelectionMode::Column: {
            //we can't use blockBegin()/blockEnd()
            BufferCoord start=blockBegin();
            BufferCoord end=blockEnd();
            if (start.line > end.line )
                std::swap(start,end);
            start.ch+=s.length(); // make sure we select a whole char in the start line
            setBlockBegin(start);
            setBlockEnd(end);
        }
            break;
        case SelectionMode::Line:
            //do nothing;
            break;
        default:
            setSelLength(s.length());
        }
    }
    doSetSelText(s);
}

void SynEdit::doUndo()
{
    if (mReadOnly)
        return;

    //Remove Group Break;
    while (mUndoList->lastChangeReason() ==  ChangeReason::GroupBreak) {
        PUndoItem item = mUndoList->popItem();
        mRedoList->addRedo(item);
    }

    PUndoItem item = mUndoList->peekItem();
    if (item) {
        size_t oldChangeNumber = item->changeNumber();
        {
            ChangeReason  lastChange = mUndoList->lastChangeReason();
            bool keepGoing;
            do {
                doUndoItem();
                item = mUndoList->peekItem();
                if (!item)
                    keepGoing = false;
                else {
                    if (item->changeNumber() == oldChangeNumber)
                        keepGoing = true;
                    else {
                        keepGoing = (mOptions.testFlag(eoGroupUndo) &&
                            (lastChange == item->changeReason()) );
                    }
                    oldChangeNumber=item->changeNumber();
                    lastChange = item->changeReason();
                }
            } while (keepGoing);
        }
    }
    updateModifiedStatus();
    onChanged();
}

void SynEdit::doUndoItem()
{
    mUndoing = true;
    bool ChangeScrollPastEol = ! mOptions.testFlag(eoScrollPastEol);

    PUndoItem item = mUndoList->popItem();
    if (item) {
        setActiveSelectionMode(item->changeSelMode());
        incPaintLock();
        auto action = finally([&,this]{
            mUndoing = false;
            if (ChangeScrollPastEol)
                mOptions.setFlag(eoScrollPastEol,false);
            decPaintLock();
        });
        mOptions.setFlag(eoScrollPastEol);
        switch(item->changeReason()) {
        case ChangeReason::Caret:
            mRedoList->addRedo(
                        item->changeReason(),
                        caretXY(),
                        caretXY(), QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            internalSetCaretXY(item->changeStartPos());
            break;
        case ChangeReason::LeftTop:
            BufferCoord p;
            p.ch = leftChar();
            p.line = topLine();
            mRedoList->addRedo(
                        item->changeReason(),
                        p,
                        p, QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            setLeftChar(item->changeStartPos().ch);
            setTopLine(item->changeStartPos().line);
            break;
        case ChangeReason::Selection:
            mRedoList->addRedo(
                        item->changeReason(),
                        mBlockBegin,
                        mBlockEnd,
                        QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            setCaretAndSelection(caretXY(), item->changeStartPos(), item->changeEndPos());
            break;
        case ChangeReason::Insert: {
            QStringList tmpText = getContent(item->changeStartPos(),item->changeEndPos(),item->changeSelMode());
            doDeleteText(item->changeStartPos(),item->changeEndPos(),item->changeSelMode());
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        tmpText,
                        item->changeSelMode(),
                        item->changeNumber());
            internalSetCaretXY(item->changeStartPos());
            break;
        }
        case ChangeReason::MoveSelectionUp:
            setBlockBegin(BufferCoord{item->changeStartPos().ch, item->changeStartPos().line-1});
            setBlockEnd(BufferCoord{item->changeEndPos().ch, item->changeEndPos().line-1});
            doMoveSelDown();
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        case ChangeReason::MoveSelectionDown:
            setBlockBegin(BufferCoord{item->changeStartPos().ch, item->changeStartPos().line+1});
            setBlockEnd(BufferCoord{item->changeEndPos().ch, item->changeEndPos().line+1});
            doMoveSelUp();
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        case ChangeReason::Delete: {
            // If there's no selection, we have to set
            // the Caret's position manualy.
//            qDebug()<<"undo delete";
//            qDebug()<<Item->changeText();
//            qDebug()<<Item->changeStartPos().Line<<Item->changeStartPos().Char;
            doInsertText(item->changeStartPos(),item->changeText(),item->changeSelMode(),
                         item->changeStartPos().line,
                         item->changeEndPos().line);
            internalSetCaretXY(item->changeEndPos());
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            ensureCursorPosVisible();
            break;
        }
        case ChangeReason::LineBreak:{
            QString s;
            if (!item->changeText().isEmpty()) {
                s=item->changeText()[0];
            }
            // If there's no selection, we have to set
            // the Caret's position manualy.
            internalSetCaretXY(item->changeStartPos());
            if (mCaretY > 0) {
                QString TmpStr = mDocument->getString(mCaretY - 1);
                if ( (mCaretX > TmpStr.length() + 1) && (leftSpaces(s) == 0))
                    TmpStr = TmpStr + QString(mCaretX - 1 - TmpStr.length(), ' ');
                properSetLine(mCaretY - 1, TmpStr + s);
                mDocument->deleteAt(mCaretY);
                doLinesDeleted(mCaretY, 1);
            }
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        }
        default:
            break;
        }
    }
}

void SynEdit::doRedo()
{
    if (mReadOnly)
        return;

    PUndoItem item = mRedoList->peekItem();
    if (!item)
        return;
    size_t oldChangeNumber = item->changeNumber();

    //skip group chain breakers
    while (mRedoList->lastChangeReason()==ChangeReason::GroupBreak) {
        PUndoItem item = mRedoList->popItem();
        mUndoList->restoreChange(item);
    }
    ChangeReason lastChange = mRedoList->lastChangeReason();
    bool keepGoing;
    do {
      doRedoItem();
      item = mRedoList->peekItem();
      if (!item)
          keepGoing = false;
      else {
        if (item->changeNumber() == oldChangeNumber)
            keepGoing = true;
        else {
            keepGoing = (mOptions.testFlag(eoGroupUndo) &&
            (lastChange == item->changeReason()));
        }
        oldChangeNumber=item->changeNumber();
        lastChange = item->changeReason();
      }
    } while (keepGoing);

    //restore Group Break
    while (mRedoList->lastChangeReason()==ChangeReason::GroupBreak) {
        PUndoItem item = mRedoList->popItem();
        mUndoList->restoreChange(item);
    }
    updateModifiedStatus();
    onChanged();
}

void SynEdit::doRedoItem()
{
    mUndoing = true;
    bool ChangeScrollPastEol = ! mOptions.testFlag(eoScrollPastEol);
    PUndoItem item = mRedoList->popItem();
    if (item) {
        setActiveSelectionMode(item->changeSelMode());
        incPaintLock();
        mOptions.setFlag(eoScrollPastEol);
        mUndoList->setInsideRedo(true);
        auto action = finally([&,this]{
            mUndoing = false;
            mUndoList->setInsideRedo(false);
            if (ChangeScrollPastEol)
                mOptions.setFlag(eoScrollPastEol,false);
            decPaintLock();
        });
        switch(item->changeReason()) {
        case ChangeReason::Caret:
            mUndoList->restoreChange(
                        item->changeReason(),
                        caretXY(),
                        caretXY(),
                        QStringList(),
                        mActiveSelectionMode,
                        item->changeNumber());
            internalSetCaretXY(item->changeStartPos());
            break;
        case ChangeReason::LeftTop:
            BufferCoord p;
            p.ch = leftChar();
            p.line = topLine();
            mUndoList->restoreChange(
                        item->changeReason(),
                        p,
                        p, QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            setLeftChar(item->changeStartPos().ch);
            setTopLine(item->changeStartPos().line);
            break;
        case ChangeReason::Selection:
            mUndoList->restoreChange(
                        item->changeReason(),
                        mBlockBegin,
                        mBlockEnd,
                        QStringList(),
                        mActiveSelectionMode,
                        item->changeNumber());
            setCaretAndSelection(
                        caretXY(),
                        item->changeStartPos(),
                        item->changeEndPos());
            break;
        case ChangeReason::MoveSelectionUp:
            setBlockBegin(BufferCoord{item->changeStartPos().ch, item->changeStartPos().line});
            setBlockEnd(BufferCoord{item->changeEndPos().ch, item->changeEndPos().line});
            doMoveSelUp();
            mUndoList->restoreChange(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        case ChangeReason::MoveSelectionDown:
            setBlockBegin(BufferCoord{item->changeStartPos().ch, item->changeStartPos().line});
            setBlockEnd(BufferCoord{item->changeEndPos().ch, item->changeEndPos().line});
            doMoveSelDown();
            mUndoList->restoreChange(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        case ChangeReason::Insert:
            setCaretAndSelection(
                        item->changeStartPos(),
                        item->changeStartPos(),
                        item->changeStartPos());
            doInsertText(item->changeStartPos(),item->changeText(), item->changeSelMode(),
                         item->changeStartPos().line,
                         item->changeEndPos().line);
            internalSetCaretXY(item->changeEndPos());
            mUndoList->restoreChange(item->changeReason(),
                                 item->changeStartPos(),
                                 item->changeEndPos(),
                                 QStringList(),
                                 item->changeSelMode(),
                                 item->changeNumber());
            break;
        case ChangeReason::Delete: {
            doDeleteText(item->changeStartPos(),item->changeEndPos(),item->changeSelMode());
            mUndoList->restoreChange(item->changeReason(), item->changeStartPos(),
                                 item->changeEndPos(),item->changeText(),
                                 item->changeSelMode(),item->changeNumber());
            internalSetCaretXY(item->changeStartPos());
            break;
        };
        case ChangeReason::LineBreak: {
            BufferCoord CaretPt = item->changeStartPos();
            mUndoList->restoreChange(item->changeReason(), item->changeStartPos(),
                                 item->changeEndPos(),item->changeText(),
                                 item->changeSelMode(),item->changeNumber());
            setCaretAndSelection(CaretPt, CaretPt, CaretPt);
            commandProcessor(EditCommand::ecLineBreak);
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
    int size = newFont.pixelSize();
    size++;
    newFont.setPixelSize(size);
    setFont(newFont);
}

void SynEdit::doZoomOut()
{
    QFont newFont = font();
    int size = newFont.pixelSize();
    size--;
    if (size<2)
        size = 2;
    newFont.setPixelSize(size);
    setFont(newFont);
}

SelectionMode SynEdit::selectionMode() const
{
    return mSelectionMode;
}

void SynEdit::setSelectionMode(SelectionMode value)
{
    if (mSelectionMode!=value) {
        mSelectionMode = value;
        setActiveSelectionMode(value);
    }
}

QString SynEdit::selText() const
{
    if (!selAvail()) {
        return "";
    } else {
        int ColFrom = blockBegin().ch;
        int First = blockBegin().line - 1;
        //
        int ColTo = blockEnd().ch;
        int Last = blockEnd().line - 1;

        switch(mActiveSelectionMode) {
        case SelectionMode::Normal:{
            PCodeFoldingRange foldRange = foldStartAtLine(blockEnd().line);
            QString s = mDocument->getString(Last);
            if ((foldRange) && foldRange->collapsed && ColTo>s.length()) {
                s=s+highlighter()->foldString();
                if (ColTo>s.length()) {
                    Last = foldRange->toLine-1;
                    ColTo = mDocument->getString(Last).length()+1;
                }
            }
            if (First == Last)
                return  mDocument->getString(First).mid(ColFrom-1, ColTo - ColFrom);
            else {
                QString result = mDocument->getString(First).mid(ColFrom-1);
                result+= lineBreak();
                for (int i = First + 1; i<=Last - 1; i++) {
                    result += mDocument->getString(i);
                    result+=lineBreak();
                }
                result += mDocument->getString(Last).leftRef(ColTo-1);
                return result;
            }
        }
        case SelectionMode::Column:
        {
              First = blockBegin().line;
              ColFrom = charToColumn(blockBegin().line, blockBegin().ch);
              Last = blockEnd().line;
              ColTo = charToColumn(blockEnd().line, blockEnd().ch);
              if (ColFrom > ColTo)
                  std::swap(ColFrom, ColTo);
              if (First>Last)
                  std::swap(First,Last);
              QString result;
              for (int i = First; i <= Last; i++) {
                  int l = columnToChar(i,ColFrom);
                  int r = columnToChar(i,ColTo-1)+1;
                  QString s = mDocument->getString(i-1);
                  result += s.mid(l-1,r-l);
                  if (i<Last)
                      result+=lineBreak();
              }
              return result;
        }
        case SelectionMode::Line:
        {
            QString result;
            // If block selection includes LastLine,
            // line break code(s) of the last line will not be added.
            for (int i= First; i<=Last - 1;i++) {
                result += mDocument->getString(i);
                result+=lineBreak();
            }
            result += mDocument->getString(Last);
            if (Last < mDocument->count() - 1)
                result+=lineBreak();
            return result;
        }
        }
    }
    return "";
}

QStringList SynEdit::getContent(BufferCoord startPos, BufferCoord endPos, SelectionMode mode) const
{
    QStringList result;
    if (startPos==endPos) {
        return result;
    }
    if (startPos>endPos) {
        std::swap(startPos,endPos);
    }
    int ColFrom = startPos.ch;
    int First = startPos.line - 1;
    //
    int ColTo = endPos.ch;
    int Last = endPos.line - 1;

    switch(mode) {
    case SelectionMode::Normal:{
        PCodeFoldingRange foldRange = foldStartAtLine(endPos.line);
        QString s = mDocument->getString(Last);
        if ((foldRange) && foldRange->collapsed && ColTo>s.length()) {
            s=s+highlighter()->foldString();
            if (ColTo>s.length()) {
                Last = foldRange->toLine-1;
                ColTo = mDocument->getString(Last).length()+1;
            }
        }
    }
        if (First == Last) {
            result.append(mDocument->getString(First).mid(ColFrom-1, ColTo - ColFrom));
        } else {
            result.append(mDocument->getString(First).mid(ColFrom-1));
            for (int i = First + 1; i<=Last - 1; i++) {
                result.append(mDocument->getString(i));
            }
            result.append(mDocument->getString(Last).left(ColTo-1));
        }
        break;
    case SelectionMode::Column:
          First = blockBegin().line;
          ColFrom = charToColumn(blockBegin().line, blockBegin().ch);
          Last = blockEnd().line;
          ColTo = charToColumn(blockEnd().line, blockEnd().ch);
          if (ColFrom > ColTo)
              std::swap(ColFrom, ColTo);
          if (First>Last)
              std::swap(First,Last);
          for (int i = First; i <= Last; i++) {
              int l = columnToChar(i,ColFrom);
              int r = columnToChar(i,ColTo-1)+1;
              QString s = mDocument->getString(i-1);
              result.append(s.mid(l-1,r-l));
          }
          break;
    case SelectionMode::Line:
        // If block selection includes LastLine,
        // line break code(s) of the last line will not be added.
        for (int i= First; i<=Last - 1;i++) {
            result.append(mDocument->getString(i));
        }
        result.append(mDocument->getString(Last));
        if (Last < mDocument->count() - 1)
            result.append("");
        break;
    }
    return result;
}

QString SynEdit::lineBreak() const
{
    return mDocument->lineBreak();
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

CodeFoldingOptions &SynEdit::codeFolding()
{
    return mCodeFolding;
}

QString SynEdit::displayLineText()
{
    if (mCaretY >= 1 && mCaretY <= mDocument->count()) {
        QString s= mDocument->getString(mCaretY - 1);
        PCodeFoldingRange foldRange = foldStartAtLine(mCaretY);
        if ((foldRange) && foldRange->collapsed) {
            return s+highlighter()->foldString();
        }
        return s;
    }
    return QString();
}

QString SynEdit::lineText() const
{
    if (mCaretY >= 1 && mCaretY <= mDocument->count())
        return mDocument->getString(mCaretY - 1);
    else
        return QString();
}

void SynEdit::setLineText(const QString s)
{
    if (mCaretY >= 1 && mCaretY <= mDocument->count())
        mDocument->putString(mCaretY-1,s);
}

PHighlighter SynEdit::highlighter() const
{
    return mHighlighter;
}

void SynEdit::setHighlighter(const PHighlighter &highlighter)
{
    PHighlighter oldHighlighter= mHighlighter;
    mHighlighter = highlighter;
    if (oldHighlighter && mHighlighter &&
            oldHighlighter->language() == highlighter->language()) {
    } else {
        recalcCharExtent();
        mDocument->beginUpdate();
        auto action=finally([this]{
            mDocument->endUpdate();
        });
        rescanRanges();
    }
    onSizeOrFontChanged(true);
    invalidate();
}

const PDocument& SynEdit::document() const
{
    return mDocument;
}

bool SynEdit::empty()
{
    return mDocument->empty();
}

void SynEdit::commandProcessor(EditCommand Command, QChar AChar, void *pData)
{
    // first the program event handler gets a chance to process the command
    onProcessCommand(Command, AChar, pData);
    if (Command != EditCommand::ecNone)
        executeCommand(Command, AChar, pData);
    onCommandProcessed(Command, AChar, pData);
}

void SynEdit::moveCaretHorz(int DX, bool isSelection)
{
    BufferCoord ptO = caretXY();
    BufferCoord ptDst = ptO;
    QString s = displayLineText();
    int nLineLen = s.length();
    // only moving or selecting one char can change the line
    //bool bChangeY = !mOptions.testFlag(SynEditorOption::eoScrollPastEol);
    bool bChangeY=true;
    if (bChangeY && (DX == -1) && (ptO.ch == 1) && (ptO.line > 1)) {
        // end of previous line
        if (mActiveSelectionMode==SelectionMode::Column) {
            return;
        }
        int row = lineToRow(ptDst.line);
        row--;
        int line = rowToLine(row);
        if (line!=ptDst.line && line>=1) {
            ptDst.line = line;
            ptDst.ch = getDisplayStringAtLine(ptDst.line).length() + 1;
        }
    } else if (bChangeY && (DX == 1) && (ptO.ch > nLineLen) && (ptO.line < mDocument->count())) {
        // start of next line
        if (mActiveSelectionMode==SelectionMode::Column) {
            return;
        }
        int row = lineToRow(ptDst.line);
        row++;
        int line = rowToLine(row);
//        qDebug()<<line<<ptDst.Line;
        if (line!=ptDst.line && line<=mDocument->count()) {
            ptDst.line = line;
            ptDst.ch = 1;
        }
    } else {
        ptDst.ch = std::max(1, ptDst.ch + DX);
        // don't go past last char when ScrollPastEol option not set
        if ((DX > 0) && bChangeY)
          ptDst.ch = std::min(ptDst.ch, nLineLen + 1);
    }
    // set caret and block begin / end
    incPaintLock();
    if (mOptions.testFlag(eoAltSetsColumnMode) &&
                         (mActiveSelectionMode != SelectionMode::Line)) {
        if (qApp->keyboardModifiers().testFlag(Qt::AltModifier) && !mReadOnly) {
            setActiveSelectionMode(SelectionMode::Column);
        } else
            setActiveSelectionMode(selectionMode());
    }
    moveCaretAndSelection(mBlockBegin, ptDst, isSelection);
    decPaintLock();
}

void SynEdit::moveCaretVert(int DY, bool isSelection)
{
    DisplayCoord ptO = displayXY();
    DisplayCoord ptDst = ptO;


    ptDst.Row+=DY;
    if (DY >= 0) {
        if (rowToLine(ptDst.Row) > mDocument->count())
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

    if (mActiveSelectionMode==SelectionMode::Column) {
        QString s=mDocument->getString(vDstLineChar.line-1);
        int cols=stringColumns(s,0);
        if (cols+1<ptO.Column)
            return;
    }

    int SaveLastCaretX = mLastCaretColumn;

    // set caret and block begin / end
    incPaintLock();
    if (mOptions.testFlag(eoAltSetsColumnMode) &&
                         (mActiveSelectionMode != SelectionMode::Line)) {
        if (qApp->keyboardModifiers().testFlag(Qt::AltModifier) && !mReadOnly)
            setActiveSelectionMode(SelectionMode::Column);
        else
            setActiveSelectionMode(selectionMode());
    }
    moveCaretAndSelection(mBlockBegin, vDstLineChar, isSelection);
    decPaintLock();

    // Restore fLastCaretX after moving caret, since
    // UpdateLastCaretX, called by SetCaretXYEx, changes them. This is the one
    // case where we don't want that.
    mLastCaretColumn = SaveLastCaretX;
}

void SynEdit::moveCaretAndSelection(const BufferCoord &ptBefore, const BufferCoord &ptAfter, bool isSelection)
{
    if (mOptions.testFlag(EditorOption::eoGroupUndo)) {
        mUndoList->addGroupBreak();
    }

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
    if (mOptions.testFlag(EditorOption::eoEnhanceHomeKey)) {
        QString s = mDocument->getString(mCaretY - 1);

        int first_nonblank = 0;
        int vMaxX = s.length();
        while ((first_nonblank < vMaxX) && (s[first_nonblank] == ' ' || s[first_nonblank] == '\t')) {
            first_nonblank++;
        }
        newX = mCaretX;

        if ((newX > first_nonblank+1)
                || (newX == 1))
            newX = first_nonblank+1;
        else
            newX = 1;
    } else
        newX = 1;
    moveCaretAndSelection(caretXY(), BufferCoord{newX, mCaretY}, isSelection);
}

void SynEdit::moveCaretToLineEnd(bool isSelection)
{
    int vNewX;
    if (mOptions.testFlag(EditorOption::eoEnhanceEndKey)) {
        QString vText = displayLineText();
        int vLastNonBlank = vText.length()-1;
        int vMinX = 0;
        while ((vLastNonBlank >= vMinX) && (vText[vLastNonBlank] == ' ' || vText[vLastNonBlank] =='\t'))
            vLastNonBlank--;
        vLastNonBlank++;
        vNewX = mCaretX;
        if ((vNewX <= vLastNonBlank) || (vNewX == vText.length() + 1))
            vNewX = vLastNonBlank + 1;
        else
            vNewX = vText.length() + 1;
    } else
        vNewX = displayLineText().length() + 1;

    moveCaretAndSelection(caretXY(), BufferCoord{vNewX, mCaretY}, isSelection);
}

void SynEdit::doGotoBlockStart(bool isSelection)
{
    if (mCaretY<0 || mCaretY>document()->count())
        return;
    HighlighterState state = document()->ranges(mCaretY-1);
    //todo: handle block other than {}
    if (document()->braceLevels(mCaretY-1)==0) {
        doGotoEditorStart(isSelection);
    } else if (document()->leftBraces(mCaretY-1)==0){
        int line=mCaretY-1;
        while (line>=1) {
            if (document()->leftBraces(line-1)>document()->rightBraces(line-1)) {
                moveCaretVert(line+1-mCaretY, isSelection);
                moveCaretToLineStart(isSelection);
                setTopLine(line-1);
                return;
            }
            line--;
        }
    }
}

void SynEdit::doGotoBlockEnd(bool isSelection)
{
    if (mCaretY<0 || mCaretY>document()->count())
        return;
    HighlighterState state = document()->ranges(mCaretY-1);
    //todo: handle block other than {}
    if (document()->braceLevels(mCaretY-1)==0) {
        doGotoEditorEnd(isSelection);
    } else if (document()->rightBraces(mCaretY-1)==0){
        int line=mCaretY+1;
        while (line<=document()->count()) {
            if (document()->rightBraces(line-1)>document()->leftBraces(line-1)) {
                moveCaretVert(line-1-mCaretY, isSelection);
                moveCaretToLineStart(isSelection);
                setTopLine(line-mLinesInWindow+1);
                return;
            }
            line++;
        }
    }
}

void SynEdit::doGotoEditorStart(bool isSelection)
{
    moveCaretVert(1-mCaretY, isSelection);
    moveCaretToLineStart(isSelection);
}

void SynEdit::doGotoEditorEnd(bool isSelection)
{
    if (!mDocument->empty()) {
        moveCaretVert(mDocument->count()-mCaretY, isSelection);
        moveCaretToLineEnd(isSelection);
    }
}

void SynEdit::setSelectedTextEmpty()
{
    BufferCoord startPos=blockBegin();
    BufferCoord endPos=blockEnd();
    doDeleteText(startPos,endPos,mActiveSelectionMode);
    internalSetCaretXY(startPos);
}

void SynEdit::setSelTextPrimitive(const QStringList &text)
{
    setSelTextPrimitiveEx(mActiveSelectionMode, text);
}

void SynEdit::setSelTextPrimitiveEx(SelectionMode mode, const QStringList &text)
{
    incPaintLock();
    bool groupUndo=false;
    BufferCoord startPos = blockBegin();
    BufferCoord endPos = blockEnd();
    if (selAvail()) {
        if (!mUndoing && !text.isEmpty()) {
            mUndoList->beginBlock();
            groupUndo=true;
        }
        doDeleteText(startPos,endPos,activeSelectionMode());
        if (mode == SelectionMode::Column) {
            int colBegin = charToColumn(startPos.line,startPos.ch);
            int colEnd = charToColumn(endPos.line,endPos.ch);
            if (colBegin<colEnd)
                internalSetCaretXY(startPos);
            else
                internalSetCaretXY(endPos);
        } else
            internalSetCaretXY(startPos);
    }
    if (!text.isEmpty()) {
        doInsertText(caretXY(),text,mode,mBlockBegin.line,mBlockEnd.line);
    }
    if (groupUndo) {
        mUndoList->endBlock();
    }
    decPaintLock();
    setStatusChanged(StatusChange::scSelection);
}

void SynEdit::doSetSelText(const QString &value)
{
    bool blockBeginned = false;
    auto action = finally([this, &blockBeginned]{
        if (blockBeginned)
            mUndoList->endBlock();
    });
    if (selAvail()) {
      mUndoList->beginBlock();
      blockBeginned = true;
//      mUndoList->AddChange(
//                  SynChangeReason::crDelete, mBlockBegin, mBlockEnd,
//                  selText(), mActiveSelectionMode);
    }
//    } else if (!colSelAvail())
//        setActiveSelectionMode(selectionMode());
    BufferCoord StartOfBlock = blockBegin();
    BufferCoord EndOfBlock = blockEnd();
    mBlockBegin = StartOfBlock;
    mBlockEnd = EndOfBlock;
    setSelTextPrimitive(splitStrings(value));
}

int SynEdit::searchReplace(const QString &sSearch, const QString &sReplace, SearchOptions sOptions, PSynSearchBase searchEngine,
                    SearchMathedProc matchedCallback, SearchConfirmAroundProc confirmAroundCallback)
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
        if (mActiveSelectionMode == SelectionMode::Line) {
            ptStart.ch = 1;
            ptEnd.ch = mDocument->getString(ptEnd.line - 1).length();
        } else if (mActiveSelectionMode == SelectionMode::Column) {
            // make sure the start column is smaller than the end column
            if (ptStart.ch > ptEnd.ch)
                std::swap(ptStart.ch,ptEnd.ch);
        }
        // ignore the cursor position when searching in the selection
        if (bBackward) {
            ptCurrent = ptEnd;
        } else {
            ptCurrent = ptStart;
        }
    } else {
        ptStart.ch = 1;
        ptStart.line = 1;
        ptEnd.line = mDocument->count();
        ptEnd.ch = mDocument->getString(ptEnd.line - 1).length();
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
    BufferCoord originCaretXY=caretXY();
    // initialize the search engine
    searchEngine->setOptions(sOptions);
    searchEngine->setPattern(sSearch);
    // search while the current search position is inside of the search range
    bool dobatchReplace = false;
    {
        auto action = finally([&,this]{
            if (dobatchReplace) {
                decPaintLock();
                mUndoList->endBlock();
            }
        });
        int i;
        // If it's a search only we can leave the procedure now.
        SearchAction searchAction = SearchAction::Exit;
        while ((ptCurrent.line >= ptStart.line) && (ptCurrent.line <= ptEnd.line)) {
            int nInLine = searchEngine->findAll(mDocument->getString(ptCurrent.line - 1));
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
                if ((mActiveSelectionMode == SelectionMode::Normal)
                        || !sOptions.testFlag(ssoSelectedOnly)) {
                    if (((ptCurrent.line == ptStart.line) && (first < ptStart.ch)) ||
                            ((ptCurrent.line == ptEnd.line) && (last > ptEnd.ch)))
                        isInValidSearchRange = false;
                } else if (mActiveSelectionMode == SelectionMode::Column) {
                    // solves bug in search/replace when smColumn mode active and no selection
                    isInValidSearchRange = ((first >= ptStart.ch) && (last <= ptEnd.ch))
                            || (ptEnd.ch - ptStart.ch < 1);
                }
                if (!isInValidSearchRange)
                    continue;
                result++;
                // Select the text, so the user can see it in the OnReplaceText event
                // handler or as the search result.
                ptCurrent.ch = nFound;
                setBlockBegin(ptCurrent);

                //Be sure to use the Ex version of CursorPos so that it appears in the middle if necessary
                setCaretXYEx(false, BufferCoord{ptCurrent.ch, ptCurrent.line});
                ensureCursorPosVisibleEx(true);
                ptCurrent.ch += nSearchLen;
                setBlockEnd(ptCurrent);

                QString replaceText = searchEngine->replace(selText(), sReplace);
                if (searchAction==SearchAction::ReplaceAndExit) {
                    searchAction=SearchAction::Exit;
                } else if (matchedCallback && !dobatchReplace) {
                    searchAction = matchedCallback(sSearch,replaceText,ptCurrent.line,
                                    nFound,nSearchLen);
                }
                if (searchAction==SearchAction::Exit) {
                    return result;
                } else if (searchAction == SearchAction::Skip) {
                    continue;
                } else if (searchAction == SearchAction::Replace
                           || searchAction == SearchAction::ReplaceAndExit
                           || searchAction == SearchAction::ReplaceAll) {
                    if (!dobatchReplace &&
                            (searchAction == SearchAction::ReplaceAll) ){
                        incPaintLock();
                        mUndoList->beginBlock();
                        dobatchReplace = true;
                    }
                    bool oldAutoIndent = mOptions.testFlag(EditorOption::eoAutoIndent);
                    mOptions.setFlag(EditorOption::eoAutoIndent,false);
                    doSetSelText(replaceText);
                    nReplaceLen = caretX() - nFound;
                    // fix the caret position and the remaining results
                    if (!bBackward) {
                        internalSetCaretX(nFound + nReplaceLen);
                        if ((nSearchLen != nReplaceLen)) {
                            iResultOffset += nReplaceLen - nSearchLen;
                            if ((mActiveSelectionMode != SelectionMode::Column) && (caretY() == ptEnd.line)) {
                                ptEnd.ch+=nReplaceLen - nSearchLen;
                                setBlockEnd(ptEnd);
                            }
                        }
                    }
                    mOptions.setFlag(EditorOption::eoAutoIndent,oldAutoIndent);
                }
            }

            // search next / previous line
            if (bBackward)
                ptCurrent.line--;
            else
                ptCurrent.line++;
            if (((ptCurrent.line < ptStart.line) || (ptCurrent.line > ptEnd.line))
                    && bFromCursor ){
                if (!sOptions.testFlag(ssoWrapAround) && confirmAroundCallback && !confirmAroundCallback())
                    break;
                //search start from cursor, search has finished but no result founds
                bFromCursor = false;
                ptStart.ch = 1;
                ptStart.line = 1;
                ptEnd.line = mDocument->count();
                ptEnd.ch = mDocument->getString(ptEnd.line - 1).length();
                if (bBackward) {
                    ptStart = originCaretXY;
                    ptCurrent = ptEnd;
                } else {
                    ptEnd= originCaretXY;
                    ptCurrent = ptStart;
                }
            }
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

void SynEdit::properSetLine(int ALine, const QString &ALineText, bool notify)
{
    if (mOptions.testFlag(eoTrimTrailingSpaces)) {
        mDocument->putString(ALine,trimRight(ALineText),notify);
    } else {
        mDocument->putString(ALine,ALineText,notify);
    }
}

void SynEdit::doDeleteText(BufferCoord startPos, BufferCoord endPos, SelectionMode mode)
{
    bool UpdateMarks = false;
    int MarkOffset = 0;
    if (mode == SelectionMode::Normal) {
        PCodeFoldingRange foldRange = foldStartAtLine(endPos.line);
        QString s = mDocument->getString(endPos.line-1);
        if ((foldRange) && foldRange->collapsed && endPos.ch>s.length()) {
            QString newS=s+highlighter()->foldString();
            if ((startPos.ch<=s.length() || startPos.line<endPos.line)
                    && endPos.ch>newS.length() ) {
                //selection has whole block
                endPos.line = foldRange->toLine;
                endPos.ch = mDocument->getString(endPos.line-1).length()+1;
            } else {
                return;
            }
        }
    }
    QStringList deleted=getContent(startPos,endPos,mode);
    switch(mode) {
    case SelectionMode::Normal:
        if (mDocument->count() > 0) {
            // Create a string that contains everything on the first line up
            // to the selection mark, and everything on the last line after
            // the selection mark.
            QString TempString = mDocument->getString(startPos.line - 1).mid(0, startPos.ch - 1)
                + mDocument->getString(endPos.line - 1).mid(endPos.ch-1);
//            bool collapsed=foldCollapsedBetween(BB.Line,BE.Line);
            // Delete all lines in the selection range.
            mDocument->deleteLines(startPos.line, endPos.line - startPos.line);
            properSetLine(startPos.line-1,TempString);
            UpdateMarks = true;
            internalSetCaretXY(startPos);
//            if (collapsed) {
//                PSynEditFoldRange foldRange = foldStartAtLine(BB.Line);
//                if (!foldRange
//                        || (!foldRange->collapsed))
//                    uncollapseAroundLine(BB.Line);
//            }
        }
        break;
    case SelectionMode::Column:
    {
        int First = startPos.line - 1;
        int ColFrom = charToColumn(startPos.line, startPos.ch);
        int Last = endPos.line - 1;
        int ColTo = charToColumn(endPos.line, endPos.ch);
        if (ColFrom > ColTo)
            std::swap(ColFrom, ColTo);
        if (First > Last)
            std::swap(First,Last);
        QString result;
        for (int i = First; i <= Last; i++) {
            int l = columnToChar(i+1,ColFrom);
            int r = columnToChar(i+1,ColTo-1)+1;
            QString s = mDocument->getString(i);
            s.remove(l-1,r-l);
            properSetLine(i,s);
        }
        // Lines never get deleted completely, so keep caret at end.
        internalSetCaretXY(startPos);
        // Column deletion never removes a line entirely, so no mark
        // updating is needed here.
        break;
    }
    case SelectionMode::Line:
        if (endPos.line == mDocument->count()) {
            mDocument->putString(endPos.line - 1,"");
            mDocument->deleteLines(startPos.line-1,endPos.line-startPos.line);
        } else {
            mDocument->deleteLines(startPos.line-1,endPos.line-startPos.line+1);
        }
        // smLine deletion always resets to first column.
        internalSetCaretXY(BufferCoord{1, startPos.line});
        UpdateMarks = true;
        MarkOffset = 1;
        break;
    }
    // Update marks
    if (UpdateMarks)
        doLinesDeleted(startPos.line, endPos.line - startPos.line + MarkOffset);
    if (!mUndoing) {
        mUndoList->addChange(ChangeReason::Delete,
                             startPos,
                             endPos,
                             deleted,
                             mode);
    }
}

void SynEdit::doInsertText(const BufferCoord& pos,
                           const QStringList& text,
                           SelectionMode mode, int startLine, int endLine) {
    if (text.isEmpty())
        return;
    if (startLine>endLine)
        std::swap(startLine,endLine);

    if (mode == SelectionMode::Normal) {
        PCodeFoldingRange foldRange = foldStartAtLine(pos.line);
        QString s = mDocument->getString(pos.line-1);
        if ((foldRange) && foldRange->collapsed && pos.ch>s.length()+1)
            return;
    }
    int insertedLines = 0;
    BufferCoord newPos;
    switch(mode){
    case SelectionMode::Normal:
        insertedLines = doInsertTextByNormalMode(pos,text, newPos);
        doLinesInserted(pos.line+1, insertedLines);
        break;
    case SelectionMode::Column:
        insertedLines = doInsertTextByColumnMode(pos,text, newPos, startLine,endLine);
        doLinesInserted(endLine-insertedLines+1,insertedLines);
        break;
    case SelectionMode::Line:
        insertedLines = doInsertTextByLineMode(pos,text, newPos);
        doLinesInserted(pos.line, insertedLines);
        break;
    }
    internalSetCaretXY(newPos);
    setBlockBegin(newPos);
    ensureCursorPosVisible();
}

int SynEdit::doInsertTextByNormalMode(const BufferCoord& pos, const QStringList& text, BufferCoord &newPos)
{
    QString sLeftSide;
    QString sRightSide;
    QString str;
    bool bChangeScroll;
//    int SpaceCount;
    int result = 0;
    int startLine = pos.line;
    QString line=mDocument->getString(pos.line-1);
    sLeftSide = line.mid(0, pos.ch - 1);
    if (pos.ch - 1 > sLeftSide.length()) {
        if (stringIsBlank(sLeftSide))
            sLeftSide = GetLeftSpacing(displayX() - 1, true);
        else
            sLeftSide += QString(pos.ch - 1 - sLeftSide.length(),' ');
    }
    sRightSide = line.mid(pos.ch - 1);
//    if (mUndoing) {
//        SpaceCount = 0;
//    } else {
//        SpaceCount = leftSpaces(sLeftSide);
//    }
    int caretY=pos.line;
    // step1: insert the first line of Value into current line
    if (text.length()>1) {
        if (!mUndoing && mHighlighter && mHighlighter->language()==HighlighterLanguage::Cpp && mOptions.testFlag(eoAutoIndent)) {
            QString s = trimLeft(text[0]);
            if (sLeftSide.isEmpty()) {
                sLeftSide = GetLeftSpacing(calcIndentSpaces(caretY,s,true),true);
            }
            str = sLeftSide + s;
        } else
            str = sLeftSide + text[0];
        properSetLine(caretY - 1, str);
        mDocument->insertLines(caretY, text.length()-1);
    } else {
        str = sLeftSide + text[0] + sRightSide;
        properSetLine(caretY - 1, str);
    }
    rescanRange(caretY);
    // step2: insert remaining lines of Value
    for (int i=1;i<text.length();i++) {
        bool notInComment = true;
//        if (mHighlighter) {
//            notInComment = !mHighlighter->isLastLineCommentNotFinished(
//                    mHighlighter->getRangeState().state)
//                && !mHighlighter->isLastLineStringNotFinished(
//                    mHighlighter->getRangeState().state);
//        }
        caretY=pos.line+i;
//        mStatusChanges.setFlag(SynStatusChange::scCaretY);
        if (text[i].isEmpty()) {
            if (i==text.length()-1) {
                str = sRightSide;
            } else {
                if (!mUndoing && mHighlighter && mHighlighter->language()==HighlighterLanguage::Cpp && mOptions.testFlag(eoAutoIndent) && notInComment) {
                    str = GetLeftSpacing(calcIndentSpaces(caretY,"",true),true);
                } else {
                    str = "";
                }
            }
        } else {
            str = text[i];
            if (i==text.length()-1)
                str += sRightSide;
            if (!mUndoing && mHighlighter && mHighlighter->language()==HighlighterLanguage::Cpp && mOptions.testFlag(eoAutoIndent) && notInComment) {
                int indentSpaces = calcIndentSpaces(caretY,str,true);
                str = GetLeftSpacing(indentSpaces,true)+trimLeft(str);
            }
        }
        properSetLine(caretY - 1, str,false);
        rescanRange(caretY);
        result++;
    }
    bChangeScroll = !mOptions.testFlag(eoScrollPastEol);
    mOptions.setFlag(eoScrollPastEol);
    auto action = finally([&,this]{
        if (bChangeScroll)
            mOptions.setFlag(eoScrollPastEol,false);
    });
    if (mOptions.testFlag(eoTrimTrailingSpaces) && (sRightSide == "")) {
        newPos=BufferCoord{mDocument->getString(caretY-1).length()+1,caretY};
    } else
        newPos=BufferCoord{str.length() - sRightSide.length()+1,caretY};
    onLinesPutted(startLine-1,result+1);
    if (!mUndoing) {
        mUndoList->addChange(
                    ChangeReason::Insert,
                    pos,newPos,
                    QStringList(),SelectionMode::Normal);
    }
    return result;
}

int SynEdit::doInsertTextByColumnMode(const BufferCoord& pos, const QStringList& text, BufferCoord &newPos, int startLine, int endLine)
{
    QString str;
    QString tempString;
    int line;
    int len;
    BufferCoord  lineBreakPos;
    int result = 0;
    DisplayCoord insertCoord = bufferToDisplayPos(caretXY());
    int insertCol = insertCoord.Column;
    line = startLine;
    if (!mUndoing) {
        mUndoList->beginBlock();
    }
    int i=0;
    while(line<=endLine) {
        str = text[i];
        int insertPos = 0;
        if (line > mDocument->count()) {
            result++;
            tempString = QString(insertCol - 1,' ') + str;
            mDocument->add("");
            if (!mUndoing) {
                result++;
                lineBreakPos.line = line - 1;
                lineBreakPos.ch = mDocument->getString(line - 2).length() + 1;
                mUndoList->addChange(ChangeReason::LineBreak,
                                 lineBreakPos,
                                 lineBreakPos,
                                 QStringList(), SelectionMode::Normal);
            }
        } else {
            tempString = mDocument->getString(line - 1);
            len = stringColumns(tempString,0);
            if (len < insertCol) {
                insertPos = tempString.length()+1;
                tempString = tempString + QString(insertCol - len - 1,' ') + str;
            } else {
                insertPos = columnToChar(line,insertCol);
                tempString.insert(insertPos-1,str);
            }
        }
        properSetLine(line - 1, tempString);
        // Add undo change here from PasteFromClipboard
        if (!mUndoing) {
            mUndoList->addChange(
                        ChangeReason::Insert,
                        BufferCoord{insertPos, line},
                        BufferCoord{insertPos+str.length(), line},
                        QStringList(),
                        SelectionMode::Normal);
        }
        if (i<text.length()-1) {
            i++;
        }
        line++;
    }
    newPos=pos;
    if (!text[0].isEmpty()) {
        newPos.ch+=text[0].length();
//        mCaretX+=firstLineLen;
//        mStatusChanges.setFlag(SynStatusChange::scCaretX);
    }
    if (!mUndoing) {
        mUndoList->endBlock();
    }
    return result;
}

int SynEdit::doInsertTextByLineMode(const BufferCoord& pos, const QStringList& text, BufferCoord &newPos)
{
    QString Str;
    int Result = 0;
    newPos=pos;
    newPos.ch=1;
//    mCaretX = 1;
//    emit statusChanged(SynStatusChange::scCaretX);
    // Insert string before current line
    for (int i=0;i<text.length();i++) {
        if ((mCaretY == mDocument->count()) || mInserting) {
            mDocument->insert(mCaretY - 1, "");
            Result++;
        }
        properSetLine(mCaretY - 1, Str);
        newPos.line++;
//        mCaretY++;
//        mStatusChanges.setFlag(SynStatusChange::scCaretY);
    }
    if (!mUndoing) {
        mUndoList->addChange(
                    ChangeReason::Insert,
                    BufferCoord{1,pos.line},newPos,
                    QStringList(),SelectionMode::Line);
    }
    return Result;
}

void SynEdit::deleteFromTo(const BufferCoord &start, const BufferCoord &end)
{
    if (mReadOnly)
        return;
    if ((start.ch != end.ch) || (start.line != end.line)) {
        mUndoList->beginBlock();
        addCaretToUndo();
        addSelectionToUndo();
        setBlockBegin(start);
        setBlockEnd(end);
        doDeleteText(start,end,SelectionMode::Normal);
        mUndoList->endBlock();
        internalSetCaretXY(start);
    }
}

bool SynEdit::onGetSpecialLineColors(int, QColor &, QColor &)
{
    return false;
}

void SynEdit::onGetEditingAreas(int, EditingAreaList &)
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
                                           PHighlighterAttribute , FontStyles &, QColor &, QColor &)
{

}

void SynEdit::onProcessCommand(EditCommand , QChar , void *)
{

}

void SynEdit::onCommandProcessed(EditCommand , QChar , void *)
{

}

void SynEdit::executeCommand(EditCommand command, QChar ch, void *pData)
{
    hideCaret();
    incPaintLock();

    auto action=finally([this] {
        decPaintLock();
        showCaret();
    });
    switch(command) {
    //horizontal caret movement or selection
    case EditCommand::ecLeft:
    case EditCommand::ecSelLeft:
        moveCaretHorz(-1, command == EditCommand::ecSelLeft);
        break;
    case EditCommand::ecRight:
    case EditCommand::ecSelRight:
        moveCaretHorz(1, command == EditCommand::ecSelRight);
        break;
    case EditCommand::ecPageLeft:
    case EditCommand::ecSelPageLeft:
        moveCaretHorz(-mCharsInWindow, command == EditCommand::ecSelPageLeft);
        break;
    case EditCommand::ecPageRight:
    case EditCommand::ecSelPageRight:
        moveCaretHorz(mCharsInWindow, command == EditCommand::ecSelPageRight);
        break;
    case EditCommand::ecLineStart:
    case EditCommand::ecSelLineStart:
        moveCaretToLineStart(command == EditCommand::ecSelLineStart);
        break;
    case EditCommand::ecLineEnd:
    case EditCommand::ecSelLineEnd:
        moveCaretToLineEnd(command == EditCommand::ecSelLineEnd);
        break;
    // vertical caret movement or selection
    case EditCommand::ecUp:
    case EditCommand::ecSelUp:
        moveCaretVert(-1, command == EditCommand::ecSelUp);
        break;
    case EditCommand::ecDown:
    case EditCommand::ecSelDown:
        moveCaretVert(1, command == EditCommand::ecSelDown);
        break;
    case EditCommand::ecPageUp:
    case EditCommand::ecSelPageUp:
    case EditCommand::ecPageDown:
    case EditCommand::ecSelPageDown:
    {
        int counter = mLinesInWindow;
        if (mOptions.testFlag(eoHalfPageScroll))
            counter /= 2;
        if (mOptions.testFlag(eoScrollByOneLess)) {
            counter -=1;
        }
        if (counter<0)
            break;
        if (command == EditCommand::ecPageUp || command == EditCommand::ecSelPageUp) {
            counter = -counter;
        }
        moveCaretVert(counter, command == EditCommand::ecSelPageUp || command == EditCommand::ecSelPageDown);
        break;
    }
    case EditCommand::ecPageTop:
    case EditCommand::ecSelPageTop:
        moveCaretVert(mTopLine-mCaretY, command == EditCommand::ecSelPageTop);
        break;
    case EditCommand::ecPageBottom:
    case EditCommand::ecSelPageBottom:
        moveCaretVert(mTopLine+mLinesInWindow-1-mCaretY, command == EditCommand::ecSelPageBottom);
        break;
    case EditCommand::ecEditorStart:
    case EditCommand::ecSelEditorStart:
        doGotoEditorStart(command == EditCommand::ecSelEditorStart);
        break;
    case EditCommand::ecEditorEnd:
    case EditCommand::ecSelEditorEnd:
        doGotoEditorEnd(command == EditCommand::ecSelEditorEnd);
        break;
    case EditCommand::ecBlockStart:
    case EditCommand::ecSelBlockStart:
        doGotoBlockStart(command == EditCommand::ecSelBlockStart);
        break;
    case EditCommand::ecBlockEnd:
    case EditCommand::ecSelBlockEnd:
        doGotoBlockEnd(command == EditCommand::ecSelBlockEnd);
        break;
    // goto special line / column position
    case EditCommand::ecGotoXY:
    case EditCommand::ecSelGotoXY:
        if (pData)
            moveCaretAndSelection(caretXY(), *((BufferCoord *)(pData)), command == EditCommand::ecSelGotoXY);
        break;
    // word selection
    case EditCommand::ecWordLeft:
    case EditCommand::ecSelWordLeft:
    {
        BufferCoord CaretNew = prevWordPos();
        moveCaretAndSelection(caretXY(), CaretNew, command == EditCommand::ecSelWordLeft);
        break;
    }
    case EditCommand::ecWordRight:
    case EditCommand::ecSelWordRight:
    {
        BufferCoord CaretNew = nextWordPos();
        moveCaretAndSelection(caretXY(), CaretNew, command == EditCommand::ecSelWordRight);
        break;
    }
    case EditCommand::ecSelWord:
        setSelWord();
        break;
    case EditCommand::ecSelectAll:
        doSelectAll();
        break;
    case EditCommand::ecExpandSelection:
        doExpandSelection(caretXY());
        break;
    case EditCommand::ecShrinkSelection:
        doShrinkSelection(caretXY());
        break;
    case EditCommand::ecDeleteLastChar:
        doDeleteLastChar();
        break;
    case EditCommand::ecDeleteChar:
        doDeleteCurrentChar();
        break;
    case EditCommand::ecDeleteWord:
        doDeleteWord();
        break;
    case EditCommand::ecDeleteEOL:
        doDeleteToEOL();
        break;
    case EditCommand::ecDeleteWordStart:
        doDeleteToWordStart();
        break;
    case EditCommand::ecDeleteWordEnd:
        doDeleteToWordEnd();
        break;
    case EditCommand::ecDeleteBOL:
        doDeleteFromBOL();
        break;
    case EditCommand::ecDeleteLine:
        doDeleteLine();
        break;
    case EditCommand::ecDuplicateLine:
        doDuplicateLine();
        break;
    case EditCommand::ecMoveSelUp:
        doMoveSelUp();
        break;
    case EditCommand::ecMoveSelDown:
        doMoveSelDown();
        break;
    case EditCommand::ecClearAll:
        clearAll();
        break;
    case EditCommand::ecInsertLine:
        insertLine(false);
        break;
    case EditCommand::ecLineBreak:
        insertLine(true);
        break;
    case EditCommand::ecLineBreakAtEnd:
        mUndoList->beginBlock();
        addCaretToUndo();
        addSelectionToUndo();
        moveCaretToLineEnd(false);
        insertLine(true);
        mUndoList->endBlock();
        break;
    case EditCommand::ecTab:
        doTabKey();
        break;
    case EditCommand::ecShiftTab:
        doShiftTabKey();
        break;
    case EditCommand::ecChar:
        doAddChar(ch);
        break;
    case EditCommand::ecInsertMode:
        if (!mReadOnly)
            setInsertMode(true);
        break;
    case EditCommand::ecOverwriteMode:
        if (!mReadOnly)
            setInsertMode(false);
        break;
    case EditCommand::ecToggleMode:
        if (!mReadOnly) {
            setInsertMode(!mInserting);
        }
        break;
    case EditCommand::ecCut:
        if (!mReadOnly)
            doCutToClipboard();
        break;
    case EditCommand::ecCopy:
        doCopyToClipboard();
        break;
    case EditCommand::ecPaste:
        if (!mReadOnly)
            doPasteFromClipboard();
        break;
    case EditCommand::ecImeStr:
    case EditCommand::ecString:
        if (!mReadOnly)
            doAddStr(*((QString*)pData));
        break;
    case EditCommand::ecUndo:
        if (!mReadOnly)
            doUndo();
        break;
    case EditCommand::ecRedo:
        if (!mReadOnly)
            doRedo();
        break;
    case EditCommand::ecZoomIn:
        doZoomIn();
        break;
    case EditCommand::ecZoomOut:
        doZoomOut();
        break;
    case EditCommand::ecComment:
        doComment();
        break;
    case EditCommand::ecUncomment:
        doUncomment();
        break;
    case EditCommand::ecToggleComment:
        doToggleComment();
        break;
    case EditCommand::ecToggleBlockComment:
        doToggleBlockComment();
        break;
    case EditCommand::ecNormalSelect:
        setSelectionMode(SelectionMode::Normal);
        break;
    case EditCommand::ecLineSelect:
        setSelectionMode(SelectionMode::Line);
        break;
    case EditCommand::ecColumnSelect:
        setSelectionMode(SelectionMode::Column);
        break;
    case EditCommand::ecScrollLeft:
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()-mMouseWheelScrollSpeed);
        break;
    case EditCommand::ecScrollRight:
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()+mMouseWheelScrollSpeed);
        break;
    case EditCommand::ecScrollUp:
        verticalScrollBar()->setValue(verticalScrollBar()->value()-mMouseWheelScrollSpeed);
        break;
    case EditCommand::ecScrollDown:
        verticalScrollBar()->setValue(verticalScrollBar()->value()+mMouseWheelScrollSpeed);
        break;
    case EditCommand::ecMatchBracket:
        {
        BufferCoord coord = getMatchingBracket();
        if (coord.ch!=0 && coord.line!=0)
            internalSetCaretXY(coord);
        }
        break;
    default:
        break;
    }


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

void SynEdit::setRainbowAttrs(const PHighlighterAttribute &attr0, const PHighlighterAttribute &attr1, const PHighlighterAttribute &attr2, const PHighlighterAttribute &attr3)
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

bool SynEdit::isCaretVisible()
{
    if (mCaretY < mTopLine)
        return false;
    if (mCaretY >= mTopLine + mLinesInWindow )
        return false;
    if (mCaretX < mLeftChar)
        return false;
    if (mCaretX >= mLeftChar + mCharsInWindow)
        return false;
    return true;
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
        QRectF cacheRC;
        qreal dpr = mContentImage->devicePixelRatioF();
        cacheRC.setLeft(rcClip.left()*dpr);
        cacheRC.setTop(rcClip.top()*dpr);
        cacheRC.setWidth(rcClip.width()*dpr);
        cacheRC.setHeight(rcClip.height()*dpr);
        painter.drawImage(rcCaret,*mContentImage,cacheRC);
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
        QRectF cacheRC;
        qreal dpr = mContentImage->devicePixelRatioF();
        cacheRC.setLeft(rcClip.left()*dpr);
        cacheRC.setTop(rcClip.top()*dpr);
        cacheRC.setWidth(rcClip.width()*dpr);
        cacheRC.setHeight(rcClip.height()*dpr);
        painter.drawImage(rcClip,*mContentImage,cacheRC);
    }
    paintCaret(painter, rcCaret);
}

void SynEdit::resizeEvent(QResizeEvent *)
{
    //resize the cache image
    qreal dpr = devicePixelRatioF();
    mContentImage = std::make_shared<QImage>(clientWidth()*dpr,clientHeight()*dpr,
                                                            QImage::Format_ARGB32);
    mContentImage->setDevicePixelRatio(dpr);
//    QRect newRect = image->rect().intersected(mContentImage->rect());

//    QPainter painter(image.get());

    //painter.drawImage(newRect,*mContentImage);

//    mContentImage = image;

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
        if (mDocument)
            mDocument->setFontMetrics(font(),mFontForNonAscii);
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
    if (event->key() == Qt::Key_Escape && mActiveSelectionMode != mSelectionMode) {
        setActiveSelectionMode(selectionMode());
        setBlockBegin(caretXY());
        setBlockEnd(caretXY());
        event->accept();
    } else {
        EditCommand cmd=TranslateKeyCode(event->key(),event->modifiers());
        if (cmd!=EditCommand::ecNone) {
            commandProcessor(cmd,QChar(),nullptr);
            event->accept();
        } else if (!event->text().isEmpty()) {
            QChar c = event->text().at(0);
            if (c=='\t' || c.isPrint()) {
                commandProcessor(EditCommand::ecChar,c,nullptr);
                event->accept();
            }
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
    Qt::MouseButton button = event->button();
    int X=event->pos().x();
    int Y=event->pos().y();

    QAbstractScrollArea::mousePressEvent(event);


    BufferCoord oldCaret=caretXY();
    if (button == Qt::RightButton) {
        if (mOptions.testFlag(eoRightMouseMovesCursor) &&
                ( (selAvail() && ! isPointInSelection(displayToBufferPos(pixelsToRowColumn(X, Y))))
                  || ! selAvail())) {
            invalidateSelection();
            //mBlockEnd=mBlockBegin;
            computeCaret();
        }else {
            return;
        }
    } else if (button == Qt::LeftButton) {
        if (selAvail()) {
            //remember selection state, as it will be cleared later
            bWasSel = true;
            mMouseDownPos = event->pos();
        }
        computeCaret();
        mStateFlags.setFlag(StateFlag::sfWaitForDragging,false);
        if (bWasSel && mOptions.testFlag(eoDragDropEditing) && (X >= mGutterWidth + 2)
                && (mSelectionMode == SelectionMode::Normal) && isPointInSelection(displayToBufferPos(pixelsToRowColumn(X, Y))) ) {
            bStartDrag = true;
        }
        if (bStartDrag && !mReadOnly) {
            mStateFlags.setFlag(StateFlag::sfWaitForDragging);
        } else {
            if (event->modifiers() == Qt::ShiftModifier) {
                //BlockBegin and BlockEnd are restored to their original position in the
                //code from above and SetBlockEnd will take care of proper invalidation
                setBlockEnd(caretXY());
            } else if (mOptions.testFlag(eoAltSetsColumnMode) &&
                     (mActiveSelectionMode != SelectionMode::Line)) {
                if (event->modifiers() == Qt::AltModifier && !mReadOnly)
                    setActiveSelectionMode(SelectionMode::Column);
                else
                    setActiveSelectionMode(selectionMode());
                //Selection mode must be set before calling SetBlockBegin
                setBlockBegin(caretXY());
            }
            computeScroll(false);
        }
    }
    if (oldCaret!=caretXY()) {
        if (mOptions.testFlag(EditorOption::eoGroupUndo))
            mUndoList->addGroupBreak();
    }
}

void SynEdit::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseReleaseEvent(event);
    int X=event->pos().x();
    /* int Y=event->pos().y(); */

    if (!mMouseMoved && (X < mGutterWidth + 2)) {
        processGutterClick(event);
    }

    BufferCoord oldCaret=caretXY();
    if (mStateFlags.testFlag(StateFlag::sfWaitForDragging) &&
            !mStateFlags.testFlag(StateFlag::sfDblClicked)) {
        computeCaret();
        if (! (event->modifiers() & Qt::ShiftModifier))
            setBlockBegin(caretXY());
        setBlockEnd(caretXY());
        mStateFlags.setFlag(StateFlag::sfWaitForDragging, false);
    }
    mStateFlags.setFlag(StateFlag::sfDblClicked,false);
    if (oldCaret!=caretXY()) {
        if (mOptions.testFlag(EditorOption::eoGroupUndo))
            mUndoList->addGroupBreak();
    }
}

void SynEdit::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseMoveEvent(event);
    mMouseMoved = true;
    Qt::MouseButtons buttons = event->buttons();
    if (mStateFlags.testFlag(StateFlag::sfWaitForDragging)
            && !mReadOnly) {
        if ( ( event->pos() - mMouseDownPos).manhattanLength()>=QApplication::startDragDistance()) {
            mStateFlags.setFlag(StateFlag::sfWaitForDragging,false);
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData;

            mimeData->setText(selText());
            drag->setMimeData(mimeData);

            drag->exec(Qt::CopyAction | Qt::MoveAction);
        }
    } else if (buttons == Qt::LeftButton) {
        if (mOptions.testFlag(eoAltSetsColumnMode) &&
                (mActiveSelectionMode != SelectionMode::Line) ) {
                if (event->modifiers() == Qt::AltModifier && !mReadOnly)
                    setActiveSelectionMode(SelectionMode::Column);
                else
                    setActiveSelectionMode(selectionMode());
        }
    } else if (buttons == Qt::NoButton) {
        updateMouseCursor();
    }
}

void SynEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseDoubleClickEvent(event);
    QPoint ptMouse = event->pos();
    if (ptMouse.x() >= mGutterWidth + 2) {
          setSelWord();
          mStateFlags.setFlag(StateFlag::sfDblClicked);
    }
}

void SynEdit::inputMethodEvent(QInputMethodEvent *event)
{
//    qDebug()<<event->replacementStart()<<":"<<event->replacementLength()<<" - "
//           << event->preeditString()<<" - "<<event->commitString();

    QString oldString = mInputPreeditString;
    mInputPreeditString = event->preeditString();
    if (oldString!=mInputPreeditString) {
        if (mActiveSelectionMode==SelectionMode::Column) {
            BufferCoord selBegin = blockBegin();
            BufferCoord selEnd = blockEnd();
            invalidateLines(selBegin.line,selEnd.line);
        } else
            invalidateLine(mCaretY);
    }
    QString s = event->commitString();
    if (!s.isEmpty()) {
        commandProcessor(EditCommand::ecImeStr,QChar(),&s);
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
    if (event->modifiers() == Qt::ShiftModifier) {
        if (event->angleDelta().y()>0) {
            horizontalScrollBar()->setValue(horizontalScrollBar()->value()-mMouseWheelScrollSpeed);
            event->accept();
            return;
        } else if (event->angleDelta().y()<0) {
            horizontalScrollBar()->setValue(horizontalScrollBar()->value()+mMouseWheelScrollSpeed);
            event->accept();
            return;
        }
    } else {
        if (event->angleDelta().y()>0) {
            verticalScrollBar()->setValue(verticalScrollBar()->value()-mMouseWheelScrollSpeed);
            event->accept();
            return;
        } else if (event->angleDelta().y()<0) {
            verticalScrollBar()->setValue(verticalScrollBar()->value()+mMouseWheelScrollSpeed);
            event->accept();
            return;
        }
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

QVariant SynEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    QRect rect = calculateInputCaretRect();

    switch(property) {
    case Qt::ImCursorRectangle:
        return rect;
    default:
        return QWidget::inputMethodQuery(property);
    }

}

void SynEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain")) {
        event->acceptProposedAction();
        mDragCaretSave = caretXY();
        mDragSelBeginSave = blockBegin();
        mDragSelEndSave = blockEnd();
        BufferCoord coord = displayToBufferPos(pixelsToNearestRowColumn(event->pos().x(),
                                                                        event->pos().y()));
        internalSetCaretXY(coord);
        setBlockBegin(mDragSelBeginSave);
        setBlockEnd(mDragSelEndSave);
        showCaret();
        computeScroll(true);
    }
}

void SynEdit::dropEvent(QDropEvent *event)
{
    //mScrollTimer->stop();

    BufferCoord coord = displayToBufferPos(pixelsToNearestRowColumn(event->pos().x(),
                                                                    event->pos().y()));
    if (coord>=mDragSelBeginSave && coord<=mDragSelEndSave) {
        //do nothing if drag onto itself
        event->acceptProposedAction();
        mDropped = true;
        return;
    }
    int topLine = mTopLine;
    int leftChar = mLeftChar;
    QStringList text=splitStrings(event->mimeData()->text());
    mUndoList->beginBlock();
    addLeftTopToUndo();
    addCaretToUndo();
    addSelectionToUndo();
    internalSetCaretXY(coord);
    if (event->proposedAction() == Qt::DropAction::CopyAction) {
        //just copy it
        doInsertText(coord,text,mActiveSelectionMode,coord.line,coord.line+text.length()-1);
    } else if (event->proposedAction() == Qt::DropAction::MoveAction)  {
        if (coord < mDragSelBeginSave ) {
            //delete old
            doDeleteText(mDragSelBeginSave,mDragSelEndSave,mActiveSelectionMode);
            //paste to new position
            doInsertText(coord,text,mActiveSelectionMode,coord.line,coord.line+text.length()-1);
        } else {
            //paste to new position
            doInsertText(coord,text,mActiveSelectionMode,coord.line,coord.line+text.length()-1);
            //delete old
            doDeleteText(mDragSelBeginSave,mDragSelEndSave,mActiveSelectionMode);
            //set caret to right pos
            if (mDragSelBeginSave.line == mDragSelEndSave.line) {
                if (coord.line == mDragSelEndSave.line) {
                    coord.ch -= mDragSelEndSave.ch-mDragSelBeginSave.ch;
                }
            } else {
                if (coord.line == mDragSelEndSave.line) {
                    coord.ch -= mDragSelEndSave.ch-1;
                } else {
                    coord.line -= mDragSelEndSave.line-mDragSelBeginSave.line;
                    topLine -= mDragSelEndSave.line-mDragSelBeginSave.line;
                }
            }
        }
        mUndoList->endBlock();

    }
    event->acceptProposedAction();
    mDropped = true;
    setTopLine(topLine);
    setLeftChar(leftChar);
    internalSetCaretXY(coord);
}

void SynEdit::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->keyboardModifiers() ==  Qt::ControlModifier) {
        event->setDropAction(Qt::CopyAction);
    } else {
        event->setDropAction(Qt::MoveAction);
    }
    // should we begin scrolling?
//    computeScroll(event->pos().x(),
//                  event->pos().y(),true);

    QPoint iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    int X=iMousePos.x();
    int Y=iMousePos.y();
    BufferCoord coord = displayToBufferPos(pixelsToNearestRowColumn(X,Y));
    internalSetCaretXY(coord);
    setBlockBegin(mDragSelBeginSave);
    setBlockEnd(mDragSelEndSave);
    showCaret();
}

void SynEdit::dragLeaveEvent(QDragLeaveEvent *)
{
//    setCaretXY(mDragCaretSave);
//    setBlockBegin(mDragSelBeginSave);
//    setBlockEnd(mDragSelEndSave);
    //    showCaret();
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

void SynEdit::setModified(bool value)
{
    if (value) {
        mLastModifyTime = QDateTime::currentDateTime();
        emit statusChanged(StatusChange::scModified);
    }
    if (value != mModified) {
        mModified = value;

        if (value) {
            mUndoList->clear();
            mRedoList->clear();
        } else {
            if (mOptions.testFlag(EditorOption::eoGroupUndo)) {
                mUndoList->addGroupBreak();
            }
            mUndoList->setInitialState();
        }
        emit statusChanged(StatusChange::scModifyChanged);
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

void SynEdit::setUndoLimit(int size)
{
    mUndoList->setMaxUndoActions(size);
}

void SynEdit::setUndoMemoryUsage(int size)
{
    mUndoList->setMaxMemoryUsage(size*1024*1024);
//        mUndoList->setMaxMemoryUsage(size*1024);
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
    SelectionMode vOldMode;
    mStateFlags.setFlag(StateFlag::sfLinesChanging, false);

    updateScrollbars();
    if (mActiveSelectionMode == SelectionMode::Column) {
        BufferCoord oldBlockStart = blockBegin();
        BufferCoord oldBlockEnd = blockEnd();
        int colEnd = charToColumn(mCaretY,mCaretX);
        oldBlockStart.ch = columnToChar(oldBlockStart.line,colEnd);
        oldBlockEnd.ch = columnToChar(oldBlockEnd.line,colEnd);
        setBlockBegin(oldBlockStart);
        setBlockEnd(oldBlockEnd);
    } else {
        vOldMode = mActiveSelectionMode;
        setBlockBegin(caretXY());
        mActiveSelectionMode = vOldMode;
    }
    if (mInvalidateRect.width()==0)
        invalidate();
    else
        invalidateRect(mInvalidateRect);
    mInvalidateRect = {0,0,0,0};
    if (mGutter.showLineNumbers() && (mGutter.autoSize()))
        mGutter.autoSizeDigitCount(mDocument->count());
    //if (!mOptions.testFlag(SynEditorOption::eoScrollPastEof))
    setTopLine(mTopLine);
}

void SynEdit::onLinesChanging()
{
    mStateFlags.setFlag(StateFlag::sfLinesChanging);
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
    mStatusChanges.setFlag(StatusChange::scAll);
}

void SynEdit::onLinesDeleted(int index, int count)
{
    if (mUseCodeFolding)
        foldOnListDeleted(index + 1, count);
    if (mHighlighter && mDocument->count() > 0)
        scanFrom(index, index+1);
    invalidateLines(index + 1, INT_MAX);
    invalidateGutterLines(index + 1, INT_MAX);
}

void SynEdit::onLinesInserted(int index, int count)
{
    if (mUseCodeFolding)
        foldOnListInserted(index + 1, count);
    if (mHighlighter && mDocument->count() > 0) {
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
    }
    invalidateLines(index + 1, vEndLine);
}

void SynEdit::onUndoAdded()
{
    updateModifiedStatus();

    // we have to clear the redo information, since adding undo info removes
    // the necessary context to undo earlier edit actions
    if (! mUndoList->insideRedo() &&
            mUndoList->peekItem() && (mUndoList->peekItem()->changeReason()!=ChangeReason::GroupBreak))
        mRedoList->clear();

    onChanged();
}

SelectionMode SynEdit::activeSelectionMode() const
{
    return mActiveSelectionMode;
}

void SynEdit::setActiveSelectionMode(const SelectionMode &Value)
{
    if (mActiveSelectionMode != Value) {
        if (selAvail())
            invalidateSelection();
        mActiveSelectionMode = Value;
        if (selAvail())
            invalidateSelection();
        setStatusChanged(StatusChange::scSelection);
    }
}

BufferCoord SynEdit::blockEnd() const
{
    if (mActiveSelectionMode==SelectionMode::Column)
        return mBlockEnd;
    if ((mBlockEnd.line < mBlockBegin.line)
      || ((mBlockEnd.line == mBlockBegin.line) && (mBlockEnd.ch < mBlockBegin.ch)))
        return mBlockBegin;
    else
        return mBlockEnd;
}

void SynEdit::setBlockEnd(BufferCoord Value)
{
    //setActiveSelectionMode(mSelectionMode);
    Value.line = minMax(Value.line, 1, mDocument->count());
    if (mActiveSelectionMode == SelectionMode::Normal) {
      if (Value.line >= 1 && Value.line <= mDocument->count())
          Value.ch = std::min(Value.ch, getDisplayStringAtLine(Value.line).length() + 1);
      else
          Value.ch = 1;
    } else {
        int maxLen = mDocument->lengthOfLongestLine();
        if (highlighter())
            maxLen = maxLen+stringColumns(highlighter()->foldString(),maxLen);
        Value.ch = minMax(Value.ch, 1, maxLen+1);
    }
    if (Value.ch != mBlockEnd.ch || Value.line != mBlockEnd.line) {
        if (mActiveSelectionMode == SelectionMode::Column && Value.ch != mBlockEnd.ch) {
            invalidateLines(
                        std::min(mBlockBegin.line, std::min(mBlockEnd.line, Value.line)),
                        std::max(mBlockBegin.line, std::max(mBlockEnd.line, Value.line)));
            mBlockEnd = Value;
        } else {
            int nLine = mBlockEnd.line;
            mBlockEnd = Value;
            if (mActiveSelectionMode != SelectionMode::Column || mBlockBegin.ch != mBlockEnd.ch)
                invalidateLines(nLine, mBlockEnd.line);
        }
        setStatusChanged(StatusChange::scSelection);
    }
}

void SynEdit::setSelLength(int Value)
{
    if (mBlockBegin.line>mDocument->count() || mBlockBegin.line<=0)
        return;

    if (Value >= 0) {
        int y = mBlockBegin.line;
        int ch = mBlockBegin.ch;
        int x = ch + Value;
        QString line;
        while (y<=mDocument->count()) {
            line = mDocument->getString(y-1);
            if (x <= line.length()+2) {
                if (x==line.length()+2)
                    x = line.length()+1;
                break;
            }
            x -= line.length()+2;
            y ++;
        }
        if (y>mDocument->count()) {
            y = mDocument->count();
            x = mDocument->getString(y-1).length()+1;
        }
        BufferCoord iNewEnd{x,y};
        setCaretAndSelection(iNewEnd, mBlockBegin, iNewEnd);
    } else {
        int y = mBlockBegin.line;
        int ch = mBlockBegin.ch;
        int x = ch + Value;
        QString line;
        while (y>=1) {
            if (x>=0) {
                if (x==0)
                    x = 1;
                break;
            }
            y--;
            line = mDocument->getString(y-1);
            x += line.length()+2;
        }
        if (y>mDocument->count()) {
            y = mDocument->count();
            x = mDocument->getString(y-1).length()+1;
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
    if (mActiveSelectionMode==SelectionMode::Column)
        return mBlockBegin;
    if ((mBlockEnd.line < mBlockBegin.line)
      || ((mBlockEnd.line == mBlockBegin.line) && (mBlockEnd.ch < mBlockBegin.ch)))
        return mBlockEnd;
    else
        return mBlockBegin;
}

void SynEdit::setBlockBegin(BufferCoord value)
{
    int nInval1, nInval2;
    bool SelChanged;
    //setActiveSelectionMode(mSelectionMode);
    value.line = minMax(value.line, 1, mDocument->count());
    if (mActiveSelectionMode == SelectionMode::Normal) {
        if (value.line >= 1 && value.line <= mDocument->count())
            value.ch = std::min(value.ch, getDisplayStringAtLine(value.line).length() + 1);
        else
            value.ch = 1;
    } else {
        int maxLen = mDocument->lengthOfLongestLine();
        if (highlighter())
            maxLen = maxLen+stringColumns(highlighter()->foldString(),maxLen);
        value.ch = minMax(value.ch, 1, maxLen+1);
    }
    if (selAvail()) {
        if (mBlockBegin.line < mBlockEnd.line) {
            nInval1 = std::min(value.line, mBlockBegin.line);
            nInval2 = std::max(value.line, mBlockEnd.line);
        } else {
            nInval1 = std::min(value.line, mBlockEnd.line);
            nInval2 = std::max(value.line, mBlockBegin.line);
        };
        mBlockBegin = value;
        mBlockEnd = value;
        invalidateLines(nInval1, nInval2);
        SelChanged = true;
    } else {
        SelChanged =
          (mBlockBegin.ch != value.ch) || (mBlockBegin.line != value.line) ||
          (mBlockEnd.ch != value.ch) || (mBlockEnd.line != value.line);
        mBlockBegin = value;
        mBlockEnd = value;
    }
    if (SelChanged)
        setStatusChanged(StatusChange::scSelection);
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
        setStatusChanged(StatusChange::scLeftChar);
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
        setStatusChanged(StatusChange::scTopLine);
    }
}

void SynEdit::onGutterChanged()
{
    if (mGutter.showLineNumbers() && mGutter.autoSize())
        mGutter.autoSizeDigitCount(mDocument->count());
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
    computeScroll(false);
    //doMouseScroll(false);
}

void SynEdit::onDraggingScrollTimeout()
{
    computeScroll(true);
    //doMouseScroll(true);
}
}
