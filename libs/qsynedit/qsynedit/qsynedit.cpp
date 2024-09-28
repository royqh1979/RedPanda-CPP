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
#include "qsynedit.h"
#include "document.h"
#include "syntaxer/syntaxer.h"
#include <QApplication>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>
#include <QScrollBar>
#include <QPaintEvent>
#include <QPainter>
#include <QTimerEvent>
#include "syntaxer/syntaxer.h"
#include "syntaxer/textfile.h"
#include "painter.h"
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
#include <QTextEdit>
#include <QMimeData>

#define UPDATE_HORIZONTAL_SCROLLBAR_EVENT ((QEvent::Type)(QEvent::User+1))
#define UPDATE_VERTICAL_SCROLLBAR_EVENT ((QEvent::Type)(QEvent::User+2))

namespace QSynedit {
QSynEdit::QSynEdit(QWidget *parent) : QAbstractScrollArea(parent),
    mEditingCount{0},
    mDropped{false},
    mWheelAccumulatedDeltaX{0},
    mWheelAccumulatedDeltaY{0}
{
    mSyntaxer = std::make_shared<TextSyntaxer>();
    mCharWidth=1;
    mTextHeight = 1;
    mLastKey = 0;
    mLastKeyModifiers = Qt::NoModifier;
    mModified = false;
    mPaintLock = 0;
    mFontDummy = QFont("monospace",14);
    mFontDummy.setStyleStrategy(QFont::PreferAntialias);
    mDocument = std::make_shared<Document>(mFontDummy, this);

    mMouseMoved = false;
    mMouseOrigin = QPoint(0,0);
    mUndoing = false;
    connect(mDocument.get(), &Document::changed, this, &QSynEdit::onLinesChanged);
    connect(mDocument.get(), &Document::changing, this, &QSynEdit::onLinesChanging);
    connect(mDocument.get(), &Document::cleared, this, &QSynEdit::onLinesCleared);
    connect(mDocument.get(), &Document::deleted, this, &QSynEdit::onLinesDeleted);
    connect(mDocument.get(), &Document::inserted, this, &QSynEdit::onLinesInserted);
    connect(mDocument.get(), &Document::putted, this, &QSynEdit::onLinesPutted);
    connect(mDocument.get(), &Document::maxLineWidthChanged,
            this, &QSynEdit::onMaxLineWidthChanged);
    connect(mDocument.get(), &Document::cleared, this, &QSynEdit::updateVScrollbar);
    connect(mDocument.get(), &Document::deleted, this, &QSynEdit::updateVScrollbar);
    connect(mDocument.get(), &Document::inserted, this, &QSynEdit::updateVScrollbar);

    mGutterWidth = 0;

    mUndoList = std::make_shared<UndoList>();
    mUndoList->connect(mUndoList.get(), &UndoList::addedUndo, this, &QSynEdit::onUndoAdded);
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
    mGutter.connect(&mGutter, &Gutter::changed, this, &QSynEdit::onGutterChanged);
    mGutterWidth = mGutter.realGutterWidth(charWidth());
    this->setCursor(Qt::CursorShape::IBeamCursor);
    mInserting = true;
    mLineSpacingFactor = 1.0;

    this->setFrameShape(QFrame::Panel);
    this->setFrameShadow(QFrame::Sunken);
    this->setLineWidth(1);
    mInsertCaret = EditCaretType::VerticalLine;
    mOverwriteCaret = EditCaretType::Block;
    mActiveSelectionMode = SelectionMode::Normal;
    mReadOnly = false;

    //stop qt to auto fill background
    setAutoFillBackground(false);
    setDefaultKeystrokes();
    mRightEdgeColor = Qt::lightGray;

    mWantReturns = true;
    mWantTabs = false;
    mLeftPos = 0;
    mTopPos = 0;
    mCaretX = 1;
    mLastCaretColumn = 1;
    mCaretY = 1;
    mBlockBegin.ch = 1;
    mBlockBegin.line = 1;
    mBlockEnd = mBlockBegin;
    mOptions = EditorOption::AutoIndent
            | EditorOption::DragDropEditing | EditorOption::EnhanceEndKey
            | EditorOption::TabIndent | EditorOption::GroupUndo
            | EditorOption::KeepCaretX
            | EditorOption::SelectWordByDblClick;

    mScrollTimer = new QTimer(this);
    //mScrollTimer->setInterval(100);
    connect(mScrollTimer, &QTimer::timeout,this, &QSynEdit::onScrollTimeout);

    qreal dpr=devicePixelRatioF();
    mContentImage = std::make_shared<QImage>(clientWidth()*dpr,clientHeight()*dpr,QImage::Format_ARGB32);
    mContentImage->setDevicePixelRatio(dpr);

    mAllFoldRanges = std::make_shared<CodeFoldingRanges>();
    mUseCodeFolding = true;
    m_blinkTimerId = 0;
    m_blinkStatus = 0;

    hideCaret();

    connect(horizontalScrollBar(),&QScrollBar::valueChanged,
            this, &QSynEdit::onHScrolled);
    connect(verticalScrollBar(),&QScrollBar::valueChanged,
            this, &QSynEdit::onVScrolled);
    connect(verticalScrollBar(), &QAbstractSlider::sliderReleased,
            this, &QSynEdit::ensureLineAlignedWithTop);
    //enable input method
    setAttribute(Qt::WA_InputMethodEnabled);

    setMouseTracking(true);
    setAcceptDrops(true);

    setFont(mFontDummy);
    setScrollBars(ScrollStyle::Both);
}

int QSynEdit::lineCount() const
{
    return mDocument->count();
}

int QSynEdit::displayLineCount() const
{
    if (mDocument->empty()) {
        return 0;
    }
    return lineToRow(mDocument->count());
}

DisplayCoord QSynEdit::displayXY() const
{
    return bufferToDisplayPos(caretXY());
}

int QSynEdit::displayX() const
{
    return displayXY().x;
}

int QSynEdit::displayY() const
{
    return displayXY().row;
}

BufferCoord QSynEdit::caretXY() const
{
    BufferCoord result;
    result.ch = caretX();
    result.line = caretY();
    return result;
}

int QSynEdit::caretX() const
{
    return mCaretX;
}

int QSynEdit::caretY() const
{
    return mCaretY;
}

void QSynEdit::setCaretX(int value)
{
    setCaretXY({value,mCaretY});
}

void QSynEdit::setCaretY(int value)
{
    setCaretXY({mCaretX,value});
}

void QSynEdit::setCaretXY(const BufferCoord &value)
{
    incPaintLock();
    auto action = finally([this](){
        decPaintLock();
    });
    setBlockBegin(value);
    setBlockEnd(value);
    internalSetCaretXY(value, true);
}

void QSynEdit::setCaretXYCentered(const BufferCoord &value)
{
    incPaintLock();
    auto action = finally([this] {
        decPaintLock();
    });
    setBlockBegin(value);
    setBlockEnd(value);
    internalSetCaretXY(value, false);
    ensureCaretVisibleEx(true); // but here after block has been set
}

void QSynEdit::uncollapseAroundLine(int line)
{
    while (true) { // Open up the closed folds around the focused line until we can see the line we're looking for
      PCodeFoldingRange fold = foldHidesLine(line);
      if (fold)
          uncollapse(fold);
      else
          break;
    }
}

PCodeFoldingRange QSynEdit::foldHidesLine(int line)
{
    return foldAroundLineEx(line, true, false, true);
}

void QSynEdit::setInsertMode(bool value)
{
    if (mInserting != value) {
        mInserting = value;
        updateCaret();
        emit statusChanged(StatusChange::InsertMode);
    }
}

bool QSynEdit::insertMode() const
{
    return mInserting;
}

bool QSynEdit::canUndo() const
{
    return !mReadOnly && mUndoList->canUndo();
}

bool QSynEdit::canRedo() const
{
    return !mReadOnly && mRedoList->canRedo();
}

int QSynEdit::maxScrollWidth() const
{
    int maxWidth = mDocument->maxLineWidth();
    if (maxWidth < 0)
        return -1;
    if (useCodeFolding())
        maxWidth += stringWidth(syntaxer()->foldString(""),maxWidth);
    if (mOptions.testFlag(EditorOption::ScrollPastEol))
        return std::max(maxWidth-2*mCharWidth, 0);
    else
        return std::max(maxWidth-viewWidth()+mCharWidth, 0);
}

bool QSynEdit::getTokenAttriAtRowCol(const BufferCoord &pos, QString &token, PTokenAttribute &attri)
{
    int tmpStart;
    return getTokenAttriAtRowColEx(pos, token, tmpStart, attri);
}

bool QSynEdit::getTokenAttriAtRowCol(
        const BufferCoord &pos, QString &token,
        PTokenAttribute &attri, SyntaxState &syntaxState)
{
    int posX, posY, endPos, start;
    QString line;
    posY = pos.line - 1;
    if ((posY >= 0) && (posY < mDocument->count())) {
        line = mDocument->getLine(posY);
        if (posY == 0) {
            mSyntaxer->resetState();
        } else {
            mSyntaxer->setState(mDocument->getSyntaxState(posY-1));
        }
        mSyntaxer->setLine(line, posY);
        posX = pos.ch;
        if ((posX > 0) && (posX <= line.length())) {
            while (!mSyntaxer->eol()) {
                start = mSyntaxer->getTokenPos() + 1;
                token = mSyntaxer->getToken();
                endPos = start + token.length()-1;
                if ((posX >= start) && (posX <= endPos)) {
                    attri = mSyntaxer->getTokenAttribute();
                    syntaxState = mSyntaxer->getState();
                    return true;
                }
                mSyntaxer->next();
            }
        }
    }
    token = "";
    attri = PTokenAttribute();
    return false;
}

bool QSynEdit::getTokenAttriAtRowColEx(const BufferCoord &pos, QString &token, int &start, PTokenAttribute &attri)
{
    int posX, posY, endPos;
    QString line;
    posY = pos.line - 1;
    if ((posY >= 0) && (posY < mDocument->count())) {
        line = mDocument->getLine(posY);
        if (posY == 0) {
            mSyntaxer->resetState();
        } else {
            mSyntaxer->setState(mDocument->getSyntaxState(posY-1));
        }
        mSyntaxer->setLine(line, posY);
        posX = pos.ch;
        if ((posX > 0) && (posX <= line.length())) {
            while (!mSyntaxer->eol()) {
                start = mSyntaxer->getTokenPos() + 1;
                token = mSyntaxer->getToken();
                endPos = start + token.length()-1;
                if ((posX >= start) && (posX <= endPos)) {
                    attri = mSyntaxer->getTokenAttribute();
                    return true;
                }
                mSyntaxer->next();
            }
        }
    }
    token = "";
    attri = PTokenAttribute();
    return false;
}

void QSynEdit::addGroupBreak()
{
    mUndoList->addGroupBreak();
}

void QSynEdit::addCaretToUndo()
{
    BufferCoord p=caretXY();
    mUndoList->addChange(ChangeReason::Caret,p,p,QStringList(), mActiveSelectionMode);
}

void QSynEdit::addLeftTopToUndo()
{
    BufferCoord p;
    //todo: use buffer coord to save left/top pos is ugly
    p.ch = leftPos();
    p.line = topPos();
    mUndoList->addChange(ChangeReason::LeftTop,p,p,QStringList(), mActiveSelectionMode);
}

void QSynEdit::addSelectionToUndo()
{
    mUndoList->addChange(ChangeReason::Selection,mBlockBegin,
                         mBlockEnd,QStringList(),mActiveSelectionMode);
}

void QSynEdit::replaceAll(const QString &text)
{
    mUndoList->addChange(ChangeReason::Selection,mBlockBegin,mBlockEnd,QStringList(), activeSelectionMode());
    selectAll();
    setSelText(text);
}

void QSynEdit::doTrimTrailingSpaces()
{
    if (mDocument->count()<=0)
        return;
    beginEditing();
    auto action=finally([this](){
        endEditing();
    });
    for (int i=0;i<mDocument->count();i++) {
        if (mDocument->getSyntaxState(i).hasTrailingSpaces) {
                int line = i+1;
                QString oldLine = mDocument->getLine(i);
                QString newLine = trimRight(oldLine);
                if (newLine.isEmpty())
                    continue;
                properSetLine(i,newLine);
                mUndoList->addChange(
                            ChangeReason::Delete,
                            BufferCoord{1,line},
                            BufferCoord{oldLine.length()+1, line},
                            QStringList(oldLine),
                            SelectionMode::Normal
                            );
                mUndoList->addChange(
                            ChangeReason::Insert,
                            BufferCoord{1, line},
                            BufferCoord{newLine.length()+1, line},
                            QStringList(),
                            SelectionMode::Normal
                            );
        }
    }
    mUndoList->endBlock();    
}

BufferCoord QSynEdit::getMatchingBracket()
{
    return getMatchingBracketEx(caretXY());
}

BufferCoord QSynEdit::getMatchingBracketEx(BufferCoord APoint)
{
    QChar Brackets[] = {'(', ')', '[', ']', '{', '}', '<', '>'};
    QString Line;
    int i, PosX, PosY, Len;
    QChar Test, BracketInc, BracketDec;
    int NumBrackets;
    QString vDummy;
    PTokenAttribute attr;
    BufferCoord p;
    bool isCommentOrStringOrChar;
    int nBrackets = sizeof(Brackets) / sizeof(QChar);

    if (mDocument->count()<1)
        return BufferCoord{0,0};
    // get char at caret
    PosX = std::max(APoint.ch,1);
    PosY = std::max(APoint.line,1);
    Line = mDocument->getLine(APoint.line - 1);
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
                                if (getTokenAttriAtRowCol(p, vDummy, attr))
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
                        Line = mDocument->getLine(PosY - 1);
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
                                if (getTokenAttriAtRowCol(p, vDummy, attr))
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
                        Line = mDocument->getLine(PosY - 1);
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

QStringList QSynEdit::contents()
{
    return document()->contents();
}

QString QSynEdit::text()
{
    return document()->text();
}

bool QSynEdit::getPositionOfMouse(BufferCoord &aPos)
{
    QPoint point = QCursor::pos();
    point = mapFromGlobal(point);
    return pointToCharLine(point,aPos);
}

bool QSynEdit::getLineOfMouse(int &line)
{
    QPoint point = QCursor::pos();
    point = mapFromGlobal(point);
    return pointToLine(point,line);
}

bool QSynEdit::pointToCharLine(const QPoint &point, BufferCoord &coord)
{
    // Make sure it fits within the SynEdit bounds (and on the gutter)
    if ((point.x() < gutterWidth() + clientLeft())
            || (point.x()> clientWidth()+clientLeft())
            || (point.y() < clientTop())
            || (point.y() > clientTop()+clientHeight())) {
        return false;
    }

    coord = displayToBufferPos(pixelsToGlyphPos(point.x(),point.y()));
    return true;
}

bool QSynEdit::pointToLine(const QPoint &point, int &line)
{
    if ((point.y() < clientTop())
            || (point.y() > clientTop()+clientHeight())) {
        return false;
    }
    line = rowToLine(yposToRow(point.y()));
    return true;
}

void QSynEdit::invalidateGutter()
{
    invalidateGutterLines(-1, -1);
}

void QSynEdit::invalidateGutterLine(int aLine)
{
    if ((aLine < 1) || (aLine > mDocument->count()))
        return;

    invalidateGutterLines(aLine, aLine);
}

void QSynEdit::invalidateGutterLines(int firstLine, int lastLine)
{
    QRect rcInval;
    if (!isVisible())
        return;
    if (firstLine == -1 && lastLine == -1) {
        rcInval = QRect(0, 0, mGutterWidth, clientHeight());
        invalidateRect(rcInval);
    } else {
        // find the visible lines first
        if (lastLine < firstLine)
            std::swap(lastLine, firstLine);
        if (useCodeFolding()) {
            firstLine = lineToRow(firstLine);
            if (lastLine <= mDocument->count())
              lastLine = lineToRow(lastLine);
            else
              lastLine = mDocument->count() + mLinesInWindow + 2;
        }
        int firstLineTop = std::max((firstLine-1)*mTextHeight, mTopPos);
        int lastLineBottom = std::min(lastLine*mTextHeight, mTopPos+clientHeight());
        // any line visible?
        if (lastLine >= firstLine) {
            rcInval = {0, firstLineTop - mTopPos,
                       mGutterWidth, lastLineBottom - firstLineTop};
            invalidateRect(rcInval);
        }
    }
}

/**
 * @brief Convert point on the edit (x,y) to (row,column)
 * @param aX
 * @param aY
 * @return
 */

DisplayCoord QSynEdit::pixelsToNearestGlyphPos(int aX, int aY) const
{
    int xpos = std::max(0, leftPos() + aX - mGutterWidth - 2);
    int row = yposToRow(aY);
    int line = rowToLine(row);
    if (line<1)
        line = 1;
    if (line>mDocument->count())
        line = mDocument->count();
    if (xpos<0) {
        xpos=0;
    } else if (xpos>mDocument->lineWidth(line-1)) {
        xpos=mDocument->lineWidth(line-1)+1;
    } else {
        int glyphIndex = mDocument->xposToGlyphIndex(line-1, xpos);
        int nextGlyphPos = mDocument->glyphStartPostion(line-1, glyphIndex+1);
        if (nextGlyphPos - xpos < mCharWidth / 2)
            xpos = nextGlyphPos;
        else
            xpos = mDocument->glyphStartPostion(line-1, glyphIndex);
    }
    return DisplayCoord{xpos, row};
}

DisplayCoord QSynEdit::pixelsToGlyphPos(int aX, int aY) const
{
    int xpos = std::max(0, leftPos() + aX - mGutterWidth - 2);
    int row = yposToRow(aY);
    int line = rowToLine(row);
    if (line<1 || line > mDocument->count() )
        return DisplayCoord{-1,-1};
    if (xpos<0 || xpos>mDocument->lineWidth(line-1))
        return DisplayCoord{-1,-1};
    int glyphIndex = mDocument->xposToGlyphIndex(line-1, xpos);
    xpos = mDocument->glyphStartPostion(line-1, glyphIndex);
    return DisplayCoord{xpos, row};
}

QPoint QSynEdit::displayCoordToPixels(const DisplayCoord &coord) const
{
    QPoint result;
    result.setX(coord.x + textOffset());
    result.setY((coord.row-1) * mTextHeight - mTopPos);
    return result;
}

/**
 * @brief takes a position in the text and transforms it into
 *  the row and column it appears to be on the screen
 * @param p
 * @return
 */
DisplayCoord QSynEdit::bufferToDisplayPos(const BufferCoord &p) const
{
    DisplayCoord result {p.ch,p.line};
    if (p.line<1)
        return result;
    // Account for tabs and charColumns
    if (p.line-1 <mDocument->count())
        result.x = charToGlyphLeft(p.line,p.ch);
    result.row = lineToRow(result.row);
    return result;
}

/**
 * @brief takes a position on screen and transfrom it into position of text
 * @param p
 * @return
 */
BufferCoord QSynEdit::displayToBufferPos(const DisplayCoord &p) const
{
    BufferCoord result{p.x,p.row};
    if (p.row<1)
        return result;
    // Account for code folding
    result.line = rowToLine(p.row);
    // Account for tabs
    if (result.line <= mDocument->count() ) {
        result.ch = xposToGlyphStartChar(result.line,p.x);
    }
    return result;
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
//        QString s = mDocument->getLine(pStart.line()-1);
//        result += s.mid(pStart.ch()-1);
//    }
//    int endLine = std::min(pEnd.line(),mDocument->count());
//    for (int i=pStart.line();i<endLine-1;i++) {
//        result += mDocument->getLine(i);
//    }
//    if (pEnd.line()<=mDocument->count()) {
//        result += mDocument->getLine(pEnd.line()-1).mid(0,pEnd.ch()-1);
//    }
//    return result;
//}

//QString SynEdit::getJoinedContents(const ContentsCoord &pStart, const ContentsCoord &pEnd, const QString &joinStr)
//{
//    return getContents(pStart,pEnd).join(joinStr);
//}

int QSynEdit::leftSpaces(const QString &line) const
{
    int result = 0;
    if (mOptions.testFlag(EditorOption::AutoIndent)) {
        for (QChar ch:line) {
            if (ch == '\t') {
                result += tabSize() - (result % tabSize());
            } else if (ch == ' ') {
                result ++;
            } else {
                break;
            }
        }
    }
    return result;
}

QString QSynEdit::GetLeftSpacing(int charCount, bool wantTabs) const
{
    if (wantTabs && !mOptions.testFlag(EditorOption::TabsToSpaces) && tabSize()>0) {
        return QString(charCount / tabSize(),'\t') + QString(charCount % tabSize(),' ');
    } else {
        return QString(charCount,' ');
    }
}

int QSynEdit::charToGlyphLeft(int line, int charPos) const
{
    QString s = getDisplayStringAtLine(line);
    return mDocument->charToGlyphStartPosition(line-1, s, charPos-1);
}

int QSynEdit::charToGlyphLeft(int line, const QString &s, int charPos) const
{
    return mDocument->charToGlyphStartPosition(line-1, s, charPos-1);
}

// int QSynEdit::charToColumn(const QString &s, int aChar) const
// {
//     return mDocument->charToColumn(s, aChar);
// }

int QSynEdit::xposToGlyphStartChar(int line, int xpos) const
{
    Q_ASSERT(line>=1 && line <= mDocument->count());
    QString s = getDisplayStringAtLine(line);
    return mDocument->xposToGlyphStartChar(line-1,s,xpos)+1;
}

int QSynEdit::xposToGlyphStartChar(int line, const QString &s, int xpos) const
{
    Q_ASSERT(line>=1 && line <= mDocument->count());
    return mDocument->xposToGlyphStartChar(line-1,s,xpos)+1;
}

int QSynEdit::xposToGlyphLeft(int line, int xpos) const
{
    Q_ASSERT(line>=1 && line <= mDocument->count());
    int glyphIndex = mDocument->xposToGlyphIndex(line-1,xpos);
    return mDocument->glyphStartPostion(line-1,glyphIndex);
}

// int QSynEdit::xposToGlyphRight(int line, int xpos) const
// {
//     Q_ASSERT(line>=1 && line <= mDocument->count());
//     int glyphIndex = mDocument->xposToGlyphIndex(line-1,xpos);
//     return mDocument->glyphStartPostion(line-1,glyphIndex) + mDocument->glyphLength(line-1,glyphIndex)+1;
// }

int QSynEdit::stringWidth(const QString &line, int left) const
{
    return mDocument->stringWidth(line, left);
}

int QSynEdit::getLineIndent(const QString &line) const
{
    int indents = 0;
    for (QChar ch:line) {
        switch(ch.unicode()) {
        case '\t':
            indents+=tabSize();
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

int QSynEdit::rowToLine(int aRow) const
{
    if (useCodeFolding())
        return foldRowToLine(aRow);
    else
        return aRow;
    //return displayToBufferPos({1, aRow}).Line;
}

int QSynEdit::lineToRow(int aLine) const
{
    if (useCodeFolding())
        return foldLineToRow(aLine);
    else
        return aLine;
}

int QSynEdit::foldRowToLine(int row) const
{
    int result = row;
    for (int i=0;i<mAllFoldRanges->count();i++) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (range->collapsed && !range->parentCollapsed() && range->fromLine < result) {
            result += range->linesCollapsed;
        }
    }
    return result;
}

int QSynEdit::foldLineToRow(int line) const
{
    int result = line;
    for (int i=mAllFoldRanges->count()-1;i>=0;i--) {
        PCodeFoldingRange range =(*mAllFoldRanges)[i];
        if (range->collapsed && !range->parentCollapsed()) {
            // Line is found after fold
            if (range->toLine < line)
                result -= range->linesCollapsed;
            // Inside fold
            else if (range->fromLine < line && line <= range->toLine)
                result -= line - range->fromLine;
        }
    }
    return result;
}

void QSynEdit::setDefaultKeystrokes()
{
    mKeyStrokes.resetDefaults();
}

void QSynEdit::setExtraKeystrokes()
{
    mKeyStrokes.setExtraKeyStrokes();
}

void QSynEdit::invalidateLine(int line)
{
    QRect rcInval;
    if (line<1 || (line>mDocument->count() &&
                   line!=1) || !isVisible())
        return;

    // invalidate text area of this line
    if (useCodeFolding())
        line = foldLineToRow(line);
    if (line >= yposToRow(0) && line <= yposToRow(clientHeight())) {
        rcInval = { mGutterWidth,
                    mTextHeight * (line-1) - mTopPos,
                    clientWidth(),
                    mTextHeight};
        invalidateRect(rcInval);
    }
}

void QSynEdit::invalidateLines(int firstLine, int lastLine)
{
    if (!isVisible())
        return;
    if (firstLine == -1 && lastLine == -1) {
        QRect rcInval = clientRect();
        rcInval.setLeft(rcInval.left()+mGutterWidth);
        invalidateRect(rcInval);
    } else {
        firstLine = std::max(firstLine, 1);
        lastLine = std::max(lastLine, 1);
        // find the visible lines first
        if (lastLine < firstLine)
            std::swap(lastLine, firstLine);

        if (lastLine >= mDocument->count())
            lastLine = mDocument->count() + mLinesInWindow + 2; // paint empty space beyond last line

        if (useCodeFolding()) {
          firstLine = lineToRow(firstLine);
          // Could avoid this conversion if (First = Last) and
          // (Length < CharsInWindow) but the dependency isn't worth IMO.
          if (lastLine < mDocument->count())
              lastLine = lineToRow(lastLine + 1) - 1;
        }

        int firstLineTop = std::max((firstLine-1)*mTextHeight, mTopPos);
        int lastLineBottom = std::min(lastLine*mTextHeight, mTopPos+clientHeight());

        // qDebug()<<firstLineTop<<lastLineBottom<<firstLine<<lastLine;
        // any line visible?
        if (lastLineBottom >= firstLineTop) {
            QRect rcInval = {
                clientLeft()+mGutterWidth,
                firstLineTop - mTopPos,
                clientWidth(), lastLineBottom - firstLineTop
            };
            invalidateRect(rcInval);
            // qDebug()<<rcInval;
        }
    }
}

void QSynEdit::invalidateSelection()
{
    invalidateLines(blockBegin().line, blockEnd().line);
}

void QSynEdit::invalidateRect(const QRect &rect)
{
    viewport()->update(rect);
}

void QSynEdit::invalidate()
{
    viewport()->update();
}

bool QSynEdit::selAvail() const
{
    if (mBlockBegin.ch == mBlockEnd.ch && mBlockBegin.line == mBlockEnd.line)
        return false;
    // start line != end line  or start char != end char
    if (mActiveSelectionMode==SelectionMode::Column) {
        if (mBlockBegin.line != mBlockEnd.line) {
            DisplayCoord coordBegin = bufferToDisplayPos(mBlockBegin);
            DisplayCoord coordEnd = bufferToDisplayPos(mBlockEnd);
            return coordBegin.x!=coordEnd.x;
        } else
            return true;
    }
    return true;
}

bool QSynEdit::colSelAvail() const
{
    if (mActiveSelectionMode != SelectionMode::Column)
        return false;
    if (mBlockBegin.ch == mBlockEnd.ch && mBlockBegin.line == mBlockEnd.line)
        return false;
    if (mBlockBegin.line == mBlockEnd.line && mBlockBegin.ch!=mBlockEnd.ch)
        return true;
    DisplayCoord coordBegin = bufferToDisplayPos(mBlockBegin);
    DisplayCoord coordEnd = bufferToDisplayPos(mBlockEnd);
    return coordBegin.x!=coordEnd.x;
}

QString QSynEdit::wordAtCursor()
{
    return wordAtRowCol(caretXY());
}

QString QSynEdit::wordAtRowCol(const BufferCoord &pos)
{
    if ((pos.line >= 1) && (pos.line <= mDocument->count())) {
        QString line = mDocument->getLine(pos.line - 1);
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

QChar QSynEdit::charAt(const BufferCoord &pos)
{
    if ((pos.line >= 1) && (pos.line <= mDocument->count())) {
        QString line = mDocument->getLine(pos.line-1);
        int len = line.length();
        if (len == 0)
            return QChar(0);
        if (pos.ch<1 || pos.ch>len)
            return QChar(0);
        return line[pos.ch-1];
    }
    return QChar(0);
}

QChar QSynEdit::nextNonSpaceChar(int line, int ch)
{
    if (ch<0)
        return QChar();
    QString s = mDocument->getLine(line);
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

QChar QSynEdit::lastNonSpaceChar(int line, int ch)
{
    if (line>=mDocument->count())
        return QChar();
    QString s = mDocument->getLine(line);
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
            s = mDocument->getLine(line);
            x = s.length()-1;
        }
    }
    return QChar();
}

void QSynEdit::setCaretAndSelection(const BufferCoord &ptCaret, const BufferCoord &ptSelBegin, const BufferCoord &ptSelEnd)
{
    incPaintLock();
    internalSetCaretXY(ptCaret);
    setActiveSelectionMode(SelectionMode::Normal);
    setBlockBegin(ptSelBegin);
    setBlockEnd(ptSelEnd);
    decPaintLock();
}

bool QSynEdit::inputMethodOn()
{
    return !mInputPreeditString.isEmpty();
}

void QSynEdit::collapseAll()
{
    incPaintLock();
    for (int i = mAllFoldRanges->count()-1;i>=0;i--){
        collapse((*mAllFoldRanges)[i]);
    }
    decPaintLock();
}

void QSynEdit::unCollpaseAll()
{
    incPaintLock();
    for (int i = mAllFoldRanges->count()-1;i>=0;i--){
        uncollapse((*mAllFoldRanges)[i]);
    }
    decPaintLock();
}

void QSynEdit::processGutterClick(QMouseEvent *event)
{
    int x = event->pos().x();
    int y = event->pos().y();
    int row = yposToRow(y);
    int line = rowToLine(row);

    // Check if we clicked on a folding thing
    if (useCodeFolding()) {
        PCodeFoldingRange foldRange = foldStartAtLine(line);
        if (foldRange) {
            // See if we actually clicked on the rectangle...
            QRect rect;
            rect.setLeft(mGutterWidth - mGutter.rightOffset());
            rect.setRight(rect.left() + mGutter.rightOffset() - 4);
            rect.setTop((row - 1) * mTextHeight - mTopPos);
            rect.setBottom(rect.top() + mTextHeight - 1);
            if (rect.contains(event->pos())) {
                if (foldRange->collapsed)
                    uncollapse(foldRange);
                else
                    collapse(foldRange);
                return;
            }
        }
    }

    // If not, check gutter marks
    if (line>=1 && line <= mDocument->count()) {
        emit gutterClicked(event->button(),x,y,line);
    }
}

void QSynEdit::clearUndo()
{
    mUndoList->clear();
    mRedoList->clear();
}

BufferCoord QSynEdit::getPreviousLeftBrace(int x, int y)
{
    QChar Test;
    QString vDummy;
    PTokenAttribute attr;
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
    QString Line = mDocument->getLine(PosY - 1);
    if ((PosX > Line.length()) || (PosX<1))
        PosX = Line.length();
    int numBrackets = 1;
    while (true) {
        if (Line.isEmpty()){
            PosY--;
            if (PosY<1)
                return Result;
            Line = mDocument->getLine(PosY - 1);
            PosX = Line.length();
            continue;
        }
        Test = Line[PosX-1];
        p.ch = PosX;
        p.line = PosY;
        if (Test=='{' || Test == '}') {
            if (getTokenAttriAtRowCol(p, vDummy, attr)) {
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
            Line = mDocument->getLine(PosY - 1);
            PosX = Line.length();
        }
    }
}

void QSynEdit::showCaret()
{
    if (m_blinkTimerId==0)
        m_blinkTimerId = startTimer(500);
    m_blinkStatus = 1;
    updateCaret();
}

void QSynEdit::hideCaret()
{
    if (m_blinkTimerId!=0) {
        killTimer(m_blinkTimerId);
        m_blinkTimerId = 0;
        m_blinkStatus = 0;
        updateCaret();
    }
}

bool QSynEdit::isPointInSelection(const BufferCoord &Value) const
{
    BufferCoord ptBegin = blockBegin();
    BufferCoord ptEnd = blockEnd();
    if ((Value.line >= ptBegin.line) && (Value.line <= ptEnd.line) &&
            ((ptBegin.line != ptEnd.line) || (ptBegin.ch != ptEnd.ch))) {
        if (mActiveSelectionMode == SelectionMode::Column) {
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

BufferCoord QSynEdit::nextWordPos()
{
    return nextWordPosEx(caretXY());
}

BufferCoord QSynEdit::nextWordPosEx(const BufferCoord &XY)
{
    int CX = XY.ch;
    int CY = XY.line;
    // valid line?
    if ((CY >= 1) && (CY <= mDocument->count())) {
        QString Line = mDocument->getLine(CY - 1);
        int LineLen = Line.length();
        if (CX >= LineLen) {
            // find first IdentChar or multibyte char in the next line
            if (CY < mDocument->count()) {
                Line = mDocument->getLine(CY);
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
                    Line = mDocument->getLine(CY);
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

BufferCoord QSynEdit::wordStart()
{
    return wordStartEx(caretXY());
}

BufferCoord QSynEdit::wordStartEx(const BufferCoord &XY)
{
    int CX = XY.ch;
    int CY = XY.line;
    // valid line?
    if ((CY >= 1) && (CY <= mDocument->count())) {
        QString Line = mDocument->getLine(CY - 1);
        CX = std::min(CX, Line.length()+1);
        if (CX > 1) {
            if (isWordChar(Line[CX - 2]))
                CX = findLastNonWordChar(Line, CX - 1) + 1;
        }
    }
    return BufferCoord{CX,CY};
}

BufferCoord QSynEdit::wordEnd()
{
    return wordEndEx(caretXY());
}

BufferCoord QSynEdit::wordEndEx(const BufferCoord &XY)
{
    int CX = XY.ch;
    int CY = XY.line;
    // valid line?
    if ((CY >= 1) && (CY <= mDocument->count())) {
        QString Line = mDocument->getLine(CY - 1);
        if (CX <= Line.length() && CX-1>=0) {
            if (isWordChar(Line[CX - 1]))
                CX = findNonWordChar(Line, CX);
            if (CX == 0)
                CX = Line.length() + 1;
        }
    }
    return BufferCoord{CX,CY};
}

BufferCoord QSynEdit::prevWordPos()
{
    return prevWordPosEx(caretXY());
}

BufferCoord QSynEdit::prevWordPosEx(const BufferCoord &XY)
{
    int CX = XY.ch;
    int CY = XY.line;
    // valid line?
    if ((CY >= 1) && (CY <= mDocument->count())) {
        QString Line = mDocument->getLine(CY - 1);
        CX = std::min(CX, Line.length());
        if (CX <= 1) {
            // find last IdentChar in the previous line
            if (CY > 1) {
                CY -- ;
                Line = mDocument->getLine(CY - 1);
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
                    Line = mDocument->getLine(CY - 1);
                    CX = findLastWordChar(Line, Line.length())+1;
                } else {
                    CX = 1;
                }
            }
        }
    }
    return BufferCoord{CX,CY};
}

void QSynEdit::setSelWord()
{
    setWordBlock(caretXY());
}

void QSynEdit::setWordBlock(BufferCoord value)
{
    incPaintLock();
    auto action = finally([this](){
        decPaintLock();
    });

    value.line = minMax(value.line, 1, mDocument->count());
    value.ch = std::max(value.ch, 1);
    QString TempString = mDocument->getLine(value.line - 1); //needed for CaretX = LineLength +1
    if (value.ch > TempString.length()) {
        internalSetCaretXY(BufferCoord{TempString.length()+1, value.line});
        return;
    }

    BufferCoord vWordStart = wordStartEx(value);
    BufferCoord vWordEnd = wordEndEx(value);
    if ((vWordStart.line == vWordEnd.line) && (vWordStart.ch < vWordEnd.ch))
        setCaretAndSelection(vWordEnd, vWordStart, vWordEnd);
}

void QSynEdit::doExpandSelection(const BufferCoord &pos)
{
    if (selAvail()) {
        //todo
    } else {
        setWordBlock(pos);
    }
}

void QSynEdit::doShrinkSelection(const BufferCoord &/*pos*/)
{
    //todo
}

int QSynEdit::calcIndentSpaces(int line, const QString& lineText, bool addIndent)
{
    line = std::min(line, mDocument->count()+1);
    if (line<=1)
        return 0;
    if (lineText.startsWith("//"))
        return 0;
    if (mFormatter) {
        return mFormatter->calcIndentSpaces(line,lineText,addIndent,this);
    }
    // find the first non-empty preceeding line
    int startLine = line-1;
    QString startLineText;
    while (startLine>=1) {
        startLineText = mDocument->getLine(startLine-1);
        if (!startLineText.trimmed().isEmpty()) {
            break;
        }
        startLine -- ;
    }
    int indentSpaces = 0;
    if (startLine>=1) {
        //calculate the indents of last statement;
        indentSpaces = leftSpaces(startLineText);
    }
    return std::max(0,indentSpaces);
}

void QSynEdit::doSelectAll()
{
    incPaintLock();
    auto action = finally([this](){
        decPaintLock();
    });
    BufferCoord lastPt;
    lastPt.ch = 1;
    if (mDocument->empty()) {
        lastPt.line = 1;
    } else {
        lastPt.line = mDocument->count();
        lastPt.ch = mDocument->getLine(lastPt.line-1).length()+1;
    }
    setCaretAndSelection(caretXY(), BufferCoord{1, 1}, lastPt);
    // Selection should have changed...
    emit statusChanged(StatusChange::Selection);
}

void QSynEdit::doComment()
{
    BufferCoord origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    if (mReadOnly)
        return;
    if (mSyntaxer->commentSymbol().isEmpty())
        return;
    beginEditing();
    auto action = finally([this]{
        endEditing();
    });
    origBlockBegin = blockBegin();
    origBlockEnd = blockEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.ch == 1)
        endLine = std::max(origBlockBegin.line - 1, origBlockEnd.line - 2);
    else
        endLine = origBlockEnd.line - 1;
    QString commentSymbol = mSyntaxer->commentSymbol();
    int symbolLen = commentSymbol.length();
    for (int i = origBlockBegin.line - 1; i<=endLine; i++) {
        mDocument->putLine(i, commentSymbol + mDocument->getLine(i));
        mUndoList->addChange(ChangeReason::Insert,
              BufferCoord{1, i + 1},
              BufferCoord{1+symbolLen, i + 1},
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

void QSynEdit::doUncomment()
{
    BufferCoord origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    QString s,s2;
    QStringList changeText;
    if (mReadOnly)
        return;
    if (mSyntaxer->commentSymbol().isEmpty())
        return;
    QString commentSymbol=mSyntaxer->commentSymbol();
    int symbolLen = commentSymbol.length();
    changeText.append(commentSymbol);
    beginEditing();
    auto action = finally([this]{
        endEditing();
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
        s = mDocument->getLine(i);
        s2=s.trimmed();
        if (!s2.startsWith(commentSymbol))
            continue;
        // Find // after blanks only
        int j = 0;
        while ((j+1 < s.length()) && (s[j] == ' ' || s[j] == '\t'))
            j++;
        s.remove(j,symbolLen);
        mDocument->putLine(i,s);
        mUndoList->addChange(ChangeReason::Delete,
                             BufferCoord{j+1, i + 1},
                             BufferCoord{j+1+symbolLen, i + 1},
                             changeText, SelectionMode::Normal);
        // Move begin of selection
        if ((i == origBlockBegin.line - 1) && (origBlockBegin.ch > 1))
            origBlockBegin.ch-=symbolLen;
        // Move end of selection
        if ((i == origBlockEnd.line - 1) && (origBlockEnd.ch > 1))
            origBlockEnd.ch-=symbolLen;
        // Move caret
        if ((i == origCaret.line - 1) && (origCaret.ch > 1))
            origCaret.ch-=symbolLen;
    }
    // When grouping similar commands, process one uncomment action per undo/redo
    mUndoList->addGroupBreak();
    setCaretAndSelection(origCaret,origBlockBegin,origBlockEnd);
}

void QSynEdit::doToggleComment()
{
    BufferCoord origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    QString s;
    bool allCommented = true;
    if (mReadOnly)
        return;
    if (mSyntaxer->commentSymbol().isEmpty())
        return;
    QString commentSymbol=mSyntaxer->commentSymbol();

    beginEditing();
    auto action = finally([this]{
        endEditing();
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
        s = mDocument->getLine(i).trimmed();
        if (!s.startsWith(commentSymbol)) {
            allCommented = false;
            break;
        }
    }
    if (allCommented)
        doUncomment();
    else
        doComment();
}

void QSynEdit::doToggleBlockComment()
{
    QString s;
    if (mReadOnly)
        return;
    if (mSyntaxer->blockCommentBeginSymbol().isEmpty())
        return;
    QString beginSymbol=mSyntaxer->blockCommentBeginSymbol();
    QString endSymbol=mSyntaxer->blockCommentEndSymbol();
    int beginLen = beginSymbol.length();
    int endLen = endSymbol.length();

    QString text=selText().trimmed();
    if (text.length()>beginLen+endLen && text.startsWith(beginSymbol) && text.endsWith(endSymbol)) {
        QString newText=selText();
        int pos = newText.indexOf(beginSymbol);
        if (pos>=0) {
            newText.remove(pos,beginLen);
        }
        pos = newText.lastIndexOf(endSymbol);
        if (pos>=0) {
            newText.remove(pos,endLen);
        }
        setSelText(newText);
    } else {
        QString newText=beginSymbol+selText()+endSymbol;
        setSelText(newText);
    }

}

void QSynEdit::doMouseScroll(bool isDragging, int scrollX, int scrollY)
{
    if (mDropped) {
        mDropped=false;
        return;
    }
    if (mStateFlags.testFlag(StateFlag::DblClicked))
        return;
    if (!hasFocus())
        return;
    Qt::MouseButtons buttons = qApp->mouseButtons();
    if (!buttons.testFlag(Qt::LeftButton))
        return;
    QPoint iMousePos;
    DisplayCoord coord;

    iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    coord = pixelsToNearestGlyphPos(iMousePos.x(), iMousePos.y());
    coord.row = minMax(coord.row, 1, displayLineCount());
    if (scrollX != 0) {
        int x;
        setLeftPos(leftPos() + scrollX * mMouseSelectionScrollSpeed);
        x = leftPos();
        if (scrollX > 0) // scrolling right?
            x+=viewWidth();
        coord.x = x;
    }
    if (scrollY != 0) {
        int y;
        setTopPos(mTopPos + scrollY * mMouseSelectionScrollSpeed);
        y = yposToRow(0);
        if (scrollY > 0)  // scrolling down?
            y+=mLinesInWindow - 1;
        coord.row = minMax(y, 1, displayLineCount());
    }
    BufferCoord vCaret = displayToBufferPos(coord);
    if ((caretX() != vCaret.ch) || (caretY() != vCaret.line)) {
        // changes to line / column in one go
        incPaintLock();
        auto action = finally([this]{
            decPaintLock();
        });
        internalSetCaretXY(vCaret, false);

        // if MouseCapture is True we're changing selection. otherwise we're dragging
        if (isDragging) {
            setBlockBegin(mDragSelBeginSave);
            setBlockEnd(mDragSelEndSave);
        } else
            setBlockEnd(caretXY());

        if (mOptions.testFlag(EditorOption::GroupUndo))
            mUndoList->addGroupBreak();
    }
    if (isDragging) {
        mScrollTimer->singleShot(20,this,&QSynEdit::onDraggingScrollTimeout);
    } else  {
        mScrollTimer->singleShot(20,this,&QSynEdit::onScrollTimeout);
    }
}

void QSynEdit::beginEditing()
{
    incPaintLock();
    if (mEditingCount==0) {
        if (!mUndoing)
            mUndoList->beginBlock();
    }
    mEditingCount++;
}

void QSynEdit::endEditing()
{
    mEditingCount--;
    if (mEditingCount==0) {
        if (!mUndoing)
            mUndoList->endBlock();
        reparseDocument();
    }
    decPaintLock();
}

QString QSynEdit::getDisplayStringAtLine(int line) const
{
    QString s = mDocument->getLine(line-1);
    PCodeFoldingRange foldRange = foldStartAtLine(line);
    if ((foldRange) && foldRange->collapsed) {
        return s+mSyntaxer->foldString(s);
    }
    return s;
}

void QSynEdit::onMaxLineWidthChanged()
{
    invalidate(); // repaint first, to update line widths
    updateHScrollBarLater();
}

void QSynEdit::updateHScrollBarLater()
{
    QEvent * event = new QEvent(UPDATE_HORIZONTAL_SCROLLBAR_EVENT);
    qApp->postEvent(this,event);
}

void QSynEdit::doDeleteLastChar()
{
    if (mReadOnly)
        return ;
    auto action = finally([this]{
        ensureCaretVisible();
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
    QString tempStr = lineText();
    int tempStrLen = tempStr.length();
    BufferCoord caretBackup = caretXY();
    QStringList helper;
    if (mCaretX > tempStrLen + 1) {
        // only move caret one column
        return;
    } else if (mCaretX == 1) {
        // join this line with the last line if possible
        if (mCaretY > 1) {
            internalSetCaretY(mCaretY - 1);
            internalSetCaretX(mDocument->getLine(mCaretY - 1).length() + 1);
            mDocument->deleteAt(mCaretY);
            doLinesDeleted(mCaretY+1, 1);
            setLineText(lineText() + tempStr);
            helper.append("");
            helper.append("");
            shouldAddGroupBreak=true;
        }
    } else {
        // delete char
        int glyphIndex = mDocument->charToGlyphIndex(mCaretY-1,mCaretX-1);
        Q_ASSERT(glyphIndex>0);
        int oldCaretX = mCaretX;
        int newCaretX = mDocument->glyphStartChar(mCaretY-1, glyphIndex-1)+1;
        //qDebug()<<"delete last char:"<<oldCaretX<<newCaretX<<glyphIndex<<mCaretY;
        QString s = tempStr.mid(newCaretX-1, oldCaretX-newCaretX);
        internalSetCaretX(newCaretX);
        if (s==' ' || s=='\t')
            shouldAddGroupBreak=true;
        helper.append(s);
        tempStr.remove(newCaretX-1, oldCaretX-newCaretX);
        properSetLine(mCaretY - 1, tempStr);
    }
    if ((caretBackup.ch != mCaretX) || (caretBackup.line != mCaretY)) {
        mUndoList->addChange(ChangeReason::Delete, caretXY(), caretBackup, helper,
                        mActiveSelectionMode);
        if (shouldAddGroupBreak)
            mUndoList->addGroupBreak();
    }
}

void QSynEdit::doDeleteCurrentChar()
{
    QStringList helper;
    BufferCoord newCaret;
    if (mReadOnly) {
        return;
    }
    auto action = finally([this]{
        ensureCaretVisible();
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
        QString tempStr = lineText();
        int tempStrLen = tempStr.length();
        if (mCaretX>tempStrLen+1) {
            return;
        } else if (mCaretX <= tempStrLen) {
            int glyphIndex = mDocument->charToGlyphIndex(mCaretY-1,mCaretX-1);
            int glyphLen = mDocument->glyphLength(mCaretY-1,glyphIndex);
            QString s = tempStr.mid(mCaretX-1,glyphLen);
            if (s==' ' || s=='\t')
                shouldAddGroupBreak=true;
            // delete char
            helper.append(s);
            newCaret.ch = mCaretX + glyphLen;
            newCaret.line = mCaretY;
            tempStr.remove(mCaretX-1, glyphLen);
            properSetLine(mCaretY - 1, tempStr);
        } else {
            // join line with the line after
            if (mCaretY < mDocument->count()) {
                shouldAddGroupBreak=true;
                properSetLine(mCaretY - 1, tempStr + mDocument->getLine(mCaretY));
                newCaret.ch = 1;
                newCaret.line = mCaretY + 1;
                helper.append("");
                helper.append("");
                mDocument->deleteAt(mCaretY);
                if (mCaretX==1)
                    doLinesDeleted(mCaretY, 1);
                else
                    doLinesDeleted(mCaretY + 1, 1);
            }
        }
        if ((newCaret.ch != mCaretX) || (newCaret.line != mCaretY)) {
            mUndoList->addChange(ChangeReason::Delete, caretXY(), newCaret,
                  helper, mActiveSelectionMode);
            if (shouldAddGroupBreak)
                mUndoList->addGroupBreak();
        }
    }
}

void QSynEdit::doDeleteWord()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    BufferCoord start = wordStart();
    BufferCoord end = wordEnd();
    deleteFromTo(start,end);
}

void QSynEdit::doDeleteToEOL()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    deleteFromTo(caretXY(),BufferCoord{lineText().length()+1,mCaretY});
}

void QSynEdit::doDeleteToWordStart()
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

void QSynEdit::doDeleteToWordEnd()
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

void QSynEdit::doDeleteFromBOL()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    deleteFromTo(BufferCoord{1,mCaretY},caretXY());
}

void QSynEdit::doDeleteLine()
{
    if (mReadOnly || (mDocument->count() <= 0))
        return;
    PCodeFoldingRange foldRange=foldStartAtLine(mCaretY);
    if (foldRange && foldRange->collapsed)
        return;
    beginEditing();
    addCaretToUndo();
    addSelectionToUndo();
    if (selAvail())
        setBlockBegin(caretXY());
    int oldCaretY = caretY();
    bool isLastLine = (oldCaretY >= mDocument->count());
    bool isFirstLine = (oldCaretY == 1);
    BufferCoord startPos;
    BufferCoord endPos;
    if (isLastLine) {
        if (isFirstLine) {
            startPos.ch = 1;
            startPos.line = oldCaretY;
        } else {
            startPos.ch = mDocument->getLine(oldCaretY-2).length()+1;
            startPos.line = oldCaretY-1;
        }
        endPos.ch = lineText().length()+1;
        endPos.line = oldCaretY;
    } else {
        startPos.ch = 1;
        startPos.line = oldCaretY;
        endPos.ch = 1;
        endPos.line = oldCaretY+1;
    }
    doDeleteText(startPos, endPos, SelectionMode::Normal);
    endEditing();
    internalSetCaretXY(BufferCoord{1, oldCaretY});
}

void QSynEdit::doSelectLine()
{
    BufferCoord ptBegin=BufferCoord{1,mCaretY};
    BufferCoord ptEnd;
    if (mCaretY==mDocument->count())
        ptEnd = BufferCoord{lineText().length()+1,mCaretY};
    else
        ptEnd = BufferCoord{1,mCaretY+1};
    setCaretAndSelection(ptBegin,ptBegin,ptEnd);
}

BufferCoord QSynEdit::ensureBufferCoordValid(const BufferCoord &coord)
{
    int nMaxX;
    BufferCoord value = coord;
    if (value.line > mDocument->count())
        value.line = mDocument->count();
    if (mActiveSelectionMode!=SelectionMode::Column) {
        if (value.line < 1) {
            // this is just to make sure if Lines stringlist should be empty
            value.line = 1;
            if (!mOptions.testFlag(EditorOption::ScrollPastEol)) {
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
    return value;
}

void QSynEdit::doDuplicateLine()
{
    if (!mReadOnly && (mDocument->count() > 0)) {
        PCodeFoldingRange foldRange=foldStartAtLine(mCaretY);
        if (foldRange && foldRange->collapsed)
            return;
        QString s = lineText();
        beginEditing();
        mDocument->insertLine(mCaretY, lineText());
        doLinesInserted(mCaretY + 1, 1);
        addCaretToUndo();
        mUndoList->addChange(ChangeReason::LineBreak,
                             BufferCoord{s.length()+1,mCaretY},
                             BufferCoord{s.length()+1,mCaretY}, QStringList(), SelectionMode::Normal);
        mUndoList->addChange(ChangeReason::Insert,
                             BufferCoord{1,mCaretY+1},
                             BufferCoord{s.length()+1,mCaretY+1}, QStringList(), SelectionMode::Normal);
        endEditing();
        internalSetCaretXY(BufferCoord{1, mCaretY}); // like seen in the Delphi editor
    }
}

void QSynEdit::doMoveSelUp()
{
    if (mActiveSelectionMode == SelectionMode::Column)
        return;
    if (!mReadOnly && (mDocument->count() > 0) && (blockBegin().line > 1)) {
        if (!mUndoing) {
            beginEditing();
            addCaretToUndo();
            addSelectionToUndo();
        }
        int origBlockBeginLine = selectionBeginLine();
        int origBlockEndLine = selectionEndLine();
        BufferCoord origBlockBegin = blockBegin();
        BufferCoord origBlockEnd = blockEnd();

        PCodeFoldingRange foldRange=foldStartAtLine(origBlockEndLine);
        if (foldRange && foldRange->collapsed)
            return;
//        for (int line=origBlockBegin.Line;line<=origBlockEnd.Line;line++) {
//            PSynEditFoldRange foldRange=foldStartAtLine(line);
//            if (foldRange && foldRange->collapsed)
//                return;
//        }

        // Delete line above selection
        QString s = mDocument->getLine(origBlockBeginLine - 2); // before start, 0 based
        mDocument->deleteAt(origBlockBeginLine - 2); // before start, 0 based
        doLinesDeleted(origBlockBeginLine - 1, 1); // before start, 1 based

        // Insert line below selection
        mDocument->insertLine(origBlockEndLine - 1, s);
        doLinesInserted(origBlockEndLine, 1);
        // Restore caret and selection
        setCaretAndSelection(
                  BufferCoord{mCaretX, origBlockBeginLine - 1},
                  BufferCoord{origBlockBegin.ch, origBlockBegin.line - 1},
                  BufferCoord{origBlockEnd.ch, origBlockEnd.line - 1}
        );
        if (!mUndoing) {
            mUndoList->addChange(ChangeReason::MoveSelectionUp,
                    origBlockBegin,
                    origBlockEnd,
                    QStringList(),
                    SelectionMode::Normal);
            endEditing();
        }
    }
}

void QSynEdit::doMoveSelDown()
{
    if (mActiveSelectionMode == SelectionMode::Column)
        return;
    if (!mReadOnly && (mDocument->count() > 0) && (blockEnd().line < mDocument->count())) {
        if (!mUndoing) {
            beginEditing();
            addCaretToUndo();
            addSelectionToUndo();
        }
        int origBlockBeginLine = selectionBeginLine();
        int origBlockEndLine = selectionEndLine();
        BufferCoord origBlockBegin = blockBegin();
        BufferCoord origBlockEnd = blockEnd();

        PCodeFoldingRange foldRange=foldStartAtLine(origBlockEndLine);
        if (foldRange && foldRange->collapsed)
            return;

        // Delete line below selection
        QString s = mDocument->getLine(origBlockEndLine); // after end, 0 based
        mDocument->deleteAt(origBlockEndLine); // after end, 0 based
        doLinesDeleted(origBlockEndLine, 1); // before start, 1 based

        // Insert line above selection
        mDocument->insertLine(origBlockBeginLine  - 1, s);
        doLinesInserted(origBlockBeginLine , 1);

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
            endEditing();
        }

    }
}

void QSynEdit::clearAll()
{
    mDocument->clear();
    mUndoList->clear();
    mRedoList->clear();
    setModified(false);
}

void QSynEdit::insertLine(bool moveCaret)
{
    if (mReadOnly)
        return;
    int nLinesInserted=0;
    if (!mUndoing)
        beginEditing();
    auto action = finally([this] {
        if (!mUndoing)
            endEditing();
    });
    QString helper;
    if (selAvail()) {
        helper = selText();
        setSelectedTextEmpty();
    }

    QString temp = lineText();

    if (mCaretX>lineText().length()+1) {
        PCodeFoldingRange foldRange = foldStartAtLine(mCaretY);
        if ((foldRange) && foldRange->collapsed) {
            QString s = temp+mSyntaxer->foldString(temp);
            if (mCaretX > s.length()) {
                if (!mUndoing) {
                    addCaretToUndo();
                    addSelectionToUndo();
                }
                mCaretY=foldRange->toLine;
                if (mCaretY>mDocument->count()) {
                    mCaretY=mDocument->count();
                }
                temp = lineText();
                mCaretX=temp.length()+1;
            }
        }
    }

    QString Temp2 = temp;
    QString Temp3;
    PTokenAttribute Attr;

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
    if (mCaretY==1) {
        mSyntaxer->resetState();
    } else {
        mSyntaxer->setState(mDocument->getSyntaxState(mCaretY-2));
    }
    mSyntaxer->setLine(leftLineText, mCaretY-1);
    mSyntaxer->nextToEol();
    mDocument->setSyntaxState(mCaretY-1,mSyntaxer->getState());
    notInComment = !mSyntaxer->isCommentNotFinished(
                mSyntaxer->getState().state)
            && !mSyntaxer->isStringNotFinished(
                mSyntaxer->getState().state);

    int indentSpaces = 0;
    if (mOptions.testFlag(EditorOption::AutoIndent)) {
        rightLineText=trimLeft(rightLineText);
        indentSpaces = calcIndentSpaces(mCaretY+1,
                                        rightLineText,mOptions.testFlag(EditorOption::AutoIndent)
                                            );
    }
    QString indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
    mDocument->insertLine(mCaretY, indentSpacesForRightLineText+rightLineText);
    nLinesInserted++;

    if (!mUndoing) {
        //insert new line in middle of "/*" and "*/"
        if (!notInComment &&
                ( leftLineText.endsWith("/*") && rightLineText.startsWith("*/")
                 )) {
            indentSpaces = calcIndentSpaces(mCaretY+1, "" , mOptions.testFlag(EditorOption::AutoIndent));
            indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
            mDocument->insertLine(mCaretY, indentSpacesForRightLineText);
            nLinesInserted++;
            mUndoList->addChange(ChangeReason::LineBreak, caretXY(), caretXY(), QStringList(),
                    SelectionMode::Normal);
        }
        //insert new line in middle of "{" and "}"
        if (notInComment &&
                ( leftLineText.endsWith('{') && rightLineText.startsWith('}')
                 )) {
            indentSpaces = calcIndentSpaces(mCaretY+1, "" , mOptions.testFlag(EditorOption::AutoIndent)
                                                                   && notInComment);
            indentSpacesForRightLineText = GetLeftSpacing(indentSpaces,true);
            mDocument->insertLine(mCaretY, indentSpacesForRightLineText);
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
    ensureCaretVisible();
    updateLastCaretX();
}

void QSynEdit::doTabKey()
{
    if (mActiveSelectionMode == SelectionMode::Column) {
        doAddChar('\t');
        return;
    }
    // Provide Visual Studio like block indenting
    if (mOptions.testFlag(EditorOption::TabIndent) && canDoBlockIndent()) {
        doBlockIndent();
        return;
    }
    beginEditing();
    if (selAvail()) {
        setSelectedTextEmpty();
    }
    QString Spaces;
    if (mOptions.testFlag(EditorOption::TabsToSpaces)) {
        int left = charToGlyphLeft(mCaretY,mCaretX);
        int i = std::ceil( (tabWidth() - (left) % tabWidth() ) / (float) tabSize());
        Spaces = QString(i,' ');
    } else {
        Spaces = '\t';
    }
    setSelTextPrimitive(QStringList(Spaces));
    endEditing();
    ensureCaretVisible();
}

void QSynEdit::doShiftTabKey()
{
    // Provide Visual Studio like block indenting
    if (mOptions.testFlag(EditorOption::TabIndent) && canDoBlockIndent()) {
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
        int spacesBefore = leftSpaces(lineText());
        int spacesToRemove = spacesBefore % tabSize();
        if (spacesToRemove == 0)
            spacesToRemove = tabSize();
        if (spacesToRemove > spacesBefore )
            spacesToRemove = spacesBefore;
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


bool QSynEdit::canDoBlockIndent()
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
        QString s = mDocument->getLine(BB.line-1).mid(0,BB.ch-1);
        if (!s.trimmed().isEmpty())
            return false;
        if (BE.ch>1) {
            QString s1=mDocument->getLine(BE.line-1).mid(BE.ch-1);
            QString s2=mDocument->getLine(BE.line-1).mid(0,BE.ch-1);
            if (!s1.trimmed().isEmpty() && !s2.trimmed().isEmpty())
                return false;
        }
    }
    if (mActiveSelectionMode == SelectionMode::Column) {
        int startPos = charToGlyphLeft(BB.line,BB.ch);
        int endPos = charToGlyphLeft(BE.line,BE.ch);
        for (int i = BB.line; i<=BE.line;i++) {
            QString line = mDocument->getLine(i-1);
            int startChar = xposToGlyphStartChar(i,startPos);
            QString s = line.mid(0,startChar-1);
            if (!s.trimmed().isEmpty())
                return false;

            int endChar = xposToGlyphStartChar(i,endPos);
            s=line.mid(endChar-1);
            if (!s.trimmed().isEmpty())
                return false;
        }
    }
    return true;
}

QRect QSynEdit::calculateCaretRect() const
{
    DisplayCoord coord = displayXY();
    if (!mInputPreeditString.isEmpty()) {
        QString sLine = lineText().left(mCaretX-1)
                + mInputPreeditString
                + lineText().mid(mCaretX-1);
        if (sLine == mGlyphPostionCacheForInputMethod.str)  {
            int glyphIdx = searchForSegmentIdx(mGlyphPostionCacheForInputMethod.glyphCharList,0,sLine.length(),mCaretX+mInputPreeditString.length()-1);
            coord.x = segmentIntervalStart(mGlyphPostionCacheForInputMethod.glyphPositionList,0,mGlyphPostionCacheForInputMethod.strWidth, glyphIdx);
        } else
            coord.x = charToGlyphLeft(mCaretY, sLine, mCaretX+mInputPreeditString.length());
    }
    int rows=1;
    if (mActiveSelectionMode == SelectionMode::Column) {
        int startRow = lineToRow(std::min(blockBegin().line, blockEnd().line));
        int endRow = lineToRow(std::max(blockBegin().line, blockEnd().line));
        coord.row = startRow;
        rows = endRow-startRow+1;
    }
    QPoint caretPos = displayCoordToPixels(coord);
    int caretWidth = mCharWidth;
    if (mCaretY <= mDocument->count() && mCaretX <= mDocument->getLine(mCaretY-1).length()) {
        int glyphIndex = mDocument->charToGlyphIndex(mCaretY-1, mCaretX-1);
        caretWidth = mDocument->glyphWidth(mCaretY-1, glyphIndex);
    }
//    qDebug()<<"caret:"<<mCaretX<<mCaretY<<caretWidth;
    if (mActiveSelectionMode == SelectionMode::Column) {
        return QRect(caretPos.x(),caretPos.y(),caretWidth,
                     mTextHeight*(rows));
    } else {
        return QRect(caretPos.x(),caretPos.y(),caretWidth,
                     mTextHeight);
    }
}

QRect QSynEdit::calculateInputCaretRect() const
{
    DisplayCoord coord = displayXY();
    QPoint caretPos = displayCoordToPixels(coord);
    int caretWidth=mCharWidth;
    if (mCaretY <= mDocument->count() && mCaretX <= mDocument->getLine(mCaretY-1).length()) {
        int glyphIndex = mDocument->charToGlyphIndex(mCaretY-1, mCaretX-1);
        caretWidth = mDocument->glyphWidth(mCaretY-1, glyphIndex);
    }
    return QRect(caretPos.x(),caretPos.y(),caretWidth,
                 mTextHeight);
}

void QSynEdit::clearAreaList(EditingAreaList areaList)
{
    areaList.clear();
}

void QSynEdit::computeCaret()
{
    QPoint iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    int x=iMousePos.x();
    int y=iMousePos.y();

    DisplayCoord vCaretNearestPos = pixelsToNearestGlyphPos(x, y);
    vCaretNearestPos.row = minMax(vCaretNearestPos.row, 1, displayLineCount());
    setCaretDisplayXY(vCaretNearestPos, false);
}

void QSynEdit::computeScroll(bool isDragging)
{
    QPoint iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    int x=iMousePos.x();
    int y=iMousePos.y();

    QRect iScrollBounds; // relative to the client area
    int dispX=2,dispY = 2;
    int left = mGutterWidth+frameWidth()+dispX;
    int top = frameWidth()+dispY;
    iScrollBounds = QRect(left,
                          top,
                          clientWidth()-left-dispX,
                          clientHeight()-top-dispY);

    int scrollX,scrollY;
    if (x < iScrollBounds.left())
        scrollX = (x - iScrollBounds.left()) / mCharWidth - 1;
    else if (x >= iScrollBounds.right())
        scrollX = (x - iScrollBounds.right()) / mCharWidth + 1;
    else
        scrollX = 0;

    if (y < iScrollBounds.top())
        scrollY = (y - iScrollBounds.top()) / mTextHeight - 1;
    else if (y >= iScrollBounds.bottom()) {
        scrollY = (y - iScrollBounds.bottom()) / mTextHeight + 1;
    } else
        scrollY = 0;

    doMouseScroll(isDragging, scrollX, scrollY);
}

void QSynEdit::doBlockIndent()
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
        if (mOptions.testFlag(EditorOption::TabsToSpaces))
            x = caretX() + tabSize();
        else
          x = caretX() + 1;
    }
    if (mOptions.testFlag(EditorOption::TabsToSpaces)) {
        spaces = QString(tabSize(),' ') ;
    } else {
        spaces = "\t";
    }
//    for (i = BB.line; i<e;i++) {
//        strToInsert.append(spaces);
//    }
//    strToInsert.append(spaces);
    beginEditing();
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
        QString line=mDocument->getLine(i-1);
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
    endEditing();
}

void QSynEdit::doBlockUnindent()
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
    beginEditing();
    mUndoList->addChange(ChangeReason::Caret, oldCaretPos, oldCaretPos,QStringList(), activeSelectionMode());
    mUndoList->addChange(ChangeReason::Selection,mBlockBegin,mBlockEnd,QStringList(), activeSelectionMode());

    int e = BE.line;
    // convert selection to complete lines
    if (BE.ch == 1)
        e = BE.line - 1;
    // build string to delete
    for (int i = BB.line; i<= e;i++) {
        QString line = mDocument->getLine(i - 1);
        if (line.isEmpty())
            continue;
        if (line[0]!=' ' && line[0]!='\t')
            continue;
        int charsToDelete = 0;
        while (charsToDelete < tabSize() &&
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
        mDocument->putLine(i-1,tempString);
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
    endEditing();
}

void QSynEdit::doAddChar(const QChar& ch)
{
    if (mReadOnly)
        return;
    if (!ch.isPrint() && ch!='\t')
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
        default:
            setSelLength(1);
        }
    }

    QChar lastCh{0};
    if (!selAvail()) {
        PUndoItem undoItem = mUndoList->peekItem();
        if (undoItem && undoItem->changeReason()==ChangeReason::Insert
                && undoItem->changeEndPos().line == mCaretY
                && undoItem->changeEndPos().ch == mCaretX
                && undoItem->changeStartPos().line == mCaretY
                && undoItem->changeStartPos().ch == mCaretX-1) {
            QString s = mDocument->getLine(mCaretY-1);
            int i=mCaretX-2;
            if (i>=0 && i<s.length())
                lastCh=s[i];
        }
    }
    if (isIdentChar(ch)) {
        if (!isIdentChar(lastCh)) {
            mUndoList->addGroupBreak();
        }
        doSetSelText(ch);
    } else if (ch.isSpace()) {
        // break group undo chain
        if (!lastCh.isSpace()) {
            mUndoList->addGroupBreak();
        }
        doSetSelText(ch);
        // break group undo chain
//        if (mActiveSelectionMode!=SynSelectionMode::smColumn)
//            mUndoList->AddChange(SynChangeReason::crNothing,
//                                 BufferCoord{0, 0},
//                                 BufferCoord{0, 0},
//                                 "", SynSelectionMode::smNormal);
    } else {
        if (lastCh.isSpace() || isIdentChar(lastCh)) {
            mUndoList->addGroupBreak();
        }
        beginEditing();
        doSetSelText(ch);
        int oldCaretX=mCaretX-1;
        int oldCaretY=mCaretY;
        // auto
        if (mActiveSelectionMode==SelectionMode::Normal
                && mOptions.testFlag(EditorOption::AutoIndent)
                && mSyntaxer->language() == ProgrammingLanguage::CPP
                && (oldCaretY<=mDocument->count()) ) {

            //unindent if ':' at end of the line
            if (ch == ':') {
                QString line = mDocument->getLine(oldCaretY-1);
                if (line.length() <= oldCaretX) {
                    int indentSpaces = calcIndentSpaces(oldCaretY,line+":", true);
                    if (indentSpaces != leftSpaces(line)) {
                        QString newLine = GetLeftSpacing(indentSpaces,true) + trimLeft(line);
                        mDocument->putLine(oldCaretY-1,newLine);
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
            } else if (ch == '*') {
                QString line = mDocument->getLine(oldCaretY-1);
                if (line.length() <= oldCaretX) {
                    int indentSpaces = calcIndentSpaces(oldCaretY,line+"*", true);
                    if (indentSpaces != leftSpaces(line)) {
                        QString newLine = GetLeftSpacing(indentSpaces,true) + trimLeft(line);
                        mDocument->putLine(oldCaretY-1,newLine);
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
            } else if (ch == '{' || ch == '}' || ch == '#') {
                //Reindent line when add '{' '}' and '#' at the beginning
                QString left = mDocument->getLine(oldCaretY-1).mid(0,oldCaretX-1);
                // and the first nonblank char is this new {
                if (left.trimmed().isEmpty()) {
                    int indentSpaces = calcIndentSpaces(oldCaretY,ch, true);
                    if (indentSpaces != leftSpaces(left)) {
                        QString right = mDocument->getLine(oldCaretY-1).mid(oldCaretX-1);
                        QString newLeft = GetLeftSpacing(indentSpaces,true);
                        mDocument->putLine(oldCaretY-1,newLeft+right);
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
        endEditing();
    }
    //DoOnPaintTransient(ttAfter);
}

void QSynEdit::doCutToClipboard()
{
    if (mReadOnly)
        return;
    beginEditing();
    addCaretToUndo();
    addSelectionToUndo();
    if (!selAvail()) {
        doSelectLine();
    }
    internalDoCopyToClipboard(selText());
    setSelectedTextEmpty();
    endEditing();
    mUndoList->addGroupBreak();
}

void QSynEdit::doCopyToClipboard()
{
    bool selected=selAvail();
    if (!selected)
        doSelectLine();
    QString sText;
    sText = selText();
    internalDoCopyToClipboard(sText);
}

void QSynEdit::internalDoCopyToClipboard(const QString &s)
{
    QClipboard* clipboard=QGuiApplication::clipboard();
    clipboard->clear();
    clipboard->setText(s);
}

void QSynEdit::doPasteFromClipboard()
{
    if (mReadOnly)
        return;
    QClipboard* clipboard = QGuiApplication::clipboard();
    QString text = clipboard->text();
    if (text.isEmpty())
        return;
    //correctly handle spaces copied from wechat
//    text.replace(QChar(0x00A0),QChar(0x0020));
    beginEditing();
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
    endEditing();
}

void QSynEdit::incPaintLock()
{
    mPaintLock ++ ;
}

void QSynEdit::decPaintLock()
{
    Q_ASSERT(mPaintLock > 0);
    mPaintLock--;
    if (mPaintLock == 0 ) {
        if (mStateFlags.testFlag(StateFlag::HScrollbarChanged)) {
            updateHScrollbar();
        }
        if (mStateFlags.testFlag(StateFlag::VScrollbarChanged)) {
            updateVScrollbar();
        }
        if (mStatusChanges!=0)
            doOnStatusChange(mStatusChanges);
    }
}

SyntaxState QSynEdit::calcSyntaxStateAtLine(int line, const QString &newLineText)
{
    if (line == 0) {
        syntaxer()->resetState();
    } else {
        syntaxer()->setState(mDocument->getSyntaxState(line-1));
    }
    syntaxer()->setLine(newLineText,line);
    syntaxer()->nextToEol();
    return syntaxer()->getState();
}

int QSynEdit::calcLineAlignedTopPos(int currentValue, bool passFirstLine)
{
    int offset = currentValue % mTextHeight;
    if (offset!=0) {
        if (passFirstLine)
            currentValue += (mTextHeight - offset);
        else
            currentValue -= offset;
    }
    return currentValue;
}

void QSynEdit::ensureLineAlignedWithTop(void)
{
    int value = mTopPos;
    int offset = value % mTextHeight;
    if (offset!=0) {
        if (offset < mTextHeight / 3)
            value -= offset;
        else
            value += (mTextHeight - offset);
    }
    setTopPos(value);
}

int QSynEdit::clientWidth() const
{
    return viewport()->size().width();
}

int QSynEdit::clientHeight() const
{
    return viewport()->size().height();
}

int QSynEdit::clientTop() const
{
    return 0;
}

int QSynEdit::clientLeft() const
{
    return 0;
}

QRect QSynEdit::clientRect() const
{
    return QRect(0,0, clientWidth(), clientHeight());
}

void QSynEdit::synFontChanged()
{
    incPaintLock();
    recalcCharExtent();
    decPaintLock();
}


void QSynEdit::updateLastCaretX()
{
    mLastCaretColumn = displayX();
}

void QSynEdit::ensureCaretVisible()
{
    ensureCaretVisibleEx(false);
}

void QSynEdit::ensureCaretVisibleEx(bool ForceToMiddle)
{
    incPaintLock();
    auto action = finally([this]{
        decPaintLock();
    });
    // Make sure Y is visible
    int vCaretRow = displayY();
    if (ForceToMiddle) {
        if ((vCaretRow-1) * mTextHeight < mTopPos
                ||
                vCaretRow * mTextHeight > mTopPos + clientHeight() )
            setTopPos( (vCaretRow - (mLinesInWindow - 1) / 2-1) * mTextHeight);
    } else {
        if ((vCaretRow-1) * mTextHeight < mTopPos)
            setTopPos((vCaretRow - 1) * mTextHeight);
        else if (vCaretRow * mTextHeight > mTopPos + clientHeight() ) {
            int value = calcLineAlignedTopPos(vCaretRow * mTextHeight - clientHeight(), true);
            setTopPos(value);
        } else
            setTopPos(mTopPos);
    }
    // Make sure X is visible
    if (mDocument->maxLineWidth()<0) {
        if (ForceToMiddle)
            mStateFlags.setFlag(StateFlag::EnsureCaretVisibleForceMiddle, true);
        else
            mStateFlags.setFlag(StateFlag::EnsureCaretVisible, true);
        return;
    }
    int visibleX = displayX();
    if (visibleX < leftPos()) {
        if (viewWidth() / 3 >visibleX)
            setLeftPos(0);
        else if (viewWidth() > tabWidth() + mCharWidth)
            setLeftPos(std::max(0,visibleX - tabWidth()));
        else
            setLeftPos(visibleX);
    } else if (visibleX > viewWidth() + leftPos() - mCharWidth && viewWidth()>0)
        if (viewWidth() >= 3*mCharWidth )
            setLeftPos(visibleX - viewWidth() + 3*mCharWidth);
        else
            setLeftPos(visibleX - viewWidth() + mCharWidth);
    else
        setLeftPos(leftPos());
}

void QSynEdit::scrollWindow(int dx, int dy)
{
    int nx = horizontalScrollBar()->value()+dx;
    int ny = verticalScrollBar()->value()+dy;
    horizontalScrollBar()->setValue(nx);
    verticalScrollBar()->setValue(ny);
}

void QSynEdit::setCaretDisplayXY(const DisplayCoord &aPos, bool ensureCaretVisible)
{
    incPaintLock();
    internalSetCaretXY(displayToBufferPos(aPos), ensureCaretVisible);
    decPaintLock();
}

void QSynEdit::internalSetCaretXY(BufferCoord value, bool ensureVisible)
{
    value = ensureBufferCoordValid(value);
    if ((value.ch != mCaretX) || (value.line != mCaretY)) {
        incPaintLock();
        auto action = finally([this]{
            decPaintLock();
        });
        if (mCaretX != value.ch) {
            mCaretX = value.ch;
            mStatusChanges.setFlag(StatusChange::CaretX);
            invalidateLine(mCaretY);
        }
        if (mCaretY != value.line) {
            int oldCaretY = mCaretY;
            mCaretY = value.line;
            invalidateLine(mCaretY);
            invalidateGutterLine(mCaretY);
            invalidateLine(oldCaretY);
            invalidateGutterLine(oldCaretY);
            mStatusChanges.setFlag(StatusChange::CaretY);
        }
        // Call UpdateLastCaretX before DecPaintLock because the event handler it
        // calls could raise an exception, and we don't want fLastCaretX to be
        // left in an undefined state if that happens.
        updateLastCaretX();
        if (ensureVisible)
            ensureCaretVisible();
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

void QSynEdit::internalSetCaretX(int value)
{
    internalSetCaretXY(BufferCoord{value, mCaretY});
}

void QSynEdit::internalSetCaretY(int value)
{
    internalSetCaretXY(BufferCoord{mCaretX,value});
}

void QSynEdit::setStatusChanged(StatusChanges changes)
{
    mStatusChanges = mStatusChanges | changes;
    if (mPaintLock == 0)
        doOnStatusChange(mStatusChanges);
}

void QSynEdit::doOnStatusChange(StatusChanges)
{
    if (mStatusChanges.testFlag(StatusChange::CaretX)
            || mStatusChanges.testFlag(StatusChange::CaretY)) {
        qApp->inputMethod()->update(Qt::ImCursorPosition);
    }
    emit statusChanged(mStatusChanges);
    mStatusChanges = StatusChange::None;
}

void QSynEdit::updateHScrollbar()
{
    if (mPaintLock!=0) {
        mStateFlags.setFlag(StateFlag::HScrollbarChanged);
    } else {
        mStateFlags.setFlag(StateFlag::HScrollbarChanged,false);
        doUpdateHScrollbar();
    }
}

void QSynEdit::doUpdateHScrollbar()
{
    int nMin = 0;
    int nMax = maxScrollWidth();
    if (nMax<0)
        return;
    int nPage = viewWidth();
    int nPos = mLeftPos;
    horizontalScrollBar()->setMinimum(nMin);
    horizontalScrollBar()->setMaximum(nMax);
    horizontalScrollBar()->setPageStep(nPage);
    horizontalScrollBar()->setValue(nPos);
    horizontalScrollBar()->setSingleStep(mCharWidth);
    if (mStateFlags.testFlag(StateFlag::EnsureCaretVisible)) {
        ensureCaretVisibleEx(false);
        mStateFlags.setFlag(StateFlag::EnsureCaretVisible,false);
    } else if (mStateFlags.testFlag(StateFlag::EnsureCaretVisibleForceMiddle)) {
        ensureCaretVisibleEx(true);
        mStateFlags.setFlag(StateFlag::EnsureCaretVisibleForceMiddle,false);
    }
}

void QSynEdit::updateVScrollbar()
{
    if (mPaintLock!=0) {
        mStateFlags.setFlag(StateFlag::VScrollbarChanged);
    } else {
        mStateFlags.setFlag(StateFlag::VScrollbarChanged,false);
        doUpdateVScrollbar();
    }
}

void QSynEdit::doUpdateVScrollbar()
{
    int nMin = 0;
    int nMax = maxScrollHeight();
    int nPage = mLinesInWindow * mTextHeight;
    int nPos = mTopPos;
    verticalScrollBar()->setMinimum(nMin);
    verticalScrollBar()->setMaximum(nMax);
    verticalScrollBar()->setPageStep(nPage);
    verticalScrollBar()->setValue(nPos);
    verticalScrollBar()->setSingleStep(mTextHeight);
}

void QSynEdit::updateCaret()
{
    if (mDocument->maxLineWidth()<0)
        return;
    invalidateRect(calculateCaretRect());
}

void QSynEdit::recalcCharExtent()
{
    int currentTopRow = mTopPos / mTextHeight;
    int currentLeftCol = mLeftPos / mCharWidth;

    FontStyle styles[] = {FontStyle::fsBold, FontStyle::fsItalic, FontStyle::fsStrikeOut, FontStyle::fsUnderline};
    bool hasStyles[] = {false,false,false,false};
    int size = 4;
    if (mSyntaxer->attributes().count()>0) {
        for (const PTokenAttribute& attribute: mSyntaxer->attributes()) {
            for (int i=0;i<size;i++) {
                if (attribute->styles().testFlag(styles[i]))
                    hasStyles[i] = true;
            }
        }
    }

    QFontMetrics fm(font());
    mTextHeight = fm.lineSpacing();
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
    mTextHeight *= mLineSpacingFactor;

    onSizeOrFontChanged();
    int newTopPos =  currentTopRow * mTextHeight;
    setTopPos(newTopPos);
    if (newTopPos!=mTopPos)
        mTopPos = newTopPos;
    setLeftPos(currentLeftCol * mCharWidth);
}

void QSynEdit::updateModifiedStatus()
{
    bool oldModified = mModified;
    mModified = !mUndoList->initialState();
    setModified(mModified);
//    qDebug()<<mModified<<oldModified;
    if (oldModified!=mModified)
        emit statusChanged(StatusChange::ModifyChanged);
}

int QSynEdit::reparseLines(int startLine, int endLine, bool needRescanFolds, bool toDocumentEnd)
{

    SyntaxState state;
    int maxLine = toDocumentEnd ? mDocument->count() : endLine+1;
    startLine = std::max(0,startLine);
    endLine = std::min(endLine, mDocument->count());
    maxLine = std::min(maxLine, mDocument->count());


    if (startLine >= endLine)
        return startLine;

    if (startLine == 0) {
        mSyntaxer->resetState();
    } else {
        mSyntaxer->setState(mDocument->getSyntaxState(startLine-1));
    }
    int line = startLine;
    do {
        mSyntaxer->setLine(mDocument->getLine(line), line);
        mSyntaxer->nextToEol();
        state = mSyntaxer->getState();
        if (line >= endLine && state == mDocument->getSyntaxState(line)) {
            break;
        }
        mDocument->setSyntaxState(line,state);
        line++;
    } while (line < maxLine);

    //don't rescan folds if only currentLine is reparsed
    if (line-startLine==1)
        return line;

    if (mEditingCount>0)
        return line;

    if (needRescanFolds && useCodeFolding())
        rescanFolds();
    return line;
}

// void QSynEdit::reparseLine(int line)
// {
//     if (!mSyntaxer)
//         return;
//     line--;
//     line = std::max(0,line);
//     if (line >= mDocument->count())
//         return;

//     if (line == 0) {
//         mSyntaxer->resetState();
//     } else {
//         mSyntaxer->setState(mDocument->getSyntaxState(line-1));
//     }
//     mSyntaxer->setLine(mDocument->getLine(line), line);
//     mSyntaxer->nextToEol();
//     SyntaxState iRange = mSyntaxer->getState();
//     mDocument->setSyntaxState(line,iRange);
// }

void QSynEdit::reparseDocument()
{
    if (!mDocument->empty()) {
//        qint64 begin=QDateTime::currentMSecsSinceEpoch();
        mSyntaxer->resetState();
        for (int i =0;i<mDocument->count();i++) {
            mSyntaxer->setLine(mDocument->getLine(i), i);
            mSyntaxer->nextToEol();
            mDocument->setSyntaxState(i, mSyntaxer->getState());
        }
//        qint64 diff= QDateTime::currentMSecsSinceEpoch() - begin;

//        qDebug()<<diff<<mDocument->count();
    }
    if (useCodeFolding())
        rescanFolds();
}

void QSynEdit::uncollapse(PCodeFoldingRange FoldRange)
{
    FoldRange->linesCollapsed = 0;
    FoldRange->collapsed = false;

    // Redraw the collapsed line
    invalidateLines(FoldRange->fromLine, INT_MAX);

    // Redraw fold mark
    invalidateGutterLines(FoldRange->fromLine, INT_MAX);
    updateHScrollbar();
    updateVScrollbar();
}

void QSynEdit::collapse(PCodeFoldingRange FoldRange)
{
    FoldRange->linesCollapsed = FoldRange->toLine - FoldRange->fromLine;
    FoldRange->collapsed = true;

    // Extract caret from fold
    if ((mCaretY > FoldRange->fromLine) && (mCaretY <= FoldRange->toLine)) {
          setCaretXY(BufferCoord{mDocument->getLine(FoldRange->fromLine - 1).length() + 1,
                                 FoldRange->fromLine});
    }

    // Redraw the collapsed line
    invalidateLines(FoldRange->fromLine, INT_MAX);

    // Redraw fold mark
    invalidateGutterLines(FoldRange->fromLine, INT_MAX);

    updateHScrollbar();
    updateVScrollbar();
}

void QSynEdit::foldOnLinesInserted(int Line, int Count)
{
    // Delete collapsed inside selection
    for (int i = mAllFoldRanges->count()-1;i>=0;i--) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (range->fromLine == Line - 1) {// insertion starts at fold line
            if (range->collapsed)
                uncollapse(range);
        } else if (range->fromLine >= Line) // insertion of count lines above FromLine
            range->move(Count);
    }
}

void QSynEdit::foldOnLinesDeleted(int Line, int Count)
{
    // Delete collapsed inside selection
    for (int i = mAllFoldRanges->count()-1;i>=0;i--) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (range->fromLine == Line && Count == 1)  {// open up because we are messing with the starting line
            if (range->collapsed)
                uncollapse(range);
        } else if (range->fromLine >= Line - 1 && range->fromLine < Line + Count) // delete inside affectec area
            mAllFoldRanges->remove(i);
        else if (range->fromLine >= Line + Count) // Move after affected area
            range->move(-Count);

    }

}

void QSynEdit::foldOnListCleared()
{
    mAllFoldRanges->clear();
}

void QSynEdit::rescanFolds()
{
    if (!useCodeFolding())
        return;

    incPaintLock();
    rescanForFoldRanges();
    invalidateGutter();
    decPaintLock();
}

void QSynEdit::rescanForFoldRanges()
{
    // Delete all uncollapsed folds
//    for (int i=mAllFoldRanges.count()-1;i>=0;i--) {
//        PSynEditFoldRange range =mAllFoldRanges[i];
//        if (!range->collapsed && !range->parentCollapsed())
//            mAllFoldRanges.remove(i);
//    }

    // Did we leave any collapsed folds and are we viewing a code file?
    if (mAllFoldRanges->count() > 0) {
        QMap<QString,PCodeFoldingRange> rangeIndexes;
        foreach(const PCodeFoldingRange& r, mAllFoldRanges->ranges()) {
            if (r->collapsed)
                rangeIndexes.insert(QString("%1-%2").arg(r->fromLine).arg(r->toLine),r);
        }
        mAllFoldRanges->clear();
        // Add folds to a separate list
        PCodeFoldingRanges temporaryAllFoldRanges = std::make_shared<CodeFoldingRanges>();
        scanForFoldRanges(temporaryAllFoldRanges);

        PCodeFoldingRange tempFoldRange;
        PCodeFoldingRange r2;
        // Combine new with old folds, preserve parent order
        for (int i = 0; i< temporaryAllFoldRanges->count();i++) {
            tempFoldRange=temporaryAllFoldRanges->range(i);
            r2=rangeIndexes.value(QString("%1-%2").arg(tempFoldRange->fromLine).arg(tempFoldRange->toLine),
                                  PCodeFoldingRange());
            if (r2) {
                tempFoldRange->collapsed=true;
                tempFoldRange->linesCollapsed=r2->linesCollapsed;
            }
            mAllFoldRanges->add(tempFoldRange);
        }
    } else {
        // We ended up with no folds after deleting, just pass standard data...
        PCodeFoldingRanges temp{mAllFoldRanges};
        scanForFoldRanges(temp);
    }
}

void QSynEdit::scanForFoldRanges(PCodeFoldingRanges topFoldRanges)
{
    PCodeFoldingRanges parentFoldRanges = topFoldRanges;

    findSubFoldRange(topFoldRanges, parentFoldRanges,PCodeFoldingRange());
}

void QSynEdit::findSubFoldRange(PCodeFoldingRanges topFoldRanges, PCodeFoldingRanges& parentFoldRanges, PCodeFoldingRange parent)
{
    PCodeFoldingRange  collapsedFold;
    int line = 0;
    QString curLine;
    if (!useCodeFolding())
        return;

    while (line < mDocument->count()) { // index is valid for LinesToScan and fLines
        // If there is a collapsed fold over here, skip it
        // Find an opening character on this line
        curLine = mDocument->getLine(line);
        int blockEnded=mDocument->blockEnded(line);
        int blockStarted=mDocument->blockStarted(line);
        if (blockEnded>0) {
            for (int i=0; i<blockEnded;i++) {
                // Stop the recursion if we find a closing char, and return to our parent
                if (parent) {
                    if (blockStarted>0)
                        parent->toLine = line;
                    else
                        parent->toLine = line + 1;
                    parent = parent->parent.lock();
                    if (!parent) {
                        parentFoldRanges = topFoldRanges;
                    } else {
                        parentFoldRanges = parent->subFoldRanges;
                    }
                }
            }
        }
        if (blockStarted>0) {
            for (int i=0; i<blockStarted;i++) {
                // Add it to the top list of folds
                parent = parentFoldRanges->addByParts(
                  parent,
                  topFoldRanges,
                  line + 1,
                  line + 1);
                parentFoldRanges = parent->subFoldRanges;
            }
        }
        line++;
    }
}

PCodeFoldingRange QSynEdit::collapsedFoldStartAtLine(int Line)
{
    for (int i = 0; i< mAllFoldRanges->count() - 1; i++ ) {
        if ((*mAllFoldRanges)[i]->collapsed && (*mAllFoldRanges)[i]->fromLine == Line) {
            return (*mAllFoldRanges)[i];
        } else if ((*mAllFoldRanges)[i]->fromLine > Line) {
            break; // sorted by line. don't bother scanning further
        }
    }
    return PCodeFoldingRange();
}

void QSynEdit::initializeCaret()
{
    //showCaret();
}

PCodeFoldingRange QSynEdit::foldStartAtLine(int Line) const
{
    for (int i = 0; i<mAllFoldRanges->count();i++) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (range->fromLine == Line ){
            return range;
        } else if (range->fromLine>Line)
            break; // sorted by line. don't bother scanning further
    }
    return PCodeFoldingRange();
}

bool QSynEdit::foldCollapsedBetween(int startLine, int endLine) const
{
    for (int i = 0; i<mAllFoldRanges->count();i++) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (startLine >=range->fromLine && range->fromLine<=endLine
                && (range->collapsed || range->parentCollapsed())){
            return true;
        } else if (range->fromLine>endLine)
            break; // sorted by line. don't bother scanning further
    }
    return false;
}

// QString QSynEdit::substringByColumns(const QString &s, int startColumn, int &colLen)
// {

//     int len = s.length();
//     int columns = 0;
//     int i = 0;
//     int oldColumns=0;
//     QList<int> glyphPositions = calcGlyphPositions(s);
//     QList<int> glyphColumnList = mDocument;
//     while (columns < startColumn) {
//         oldColumns = columns;
//         if (i>=len)
//             break;
//         if (s[i] == '\t')
//             columns += tabWidth() - (columns % tabWidth());
//         else
//             columns += charColumns(s[i]);
//         i++;
//     }
//     QString result;
//     if (i>=len) {
//         colLen = 0;
//         return result;
//     }
//     if (colLen>result.capacity()) {
//         result.resize(colLen);
//     }
//     int j=0;
//     if (i>0) {
//         result[0]=s[i-1];
//         j++;
//     }
//     while (i<len && columns<startColumn+colLen) {
//         result[j]=s[i];
//         if (i < len && s[i] == '\t')
//             columns += tabWidth() - (columns % tabWidth());
//         else
//             columns += charColumns(s[i]);
//         i++;
//         j++;
//     }
//     result.resize(j);
//     colLen = columns-oldColumns;
//     return result;
// }

PCodeFoldingRange QSynEdit::foldAroundLine(int line)
{
    return foldAroundLineEx(line,false,false,false);
}

PCodeFoldingRange QSynEdit::foldAroundLineEx(int line, bool wantCollapsed, bool acceptFromLine, bool acceptToLine)
{
    // Check global list
    PCodeFoldingRange result = checkFoldRange(mAllFoldRanges, line, wantCollapsed, acceptFromLine, acceptToLine);

    // Found an item in the top level list?
    if (result) {
        while (true) {
            PCodeFoldingRange ResultChild = checkFoldRange(result->subFoldRanges, line, wantCollapsed, acceptFromLine, acceptToLine);
            if (!ResultChild)
                break;
            result = ResultChild; // repeat for this one
        }
    }
    return result;
}

PCodeFoldingRange QSynEdit::checkFoldRange(PCodeFoldingRanges foldRangesToCheck, int Line, bool WantCollapsed, bool AcceptFromLine, bool AcceptToLine)
{
    for (int i = 0; i< foldRangesToCheck->count(); i++) {
        PCodeFoldingRange range = (*foldRangesToCheck)[i];
        if (((range->fromLine < Line) || ((range->fromLine <= Line) && AcceptFromLine)) &&
          ((range->toLine > Line) || ((range->toLine >= Line) && AcceptToLine))) {
            if (range->collapsed == WantCollapsed) {
                return range;
            }
        }
    }
    return PCodeFoldingRange();
}

PCodeFoldingRange QSynEdit::foldEndAtLine(int Line)
{
    for (int i = 0; i<mAllFoldRanges->count();i++) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (range->toLine == Line ){
            return range;
        } else if (range->fromLine>Line)
            break; // sorted by line. don't bother scanning further
    }
    return PCodeFoldingRange();
}

void QSynEdit::paintCaret(QPainter &painter, const QRect rcClip)
{
    if (m_blinkStatus!=1)
        return;
    painter.setClipRect(rcClip);
    if (rcClip.left() < mGutterWidth)
        return;
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
    case EditCaretType::VerticalLine: {
        QRect caretRC;
        int size = std::max(1, mTextHeight/15);
        caretRC.setLeft(rcClip.left()+1);
        caretRC.setTop(rcClip.top());
        caretRC.setBottom(rcClip.bottom());
        caretRC.setRight(rcClip.left()+1+size);
        painter.fillRect(caretRC,caretColor);
        break;
    }
    case EditCaretType::HorizontalLine: {
        QRect caretRC;
        int size = std::max(1,mTextHeight/15);
        caretRC.setLeft(rcClip.left());
        caretRC.setTop(rcClip.bottom()-1-size);
        caretRC.setBottom(rcClip.bottom()-1);
        caretRC.setRight(rcClip.right());
        painter.fillRect(caretRC,caretColor);
        break;
    }
    case EditCaretType::Block:
        painter.fillRect(rcClip, caretColor);
        break;
    case EditCaretType::HalfBlock:
        QRect rc=rcClip;
        rc.setTop(rcClip.top()+rcClip.height() / 2);
        painter.fillRect(rcClip, caretColor);
        break;
    }
}

int QSynEdit::textOffset() const
{
    return mGutterWidth + 2 - mLeftPos ;
}

EditCommand QSynEdit::TranslateKeyCode(int key, Qt::KeyboardModifiers modifiers)
{
    PEditKeyStroke keyStroke = mKeyStrokes.findKeycode2(mLastKey,mLastKeyModifiers,
                                                           key, modifiers);
    EditCommand cmd=EditCommand::None;
    if (keyStroke)
        cmd = keyStroke->command();
    else {
        keyStroke = mKeyStrokes.findKeycode(key,modifiers);
        if (keyStroke)
            cmd = keyStroke->command();
    }
    if (cmd == EditCommand::None) {
        mLastKey = key;
        mLastKeyModifiers = modifiers;
    } else {
        mLastKey = 0;
        mLastKeyModifiers = Qt::NoModifier;
    }
    return cmd;
}

void QSynEdit::onSizeOrFontChanged()
{
    mLinesInWindow = clientHeight() / mTextHeight;
    if (mGutter.showLineNumbers())
        onGutterChanged();
    updateHScrollbar();
    updateVScrollbar();
    invalidate();
}

void QSynEdit::onChanged()
{
    emit changed();
}

void QSynEdit::onHScrolled(int value)
{
    mLeftPos = value;
    invalidate();
}

void QSynEdit::onVScrolled(int value)
{
    mTopPos = value;
    invalidate();
}


const PFormatter &QSynEdit::formatter() const
{
    return mFormatter;
}

void QSynEdit::setFormatter(const PFormatter &newFormatter)
{
    mFormatter = newFormatter;
}

const QDateTime &QSynEdit::lastModifyTime() const
{
    return mLastModifyTime;
}

double QSynEdit::lineSpacingFactor() const
{
    return mLineSpacingFactor;
}

void QSynEdit::setLineSpacingFactor(double newLineSpacingFactor)
{
    if (newLineSpacingFactor<1.0)
        newLineSpacingFactor = 1.0;
    if (mLineSpacingFactor != newLineSpacingFactor) {
        incPaintLock();
        mLineSpacingFactor = newLineSpacingFactor;
        recalcCharExtent();
        decPaintLock();
    }
}

ScrollStyle QSynEdit::scrollBars() const
{
    return mScrollBars;
}

void QSynEdit::setScrollBars(ScrollStyle newScrollBars)
{
    mScrollBars = newScrollBars;
    if (mScrollBars == ScrollStyle::Both ||  mScrollBars == ScrollStyle::OnlyHorizontal) {
        if (mOptions.testFlag(EditorOption::AutoHideScrollbars)) {
            setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
        } else {
            setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
        }
        updateHScrollbar();
    } else {
        setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    }
    if (mScrollBars == ScrollStyle::Both ||  mScrollBars == ScrollStyle::OnlyVertical) {
        if (mOptions.testFlag(EditorOption::AutoHideScrollbars)) {
            setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
        } else {
            setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOn);
        }
        updateVScrollbar();
    } else {
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    }
}

int QSynEdit::mouseSelectionScrollSpeed() const
{
    return mMouseSelectionScrollSpeed;
}

void QSynEdit::setMouseSelectionScrollSpeed(int newMouseSelectionScrollSpeed)
{
    mMouseSelectionScrollSpeed = newMouseSelectionScrollSpeed;
}

const QColor &QSynEdit::backgroundColor() const
{
    return mBackgroundColor;
}

void QSynEdit::setBackgroundColor(const QColor &newBackgroundColor)
{
    mBackgroundColor = newBackgroundColor;
}

bool QSynEdit::isEmpty()
{
    if (mDocument->count()>1)
        return false;
    if (mDocument->count()==1)
        return mDocument->getLine(0).isEmpty();
    return true;
}

const QColor &QSynEdit::foregroundColor() const
{
    return mForegroundColor;
}

void QSynEdit::setForegroundColor(const QColor &newForegroundColor)
{
    mForegroundColor = newForegroundColor;
}

int QSynEdit::mouseWheelScrollSpeed() const
{
    return mMouseWheelScrollSpeed;
}

void QSynEdit::setMouseWheelScrollSpeed(int newMouseWheelScrollSpeed)
{
    mMouseWheelScrollSpeed = newMouseWheelScrollSpeed;
}

const PTokenAttribute &QSynEdit::rainbowAttr3() const
{
    return mRainbowAttr3;
}

const PTokenAttribute &QSynEdit::rainbowAttr2() const
{
    return mRainbowAttr2;
}

const PTokenAttribute &QSynEdit::rainbowAttr1() const
{
    return mRainbowAttr1;
}

const PTokenAttribute &QSynEdit::rainbowAttr0() const
{
    return mRainbowAttr0;
}

bool QSynEdit::caretUseTextColor() const
{
    return mCaretUseTextColor;
}

void QSynEdit::setCaretUseTextColor(bool newCaretUseTextColor)
{
    mCaretUseTextColor = newCaretUseTextColor;
}

const QColor &QSynEdit::rightEdgeColor() const
{
    return mRightEdgeColor;
}

void QSynEdit::setRightEdgeColor(const QColor &newRightEdgeColor)
{
    if (newRightEdgeColor!=mRightEdgeColor) {
        mRightEdgeColor = newRightEdgeColor;
    }
}

int QSynEdit::rightEdge() const
{
    return mRightEdge;
}

void QSynEdit::setRightEdge(int newRightEdge)
{
    if (mRightEdge != newRightEdge) {
        incPaintLock();
        mRightEdge = newRightEdge;
        invalidate();
        decPaintLock();
    }
}

const QColor &QSynEdit::selectedBackground() const
{
    return mSelectedBackground;
}

void QSynEdit::setSelectedBackground(const QColor &newSelectedBackground)
{
    mSelectedBackground = newSelectedBackground;
}

const QColor &QSynEdit::selectedForeground() const
{
    return mSelectedForeground;
}

void QSynEdit::setSelectedForeground(const QColor &newSelectedForeground)
{
    mSelectedForeground = newSelectedForeground;
}

int QSynEdit::textHeight() const
{
    return mTextHeight;
}

bool QSynEdit::readOnly() const
{
    return mReadOnly;
}

void QSynEdit::setReadOnly(bool readOnly)
{
    if (mReadOnly != readOnly) {
        mReadOnly = readOnly;
        emit statusChanged(StatusChange::ReadOnly);
    }
}

Gutter& QSynEdit::gutter()
{
    return mGutter;
}

EditCaretType QSynEdit::insertCaret() const
{
    return mInsertCaret;
}

void QSynEdit::setInsertCaret(const EditCaretType &insertCaret)
{
    mInsertCaret = insertCaret;
}

EditCaretType QSynEdit::overwriteCaret() const
{
    return mOverwriteCaret;
}

void QSynEdit::setOverwriteCaret(const EditCaretType &overwriteCaret)
{
    mOverwriteCaret = overwriteCaret;
}

QColor QSynEdit::activeLineColor() const
{
    return mActiveLineColor;
}

void QSynEdit::setActiveLineColor(const QColor &activeLineColor)
{
    if (mActiveLineColor!=activeLineColor) {
        mActiveLineColor = activeLineColor;
        invalidateLine(mCaretY);
    }
}

QColor QSynEdit::caretColor() const
{
    return mCaretColor;
}

void QSynEdit::setCaretColor(const QColor &caretColor)
{
    mCaretColor = caretColor;
}

void QSynEdit::setTabSize(int newTabSize)
{
    if (newTabSize!=tabSize()) {
        incPaintLock();
        mDocument->setTabSize(newTabSize);
        invalidate();
        decPaintLock();
    }
}

int QSynEdit::tabWidth() const
{
    return mDocument->tabWidth();
}

EditorOptions QSynEdit::getOptions() const
{
    return mOptions;
}

static bool sameEditorOption(const EditorOptions& value1, const EditorOptions& value2, EditorOption flag) {
    return value1.testFlag(flag)==value2.testFlag(flag);
}
void QSynEdit::setOptions(const EditorOptions &value)
{
    if (value != mOptions) {
        incPaintLock();
        bool bUpdateAll =
                !sameEditorOption(value,mOptions, EditorOption::ShowLeadingSpaces)
                || !sameEditorOption(value,mOptions, EditorOption::LigatureSupport)
                || !sameEditorOption(value,mOptions, EditorOption::ForceMonospace)
                || !sameEditorOption(value,mOptions, EditorOption::ShowInnerSpaces)
                || !sameEditorOption(value,mOptions, EditorOption::ShowTrailingSpaces)
                || !sameEditorOption(value,mOptions, EditorOption::ShowLineBreaks)
                || !sameEditorOption(value,mOptions, EditorOption::ShowRainbowColor);
        mOptions = value;

        setScrollBars(mScrollBars);
        mDocument->setForceMonospace(mOptions.testFlag(EditorOption::ForceMonospace) );

        // constrain caret position to MaxScrollWidth if eoScrollPastEol is enabled
        internalSetCaretXY(caretXY());
        if (mOptions.testFlag(EditorOption::ScrollPastEol)) {
            BufferCoord vTempBlockBegin = blockBegin();
            BufferCoord vTempBlockEnd = blockEnd();
            setBlockBegin(vTempBlockBegin);
            setBlockEnd(vTempBlockEnd);
        }
        updateHScrollbar();
        updateVScrollbar();
        if (bUpdateAll)
            invalidate();
        decPaintLock();
    }
}

int QSynEdit::tabSize() const
{
    return mDocument->tabSize();
}

void QSynEdit::doAddStr(const QString &s)
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
        default:
            setSelLength(s.length());
        }
    }
    doSetSelText(s);
}

void QSynEdit::doUndo()
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
                        keepGoing = (mOptions.testFlag(EditorOption::GroupUndo) &&
                            (lastChange == item->changeReason()) );
                    }
                    oldChangeNumber=item->changeNumber();
                    lastChange = item->changeReason();
                }
            } while (keepGoing);
        }
    }
    ensureCaretVisible();
    updateModifiedStatus();
    onChanged();
}

void QSynEdit::doUndoItem()
{
    mUndoing = true;
    beginEditing();
    bool ChangeScrollPastEol = ! mOptions.testFlag(EditorOption::ScrollPastEol);
    mOptions.setFlag(EditorOption::ScrollPastEol);
    auto action = finally([&,this]{
        endEditing();
        if (ChangeScrollPastEol)
            mOptions.setFlag(EditorOption::ScrollPastEol,false);
        mUndoing = false;
    });

    PUndoItem item = mUndoList->popItem();
    if (item) {
        setActiveSelectionMode(item->changeSelMode());
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
            p.ch = leftPos();
            p.line = topPos();
            mRedoList->addRedo(
                        item->changeReason(),
                        p,
                        p, QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            setLeftPos(item->changeStartPos().ch);
            setTopPos(item->changeStartPos().line);
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
            setBlockBegin(caretXY());
            ensureCaretVisible();
            break;
        }
        case ChangeReason::ReplaceLine:
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(mDocument->getLine(item->changeStartPos().line-1)),
                        item->changeSelMode(),
                        item->changeNumber()
                        );
            mDocument->putLine(item->changeStartPos().line-1,item->changeText()[0]);
            break;
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
            BufferCoord startPos = item->changeStartPos();
            BufferCoord endPos = item->changeEndPos();
            if (item->changeSelMode()==SelectionMode::Column) {
                int xFrom = charToGlyphLeft(startPos.line, startPos.ch);
                int xTo = charToGlyphLeft(endPos.line, endPos.ch);
                if (xFrom > xTo)
                    std::swap(xFrom, xTo);
                startPos.ch = xposToGlyphStartChar(startPos.line,xFrom);
            }
            doInsertText(startPos,item->changeText(),item->changeSelMode(),
                         startPos.line,
                         endPos.line);
            internalSetCaretXY(item->changeEndPos());
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            setBlockBegin(caretXY());
            ensureCaretVisible();
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
                QString TmpStr = mDocument->getLine(mCaretY - 1);
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

void QSynEdit::doRedo()
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
            keepGoing = (mOptions.testFlag(EditorOption::GroupUndo) &&
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
    ensureCaretVisible();
    updateModifiedStatus();
    onChanged();
}

void QSynEdit::doRedoItem()
{
    mUndoing = true;
    bool ChangeScrollPastEol = ! mOptions.testFlag(EditorOption::ScrollPastEol);
    mOptions.setFlag(EditorOption::ScrollPastEol);
    mUndoList->setInsideRedo(true);
    beginEditing();
    auto action = finally([&,this]{
        endEditing();
        mUndoList->setInsideRedo(false);
        if (ChangeScrollPastEol)
            mOptions.setFlag(EditorOption::ScrollPastEol,false);
        mUndoing = false;
    });
    PUndoItem item = mRedoList->popItem();
    if (item) {
        setActiveSelectionMode(item->changeSelMode());
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
            p.ch = leftPos();
            p.line = topPos();
            mUndoList->restoreChange(
                        item->changeReason(),
                        p,
                        p, QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            setLeftPos(item->changeStartPos().ch);
            setTopPos(item->changeStartPos().line);
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
        case ChangeReason::ReplaceLine:
            mUndoList->restoreChange(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(mDocument->getLine(item->changeStartPos().line-1)),
                        item->changeSelMode(),
                        item->changeNumber()
                        );
            mDocument->putLine(item->changeStartPos().line-1,item->changeText()[0]);
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
            processCommand(EditCommand::LineBreak);
            break;
        }
        default:
            break;
        }
    }
}

void QSynEdit::doZoomIn()
{
    QFont newFont = font();
    int size = newFont.pixelSize();
    size++;
    newFont.setPixelSize(size);
    setFont(newFont);
}

void QSynEdit::doZoomOut()
{
    QFont newFont = font();
    int size = newFont.pixelSize();
    size--;
    if (size<2)
        size = 2;
    newFont.setPixelSize(size);
    setFont(newFont);
}

QString QSynEdit::selText() const
{
    if (!selAvail()) {
        return "";
    } else {
        int firstLine = blockBegin().line - 1;
        int lastLine = blockEnd().line - 1;

        switch(mActiveSelectionMode) {
        case SelectionMode::Normal:{
            int charFrom = blockBegin().ch;
            int charTo = blockEnd().ch;
            PCodeFoldingRange foldRange = foldStartAtLine(blockEnd().line);
            QString s = mDocument->getLine(lastLine);
            if ((foldRange) && foldRange->collapsed && charTo>s.length()) {
                s=s+mSyntaxer->foldString(s);
                if (charTo>s.length()) {
                    lastLine = foldRange->toLine-1;
                    charTo = mDocument->getLine(lastLine).length()+1;
                }
            }
            if (firstLine == lastLine)
                return  mDocument->getLine(firstLine).mid(charFrom-1, charTo - charFrom);
            else {
                QString result = mDocument->getLine(firstLine).mid(charFrom-1);
                result+= lineBreak();
                for (int i = firstLine + 1; i<=lastLine - 1; i++) {
                    result += mDocument->getLine(i);
                    result+=lineBreak();
                }
                const QString &line = mDocument->getLine(lastLine);
                result.append(line.constData(), charTo-1);
                return result;
            }
        }
        case SelectionMode::Column:
        {
            int xFrom, xTo;
            firstLine = blockBegin().line;
            xFrom = charToGlyphLeft(blockBegin().line, blockBegin().ch);
            lastLine = blockEnd().line;
            xTo = charToGlyphLeft(blockEnd().line, blockEnd().ch);
            if (xFrom > xTo)
              std::swap(xFrom, xTo);
            if (firstLine>lastLine)
              std::swap(firstLine,lastLine);
            QString result;
            for (int i = firstLine; i <= lastLine; i++) {
              int l = xposToGlyphStartChar(i,xFrom);
              int r = xposToGlyphStartChar(i,xTo);
              QString s = mDocument->getLine(i-1);
              result += s.mid(l-1,r-l);
              if (i<lastLine)
                  result+=lineBreak();
            }
            return result;
        }
        }
    }
    return "";
}

int QSynEdit::selCount() const
{
    if (!selAvail())
        return 0;
    if (mActiveSelectionMode == SelectionMode::Column)
        return selText().length();
    BufferCoord begin=blockBegin();
    BufferCoord end = blockEnd();
    if (begin.line == end.line)
        return (end.ch - begin.ch);
    int count = mDocument->getLine(begin.line-1).length()+1-begin.ch;
    int lineEndCount = lineBreak().length();
    count += lineEndCount;
    for (int i=begin.line;i<end.line-1;i++) {
        count += mDocument->getLine(i).length();
        count += lineEndCount;
    }
    count+= end.ch-1;
    return count;
}

QStringList QSynEdit::getContent(BufferCoord startPos, BufferCoord endPos, SelectionMode mode) const
{
    QStringList result;
    if (startPos==endPos) {
        return result;
    }
    if (startPos>endPos) {
        std::swap(startPos,endPos);
    }
    int firstLine = startPos.line - 1;
    int lastLine = endPos.line - 1;

    switch(mode) {
    case SelectionMode::Normal:{
        int chFrom = startPos.ch;
        int chTo = endPos.ch;
        PCodeFoldingRange foldRange = foldStartAtLine(endPos.line);
        QString s = mDocument->getLine(lastLine);
        if ((foldRange) && foldRange->collapsed && chTo>s.length()) {
            s=s+mSyntaxer->foldString(s);
            if (chTo>s.length()) {
                lastLine = foldRange->toLine-1;
                chTo = mDocument->getLine(lastLine).length()+1;
            }
        }
        if (firstLine == lastLine) {
            result.append(mDocument->getLine(firstLine).mid(chFrom-1, chTo - chFrom));
        } else {
            result.append(mDocument->getLine(firstLine).mid(chFrom-1));
            for (int i = firstLine + 1; i<=lastLine - 1; i++) {
                result.append(mDocument->getLine(i));
            }
            result.append(mDocument->getLine(lastLine).left(chTo-1));
        }
    }
        break;
    case SelectionMode::Column: {
        int xFrom, xTo;
        firstLine = blockBegin().line;
        xFrom = charToGlyphLeft(blockBegin().line, blockBegin().ch);
        lastLine = blockEnd().line;
        xTo = charToGlyphLeft(blockEnd().line, blockEnd().ch);
        if (xFrom > xTo)
          std::swap(xFrom, xTo);
        if (firstLine>lastLine)
          std::swap(firstLine,lastLine);
        for (int i = firstLine; i <= lastLine; i++) {
          int l = xposToGlyphStartChar(i,xFrom);
          int r = xposToGlyphStartChar(i,xTo-1)+1;
          QString s = mDocument->getLine(i-1);
          result.append(s.mid(l-1,r-l));
        }
    }
        break;
    }
    return result;
}

QString QSynEdit::lineBreak() const
{
    return mDocument->lineBreak();
}

bool QSynEdit::useCodeFolding() const
{
    return mUseCodeFolding && mSyntaxer->supportFolding();
}

void QSynEdit::setUseCodeFolding(bool value)
{
    if (mUseCodeFolding!=value) {
        mUseCodeFolding = value;
    }
}

CodeFoldingOptions &QSynEdit::codeFolding()
{
    return mCodeFolding;
}

QString QSynEdit::displayLineText()
{
    if (mCaretY >= 1 && mCaretY <= mDocument->count()) {
        QString s= mDocument->getLine(mCaretY - 1);
        PCodeFoldingRange foldRange = foldStartAtLine(mCaretY);
        if ((foldRange) && foldRange->collapsed) {
            return s+mSyntaxer->foldString(s);
        }
        return s;
    }
    return QString();
}

QString QSynEdit::lineText() const
{
    if (mCaretY >= 1 && mCaretY <= mDocument->count())
        return mDocument->getLine(mCaretY - 1);
    else
        return QString();
}

QString QSynEdit::lineText(int line) const
{
    return mDocument->getLine(line-1);
}

void QSynEdit::setLineText(const QString s)
{
    if (mCaretY >= 1 && mCaretY <= mDocument->count())
        mDocument->putLine(mCaretY-1,s);
}

PSyntaxer QSynEdit::syntaxer() const
{
    return mSyntaxer;
}

void QSynEdit::setSyntaxer(const PSyntaxer &syntaxer)
{
    Q_ASSERT(syntaxer!=nullptr);
    PSyntaxer oldSyntaxer = mSyntaxer;
    mSyntaxer = syntaxer;
    if (oldSyntaxer ->language() != syntaxer->language()) {
        recalcCharExtent();
        mDocument->beginUpdate();
        reparseDocument();
        mDocument->endUpdate();
    }
    mDocument->invalidateAllNonTempLineWidth();
    invalidate();
}

const PDocument& QSynEdit::document() const
{
    return mDocument;
}

bool QSynEdit::empty()
{
    return mDocument->empty();
}

void QSynEdit::processCommand(EditCommand Command, QChar AChar, void *pData)
{
    // first the program event handler gets a chance to process the command
    onProcessCommand(Command, AChar, pData);
    if (Command != EditCommand::None)
        executeCommand(Command, AChar, pData);
    onCommandProcessed(Command, AChar, pData);
}

void QSynEdit::moveCaretHorz(int deltaX, bool isSelection)
{
    BufferCoord ptDst = caretXY();
    QString s = displayLineText();
    int nLineLen = s.length();
    if (!isSelection && selAvail() && (deltaX!=0)) {
        if (deltaX<0)
            ptDst = blockBegin();
        else
            ptDst = blockEnd();
    } else {
        if (mOptions.testFlag(EditorOption::AltSetsColumnMode)) {
            if (qApp->keyboardModifiers().testFlag(Qt::AltModifier) && !mReadOnly) {
                setActiveSelectionMode(SelectionMode::Column);
            } else
                setActiveSelectionMode(SelectionMode::Normal);
        }

        // only moving or selecting one char can change the line
        //bool bChangeY = !mOptions.testFlag(SynEditorOption::eoScrollPastEol);
        bool bChangeY=true;
        int glyphIndex = mDocument->charToGlyphIndex(ptDst.line-1,ptDst.ch-1);
        if (bChangeY && (deltaX == -1) && (glyphIndex==0) && (ptDst.line > 1)) {
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
        } else if (bChangeY && (deltaX == 1) && (glyphIndex >= mDocument->glyphCount(ptDst.line-1)) && (ptDst.line < mDocument->count())) {
            // start of next line
            if (mActiveSelectionMode==SelectionMode::Column) {
                return;
            }
            int row = lineToRow(ptDst.line);
            row++;
            int line = rowToLine(row);
            if (line!=ptDst.line && line<=mDocument->count()) {
                ptDst.line = line;
                ptDst.ch = 1;
            }
        } else {
            ptDst.ch = std::max(1, mDocument->glyphStartChar(ptDst.line-1, glyphIndex + deltaX)+1);
            // don't go past last char when ScrollPastEol option not set
            if ((deltaX > 0) && bChangeY)
              ptDst.ch = std::min(ptDst.ch, nLineLen + 1);
        }
    }
    // set caret and block begin / end
    incPaintLock();
    ensureCaretVisible();
    moveCaretAndSelection(mBlockBegin, ptDst, isSelection);
    decPaintLock();
}

void QSynEdit::moveCaretVert(int deltaY, bool isSelection)
{
    DisplayCoord ptO = displayXY();
    DisplayCoord ptDst = ptO;
    if (!isSelection && selAvail()) {
        if (deltaY<0)
            ptDst = bufferToDisplayPos(blockBegin());
        else
            ptDst = bufferToDisplayPos(blockEnd());
    }
    ptDst.row+=deltaY;

    if (deltaY >= 0) {
        if (rowToLine(ptDst.row) > mDocument->count()) {
            ptDst.row = std::max(1, displayLineCount());
        }
    } else {
        if (ptDst.row < 1) {
            ptDst.row = 1;
        }
    }

    if (ptO.row != ptDst.row) {
        if (mOptions.testFlag(EditorOption::KeepCaretX))
            ptDst.x = mLastCaretColumn;
    }
    if (mOptions.testFlag(EditorOption::AltSetsColumnMode)) {
        if (qApp->keyboardModifiers().testFlag(Qt::AltModifier) && !mReadOnly)
            setActiveSelectionMode(SelectionMode::Column);
        else
            setActiveSelectionMode(SelectionMode::Normal);
    }

    BufferCoord vDstLineChar;
    if (ptDst.row == ptO.row && isSelection && deltaY!=0) {
        if (ptDst.row==1 && mDocument->count()>1) {
            vDstLineChar.ch=1;
            vDstLineChar.line=1;
        } else {
            vDstLineChar.line = mDocument->count();
            vDstLineChar.ch = mDocument->getLine(vDstLineChar.line-1).length()+1;
        }
    } else
        vDstLineChar = displayToBufferPos(ptDst);

    if (mActiveSelectionMode==SelectionMode::Column) {
        int w=mDocument->lineWidth(vDstLineChar.line-1);
        if (w+1<ptO.x)
            return;
    }

    int SaveLastCaretX = mLastCaretColumn;

    // set caret and block begin / end
    incPaintLock();
    ensureCaretVisible();
    moveCaretAndSelection(mBlockBegin, vDstLineChar, isSelection);
    decPaintLock();

    // Restore fLastCaretX after moving caret, since
    // UpdateLastCaretX, called by SetCaretXYEx, changes them. This is the one
    // case where we don't want that.
    mLastCaretColumn = SaveLastCaretX;
}

void QSynEdit::moveCaretAndSelection(const BufferCoord &ptBefore, const BufferCoord &ptAfter, bool isSelection, bool ensureCaretVisible)
{
    if (mOptions.testFlag(EditorOption::GroupUndo)) {
        mUndoList->addGroupBreak();
    }

    incPaintLock();
    if (isSelection) {
        if (!selAvail())
          setBlockBegin(ptBefore);
        setBlockEnd(ptAfter);
    } else
        setBlockBegin(ptAfter);
    internalSetCaretXY(ptAfter,ensureCaretVisible);
    decPaintLock();
}

void QSynEdit::moveCaretToLineStart(bool isSelection)
{
    int newX;
    // home key enhancement
    if (mOptions.testFlag(EditorOption::EnhanceHomeKey)) {
        QString s = mDocument->getLine(mCaretY - 1);

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

void QSynEdit::moveCaretToLineEnd(bool isSelection, bool ensureCaretVisible)
{
    int vNewX;
    if (mOptions.testFlag(EditorOption::EnhanceEndKey)) {
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

    moveCaretAndSelection(caretXY(), BufferCoord{vNewX, mCaretY}, isSelection, ensureCaretVisible);
}

void QSynEdit::doGotoBlockStart(bool isSelection)
{
    if (mCaretY<0 || mCaretY>lineCount())
        return;
    SyntaxState state = document()->getSyntaxState(mCaretY-1);
    //todo: handle block other than {}
    if (document()->braceLevel(mCaretY-1)==0) {
        doGotoEditorStart(isSelection);
    } else if (document()->blockStarted(mCaretY-1)==0){
        int line=mCaretY-1;
        while (line>=1) {
            if (document()->blockStarted(line-1)>document()->blockEnded(line-1)) {
                moveCaretVert(line+1-mCaretY, isSelection);
                moveCaretToLineStart(isSelection);
                line = lineToRow(line);
                setTopPos((line-1)*mTextHeight);
                return;
            }
            line--;
        }
    }
}

void QSynEdit::doGotoBlockEnd(bool isSelection)
{
    if (mCaretY<0 || mCaretY>lineCount())
        return;
    SyntaxState state = document()->getSyntaxState(mCaretY-1);
    //todo: handle block other than {}
    if (document()->blockLevel(mCaretY-1)==0) {
        doGotoEditorEnd(isSelection);
    } else if (document()->blockEnded(mCaretY-1)==0){
        int line=mCaretY+1;
        while (line<=lineCount()) {
            if (document()->blockEnded(line-1)>document()->blockStarted(line-1)) {
                moveCaretVert(line-1-mCaretY, isSelection);
                moveCaretToLineStart(isSelection);
                line = lineToRow(line) - mLinesInWindow + 1;
                setTopPos((line-1)*mTextHeight);
                return;
            }
            line++;
        }
    }
}

void QSynEdit::doGotoEditorStart(bool isSelection)
{
    moveCaretVert(1-mCaretY, isSelection);
    moveCaretToLineStart(isSelection);
}

void QSynEdit::doGotoEditorEnd(bool isSelection)
{
    if (!mDocument->empty()) {
        moveCaretVert(mDocument->count()-mCaretY, isSelection);
        moveCaretToLineEnd(isSelection);
    }
}

void QSynEdit::setSelectedTextEmpty()
{
    BufferCoord startPos=blockBegin();
    BufferCoord endPos=blockEnd();
    doDeleteText(startPos,endPos,mActiveSelectionMode);
    internalSetCaretXY(startPos);
}

void QSynEdit::setSelTextPrimitive(const QStringList &text)
{
    setSelTextPrimitiveEx(mActiveSelectionMode, text);
}

void QSynEdit::setSelTextPrimitiveEx(SelectionMode mode, const QStringList &text)
{
    incPaintLock();
    bool groupUndo=false;
    BufferCoord startPos = blockBegin();
    BufferCoord endPos = blockEnd();
    if (selAvail()) {
        if (!mUndoing && !text.isEmpty()) {
            beginEditing();
            groupUndo=true;
        }
        doDeleteText(startPos,endPos,activeSelectionMode());
        if (mode == SelectionMode::Column) {
            int xBegin = charToGlyphLeft(startPos.line,startPos.ch);
            int xEnd = charToGlyphLeft(endPos.line,endPos.ch);
            int x;
            if (xBegin<xEnd) {
                internalSetCaretXY(startPos);
                x=xBegin;
            } else {
                internalSetCaretXY(endPos);
                x=xEnd;
            }
            startPos.ch = xposToGlyphStartChar(startPos.line, x);
            endPos.ch = xposToGlyphStartChar(endPos.line, x);
            setBlockBegin(startPos);
            setBlockEnd(endPos);
        } else
            internalSetCaretXY(startPos);
    }
    if (!text.isEmpty()) {
        doInsertText(caretXY(),text,mode,mBlockBegin.line,mBlockEnd.line);
    }
    if (groupUndo) {
        endEditing();
    }
    decPaintLock();
    setStatusChanged(StatusChange::Selection);
}

void QSynEdit::doSetSelText(const QString &value)
{
    bool blockBeginned = false;
    auto action = finally([this, &blockBeginned]{
        if (blockBeginned)
            endEditing();
    });
    if (selAvail()) {
      beginEditing();
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

int QSynEdit::searchReplace(const QString &sSearch, const QString &sReplace, SearchOptions sOptions, PSynSearchBase searchEngine,
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
        if (mActiveSelectionMode == SelectionMode::Column) {
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
        ptStart.ch = 0;
        ptStart.line = 1;
        ptEnd.line = mDocument->count();
        ptEnd.ch = mDocument->getLine(ptEnd.line - 1).length()+1;
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
                endEditing();
            }
        });
        int i;
        // If it's a search only we can leave the procedure now.
        SearchAction searchAction = SearchAction::Exit;
        while ((ptCurrent.line >= ptStart.line) && (ptCurrent.line <= ptEnd.line)) {
            int nInLine = searchEngine->findAll(mDocument->getLine(ptCurrent.line - 1));
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
//                    qDebug()<<ptStart.line<<ptStart.ch<<ptEnd.line<<ptEnd.ch<<ptCurrent.line<<first<<last;
                    if  ((nSearchLen==0) &&
                         (((ptCurrent.line == ptStart.line) && (first == ptStart.ch) && !bBackward)
                          ||  ((ptCurrent.line == ptEnd.line) && (last == ptEnd.ch) && bBackward))
                         ) {
                        isInValidSearchRange = false;
                    } else if (((ptCurrent.line == ptStart.line) && (first < ptStart.ch)) ||
                            ((ptCurrent.line == ptEnd.line) && (last > ptEnd.ch))) {
                        isInValidSearchRange = false;
                    }
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
                internalSetCaretXY(BufferCoord{ptCurrent.ch, ptCurrent.line}, false);
                ensureCaretVisibleEx(true);
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
                        beginEditing();
                        dobatchReplace = true;
                    }
                    bool oldAutoIndent = mOptions.testFlag(EditorOption::AutoIndent);
                    mOptions.setFlag(EditorOption::AutoIndent,false);
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
                    mOptions.setFlag(EditorOption::AutoIndent,oldAutoIndent);
                }
            }

            // search next / previous line
            if (bBackward) {
                ptCurrent.line--;
            } else {
                ptCurrent.line++;
            }
            if (((ptCurrent.line < ptStart.line) || (ptCurrent.line > ptEnd.line))
                    && bFromCursor ){
                if (!sOptions.testFlag(ssoWrapAround) && confirmAroundCallback && !confirmAroundCallback())
                    break;
                //search start from cursor, search has finished but no result founds
                bFromCursor = false;
                ptStart.ch = 0;
                ptStart.line = 1;
                ptEnd.line = mDocument->count();
                ptEnd.ch = mDocument->getLine(ptEnd.line - 1).length()+1;
                if (bBackward) {
                    ptStart = originCaretXY;
                    ptEnd.ch++;
                    ptCurrent = ptEnd;
                } else {
                    ptEnd= originCaretXY;
                    ptStart.ch--;
                    ptCurrent = ptStart;
                }
            }
        }
    }
    return result;
}

void QSynEdit::doLinesDeleted(int firstLine, int count)
{
    emit linesDeleted(firstLine, count);
}

void QSynEdit::doLinesInserted(int firstLine, int count)
{
    emit linesInserted(firstLine, count);
}

void QSynEdit::properSetLine(int ALine, const QString &ALineText, bool notify)
{
    mDocument->putLine(ALine,ALineText,notify);
}

void QSynEdit::doDeleteText(BufferCoord startPos, BufferCoord endPos, SelectionMode mode)
{
    int MarkOffset = 0;
    if (mode == SelectionMode::Normal) {
        PCodeFoldingRange foldRange = foldStartAtLine(endPos.line);
        QString s = mDocument->getLine(endPos.line-1);
        if ((foldRange) && foldRange->collapsed && endPos.ch>s.length()) {
            QString newS=s+mSyntaxer->foldString(s);
            if ((startPos.ch<=s.length() || startPos.line<endPos.line)
                    && endPos.ch>newS.length() ) {
                //selection has whole block
                endPos.line = foldRange->toLine;
                endPos.ch = mDocument->getLine(endPos.line-1).length()+1;
            } else {
                return;
            }
        }
    }
    QStringList deleted=getContent(startPos,endPos,mode);
    beginEditingWithoutUndo();
    switch(mode) {
    case SelectionMode::Normal:
        if (mDocument->count() > 0) {
            // Create a string that contains everything on the first line up
            // to the selection mark, and everything on the last line after
            // the selection mark.
            QString TempString = mDocument->getLine(startPos.line - 1).mid(0, startPos.ch - 1)
                + mDocument->getLine(endPos.line - 1).mid(endPos.ch-1);
            // Delete all lines in the selection range.
            mDocument->deleteLines(startPos.line, endPos.line - startPos.line);
            properSetLine(startPos.line-1,TempString);
            internalSetCaretXY(startPos);
            doLinesDeleted(startPos.line, endPos.line - startPos.line + MarkOffset);
        }
        break;
    case SelectionMode::Column:
    {
        int firstLine = startPos.line;
        int xFrom = charToGlyphLeft(startPos.line, startPos.ch);
        int lastLine = endPos.line;
        int xTo = charToGlyphLeft(endPos.line, endPos.ch);
        if (xFrom > xTo)
            std::swap(xFrom, xTo);
        if (firstLine > lastLine)
            std::swap(firstLine,lastLine);
        QString result;
        for (int i = firstLine; i <= lastLine; i++) {
            int l = xposToGlyphStartChar(i,xFrom);
            int r = xposToGlyphStartChar(i,xTo);
            QString s = mDocument->getLine(i-1);
            s.remove(l-1,r-l);
            properSetLine(i-1,s);
        }
        // Lines never get deleted completely, so keep caret at end.
        BufferCoord newStartPos = startPos;
        BufferCoord newEndPos = endPos;
        newStartPos.line = firstLine;
        newStartPos.ch = xposToGlyphStartChar(newStartPos.line,xFrom);
        newEndPos.line = lastLine;
        newEndPos.ch = xposToGlyphStartChar(newEndPos.line, xFrom);
        internalSetCaretXY(newStartPos);
        setBlockBegin(newStartPos);
        setBlockEnd(newEndPos);
        // Column deletion never removes a line entirely, so no mark
        // updating is needed here.
        break;
    }
    }
    endEditingWithoutUndo();
    if (!mUndoing) {
        mUndoList->addChange(ChangeReason::Delete,
                             startPos,
                             endPos,
                             deleted,
                             mode);
    }
}

void QSynEdit::doInsertText(const BufferCoord& pos,
                           const QStringList& text,
                           SelectionMode mode, int startLine, int endLine) {
    if (text.isEmpty())
        return;
    if (startLine>endLine)
        std::swap(startLine,endLine);

    if (mode == SelectionMode::Normal) {
        PCodeFoldingRange foldRange = foldStartAtLine(pos.line);
        QString s = mDocument->getLine(pos.line-1);
        if ((foldRange) && foldRange->collapsed && pos.ch>s.length()+1)
            return;
    }
    int insertedLines = 0;
    BufferCoord newPos;
    switch(mode){
    case SelectionMode::Normal:
        insertedLines = doInsertTextByNormalMode(pos,text, newPos);
        doLinesInserted(pos.line+1, insertedLines);
        internalSetCaretXY(newPos);
        setBlockBegin(newPos);
        ensureCaretVisible();
        break;
    case SelectionMode::Column:{
        BufferCoord bb=blockBegin();
        BufferCoord be=blockEnd();
        int lenBefore = mDocument->getLine(be.line-1).length();
        insertedLines = doInsertTextByColumnMode(pos, text, startLine,endLine);
        doLinesInserted(endLine-insertedLines+1,insertedLines);
        if (!text.isEmpty()) {
            int textLen = mDocument->getLine(be.line-1).length()-lenBefore;
            bb.ch+=textLen;
            be.ch+=textLen;
            internalSetCaretXY(bb);
            setBlockBegin(bb);
            setBlockEnd(be);
            ensureCaretVisible();
        }
    }
        break;
    }

}

int QSynEdit::doInsertTextByNormalMode(const BufferCoord& pos, const QStringList& text, BufferCoord &newPos)
{
    QString sLeftSide;
    QString sRightSide;
    QString str;
    QElapsedTimer timer;
    bool bChangeScroll;
//    int SpaceCount;
    int result = 0;
    QString line=mDocument->getLine(pos.line-1);
    sLeftSide = line.mid(0, pos.ch - 1);
    if (pos.ch - 1 > sLeftSide.length()) {
        if (stringIsBlank(sLeftSide))
            sLeftSide = GetLeftSpacing(displayX() - 1, true);
        else
            sLeftSide += QString(pos.ch - 1 - sLeftSide.length(),' ');
    }
    sRightSide = line.mid(pos.ch - 1);
    int caretY=pos.line;
    // step1: insert the first line of Value into current line
    if (text.length()>1) {
        if (!mUndoing && mSyntaxer->language()==ProgrammingLanguage::CPP && mOptions.testFlag(EditorOption::AutoIndent)) {
            QString s = trimLeft(text[0]);
            if (sLeftSide.isEmpty()) {
                sLeftSide = GetLeftSpacing(calcIndentSpaces(caretY,s,true),true);
            }
            str = sLeftSide + s;
        } else
            str = sLeftSide + text[0];
        properSetLine(caretY - 1, str, false);
        mDocument->insertLines(caretY, text.length()-1);
    } else {
        str = sLeftSide + text[0] + sRightSide;
        properSetLine(caretY - 1, str, false);
    }
    reparseLines(caretY-1,caretY, false, false);
    timer.start();
    // step2: insert remaining lines of Value
    for (int i=1;i<text.length();i++) {
        bool notInComment = true;
        caretY=pos.line+i;
        if (text[i].isEmpty()) {
            if (i==text.length()-1) {
                str = sRightSide;
            } else {
                str = "";
            }
        } else {
            str = text[i];
            if (i==text.length()-1)
                str += sRightSide;
        }
        if (!mUndoing && mSyntaxer->language()==ProgrammingLanguage::CPP && mOptions.testFlag(EditorOption::AutoIndent) && notInComment) {
            int indentSpaces = calcIndentSpaces(caretY,str,true);
            str = GetLeftSpacing(indentSpaces,true)+trimLeft(str);
        }
        properSetLine(caretY - 1, str,false);
        reparseLines(caretY-1,caretY, false, false);
        result++;
    }
    reparseLines(caretY, caretY+1);
    bChangeScroll = !mOptions.testFlag(EditorOption::ScrollPastEol);
    mOptions.setFlag(EditorOption::ScrollPastEol);
    auto action = finally([&,this]{
        if (bChangeScroll)
            mOptions.setFlag(EditorOption::ScrollPastEol,false);
    });
    newPos=BufferCoord{str.length() - sRightSide.length()+1,caretY};
    //onLinesPutted(startLine-1,result+1);
    if (!mUndoing) {
        mUndoList->addChange(
                    ChangeReason::Insert,
                    pos,newPos,
                    QStringList(),SelectionMode::Normal);
    }
    return result;
}

int QSynEdit::doInsertTextByColumnMode(const BufferCoord& pos, const QStringList& text, int startLine, int endLine)
{
    QString str;
    QString tempString;
    int line;
    int len;
    BufferCoord  lineBreakPos;
    int result = 0;
    DisplayCoord insertCoord = bufferToDisplayPos(pos);
    int insertXPos = insertCoord.x;
    line = startLine;
    if (!mUndoing) {
        beginEditing();
    }
    int i=0;
    while(line<=endLine) {
        str = text[i];
        int insertPos = 0;
        if (line > mDocument->count()) {
            result++;
            tempString = QString(insertXPos - 1,' ') + str;
            mDocument->addLine("");
            if (!mUndoing) {
                result++;
                lineBreakPos.line = line - 1;
                lineBreakPos.ch = mDocument->getLine(line - 2).length() + 1;
                mUndoList->addChange(ChangeReason::LineBreak,
                                 lineBreakPos,
                                 lineBreakPos,
                                 QStringList(), SelectionMode::Normal);
            }
        } else {
            tempString = mDocument->getLine(line - 1);
            len = mDocument->lineWidth(line-1);
            if (len < insertXPos) {
                insertPos = tempString.length()+1;
                tempString = tempString + QString(insertXPos - len - 1,' ') + str;
            } else {
                insertPos = xposToGlyphStartChar(line,insertXPos);
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
    if (!mUndoing) {
        endEditing();
    }
    return result;
}

void QSynEdit::deleteFromTo(const BufferCoord &start, const BufferCoord &end)
{
    if (mReadOnly)
        return;
    if ((start.ch != end.ch) || (start.line != end.line)) {
        beginEditing();
        addCaretToUndo();
        addSelectionToUndo();
        setBlockBegin(start);
        setBlockEnd(end);
        doDeleteText(start,end,SelectionMode::Normal);
        endEditing();
        internalSetCaretXY(start);
    }
}

bool QSynEdit::onGetSpecialLineColors(int, QColor &, QColor &)
{
    return false;
}

void QSynEdit::onGetEditingAreas(int, EditingAreaList &)
{

}

void QSynEdit::onGutterGetText(int , QString &)
{

}

void QSynEdit::onGutterPaint(QPainter &, int , int , int )
{

}

void QSynEdit::onPaint(QPainter &)
{

}

void QSynEdit::onPreparePaintHighlightToken(int , int , const QString &,
                                           PTokenAttribute , FontStyles &, QColor &, QColor &)
{

}

void QSynEdit::onProcessCommand(EditCommand , QChar , void *)
{

}

void QSynEdit::onCommandProcessed(EditCommand , QChar , void *)
{

}

void QSynEdit::executeCommand(EditCommand command, QChar ch, void *pData)
{
    hideCaret();
    incPaintLock();

    auto action=finally([this] {
        decPaintLock();
        showCaret();
    });
    switch(command) {
    //horizontal caret movement or selection
    case EditCommand::Left:
    case EditCommand::SelLeft:
        moveCaretHorz(-1, command == EditCommand::SelLeft);
        break;
    case EditCommand::Right:
    case EditCommand::SelRight:
        moveCaretHorz(1, command == EditCommand::SelRight);
        break;
    case EditCommand::PageLeft:
    case EditCommand::SelPageLeft:
        moveCaretHorz(-viewWidth(), command == EditCommand::SelPageLeft);
        break;
    case EditCommand::PageRight:
    case EditCommand::SelPageRight:
        moveCaretHorz(viewWidth(), command == EditCommand::SelPageRight);
        break;
    case EditCommand::LineStart:
    case EditCommand::SelLineStart:
        moveCaretToLineStart(command == EditCommand::SelLineStart);
        break;
    case EditCommand::LineEnd:
    case EditCommand::SelLineEnd:
        moveCaretToLineEnd(command == EditCommand::SelLineEnd);
        break;
    // vertical caret movement or selection
    case EditCommand::Up:
    case EditCommand::SelUp:
        moveCaretVert(-1, command == EditCommand::SelUp);
        break;
    case EditCommand::Down:
    case EditCommand::SelDown:
        moveCaretVert(1, command == EditCommand::SelDown);
        break;
    case EditCommand::PageUp:
    case EditCommand::SelPageUp:
    case EditCommand::PageDown:
    case EditCommand::SelPageDown:
    {
        int counter = mLinesInWindow;
        if (mOptions.testFlag(EditorOption::HalfPageScroll))
            counter /= 2;
        if (counter<0)
            break;
        if (command == EditCommand::PageUp || command == EditCommand::SelPageUp) {
            counter = -counter;
        }
        incPaintLock();
        ensureCaretVisibleEx(true);
        int gap = (lineToRow(caretY())-1) * mTextHeight - topPos();
        moveCaretVert(counter, command == EditCommand::SelPageUp || command == EditCommand::SelPageDown);
        setTopPos((lineToRow(caretY())-1) * mTextHeight - gap);
        decPaintLock();
        break;
    }
    case EditCommand::PageTop:
    case EditCommand::SelPageTop:
        moveCaretVert(yposToRow(0)-mCaretY, command == EditCommand::SelPageTop);
        break;
    case EditCommand::PageBottom:
    case EditCommand::SelPageBottom:
        moveCaretVert(yposToRow(0)+mLinesInWindow-1-mCaretY, command == EditCommand::SelPageBottom);
        break;
    case EditCommand::FileStart:
    case EditCommand::SelFileStart:
        doGotoEditorStart(command == EditCommand::SelFileStart);
        break;
    case EditCommand::FileEnd:
    case EditCommand::SelFileEnd:
        doGotoEditorEnd(command == EditCommand::SelFileEnd);
        break;
    case EditCommand::BlockStart:
    case EditCommand::SelBlockStart:
        doGotoBlockStart(command == EditCommand::SelBlockStart);
        break;
    case EditCommand::BlockEnd:
    case EditCommand::SelBlockEnd:
        doGotoBlockEnd(command == EditCommand::SelBlockEnd);
        break;
    // goto special line / column position
    case EditCommand::GotoXY:
    case EditCommand::SelGotoXY:
        if (pData)
            moveCaretAndSelection(caretXY(), *((BufferCoord *)(pData)), command == EditCommand::SelGotoXY);
        break;
    // word selection
    case EditCommand::WordLeft:
    case EditCommand::SelWordLeft:
    {
        BufferCoord CaretNew = prevWordPos();
        moveCaretAndSelection(caretXY(), CaretNew, command == EditCommand::SelWordLeft);
        break;
    }
    case EditCommand::WordRight:
    case EditCommand::SelWordRight:
    {
        BufferCoord CaretNew = nextWordPos();
        moveCaretAndSelection(caretXY(), CaretNew, command == EditCommand::SelWordRight);
        break;
    }
    case EditCommand::SelWord:
        setSelWord();
        break;
    case EditCommand::SelectAll:
        doSelectAll();
        break;
    case EditCommand::ExpandSelection:
        doExpandSelection(caretXY());
        break;
    case EditCommand::ShrinkSelection:
        doShrinkSelection(caretXY());
        break;
    case EditCommand::DeleteLastChar:
        doDeleteLastChar();
        break;
    case EditCommand::DeleteChar:
        doDeleteCurrentChar();
        break;
    case EditCommand::DeleteWord:
        doDeleteWord();
        break;
    case EditCommand::DeleteEOL:
        doDeleteToEOL();
        break;
    case EditCommand::DeleteWordStart:
        doDeleteToWordStart();
        break;
    case EditCommand::DeleteWordEnd:
        doDeleteToWordEnd();
        break;
    case EditCommand::DeleteBOL:
        doDeleteFromBOL();
        break;
    case EditCommand::DeleteLine:
        doDeleteLine();
        break;
    case EditCommand::DuplicateLine:
        doDuplicateLine();
        break;
    case EditCommand::MoveSelUp:
        doMoveSelUp();
        break;
    case EditCommand::MoveSelDown:
        doMoveSelDown();
        break;
    case EditCommand::ClearAll:
        clearAll();
        break;
    case EditCommand::InsertLine:
        insertLine(false);
        break;
    case EditCommand::LineBreak:
        insertLine(true);
        break;
    case EditCommand::LineBreakAtEnd:
        beginEditing();
        addLeftTopToUndo();
        addCaretToUndo();
        addSelectionToUndo();
        moveCaretToLineEnd(false, false);
        insertLine(true);
        endEditing();
        break;
    case EditCommand::Tab:
        doTabKey();
        break;
    case EditCommand::ShiftTab:
        doShiftTabKey();
        break;
    case EditCommand::Char:
        doAddChar(ch);
        break;
    case EditCommand::InsertMode:
        if (!mReadOnly)
            setInsertMode(true);
        break;
    case EditCommand::OverwriteMode:
        if (!mReadOnly)
            setInsertMode(false);
        break;
    case EditCommand::ToggleMode:
        if (!mReadOnly) {
            setInsertMode(!mInserting);
        }
        break;
    case EditCommand::Cut:
        if (!mReadOnly)
            doCutToClipboard();
        break;
    case EditCommand::Copy:
        doCopyToClipboard();
        break;
    case EditCommand::Paste:
        if (!mReadOnly)
            doPasteFromClipboard();
        break;
    case EditCommand::ImeStr:
    case EditCommand::String:
        if (!mReadOnly)
            doAddStr(*((QString*)pData));
        break;
    case EditCommand::Undo:
        if (!mReadOnly)
            doUndo();
        break;
    case EditCommand::Redo:
        if (!mReadOnly)
            doRedo();
        break;
    case EditCommand::ZoomIn:
        doZoomIn();
        break;
    case EditCommand::ZoomOut:
        doZoomOut();
        break;
    case EditCommand::Comment:
        doComment();
        break;
    case EditCommand::Uncomment:
        doUncomment();
        break;
    case EditCommand::ToggleComment:
        doToggleComment();
        break;
    case EditCommand::ToggleBlockComment:
        doToggleBlockComment();
        break;
    case EditCommand::ScrollLeft:
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()-mMouseWheelScrollSpeed);
        break;
    case EditCommand::ScrollRight:
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()+mMouseWheelScrollSpeed);
        break;
    case EditCommand::ScrollUp:
        verticalScrollBar()->setValue(verticalScrollBar()->value()-mMouseWheelScrollSpeed);
        break;
    case EditCommand::ScrollDown:
        verticalScrollBar()->setValue(verticalScrollBar()->value()+mMouseWheelScrollSpeed);
        break;
    case EditCommand::MatchBracket:
        {
        BufferCoord coord = getMatchingBracket();
        if (coord.ch!=0 && coord.line!=0)
            internalSetCaretXY(coord);
        }
        break;
    case EditCommand::TrimTrailingSpaces:
        if (!mReadOnly)
            doTrimTrailingSpaces();
        break;
    default:
        break;
    }
}

void QSynEdit::beginEditingWithoutUndo()
{
    mEditingCount++;
}

void QSynEdit::endEditingWithoutUndo()
{
    mEditingCount--;
    if (mEditingCount==0)
        reparseDocument();
}

bool QSynEdit::isIdentChar(const QChar &ch)
{
    return mSyntaxer->isIdentChar(ch);
}

bool QSynEdit::isIdentStartChar(const QChar &ch)
{
    return mSyntaxer->isIdentStartChar(ch);
}

void QSynEdit::setRainbowAttrs(const PTokenAttribute &attr0, const PTokenAttribute &attr1, const PTokenAttribute &attr2, const PTokenAttribute &attr3)
{
    mRainbowAttr0 = attr0;
    mRainbowAttr1 = attr1;
    mRainbowAttr2 = attr2;
    mRainbowAttr3 = attr3;
}

void QSynEdit::updateMouseCursor(){
    QPoint p = mapFromGlobal(cursor().pos());
    if (p.y() >= clientHeight() || p.x()>= clientWidth()) {
        setCursor(Qt::ArrowCursor);
    } else if (p.x() > mGutterWidth) {
        setCursor(Qt::IBeamCursor);
    } else {
        setCursor(Qt::PointingHandCursor);
    }
}

bool QSynEdit::isCaretVisible()
{
    DisplayCoord caret = displayXY();
    if (caret.row < yposToRow(0))
        return false;
    if (caret.row >= yposToRow(clientHeight()) )
        return false;
    if (caret.x < mLeftPos)
        return false;
    if (caret.x >= mLeftPos + viewWidth())
        return false;
    return true;
}

void QSynEdit::paintEvent(QPaintEvent *event)
{
    // Now paint everything while the caret is hidden.
    QPainter painter(viewport());
    //Get the invalidated rect.
    QRect rcClip = event->rect();
    QRect rcCaret;
    bool onlyUpdateCaret = false;
    if (mDocument->maxLineWidth()>=0) {
        onlyUpdateCaret = (calculateCaretRect() == rcClip);
    }
    if (onlyUpdateCaret) {
        rcCaret = rcClip;
        // only update caret
        // calculate the needed invalid area for caret
        // qDebug()<<"update caret"<<rcCaret;
        QRectF cacheRC;
        qreal dpr = mContentImage->devicePixelRatioF();
        cacheRC.setLeft(rcClip.left()*dpr);
        cacheRC.setTop(rcClip.top()*dpr);
        cacheRC.setWidth(rcClip.width()*dpr);
        cacheRC.setHeight(rcClip.height()*dpr);
        painter.drawImage(rcCaret,*mContentImage,cacheRC);
    } else {
        //qDebug()<<"paint event:"<<QDateTime::currentDateTime()<<rcClip;
        QRect rcDraw;
        int nL1, nL2, nX1, nX2;
        // Compute the invalid area in lines / columns.
        // columns
        nX1 = mLeftPos;
        if (rcClip.left() > mGutterWidth + 2 )
            nX1 += (rcClip.left() - mGutterWidth - 2 ) ;
        nX2 = mLeftPos + (rcClip.right() - mGutterWidth - 2);
        // lines
        nL1 = minMax(yposToRow(0) + rcClip.top() / mTextHeight, yposToRow(0), displayLineCount());
        nL2 = minMax(yposToRow(0) + (rcClip.bottom() + mTextHeight - 1) / mTextHeight, 1, displayLineCount());

        //qDebug()<<"Paint:"<<nL1<<nL2<<nC1<<nC2;

        QPainter cachePainter(mContentImage.get());
        cachePainter.setFont(font());
        QSynEditPainter textPainter(this, &cachePainter,
                                       nL1,nL2,nX1,nX2);
        // First paint paint the text area if it was (partly) invalidated.
        if (rcClip.right() > mGutterWidth ) {
            rcDraw = rcClip;
            rcDraw.setLeft( std::max(rcDraw.left(), mGutterWidth));
            textPainter.paintEditingArea(rcDraw);
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
        //glyph positions may be updated while painting, so we need to recalc here.
        rcCaret = calculateCaretRect();
    }
    paintCaret(painter, rcCaret);
}

void QSynEdit::resizeEvent(QResizeEvent *)
{
    //resize the cache image
    qreal dpr = devicePixelRatioF();
    mContentImage = std::make_shared<QImage>(clientWidth()*dpr,clientHeight()*dpr,
                                                            QImage::Format_ARGB32);
    mContentImage->setDevicePixelRatio(dpr);

    onSizeOrFontChanged();
}

void QSynEdit::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_blinkTimerId) {
        if (mPaintLock>0)
            return;
        m_blinkStatus = 1- m_blinkStatus;
        updateCaret();
    }
}

bool QSynEdit::event(QEvent *event)
{
    switch(event->type()) {
    case UPDATE_HORIZONTAL_SCROLLBAR_EVENT:
        event->setAccepted(true);
        doUpdateHScrollbar();
        break;
    case QEvent::KeyPress:{
        QKeyEvent* keyEvent = dynamic_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
        {
            // process tab key presse event
            keyPressEvent(keyEvent);
            return true;
        }
    }
        break;
    case QEvent::MouseMove: {
        updateMouseCursor();
        break;
    }
    case QEvent::FontChange: {
        if (mDocument)
            mDocument->setFont(font());
        synFontChanged();
        break;
    }
    default:
        break;
    }
    return QAbstractScrollArea::event(event);
}

void QSynEdit::focusInEvent(QFocusEvent *)
{
    showCaret();
    updateMouseCursor();
}

void QSynEdit::focusOutEvent(QFocusEvent *)
{
    hideCaret();
}

void QSynEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && mActiveSelectionMode != SelectionMode::Normal) {
        setActiveSelectionMode(SelectionMode::Normal);
        setBlockBegin(caretXY());
        setBlockEnd(caretXY());
        event->accept();
    } else {
        EditCommand cmd=TranslateKeyCode(event->key(),event->modifiers());
        if (cmd!=EditCommand::None) {
            processCommand(cmd,QChar(),nullptr);
            event->accept();
        } else if (!event->text().isEmpty()) {
            QChar c = event->text().at(0);
            if (c=='\t' || c.isPrint()) {
                processCommand(EditCommand::Char,c,nullptr);
                event->accept();
            }
        }
    }
    if (!event->isAccepted()) {
        QAbstractScrollArea::keyPressEvent(event);
    }
}

void QSynEdit::mousePressEvent(QMouseEvent *event)
{
    bool bWasSel = false;
    bool bStartDrag = false;
    mMouseMoved = false;
    mMouseOrigin = event->pos();
    Qt::MouseButton button = event->button();
    int X=event->pos().x();
    int Y=event->pos().y();

    QAbstractScrollArea::mousePressEvent(event);

    BufferCoord oldCaret=caretXY();
    if (button == Qt::RightButton) {
        if (mOptions.testFlag(EditorOption::RightMouseMovesCursor) &&
                ( (selAvail() && ! isPointInSelection(displayToBufferPos(pixelsToGlyphPos(X, Y))))
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
        mStateFlags.setFlag(StateFlag::WaitForDragging,false);
        if (bWasSel && mOptions.testFlag(EditorOption::DragDropEditing) && (X >= mGutterWidth + 2)
                && (mActiveSelectionMode == SelectionMode::Normal) && isPointInSelection(displayToBufferPos(pixelsToGlyphPos(X, Y))) ) {
            bStartDrag = true;
        }
        if (bStartDrag && !mReadOnly) {
            mStateFlags.setFlag(StateFlag::WaitForDragging);
        } else {
            if (event->modifiers() == Qt::ShiftModifier) {
                //BlockBegin and BlockEnd are restored to their original position in the
                //code from above and SetBlockEnd will take care of proper invalidation
                setBlockEnd(caretXY());
            } else if (mOptions.testFlag(EditorOption::AltSetsColumnMode)) {
                if (event->modifiers() == Qt::AltModifier && !mReadOnly)
                    setActiveSelectionMode(SelectionMode::Column);
                else
                    setActiveSelectionMode(SelectionMode::Normal);
                //Selection mode must be set before calling SetBlockBegin
                setBlockBegin(caretXY());
            }
            computeScroll(false);
        }
    }
    if (oldCaret!=caretXY()) {
        if (mOptions.testFlag(EditorOption::GroupUndo))
            mUndoList->addGroupBreak();
    }
}

void QSynEdit::mouseReleaseEvent(QMouseEvent *event)
{
    incPaintLock();
    auto action = finally([this](){
       decPaintLock();
    });
    QAbstractScrollArea::mouseReleaseEvent(event);
    int X=event->pos().x();
    /* int Y=event->pos().y(); */

    if (!mMouseMoved && (X < mGutterWidth )) {
        processGutterClick(event);
    }

    BufferCoord oldCaret=caretXY();
    if (mStateFlags.testFlag(StateFlag::WaitForDragging) &&
            !mStateFlags.testFlag(StateFlag::DblClicked)) {
        computeCaret();
        if (! (event->modifiers() & Qt::ShiftModifier))
            setBlockBegin(caretXY());
        setBlockEnd(caretXY());
        mStateFlags.setFlag(StateFlag::WaitForDragging, false);
    }
    mStateFlags.setFlag(StateFlag::DblClicked,false);
    ensureLineAlignedWithTop();
    ensureCaretVisible();
    if (oldCaret!=caretXY()) {
        if (mOptions.testFlag(EditorOption::GroupUndo))
            mUndoList->addGroupBreak();
    }
}

void QSynEdit::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseMoveEvent(event);
    if ( (std::abs(event->pos().y()-mMouseOrigin.y()) > 2)
            || (std::abs(event->pos().x()-mMouseOrigin.x()) > 2) )
        mMouseMoved = true;
    Qt::MouseButtons buttons = event->buttons();
    if (mStateFlags.testFlag(StateFlag::WaitForDragging)
            && !mReadOnly) {
        if ( ( event->pos() - mMouseDownPos).manhattanLength()>=QApplication::startDragDistance()) {
            mStateFlags.setFlag(StateFlag::WaitForDragging,false);
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData;

            mimeData->setText(selText());
            drag->setMimeData(mimeData);

            drag->exec(Qt::DropActions(Qt::CopyAction | Qt::MoveAction));
        }
    } else if (buttons == Qt::LeftButton) {
        if (mOptions.testFlag(EditorOption::AltSetsColumnMode)) {
                if (event->modifiers() == Qt::AltModifier && !mReadOnly)
                    setActiveSelectionMode(SelectionMode::Column);
                else
                    setActiveSelectionMode(SelectionMode::Normal);
        }
    } else if (buttons == Qt::NoButton) {
        updateMouseCursor();
    }
}

void QSynEdit::mouseDoubleClickEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseDoubleClickEvent(event);
    QPoint ptMouse = event->pos();
    if (ptMouse.x() >= mGutterWidth) {
        if (mOptions.testFlag(EditorOption::SelectWordByDblClick))
            setSelWord();
        mStateFlags.setFlag(StateFlag::DblClicked);
    }
}

void QSynEdit::inputMethodEvent(QInputMethodEvent *event)
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
        processCommand(EditCommand::ImeStr,QChar(),&s);
//        for (QChar ch:s) {
//            CommandProcessor(SynEditorCommand::ecChar,ch);
//        }
    }
}

void QSynEdit::leaveEvent(QEvent *)
{
    setCursor(Qt::ArrowCursor);
}

void QSynEdit::wheelEvent(QWheelEvent *event)
{
    int sign = mOptions.testFlag(EditorOption::InvertMouseScroll)?+1:-1;
    if (event->modifiers() == Qt::ShiftModifier) {
        if ( (mWheelAccumulatedDeltaX>0 &&event->angleDelta().y()<0)
             || (mWheelAccumulatedDeltaX<0 &&event->angleDelta().y()>0))
            mWheelAccumulatedDeltaX=0;
        mWheelAccumulatedDeltaX+=event->angleDelta().y();
        int value = horizontalScrollBar()->value();
        int oldValue = value;
        while (mWheelAccumulatedDeltaX>=120) {
            mWheelAccumulatedDeltaX-=120;
            value += sign*mMouseWheelScrollSpeed*mCharWidth;
        }
        while (mWheelAccumulatedDeltaX<=-120) {
            mWheelAccumulatedDeltaX+=120;
            value -= sign*mMouseWheelScrollSpeed*mCharWidth;
        }
        if (value != oldValue)
            horizontalScrollBar()->setValue(value);
    } else {
        if ( (mWheelAccumulatedDeltaY>0 &&event->angleDelta().y()<0)
             || (mWheelAccumulatedDeltaY<0 &&event->angleDelta().y()>0))
            mWheelAccumulatedDeltaY=0;
        mWheelAccumulatedDeltaY+=event->angleDelta().y();
        int value = verticalScrollBar()->value();
        int oldValue = value;
        while (mWheelAccumulatedDeltaY>=120) {
            mWheelAccumulatedDeltaY-=120;
            value += sign*mMouseWheelScrollSpeed*mTextHeight;
        }
        while (mWheelAccumulatedDeltaY<=-120) {
            mWheelAccumulatedDeltaY+=120;
            value -= sign*mMouseWheelScrollSpeed*mTextHeight;
        }
        if (value != oldValue)
            verticalScrollBar()->setValue(value);

        if ( (mWheelAccumulatedDeltaX>0 &&event->angleDelta().x()<0)
             || (mWheelAccumulatedDeltaX<0 &&event->angleDelta().x()>0))
            mWheelAccumulatedDeltaX=0;
        mWheelAccumulatedDeltaX+=event->angleDelta().x();
        value = horizontalScrollBar()->value();
        oldValue = value;
        while (mWheelAccumulatedDeltaX>=120) {
            mWheelAccumulatedDeltaX-=120;
            value += sign*mMouseWheelScrollSpeed*mCharWidth;
        }
        while (mWheelAccumulatedDeltaX<=-120) {
            mWheelAccumulatedDeltaX+=120;
            value -= sign*mMouseWheelScrollSpeed*mCharWidth;
        }
        if (value != oldValue)
            horizontalScrollBar()->setValue(value);
    }
    event->accept();
}

bool QSynEdit::viewportEvent(QEvent * event)
{
//    switch (event->type()) {
//        case QEvent::Resize:
//            sizeOrFontChanged(false);
//        break;
//    }
    return QAbstractScrollArea::viewportEvent(event);
}

QVariant QSynEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    QRect rect = calculateInputCaretRect();

    switch(property) {
    case Qt::ImCursorRectangle:
        return rect;
    default:
        return QWidget::inputMethodQuery(property);
    }

}

void QSynEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain")) {
        event->acceptProposedAction();
        mDragCaretSave = caretXY();
        mDragSelBeginSave = blockBegin();
        mDragSelEndSave = blockEnd();
        BufferCoord coord = displayToBufferPos(pixelsToNearestGlyphPos(event->pos().x(),
                                                                        event->pos().y()));
        internalSetCaretXY(coord);
        setBlockBegin(mDragSelBeginSave);
        setBlockEnd(mDragSelEndSave);
        mDocument->addLine(""); //add a line to handle drag to the last
        showCaret();
        computeScroll(true);
    }
}

void QSynEdit::dropEvent(QDropEvent *event)
{
    //mScrollTimer->stop();

    BufferCoord coord = displayToBufferPos(pixelsToNearestGlyphPos(event->pos().x(),
                                                                    event->pos().y()));
    if (
            (event->proposedAction() == Qt::DropAction::CopyAction
             && coord>mDragSelBeginSave && coord<mDragSelEndSave)
             ||
             (event->proposedAction() != Qt::DropAction::CopyAction
             && coord>=mDragSelBeginSave && coord<=mDragSelEndSave)
            ) {
        mDocument->deleteAt(mDocument->count()-1);
        ensureLineAlignedWithTop();
        ensureCaretVisible();
        //do nothing if drag onto itself
        event->acceptProposedAction();
        mDropped = true;
        return;
    }
//    if (coord.line<=0 || coord.line>=mDocument->count()) {
//        //do nothing if drag out of range
//        event->acceptProposedAction();
//        mDropped = true;
//        return;
//    }
    coord = ensureBufferCoordValid(coord);
    bool lastLineUsed = coord.line == mDocument->count();
    int topPos = mTopPos;
    int leftPos = mLeftPos;
    int line=mDocument->count()-1;
    QString s=mDocument->getLine(line-1);
    QStringList text=splitStrings(event->mimeData()->text());
    beginEditing();
    if (lastLineUsed)
        mUndoList->addChange(ChangeReason::LineBreak,
                         BufferCoord{s.length()+1,line},
                         BufferCoord{s.length()+1,line}, QStringList(), SelectionMode::Normal);
    else
        mDocument->deleteAt(mDocument->count()-1);
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
            if (coord.line>mDocument->count()) {
                int line=mDocument->count();
                QString s=mDocument->getLine(line-1);
                beginEditing();
                mDocument->addLine("");

                mUndoList->addChange(ChangeReason::LineBreak,
                                     BufferCoord{s.length()+1,line},
                                     BufferCoord{s.length()+1,line}, QStringList(), SelectionMode::Normal);
                endEditing();
                coord.line = line+1;
                coord.ch=1;
            } else {

            }
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
                    topPos -= (mDragSelEndSave.line-mDragSelBeginSave.line) * mTextHeight;
                }
            }
        }
    }
    endEditing();
    event->acceptProposedAction();
    mDropped = true;
    topPos = calcLineAlignedTopPos(topPos, false);
    setTopPos(topPos);
    setLeftPos(leftPos);
    internalSetCaretXY(coord);
}

void QSynEdit::dragMoveEvent(QDragMoveEvent *event)
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
    int x=iMousePos.x();
    int y=iMousePos.y();
    BufferCoord coord = displayToBufferPos(pixelsToNearestGlyphPos(x,y));
    internalSetCaretXY(coord);
    setBlockBegin(mDragSelBeginSave);
    setBlockEnd(mDragSelEndSave);
    showCaret();
}

void QSynEdit::dragLeaveEvent(QDragLeaveEvent *)
{
    mDocument->deleteAt(mDocument->count()-1);
//    setCaretXY(mDragCaretSave);
//    setBlockBegin(mDragSelBeginSave);
//    setBlockEnd(mDragSelEndSave);
    //    showCaret();
}

int QSynEdit::maxScrollHeight() const
{
    if (mOptions.testFlag(EditorOption::ScrollPastEof))
        return (std::max(displayLineCount(),1) - 1) * mTextHeight;
    else
        return std::max((displayLineCount()-mLinesInWindow+1) * mTextHeight, 0) ;
}

bool QSynEdit::modified() const
{
    return mModified;
}

void QSynEdit::setModified(bool value)
{
    if (value) {
        mLastModifyTime = QDateTime::currentDateTime();
        emit statusChanged(StatusChange::Modified);
    }
    if (value != mModified) {
        mModified = value;

        if (value) {
            mUndoList->clear();
            mRedoList->clear();
        } else {
            if (mOptions.testFlag(EditorOption::GroupUndo)) {
                mUndoList->addGroupBreak();
            }
            mUndoList->setInitialState();
        }
        emit statusChanged(StatusChange::ModifyChanged);
    }
}

int QSynEdit::gutterWidth() const
{
    return mGutterWidth;
}

void QSynEdit::setGutterWidth(int Value)
{
    Value = std::max(Value, 0);
    if (mGutterWidth != Value) {
        mGutterWidth = Value;
        // onSizeOrFontChanged(false);
        invalidate();
    }
}

void QSynEdit::onBookMarkOptionsChanged()
{
    invalidateGutter();
}

void QSynEdit::onLinesChanged()
{
    SelectionMode vOldMode;

    if (mActiveSelectionMode == SelectionMode::Column) {
        BufferCoord oldBlockStart = blockBegin();
        BufferCoord oldBlockEnd = blockEnd();
        int xEnd = charToGlyphLeft(mCaretY,mCaretX);
        oldBlockStart.ch = xposToGlyphStartChar(oldBlockStart.line,xEnd);
        oldBlockEnd.ch = xposToGlyphStartChar(oldBlockEnd.line,xEnd);
        setBlockBegin(oldBlockStart);
        setBlockEnd(oldBlockEnd);
    } else {
        vOldMode = mActiveSelectionMode;
        setBlockBegin(caretXY());
        mActiveSelectionMode = vOldMode;
    }
    if (mGutter.showLineNumbers() && (mGutter.autoSize()))
        mGutter.autoSizeDigitCount(mDocument->count());
    setTopPos(mTopPos);
    decPaintLock();
}

void QSynEdit::onLinesChanging()
{
    incPaintLock();
}

void QSynEdit::onLinesCleared()
{
    if (useCodeFolding())
        foldOnListCleared();
    clearUndo();
    // invalidate the *whole* client area
    invalidate();
    // set caret and selected block to start of text
    setCaretXY({1,1});
    // scroll to start of text
    setTopPos(0);
    setLeftPos(0);
    mStatusChanges.setFlag(StatusChange::AllCleared);
}

void QSynEdit::onLinesDeleted(int line, int count)
{
    if (useCodeFolding())
        foldOnLinesDeleted(line + 1, count);
    if (mSyntaxer->needsLineState()) {
        reparseLines(line, line + 1);
    }
    invalidateLines(line + 1, INT_MAX);
}

void QSynEdit::onLinesInserted(int line, int count)
{
    if (useCodeFolding())
        foldOnLinesInserted(line + 1, count);
    if (mSyntaxer->needsLineState()) {
        reparseLines(line, line + count);
    } else {
        // new lines should be parsed
        reparseLines(line, line + count);
    }
    invalidateLines(line + 1, INT_MAX);
}

void QSynEdit::onLinesPutted(int line)
{
    if (mSyntaxer->needsLineState()) {
        reparseLines(line, line + 1);
        invalidateLines(line + 1, INT_MAX);
    } else {
        reparseLines(line, line + 1);
        invalidateLine( line + 1 );
    }
}

void QSynEdit::onUndoAdded()
{
    updateModifiedStatus();

    // we have to clear the redo information, since adding undo info removes
    // the necessary context to undo earlier edit actions
    if (! mUndoList->insideRedo() &&
            mUndoList->peekItem() && (mUndoList->peekItem()->changeReason()!=ChangeReason::GroupBreak))
        mRedoList->clear();

    onChanged();
}

SelectionMode QSynEdit::activeSelectionMode() const
{
    return mActiveSelectionMode;
}

void QSynEdit::setActiveSelectionMode(const SelectionMode &Value)
{
    if (mActiveSelectionMode != Value) {
        if (selAvail())
            invalidateSelection();
        mActiveSelectionMode = Value;
        if (selAvail())
            invalidateSelection();
        setStatusChanged(StatusChange::Selection);
    }
}

BufferCoord QSynEdit::blockEnd() const
{
    if (mActiveSelectionMode==SelectionMode::Column)
        return mBlockEnd;
    if ((mBlockEnd.line < mBlockBegin.line)
      || ((mBlockEnd.line == mBlockBegin.line) && (mBlockEnd.ch < mBlockBegin.ch)))
        return mBlockBegin;
    else
        return mBlockEnd;
}

int QSynEdit::selectionBeginLine() const
{
    if (mActiveSelectionMode == SelectionMode::Column) {
        return (mBlockBegin.line < mBlockEnd.line)? mBlockBegin.line : mBlockEnd.line;
    } else {
        return (mBlockBegin.line < mBlockEnd.line)? mBlockBegin.line : mBlockEnd.line;
    }
}

int QSynEdit::selectionEndLine() const
{
    if (mActiveSelectionMode == SelectionMode::Column) {
        return (mBlockBegin.line < mBlockEnd.line)? mBlockEnd.line : mBlockBegin.line;
    } else {
        if (mBlockBegin.line < mBlockEnd.line) {
            return (mBlockEnd.ch==1)?mBlockEnd.line-1 : mBlockEnd.line;
        } else if (mBlockBegin.line == mBlockEnd.line){
            return mBlockBegin.line;
        } else {
            return (mBlockBegin.ch==1)?mBlockBegin.line-1 : mBlockBegin.line;
        }
    }
}

void QSynEdit::clearSelection()
{
    setActiveSelectionMode(SelectionMode::Normal);
    setBlockBegin(caretXY());
}

void QSynEdit::setBlockEnd(BufferCoord value)
{
    incPaintLock();
    auto action = finally([this](){
        decPaintLock();
    });
    value.line = minMax(value.line, 1, mDocument->count());
    if (mActiveSelectionMode == SelectionMode::Normal) {
      if (value.line >= 1 && value.line <= mDocument->count())
          value.ch = std::min(value.ch, getDisplayStringAtLine(value.line).length() + 1);
      else
          value.ch = 1;
    } else {
        value.ch = std::max(value.ch, 1);
    }
    if (value.ch != mBlockEnd.ch || value.line != mBlockEnd.line) {
        if (mActiveSelectionMode == SelectionMode::Column && value.ch != mBlockEnd.ch) {
            BufferCoord oldBlockEnd = mBlockEnd;
            mBlockEnd = value;
            invalidateLines(
                        std::min(mBlockBegin.line, std::min(mBlockEnd.line, oldBlockEnd.line)),
                        std::max(mBlockBegin.line, std::max(mBlockEnd.line, oldBlockEnd.line)));
        } else {
            BufferCoord oldBlockEnd = mBlockEnd;
            mBlockEnd = value;
            if (mActiveSelectionMode != SelectionMode::Column || mBlockBegin.ch != mBlockEnd.ch) {
                invalidateLines(
                            std::min(mBlockEnd.line, oldBlockEnd.line),
                            std::max(mBlockEnd.line, oldBlockEnd.line));
            }
        }
        setStatusChanged(StatusChange::Selection);
    }
}

void QSynEdit::setSelLength(int Value)
{
    if (mBlockBegin.line>mDocument->count() || mBlockBegin.line<=0)
        return;

    if (Value >= 0) {
        int y = mBlockBegin.line;
        int ch = mBlockBegin.ch;
        int x = ch + Value;
        QString line;
        while (y<=mDocument->count()) {
            line = mDocument->getLine(y-1);
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
            x = mDocument->getLine(y-1).length()+1;
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
            line = mDocument->getLine(y-1);
            x += line.length()+2;
        }
        if (y>mDocument->count()) {
            y = mDocument->count();
            x = mDocument->getLine(y-1).length()+1;
        }
        BufferCoord iNewStart{x,y};
        setCaretAndSelection(iNewStart, iNewStart, mBlockBegin);
    }
}

void QSynEdit::setSelText(const QString &text)
{
    doSetSelText(text);
}

void QSynEdit::replaceLine(int line, const QString &lineText)
{
    BufferCoord pos;
    pos.line=line;
    pos.ch=1;
    mUndoList->addChange(ChangeReason::ReplaceLine,pos,pos,QStringList(mDocument->getLine(line-1)),SelectionMode::Normal);
    mDocument->putLine(line-1,lineText);
}

BufferCoord QSynEdit::blockBegin() const
{
    if (mActiveSelectionMode==SelectionMode::Column)
        return mBlockBegin;
    if ((mBlockEnd.line < mBlockBegin.line)
      || ((mBlockEnd.line == mBlockBegin.line) && (mBlockEnd.ch < mBlockBegin.ch)))
        return mBlockEnd;
    else
        return mBlockBegin;
}

void QSynEdit::setBlockBegin(BufferCoord value)
{
    incPaintLock();
    auto action = finally([this](){
        decPaintLock();
    });
    int nInval1, nInval2;
    bool SelChanged;
    value.line = minMax(value.line, 1, mDocument->count());
    if (mActiveSelectionMode == SelectionMode::Normal) {
        if (value.line >= 1 && value.line <= mDocument->count())
            value.ch = std::min(value.ch, getDisplayStringAtLine(value.line).length() + 1);
        else
            value.ch = 1;
    } else {
        value.ch = std::max(value.ch, 1);
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
        setStatusChanged(StatusChange::Selection);
}

int QSynEdit::leftPos() const
{
    return mLeftPos;
}

void QSynEdit::setLeftPos(int value)
{
    //int MaxVal;
    //QRect iTextArea;
    //value = std::min(value,maxScrollWidth());
    value = std::max(value, 0);
    if (value != mLeftPos) {
        if (mDocument->maxLineWidth()<0)
            mLeftPos = value;
        setStatusChanged(StatusChange::LeftPos);
        if (mScrollBars == ScrollStyle::Both ||  mScrollBars == ScrollStyle::OnlyHorizontal)
            horizontalScrollBar()->setValue(value);
        else {
            if (mDocument->maxLineWidth()>0) {
                mLeftPos = std::max(mLeftPos, 0);
                mLeftPos = std::min(mLeftPos, maxScrollWidth());
            }
            invalidate();
        }
    }
}

int QSynEdit::linesInWindow() const
{
    return mLinesInWindow;
}

int QSynEdit::topPos() const
{
    return mTopPos;
}

void QSynEdit::setTopPos(int value)
{
    //value = std::min(value,maxScrollHeight());
    value = std::max(value, 0);
    if (value != mTopPos) {
        setStatusChanged(StatusChange::TopPos);
        mTopPos = value;
        if (mScrollBars == ScrollStyle::Both ||  mScrollBars == ScrollStyle::OnlyVertical) {
            verticalScrollBar()->setValue(value);
        } else {
            invalidate();
        }
    }
}

void QSynEdit::onGutterChanged()
{
    incPaintLock();
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
    decPaintLock();
}

void QSynEdit::onScrollTimeout()
{
    computeScroll(false);
    //doMouseScroll(false);
}

void QSynEdit::onDraggingScrollTimeout()
{
    computeScroll(true);
    //doMouseScroll(true);
}
}
