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
#include "syntaxer/cpp.h"
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
    mWheelAccumulatedDeltaY{0},
    mDragging{false}
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
    mMergeCaretStatusChangeLock = 0;
    connect(mDocument.get(), &Document::changed, this, &QSynEdit::onLinesChanged);
    connect(mDocument.get(), &Document::changing, this, &QSynEdit::onLinesChanging);
//    connect(mDocument.get(), &Document::deleted, this, &QSynEdit::onLinesDeleted);
//    connect(mDocument.get(), &Document::inserted, this, &QSynEdit::onLinesInserted);
//    connect(mDocument.get(), &Document::putted, this, &QSynEdit::onLinePutted);
    connect(mDocument.get(), &Document::maxLineWidthChanged,
            this, &QSynEdit::onMaxLineWidthChanged);
//    connect(mDocument.get(), &Document::deleted, this, &QSynEdit::updateVScrollbar);
//    connect(mDocument.get(), &Document::inserted, this, &QSynEdit::updateVScrollbar);

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
    mCaretX = 0;
    mLastCaretColumn = 0;
    mCaretY = 0;
    mSelectionBegin.ch = 0;
    mSelectionBegin.line = 0;
    mSelectionEnd = mSelectionBegin;
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
    return lineToRow(mDocument->count()-1);
}

void QSynEdit::setCaretXY(const CharPos &pos)
{
    beginInternalChanges();
    auto action = finally([this](){
        endInternalChanges();
    });
    setSelBegin(pos);
    internalSetCaretXY(pos, true);
}

void QSynEdit::setCaretXYCentered(const CharPos &pos)
{
    beginInternalChanges();
    auto action = finally([this] {
        endInternalChanges();
    });
    setSelBegin(pos);
    internalSetCaretXY(pos, false);
    ensureCaretVisible(true); // but here after block has been set
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
        setStatusChanged(StatusChange::InsertMode);
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

bool QSynEdit::getTokenAttriAtRowCol(const CharPos &pos, QString &token, PTokenAttribute &attri) const
{
    int tmpStart;
    PSyntaxState syntaxState;
    return getTokenAttriAtRowCol(pos, token, tmpStart, attri,syntaxState);
}

bool QSynEdit::getTokenAttriAtRowCol(
        const CharPos &pos, QString &token,
        PTokenAttribute &attri, PSyntaxState &syntaxState) const
{
    int start;
    return getTokenAttriAtRowCol(pos, token, start, attri, syntaxState);
}

bool QSynEdit::getTokenAttriAtRowCol(const CharPos &pos, QString &token, int &start, PTokenAttribute &attri) const
{
    PSyntaxState syntaxState;
    return getTokenAttriAtRowCol(pos, token, start, attri, syntaxState);
}

bool QSynEdit::getTokenAttriAtRowCol(const CharPos &pos, QString &token, int &start, PTokenAttribute &attri, PSyntaxState &syntaxState) const
{
    int chIdx, lineIdx, endPos;
    QString lineText;
    lineIdx = pos.line;
    if ((lineIdx >= 0) && (lineIdx < mDocument->count())) {
        lineText = mDocument->getLine(lineIdx);
        prepareSyntaxerState(*mSyntaxer, lineIdx, lineText, mDocument->getLineSeq(lineIdx));
        chIdx = pos.ch;
        if ((chIdx >= 0) && (chIdx < lineText.length())) {
            while (!mSyntaxer->eol()) {
                start = mSyntaxer->getTokenPos();
                token = mSyntaxer->getToken();
                syntaxState = mSyntaxer->getState();
                endPos = start + token.length();
                if ((chIdx >= start) && (chIdx < endPos)) {
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

void QSynEdit::getTokenAttriList(int line, QStringList &lstToken, QList<int> &lstPos, QList<PTokenAttribute> lstAttri)
{
    lstToken.clear();
    lstPos.clear();
    lstAttri.clear();
    if ((line >= 0) && (line < mDocument->count())) {
        prepareSyntaxerState(*mSyntaxer, line, mDocument->getLine(line), mDocument->getLineSeq(line));
        while (!mSyntaxer->eol()) {
            lstPos.append(mSyntaxer->getTokenPos());
            lstToken.append(mSyntaxer->getToken());
            lstAttri.append(mSyntaxer->getTokenAttribute());
            mSyntaxer->next();
        }
    }
}

void QSynEdit::addGroupUndoBreak()
{
    if (!mUndoing && mOptions.testFlag(EditorOption::GroupUndo))
        mUndoList->addGroupBreak();
}

void QSynEdit::addChangeToUndo(ChangeReason reason, const CharPos &start, const CharPos &end, const QStringList &changeText, SelectionMode selMode)
{
    if (!mUndoing)
        mUndoList->addChange(reason, start, end, changeText, selMode);
}

void QSynEdit::addCaretToUndo()
{
    CharPos p=caretXY();
    addChangeToUndo(ChangeReason::Caret,p,p,QStringList(), mActiveSelectionMode);
}

void QSynEdit::addLeftTopToUndo()
{
    CharPos p;
    //todo: use buffer coord to save left/top pos is ugly
    p.ch = leftPos();
    p.line = topPos();
    addChangeToUndo(ChangeReason::LeftTop,p,p,QStringList(), mActiveSelectionMode);
}

void QSynEdit::addSelectionToUndo()
{
    addChangeToUndo(ChangeReason::Selection,mSelectionBegin,
                         mSelectionEnd,QStringList(),mActiveSelectionMode);
}

void QSynEdit::replaceAll(const QString &text)
{
    addSelectionToUndo();
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
    for (int line=0;line<mDocument->count();line++) {
        if (mDocument->getSyntaxState(line)->hasTrailingSpaces) {
                QString oldLine = mDocument->getLine(line);
                QString newLine = trimRight(oldLine);
                if (newLine.isEmpty())
                    continue;
                QString deleted = oldLine.mid(oldLine.length(),newLine.length()-oldLine.length());
                properSetLine(line,newLine,false);
                addChangeToUndo(ChangeReason::Delete,
                        CharPos{oldLine.length(), line},
                        CharPos{newLine.length(), line},
                        QStringList({deleted}),
                        SelectionMode::Normal
                        );
        }
    } 
}

CharPos QSynEdit::getMatchingBracket()
{
    return getMatchingBracketEx(caretXY());
}

CharPos QSynEdit::getMatchingBracketEx(CharPos APoint)
{
    QChar Brackets[] = {'(', ')', '[', ']', '{', '}', '<', '>'};
    QString lineStr;
    int i, Len;
    QChar Test, BracketInc, BracketDec;
    int NumBrackets;
    QString vDummy;
    PTokenAttribute attr;
    CharPos p;
    bool isCommentOrStringOrChar;
    int nBrackets = sizeof(Brackets) / sizeof(QChar);

    if (mDocument->count()<1)
        return CharPos{-1,-1};
    // get char at caret
    int posX = std::max(APoint.ch,0);
    int posY = std::max(APoint.line,0);
    lineStr = mDocument->getLine(APoint.line);
    if (posX < lineStr.length()) {
        Test = lineStr[posX];
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
                        while ((--posX) >= 0) {
                            Test = lineStr[posX];
                            p.ch = posX;
                            p.line = posY;
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
                        if (posY == 0)
                            break;
                        posY--;
                        lineStr = mDocument->getLine(posY);
                        posX = lineStr.length();
                    }
                } else {
                    while (true) {
                        // search until end of line
                        Len = lineStr.length();
                        while ((++posX) < Len) {
                            Test = lineStr[posX];
                            p.ch = posX;
                            p.line = posY;
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
                        if (posY+1 < mDocument->count())
                            break;
                        posY++;
                        lineStr = mDocument->getLine(posY);
                        posX = -1;
                    }
                }
                // don't test the other brackets, we're done
                break;
            }
        }
    }
    return CharPos{-1,-1};
}

QStringList QSynEdit::content()
{
    return document()->content();
}

QString QSynEdit::text()
{
    return document()->text();
}

bool QSynEdit::validLine(int line) const
{
    return line>=0 && line<mDocument->count();
}

bool QSynEdit::validInDoc(int line, int ch) const
{
    if (ch == 0 && line ==0)
        return true;
    if (line<0 || line>=mDocument->count())
        return false;
    if (ch<0)
        return false;
    return ch<= mDocument->getLine(line).length();
}


bool QSynEdit::getPositionOfMouse(CharPos &aPos) const
{
    QPoint point = QCursor::pos();
    point = mapFromGlobal(point);
    return pointToCharLine(point,aPos);
}

bool QSynEdit::getLineOfMouse(int &line) const
{
    QPoint point = QCursor::pos();
    point = mapFromGlobal(point);
    return pointToLine(point,line);
}

bool QSynEdit::pointToCharLine(const QPoint &point, CharPos &coord) const
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

bool QSynEdit::pointToLine(const QPoint &point, int &line) const
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

void QSynEdit::invalidateGutterLine(int line)
{
    if ((line < 0) || (line >= mDocument->count()))
        return;

    invalidateGutterLines(line, line+1);
}

void QSynEdit::invalidateGutterLines(int startLine, int endLine)
{
    QRect rcInval;
    if (!isVisible())
        return;
    if (startLine == -1 && endLine == -1) {
        rcInval = QRect(0, 0, mGutterWidth, clientHeight());
        invalidateRect(rcInval);
    } else if (endLine > startLine){
        // find the visible lines first
        if (endLine < startLine)
            std::swap(endLine, startLine);
        int firstRow = lineToRow(startLine);
        int endRow;
        if (endLine <= mDocument->count())
            endRow = lineToRow(endLine-1);
        else
            endRow = mDocument->count() + mLinesInWindow + 2;
        // any line visible?
        int firstLineTop = std::max((firstRow-1)*mTextHeight, mTopPos);
        int lastLineBottom = std::min((endRow)*mTextHeight, mTopPos+clientHeight());
        rcInval = {0, firstLineTop - mTopPos,
                   mGutterWidth, lastLineBottom - firstLineTop};
        invalidateRect(rcInval);
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
    if (line<0)
        line = 0;
    if (line>=mDocument->count())
        line = mDocument->count()-1;
    if (xpos<0) {
        xpos=0;
    } else if (xpos>mDocument->lineWidth(line)) {
        xpos=mDocument->lineWidth(line)+1;
    } else {
        int glyphIndex = mDocument->xposToGlyphIndex(line, xpos);
        int nextGlyphPos = mDocument->glyphStartPostion(line, glyphIndex+1);
        if (nextGlyphPos - xpos < mCharWidth / 2)
            xpos = nextGlyphPos;
        else
            xpos = mDocument->glyphStartPostion(line, glyphIndex);
    }
    return DisplayCoord{xpos, row};
}

DisplayCoord QSynEdit::pixelsToGlyphPos(int aX, int aY) const
{
    int xpos = std::max(0, leftPos() + aX - mGutterWidth - 2);
    int row = yposToRow(aY);
    int line = rowToLine(row);
    if (line<0 || line >= mDocument->count() )
        return DisplayCoord{-1,-1};
    if (xpos<0 || xpos>mDocument->lineWidth(line))
        return DisplayCoord{-1,-1};
    int glyphIndex = mDocument->xposToGlyphIndex(line, xpos);
    xpos = mDocument->glyphStartPostion(line, glyphIndex);
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
DisplayCoord QSynEdit::bufferToDisplayPos(const CharPos &p) const
{
    Q_ASSERT(validInDoc(p));
    DisplayCoord result {p.ch,p.line};
    if (p.line<0)
        return result;
    // Account for tabs and charColumns
    if (p.line<mDocument->count())
        result.x = charToGlyphLeft(p.line,p.ch);
    result.row = lineToRow(result.row);
    return result;
}

/**
 * @brief takes a position on screen and transfrom it into position of text
 * @param p
 * @return
 */
CharPos QSynEdit::displayToBufferPos(const DisplayCoord &p) const
{
    CharPos result{p.x,p.row};
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

QString QSynEdit::genSpaces(int charCount) const
{
    if (!mOptions.testFlag(EditorOption::TabsToSpaces) && tabSize()>0) {
        return QString(charCount / tabSize(),'\t') + QString(charCount % tabSize(),' ');
    } else {
        return QString(charCount,' ');
    }
}

int QSynEdit::charToGlyphLeft(int line, int ch) const
{
    Q_ASSERT(validInDoc(line, ch));
    QString s = getDisplayStringAtLine(line);
    return mDocument->charToGlyphStartPosition(line, s, ch);
}

int QSynEdit::charToGlyphLeft(int line, const QString &s, int ch) const
{
    Q_ASSERT(validInDoc(line, ch));
    return mDocument->charToGlyphStartPosition(line, s, ch);
}

// int QSynEdit::charToColumn(const QString &s, int aChar) const
// {
//     return mDocument->charToColumn(s, aChar);
// }

int QSynEdit::xposToGlyphStartChar(int line, int xpos) const
{
    Q_ASSERT(line>=0 && line < mDocument->count());
    QString s = getDisplayStringAtLine(line);
    return mDocument->xposToGlyphStartChar(line,s,xpos);
}

int QSynEdit::xposToGlyphStartChar(int line, const QString &s, int xpos) const
{
    Q_ASSERT(line>=0 && line < mDocument->count());
    return mDocument->xposToGlyphStartChar(line,s,xpos);
}

int QSynEdit::xposToGlyphLeft(int line, int xpos) const
{
    Q_ASSERT(line>=0 && line < mDocument->count());
    int glyphIndex = mDocument->xposToGlyphIndex(line,xpos);
    return mDocument->glyphStartPostion(line,glyphIndex);
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
        return aRow-1;
}

int QSynEdit::lineToRow(int aLine) const
{
    if (useCodeFolding())
        return foldLineToRow(aLine);
    else
        return aLine+1;
}

int QSynEdit::foldRowToLine(int row) const
{
    int resultLine = row-1;
    for (int i=0;i<mAllFoldRanges->count();i++) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (range->collapsed && !range->parentCollapsed() && range->fromLine < resultLine) {
            resultLine += range->linesCollapsed;
        }
    }
    return resultLine;
}

int QSynEdit::foldLineToRow(int line) const
{
    int resultRow = line+1;
    for (int i=mAllFoldRanges->count()-1;i>=0;i--) {
        PCodeFoldingRange range =(*mAllFoldRanges)[i];
        if (range->collapsed && !range->parentCollapsed()) {
            // Line is found after fold
            if (range->toLine < line)
                resultRow -= range->linesCollapsed;
            // Inside fold
            else if (range->fromLine < line && line <= range->toLine)
                resultRow -= line - range->fromLine;
        }
    }
    return resultRow;
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
    if (line<0 || (line>=mDocument->count() &&
                   line!=0) || !isVisible())
        return;

    // invalidate text area of this line
    int row = lineToRow(line);
    if (row >= yposToRow(0) && row <= yposToRow(clientHeight())) {
        rcInval = { mGutterWidth,
                    mTextHeight * (row-1) - mTopPos,
                    clientWidth(),
                    mTextHeight};
        invalidateRect(rcInval);
    }
}

void QSynEdit::invalidateLines(int startLine, int endLine)
{
    if (!isVisible())
        return;
    if (startLine == -1 && endLine == -1) {
        QRect rcInval = clientRect();
        rcInval.setLeft(rcInval.left()+mGutterWidth);
        invalidateRect(rcInval);
    } else if (startLine < endLine){
        startLine = std::max(startLine, 0);
        endLine = std::max(endLine, 0);

        int firstRow = lineToRow(startLine);
        int lastRow;
        if (endLine>= mDocument->count())
            lastRow = lineToRow(mDocument->count() + mLinesInWindow + 2); // paint empty space beyond last line
        else
            lastRow = lineToRow(endLine-1);

        int firstLineTop = std::max((firstRow-1)*mTextHeight, mTopPos);
        int lastLineBottom = std::min(lastRow*mTextHeight, mTopPos+clientHeight());

        // qDebug()<<firstLineTop<<lastLineBottom<<firstLine<<lastLine;
        // any line visible?
        QRect rcInval = {
            clientLeft()+mGutterWidth,
            firstLineTop - mTopPos,
            clientWidth(), lastLineBottom - firstLineTop
        };
        invalidateRect(rcInval);
        // qDebug()<<rcInval;
    }
}

void QSynEdit::invalidateSelection()
{
    invalidateLines(selBegin().line, selEnd().line+1);
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
    if (mSelectionBegin.ch == mSelectionEnd.ch && mSelectionBegin.line == mSelectionEnd.line)
        return false;
    // start line != end line  or start char != end char
    if (mActiveSelectionMode==SelectionMode::Column) {
        if (mSelectionBegin.line != mSelectionEnd.line) {
            DisplayCoord coordBegin = bufferToDisplayPos(mSelectionBegin);
            DisplayCoord coordEnd = bufferToDisplayPos(mSelectionEnd);
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
    if (mSelectionBegin.ch == mSelectionEnd.ch && mSelectionBegin.line == mSelectionEnd.line)
        return false;
    if (mSelectionBegin.line == mSelectionEnd.line && mSelectionBegin.ch!=mSelectionEnd.ch)
        return true;
    DisplayCoord coordBegin = bufferToDisplayPos(mSelectionBegin);
    DisplayCoord coordEnd = bufferToDisplayPos(mSelectionEnd);
    return coordBegin.x!=coordEnd.x;
}

QString QSynEdit::wordAtCursor() const
{
    return tokenAt(caretXY());
}

QString QSynEdit::tokenAt(const CharPos &pos) const
{
    Q_ASSERT(validInDoc(pos));
    int start;
    QString token;
    PTokenAttribute attr;
    if (getTokenAttriAtRowCol(pos, token, start, attr)) {
        return token;
    }
    return QString();
}

QChar QSynEdit::charAt(const CharPos &pos) const
{
    if ((pos.line >= 0) && (pos.line < mDocument->count())) {
        QString line = mDocument->getLine(pos.line);
        int len = line.length();
        if (len == 0)
            return QChar(0);
        if (pos.ch<0 || pos.ch>=len)
            return QChar(0);
        return line[pos.ch];
    }
    return QChar(0);
}

QChar QSynEdit::nextNonSpaceChar(int line, int ch) const
{
    Q_ASSERT(validInDoc(line,ch));
    QString s = mDocument->getLine(line);
    if (s.isEmpty())
        return QChar();
    int x=ch;
    while (x<s.length()) {
        QChar ch = s[x];
        if (!isSpaceChar(ch))
            return ch;
        x++;
    }
    return QChar();
}

QChar QSynEdit::lastNonSpaceChar(int line, int ch) const
{
    Q_ASSERT(validInDoc(line,ch));
    QString s = mDocument->getLine(line);
    int x = std::min(ch,s.length()-1);
    while (line>=0) {
        while (x>=0) {
            QChar c = s[x];
            if (!isSpaceChar(c))
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

void QSynEdit::setCaretAndSelection(const CharPos &posCaret, const CharPos &posSelBegin, const CharPos &posSelEnd)
{
    beginInternalChanges();
    internalSetCaretXY(posCaret);
    setActiveSelectionMode(SelectionMode::Normal);
    setSelBegin(posSelBegin);
    setSelEnd(posSelEnd);
    endInternalChanges();
}

void QSynEdit::loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding)
{
    beginInternalChanges();
    clearAll();
    mDocument->loadFromFile(filename, encoding, realEncoding);
    reparseDocument();
    emit linesInserted(0, mDocument->count());
    endInternalChanges();
}

void QSynEdit::setContent(const QString &text)
{
    beginInternalChanges();
    clearAll();
    mDocument->setText(text);
    reparseDocument();
    emit linesInserted(0, mDocument->count());
    endInternalChanges();
}

void QSynEdit::setContent(const QStringList &text)
{
    beginInternalChanges();
    clearAll();
    mDocument->setContents(text);
    reparseDocument();
    emit linesInserted(0, mDocument->count());
    endInternalChanges();
}

void QSynEdit::collapseAll()
{
    beginInternalChanges();
    for (int i = mAllFoldRanges->count()-1;i>=0;i--){
        collapse((*mAllFoldRanges)[i]);
    }
    endInternalChanges();
}

void QSynEdit::unCollpaseAll()
{
    beginInternalChanges();
    for (int i = mAllFoldRanges->count()-1;i>=0;i--){
        uncollapse((*mAllFoldRanges)[i]);
    }
    endInternalChanges();
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
    if (line>=0 && line < mDocument->count()) {
        emit gutterClicked(event->button(),x,y,line);
    }
}

void QSynEdit::clearUndo()
{
    mUndoList->clear();
    mRedoList->clear();
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

bool QSynEdit::inSelection(const CharPos &pos) const
{
    Q_ASSERT(validInDoc(pos));
    CharPos ptBegin = selBegin();
    CharPos ptEnd = selEnd();
    if ((pos.line >= ptBegin.line) && (pos.line <= ptEnd.line) &&
            ((ptBegin.line != ptEnd.line) || (ptBegin.ch != ptEnd.ch))) {
        if (mActiveSelectionMode == SelectionMode::Column) {
            if (ptBegin.ch > ptEnd.ch)
                return (pos.ch >= ptEnd.ch) && (pos.ch < ptBegin.ch);
            else if (ptBegin.ch < ptEnd.ch)
                return (pos.ch >= ptBegin.ch) && (pos.ch < ptEnd.ch);
            else
                return false;
        } else
            return ((pos.line > ptBegin.line) || (pos.ch >= ptBegin.ch)) &&
      ((pos.line < ptEnd.line) || (pos.ch < ptEnd.ch));
    } else
        return false;
}

CharPos QSynEdit::findNextChar(const CharPos &pos, CharType type) const
{
    Q_ASSERT(validInDoc(pos));
    int ch = pos.ch+1;
    int line = pos.line;
    if (mDocument->count()<=0)
        return CharPos{};
    while (line < mDocument->count()) {
        QString s = mDocument->getLine(line);
        int lineLen = s.length();
        while (ch<lineLen) {
            if (type == CharType::SpaceChar && isSpaceChar(s[ch]))
                return CharPos{ch,line};
            else if (type == CharType::NonSpaceChar && !isSpaceChar(s[ch]))
                return CharPos{ch,line};
            ch++;
        }
        line++;
        ch = 0;
        continue;
    }
    return CharPos{};
}

CharPos QSynEdit::findPrevChar(const CharPos &pos, CharType type) const
{
    Q_ASSERT(validInDoc(pos));
    int ch = pos.ch-1;
    int line = pos.line;
    if (mDocument->empty())
       return CharPos{};
    QString s = mDocument->getLine(line);
    if (ch>=s.length())
        ch = s.length()-1;
    while (line >=0 ) {
        while (ch>=0) {
            if (type == CharType::SpaceChar && isSpaceChar(s[ch]))
                return CharPos{ch,line};
            else if (type == CharType::NonSpaceChar && !isSpaceChar(s[ch]))
                return CharPos{ch,line};
            ch--;
        }
        line--;
        if (line>=0) {
            s = mDocument->getLine(line);
            ch = s.length()-1;
        }
    }
    return CharPos{};
}

bool QSynEdit::inWord(const CharPos &pos) const
{
    Q_ASSERT(validInDoc(pos));
    return !isSpaceChar(charAt(pos));
}

CharPos QSynEdit::getTokenBegin(const CharPos &pos) const
{
    Q_ASSERT(validInDoc(pos));
    int start;
    QString token;
    PTokenAttribute attr;
    if (getTokenAttriAtRowCol(pos, token, start, attr)) {
        if (!token.isEmpty())
            return CharPos(start, pos.line);
    }
    return pos;
}


CharPos QSynEdit::getTokenEnd(const CharPos &pos) const
{
    Q_ASSERT(validInDoc(pos));
    int start;
    QString token;
    PTokenAttribute attr;
    if (getTokenAttriAtRowCol(pos, token, start, attr)) {
        if (!token.isEmpty())
            return CharPos(start+token.length(), pos.line);
    }
    return pos;
}

CharPos QSynEdit::prevWordBegin(CharPos pos) const
{
    Q_ASSERT(validInDoc(pos));
    if (mDocument->empty())
        return fileBegin();
    if (pos.ch == 0
            || pos.ch >= mDocument->getLine(pos.line).length()
            || isSpaceChar(charAt(pos))) {
        CharPos p = prevNonSpaceChar(pos);
        if (p.isValid())
            return getTokenBegin(p);
        else
            return fileBegin();
    } else {
        CharPos p=getTokenBegin(pos);
        if (p==pos) {// pos at word start
            p = prevNonSpaceChar(pos);
            if (p.isValid())
                return getTokenBegin(p);
            else
                return fileBegin();
         } else
            return p;
    }
}

CharPos QSynEdit::prevWordEnd(const CharPos &pos) const
{
    Q_ASSERT(validInDoc(pos));
    if (mDocument->empty())
        return fileBegin();
    CharPos p;
    if (pos.ch == 0) {
        p = prevNonSpaceChar(pos);
    } else if (pos.ch >= mDocument->getLine(pos.line).length()) {
        p = prevSpaceChar(CharPos{mDocument->getLine(pos.line).length()-1,pos.line});
        if (p.isValid())
            p = prevNonSpaceChar(p);
    } else if (isSpaceChar(charAt(CharPos{pos.ch-1,pos.line}))){
        p = prevNonSpaceChar(pos);
    } else {
        p = prevSpaceChar(pos);
        if (p.isValid())
            p = prevNonSpaceChar(p);
    }
    if (!p.isValid())
        return fileBegin();
    p.ch++;
    return p;
}

CharPos QSynEdit::nextWordBegin(const CharPos &pos) const
{
    Q_ASSERT(validInDoc(pos));
    if (mDocument->empty())
        return fileBegin();
    CharPos p;
    if (pos.ch>=mDocument->getLine(pos.line).length()
            || isSpaceChar(charAt(pos))) {
        p = nextNonSpaceChar(pos);
    } else {
        p = getTokenEnd(pos);
        if (p.isValid()) {
            if (isSpaceChar(charAt(p)))
                p = nextNonSpaceChar(p);
        }
    }
    return (p.isValid())?p:fileEnd();
}

CharPos QSynEdit::nextWordEnd(const CharPos &pos) const
{
    Q_ASSERT(validInDoc(pos));
    if (mDocument->empty())
        return fileBegin();
    CharPos p;
    if (pos.ch>=mDocument->getLine(pos.line).length()
            || isSpaceChar(charAt(pos))) {
        p = nextNonSpaceChar(pos);
        if (p.isValid())
            p = getTokenEnd(p);
    } else {
        p = getTokenEnd(pos);
    }
    return (p.isValid())?p:fileEnd();
}

CharPos QSynEdit::fileEnd() const
{
    if (mDocument->empty())
        return fileBegin();
    int line = mDocument->count()-1;
    return CharPos{mDocument->getLine(line).length(),line};
}

void QSynEdit::selCurrentToken()
{
    selTokenAt(caretXY());
}

void QSynEdit::selTokenAt(const CharPos &pos)
{
    Q_ASSERT(validInDoc(pos));
    beginInternalChanges();
    int start;
    QString token;
    PTokenAttribute attr;
    if (getTokenAttriAtRowCol(pos, token, start, attr)) {
        if (!token.isEmpty()) {
            CharPos vWordStart{start, pos.line};
            CharPos vWordEnd{start+token.length(), pos.line};
            setCaretAndSelection(vWordEnd, vWordStart, vWordEnd);
        }
    }
    endInternalChanges();
}

void QSynEdit::doExpandSelection(const CharPos &pos)
{
    Q_ASSERT(validInDoc(pos));
    //todo
}

void QSynEdit::doShrinkSelection(const CharPos &pos)
{
    Q_ASSERT(validInDoc(pos));
    //todo
}

int QSynEdit::calcIndentSpaces(int line, const QString& lineText, bool addIndent)
{
    line = std::min(line, mDocument->count());
    if (line<=0)
        return 0;
    if (mFormatter) {
        return mFormatter->calcIndentSpaces(line,lineText,addIndent,this);
    }
    // find the first non-empty preceeding line
    int startLine = line-1;
    QString startLineText;
    while (startLine>=0) {
        startLineText = mDocument->getLine(startLine);
        if (!startLineText.trimmed().isEmpty()) {
            break;
        }
        startLine--;
    }
    int indentSpaces = 0;
    if (startLine>=0) {
        //calculate the indents of last statement;
        indentSpaces = leftSpaces(startLineText);
    }
    return std::max(0,indentSpaces);
}

void QSynEdit::doSelectAll()
{
    beginInternalChanges();
    auto action = finally([this](){
        endInternalChanges();
    });
    setCaretAndSelection(caretXY(), fileBegin(), fileEnd());
}

void QSynEdit::doComment()
{
    CharPos origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    if (mReadOnly)
        return;
    if (mSyntaxer->commentSymbol().isEmpty())
        return;
    beginEditing();
    auto action = finally([this]{
        endEditing();
    });
    origBlockBegin = selBegin();
    origBlockEnd = selEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.ch == 0)
        endLine = std::max(origBlockBegin.line, origBlockEnd.line - 1);
    else
        endLine = origBlockEnd.line;
    QString commentSymbol = mSyntaxer->commentSymbol();
    int symbolLen = commentSymbol.length();
    for (int i = origBlockBegin.line; i<=endLine; i++) {
        properSetLine(i, commentSymbol + mDocument->getLine(i),false);
        addChangeToUndo(ChangeReason::Insert,
              CharPos{0, i},
              CharPos{symbolLen, i},
              QStringList(), SelectionMode::Normal);
    }
    reparseLines(origBlockBegin.line, endLine+1,true);
    // Move begin of selection
    if (origBlockBegin.ch > 0)
        origBlockBegin.ch+=2;
    // Move end of selection
    if (origBlockEnd.ch > 0)
        origBlockEnd.ch+=2;
    // Move caret
    if (origCaret.ch > 0)
          origCaret.ch+=2;
    setCaretAndSelection(origCaret, origBlockBegin, origBlockEnd);
}

void QSynEdit::doUncomment()
{
    CharPos origBlockBegin, origBlockEnd, origCaret;
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
    origBlockBegin = selBegin();
    origBlockEnd = selEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.ch == 0)
        endLine = std::max(origBlockBegin.line, origBlockEnd.line - 1);
    else
        endLine = origBlockEnd.line;
    for (int i = origBlockBegin.line; i<= endLine; i++) {
        s = mDocument->getLine(i);
        s2=s.trimmed();
        if (!s2.startsWith(commentSymbol))
            continue;
        // Find // after blanks only
        int j = 0;
        while ((j+1 < s.length()) && (s[j] == ' ' || s[j] == '\t'))
            j++;
        s.remove(j,symbolLen);
        properSetLine(i,s,false);
        addChangeToUndo(ChangeReason::Delete,
                             CharPos{j, i},
                             CharPos{j+symbolLen, i},
                             changeText, SelectionMode::Normal);
        // Move begin of selection
        if ((i == origBlockBegin.line) && (origBlockBegin.ch > 0))
            origBlockBegin.ch-=symbolLen;
        // Move end of selection
        if ((i == origBlockEnd.line) && (origBlockEnd.ch > 0))
            origBlockEnd.ch-=symbolLen;
        // Move caret
        if ((i == origCaret.line) && (origCaret.ch > 0))
            origCaret.ch-=symbolLen;
    }
    // When grouping similar commands, process one uncomment action per undo/redo
    reparseLines(origBlockBegin.line, endLine+1,true);
    setCaretAndSelection(origCaret,origBlockBegin,origBlockEnd);
}

void QSynEdit::doToggleComment()
{
    CharPos origBlockBegin, origBlockEnd, origCaret;
    int endLine;
    QString s;
    bool allCommented = true;
    if (mReadOnly)
        return;
    if (mSyntaxer->commentSymbol().isEmpty())
        return;
    QString commentSymbol=mSyntaxer->commentSymbol();

    origBlockBegin = selBegin();
    origBlockEnd = selEnd();
    origCaret = caretXY();
    // Ignore the last line the cursor is placed on
    if (origBlockEnd.ch == 0)
        endLine = std::max(origBlockBegin.line, origBlockEnd.line - 1);
    else
        endLine = origBlockEnd.line;
    for (int i = origBlockBegin.line; i<= endLine; i++) {
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
    if (mReadOnly)
        return;
    if (mSyntaxer->blockCommentBeginSymbol().isEmpty())
        return;
    QString beginSymbol=mSyntaxer->blockCommentBeginSymbol();
    QString endSymbol=mSyntaxer->blockCommentEndSymbol();
    int beginSymLen = beginSymbol.length();
    int endSymLen = endSymbol.length();

    QString text=selText().trimmed();
    QString trimmedText = text.trimmed();
    if (trimmedText.length()>beginSymLen+endSymLen && trimmedText.startsWith(beginSymbol) && trimmedText.endsWith(endSymbol)) {
        QString newText=text;
        int pos = newText.indexOf(beginSymbol);
        if (pos>=0) {
            newText.remove(pos,beginSymLen);
        }
        pos = newText.lastIndexOf(endSymbol);
        if (pos>=0) {
            newText.remove(pos,endSymLen);
        }
        setSelText(newText);
    } else {
        QString newText=QString("%1%2%3").arg(beginSymbol,selText(),endSymbol);
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
    CharPos vCaret = displayToBufferPos(coord);
    if ((mCaretX != vCaret.ch) || (mCaretY != vCaret.line)) {
        // changes to line / column in one go
        beginInternalChanges();
        auto action = finally([this]{
            endInternalChanges();
        });
        internalSetCaretXY(vCaret, false);

        // if MouseCapture is True we're changing selection. otherwise we're dragging
        if (isDragging) {
            setSelBegin(mDragSelBeginSave);
            setSelEnd(mDragSelEndSave);
        } else
            setSelEnd(caretXY());

    }
    if (isDragging) {
        mScrollTimer->singleShot(20,this,&QSynEdit::onDraggingScrollTimeout);
    } else  {
        mScrollTimer->singleShot(20,this,&QSynEdit::onScrollTimeout);
    }
}

void QSynEdit::beginEditing()
{
    beginInternalChanges();
    if (mEditingCount==0) {
        if (!mUndoing) {
            mUndoList->beginBlock();
        }
    }
    mEditingCount++;
}

void QSynEdit::endEditing()
{
    mEditingCount--;
    Q_ASSERT(mEditingCount>=0);
    if (mEditingCount==0) {
        if (!mUndoing)
            mUndoList->endBlock();
        rescanFolds();
    }
    endInternalChanges();
}

QString QSynEdit::getDisplayStringAtLine(int line) const
{
    QString s = mDocument->getLine(line);
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

void QSynEdit::doDeletePrevChar()
{
    if (mReadOnly)
        return ;
    auto action = finally([this]{
        ensureCaretVisible();
    });

    if (mActiveSelectionMode==SelectionMode::Column) {
        CharPos start=selBegin();
        CharPos end=selEnd();
        if (!selAvail()) {
            start.ch--;
            setSelBegin(start);
            setSelEnd(end);
        }
        deleteSelection();
        return;
    }
    if (selAvail()) {
        deleteSelection();
        return;
    }
    QString tempStr = lineText();
    int tempStrLen = tempStr.length();
    CharPos caretBackup{caretXY()};
    QStringList helper;
    if (mCaretX > tempStrLen ) {
        // only move caret one column
        return;
    } else if (mCaretX == 0) {
        // join this line with the last line if possible
        if (mCaretY > 0) {
            beginEditing();
            QString lastLine = mDocument->getLine(mCaretY-1);
            if (lastLine.trimmed().isEmpty()) {
                properDeleteLine(mCaretY-1, false);
            } else {
                properDeleteLine(mCaretY, false);
            }
            properSetLine(mCaretY-1, lastLine+tempStr, true);
            setCaretXY(CharPos{lastLine.length(), mCaretY - 1});
            helper.append("");
            helper.append("");
            addChangeToUndo(ChangeReason::MergeWithPrevLine, caretXY(), caretBackup, helper,
                            mActiveSelectionMode);
            endEditing();
        }
    } else {
        // delete char
        QChar lastDelCh{0};
        bool shouldAddGroupBreak = false;
        if (!mUndoing && mOptions.testFlag(EditorOption::GroupUndo)) {
            PUndoItem undoItem = mUndoList->peekItem();
            if (undoItem &&
                    (undoItem->changeReason() ==ChangeReason::DeletePreviousChar
                    && undoItem->changeStartPos().line == mCaretY
                    && undoItem->changeStartPos().ch == mCaretX
                    ))
                lastDelCh = undoItem->changeText()[0][0];
            else
                shouldAddGroupBreak = true;
        }

        int glyphIndex = mDocument->charToGlyphIndex(mCaretY,mCaretX);
        Q_ASSERT(glyphIndex>0);
        int oldCaretX = mCaretX;
        int newCaretX = mDocument->glyphStartChar(mCaretY, glyphIndex-1);
        //qDebug()<<"delete last char:"<<oldCaretX<<newCaretX<<glyphIndex<<mCaretY;
        QString s = tempStr.mid(newCaretX, oldCaretX-newCaretX);
        setCaretX(newCaretX);
        if (!shouldAddGroupBreak) {
            if (isSpaceChar(s[0]))
                shouldAddGroupBreak = !isSpaceChar(lastDelCh);
            else if (isIdentChar(s[0]))
                shouldAddGroupBreak = !isIdentChar(lastDelCh);
            else
                shouldAddGroupBreak = isIdentChar(lastDelCh) || isSpaceChar(lastDelCh);
        }

        helper.append(s);
        tempStr.remove(newCaretX, oldCaretX-newCaretX);
        properSetLine(mCaretY, tempStr, true);
        if (shouldAddGroupBreak)
            addGroupUndoBreak();
        addChangeToUndo(ChangeReason::DeletePreviousChar, caretXY(), caretBackup, helper,
                        mActiveSelectionMode);
    }

}

void QSynEdit::doDeleteCurrentChar()
{
    QStringList helper;
    CharPos newCaret{};
    if (mReadOnly) {
        return;
    }
    auto action = finally([this]{
        ensureCaretVisible();
    });

    if (mActiveSelectionMode==SelectionMode::Column) {
        CharPos start=selBegin();
        CharPos end=selEnd();
        if (!selAvail()) {
            end.ch++;
            setSelBegin(start);
            setSelEnd(end);
        }
        deleteSelection();
        return;
    }
    if (selAvail())
        deleteSelection();
    else {
        // Call UpdateLastCaretX. Even though the caret doesn't move, the
        // current caret position should "stick" whenever text is modified.
        updateLastCaretX();
        QString tempStr = lineText();
        int tempStrLen = tempStr.length();
        if (mCaretX>tempStrLen) {
            return;
        } else if (mCaretX < tempStrLen) {
            QChar lastDelCh{0};
            bool shouldAddGroupBreak = false;
            if (!mUndoing && mOptions.testFlag(EditorOption::GroupUndo)) {
                PUndoItem undoItem = mUndoList->peekItem();
                if (undoItem &&
                        (undoItem->changeReason() ==ChangeReason::DeleteChar
                        && undoItem->changeStartPos().line == mCaretY
                        && undoItem->changeStartPos().ch == mCaretX
                        ))
                    lastDelCh = undoItem->changeText()[0][0];
                else
                    shouldAddGroupBreak = true;
            }

            int glyphIndex = mDocument->charToGlyphIndex(mCaretY,mCaretX);
            int glyphLen = mDocument->glyphLength(mCaretY,glyphIndex);
            QString s = tempStr.mid(mCaretX,glyphLen);
            if (!shouldAddGroupBreak) {
                if (isSpaceChar(s[0]))
                    shouldAddGroupBreak = !isSpaceChar(lastDelCh);
                else if (isIdentChar(s[0]))
                    shouldAddGroupBreak = !isIdentChar(lastDelCh);
                else
                    shouldAddGroupBreak = isIdentChar(lastDelCh) || isSpaceChar(lastDelCh);
            }

            // delete char
            helper.append(s);
            newCaret.ch = mCaretX + glyphLen;
            newCaret.line = mCaretY;
            tempStr.remove(mCaretX, glyphLen);
            properSetLine(mCaretY, tempStr, true);
            if (!mUndoing) {
                if (shouldAddGroupBreak)
                    addGroupUndoBreak();
                addChangeToUndo(ChangeReason::DeleteChar, caretXY(), newCaret,
                      helper, mActiveSelectionMode);
            }
        } else {
            // join line with the line after
            if (mCaretY+1 < mDocument->count()) {
                beginEditing();
                newCaret.ch = 0;
                newCaret.line = mCaretY + 1;
                helper.append("");
                helper.append("");
                QString newString = tempStr + mDocument->getLine(mCaretY+1);
                if (tempStr.trimmed().isEmpty()) {
                    properDeleteLine(mCaretY, false);
                } else {
                    properDeleteLine(mCaretY + 1, false);
                }
                properSetLine(mCaretY, newString, true);
                addChangeToUndo(ChangeReason::MergeWithNextLine, caretXY(), newCaret,
                      helper, mActiveSelectionMode);
                endEditing();
            }
        }
    }
}

void QSynEdit::doDeleteCurrentToken()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length())
        return;
    CharPos pos{caretXY()};
    int start;
    QString token;
    PTokenAttribute attr;
    if (getTokenAttriAtRowCol(pos, token, start, attr)) {
        if (!token.isEmpty()) {
            CharPos wordStart{start, pos.line};
            CharPos wordEnd{start+token.length(), pos.line};
            deleteFromTo(wordStart, wordEnd);
        }
    }
}

void QSynEdit::doDeleteToEOL()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length())
        return;

    deleteFromTo(caretXY(),CharPos{lineText().length(),mCaretY});
}

void QSynEdit::doDeleteToWordStart()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    CharPos end = caretXY();
    CharPos start = getTokenBegin(end);
    if (start==end)
        start = prevWordEnd(end);
    if (start.isValid() && end.isValid())
    deleteFromTo(start,end);
}

void QSynEdit::doDeleteToWordEnd()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    CharPos start = caretXY();
    CharPos end = getTokenEnd(start);
    if (start.isValid() && end.isValid())
        deleteFromTo(start,end);
}

void QSynEdit::doDeleteCurrentTokenAndTralingSpaces()
{
    if (mReadOnly)
        return;
    if (mCaretX>lineText().length()+1)
        return;

    CharPos start = getTokenBegin(caretXY());
    CharPos end = nextWordBegin(caretXY());
    if (start.isValid() && end.isValid())
        deleteFromTo(start,end);
}

void QSynEdit::doDeleteFromBOL()
{
    if (mReadOnly)
        return;
    deleteFromTo(CharPos{0,mCaretY},caretXY());
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
        setSelBegin(caretXY());
    int oldCaretY = mCaretY;
    bool isLastLine = (oldCaretY >= mDocument->count() - 1);
    bool isFirstLine = (oldCaretY == 0);
    CharPos startPos;
    CharPos endPos;
    if (isLastLine) {
        if (isFirstLine) {
            startPos.ch = 0;
            startPos.line = oldCaretY;
        } else {
            startPos.ch = mDocument->getLine(oldCaretY-1).length();
            startPos.line = oldCaretY-1;
        }
        endPos.ch = lineText().length();
        endPos.line = oldCaretY;
    } else {
        startPos.ch = 0;
        startPos.line = oldCaretY;
        endPos.ch = 0;
        endPos.line = oldCaretY+1;
    }
    doDeleteText(startPos, endPos, SelectionMode::Normal);
    endEditing();
    if (isLastLine) {
        setCaretXY(fileEnd());
    } else
        setCaretXY(CharPos{0, oldCaretY});
}

void QSynEdit::doSelectLine()
{
    CharPos ptBegin{0,mCaretY};
    CharPos ptEnd;
    if (mCaretY==mDocument->count()-1)
        ptEnd = CharPos{lineText().length(),mCaretY};
    else
        ptEnd = CharPos{0,mCaretY+1};
    setCaretAndSelection(ptBegin,ptBegin,ptEnd);
}

CharPos QSynEdit::ensureCharPosValid(const CharPos &coord) const
{
    if (mDocument->empty())
        return fileBegin();
    int nMaxX;
    CharPos value = coord;
    if (value.line >= mDocument->count())
        value.line = mDocument->count()-1;
    if (value.line < 0)
        value.line = 0;
    if (mActiveSelectionMode!=SelectionMode::Column) {
        nMaxX = mDocument->getLine(value.line).length();
        value.ch = std::min(value.ch, nMaxX);
    }
    value.ch = std::max(value.ch,0);
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
        properInsertLine(mCaretY+1, lineText(), true);
        addCaretToUndo();
        addChangeToUndo(ChangeReason::LineBreak,
                CharPos{s.length(),mCaretY},
                CharPos{s.length(),mCaretY}, QStringList(), SelectionMode::Normal);
        addChangeToUndo(ChangeReason::Insert,
                CharPos{0,mCaretY+1},
                CharPos{s.length(),mCaretY+1}, QStringList(), SelectionMode::Normal);
        endEditing();
        setCaretXY(CharPos{0, mCaretY});
    }
}

void QSynEdit::doMoveSelUp()
{
    if (mActiveSelectionMode == SelectionMode::Column)
        return;
    if (!mReadOnly && (mDocument->count() > 0) && (selBegin().line > 0)) {
        if (!mUndoing) {
            beginEditing();
            addCaretToUndo();
            addSelectionToUndo();
        }
        int origBlockBeginLine = selectionBeginLine();
        int origBlockEndLine = selectionEndLine();
        CharPos origBlockBegin = selBegin();
        CharPos origBlockEnd = selEnd();

        PCodeFoldingRange foldRange=foldStartAtLine(origBlockEndLine);
        if (foldRange && foldRange->collapsed)
            return;
//        for (int line=origBlockBegin.Line;line<=origBlockEnd.Line;line++) {
//            PSynEditFoldRange foldRange=foldStartAtLine(line);
//            if (foldRange && foldRange->collapsed)
//                return;
//        }

        // Move line above selection to below selection
        properMoveLine(origBlockBeginLine-1, origBlockEndLine, true);

        // Restore caret and selection
        setCaretAndSelection(
                  CharPos{mCaretX, origBlockBeginLine - 1},
                  CharPos{origBlockBegin.ch, origBlockBeginLine-1},
                  CharPos{origBlockEnd.ch, origBlockEndLine}
        );
        if (!mUndoing) {
            addChangeToUndo(ChangeReason::MoveLine,
                    CharPos{0, origBlockBegin.line - 1},
                    CharPos{0,origBlockEnd.line},
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
    if (!mReadOnly && (mDocument->count() > 0) && (selEnd().line+1 < mDocument->count())) {
        if (!mUndoing) {
            beginEditing();
            addCaretToUndo();
            addSelectionToUndo();
        }
        int origBlockBeginLine = selectionBeginLine();
        int origBlockEndLine = selectionEndLine();
        CharPos origBlockBegin = selBegin();
        CharPos origBlockEnd = selEnd();

        PCodeFoldingRange foldRange=foldStartAtLine(origBlockEndLine);
        if (foldRange && foldRange->collapsed)
            return;

        // Move line below selection to above selection
        properMoveLine(origBlockEndLine + 1, origBlockBeginLine, true);

        // Restore caret and selection
        setCaretAndSelection(
                  CharPos{mCaretX, origBlockEnd.line + 1},
                  CharPos{origBlockBegin.ch, origBlockBegin.line + 1},
                  CharPos{origBlockEnd.ch, origBlockEnd.line + 1}
                    );

        if (!mUndoing) {
            addChangeToUndo(ChangeReason::MoveLine,
                    CharPos{0, origBlockEndLine + 1},
                    CharPos{0, origBlockBeginLine},
                    QStringList(),
                    SelectionMode::Normal);
            endEditing();
        }

    }
}

void QSynEdit::clearAll()
{
    beginInternalChanges();
    setCaretXY(fileBegin());
    mDocument->clear();
    onLinesDeleted(0,mDocument->count());
    clearUndo();
    setModified(false);
    endInternalChanges();
}

void QSynEdit::doBreakLine()
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
        deleteSelection();
    }

    QString temp = lineText();
    if (mCaretX>lineText().length()) {
        PCodeFoldingRange foldRange = foldStartAtLine(mCaretY);
        if ((foldRange) && foldRange->collapsed) {
            QString s = temp+mSyntaxer->foldString(temp);
            if (mCaretX >= s.length()) {
                if (!mUndoing) {
                    addCaretToUndo();
                    addSelectionToUndo();
                }
                mCaretY=foldRange->toLine;
                if (mCaretY>=mDocument->count()) {
                    mCaretY=mDocument->count()-1;
                }
                temp = lineText();
                mCaretX=temp.length();
            }
        }
    }

    PTokenAttribute Attr;

    // This is sloppy, but the Right Thing would be to track the column of markers
    // too, so they could be moved depending on whether they are after the caret...
    QString leftLineText = lineText().mid(0, mCaretX);
    QString rightLineText = lineText().mid(mCaretX);
    if (!mUndoing)
        addChangeToUndo(ChangeReason::LineBreak, caretXY(), caretXY(), QStringList(rightLineText),
              SelectionMode::Normal);
    bool notInComment=true;
    QString trimmedleftLineText=trimLeft(leftLineText);
    prepareSyntaxerState(*mSyntaxer, mCaretY, trimmedleftLineText, mDocument->getLineSeq(mCaretY));
    int indentSpaces = 0;
    if (!mUndoing && mSyntaxer->language() == ProgrammingLanguage::CPP && mOptions.testFlag(EditorOption::AutoIndent)
            && mSyntaxer->getToken()=="else") {
        indentSpaces = calcIndentSpaces(mCaretY,
                                        trimmedleftLineText,mOptions.testFlag(EditorOption::AutoIndent)
                                        );
        QString indentSpacesForLeftLineText = genSpaces(indentSpaces);
        leftLineText = indentSpacesForLeftLineText + trimmedleftLineText;
    }
    properSetLine(mCaretY, leftLineText, false);
    reparseLines(mCaretY, mCaretY+1, false);

    notInComment = !mSyntaxer->isCommentNotFinished(
                mSyntaxer->getState())
            && !mSyntaxer->isStringNotFinished(
                mSyntaxer->getState());

    indentSpaces = 0;
    if (mOptions.testFlag(EditorOption::AutoIndent)) {
        rightLineText=trimLeft(rightLineText);
        indentSpaces = calcIndentSpaces(mCaretY+1,
                                        rightLineText,mOptions.testFlag(EditorOption::AutoIndent)
                                            );
    }
    QString indentSpacesForRightLineText = genSpaces(indentSpaces);
    properInsertLine(mCaretY+1, indentSpacesForRightLineText+rightLineText, false);
    nLinesInserted++;

    if (!mUndoing) {
        //insert new line in middle of "/*" and "*/"
        if (!notInComment && (!syntaxer()->blockCommentBeginSymbol().isEmpty())
                && (!syntaxer()->blockCommentEndSymbol().isEmpty())
                && ( leftLineText.endsWith(syntaxer()->blockCommentBeginSymbol())
                     && rightLineText.startsWith(syntaxer()->blockCommentEndSymbol())
                 )) {
            indentSpaces = calcIndentSpaces(mCaretY+1, "" , mOptions.testFlag(EditorOption::AutoIndent));
            indentSpacesForRightLineText = genSpaces(indentSpaces);
            properInsertLine(mCaretY+1, indentSpacesForRightLineText ,false);
            nLinesInserted++;
            addChangeToUndo(ChangeReason::LineBreak, caretXY(), caretXY(), QStringList(),
                    SelectionMode::Normal);
        }
        //insert new line in middle of "{" and "}"
        if (notInComment &&
                ( leftLineText.endsWith('{') && rightLineText.startsWith('}')
                 )) {
            indentSpaces = calcIndentSpaces(mCaretY+1, "" , mOptions.testFlag(EditorOption::AutoIndent)
                                                                   && notInComment);
            indentSpacesForRightLineText = genSpaces(indentSpaces);
            properInsertLine(mCaretY+1, indentSpacesForRightLineText, false);
            nLinesInserted++;
            addChangeToUndo(ChangeReason::LineBreak, caretXY(), caretXY(), QStringList(),
                    SelectionMode::Normal);
        }
    }
    onLinesInserted(mCaretY+1, nLinesInserted);
    setCaretXY(CharPos{indentSpacesForRightLineText.length(),mCaretY + 1});
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
        deleteSelection();
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
}

void QSynEdit::doShiftTabKey()
{
    // Provide Visual Studio like block indenting
    if (!mOptions.testFlag(EditorOption::TabIndent))
        return;
    doBlockUnindent();
}


bool QSynEdit::canDoBlockIndent() const
{
    if (selAvail())
        return true;

    if (caretY()<0 || caretY() >= mDocument->count()) {
        return false;
    }

    QString s = mDocument->getLine(caretY()).left(caretX());
    return (s.trimmed().isEmpty());
}

QRect QSynEdit::calculateCaretRect() const
{
    DisplayCoord coord = displayXY();
    if (!mInputPreeditString.isEmpty()) {
        QString sLine = lineText().left(mCaretX)
                + mInputPreeditString
                + lineText().mid(mCaretX);
        if (sLine == mGlyphPostionCacheForInputMethod.str)  {
            int glyphIdx = searchForSegmentIdx(mGlyphPostionCacheForInputMethod.glyphCharList,sLine.length(),mCaretX+mInputPreeditString.length());
            coord.x = segmentIntervalStart(mGlyphPostionCacheForInputMethod.glyphPositionList,mGlyphPostionCacheForInputMethod.strWidth, glyphIdx);
        } else
            coord.x = charToGlyphLeft(mCaretY, sLine, mCaretX+mInputPreeditString.length());
    }
    int rows=1;
    if (mActiveSelectionMode == SelectionMode::Column) {
        int startRow = lineToRow(std::min(selBegin().line, selEnd().line));
        int endRow = lineToRow(std::max(selBegin().line, selEnd().line));
        coord.row = startRow;
        rows = endRow-startRow+1;
    }
    QPoint caretPos = displayCoordToPixels(coord);
    int caretWidth = mCharWidth;
    if (mCaretY >= 0 && mCaretY < mDocument->count()
            && mCaretX >=0 && mCaretX < mDocument->getLine(mCaretY).length()) {
        int glyphIndex = mDocument->charToGlyphIndex(mCaretY, mCaretX);
        caretWidth = mDocument->glyphWidth(mCaretY, glyphIndex);
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
    if (mCaretY >= 0 && mCaretY < mDocument->count()
            && mCaretX >=0 && mCaretX < mDocument->getLine(mCaretY).length()) {
        int glyphIndex = mDocument->charToGlyphIndex(mCaretY, mCaretX);
        caretWidth = mDocument->glyphWidth(mCaretY, glyphIndex);
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
    CharPos  oldCaretPos{caretXY()};
    CharPos  blockBegin, blockEnd;
    QStringList strToInsert;
    int endLine,newCaretX;
    QString spaces;

    // keep current selection detail
    if (selAvail()) {
        blockBegin = selBegin();
        blockEnd = selEnd();
    } else {
        blockBegin = caretXY();
        blockEnd = caretXY();
    }
    // build text to insert
    if (blockEnd.ch == 0 && blockEnd.line != blockBegin.line) {
        endLine = blockEnd.line - 1;
        newCaretX = 0;
    } else {
        endLine = blockEnd.line;
        if (mOptions.testFlag(EditorOption::TabsToSpaces))
            newCaretX = caretX() + tabSize();
        else
          newCaretX = caretX() + 1;
    }
    endLine = std::min(endLine, mDocument->count()-1);
    if (mOptions.testFlag(EditorOption::TabsToSpaces)) {
        spaces = QString(tabSize(),' ') ;
    } else {
        spaces = "\t";
    }
    beginEditing();
    addCaretToUndo();
    addSelectionToUndo();
    for (int i = blockBegin.line; i<=endLine;i++) {
        QString line=mDocument->getLine(i);
        addChangeToUndo(ChangeReason::Insert,
                CharPos{0, i},
                CharPos{spaces.length()-1, i},
                QStringList(),
                SelectionMode::Normal);
        if (line.isEmpty()) {
            line = spaces;
        } else {
            line = spaces+line;
        }
        properSetLine(i,line, i==endLine);
    }
    //adjust caret and selection
    oldCaretPos.ch = newCaretX;
    if (blockBegin.ch > 0)
        blockBegin.ch += spaces.length();
    if (blockEnd.ch > 0)
      blockEnd.ch+=spaces.length();
    setCaretAndSelection(oldCaretPos,
      blockBegin, blockEnd);
    endEditing();
}

void QSynEdit::doBlockUnindent()
{
    int lastLineIndent = 0;
    int firstLineIndent = 0;

    CharPos blockBegin,blockEnd;
    // keep current selection detail
    if (selAvail()) {
        blockBegin = selBegin();
        blockEnd = selEnd();
    } else {
        blockBegin = caretXY();
        blockEnd = caretXY();
    }
    CharPos oldCaretPos = caretXY();
    int caretLineIndent = 0;
    beginEditing();
    addCaretToUndo();
    addSelectionToUndo();

    int endLine = blockEnd.line;
    // convert selection to complete lines
    if (blockEnd.ch == 0 && blockEnd.line != blockBegin.line)
        endLine = blockEnd.line - 1;
    // build string to delete
    for (int i = blockBegin.line; i<= endLine;i++) {
        QString line = mDocument->getLine(i);
        if (line.isEmpty())
            continue;
        int charsToDelete = 0;
        if (line[0]=='\t') {
            charsToDelete = 1;
        } else {
            while (charsToDelete < tabSize() &&
                   charsToDelete < line.length() &&
                   line[charsToDelete] == ' ')
                charsToDelete++;
        }
        if (charsToDelete == 0)
            continue;
        if (i==blockBegin.line)
            firstLineIndent = charsToDelete;
        if (i==endLine)
            lastLineIndent = charsToDelete;
        if (i==oldCaretPos.line)
            caretLineIndent = charsToDelete;
        QString tempString = line.mid(charsToDelete);
        properSetLine(i,tempString, i==endLine);
        addChangeToUndo(ChangeReason::Delete,
                CharPos{0,i},
                CharPos{charsToDelete,i},
                QStringList(line.left(charsToDelete)),
                SelectionMode::Normal);
    }
  // restore selection
  //adjust the x position of orgcaretpos appropriately

    oldCaretPos.ch -= caretLineIndent;
    blockBegin.ch -= firstLineIndent;
    blockEnd.ch -= lastLineIndent;
    setCaretAndSelection(oldCaretPos, blockBegin, blockEnd);
    endEditing();
}

void QSynEdit::internalAddChar(const QChar& ch)
{
    if ((mActiveSelectionMode != SelectionMode::Normal)
            || selAvail()) {
        doSetSelText(ch);
        return;
    }
    beginEditing();
    if (mDocument->empty()) {
        properInsertLine(0,"",false);
        addChangeToUndo(ChangeReason::InsertLine,
                fileBegin(),fileBegin(),
                QStringList(),SelectionMode::Normal);
    }
    QString s = lineText();
    QString newS = s.left(mCaretX)+ch+s.mid(mCaretX);
    properSetLine(mCaretY,newS,true);
    addChangeToUndo(ChangeReason::AddChar,
            CharPos{mCaretX, mCaretY},
            CharPos{mCaretX+1, mCaretY},
            QStringList(),
            SelectionMode::Normal
            );
    setCaretX(mCaretX+1);
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
            CharPos start=mSelectionBegin;
            CharPos end=mSelectionEnd;
            if (start.line > end.line )
                std::swap(start,end);
            start.ch++; // make sure we select a whole char in the start line
            setSelBegin(start);
            setSelEnd(end);
        }
            break;
        default:
            setSelLength(1);
        }
    }

    QChar lastCh{0};
    if (!selAvail()) {
        PUndoItem undoItem = mUndoList->peekItem();
        if (undoItem && undoItem->changeReason()==ChangeReason::AddChar
                && undoItem->changeEndPos().line == mCaretY
                && undoItem->changeEndPos().ch == mCaretX
                && undoItem->changeStartPos().line == mCaretY
                && undoItem->changeStartPos().ch == mCaretX-1) {
            QString s = mDocument->getLine(mCaretY);
            int i=mCaretX-1;
            if (i>=0 && i<s.length())
                lastCh=s[i];
        }
    }
    if (isIdentChar(ch)) {
        if (!isIdentChar(lastCh)) {
            addGroupUndoBreak();
        }
        internalAddChar(ch);
    } else if (isSpaceChar(ch)) {
        // break group undo chain
        if (!isSpaceChar(lastCh)) {
            addGroupUndoBreak();
        }
        internalAddChar(ch);
    } else {
        if (isSpaceChar(lastCh) || isIdentChar(lastCh)) {
            addGroupUndoBreak();
        }
        beginEditing();
        internalAddChar(ch);
        int oldCaretX=mCaretX-1;
        int oldCaretY=mCaretY;
        // auto
        if (mActiveSelectionMode==SelectionMode::Normal
                && mOptions.testFlag(EditorOption::AutoIndent)
                && mSyntaxer->language() == ProgrammingLanguage::CPP
                && (oldCaretY<=mDocument->count()) ) {

            //unindent if ':' at end of the line
            if (ch == ':') {
                QString line = mDocument->getLine(oldCaretY);
                if (mCaretX == line.length()) {
                    int indentSpaces = calcIndentSpaces(oldCaretY,line, true);
                    if (indentSpaces != leftSpaces(line)) {
                        QString newLine = genSpaces(indentSpaces) + trimLeft(line);
                        properSetLine(oldCaretY,newLine,true);
                        setCaretXY(CharPos{newLine.length(),oldCaretY});
                        addChangeToUndo(ChangeReason::Delete,
                                CharPos{0, oldCaretY},
                                CharPos{line.length(), oldCaretY},
                                QStringList(line),
                                SelectionMode::Normal
                                );
                        addChangeToUndo(ChangeReason::Insert,
                                CharPos{0, oldCaretY},
                                CharPos{newLine.length(), oldCaretY},
                                QStringList(),
                                SelectionMode::Normal
                                );
                    }
                }
            } else if (ch == '*') {
                QString line = mDocument->getLine(oldCaretY);
                if (mCaretX == line.length()) {
                    int indentSpaces = calcIndentSpaces(oldCaretY,line+"*", true);
                    if (indentSpaces != leftSpaces(line)) {
                        QString newLine = genSpaces(indentSpaces) + trimLeft(line);
                        properSetLine(oldCaretY,newLine, true);
                        setCaretXY(CharPos{newLine.length(), oldCaretY});
                        addChangeToUndo(ChangeReason::Delete,
                                CharPos{0, oldCaretY},
                                CharPos{line.length(), oldCaretY},
                                QStringList(line),
                                SelectionMode::Normal
                                );
                        addChangeToUndo(ChangeReason::Insert,
                                CharPos{0, oldCaretY},
                                CharPos{newLine.length(), oldCaretY},
                                QStringList(),
                                SelectionMode::Normal
                                );
                    }
                }
            } else if (ch == '{' || ch == '}' || ch == '#') {
                //Reindent line when add '{' '}' and '#' at the beginning
                QString left = mDocument->getLine(oldCaretY).left(oldCaretX);
                // and the first nonblank char is this new {
                if (left.trimmed().isEmpty()) {
                    int indentSpaces = calcIndentSpaces(oldCaretY,ch, true);
                    if (indentSpaces != leftSpaces(left)) {
                        QString right = mDocument->getLine(oldCaretY).mid(oldCaretX);
                        QString newLeft = genSpaces(indentSpaces);
                        properSetLine(oldCaretY,newLeft+right, true);
                        setCaretXY(CharPos{newLeft.length(),oldCaretY});
                        addChangeToUndo(ChangeReason::Delete,
                                CharPos{0, oldCaretY},
                                CharPos{left.length(), oldCaretY},
                                QStringList(left),
                                SelectionMode::Normal
                                );
                        addChangeToUndo(ChangeReason::Insert,
                                CharPos{0, oldCaretY},
                                CharPos{newLeft.length(), oldCaretY},
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
    deleteSelection();
    endEditing();
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
    beginEditing();
    setSelTextPrimitive(splitStrings(text));
    endEditing();
}

void QSynEdit::beginInternalChanges()
{
    mPaintLock ++ ;
}

void QSynEdit::endInternalChanges()
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
            notifyStatusChange(mStatusChanges);
    }
}

void QSynEdit::beginMergeCaretStatusChange()
{
    if (mMergeCaretStatusChangeLock == 0) {
        mCaretBeforeMerging = caretXY();
    }
    ++mMergeCaretStatusChangeLock;
}

void QSynEdit::endMergeCaretStatusChange()
{
    --mMergeCaretStatusChangeLock;
    if (mMergeCaretStatusChangeLock == 0) {
        if (mCaretBeforeMerging.ch != mCaretX)
            setStatusChanged(StatusChange::CaretX);
        if (mCaretBeforeMerging.line != mCaretY)
            setStatusChanged(StatusChange::CaretY);
    }
}

PSyntaxState QSynEdit::calcSyntaxStateAtLine(int line, const QString &newLineText, bool handleLastBackSlash)
{
    bool oldHandleLastBackSlash = true;
    if (mSyntaxer->language() == ProgrammingLanguage::CPP) {
        std::shared_ptr<QSynedit::CppSyntaxer> cppSyntaxer = std::dynamic_pointer_cast<QSynedit::CppSyntaxer>(mSyntaxer);
        oldHandleLastBackSlash = cppSyntaxer->handleLastBackSlash();
        cppSyntaxer->setHandleLastBackSlash(handleLastBackSlash);
    }
    prepareSyntaxerState(*mSyntaxer, line, newLineText, mDocument->getLineSeq(line));
    if (mSyntaxer->language() == ProgrammingLanguage::CPP) {
        std::shared_ptr<QSynedit::CppSyntaxer> cppSyntaxer = std::dynamic_pointer_cast<QSynedit::CppSyntaxer>(mSyntaxer);
        cppSyntaxer->setHandleLastBackSlash(oldHandleLastBackSlash);
    }
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
    beginInternalChanges();
    recalcCharExtent();
    endInternalChanges();
}


void QSynEdit::updateLastCaretX()
{
    mLastCaretColumn = displayX();
}

void QSynEdit::ensureCaretVisible()
{
    ensureCaretVisible(false);
}

void QSynEdit::ensureCaretVisible(bool forceToMiddle)
{
    beginInternalChanges();
    auto action = finally([this]{
        endInternalChanges();
    });
    // Make sure Y is visible
    int vCaretRow = displayY();
    if (forceToMiddle) {
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
        if (forceToMiddle)
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
    beginInternalChanges();
    internalSetCaretXY(displayToBufferPos(aPos), ensureCaretVisible);
    endInternalChanges();
}

void QSynEdit::internalSetCaretXY(CharPos value, bool ensureVisible)
{
    Q_ASSERT(validInDoc(value));
    //value = ensureBufferCoordValid(value);
    if ((value.ch != mCaretX) || (value.line != mCaretY)) {
        beginInternalChanges();
        auto action = finally([this]{
            endInternalChanges();
        });
        if (mCaretX != value.ch) {
            mCaretX = value.ch;
            setStatusChanged(StatusChange::CaretX);
            invalidateLine(mCaretY);
        }
        if (mCaretY != value.line) {
            int oldCaretY = mCaretY;
            mCaretY = value.line;
            invalidateLine(mCaretY);
            invalidateGutterLine(mCaretY);
            invalidateLine(oldCaretY);
            invalidateGutterLine(oldCaretY);
            setStatusChanged(StatusChange::CaretY);
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
    internalSetCaretXY(CharPos{value, mCaretY});
}

void QSynEdit::setStatusChanged(StatusChanges changes)
{
    if (mMergeCaretStatusChangeLock>0)
        changes &= ~(StatusChange::CaretX | StatusChange::CaretY);
    mStatusChanges = mStatusChanges | changes;
    if (mPaintLock == 0)
        notifyStatusChange(mStatusChanges);
}

void QSynEdit::notifyStatusChange(StatusChanges)
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
        ensureCaretVisible(false);
        mStateFlags.setFlag(StateFlag::EnsureCaretVisible,false);
    } else if (mStateFlags.testFlag(StateFlag::EnsureCaretVisibleForceMiddle)) {
        ensureCaretVisible(true);
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
        foreach (const PTokenAttribute& attribute, mSyntaxer->attributes()) {
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

void QSynEdit::updateModifiedStatusForUndoRedo()
{
    setModified(!mUndoList->initialState(), true);
//    qDebug()<<mModified<<oldModified;
}

int QSynEdit::reparseLines(int startLine, int endLine, bool toDocumentEnd)
{
    PSyntaxState state;
    int maxLine = toDocumentEnd ? mDocument->count() : endLine;
    startLine = std::max(0,startLine);
    endLine = std::min(endLine, mDocument->count());
    maxLine = std::min(maxLine, mDocument->count());


    if (startLine >= maxLine)
        return startLine;

    if (startLine == 0) {
        mSyntaxer->resetState();
    } else {
        mSyntaxer->setState(mDocument->getSyntaxState(startLine-1));
    }
    int line = startLine;
    do {
        mSyntaxer->setLine(line, mDocument->getLine(line), mDocument->getLineSeq(line));
        mSyntaxer->nextToEol();
        state = mSyntaxer->getState();
        if (line >= endLine && state->equals(mDocument->getSyntaxState(line)) ) {
            break;
        }
        mDocument->setSyntaxState(line,state);
        line++;
    } while (line < maxLine);
    invalidateLines(startLine, line);
#ifdef QT_DEBUG
//    qDebug()<<"parse endLine"<<endLine<<"real end"<<line;
#endif
    //don't rescan folds if only currentLine is reparsed
#ifdef QSYNEDIT_TEST
        emit linesReparesd(startLine, line-startLine);
#endif
    if (line-startLine==1)
        return line;

    if (mEditingCount>0) {
        return line;
    }
    return line;
}

void QSynEdit::reparseDocument()
{
    if (!mDocument->empty()) {
        mSyntaxer->resetState();
        for (int i =0;i<mDocument->count();i++) {
            mSyntaxer->setLine(i, mDocument->getLine(i), mDocument->getLineSeq(i));
            mSyntaxer->nextToEol();
            mDocument->setSyntaxState(i, mSyntaxer->getState());
        }
#ifdef QSYNEDIT_TEST
        emit linesReparesd(0, mDocument->count());
#endif
        invalidateLines(0,mDocument->count());
        rescanFolds();
    }
}

void QSynEdit::uncollapse(PCodeFoldingRange foldRange)
{
    foldRange->linesCollapsed = 0;
    foldRange->collapsed = false;

    // Redraw the collapsed line
    invalidateLines(foldRange->fromLine, INT_MAX);

    // Redraw fold mark
    invalidateGutterLines(foldRange->fromLine, INT_MAX);
    updateHScrollbar();
    updateVScrollbar();
}

void QSynEdit::collapse(PCodeFoldingRange foldRange)
{
    foldRange->linesCollapsed = foldRange->toLine - foldRange->fromLine;
    foldRange->collapsed = true;

    // Extract caret from fold
    if ((mCaretY > foldRange->fromLine) && (mCaretY <= foldRange->toLine)) {
          setCaretXY(CharPos{mDocument->getLine(foldRange->fromLine).length(),
                                 foldRange->fromLine});
    }

    // Redraw the collapsed line
    invalidateLines(foldRange->fromLine, INT_MAX);

    // Redraw fold mark
    invalidateGutterLines(foldRange->fromLine, INT_MAX);

    updateHScrollbar();
    updateVScrollbar();
}

void QSynEdit::processFoldsOnLinesInserted(int line, int count)
{
    if (!useCodeFolding())
        return;
    // Delete collapsed inside selection
    for (int i = mAllFoldRanges->count()-1;i>=0;i--) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (range->fromLine >= line) // insertion of count lines above FromLine
            range->move(count);
        else if (range->toLine >= line)
            range->toLine += count;
    }
}

void QSynEdit::processFoldsOnLinesDeleted(int line, int count)
{
    if (!useCodeFolding())
        return ;
    // Delete collapsed inside selection
    for (int i = mAllFoldRanges->count()-1;i>=0;i--) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (range->fromLine >= line
                && range->fromLine < line+count)  {
            mAllFoldRanges->remove(i);
        } else if (range->toLine >= line
                   && range->toLine < line+count) {
//            uncollapse(range);
            mAllFoldRanges->remove(i);
        } else if (range->fromLine >= line + count) { // Move after affected area
            range->move(-count);
        } else if (range->toLine >= line + count) {
            range->toLine -= count;
            if (range->toLine <= range->fromLine)
                mAllFoldRanges->remove(i);
        }
    }

}

void QSynEdit::processFoldsOnLineMoved(int from, int to)
{
    if (!useCodeFolding())
        return;
    for (int i = mAllFoldRanges->count()-1;i>=0;i--) {
        PCodeFoldingRange range = (*mAllFoldRanges)[i];
        if (from<to) {
            if (range->fromLine == from)
                range->fromLine = to;
            else if (from < range->fromLine && range->fromLine <= to)
                range->fromLine -=1;
            if (range->toLine == from)
                range->toLine = to;
            else if (from < range->toLine && range->toLine <= to)
                range->toLine -= 1;
        } else if (to > from) {
            if (range->fromLine == from)
                range->fromLine = to;
            else if (to <= range->fromLine && range->fromLine < from)
                range->fromLine +=1;
            if (range->toLine == from)
                range->toLine = to;
            else if (to <= range->toLine && range->toLine < from)
                range->toLine +=1;
        }
        if (range->toLine<=range->fromLine)
            mAllFoldRanges->remove(i);
    }
}

void QSynEdit::clearFoldRanges()
{
    mAllFoldRanges->clear();
}

void QSynEdit::rescanFolds()
{
    if (!useCodeFolding())
        return;

    beginInternalChanges();
    rescanForFoldRanges();
#ifdef QSYNEDIT_TEST
    emit foldsRescaned();
#endif
    invalidateGutter();
    endInternalChanges();
}

void QSynEdit::rescanForFoldRanges()
{
    // Did we leave any collapsed folds and are we viewing a code file?
    if (mAllFoldRanges->count() > 0) {
        QMap<QString,PCodeFoldingRange> rangeIndexes;
        foreach(const PCodeFoldingRange& r, mAllFoldRanges->ranges()) {
            if (r->collapsed)
                rangeIndexes.insert(QString("%1-%2").arg(r->fromLine).arg(r->toLine),r);
        }

        // Add folds to a separate list
        scanForFoldRanges();

        // Combine new with old folds, preserve parent order
        for (int i = 0; i< mAllFoldRanges->count();i++) {
            PCodeFoldingRange tempFoldRange=mAllFoldRanges->range(i);
            PCodeFoldingRange r2=rangeIndexes.value(QString("%1-%2").arg(tempFoldRange->fromLine).arg(tempFoldRange->toLine),
                                  PCodeFoldingRange());
            if (r2) {
                tempFoldRange->collapsed=true;
                tempFoldRange->linesCollapsed=r2->linesCollapsed;
            }
        }
    } else {
        // We ended up with no folds after deleting, just pass standard data...
        scanForFoldRanges();
    }
}

void QSynEdit::scanForFoldRanges()
{
    if (!useCodeFolding())
        return;

    mAllFoldRanges->clear();
    PCodeFoldingRanges& parentFoldRanges = mAllFoldRanges;
    PCodeFoldingRange parent;
    int line = 0;
    while (line < mDocument->count()) { // index is valid for LinesToScan and fLines
        // If there is a collapsed fold over here, skip it
        // Find an opening character on this line
        int blockEnded=mDocument->blockEnded(line);
        int blockStarted=mDocument->blockStarted(line);
        if (blockEnded>0) {
            for (int i=0; i<blockEnded;i++) {
                // Stop the recursion if we find a closing char, and return to our parent
                if (parent) {
                    if (blockStarted>0)
                        parent->toLine = line - 1;
                    else
                        parent->toLine = line;
                    parent = parent->parent.lock();
                    if (!parent) {
                        parentFoldRanges = mAllFoldRanges;
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
                  mAllFoldRanges,
                  line,
                  line);
                parentFoldRanges = parent->subFoldRanges;
            }
        }
        line++;
    }
}

PCodeFoldingRange QSynEdit::foldStartAtLine(int line) const
{
    foreach(const PCodeFoldingRange& range, mAllFoldRanges->ranges()) {
        if (range->fromLine == line ){
            return range;
        } else if (range->fromLine>line)
            break; // sorted by line. don't bother scanning further
    }
    return PCodeFoldingRange();
}

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

PCodeFoldingRange QSynEdit::checkFoldRange(PCodeFoldingRanges foldRangesToCheck, int line, bool wantCollapsed, bool acceptFromLine, bool acceptToLine)
{
    for (int i = 0; i< foldRangesToCheck->count(); i++) {
        PCodeFoldingRange range = (*foldRangesToCheck)[i];
        if (((range->fromLine < line) || ((range->fromLine <= line) && acceptFromLine)) &&
          ((range->toLine > line) || ((range->toLine >= line) && acceptToLine))) {
            if (range->collapsed == wantCollapsed) {
                return range;
            }
        }
    }
    return PCodeFoldingRange();
}

PCodeFoldingRange QSynEdit::foldEndAtLine(int line)
{
    foreach(const PCodeFoldingRange& range, mAllFoldRanges->ranges()) {
        if (range->toLine == line ){
            return range;
        } else if (range->fromLine>line)
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
        beginInternalChanges();
        mLineSpacingFactor = newLineSpacingFactor;
        recalcCharExtent();
        endInternalChanges();
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
        beginInternalChanges();
        mRightEdge = newRightEdge;
        invalidate();
        endInternalChanges();
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
        setStatusChanged(StatusChange::ReadOnlyChanged);
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
        beginInternalChanges();
        mDocument->setTabSize(newTabSize);
        invalidate();
        endInternalChanges();
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
        beginInternalChanges();
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
            setSelBegin(selBegin());
            setSelEnd(selEnd());
        }
        updateHScrollbar();
        updateVScrollbar();
        if (bUpdateAll)
            invalidate();
        endInternalChanges();
    }
}

int QSynEdit::tabSize() const
{
    return mDocument->tabSize();
}

void QSynEdit::doAddStr(const QString &s)
{
    beginEditing();
    if (mInserting == false && !selAvail()) {
        switch(mActiveSelectionMode) {
        case SelectionMode::Column: {
            //we can't use blockBegin()/blockEnd()
            CharPos start=selBegin();
            CharPos end=selEnd();
            if (start.line > end.line )
                std::swap(start,end);
            start.ch+=s.length(); // make sure we select a whole char in the start line
            setSelBegin(start);
            setSelEnd(end);
        }
            break;
        default:
            setSelLength(s.length());
        }
    }
    doSetSelText(s);
    endEditing();
}

inline bool isGroupChange(ChangeReason reason){
    return (reason == ChangeReason::AddChar
            || reason == ChangeReason::DeleteChar
            || reason == ChangeReason::DeletePreviousChar);
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
                        keepGoing = (mOptions.testFlag(EditorOption::GroupUndo)
                                     && (lastChange == item->changeReason())
                                     && (isGroupChange(item->changeReason())));
                    }
                    oldChangeNumber=item->changeNumber();
                    lastChange = item->changeReason();
                }
            } while (keepGoing);
        }
    }
    ensureCaretVisible();
    updateModifiedStatusForUndoRedo();
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
        {
            CharPos p;
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
        }
            break;
        case ChangeReason::Selection:
            mRedoList->addRedo(
                        item->changeReason(),
                        mSelectionBegin,
                        mSelectionEnd,
                        QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            setCaretAndSelection(caretXY(), item->changeStartPos(), item->changeEndPos());
            break;
        case ChangeReason::InsertLine:
            Q_ASSERT(mDocument->count()==1);
            properDeleteLine(0,true);
            setCaretXY(fileBegin());
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList{},
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        case ChangeReason::AddChar:
        case ChangeReason::Insert: {
            beginMergeCaretStatusChange();
            QStringList tmpText = getContent(item->changeStartPos(),item->changeEndPos(),item->changeSelMode());
            doDeleteText(item->changeStartPos(),item->changeEndPos(),item->changeSelMode());
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        tmpText,
                        item->changeSelMode(),
                        item->changeNumber());
            setCaretXY(item->changeStartPos());
            endMergeCaretStatusChange();
            break;
        }
        case ChangeReason::ReplaceLine:
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(mDocument->getLine(item->changeStartPos().line)),
                        item->changeSelMode(),
                        item->changeNumber()
                        );
            properSetLine(item->changeStartPos().line,item->changeText()[0], true);
            break;
        case ChangeReason::MoveLine: {
            int fromLine = item->changeEndPos().line;
            int toLine = item->changeStartPos().line;
            properMoveLine(fromLine, toLine, true);
            setSelBegin(CharPos{item->changeStartPos().ch, item->changeStartPos().line-1});
            setSelEnd(CharPos{item->changeEndPos().ch, item->changeEndPos().line-1});
            doMoveSelDown();
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
        }
            break;
        case ChangeReason::DeleteChar:
        case ChangeReason::DeletePreviousChar:
        case ChangeReason::MergeWithNextLine:
        case ChangeReason::MergeWithPrevLine:
        case ChangeReason::Delete: {
            // If there's no selection, we have to set
            // the Caret's position manualy.
//            qDebug()<<"undo delete";
//            qDebug()<<Item->changeText();
//            qDebug()<<Item->changeStartPos().Line<<Item->changeStartPos().Char;
            CharPos startPos = item->changeStartPos();
            CharPos endPos = item->changeEndPos();
            if (item->changeSelMode()==SelectionMode::Column) {
                int xFrom = charToGlyphLeft(startPos.line, startPos.ch);
                int xTo = charToGlyphLeft(endPos.line, endPos.ch);
                if (xFrom > xTo)
                    std::swap(xFrom, xTo);
                startPos.ch = xposToGlyphStartChar(startPos.line,xFrom);
            }
            beginMergeCaretStatusChange();
            doInsertText(startPos,item->changeText(),item->changeSelMode(),
                         startPos.line,
                         endPos.line);
            if (item->changeReason() == ChangeReason::DeleteChar
                    || item->changeReason() == ChangeReason::MergeWithNextLine)
                setCaretXY(item->changeStartPos());
            else
                setCaretXY(item->changeEndPos());
            endMergeCaretStatusChange();
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        }
        case ChangeReason::LineBreak:{
            QString s;
            if (!item->changeText().isEmpty()) {
                s=item->changeText()[0];
            }
            // If there's no selection, we have to set
            // the Caret's position manualy.
            setCaretXY(item->changeStartPos());
            if (mCaretY >= 0) {
                QString tmpStr = mDocument->getLine(mCaretY);
                if ( (mCaretX > tmpStr.length()) && (leftSpaces(s) == 0))
                    tmpStr = tmpStr + QString(mCaretX - tmpStr.length(), ' ');
                if (tmpStr.trimmed().isEmpty()) {
                    properDeleteLine(mCaretY, false);
                } else {
                    properDeleteLine(mCaretY+1, false);
                }
                properSetLine(mCaretY, tmpStr+s, true);
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
    updateModifiedStatusForUndoRedo();
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
            setCaretXY(item->changeStartPos());
            break;
        case ChangeReason::LeftTop:
        {
            CharPos p;
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
        }
            break;
        case ChangeReason::Selection:
            mUndoList->restoreChange(
                        item->changeReason(),
                        mSelectionBegin,
                        mSelectionEnd,
                        QStringList(),
                        mActiveSelectionMode,
                        item->changeNumber());
            setCaretAndSelection(
                        caretXY(),
                        item->changeStartPos(),
                        item->changeEndPos());
            break;
        case ChangeReason::MoveLine: {
            int fromLine = item->changeStartPos().line;
            int toLine = item->changeEndPos().line;
            properMoveLine(fromLine, toLine, true);
            mUndoList->restoreChange(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
        }
            break;
        case ChangeReason::InsertLine:
            Q_ASSERT(mDocument->count()==0);
            properInsertLine(0,"",true);
            setCaretXY(fileBegin());
            mUndoList->restoreChange(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList{},
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        case ChangeReason::ReplaceLine:
            mUndoList->restoreChange(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(mDocument->getLine(item->changeStartPos().line)),
                        item->changeSelMode(),
                        item->changeNumber()
                        );
            properSetLine(item->changeStartPos().line,item->changeText()[0], true);
            break;
        case ChangeReason::AddChar:
        case ChangeReason::Insert:
            setCaretAndSelection(
                        item->changeStartPos(),
                        item->changeStartPos(),
                        item->changeStartPos());
            doInsertText(item->changeStartPos(),item->changeText(), item->changeSelMode(),
                         item->changeStartPos().line,
                         item->changeEndPos().line);
            setCaretXY(item->changeEndPos());
            mUndoList->restoreChange(item->changeReason(),
                                 item->changeStartPos(),
                                 item->changeEndPos(),
                                 QStringList(),
                                 item->changeSelMode(),
                                 item->changeNumber());
            break;
        case ChangeReason::DeleteChar:
        case ChangeReason::DeletePreviousChar:
        case ChangeReason::MergeWithNextLine:
        case ChangeReason::MergeWithPrevLine:
        case ChangeReason::Delete:
            doDeleteText(item->changeStartPos(),item->changeEndPos(),item->changeSelMode());
            mUndoList->restoreChange(item->changeReason(), item->changeStartPos(),
                                 item->changeEndPos(),item->changeText(),
                                 item->changeSelMode(),item->changeNumber());
            setCaretXY(item->changeStartPos());
            break;
        case ChangeReason::LineBreak: {
            CharPos CaretPt = item->changeStartPos();
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
        int firstLine = selBegin().line;
        int lastLine = selEnd().line;

        switch(mActiveSelectionMode) {
        case SelectionMode::Normal:{
            int charFrom = selBegin().ch;
            int charTo = selEnd().ch;
            PCodeFoldingRange foldRange = foldStartAtLine(selEnd().line);
            QString s = mDocument->getLine(lastLine);
            if ((foldRange) && foldRange->collapsed && charTo>s.length()) {
                s=s+mSyntaxer->foldString(s);
                if (charTo>s.length()) {
                    lastLine = foldRange->toLine;
                    charTo = mDocument->getLine(lastLine).length();
                }
            }
            if (firstLine == lastLine)
                return  mDocument->getLine(firstLine).mid(charFrom, charTo - charFrom);
            else {
                QString result = mDocument->getLine(firstLine).mid(charFrom);
                result+= lineBreak();
                for (int i = firstLine + 1; i<=lastLine - 1; i++) {
                    result += mDocument->getLine(i);
                    result+=lineBreak();
                }
                const QString &line = mDocument->getLine(lastLine);
                result.append(line.constData(), charTo);
                return result;
            }
        }
        case SelectionMode::Column:
        {
            int xFrom, xTo;
            firstLine = selBegin().line;
            xFrom = charToGlyphLeft(selBegin().line, selBegin().ch);
            lastLine = selEnd().line;
            xTo = charToGlyphLeft(selEnd().line, selEnd().ch);
            if (xFrom > xTo)
              std::swap(xFrom, xTo);
            if (firstLine>lastLine)
              std::swap(firstLine,lastLine);
            QString result;
            for (int i = firstLine; i <= lastLine; i++) {
                int l = xposToGlyphStartChar(i,xFrom);
                int r = xposToGlyphStartChar(i,xTo);
                QString s = mDocument->getLine(i);
                result += s.mid(l,r-l);
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
    CharPos begin=selBegin();
    CharPos end = selEnd();
    if (begin.line == end.line)
        return (end.ch - begin.ch);
    int count = mDocument->getLine(begin.line).length()-begin.ch;
    int lineEndCount = lineBreak().length();
    count += lineEndCount;
    for (int i=begin.line+1;i<end.line;i++) {
        count += mDocument->getLine(i).length();
        count += lineEndCount;
    }
    count+= end.ch;
    return count;
}

QStringList QSynEdit::getContent(CharPos startPos, CharPos endPos, SelectionMode mode) const
{
    Q_ASSERT(validInDoc(startPos));
    Q_ASSERT(validInDoc(endPos));

    QStringList result;
    if (startPos==endPos) {
        return result;
    }
    if (startPos>endPos) {
        std::swap(startPos,endPos);
    }
    int firstLine = startPos.line;
    int lastLine = endPos.line;

    switch(mode) {
    case SelectionMode::Normal:{
        int chFrom = startPos.ch;
        int chTo = endPos.ch;
        PCodeFoldingRange foldRange = foldStartAtLine(endPos.line);
        QString s = mDocument->getLine(lastLine);
        if ((foldRange) && foldRange->collapsed && chTo>s.length()) {
            s=s+mSyntaxer->foldString(s);
            if (chTo>s.length()) {
                lastLine = foldRange->toLine;
                chTo = s.length();
            }
        }
        if (firstLine == lastLine) {
            result.append(mDocument->getLine(firstLine).mid(chFrom, chTo - chFrom));
        } else {
            result.append(mDocument->getLine(firstLine).mid(chFrom));
            for (int i = firstLine + 1; i<=lastLine - 1; i++) {
                result.append(mDocument->getLine(i));
            }
            result.append(mDocument->getLine(lastLine).left(chTo));
        }
    }
        break;
    case SelectionMode::Column: {
        int xFrom, xTo;
        firstLine = selBegin().line;
        xFrom = charToGlyphLeft(selBegin().line, selBegin().ch);
        lastLine = selEnd().line;
        xTo = charToGlyphLeft(selEnd().line, selEnd().ch);
        if (xFrom > xTo)
          std::swap(xFrom, xTo);
        if (firstLine>lastLine)
          std::swap(firstLine,lastLine);
        for (int i = firstLine; i <= lastLine; i++) {
          int l = xposToGlyphStartChar(i,xFrom);
          int r = xposToGlyphStartChar(i,xTo);
          QString s = mDocument->getLine(i);
          result.append(s.mid(l,r-l));
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
    if (mDocument->empty())
        return QString();
    return getDisplayStringAtLine(mCaretY);
}

QString QSynEdit::lineText() const
{
    if (mDocument->empty())
        return QString();
    return mDocument->getLine(mCaretY);
}

QString QSynEdit::lineText(int line) const
{
    return mDocument->getLine(line);
}

size_t QSynEdit::lineSeq(int line) const
{
    return mDocument->getLineSeq(line);
}

int QSynEdit::findPrevLineBySeq(int startLine, size_t lineSeq) const
{
    //start at 0
    //-1 not found
    return mDocument->findPrevLineBySeq(startLine, lineSeq);
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

const std::shared_ptr<const Document> QSynEdit::document() const
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

bool QSynEdit::isSpaceChar(const QChar &ch) const
{
    return mSyntaxer->isSpaceChar(ch);
}

void QSynEdit::prepareSyntaxerState(Syntaxer &syntaxer, int lineIndex) const
{
    Q_ASSERT(validLine(lineIndex));
    if (lineIndex == 0) {
        syntaxer.resetState();
    } else {
        syntaxer.setState(mDocument->getSyntaxState(lineIndex-1));
    }
    syntaxer.setLine(lineIndex, mDocument->getLine(lineIndex), mDocument->getLineSeq(lineIndex));
}

void QSynEdit::prepareSyntaxerState(Syntaxer &syntaxer, int lineIndex, const QString lineText, size_t lineSeq) const
{
    Q_ASSERT(validLine(lineIndex));
    if (lineIndex == 0) {
        syntaxer.resetState();
    } else {
        syntaxer.setState(mDocument->getSyntaxState(lineIndex-1));
    }
    syntaxer.setLine(lineIndex, lineText, lineSeq);
}


void QSynEdit::moveCaretHorz(int deltaX, bool isSelection)
{
    if (mDocument->empty())
        return;
    CharPos ptDst = caretXY();
    QString s = displayLineText();
    int nLineLen = s.length();
    if (!isSelection && selAvail() && (deltaX!=0)) {
        if (deltaX<0)
            ptDst = selBegin();
        else
            ptDst = selEnd();
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
        int glyphIndex = mDocument->charToGlyphIndex(ptDst.line,ptDst.ch);
        if (bChangeY && (deltaX == -1) && (glyphIndex==0) && (ptDst.line > 0)) {
            // end of previous line
            if (mActiveSelectionMode==SelectionMode::Column) {
                return;
            }
            int row = lineToRow(ptDst.line);
            row--;
            int line = rowToLine(row);
            if (line!=ptDst.line && line>=0) {
                ptDst.line = line;
                ptDst.ch = getDisplayStringAtLine(ptDst.line).length();
            }
        } else if (bChangeY && (deltaX == 1) && (glyphIndex >= mDocument->glyphCount(ptDst.line)) && (ptDst.line < mDocument->count()-1)) {
            // start of next line
            if (mActiveSelectionMode==SelectionMode::Column) {
                return;
            }
            int row = lineToRow(ptDst.line);
            row++;
            int line = rowToLine(row);
            if (line!=ptDst.line && line<=mDocument->count()) {
                ptDst.line = line;
                ptDst.ch = 0;
            }
        } else {
            ptDst.ch = std::max(0, mDocument->glyphStartChar(ptDst.line, glyphIndex + deltaX));
            // don't go past last char when ScrollPastEol option not set
            if ((deltaX > 0) && bChangeY)
              ptDst.ch = std::min(ptDst.ch, nLineLen);
        }
    }
    // set caret and block begin / end
    beginInternalChanges();
    //ensureCaretVisible();
    moveCaretAndSelection(mSelectionBegin, ptDst, isSelection);
    endInternalChanges();
}

void QSynEdit::moveCaretVert(int deltaY, bool isSelection)
{
    if (mDocument->empty())
        return;
    DisplayCoord ptO = displayXY();
    DisplayCoord ptDst = ptO;
    if (!isSelection && selAvail()) {
        if (deltaY<0)
            ptDst = bufferToDisplayPos(selBegin());
        else
            ptDst = bufferToDisplayPos(selEnd());
    }
    ptDst.row+=deltaY;

    if (deltaY >= 0) {
        if (rowToLine(ptDst.row) >= mDocument->count()) {
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

    CharPos vDstLineChar;
    if (ptDst.row == ptO.row && isSelection && deltaY!=0) {
        if (ptDst.row==1 && mDocument->count()>1) {
            vDstLineChar.ch=0;
            vDstLineChar.line=0;
        } else {
            vDstLineChar.line = mDocument->count()-1;
            vDstLineChar.ch = mDocument->getLine(vDstLineChar.line).length();
        }
    } else
        vDstLineChar = displayToBufferPos(ptDst);

    if (mActiveSelectionMode==SelectionMode::Column) {
        int w=mDocument->lineWidth(vDstLineChar.line);
        if (w+1<ptO.x)
            return;
    }

    int SaveLastCaretX = mLastCaretColumn;

    // set caret and block begin / end
    beginInternalChanges();
    //ensureCaretVisible();
    moveCaretAndSelection(mSelectionBegin, vDstLineChar, isSelection);
    endInternalChanges();

    // Restore fLastCaretX after moving caret, since
    // UpdateLastCaretX, called by SetCaretXYEx, changes them. This is the one
    // case where we don't want that.
    mLastCaretColumn = SaveLastCaretX;
}

void QSynEdit::moveCaretAndSelection(const CharPos &ptBefore, const CharPos &ptAfter, bool isSelection, bool ensureCaretVisible)
{
    beginInternalChanges();
    if (isSelection) {
        if (!selAvail())
          setSelBegin(ptBefore);
        setSelEnd(ptAfter);
    } else
        setSelBegin(ptAfter);
    internalSetCaretXY(ptAfter,ensureCaretVisible);
    endInternalChanges();
}

void QSynEdit::moveCaretToLineStart(bool isSelection)
{
    if (mDocument->empty())
        return;
    int newX;
    // home key enhancement
    if (mOptions.testFlag(EditorOption::EnhanceHomeKey)) {
        QString s = mDocument->getLine(mCaretY);

        int first_nonblank = 0;
        int vMaxX = s.length();
        while ((first_nonblank < vMaxX) && (s[first_nonblank] == ' ' || s[first_nonblank] == '\t')) {
            first_nonblank++;
        }
        newX = mCaretX;

        if ((newX > first_nonblank)
                || (newX == 0))
            newX = first_nonblank;
        else
            newX = 0;
    } else
        newX = 0;
    moveCaretAndSelection(caretXY(), CharPos{newX, mCaretY}, isSelection);
}

void QSynEdit::moveCaretToLineEnd(bool isSelection, bool ensureCaretVisible)
{
    if (mDocument->empty())
        return;
    int vNewX;
    if (mOptions.testFlag(EditorOption::EnhanceEndKey)) {
        QString vText = displayLineText();
        int vLastNonBlank = vText.length()-1;
        int vMinX = 0;
        while ((vLastNonBlank >= vMinX) && (vText[vLastNonBlank] == ' ' || vText[vLastNonBlank] =='\t'))
            vLastNonBlank--;
        vNewX = mCaretX;
        if ((vNewX <= vLastNonBlank) || (vNewX == vText.length()))
            vNewX = vLastNonBlank+1;
        else
            vNewX = vText.length();
    } else
        vNewX = displayLineText().length();

    moveCaretAndSelection(caretXY(), CharPos{vNewX, mCaretY}, isSelection, ensureCaretVisible);
}

void QSynEdit::doGotoBlockStart(bool isSelection)
{
    if (mDocument->empty())
        return;
    //todo: handle block other than {}
    if (document()->braceLevel(mCaretY)==0) {
        doGotoEditorStart(isSelection);
    } else if (document()->blockStarted(mCaretY)==0){
        int line=mCaretY-1;
        while (line>=0) {
            if (document()->blockStarted(line)>document()->blockEnded(line)) {
                CharPos newPos{0,line+1};
                if (!isSelection)
                    setCaretXY(newPos);
                else
                    setCaretAndSelection(newPos, newPos, caretXY());
                setTopPos((lineToRow(line)-1)*mTextHeight);
                return;
            }
            line--;
        }
    }
}

void QSynEdit::doGotoBlockEnd(bool isSelection)
{
    if (mDocument->empty())
        return;
    //todo: handle block other than {}
    if (document()->blockLevel(mCaretY)==0) {
        doGotoEditorEnd(isSelection);
    } else if (document()->blockEnded(mCaretY)==0){
        int line=mCaretY;
        while (line<lineCount()) {
            if (document()->blockEnded(line)>document()->blockStarted(line)) {
                CharPos newPos{0,line-1};
                if (!isSelection)
                    setCaretXY(newPos);
                else
                    setCaretAndSelection(newPos, newPos, caretXY());
                setTopPos((lineToRow(line) - mLinesInWindow)*mTextHeight);
                return;
            }
            line++;
        }
    }
}

void QSynEdit::doGotoEditorStart(bool isSelection)
{
    if (mDocument->empty())
        return;
    CharPos newPos{fileBegin()};
    if (!isSelection)
        setCaretXY(newPos);
    else
        setCaretAndSelection(newPos, newPos, caretXY());
}

void QSynEdit::doGotoEditorEnd(bool isSelection)
{
    if (mDocument->empty())
        return;
    CharPos newPos{fileEnd()};
    if (!isSelection)
        setCaretXY(newPos);
    else
        setCaretAndSelection(newPos, newPos, caretXY());
}

void QSynEdit::deleteSelection()
{
    if (mDocument->empty() || readOnly())
        return;
    CharPos startPos=selBegin();
    CharPos endPos=selEnd();
    doDeleteText(startPos,endPos,mActiveSelectionMode);
    setCaretXY(startPos);
}

void QSynEdit::setSelTextPrimitive(const QStringList &text)
{
    SelectionMode mode = mActiveSelectionMode;
    beginInternalChanges();
    bool groupUndo=false;
    CharPos startPos = selBegin();
    CharPos endPos = selEnd();
    bool selChanged = false;
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
            setSelBegin(startPos);
            setSelEnd(endPos);
        } else
            setCaretXY(startPos);
        selChanged = true;
    }
    if (!text.isEmpty()) {
        doInsertText(caretXY(),text,mode,mSelectionBegin.line,mSelectionEnd.line);
    }
    if (groupUndo) {
        endEditing();
    }
    if (selChanged)
        setStatusChanged(StatusChange::Selection);
    endInternalChanges();
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
    }
    CharPos startOfBlock = selBegin();
    CharPos endOfBlock = selEnd();
    mSelectionBegin = startOfBlock;
    mSelectionEnd = endOfBlock;
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
    CharPos ptCurrent;
    CharPos ptStart;
    CharPos ptEnd;
    if (!selAvail())
        sOptions.setFlag(ssoSelectedOnly,false);
    if (sOptions.testFlag(ssoSelectedOnly)) {
        ptStart = selBegin();
        ptEnd = selEnd();
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
        ptStart.line = 0;
        ptEnd.line = mDocument->count()-1;
        ptEnd.ch = mDocument->getLine(ptEnd.line).length();
        if (bFromCursor) {
            Qt::CaseSensitivity caseSensitivity = sOptions.testFlag(ssoMatchCase)?Qt::CaseSensitive:Qt::CaseInsensitive;
            if (selAvail()
                    && matchedCallback // we are replacing
                    && QString::compare(sSearch,sReplace, Qt::CaseSensitive)!=0
                    && ((bBackward && selBegin() == caretXY())
                        || (!bBackward && selEnd() == caretXY()))
                    && selCount() == sSearch.length()
                    && QString::compare(sSearch,selText(), caseSensitivity)==0 ) {
                if (bBackward) {
                    ptEnd = selEnd();
                } else {
                    ptStart = selBegin();
                }
            } else {
                if (bBackward) {
                    ptEnd = caretXY();
                } else {
                    ptStart = caretXY();
                }
            }
        }
        if (bBackward)
            ptCurrent = ptEnd;
        else
            ptCurrent = ptStart;
    }
    CharPos originCaretXY=caretXY();
    // initialize the search engine
    searchEngine->setOptions(sOptions);
    searchEngine->setPattern(sSearch);
    // search while the current search position is inside of the search range
    bool dobatchReplace = false;
    {
        auto action = finally([&,this]{
            if (dobatchReplace) {
                endInternalChanges();
                endEditing();
            }
        });
        int i;
        // If it's a search only we can leave the procedure now.
        SearchAction searchAction = SearchAction::Exit;
        while ((ptCurrent.line >= ptStart.line) && (ptCurrent.line <= ptEnd.line)) {
            int nInLine = searchEngine->findAll(mDocument->getLine(ptCurrent.line));
            int iResultOffset = 0;
            if (bBackward)
                i = searchEngine->resultCount()-1;
            else
                i = 0;
            // Operate on all results in this line.
            while (nInLine > 0) {
                // An occurrence may have been replaced with a text of different length
                int nFound = searchEngine->result(i) + iResultOffset;
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
                setSelBegin(ptCurrent);
                ptCurrent.ch += nSearchLen;
                setSelEnd(ptCurrent);

                if (bBackward)
                    internalSetCaretXY(selBegin(),false);
                else
                    internalSetCaretXY(selEnd(),false);
                ensureCaretVisible(true);

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
                        beginInternalChanges();
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
                                setSelEnd(ptEnd);
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
                ptStart.line = 0;
                ptEnd.line = mDocument->count()-1;
                ptEnd.ch = mDocument->getLine(ptEnd.line).length();
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

void QSynEdit::properSetLine(int line, const QString &sLineText, bool parseToEnd)
{
    mDocument->putLine(line,sLineText);
    reparseLines(line, line + 1, parseToEnd && mSyntaxer->needsLineState());
}

void QSynEdit::properInsertLine(int line, const QString &sLineText, bool parseToEnd)
{
    mDocument->insertLine(line, sLineText);
    processFoldsOnLinesInserted(line,1);
    if (parseToEnd)
        onLinesInserted(line, 1);
    else
        reparseLines(line,line+1, false);
    emit linesInserted(line, 1);
}

void QSynEdit::properDeleteLines(int line, int count, bool parseToEnd)
{
    if (count<=0)
        return;
    mDocument->deleteLines(line, count);
    processFoldsOnLinesDeleted(line, count);
    if (parseToEnd)
        onLinesDeleted(line,count);
    emit linesDeleted(line,count);
}

void QSynEdit::properInsertLines(int line, int count, bool parseToEnd)
{
    if (count<=0)
        return;
    mDocument->insertLines(line, count);
    processFoldsOnLinesInserted(line, count);
    if (parseToEnd)
        onLinesInserted(line,count);
    else
        reparseLines(line,line+count, false);
    emit linesInserted(line, count);
}

void QSynEdit::properMoveLine(int from, int to, bool parseToEnd)
{
    if (from==to)
        return;
    mDocument->moveLine(from, to);
    processFoldsOnLineMoved(from,to);
    int minLine = std::min(from,to);
    int maxLine = std::max(from,to);
    reparseLines(minLine, maxLine+1, parseToEnd && mSyntaxer->needsLineState());
    emit lineMoved(from, to);
}

void QSynEdit::doDeleteText(CharPos startPos, CharPos endPos, SelectionMode mode)
{
    if (startPos == endPos)
        return;
    if (mode == SelectionMode::Normal) {
        PCodeFoldingRange foldRange = foldStartAtLine(endPos.line);
        QString s = mDocument->getLine(endPos.line);
        if ((foldRange) && foldRange->collapsed && endPos.ch>=s.length()) {
            QString newS=s+mSyntaxer->foldString(s);
            if ((startPos.ch<s.length() || startPos.line<endPos.line)
                    && endPos.ch>=newS.length() ) {
                //selection has whole block
                endPos.line = foldRange->toLine;
                endPos.ch = mDocument->getLine(endPos.line).length();
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
            QString startLineLeft = mDocument->getLine(startPos.line).left(startPos.ch);
            QString newString = startLineLeft + mDocument->getLine(endPos.line).mid(endPos.ch);
            // Delete all lines in the selection range.
            if (startLineLeft.trimmed().isEmpty()) {
                properDeleteLines(startPos.line, endPos.line - startPos.line, false);
            } else {
                properDeleteLines(startPos.line+1, endPos.line - startPos.line, false);
            }
            setCaretXY(startPos);
            properSetLine(startPos.line,newString,true);
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
        for (int i = firstLine; i <= lastLine; i++) {
            int l = xposToGlyphStartChar(i,xFrom);
            int r = xposToGlyphStartChar(i,xTo);
            QString s = mDocument->getLine(i);
            s.remove(l,r-l);
            properSetLine(i,s, i==lastLine);
        }
        // Lines never get deleted completely, so keep caret at end.
        CharPos newStartPos = startPos;
        CharPos newEndPos = endPos;
        newStartPos.line = firstLine;
        newStartPos.ch = xposToGlyphStartChar(newStartPos.line,xFrom);
        newEndPos.line = lastLine;
        newEndPos.ch = xposToGlyphStartChar(newEndPos.line, xFrom);
        setCaretAndSelection(newStartPos,newStartPos,newEndPos);
        // Column deletion never removes a line entirely, so no mark
        // updating is needed here.
        break;
    }
    }
    endEditingWithoutUndo();
    addChangeToUndo(ChangeReason::Delete,
            startPos,
            endPos,
            deleted,
            mode);
}

void QSynEdit::doInsertText(const CharPos& pos,
                           const QStringList& text,
                           SelectionMode mode, int startLine, int endLine) {
    if (text.isEmpty())
        return;
    if (startLine>endLine)
        std::swap(startLine,endLine);
    if (mode == SelectionMode::Normal && !mDocument->empty()) {
        PCodeFoldingRange foldRange = foldStartAtLine(pos.line);
        QString s = mDocument->getLine(pos.line);
        if ((foldRange) && foldRange->collapsed && pos.ch>s.length()+1)
            return;
    }
    int insertedLines = 0;
    CharPos newPos;
    switch(mode){
    case SelectionMode::Normal:
        doInsertTextByNormalMode(pos,text, newPos);
        setCaretXY(newPos);
        break;
    case SelectionMode::Column:{
        CharPos bb=selBegin();
        CharPos be=selEnd();
        int lenBefore = mDocument->getLine(be.line).length();
        insertedLines = doInsertTextByColumnMode(pos, text, startLine,endLine);
        onLinesInserted(endLine-insertedLines+1,insertedLines);
        if (!text.isEmpty()) {
            int textLen = mDocument->getLine(be.line).length()-lenBefore;
            bb.ch+=textLen;
            be.ch+=textLen;
            setCaretAndSelection(bb,bb,be);
        }
    }
        break;
    }

}

void QSynEdit::doInsertTextByNormalMode(const CharPos& pos, const QStringList& text, CharPos &newPos)
{
    QString sLeftSide;
    QString sRightSide;
    QString str;
    bool bChangeScroll;
    beginEditing();
//    int SpaceCount;
    if (mDocument->empty()) {
        properInsertLine(0,"",false);
        addChangeToUndo(ChangeReason::InsertLine,
                fileBegin(),fileBegin(),
                QStringList(),SelectionMode::Normal);
    }
    QString line = mDocument->getLine(pos.line);
    sLeftSide = line.left(pos.ch);
    sRightSide = line.mid(pos.ch);
    int caretY=pos.line;
    if (text.length()>1) {
        // step1: insert the first line of Value into current line
        if (!mUndoing && mSyntaxer->language()==ProgrammingLanguage::CPP && mOptions.testFlag(EditorOption::AutoIndent)) {
            QString s = text[0];
            if (sLeftSide.isEmpty()) {
                s=s.trimmed();
                sLeftSide = genSpaces(calcIndentSpaces(caretY,s,true));
            }
            str = sLeftSide + s;
        } else
            str = sLeftSide + text[0];
        if (sLeftSide.trimmed().isEmpty()) {
            properInsertLines(caretY, text.length()-1, true);
        } else {
            properInsertLines(caretY+1, text.length()-1, true);
        }
        properSetLine(caretY, str, false);
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
                str = genSpaces(indentSpaces)+trimLeft(str);
            }
            properSetLine(caretY, str, i==text.length()-1);
        }
    } else {
        str = sLeftSide + text[0] + sRightSide;
        properSetLine(caretY, str, true);
    }

    bChangeScroll = !mOptions.testFlag(EditorOption::ScrollPastEol);
    mOptions.setFlag(EditorOption::ScrollPastEol);
    auto action = finally([&,this]{
        if (bChangeScroll)
            mOptions.setFlag(EditorOption::ScrollPastEol,false);
    });
    newPos=CharPos{str.length() - sRightSide.length(),caretY};
    //onLinesPutted(startLine-1,result+1);
    addChangeToUndo(ChangeReason::Insert,
            pos,newPos,
            QStringList(),SelectionMode::Normal);
    endEditing();
}

int QSynEdit::doInsertTextByColumnMode(const CharPos& pos, const QStringList& text, int startLine, int endLine)
{
    QString str;
    QString tempString;
    int line;
    int len;
    CharPos  lineBreakPos;
    int result = 0;
    int startGlyph = mDocument->charToGlyphIndex(pos.line,pos.ch);
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
            tempString = QString(startGlyph,' ') + str;
            mDocument->addLine("");
            if (!mUndoing) {
                result++;
                lineBreakPos.line = line - 1;
                lineBreakPos.ch = mDocument->getLine(line - 2).length() + 1;
                addChangeToUndo(ChangeReason::LineBreak,
                                 lineBreakPos,
                                 lineBreakPos,
                                 QStringList(), SelectionMode::Normal);
            }
        } else {
            tempString = mDocument->getLine(line);
            len = mDocument->getLineGlyphsCount(line);
            if (len < startGlyph) {
                insertPos = tempString.length()+1;
                tempString = tempString + QString(startGlyph - len,' ') + str;
            } else {
                insertPos = xposToGlyphStartChar(line,startGlyph);
                tempString.insert(insertPos,str);
            }
        }
        properSetLine(line, tempString, false);
        // Add undo change here from PasteFromClipboard
        if (!mUndoing) {
            addChangeToUndo(ChangeReason::Insert,
                    CharPos{insertPos, line},
                    CharPos{insertPos+str.length(), line},
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

void QSynEdit::deleteFromTo(const CharPos &start, const CharPos &end)
{
    if (mReadOnly)
        return;
    if ((start.ch != end.ch) || (start.line != end.line)) {
        beginEditing();
        addCaretToUndo();
        addSelectionToUndo();
        setSelBegin(start);
        setSelEnd(end);
        doDeleteText(start,end,SelectionMode::Normal);
        endEditing();
        setCaretXY(start);
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
    beginInternalChanges();

    auto action=finally([this] {
        endInternalChanges();
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
        beginInternalChanges();
        int gap = (lineToRow(caretY())-1) * mTextHeight - topPos();
        moveCaretVert(counter, command == EditCommand::SelPageUp || command == EditCommand::SelPageDown);
        setTopPos((lineToRow(caretY())-1) * mTextHeight - gap);
        endInternalChanges();
        break;
    }
    case EditCommand::PageTop:
    case EditCommand::SelPageTop:
        moveCaretVert(yposToRow(0)-lineToRow(mCaretY), command == EditCommand::SelPageTop);
        break;
    case EditCommand::PageBottom:
    case EditCommand::SelPageBottom:
        moveCaretVert(yposToRow(0) + mLinesInWindow-1-lineToRow(mCaretY), command == EditCommand::SelPageBottom);
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
            moveCaretAndSelection(caretXY(), *((CharPos *)(pData)), command == EditCommand::SelGotoXY);
        break;
    // word selection
    case EditCommand::PrevWordBegin:
    case EditCommand::SelPrevWordBegin:
    {
        CharPos CaretNew = prevWordBegin(caretXY());
        moveCaretAndSelection(caretXY(), CaretNew, command == EditCommand::SelPrevWordBegin);
        break;
    }
    case EditCommand::nextWordBegin:
    case EditCommand::SelNextWordBegin:
    {
        CharPos CaretNew = nextWordBegin(caretXY());
        moveCaretAndSelection(caretXY(), CaretNew, command == EditCommand::SelNextWordBegin);
        break;
    }
    case EditCommand::SelWord:
        selCurrentToken();
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
        doDeletePrevChar();
        break;
    case EditCommand::DeleteChar:
        doDeleteCurrentChar();
        break;
    case EditCommand::DeleteWord:
        doDeleteCurrentToken();
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
    case EditCommand::LineBreak:
        doBreakLine();
        break;
    case EditCommand::LineBreakAtEnd:
        beginEditing();
        addLeftTopToUndo();
        addCaretToUndo();
        addSelectionToUndo();
        if (mCaretY>=0 && mCaretY<mDocument->count())
            mCaretX = mDocument->getLine(mCaretY).length();
        doBreakLine();
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
        CharPos coord = getMatchingBracket();
        if (coord.ch!=0 && coord.line!=0)
            setCaretXY(coord);
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
    beginInternalChanges();
    mEditingCount++;
}

void QSynEdit::endEditingWithoutUndo()
{
    mEditingCount--;
    Q_ASSERT(mEditingCount>=0);
    if (mEditingCount==0) {
        rescanFolds();
    }
    endInternalChanges();
}

bool QSynEdit::isIdentChar(const QChar &ch) const
{
    return mSyntaxer->isIdentChar(ch);
}

bool QSynEdit::isIdentStartChar(const QChar &ch) const
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
        setSelBegin(caretXY());
        setSelEnd(caretXY());
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
    int x=event->pos().x();
    int y=event->pos().y();

    QAbstractScrollArea::mousePressEvent(event);

    if (button == Qt::RightButton) {
        if (mOptions.testFlag(EditorOption::RightMouseMovesCursor) &&
                ( (selAvail() && ! inSelection(displayToBufferPos(pixelsToGlyphPos(x, y))))
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
        if (bWasSel && mOptions.testFlag(EditorOption::DragDropEditing) && (x >= mGutterWidth + 2)
                && (mActiveSelectionMode == SelectionMode::Normal) && inSelection(displayToBufferPos(pixelsToGlyphPos(x, y))) ) {
            bStartDrag = true;
        }
        if (bStartDrag && !mReadOnly) {
            mStateFlags.setFlag(StateFlag::WaitForDragging);
        } else {
            if (event->modifiers() == Qt::ShiftModifier) {
                //BlockBegin and BlockEnd are restored to their original position in the
                //code from above and SetBlockEnd will take care of proper invalidation
                setSelEnd(caretXY());
            } else if (mOptions.testFlag(EditorOption::AltSetsColumnMode)) {
                if (event->modifiers() == Qt::AltModifier && !mReadOnly)
                    setActiveSelectionMode(SelectionMode::Column);
                else
                    setActiveSelectionMode(SelectionMode::Normal);
                //Selection mode must be set before calling SetBlockBegin
                setSelBegin(caretXY());
            }
            computeScroll(false);
        }
    }
}

void QSynEdit::mouseReleaseEvent(QMouseEvent *event)
{
    beginInternalChanges();
    auto action = finally([this](){
       endInternalChanges();
    });
    QAbstractScrollArea::mouseReleaseEvent(event);
    int x=event->pos().x();
    /* int Y=event->pos().y(); */

    if (!mMouseMoved && (x < mGutterWidth )) {
        processGutterClick(event);
    }

    if (mStateFlags.testFlag(StateFlag::WaitForDragging) &&
            !mStateFlags.testFlag(StateFlag::DblClicked)) {
        computeCaret();
        if (! (event->modifiers() & Qt::ShiftModifier))
            setSelBegin(caretXY());
        setSelEnd(caretXY());
        mStateFlags.setFlag(StateFlag::WaitForDragging, false);
    }
    mStateFlags.setFlag(StateFlag::DblClicked,false);
    ensureLineAlignedWithTop();
    ensureCaretVisible();
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
            selCurrentToken();
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
            invalidateLines(selBegin().line,selEnd().line+1);
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
        mDragging = true;
        mDragCaretSave = caretXY();
        mDragSelBeginSave = selBegin();
        mDragSelEndSave = selEnd();
        CharPos coord = displayToBufferPos(pixelsToNearestGlyphPos(event->pos().x(),
                                                                        event->pos().y()));
        internalSetCaretXY(coord);
        setSelBegin(mDragSelBeginSave);
        setSelEnd(mDragSelEndSave);
        mDocument->addLine(""); //add a line to handle drag to the last
        showCaret();
        computeScroll(true);
    }
}

void QSynEdit::dropEvent(QDropEvent *event)
{
    //mScrollTimer->stop();

    CharPos coord = displayToBufferPos(pixelsToNearestGlyphPos(event->pos().x(),
                                                                    event->pos().y()));
    if (
            (event->proposedAction() == Qt::DropAction::CopyAction
             && coord>mDragSelBeginSave && coord<mDragSelEndSave)
             ||
             (event->proposedAction() != Qt::DropAction::CopyAction
             && coord>=mDragSelBeginSave && coord<=mDragSelEndSave)
            ) {
        properDeleteLine(mDocument->count()-1,true);
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
    // coord = ensureBufferCoordValid(coord);
    bool lastLineUsed = (coord.line == mDocument->count()-1);
    int topPos = mTopPos;
    int leftPos = mLeftPos;
    QStringList text=splitStrings(event->mimeData()->text());
    beginEditing();
    if (lastLineUsed) {
        int line=mDocument->count()-1;
        QString s=mDocument->getLine(line);
        addChangeToUndo(ChangeReason::LineBreak,
                         CharPos{s.length(),line},
                         CharPos{s.length(),line}, QStringList(), SelectionMode::Normal);
    } else
        properDeleteLine(mDocument->count()-1, true);
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
                QString s=mDocument->getLine(line);
                beginEditing();
                mDocument->addLine("");

                addChangeToUndo(ChangeReason::LineBreak,
                                     CharPos{s.length()+1,line},
                                     CharPos{s.length()+1,line}, QStringList(), SelectionMode::Normal);
                endEditing();
                coord.line = line;
                coord.ch=0;
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
    mDragging = false;
    mDropped = true;
    topPos = calcLineAlignedTopPos(topPos, false);
    setTopPos(topPos);
    setLeftPos(leftPos);
    setCaretXY(coord);
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
    CharPos coord = displayToBufferPos(pixelsToNearestGlyphPos(x,y));
    internalSetCaretXY(coord);
    setSelBegin(mDragSelBeginSave);
    setSelEnd(mDragSelEndSave);
    showCaret();
}

void QSynEdit::dragLeaveEvent(QDragLeaveEvent *)
{
    mDragging = false;
    properDeleteLine(mDocument->count()-1,true);
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

void QSynEdit::setModified(bool value, bool skipUndo)
{
    if (value) {
        mLastModifyTime = QDateTime::currentDateTime();
        setStatusChanged(StatusChange::Modified);
    }
    if (value != mModified) {
        mModified = value;

        if (!skipUndo) {
            if (value) {
                mUndoList->clear();
                mRedoList->clear();
            } else {
                mUndoList->setInitialState();
            }
        }
        setStatusChanged(StatusChange::ModifyChanged);
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
        CharPos oldBlockStart = selBegin();
        CharPos oldBlockEnd = selEnd();
        int xEnd = charToGlyphLeft(mCaretY,mCaretX);
        oldBlockStart.ch = xposToGlyphStartChar(oldBlockStart.line,xEnd);
        oldBlockEnd.ch = xposToGlyphStartChar(oldBlockEnd.line,xEnd);
        setSelBegin(oldBlockStart);
        setSelEnd(oldBlockEnd);
    } else {
        vOldMode = mActiveSelectionMode;
        mActiveSelectionMode = vOldMode;
    }
    if (mGutter.showLineNumbers() && (mGutter.autoSize()))
        mGutter.autoSizeDigitCount(mDocument->count());
    setTopPos(mTopPos);
    endInternalChanges();
}

void QSynEdit::onLinesChanging()
{
    beginInternalChanges();
}

void QSynEdit::onLinesDeleted(int line, int count)
{
    if (count<=0)
        return;
    if (mSyntaxer->needsLineState()) {
        reparseLines(line, line + count, true);
    }
    updateVScrollbar();
}

void QSynEdit::onLinesInserted(int line, int count)
{
    if (count<=0)
        return;
    processFoldsOnLinesInserted(line, count);
    if (mSyntaxer->needsLineState()) {
        reparseLines(line, line + count, false);
    } else {
        // new lines should be parsed
        reparseLines(line, line + count, true);
    }
    updateVScrollbar();
}

void QSynEdit::onUndoAdded()
{
    updateModifiedStatusForUndoRedo();

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

const CharPos &QSynEdit::selEnd() const
{
    if (mActiveSelectionMode==SelectionMode::Column)
        return mSelectionEnd;
    if (mSelectionBegin > mSelectionEnd)
        return mSelectionBegin;
    else
        return mSelectionEnd;
}

int QSynEdit::selectionBeginLine() const
{
    if (mActiveSelectionMode == SelectionMode::Column) {
        return (mSelectionBegin.line < mSelectionEnd.line)? mSelectionBegin.line : mSelectionEnd.line;
    } else {
        return (mSelectionBegin.line < mSelectionEnd.line)? mSelectionBegin.line : mSelectionEnd.line;
    }
}

int QSynEdit::selectionEndLine() const
{
    if (mActiveSelectionMode == SelectionMode::Column) {
        return (mSelectionBegin.line < mSelectionEnd.line)? mSelectionEnd.line : mSelectionBegin.line;
    } else {
        if (mSelectionBegin.line < mSelectionEnd.line) {
            return (mSelectionEnd.ch==1)?mSelectionEnd.line-1 : mSelectionEnd.line;
        } else if (mSelectionBegin.line == mSelectionEnd.line){
            return mSelectionBegin.line;
        } else {
            return (mSelectionBegin.ch==1)?mSelectionBegin.line-1 : mSelectionBegin.line;
        }
    }
}

void QSynEdit::clearSelection()
{
    setActiveSelectionMode(SelectionMode::Normal);
    setSelBegin(caretXY());
}

void QSynEdit::setSelEnd(const CharPos &value)
{
    Q_ASSERT(validInDoc(value));
    beginInternalChanges();
    auto action = finally([this](){
        endInternalChanges();
    });
    if (value.ch != mSelectionEnd.ch || value.line != mSelectionEnd.line) {
        if (mActiveSelectionMode == SelectionMode::Column && value.ch != mSelectionEnd.ch) {
            CharPos oldBlockEnd = mSelectionEnd;
            mSelectionEnd = value;
            invalidateLines(
                        std::min(mSelectionBegin.line, std::min(mSelectionEnd.line, oldBlockEnd.line)),
                        std::max(mSelectionBegin.line, std::max(mSelectionEnd.line, oldBlockEnd.line)));
        } else {
            CharPos oldBlockEnd = mSelectionEnd;
            mSelectionEnd = value;
            if (mActiveSelectionMode != SelectionMode::Column || mSelectionBegin.ch != mSelectionEnd.ch) {
                invalidateLines(
                            std::min(mSelectionEnd.line, oldBlockEnd.line),
                            std::max(mSelectionEnd.line, oldBlockEnd.line));
            }
        }
        setStatusChanged(StatusChange::Selection);
    }
}

void QSynEdit::setSelBeginEnd(const CharPos &beginPos, const CharPos &endPos)
{
    beginInternalChanges();
    setSelBegin(beginPos);
    setSelEnd(endPos);
    endInternalChanges();
}

void QSynEdit::setSelLength(int len)
{
    if (mSelectionBegin.line>mDocument->count() || mSelectionBegin.line<=0)
        return;

    if (len >= 0) {
        int y = mSelectionBegin.line;
        int ch = mSelectionBegin.ch;
        int x = ch + len;
        QString lineText;
        while (y<=mDocument->count()) {
            lineText = mDocument->getLine(y);
            if (x <= lineText.length()) {
                break;
            }
            x -= lineText.length()+1;
            y++;
        }
        if (y>=mDocument->count()) {
            y = mDocument->count()-1;
            x = mDocument->getLine(y).length();
        }
        CharPos iNewEnd{x,y};
        setCaretAndSelection(iNewEnd, mSelectionBegin, iNewEnd);
    } else {
        int y = mSelectionBegin.line;
        int ch = mSelectionBegin.ch;
        int x = len;
        QString line = mDocument->getLine(y);
        while (true) {
            if (x<=ch)
                break;
            x-=(ch+1);
            y--;
            if (y<0)
                break;
            line = mDocument->getLine(y);
            ch =  line.length();
        }
        if (y<0) {
            y = 0;
            x = 0;
        } else {
            x = ch - x;
        }
        CharPos iNewStart{x,y};
        setCaretAndSelection(iNewStart, iNewStart, mSelectionBegin);
    }
}

void QSynEdit::setSelText(const QString &text)
{
    doSetSelText(text);
}

void QSynEdit::replaceLine(int line, const QString &lineText)
{
    beginEditing();
    CharPos pos{0,line};
    addChangeToUndo(ChangeReason::ReplaceLine,pos,pos,QStringList(mDocument->getLine(line)),SelectionMode::Normal);
    properSetLine(line, lineText,true);
    endEditing();
}

const CharPos &QSynEdit::selBegin() const
{
    if (mActiveSelectionMode==SelectionMode::Column)
        return mSelectionBegin;
    if (mSelectionEnd < mSelectionBegin)
        return mSelectionEnd;
    else
        return mSelectionBegin;
}

void QSynEdit::setSelBegin(const CharPos &value)
{
    Q_ASSERT(validInDoc(value));
    beginInternalChanges();
    auto action = finally([this](){
        endInternalChanges();
    });
    int nInval1, nInval2;
    if (selAvail()) {
        if (mSelectionBegin.line < mSelectionEnd.line) {
            nInval1 = std::min(value.line, mSelectionBegin.line);
            nInval2 = std::max(value.line, mSelectionEnd.line);
        } else {
            nInval1 = std::min(value.line, mSelectionEnd.line);
            nInval2 = std::max(value.line, mSelectionBegin.line);
        };
        mSelectionBegin = value;
        mSelectionEnd = value;
        invalidateLines(nInval1, nInval2+1);
        setStatusChanged(StatusChange::Selection);
    } else {
        mSelectionBegin = value;
        mSelectionEnd = value;
    }
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
    value = std::min(value,maxScrollHeight());
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
    beginInternalChanges();
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
    endInternalChanges();
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
