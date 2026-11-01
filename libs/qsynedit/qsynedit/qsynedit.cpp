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
    connect(mDocument.get(), &Document::maxLineWidthChanged,
            this, &QSynEdit::onMaxLineWidthChanged);

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
    mCaretCh = 0;
    mLastCaretX = 0;
    mCaretLine = 0;
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
    reparseDocument();
}

int QSynEdit::lineCount() const
{
    return mDocument->count();
}

int QSynEdit::displayLineCount() const
{
    // returns the count of display rows (rows are 0-based internally,
    // so the last row index is lineToRow(count-1))
    return lineToRow(mDocument->count()-1)+1;
}

void QSynEdit::setCaretPos(const CharPos &pos)
{
    CharPos safePos = ensureCharPosValid(pos);
    beginInternalChanges();
    internalSetSelBegin(safePos);
    internalSetCaretPos(safePos, true);
    endInternalChanges();
}

void QSynEdit::setCaretPosCentered(const CharPos &pos)
{
    CharPos safePos = ensureCharPosValid(pos);
    beginInternalChanges();
    internalSetSelBegin(safePos);
    internalSetCaretPos(safePos, false);
    ensureCaretVisible(true); // but here after block has been set
    endInternalChanges();
}

void QSynEdit::uncollapseAroundLine(int line)
{
    while (true) { // Open up the closed folds around the focused line until we can see the line we're looking for
      PCodeBlock fold = foldHidesLine(line);
      if (fold)
          uncollapse(fold);
      else
          break;
    }
}

PCodeBlock QSynEdit::foldHidesLine(int line)
{
    return foldAroundLineEx(line, true, false, true);
}

void QSynEdit::setInsertMode(bool value)
{
    if (mReadOnly)
        return;
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
        maxWidth += stringWidth(mSyntaxer->foldString(""),maxWidth);
    if (mOptions.testFlag(EditorOption::ScrollPastEol))
        return std::max(maxWidth-2*mCharWidth, 0);
    else
        return std::max(maxWidth-viewWidth()+mCharWidth, 0);
}

bool QSynEdit::getTokenAttr(const CharPos &pos, QString &token, PTokenAttribute &attr) const
{
    int tmpStart;
    PSyntaxState syntaxState;
    return getTokenAttr(pos, token, tmpStart, attr,syntaxState);
}

bool QSynEdit::getTokenAttr(
        const CharPos &pos, QString &token,
        PTokenAttribute &attr, PSyntaxState &syntaxState) const
{
    int start;
    return getTokenAttr(pos, token, start, attr, syntaxState);
}

bool QSynEdit::getTokenAttr(const CharPos &pos, QString &token, int &start, PTokenAttribute &attr) const
{
    PSyntaxState syntaxState;
    return getTokenAttr(pos, token, start, attr, syntaxState);
}

bool QSynEdit::getTokenAttr(const CharPos &pos, QString &token, int &start, PTokenAttribute &attr, PSyntaxState &syntaxState) const
{
    int chIdx, lineIdx, endPos;
    QString lineText;
    lineIdx = pos.line;
    if ((lineIdx >= 0) && (lineIdx < mDocument->count())) {
        lineText = mDocument->getLine(lineIdx);
        chIdx = pos.ch;
        if ((chIdx >= 0) && (chIdx < lineText.length())) {
            PSyntaxer syntaxer = mSyntaxer->createInstance();
            startParseLine(syntaxer.get(), lineIdx, lineText);
            while (!syntaxer->eol()) {
                start = syntaxer->getTokenPos();
                token = syntaxer->getToken();
                syntaxState = syntaxer->getState();
                endPos = start + token.length();
                if ((chIdx >= start) && (chIdx < endPos)) {
                    attr = syntaxer->getTokenAttribute();
                    return true;
                }
                syntaxer->next();
            }
        }
    }
    token = "";
    attr = PTokenAttribute();
    return false;
}

QSet<int> QSynEdit::getTokenBorders(int line) const
{
    Q_ASSERT((line >= 0) && (line < mDocument->count()));
    QSet<int> result;
    result.insert(0);
    result.insert(mDocument->getLine(line).length());
    PSyntaxer syntaxer = mSyntaxer->createInstance();
    startParseLine(syntaxer.get(), line);
    while (!syntaxer->eol()) {
        result.insert(syntaxer->getTokenPos());
        syntaxer->next();
    }
    return result;
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
    CharPos p=caretPos();
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
    if (mReadOnly || mDocument->empty())
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
                        CharPos{(int)oldLine.length(), line},
                        CharPos{(int)newLine.length(), line},
                        QStringList({deleted}),
                        SelectionMode::Normal
                        );
        }
    } 
}

CharPos QSynEdit::getMatchingBracket()
{
    return getMatchingBracket(caretPos());
}

CharPos QSynEdit::getMatchingBracket(const CharPos &pos)
{
    QChar brackets[] = {'(', ')', '[', ']', '{', '}', '<', '>'};
    int bracketCount = sizeof(brackets) / sizeof(QChar);

    Q_ASSERT(validInDoc(pos));
    if (!validInDoc(pos))
        return CharPos{-1,-1};

    PSyntaxer syntaxer = mSyntaxer->createInstance();
    // get char at caret
    QChar testCh = charAt(pos);
    // is it one of the recognized brackets?
    for (int i = 0; i<bracketCount; i++) {
        if (testCh == brackets[i]) {
            // this is the bracket, get the matching one and the direction
            QChar bracketOpen = brackets[i];
            QChar bracketClose = brackets[i ^ 1]; // 0 -> 1, 1 -> 0, ...
            // search for the matching bracket (that is until NumBrackets = 0)
            int bracketsLevel = 1;
            QString dummy;
            PTokenAttribute attr;
            CharPos p;
            if (i%2==1) {
                //search backward
                for (int line = pos.line;line >= 0;line--) {
                    startParseLine(syntaxer.get(), line);
                    QList<QChar> bracketsFound;
                    QList<int> bracketsPos;
                    while(!syntaxer->eol()) {
                        if (line == pos.line && syntaxer->getTokenPos() >= pos.ch) {
                            break;
                        }
                        TokenType tokenType = syntaxer->getTokenAttribute()->tokenType();
                        if ( tokenType != TokenType::String
                             && tokenType != TokenType::Character
                             && tokenType != TokenType::Comment ) {
                            if (syntaxer->getToken() == bracketOpen
                                    || syntaxer->getToken() == bracketClose) {
                               bracketsFound.append(syntaxer->getToken()[0]);
                               bracketsPos.append(syntaxer->getTokenPos());
                            }
                        }
                        syntaxer->next();
                    }
                    for (int j=bracketsFound.length()-1;j>=0;j--) {
                        if (bracketsFound[j] == bracketOpen)
                            bracketsLevel++;
                        else if (bracketsFound[j] == bracketClose) {
                            bracketsLevel--;
                            if (bracketsLevel == 0)
                                return CharPos{bracketsPos[j], line};
                        }
                    }
                }
            } else {
                // search until end of line
                if (pos.line == 0) {
                    syntaxer->resetState();
                } else {
                    syntaxer->setState(mDocument->getSyntaxState(pos.line-1));
                }
                for (int line=pos.line;line<mDocument->count();line++) {
                    syntaxer->setLine(line, mDocument->getLine(line), mDocument->getLineSeq(line));
                    while(!syntaxer->eol()) {
                        TokenType tokenType = syntaxer->getTokenAttribute()->tokenType();
                        if (line == pos.line && syntaxer->getTokenPos() <= pos.ch) {
                            goto MOVE_NEXT_2;
                        }
                        if ( tokenType != TokenType::String
                             && tokenType != TokenType::Character
                             && tokenType != TokenType::Comment ) {
                            if (syntaxer->getToken() == bracketOpen)
                                ++bracketsLevel;
                            else if (syntaxer->getToken() == bracketClose) {
                                --bracketsLevel;
                                if (bracketsLevel == 0)
                                    return CharPos{syntaxer->getTokenPos(), line};
                            }
                        }
MOVE_NEXT_2:
                        syntaxer->next();
                    }
                }
            }
            // don't test the other brackets, we're done
            break;
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

bool QSynEdit::validGlyphStart(const CharPos &pos) const
{
    return pos.ch == mDocument->charToGlyphStartChar(pos.line, pos.ch);

}


bool QSynEdit::getPositionOfMouse(CharPos &pos) const
{
    QPoint point = QCursor::pos();
    point = mapFromGlobal(point);
    return pointToCharLine(point,pos);
}

bool QSynEdit::getLineOfMouse(int &line) const
{
    QPoint point = QCursor::pos();
    point = mapFromGlobal(point);
    return pointToLine(point,line);
}

bool QSynEdit::pointToCharLine(const QPoint &point, CharPos &pos) const
{
    // Make sure it fits within the SynEdit bounds (and on the gutter)
    if ((point.x() < gutterWidth() + clientLeft())
            || (point.x()> clientWidth()+clientLeft())
            || (point.y() < clientTop())
            || (point.y() > clientTop()+clientHeight())) {
        return false;
    }

    pos = displayToBufferPos(pixelsToGlyphPos(point.x(),point.y()));
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
        int firstLineTop = std::max(firstRow*mTextHeight, mTopPos);
        int lastLineBottom = std::min((endRow+1)*mTextHeight, mTopPos+clientHeight());
        rcInval = {0, firstLineTop - mTopPos,
                   mGutterWidth, lastLineBottom - firstLineTop};
        invalidateRect(rcInval);
    }
}

/**
 * @brief Convert point on the edit (x,y) to (row,column)
 * @param x
 * @param y
 * @return
 */

DisplayCoord QSynEdit::pixelsToNearestGlyphPos(int x, int y) const
{
    int xpos = std::max(0, leftPos() + x - mGutterWidth - 2);
    int row = yposToRow(y);
    int line = rowToLine(row);
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

DisplayCoord QSynEdit::pixelsToGlyphPos(int x, int y) const
{
    int xpos = std::max(0, leftPos() + x - mGutterWidth - 2);
    int row = yposToRow(y);
    int line = rowToLine(row);
    if (xpos<0) {
        xpos=0;
    } else if (xpos>mDocument->lineWidth(line)) {
        xpos=mDocument->lineWidth(line)+1;
    } else {
        int glyphIndex = mDocument->xposToGlyphIndex(line, xpos);
        xpos = mDocument->glyphStartPostion(line, glyphIndex);
    }
    return DisplayCoord{xpos, row};
}

QPoint QSynEdit::displayCoordToPixels(const DisplayCoord &coord) const
{
    QPoint result;
    result.setX(coord.x + textOffset());
    result.setY(coord.row * mTextHeight - mTopPos);
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
    if (p.row<0)
        return result;
    // Account for code folding
    result.line = rowToLine(p.row);
    // Account for tabs
    if (result.line < mDocument->count() ) {
        result.ch = xposToGlyphStartChar(result.line,p.x);
    }
    return result;
}

int QSynEdit::leftSpaces(const QString &lineText) const
{
    int result = 0;
    if (mOptions.testFlag(EditorOption::AutoIndent)) {
        for (QChar ch:lineText) {
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
    QString s = getDisplayStringAtLine(line);
    Q_ASSERT(ch>=0 && ch<=s.length());
    return mDocument->charToGlyphStartPosition(line, s, ch);
}

int QSynEdit::charToGlyphLeft(int line, const QString &s, int ch) const
{
    return mDocument->charToGlyphStartPosition(line, s, ch);
}

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

// {
//     Q_ASSERT(line>=1 && line <= mDocument->count());
//     int glyphIndex = mDocument->xposToGlyphIndex(line-1,xpos);
//     return mDocument->glyphStartPostion(line-1,glyphIndex) + mDocument->glyphLength(line-1,glyphIndex)+1;
// }

int QSynEdit::stringWidth(const QString &lineText, int left) const
{
    return mDocument->stringWidth(lineText, left);
}

int QSynEdit::getLineIndent(const QString &lineText) const
{
    int indents = 0;
    for (QChar ch:lineText) {
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

int QSynEdit::rowToLine(int row) const
{
    int line;
    if (useCodeFolding())
        line = foldRowToLine(row);
    else
        line = row;
    if (line<0)
        line = 0;
    if (line>=mDocument->count())
        line = mDocument->count()-1;
    return line;
}

int QSynEdit::lineToRow(int line) const
{
    if (useCodeFolding())
        return foldLineToRow(line);
    else
        return line;
}

int QSynEdit::foldRowToLine(int row) const
{
    int resultLine = row;
    for (int i=0;i<mCodeBlocks.count();i++) {
        PCodeBlock range = mCodeBlocks[i];
        if (range->collapsed && !range->parentCollapsed() && range->fromLine < resultLine) {
            resultLine += range->linesCollapsed();
        }
    }
    return resultLine;
}

int QSynEdit::foldLineToRow(int line) const
{
    int resultRow = line;
    for (int i=mCodeBlocks.count()-1;i>=0;i--) {
        PCodeBlock range =mCodeBlocks[i];
        if (range->collapsed && !range->parentCollapsed()) {
            // Line is found after fold
            if (range->toLine < line)
                resultRow -= range->linesCollapsed();
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
                    mTextHeight * row - mTopPos,
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

        int firstLineTop = std::max(firstRow*mTextHeight, mTopPos);
        int lastLineBottom = std::min((lastRow+1)*mTextHeight, mTopPos+clientHeight());

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
    // a selection exists iff its anchor differs from its active end,
    // even if its content is empty (e.g. a zero-width column selection)
    return mSelectionBegin != mSelectionEnd;
}

bool QSynEdit::hasSelContent() const
{
    if (!selAvail())
        return false;
    if (mActiveSelectionMode != SelectionMode::Column)
        return true;
    if (mSelectionBegin.line == mSelectionEnd.line)
        return true; // same line and begin != end, so chars differ
    DisplayCoord coordBegin = bufferToDisplayPos(mSelectionBegin);
    DisplayCoord coordEnd = bufferToDisplayPos(mSelectionEnd);
    return coordBegin.x != coordEnd.x;
}

QString QSynEdit::wordAtCursor() const
{
    return tokenAt(caretPos());
}

QString QSynEdit::tokenAt(const CharPos &pos) const
{
    Q_ASSERT(validInDoc(pos));
    int start;
    QString token;
    PTokenAttribute attr;
    if (getTokenAttr(pos, token, start, attr)) {
        return token;
    }
    return QString();
}

QChar QSynEdit::charAt(const CharPos &pos) const
{
    if ((pos.line >= 0) && (pos.line < mDocument->count())) {
        QString lineText = mDocument->getLine(pos.line);
        int len = lineText.length();
        if (len == 0)
            return QChar(0);
        if (pos.ch<0 || pos.ch>=len)
            return QChar(0);
        return lineText[pos.ch];
    }
    return QChar(0);
}

void QSynEdit::setCaretAndSelection(const CharPos &posCaret, const CharPos &posSelBegin, const CharPos &posSelEnd)
{
    beginInternalChanges();
    internalSetCaretPos(ensureCharPosValid(posCaret));
    setActiveSelectionMode(SelectionMode::Normal);
    internalSetSelBegin(ensureCharPosValid(posSelBegin));
    internalSetSelEnd(ensureCharPosValid(posSelEnd));
    endInternalChanges();
}

void QSynEdit::loadFromFile(const QString& filename, const QByteArray& encoding, QByteArray& realEncoding)
{
    int oldCount = mDocument->count();
    beginEditing();
    internalClearAll();
    mDocument->loadFromFile(filename, encoding, realEncoding);
    reparseDocument();
    emit linesDeleted(0, oldCount);
    emit linesInserted(0, mDocument->count());
    updateVScrollbar();
    endEditing();
}

void QSynEdit::setContent(const QString &text)
{
    int oldCount = mDocument->count();
    beginEditing();
    internalClearAll();
    mDocument->setText(text);
    reparseDocument();
    emit linesDeleted(0, oldCount);
    emit linesInserted(0, mDocument->count());
    endEditing();
}

void QSynEdit::setContent(const QStringList &text)
{
    int oldCount = mDocument->count();
    beginEditing();
    internalClearAll();
    mDocument->setContents(text);
    reparseDocument();
    emit linesDeleted(0, oldCount);
    emit linesInserted(0, mDocument->count());
    endEditing();
}

void QSynEdit::collapseAll()
{
    beginInternalChanges();
    foreach(const PCodeBlock &block, mCodeBlocks){
        block->collapsed=true;
    }
    updateHScrollbar();
    updateVScrollbar();
    ensureCaretVisible();
    invalidate();
    endInternalChanges();
}

void QSynEdit::unCollpaseAll()
{
    beginInternalChanges();
    foreach(const PCodeBlock &block, mCodeBlocks){
        block->collapsed=false;
    }
    updateHScrollbar();
    updateVScrollbar();
    ensureCaretVisible();
    invalidate();
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
        PCodeBlock foldRange = foldStartAtLine(line);
        if (foldRange) {
            // See if we actually clicked on the rectangle...
            QRect rect;
            rect.setLeft(mGutterWidth - mGutter.rightOffset());
            rect.setRight(rect.left() + mGutter.rightOffset() - 4);
            rect.setTop(row * mTextHeight - mTopPos);
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
    CharPos blockBegin = selBegin();
    CharPos blockEnd = selEnd();
    if ((pos.line >= blockBegin.line) && (pos.line <= blockEnd.line) &&
            ((blockBegin.line != blockEnd.line) || (blockBegin.ch != blockEnd.ch))) {
        if (mActiveSelectionMode == SelectionMode::Column) {
            if (blockBegin.ch > blockEnd.ch)
                return (pos.ch >= blockEnd.ch) && (pos.ch < blockBegin.ch);
            else if (blockBegin.ch < blockEnd.ch)
                return (pos.ch >= blockBegin.ch) && (pos.ch < blockEnd.ch);
            else
                return false;
        } else
            return ((pos.line > blockBegin.line) || (pos.ch >= blockBegin.ch)) &&
      ((pos.line < blockEnd.line) || (pos.ch < blockEnd.ch));
    } else
        return false;
}

CharPos QSynEdit::findNextChar(const CharPos &pos, CharType type) const
{
    Q_ASSERT(validInDoc(pos));
    int ch = pos.ch;
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
    if (getTokenAttr(pos, token, start, attr)) {
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
    if (getTokenAttr(pos, token, start, attr)) {
        if (!token.isEmpty())
            return CharPos(start+token.length(), pos.line);
    }
    return pos;
}

CharPos QSynEdit::prevWordBegin(CharPos pos) const
{
    Q_ASSERT(validInDoc(pos));
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
    CharPos p;
    if (pos.ch == 0) {
        p = prevNonSpaceChar(pos);
    } else if (pos.ch >= mDocument->getLine(pos.line).length()) {
        p = prevSpaceChar(CharPos{(int)mDocument->getLine(pos.line).length()-1,pos.line});
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

CharPos QSynEdit::lineBegin(int line) const
{
    Q_ASSERT(line>=0 && line<mDocument->count());
    return CharPos{0, line};
}

CharPos QSynEdit::lineEnd(int line) const
{
    Q_ASSERT(line>=0 && line<mDocument->count());
    return CharPos{(int)mDocument->getLine(line).length(), line};
}

bool QSynEdit::getContainingBlockStart(const CharPos &pos, CharPos &blockStart) const
{
    if (!useCodeFolding())
        return false;

    if (!validInDoc(pos))
        return false;

    if (mSyntaxer->language()!=ProgrammingLanguage::CPP)
        return false;

    PSyntaxer syntaxer = mSyntaxer->createInstance();

    // find begin brace
    blockStart = fileBegin();
    int bracketsLevel = 1;
    for (int line = pos.line;line >= 0;line--) {
        startParseLine(syntaxer.get(), line);
        QList<QChar> bracketsFound;
        QList<int> bracketsPos;
        while(!syntaxer->eol()) {
            if (line == pos.line && syntaxer->getTokenPos() >= pos.ch) {
                break;
            }
            TokenType tokenType = syntaxer->getTokenAttribute()->tokenType();
            if ( tokenType != TokenType::String
                 && tokenType != TokenType::Character
                 && tokenType != TokenType::Comment) {
                if (syntaxer->getToken() == '{' || syntaxer->getToken() == '}') {
                   bracketsFound.append(syntaxer->getToken()[0]);
                   bracketsPos.append(syntaxer->getTokenPos());
                }
            }
            syntaxer->next();
        }
        for (int i=bracketsFound.length()-1;i>=0;i--) {
            if (bracketsFound[i] == '}')
                bracketsLevel++;
            else if (bracketsFound[i] == '{') {
                bracketsLevel--;
                if (bracketsLevel == 0) {
                    blockStart = CharPos{bracketsPos[i], line};
                    return true;
                }
            }
        }
    }
    return false;
}

bool QSynEdit::getContainingBlockEnd(const CharPos &pos, CharPos &blockEnd) const
{
    if (!useCodeFolding())
        return false;

    if (!validInDoc(pos))
        return false;

    if (mSyntaxer->language()!=ProgrammingLanguage::CPP)
        return false;

    PSyntaxer syntaxer = mSyntaxer->createInstance();
    // find end brace;
    blockEnd = fileEnd();
    int bracketsLevel = 1;
    if (pos.line == 0) {
        syntaxer->resetState();
    } else {
        syntaxer->setState(mDocument->getSyntaxState(pos.line-1));
    }
    for (int line=pos.line;line<mDocument->count();line++) {
        syntaxer->setLine(line, mDocument->getLine(line), mDocument->getLineSeq(line));
        while(!syntaxer->eol()) {
            TokenType tokenType = syntaxer->getTokenAttribute()->tokenType();
            if (line == pos.line && syntaxer->getTokenPos() <= pos.ch) {
                goto MOVE_NEXT_2;
            }
            if ( tokenType != TokenType::String
                 && tokenType != TokenType::Character
                 && tokenType != TokenType::Comment ) {
                if (syntaxer->getToken() == '{')
                    ++bracketsLevel;
                else if (syntaxer->getToken() == '}') {
                    --bracketsLevel;
                    if (bracketsLevel == 0) {
                        blockEnd = CharPos{syntaxer->getTokenPos(), line};
                        return true;
                    }
                }
            }
MOVE_NEXT_2:
            syntaxer->next();
        }
    }
    return false;
}

CharPos QSynEdit::fileEnd() const
{
    int line = mDocument->count()-1;
    return CharPos{(int)mDocument->getLine(line).length(),line};
}

void QSynEdit::selCurrentToken()
{
    selTokenAt(caretPos());
}

void QSynEdit::selTokenAt(const CharPos &pos)
{
    Q_ASSERT(validInDoc(pos));
    beginInternalChanges();
    int start;
    QString token;
    PTokenAttribute attr;
    if (getTokenAttr(pos, token, start, attr)) {
        if (!token.isEmpty()) {
            CharPos wordStart{start, pos.line};
            CharPos wordEnd{start+(int)token.length(), pos.line};
            setCaretAndSelection(wordEnd, wordStart, wordEnd);
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

bool QSynEdit::shouldInsertAfterCurrentLine(int line, const QString &newLineText, const QString &newLineText2, bool undoingItem) const
{
    if (undoingItem) {
        if (line+1>=mDocument->count()-1) //undoing
            return false;
    } else {
        if (line+1>=mDocument->count())
            return false;
    }
    if (newLineText2.trimmed().isEmpty())
        return true;
    PSyntaxState oldState = mDocument->getSyntaxState(line);
    PSyntaxState newState = calcSyntaxStateAtLine(line, newLineText);
    if (newState->blockStarted == 0 && newState->blockEnded==0)
        return false;
    return true;
}

bool QSynEdit::shouldDeleteNextLine(int line, const QString &currentLineText, const QString &nextLineText) const
{
    Q_UNUSED(currentLineText);
    Q_ASSERT(line+1<mDocument->count());
    if (nextLineText.trimmed().isEmpty())
        return true;
    PSyntaxState prevLineState = mDocument->getSyntaxState(line);
    if (prevLineState->blockStarted == 0 && prevLineState->blockEnded==0)
        return false;
    return true;
}

void QSynEdit::calcEffectiveFromToLine(const CharPos &startPos, const CharPos &endPos, int &fromLine, int &toLine)
{
    Q_ASSERT(validInDoc(startPos));
    Q_ASSERT(validInDoc(endPos));
    Q_ASSERT(startPos <= endPos);
    fromLine = startPos.line;
    toLine = endPos.line;
    if (startPos.line == endPos.line) {
        return;
    }
    if (endPos.ch == 0) {
        toLine--;
    }
    if (startPos.ch >= mDocument->getLine(startPos.line).length()) {
        fromLine++;
    }
    if (fromLine>toLine) {
        fromLine = toLine;
    }
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

bool QSynEdit::shouldRecalcIndent(int line)
{
    if (!mOptions.testFlag(EditorOption::AutoIndent))
        return false;
    if (mFormatter) {
        return mFormatter->shouldRecalcIndent(line,this);
    }
    return true;
}

void QSynEdit::doSelectAll()
{
    beginInternalChanges();
    auto action = finally([this](){
        endInternalChanges();
    });
    setCaretAndSelection(caretPos(), fileBegin(), fileEnd());
}

void QSynEdit::doComment()
{

    if (mReadOnly || mDocument->empty())
        return;
    if (mSyntaxer->lineCommentSymbol().isEmpty())
        return;
    beginEditing();
    CharPos origBlockBegin, origBlockEnd, origCaret;
    int fromLine, toLine;
    origBlockBegin = selBegin();
    origBlockEnd = selEnd();
    origCaret = caretPos();
    calcEffectiveFromToLine(origBlockBegin,origBlockEnd, fromLine, toLine);
    QString lineCommentSymbol = mSyntaxer->lineCommentSymbol();
    int symbolLen = lineCommentSymbol.length();
    for (int i = fromLine; i<=toLine; i++) {
        properSetLine(i, lineCommentSymbol + mDocument->getLine(i), i==toLine);
        addChangeToUndo(ChangeReason::Insert,
              CharPos{0, i},
              CharPos{symbolLen, i},
              QStringList(), SelectionMode::Normal);
        // Move begin of selection
        if (i == origBlockBegin.line)
            origBlockBegin.ch+=symbolLen;
        // Move end of selection
        if (i == origBlockEnd.line)
            origBlockEnd.ch+=symbolLen;
        // Move caret
        if (i == origCaret.line)
            origCaret.ch+=symbolLen;
    }
    setCaretAndSelection(origCaret, origBlockBegin, origBlockEnd);
    endEditing();
}

void QSynEdit::doUncomment()
{
    if (mReadOnly || mDocument->empty())
        return;
    if (mSyntaxer->lineCommentSymbol().isEmpty())
        return;
    beginEditing();
    QString lineCommentSymbol=mSyntaxer->lineCommentSymbol();
    int symbolLen = lineCommentSymbol.length();
    QStringList changeText({lineCommentSymbol});
    CharPos origBlockBegin, origBlockEnd, origCaret;
    int fromLine,toLine;
    origBlockBegin = selBegin();
    origBlockEnd = selEnd();
    origCaret = caretPos();
    calcEffectiveFromToLine(origBlockBegin,origBlockEnd, fromLine, toLine);
    for (int i = fromLine; i<= toLine; i++) {
        QString s = mDocument->getLine(i);
        if (!s.trimmed().startsWith(lineCommentSymbol))
            continue;
        // Find "//" after blanks only
        int j = 0;
        while ((j+1 < s.length()) && (s[j] == ' ' || s[j] == '\t'))
            j++;
        s.remove(j,symbolLen);
        properSetLine(i,s, i==toLine);
        addChangeToUndo(ChangeReason::Delete,
                             CharPos{j, i},
                             CharPos{j+symbolLen, i},
                             changeText, SelectionMode::Normal);
        // Move begin of selection
        if (i == origBlockBegin.line) {
            origBlockBegin.ch = std::max(0, origBlockBegin.ch-symbolLen);
        }
        // Move end of selection
        if (i == origBlockEnd.line) {
            origBlockEnd.ch = std::max(0, origBlockEnd.ch-symbolLen);
        }
        // Move caret
        if (i == origCaret.line) {
            origCaret.ch = std::max(0, origCaret.ch-symbolLen);
        }
    }
    setCaretAndSelection(origCaret,origBlockBegin,origBlockEnd);
    endEditing();
}

void QSynEdit::doToggleComment()
{
    CharPos origBlockBegin, origBlockEnd, origCaret;
    int fromLine, toLine;
    QString s;
    bool allCommented = true;
    if (mReadOnly || mDocument->empty())
        return;
    if (mSyntaxer->lineCommentSymbol().isEmpty())
        return;
    QString commentSymbol=mSyntaxer->lineCommentSymbol();

    origBlockBegin = selBegin();
    origBlockEnd = selEnd();
    origCaret = caretPos();
    // Ignore the last line the cursor is placed on
    calcEffectiveFromToLine(origBlockBegin, origBlockEnd, fromLine, toLine);
    for (int i = fromLine; i<= toLine; i++) {
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
    if (mReadOnly || mDocument->empty())
        return;
    if (mSyntaxer->blockCommentBeginSymbol().isEmpty())
        return;

    beginEditing();
    EditorOptions oldOptions = mOptions;
    mOptions.setFlag(EditorOption::AutoIndent, false); //disable auto indent

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

    mOptions = oldOptions;
    endEditing();
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
    QPoint mousePos;
    DisplayCoord coord;

    mousePos = QCursor::pos();
    mousePos = mapFromGlobal(mousePos);
    coord = pixelsToNearestGlyphPos(mousePos.x(), mousePos.y());
    coord.row = minMax(coord.row, 0, displayLineCount()-1);
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
        coord.row = minMax(y, 0, displayLineCount()-1);
    }
    CharPos caret = displayToBufferPos(coord);
    if ((mCaretCh != caret.ch) || (mCaretLine != caret.line)) {
        // changes to line / column in one go
        beginInternalChanges();
        auto action = finally([this]{
            endInternalChanges();
        });
        internalSetCaretPos(caret, false);

        // if MouseCapture is True we're changing selection. otherwise we're dragging
        if (isDragging) {
            internalSetSelBegin(mDragSelBeginSave);
            internalSetSelEnd(mDragSelEndSave);
        } else
            internalSetSelEnd(caretPos());

    }
    if (isDragging) {
        mScrollTimer->singleShot(20,this,&QSynEdit::onDraggingScrollTimeout);
    } else  {
        mScrollTimer->singleShot(20,this,&QSynEdit::onScrollTimeout);
    }
}

void QSynEdit::doUndoRedoInput(const QString &oldGlyph,
                             const CharPos &changeStartPos,
                             const CharPos &changeEndPos,
                             SelectionMode changeSelMode,
                             size_t changeNumber,
                             bool undo)
{
    int changeLine = changeStartPos.line;
    QString lineText = mDocument->getLine(changeLine);
    QString replacedStr = lineText.mid(
                changeStartPos.ch,
                changeEndPos.ch-changeStartPos.ch);
    QString leftStr = lineText.left(changeStartPos.ch);
    QString rightStr = lineText.mid(changeEndPos.ch);
    QString restoredStr = leftStr+oldGlyph+rightStr;
    properSetLine(changeLine, restoredStr, true);
    if (undo) {
        mRedoList->addRedo(
                    ChangeReason::Input,
                    changeStartPos,
                    CharPos{changeStartPos.ch+(int)oldGlyph.length(),changeLine},
                    QStringList{replacedStr},
                    changeSelMode,
                    changeNumber);
    } else { //redo
        mUndoList->restoreChange(
                    ChangeReason::Input,
                    changeStartPos,
                    CharPos{changeStartPos.ch+(int)oldGlyph.length(),changeLine},
                    QStringList{replacedStr},
                    changeSelMode,
                    changeNumber);
    }
    internalSetCaretPos(changeStartPos);
}

void QSynEdit::beginEditing()
{
    beginInternalChanges();
    if (mEditingCount==0) {
        if (!mUndoing) {
            mUndoList->beginBlock(caretPos(), mSelectionBegin, mSelectionEnd, mActiveSelectionMode);
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
            mUndoList->endBlock(caretPos(), mSelectionBegin, mSelectionEnd, mActiveSelectionMode);
        rescanCodeBlocks();
    }
    endInternalChanges();
}

QString QSynEdit::getDisplayStringAtLine(int line) const
{
    QString s = mDocument->getLine(line);
    PCodeBlock foldRange = foldStartAtLine(line);
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
    if (mReadOnly || mDocument->empty())
        return ;

    if (mActiveSelectionMode==SelectionMode::Column) {
        CharPos start=selBegin();
        CharPos end=selEnd();
        if (!hasSelContent()) {
            start.ch--;
            internalSetSelBegin(start);
            internalSetSelEnd(end);
        }
        doDeleteSelection();
        return;
    }
    if (selAvail()) {
        doDeleteSelection();
        return;
    }
    QString tempStr = lineText();
    int tempStrLen = tempStr.length();
    CharPos caretBackup{caretPos()};
    QStringList helper;
    if (mCaretCh > tempStrLen ) {
        // only move caret one column
        return;
    } else if (mCaretCh == 0) {
        // join this line with the last line if possible
        doMergeWithPrevLine();
    } else {
        // delete char
        QString lastDelText;
        bool shouldAddGroupBreak = false;
        if (!mUndoing && mOptions.testFlag(EditorOption::GroupUndo)) {
            PUndoItem undoItem = mUndoList->peekItem();
            if (undoItem &&
                    (undoItem->changeReason() ==ChangeReason::DeletePreviousChar
                    && undoItem->changeStartPos().line == mCaretLine
                    && undoItem->changeStartPos().ch == mCaretCh
                    ))
                lastDelText = undoItem->changeText()[0];
            else
                shouldAddGroupBreak = true;
        }

        beginEditing();
        int glyphIndex = mDocument->charToGlyphIndex(mCaretLine,mCaretCh);
        Q_ASSERT(glyphIndex>0);
        int oldCaretX = mCaretCh;
        int newCaretX = mDocument->glyphStartChar(mCaretLine, glyphIndex-1);
        //qDebug()<<"delete last char:"<<oldCaretX<<newCaretX<<glyphIndex<<mCaretLine;
        QString deletedGlyph = tempStr.mid(newCaretX, oldCaretX-newCaretX);
        setCaretCh(newCaretX);
        if (!shouldAddGroupBreak) {
            if (deletedGlyph.length() == 1) {
                if (lastDelText.length()==1) {
                    QChar lastDelCh = lastDelText[0];
                    if (isSpaceChar(deletedGlyph[0]))
                        shouldAddGroupBreak = !isSpaceChar(lastDelCh);
                    else if (isIdentChar(deletedGlyph[0]))
                        shouldAddGroupBreak = !isIdentChar(lastDelCh);
                    else
                        shouldAddGroupBreak = isIdentChar(lastDelCh) || isSpaceChar(lastDelCh);
                } else
                    shouldAddGroupBreak = true;
            } else {
                if (lastDelText.length()==1)
                    shouldAddGroupBreak = true;
            }
        }

        helper.append(deletedGlyph);
        tempStr.remove(newCaretX, oldCaretX-newCaretX);
        properSetLine(mCaretLine, tempStr, true);
        if (shouldAddGroupBreak)
            addGroupUndoBreak();
        addChangeToUndo(ChangeReason::DeletePreviousChar, caretPos(), caretBackup, helper,
                        mActiveSelectionMode);
        endEditing();
    }

}

void QSynEdit::doDeleteCurrentChar()
{
    QStringList helper;
    CharPos newCaret{};
    if (mReadOnly || mDocument->empty())
        return;

    if (mActiveSelectionMode==SelectionMode::Column) {
        CharPos start=selBegin();
        CharPos end=selEnd();
        if (!hasSelContent()) {
            end.ch++;
            internalSetSelBegin(start);
            internalSetSelEnd(end);
        }
        doDeleteSelection();
        return;
    }
    if (selAvail())
        doDeleteSelection();
    else {
        // Call UpdateLastCaretX. Even though the caret doesn't move, the
        // current caret position should "stick" whenever text is modified.
        updateLastCaretX();
        QString tempStr = lineText();
        int tempStrLen = tempStr.length();
        if (mCaretCh>tempStrLen) {
            return;
        } else if (mCaretCh < tempStrLen) {
            QString lastDelText;
            bool shouldAddGroupBreak = false;
            if (!mUndoing && mOptions.testFlag(EditorOption::GroupUndo)) {
                PUndoItem undoItem = mUndoList->peekItem();
                if (undoItem &&
                        (undoItem->changeReason() ==ChangeReason::DeleteChar
                        && undoItem->changeStartPos().line == mCaretLine
                        && undoItem->changeStartPos().ch == mCaretCh
                        )) {
                    lastDelText = undoItem->changeText()[0];
                } else
                    shouldAddGroupBreak = true;
            }

            beginEditing();
            QString deletedGlyph = mDocument->glyphAt(mCaretLine, mCaretCh);
            if (!shouldAddGroupBreak) {
                if (deletedGlyph.length() == 1) {
                    if (lastDelText.length()==1) {
                        QChar lastDelCh = lastDelText[0];
                        if (isSpaceChar(deletedGlyph[0]))
                            shouldAddGroupBreak = !isSpaceChar(lastDelCh);
                        else if (isIdentChar(deletedGlyph[0]))
                            shouldAddGroupBreak = !isIdentChar(lastDelCh);
                        else
                            shouldAddGroupBreak = isIdentChar(lastDelCh) || isSpaceChar(lastDelCh);
                    } else
                        shouldAddGroupBreak = true;
                } else {
                    if (lastDelText.length()==1)
                        shouldAddGroupBreak = true;
                }
            }

            // delete char
            helper.append(deletedGlyph);
            newCaret.ch = mCaretCh + deletedGlyph.length();
            newCaret.line = mCaretLine;
            tempStr.remove(mCaretCh, deletedGlyph.length());
            properSetLine(mCaretLine, tempStr, true);
            if (!mUndoing) {
                if (shouldAddGroupBreak)
                    addGroupUndoBreak();
                addChangeToUndo(ChangeReason::DeleteChar, caretPos(), newCaret,
                      helper, mActiveSelectionMode);
            }
            endEditing();
        } else {
            // join line with the line after
            doMergeWithNextLine();
        }
    }
}

void QSynEdit::doMergeWithNextLine()
{
    if (mActiveSelectionMode != SelectionMode::Normal)
        return;
    if (mCaretLine+1 < mDocument->count()) {
        CharPos newCaret{0, mCaretLine+1};
        QString currentLineText = mDocument->getLine(mCaretLine);
        beginEditing();
        QString nextLineText = mDocument->getLine(mCaretLine+1);
        QString newString = currentLineText + nextLineText;
        if (shouldDeleteNextLine(mCaretLine, currentLineText, nextLineText)) {
            properDeleteLine(mCaretLine + 1, false);
        } else {
            properDeleteLine(mCaretLine, false);
        }
        properSetLine(mCaretLine, newString, true);
        addChangeToUndo(ChangeReason::MergeWithNextLine, caretPos(), newCaret,
                        QStringList{}, mActiveSelectionMode);
        endEditing();
    }
}

void QSynEdit::doMergeWithPrevLine()
{
    if (mActiveSelectionMode!=SelectionMode::Normal)
        return;
    if (mCaretLine > 0) {
        CharPos caretBackup{caretPos()};
        QString tempStr = lineText();
        beginEditing();
        QString lastLine = mDocument->getLine(mCaretLine-1);
        if (shouldDeleteNextLine(mCaretLine-1, lastLine, tempStr)) {
            properDeleteLine(mCaretLine, false);
        } else {
            properDeleteLine(mCaretLine-1, false);
        }
        properSetLine(mCaretLine-1, lastLine+tempStr, true);
        setCaretPos(CharPos{(int)lastLine.length(), mCaretLine - 1});
        addChangeToUndo(ChangeReason::MergeWithPrevLine, caretPos(), caretBackup, QStringList{},
                        mActiveSelectionMode);
        endEditing();
    }

}

void QSynEdit::doDeleteCurrentToken()
{
    if (mReadOnly || mDocument->empty())
        return;
    if (mCaretCh>lineText().length())
        return;
    CharPos pos{caretPos()};
    int start;
    QString token;
    PTokenAttribute attr;
    if (getTokenAttr(pos, token, start, attr)) {
        if (!token.isEmpty()) {
            CharPos wordStart{start, pos.line};
            CharPos wordEnd{start+(int)token.length(), pos.line};
            doDeleteText(wordStart, wordEnd, SelectionMode::Normal);
        }
    }
}

void QSynEdit::doDeleteToEOL()
{
    if (mReadOnly || mDocument->empty())
        return;
    if (mCaretCh>lineText().length())
        return;

    doDeleteText(caretPos(),CharPos{(int)lineText().length(),mCaretLine},SelectionMode::Normal);
}

void QSynEdit::doDeleteToWordStart()
{
    if (mReadOnly || mDocument->empty())
        return;
    if (mCaretCh>lineText().length()+1)
        return;

    CharPos end = caretPos();
    CharPos start = getTokenBegin(end);
    if (start==end)
        start = prevWordEnd(end);
    if (start.isValid() && end.isValid())
    doDeleteText(start,end,SelectionMode::Normal);
}

void QSynEdit::doDeleteToWordEnd()
{
    if (mReadOnly || mDocument->empty())
        return;
    if (mCaretCh>lineText().length()+1)
        return;

    CharPos start = caretPos();
    CharPos end = getTokenEnd(start);
    if (start.isValid() && end.isValid())
        doDeleteText(start,end,SelectionMode::Normal);
}

void QSynEdit::doDeleteCurrentTokenAndTralingSpaces()
{
    if (mReadOnly || mDocument->empty())
        return;
    if (mCaretCh>lineText().length()+1)
        return;

    CharPos start = getTokenBegin(caretPos());
    CharPos end = nextWordBegin(caretPos());
    if (start.isValid() && end.isValid())
        doDeleteText(start,end,SelectionMode::Normal);
}

void QSynEdit::doDeleteFromBOL()
{
    if (mReadOnly || mDocument->empty())
        return;
    doDeleteText(CharPos{0,mCaretLine},caretPos(),SelectionMode::Normal);
}

void QSynEdit::doDeleteCurrentLine()
{
    if (mReadOnly || mDocument->empty())
        return;
//    PCodeFoldingRange foldRange=foldStartAtLine(mCaretLine);
//    if (foldRange && foldRange->collapsed)
//        return;
    beginEditing();
    if (selAvail())
        internalSetSelBegin(caretPos());
    int oldCaretY = mCaretLine;
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
        setCaretPos(fileEnd());
    } else
        setCaretPos(lineBegin(oldCaretY));
}

void QSynEdit::doDuplicate()
{
    if (mReadOnly)
        return;
    if (hasSelContent()) {
        doDuplicateSelection();
    } else {
        doDuplicateCurrentLine();
    }
}

void QSynEdit::doDuplicateSelection()
{
    if (mReadOnly)
        return;
    if (!hasSelContent())
        return;
    //CharPos beginPos = selBegin();
    CharPos endPos = selEnd();
    beginEditing();
    EditorOptions oldOptions = mOptions;
    mOptions.setFlag(EditorOption::AutoIndent, false); //disable auto indent
    doInsertTextByNormalMode(endPos,selContent());
    mOptions = oldOptions;
    CharPos newSelBegin{endPos};
    CharPos newSelEnd{caretPos()};
    setCaretAndSelection(newSelEnd, newSelBegin, newSelEnd);
    endEditing();
}

void QSynEdit::doSelectLine()
{
    CharPos blockBegin{0,mCaretLine};
    CharPos blockEnd;
    if (mCaretLine==mDocument->count()-1)
        blockEnd = CharPos{(int)lineText().length(),mCaretLine};
    else
        blockEnd = CharPos{0,mCaretLine+1};
    setCaretAndSelection(blockBegin,blockBegin,blockEnd);
}

CharPos QSynEdit::ensureCharPosValid(const CharPos &pos) const
{
    int maxX;
    CharPos value = pos;
    if (value.line >= mDocument->count())
        return fileEnd();
    if (value.line < 0)
        return fileBegin();
    if (mActiveSelectionMode!=SelectionMode::Column) {
        maxX = mDocument->getLine(value.line).length();
        value.ch = std::min(value.ch, maxX);
    }
    value.ch = std::max(value.ch,0);
    value.ch = mDocument->charToGlyphStartChar(value.line, value.ch);
    return value;
}

void QSynEdit::doDuplicateCurrentLine()
{
    if (!mReadOnly) {
        beginEditing();
        QString s = lineText();
        properInsertLine(mCaretLine+1, lineText(), true);
        addChangeToUndo(ChangeReason::DuplicateLine,
                        caretPos(),
                        CharPos{0,mCaretLine+1}, QStringList(), SelectionMode::Normal);
        setCaretLine(mCaretLine+1);
        endEditing();
    }
}

void QSynEdit::doMoveSelUp()
{
    if (mActiveSelectionMode == SelectionMode::Column)
        return;
    int fromLine,toLine;
    CharPos origBlockBegin = selBegin();
    CharPos origBlockEnd = selEnd();
    calcEffectiveFromToLine(origBlockBegin, origBlockEnd, fromLine, toLine);
    if (!mReadOnly && (!mDocument->empty()) && (fromLine > 0)) {
        CharPos oldCaret = caretPos();
        beginEditing();
        // Move line above selection to below selection
        properMoveLine(fromLine-1, toLine, true);

        // Restore caret and selection
        CharPos newCaret;
        if (oldCaret>= origBlockBegin && oldCaret <= origBlockEnd) {
            newCaret = ensureCharPosValid(CharPos{oldCaret.ch, oldCaret.line-1});
        } else if (oldCaret.line == fromLine-1) {
            newCaret = ensureCharPosValid(CharPos{oldCaret.ch, toLine});
        }
        CharPos newSelBegin;
        if (fromLine != origBlockBegin.line) {
            if (origBlockBegin.line ==0 ) {
                newSelBegin = fileBegin();
            } else {
                newSelBegin.line = origBlockBegin.line-1;
                newSelBegin.ch = mDocument->getLine(newSelBegin.line).length();
            }
        } else {
            newSelBegin=CharPos{origBlockBegin.ch, origBlockBegin.line-1};
        }
        CharPos newSelEnd = ensureCharPosValid(CharPos{origBlockEnd.ch, origBlockEnd.line-1});
        setCaretAndSelection(
                  newCaret,
                  newSelBegin,
                  newSelEnd
        );
        addChangeToUndo(ChangeReason::MoveLine,
                CharPos{0, fromLine-1},
                CharPos{0, toLine},
                QStringList(),
                SelectionMode::Normal);
        endEditing();
    }
}

void QSynEdit::doMoveSelDown()
{
    if (mActiveSelectionMode == SelectionMode::Column)
        return;
    int fromLine,toLine;
    CharPos origBlockBegin = selBegin();
    CharPos origBlockEnd = selEnd();
    calcEffectiveFromToLine(origBlockBegin, origBlockEnd, fromLine, toLine);
    if (!mReadOnly && (!mDocument->empty()) && (toLine+1 < mDocument->count())) {
        CharPos oldCaret = caretPos();
        beginEditing();

        // Move line below selection to above selection
        properMoveLine(toLine + 1, fromLine, true);

        // Restore caret and selection
        CharPos newCaret;
        if (oldCaret>= origBlockBegin && oldCaret <= origBlockEnd) {
            newCaret = ensureCharPosValid(CharPos{oldCaret.ch, oldCaret.line+1});
        } else if (oldCaret.line == toLine+1) {
            newCaret = ensureCharPosValid(CharPos{oldCaret.ch, fromLine});
        }
        CharPos newSelBegin;
        if (fromLine != origBlockBegin.line) {
            newSelBegin.line = origBlockBegin.line+1;
            newSelBegin.ch = mDocument->getLine(newSelBegin.line).length();
        } else {
            newSelBegin=CharPos{origBlockBegin.ch, origBlockBegin.line+1};
        }
        CharPos newSelEnd = ensureCharPosValid(CharPos{origBlockEnd.ch, origBlockEnd.line+1});
        setCaretAndSelection(
                    newCaret,
                    newSelBegin,
                    newSelEnd
                    );

        addChangeToUndo(ChangeReason::MoveLine,
                CharPos{0, toLine + 1},
                CharPos{0, fromLine},
                QStringList(),
                SelectionMode::Normal);
        endEditing();
    }
}

void QSynEdit::internalClearAll()
{
    beginEditing();
    setCaretPos(fileBegin());
    mCodeBlocks.clear();
    mDocument->clear();
    reparseDocument();
    setModified(false);
    clearUndo();
    endEditing();
}

void QSynEdit::doClearAll()
{
    int oldCount = mDocument->count();
    internalClearAll();
    emit linesDeleted(0, oldCount-1);
}

void QSynEdit::doBreakLine()
{
    if (mReadOnly)
        return;
    if (!mUndoing)
        beginEditing();
    auto action = finally([this] {
        if (!mUndoing)
            endEditing();
    });
    QString helper;
    if (hasSelContent()) {
        helper = selText();
        doDeleteSelection();
    }

    QString temp = lineText();
//    if (mCaretCh>lineText().length()) {
//        PCodeFoldingRange foldRange = foldStartAtLine(mCaretLine);
//        if ((foldRange) && foldRange->collapsed) {
//            QString s = temp+mSyntaxer->foldString(temp);
//            if (mCaretCh >= s.length()) {
//                if (!mUndoing) {
//                    addCaretToUndo();
//                    addSelectionToUndo();
//                }
//                mCaretLine=foldRange->toLine;
//                if (mCaretLine>=mDocument->count()) {
//                    mCaretLine=mDocument->count()-1;
//                }
//                temp = lineText();
//                mCaretCh=temp.length();
//            }
//        }
//    }

    // This is sloppy, but the Right Thing would be to track the column of markers
    // too, so they could be moved depending on whether they are after the caret...
    QString leftLineText = lineText().mid(0, mCaretCh);
    QString rightLineText = lineText().mid(mCaretCh);
    if (!mUndoing)
        addChangeToUndo(ChangeReason::LineBreak, caretPos(), caretPos(), QStringList(rightLineText),
              SelectionMode::Normal);
    bool notInComment=true;
    QString trimmedleftLineText=trimLeft(leftLineText);
    startParseLine(mSyntaxer.get(), mCaretLine, trimmedleftLineText);
    int indentSpaces = 0;
    if (!mUndoing
            && mSyntaxer->language() == ProgrammingLanguage::CPP
            && mSyntaxer->getToken()=="else"
            && shouldRecalcIndent(mCaretLine)) {
        indentSpaces = calcIndentSpaces(mCaretLine,
                                        trimmedleftLineText,true
                                        );
        QString indentSpacesForLeftLineText = genSpaces(indentSpaces);
        leftLineText = indentSpacesForLeftLineText + trimmedleftLineText;
    }
    bool insertAfter = shouldInsertAfterCurrentLine(mCaretLine, leftLineText, rightLineText, false);
    if (insertAfter)
        properSetLine(mCaretLine, leftLineText, false);
    else
        properInsertLine(mCaretLine,leftLineText,false);


    notInComment = !mSyntaxer->isCommentNotFinished(
                mSyntaxer->getState())
            && !mSyntaxer->isStringNotFinished(
                mSyntaxer->getState());

    indentSpaces = 0;
    if (shouldRecalcIndent(mCaretLine+1)) {
        rightLineText=trimLeft(rightLineText);
        indentSpaces = calcIndentSpaces(mCaretLine+1,
                                        rightLineText,true
                                            );
    }
    QString indentSpacesForRightLineText = genSpaces(indentSpaces);
    if (insertAfter)
        properInsertLine(mCaretLine+1, indentSpacesForRightLineText+rightLineText, true);
    else
        properSetLine(mCaretLine+1, indentSpacesForRightLineText+rightLineText, true);

    if (!mUndoing) {
        //insert new line in middle of "/*" and "*/"
        if (!notInComment && (!syntaxer()->blockCommentBeginSymbol().isEmpty())
                && (!syntaxer()->blockCommentEndSymbol().isEmpty())
                && ( leftLineText.trimmed().endsWith(syntaxer()->blockCommentBeginSymbol())
                     && rightLineText.trimmed().startsWith(syntaxer()->blockCommentEndSymbol())
                 )) {
            indentSpaces = calcIndentSpaces(mCaretLine+1, "" , mOptions.testFlag(EditorOption::AutoIndent));
            indentSpacesForRightLineText = genSpaces(indentSpaces);
            properInsertLine(mCaretLine+1, indentSpacesForRightLineText ,false);
            addChangeToUndo(ChangeReason::LineBreak, caretPos(), caretPos(), QStringList(),
                    SelectionMode::Normal);
        }
        //insert new line in middle of "{" and "}"
        if (notInComment &&
                ( leftLineText.trimmed().endsWith('{') && rightLineText.trimmed().startsWith('}')
                 )) {
            indentSpaces = calcIndentSpaces(mCaretLine+1, "" , mOptions.testFlag(EditorOption::AutoIndent)
                                                                   && notInComment);
            indentSpacesForRightLineText = genSpaces(indentSpaces);
            properInsertLine(mCaretLine+1, indentSpacesForRightLineText, false);
            addChangeToUndo(ChangeReason::LineBreak, caretPos(), caretPos(), QStringList(),
                    SelectionMode::Normal);
        }
    }
    setCaretPos(CharPos{(int)indentSpacesForRightLineText.length(),mCaretLine + 1});
    updateLastCaretX();
}

void QSynEdit::doTabKey()
{
    // Provide Visual Studio like block indenting
    if (mActiveSelectionMode != SelectionMode::Column && mOptions.testFlag(EditorOption::TabIndent) && canDoBlockIndent()) {
        doBlockIndent();
        return;
    }
    beginEditing();
    if (hasSelContent()) {
        doDeleteSelection();
    }
    QString spaces;
    if (mOptions.testFlag(EditorOption::TabsToSpaces)) {
        int left = charToGlyphLeft(mCaretLine,mCaretCh);
        int i = std::ceil( (tabWidth() - (left) % tabWidth() ) / (float) tabSize());
        spaces = QString(i,' ');
    } else {
        spaces = '\t';
    }
    doInputStr(spaces);
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

    if (caretLine()<0 || caretLine() >= mDocument->count()) {
        return false;
    }

    QString s = mDocument->getLine(caretLine()).left(caretCh());
    return (s.trimmed().isEmpty());
}

QRect QSynEdit::calculateCaretRect() const
{
    DisplayCoord coord = displayXY();
    if (!mInputPreeditString.isEmpty()) {
        QString lineTextWithPreedit = lineText().left(mCaretCh)
                + mInputPreeditString
                + lineText().mid(mCaretCh);
        if (lineTextWithPreedit == mGlyphPostionCacheForInputMethod.str)  {
            int glyphIdx = searchForSegmentIdx(mGlyphPostionCacheForInputMethod.glyphCharList,lineTextWithPreedit.length(),mCaretCh+mInputPreeditString.length());
            coord.x = segmentIntervalStart(mGlyphPostionCacheForInputMethod.glyphPositionList,mGlyphPostionCacheForInputMethod.strWidth, glyphIdx);
        } else
            coord.x = charToGlyphLeft(mCaretLine, lineTextWithPreedit, mCaretCh+mInputPreeditString.length());
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
    if (mCaretLine >= 0 && mCaretLine < mDocument->count()
            && mCaretCh >=0 && mCaretCh < mDocument->getLine(mCaretLine).length()) {
        int glyphIndex = mDocument->charToGlyphIndex(mCaretLine, mCaretCh);
        caretWidth = mDocument->glyphWidth(mCaretLine, glyphIndex);
    }
//    qDebug()<<"caret:"<<mCaretCh<<mCaretLine<<caretWidth;
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
    if (mCaretLine >= 0 && mCaretLine < mDocument->count()
            && mCaretCh >=0 && mCaretCh < mDocument->getLine(mCaretLine).length()) {
        int glyphIndex = mDocument->charToGlyphIndex(mCaretLine, mCaretCh);
        caretWidth = mDocument->glyphWidth(mCaretLine, glyphIndex);
    }
    return QRect(caretPos.x(),caretPos.y(),caretWidth,
                 mTextHeight);
}

void QSynEdit::computeCaret()
{
    QPoint mousePos = QCursor::pos();
    mousePos = mapFromGlobal(mousePos);
    int x=mousePos.x();
    int y=mousePos.y();

    DisplayCoord caretNearestPos = pixelsToNearestGlyphPos(x, y);
    caretNearestPos.row = minMax(caretNearestPos.row, 0, displayLineCount()-1);
    setCaretDisplayXY(caretNearestPos, false);
}

void QSynEdit::computeScroll(bool isDragging)
{
    QPoint mousePos = QCursor::pos();
    mousePos = mapFromGlobal(mousePos);
    int x=mousePos.x();
    int y=mousePos.y();

    QRect scrollBounds; // relative to the client area
    int dispX=2,dispY = 2;
    int left = mGutterWidth+frameWidth()+dispX;
    int top = frameWidth()+dispY;
    scrollBounds = QRect(left,
                          top,
                          clientWidth()-left-dispX,
                          clientHeight()-top-dispY);

    int scrollX,scrollY;
    if (x < scrollBounds.left())
        scrollX = (x - scrollBounds.left()) / mCharWidth - 1;
    else if (x >= scrollBounds.right())
        scrollX = (x - scrollBounds.right()) / mCharWidth + 1;
    else
        scrollX = 0;

    if (y < scrollBounds.top())
        scrollY = (y - scrollBounds.top()) / mTextHeight - 1;
    else if (y >= scrollBounds.bottom()) {
        scrollY = (y - scrollBounds.bottom()) / mTextHeight + 1;
    } else
        scrollY = 0;

    doMouseScroll(isDragging, scrollX, scrollY);
}

void QSynEdit::doBlockIndent()
{
    CharPos  oldCaretPos{caretPos()};
    CharPos  blockBegin, blockEnd;
    QStringList strToInsert;
    int endLine,newCaretX;
    QString spaces;

    // keep current selection detail
    if (hasSelContent()) {
        blockBegin = selBegin();
        blockEnd = selEnd();
    } else {
        blockBegin = caretPos();
        blockEnd = caretPos();
    }
    // build text to insert
    if (blockEnd.ch == 0 && blockEnd.line != blockBegin.line) {
        endLine = blockEnd.line - 1;
        newCaretX = 0;
    } else {
        endLine = blockEnd.line;
        if (mOptions.testFlag(EditorOption::TabsToSpaces))
            newCaretX = caretCh() + tabSize();
        else
          newCaretX = caretCh() + 1;
    }
    endLine = std::min(endLine, mDocument->count()-1);
    if (mOptions.testFlag(EditorOption::TabsToSpaces)) {
        spaces = QString(tabSize(),' ') ;
    } else {
        spaces = "\t";
    }
    beginEditing();
    for (int i = blockBegin.line; i<=endLine;i++) {
        QString lineText=mDocument->getLine(i);
        addChangeToUndo(ChangeReason::Insert,
                CharPos{0, i},
                CharPos{(int)spaces.length(), i},
                QStringList(),
                SelectionMode::Normal);
        if (lineText.isEmpty()) {
            lineText = spaces;
        } else {
            lineText = spaces+lineText;
        }
        properSetLine(i,lineText, i==endLine);
    }
    //adjust caret and selection
    oldCaretPos.ch = newCaretX;
    if (blockBegin == blockEnd) {
        setCaretAndSelection(oldCaretPos,
          oldCaretPos, oldCaretPos);
    } else {
        if (blockBegin.ch > 0)
            blockBegin.ch += spaces.length();
        if (blockEnd.ch > 0)
          blockEnd.ch+=spaces.length();
        setCaretAndSelection(oldCaretPos,
          blockBegin, blockEnd);
    }
    endEditing();
}

void QSynEdit::doBlockUnindent()
{
    int lastLineIndent = 0;
    int firstLineIndent = 0;

    CharPos blockBegin,blockEnd;
    // keep current selection detail
    if (hasSelContent()) {
        blockBegin = selBegin();
        blockEnd = selEnd();
    } else {
        blockBegin = caretPos();
        blockEnd = caretPos();
    }
    CharPos oldCaretPos = caretPos();
    int caretLineIndent = 0;
    beginEditing();

    int endLine = blockEnd.line;
    // convert selection to complete lines
    if (blockEnd.ch == 0 && blockEnd.line != blockBegin.line)
        endLine = blockEnd.line - 1;
    // build string to delete
    for (int i = blockBegin.line; i<= endLine;i++) {
        QString lineText = mDocument->getLine(i);
        if (lineText.isEmpty())
            continue;
        int charsToDelete = 0;
        if (lineText[0]=='\t') {
            charsToDelete = 1;
        } else {
            while (charsToDelete < tabSize() &&
                   charsToDelete < lineText.length() &&
                   lineText[charsToDelete] == ' ')
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
        QString tempString = lineText.mid(charsToDelete);
        properSetLine(i,tempString, i==endLine);
        addChangeToUndo(ChangeReason::Delete,
                CharPos{0,i},
                CharPos{charsToDelete,i},
                QStringList(lineText.left(charsToDelete)),
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

void QSynEdit::internalInputStr(const QString& inputStr)
{
    if ((mActiveSelectionMode != SelectionMode::Normal)
            || selAvail()) {
        doSetSelText(inputStr);
        return;
    }
    Q_ASSERT(validGlyphStart(caretPos()));
    beginEditing();
    QString s = lineText();
    QString replaceStr;
    QString newS;
    if (!mInserting) {
        int inputGlyphCounts = calcGlyphStartCharList(inputStr).length();
        int currentGlyphIdx = mDocument->charToGlyphIndex(mCaretLine, mCaretCh);
        if (currentGlyphIdx < mDocument->glyphCount(mCaretLine)) {
            int endDeleteGlyphIdx = std::min(currentGlyphIdx + inputGlyphCounts - 1, mDocument->glyphCount(mCaretLine) - 1);
            int replaceEnd = mDocument->glyphEndChar(mCaretLine, endDeleteGlyphIdx);
            replaceStr = s.mid(mCaretCh, replaceEnd-mCaretCh);
            newS = s.left(mCaretCh)+inputStr+s.mid(replaceEnd);
        }
    }
    if (replaceStr.isEmpty()) {
        newS = s.left(mCaretCh)+inputStr+s.mid(mCaretCh);
    }
    properSetLine(mCaretLine,newS,true);
    addChangeToUndo(ChangeReason::Input,
            CharPos{mCaretCh, mCaretLine},
            CharPos{mCaretCh+(int)inputStr.length(), mCaretLine},
            QStringList(replaceStr),
            SelectionMode::Normal
            );
    setCaretCh(mCaretCh+inputStr.length());
    endEditing();
}

void QSynEdit::doInputStr(const QString& s)
{
    if (mReadOnly)
        return;
    QString inputStr = s;
    for(int i=0;i<s.length();i++) {
        if (s[i]=='\r' || s[i]=='\n' || s[i]=='\0') {
            inputStr = s.left(i);
            break;
        }
    }
    if (inputStr.isEmpty())
        return;
    if (inputStr.length() == 1 ) {
        if (!inputStr[0].isPrint() && inputStr[0]!='\t')
            return;
    }

    if (mActiveSelectionMode == SelectionMode::Column) {
        //todo: column-mode input is not implemented
        if (mInserting == false && !hasSelContent()) {
            CharPos start=mSelectionBegin;
            CharPos end=mSelectionEnd;
            if (start.line > end.line )
                std::swap(start,end);
            start.ch++; // make sure we select a whole char in the start line
            internalSetSelBegin(start);
            internalSetSelEnd(end);
        }
        return;
    }
    beginEditing();
    {

        if (inputStr.length()>1) {
            bool shouldAddBreak = true;
            if (!selAvail()) {
                PUndoItem undoItem = mUndoList->peekItem();
                if (undoItem && undoItem->changeReason()==ChangeReason::Input
                        && undoItem->changeEndPos().line == mCaretLine
                        && undoItem->changeEndPos().ch == mCaretCh
                        && undoItem->changeEndPos().ch - undoItem->changeStartPos().ch>1) {
                    shouldAddBreak = false;
                }
            }
            if (shouldAddBreak)
                addGroupUndoBreak();
            internalInputStr(inputStr);
        } else {
            QChar lastCh{0};
            if (!selAvail()) {
                PUndoItem undoItem = mUndoList->peekItem();
                if (undoItem && undoItem->changeReason()==ChangeReason::Input) {
                    if (undoItem->changeEndPos().line == mCaretLine
                        && undoItem->changeEndPos().ch == mCaretCh
                        && undoItem->changeStartPos().line == mCaretLine
                        && undoItem->changeStartPos().ch == mCaretCh-1) {
                        QString lineText = mDocument->getLine(mCaretLine);
                        int i=mCaretCh-1;
                        if (i>=0 && i<lineText.length())
                            lastCh=lineText[i];
                    } else {
                        addGroupUndoBreak();
                    }
                }
            }
            if (isIdentChar(inputStr[0])) {
                if (lastCh!=0 && !isIdentChar(lastCh)) {
                    addGroupUndoBreak();
                }
                internalInputStr(inputStr);
            } else if (isSpaceChar(inputStr[0])) {
                // break group undo chain
                if (lastCh!=0 && !isSpaceChar(lastCh)) {
                    addGroupUndoBreak();
                }
                internalInputStr(inputStr);
            } else {
                if ((lastCh!=0 && isSpaceChar(lastCh)) || isIdentChar(lastCh)) {
                    addGroupUndoBreak();
                }
                internalInputStr(inputStr);
                int oldCaretX=mCaretCh - inputStr.length();
                int oldCaretY=mCaretLine;
                // auto indent
                if (mActiveSelectionMode==SelectionMode::Normal
                        && shouldRecalcIndent(oldCaretY)
                        ) {

                    //unindent if ':' at end of the line
                    if (inputStr == ":") {
                        QString lineText = mDocument->getLine(oldCaretY);
                        if (mCaretCh == lineText.length()) {
                            int indentSpaces = calcIndentSpaces(oldCaretY,lineText, true);
                            if (indentSpaces != leftSpaces(lineText)) {
                                QString newLine = genSpaces(indentSpaces) + trimLeft(lineText);
                                properSetLine(oldCaretY,newLine,true);
                                setCaretPos(CharPos{(int)newLine.length(),oldCaretY});
                                addChangeToUndo(ChangeReason::Delete,
                                        CharPos{0, oldCaretY},
                                        CharPos{(int)lineText.length(), oldCaretY},
                                        QStringList(lineText),
                                        SelectionMode::Normal
                                        );
                                addChangeToUndo(ChangeReason::Insert,
                                        CharPos{0, oldCaretY},
                                        CharPos{(int)newLine.length(), oldCaretY},
                                        QStringList(),
                                        SelectionMode::Normal
                                        );
                            }
                        }
                    } else if (inputStr == "*") {
                        QString lineText = mDocument->getLine(oldCaretY);
                        if (mCaretCh == lineText.length()) {
                            int indentSpaces = calcIndentSpaces(oldCaretY,lineText+"*", true);
                            if (indentSpaces != leftSpaces(lineText)) {
                                QString newLine = genSpaces(indentSpaces) + trimLeft(lineText);
                                properSetLine(oldCaretY,newLine, true);
                                setCaretPos(CharPos{(int)newLine.length(), oldCaretY});
                                addChangeToUndo(ChangeReason::Delete,
                                        CharPos{0, oldCaretY},
                                        CharPos{(int)lineText.length(), oldCaretY},
                                        QStringList(lineText),
                                        SelectionMode::Normal
                                        );
                                addChangeToUndo(ChangeReason::Insert,
                                        CharPos{0, oldCaretY},
                                        CharPos{(int)newLine.length(), oldCaretY},
                                        QStringList(),
                                        SelectionMode::Normal
                                        );
                            }
                        }
                    } else if (inputStr[0] == "{" || inputStr[0] == "}" || inputStr[0] == "#") {
                        //Reindent line when add '{' '}' and '#' at the beginning
                        QString left = mDocument->getLine(oldCaretY).left(oldCaretX);
                        // and the first nonblank char is this new {
                        if (left.trimmed().isEmpty()) {
                            int indentSpaces = calcIndentSpaces(oldCaretY,inputStr, true);
                            if (indentSpaces != leftSpaces(left)) {
                                QString right = mDocument->getLine(oldCaretY).mid(oldCaretX);
                                QString newLeft = genSpaces(indentSpaces);
                                properSetLine(oldCaretY,newLeft+right, true);
                                setCaretPos(CharPos{(int)newLeft.length(),oldCaretY});
                                addChangeToUndo(ChangeReason::Delete,
                                        CharPos{0, oldCaretY},
                                        CharPos{(int)left.length(), oldCaretY},
                                        QStringList(left),
                                        SelectionMode::Normal
                                        );
                                addChangeToUndo(ChangeReason::Insert,
                                        CharPos{0, oldCaretY},
                                        CharPos{(int)newLeft.length(), oldCaretY},
                                        QStringList(""),
                                        SelectionMode::Normal
                                        );

                            }
                        }
                    }
                }
            }
        }
    }
    endEditing();
}

void QSynEdit::doCutToClipboard()
{
    if (mReadOnly || mDocument->empty())
        return;
    beginEditing();
    if (!hasSelContent()) {
        doSelectLine();
    }
    internalDoCopyToClipboard(selText());
    doDeleteSelection();
    endEditing();
}

void QSynEdit::doCopyToClipboard()
{
    bool selected=hasSelContent();
    if (!selected)
        doSelectLine();
    QString text;
    text = selText();
    internalDoCopyToClipboard(text);
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
    doSetSelTextPrimitive(splitStrings(text));
    endEditing();
}

void QSynEdit::beginInternalChanges()
{
    mPaintLock ++ ;
    beginMergeCaretAndSelectionStatusChange();
}

void QSynEdit::endInternalChanges()
{
    Q_ASSERT(mPaintLock > 0);
    endMergeCaretAndSelectionStatusChange();
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

void QSynEdit::beginMergeCaretAndSelectionStatusChange()
{
    if (mMergeCaretStatusChangeLock == 0) {
        mCaretBeforeMerging = caretPos();
        mSelBeginBeforeMerging = selBegin();
        mSelEndBeforeMerging = selEnd();
        mSelModeBeforeMerging = mActiveSelectionMode;
    }
    ++mMergeCaretStatusChangeLock;
}

void QSynEdit::endMergeCaretAndSelectionStatusChange()
{
    --mMergeCaretStatusChangeLock;
    if (mMergeCaretStatusChangeLock == 0) {
        //ensureCaretVisible();
        if (mCaretBeforeMerging.ch != mCaretCh)
            setStatusChanged(StatusChange::CaretX);
        if (mCaretBeforeMerging.line != mCaretLine)
            setStatusChanged(StatusChange::CaretY);
        CharPos selBeginNow = selBegin();
        CharPos selEndNow = selEnd();
        if (mSelModeBeforeMerging != mActiveSelectionMode)
            setStatusChanged(StatusChange::Selection);
        if (mSelBeginBeforeMerging == mSelEndBeforeMerging) {
            if (selBeginNow != selEndNow)
                setStatusChanged(StatusChange::Selection);
        } else {
            if (selBeginNow != mSelBeginBeforeMerging
                    || selEndNow != mSelEndBeforeMerging)
                setStatusChanged(StatusChange::Selection);
        }        
    }
}

PSyntaxState QSynEdit::calcSyntaxStateAtLine(int line, const QString &newLineText, bool handleLastBackSlash) const
{
    PSyntaxer syntaxer = mSyntaxer->createInstance();
    if (syntaxer->language() == ProgrammingLanguage::CPP) {
        std::shared_ptr<QSynedit::CppSyntaxer> cppSyntaxer = std::dynamic_pointer_cast<QSynedit::CppSyntaxer>(syntaxer);
        cppSyntaxer->setHandleLastBackSlash(handleLastBackSlash);
    }
    startParseLine(syntaxer.get(), line, newLineText);
    syntaxer->nextToEol();
    return syntaxer->getState();
}

void QSynEdit::invalidateAllNonTempLineWidth()
{
    mDocument->invalidateAllNonTempLineWidth();
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
    mLastCaretX = displayX();
}

void QSynEdit::ensureCaretVisible(bool forceToMiddle)
{
    // Make sure Y is visible
    int caretRow = displayY();
    if (forceToMiddle) {
        if (caretRow * mTextHeight < mTopPos
                ||
                (caretRow+1) * mTextHeight > mTopPos + clientHeight() )
            setTopPos( (caretRow - (mLinesInWindow - 1) / 2) * mTextHeight);
    } else {
        if (caretRow * mTextHeight < mTopPos)
            setTopPos(caretRow * mTextHeight);
        else if ((caretRow+1) * mTextHeight > mTopPos + clientHeight() ) {
            int value = calcLineAlignedTopPos((caretRow+1) * mTextHeight - clientHeight(), true);
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

void QSynEdit::setCaretDisplayXY(const DisplayCoord &pos, bool ensureCaretVisible)
{
    beginInternalChanges();
    internalSetCaretPos(displayToBufferPos(pos), ensureCaretVisible);
    endInternalChanges();
}

void QSynEdit::internalSetCaretPos(CharPos value, bool ensureVisible)
{
    Q_ASSERT(validInDoc(value));
    Q_ASSERT(validGlyphStart(value));
    if ((value.ch != mCaretCh) || (value.line != mCaretLine)) {
        beginInternalChanges();
        auto action = finally([this]{
            endInternalChanges();
        });
        if (mCaretCh != value.ch) {
            mCaretCh = value.ch;
            setStatusChanged(StatusChange::CaretX);
            invalidateLine(mCaretLine);
        }
        if (mCaretLine != value.line) {
            int oldCaretLine = mCaretLine;
            mCaretLine = value.line;
            invalidateLine(mCaretLine);
            invalidateGutterLine(mCaretLine);
            invalidateLine(oldCaretLine);
            invalidateGutterLine(oldCaretLine);
            setStatusChanged(StatusChange::CaretY);
        }
        updateLastCaretX();
        if (ensureVisible)
            ensureCaretVisible();
    } else {
        // Always update mLastCaretX whenever the caret is assigned to,
        // even if it didn't move.
        updateLastCaretX();
    }
}

void QSynEdit::internalSetCaretCh(int value)
{
    internalSetCaretPos(CharPos{value, mCaretLine});
}

void QSynEdit::setStatusChanged(StatusChanges changes)
{
    if (mMergeCaretStatusChangeLock>0)
        changes &= ~(StatusChange::CaretX | StatusChange::CaretY | StatusChange::Selection);
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
    int minValue = 0;
    int maxValue = maxScrollWidth();
    if (maxValue<0)
        return;
    int pageStep = viewWidth();
    int value = mLeftPos;
    horizontalScrollBar()->setMinimum(minValue);
    horizontalScrollBar()->setMaximum(maxValue);
    horizontalScrollBar()->setPageStep(pageStep);
    horizontalScrollBar()->setValue(value);
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
    int minValue = 0;
    int maxValue = maxScrollHeight();
    int pageStep = mLinesInWindow * mTextHeight;
    int value = mTopPos;
    verticalScrollBar()->setMinimum(minValue);
    verticalScrollBar()->setMaximum(maxValue);
    verticalScrollBar()->setPageStep(pageStep);
    verticalScrollBar()->setValue(value);
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
        QFontMetrics styleFm(f);
        if (styleFm.lineSpacing()>mTextHeight)
            mTextHeight=styleFm.lineSpacing();
        if (styleFm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = styleFm.horizontalAdvance("M");
    }
    if (hasStyles[1]) { // has italic font
        QFont f = font();
        f.setItalic(true);
        QFontMetrics styleFm(f);
        if (styleFm.lineSpacing()>mTextHeight)
            mTextHeight=styleFm.lineSpacing();
        if (styleFm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = styleFm.horizontalAdvance("M");
    }
    if (hasStyles[2]) { // has strikeout
        QFont f = font();
        f.setStrikeOut(true);
        QFontMetrics styleFm(f);
        if (styleFm.lineSpacing()>mTextHeight)
            mTextHeight=styleFm.lineSpacing();
        if (styleFm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = styleFm.horizontalAdvance("M");
    }
    if (hasStyles[3]) { // has underline
        QFont f = font();
        f.setUnderline(true);
        QFontMetrics styleFm(f);
        if (styleFm.lineSpacing()>mTextHeight)
            mTextHeight=styleFm.lineSpacing();
        if (styleFm.horizontalAdvance("M")>mCharWidth)
            mCharWidth = styleFm.horizontalAdvance("M");
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


    if (startLine >= maxLine) {
        invalidateLines(startLine, INT_MAX);
        return startLine;
    }

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
    rescanCodeBlocks();
}

void QSynEdit::uncollapse(const PCodeBlock &foldRange)
{
    beginInternalChanges();
    foldRange->collapsed = false;

    // Redraw the collapsed line
    invalidateLines(foldRange->fromLine, INT_MAX);

    // Redraw fold mark
    invalidateGutterLines(foldRange->fromLine, INT_MAX);
    updateHScrollbar();
    updateVScrollbar();
    ensureCaretVisible();
    endInternalChanges();
}

void QSynEdit::collapse(const PCodeBlock &foldRange)
{
    beginInternalChanges();
    foldRange->collapsed = true;

    // Extract caret from fold
    if ((mCaretLine > foldRange->fromLine) && (mCaretLine <= foldRange->toLine)) {
          setCaretPos(CharPos{(int)mDocument->getLine(foldRange->fromLine).length(),
                                 foldRange->fromLine});
    }

    // Redraw the collapsed line
    invalidateLines(foldRange->fromLine, INT_MAX);

    // Redraw fold mark
    invalidateGutterLines(foldRange->fromLine, INT_MAX);

    updateHScrollbar();
    updateVScrollbar();
    ensureCaretVisible();
    endInternalChanges();
}

bool QSynEdit::collapse(int fromLine, int toLine)
{
    foreach(const PCodeBlock &block, mCodeBlocks) {
        if (block->fromLine == fromLine
                && block->toLine == toLine) {
            collapse(block);
            return true;
        }
    }
    return false;
}

bool QSynEdit::uncollapase(int fromLine, int toLine)
{
    foreach(const PCodeBlock &block, mCodeBlocks) {
        if (block->fromLine == fromLine
                && block->toLine == toLine) {
            uncollapse(block);
            return true;
        }
    }
    return false;
}

#ifdef QSYNEDIT_TEST
int QSynEdit::subBlockCounts(int fromLine, int toLine) const
{
    foreach(const PCodeBlock &block, mCodeBlocks) {
        if (block->fromLine == fromLine
                && block->toLine == toLine) {
            return block->subBlocks.count();
        }
    }
    return 0;
}

bool QSynEdit::isCollapsed(int fromLine, int toLine) const
{
    foreach(const PCodeBlock &block, mCodeBlocks) {
        if (block->fromLine == fromLine
                && block->toLine == toLine) {
            return block->collapsed;
        }
    }
    return false;
}

int QSynEdit::codeBlockCount() const
{
    return mCodeBlocks.count();
}

bool QSynEdit::hasCodeBlock(int fromLine, int toLine) const
{
    foreach(const PCodeBlock &block, mCodeBlocks) {
        if (block->fromLine == fromLine
                && block->toLine == toLine) {
            return true;
        }
    }
    return false;
}
#endif

void QSynEdit::processCodeBlocksOnLinesInserted(int line, int count)
{
    if (!useCodeFolding())
        return;
    bool collapseChanged=false;
    for (int i = mCodeBlocks.count()-1;i>=0;i--) {
        PCodeBlock block = mCodeBlocks[i];
        if (block->fromLine >= line) // insertion of count lines above FromLine
            block->move(count);
        else if (block->toLine >= line) {
            block->toLine += count;
            if (block->collapsed) { // uncollapse it
                collapseChanged = true;
                block->collapsed = false;
            }
        }
    }
    if (collapseChanged) {
        beginInternalChanges();
        updateHScrollbar();
        updateVScrollbar();
        invalidate();
        endInternalChanges();
    }
}

void QSynEdit::processFoldsOnLinesDeleted(int line, int count)
{
    if (!useCodeFolding())
        return ;

    bool collapseChanged=false;
    beginInternalChanges();
    for (int i = mCodeBlocks.count()-1;i>=0;i--) {
        PCodeBlock block = mCodeBlocks[i];
        if (block->fromLine >= line
                && block->fromLine < line+count)  {
            mCodeBlocks.remove(i);
        } else if (block->toLine >= line
                   && block->toLine < line+count) {
            mCodeBlocks.remove(i);
        } else if (block->fromLine >= line + count) { // Move after affected area
            block->move(-count);
        } else if (block->toLine >= line + count) {
            if (block->toLine <= block->fromLine)
                mCodeBlocks.remove(i);
            else {
                block->toLine -= count;
                if (block->collapsed) { // uncollapse it
                    collapseChanged = true;
                    block->collapsed = false;
                }
            }
        }
    }
    if (collapseChanged) {
        updateHScrollbar();
        updateVScrollbar();
        invalidate();
    }
    endInternalChanges();
}

void QSynEdit::processFoldsOnLineMoved(int from, int to)
{
    if (!useCodeFolding())
        return;
    bool collapseChanged=false;
    for (int i = mCodeBlocks.count()-1;i>=0;i--) {
        PCodeBlock block = mCodeBlocks[i];
        if (block->fromLine == from || block->toLine == from) {
            if (block->collapsed) {
                collapseChanged = true;
                block->collapsed=false;
            }
        }
        if (from<to) {
            if (block->fromLine == from) {
                block->fromLine = to;
            } else if (from < block->fromLine && block->fromLine <= to)
                block->fromLine -=1;
            if (block->toLine == from) {
                block->toLine = to;
            } else if (from < block->toLine && block->toLine <= to)
                block->toLine -= 1;
        } else if (to > from) {
            if (block->fromLine == from) {
                block->fromLine = to;
            } else if (to <= block->fromLine && block->fromLine < from)
                block->fromLine +=1;
            if (block->toLine == from) {
                block->toLine = to;
            } else if (to <= block->toLine && block->toLine < from)
                block->toLine +=1;
        }
        if (block->toLine<=block->fromLine)
            mCodeBlocks.remove(i);
    }
    if (collapseChanged) {
        beginInternalChanges();
        updateHScrollbar();
        updateVScrollbar();
        invalidate();
        endInternalChanges();
    }
}

void QSynEdit::rescanCodeBlocks()
{
    if (!useCodeFolding())
        return;

    beginInternalChanges();
    // Did we leave any collapsed folds and are we viewing a code file?
    if (mCodeBlocks.count() > 0) {
        QMap<QString,PCodeBlock> rangeIndexes;
        foreach(const PCodeBlock& block, mCodeBlocks) {
            if (block->collapsed)
                rangeIndexes.insert(QString("%1-%2").arg(block->fromLine).arg(block->toLine),block);
        }

        // Add folds to a separate list
        internalScanCodeBlocks();

        // Combine new with old folds, preserve parent order
        foreach(const PCodeBlock &tempBlock, mCodeBlocks) {
            PCodeBlock b2=rangeIndexes.value(QString("%1-%2").arg(tempBlock->fromLine).arg(tempBlock->toLine),
                                  PCodeBlock());
            if (b2) {
                tempBlock->collapsed=true;
            }
        }
    } else {
        // We ended up with no folds after deleting, just pass standard data...
        internalScanCodeBlocks();
    }
#ifdef QSYNEDIT_TEST
    emit foldsRescaned();
#endif
    invalidateGutter();
    endInternalChanges();
}

void QSynEdit::internalScanCodeBlocks()
{
    if (!useCodeFolding())
        return;

    mCodeBlocks.clear();
    PCodeBlock parent;
    int line = 0;
    while (line < mDocument->count()) { // index is valid for LinesToScan and fLines
        // If there is a collapsed fold over here, skip it
        // Find an opening character on this line
        int blockEnded=mDocument->blockEnded(line);
        int blockStarted=mDocument->blockStarted(line);
        if (blockEnded>0) {
            for (int i=0; i<blockEnded;i++) {
                if (!parent)
                    break;
                if (blockStarted>0)
                    parent->toLine = line - 1;
                else
                    parent->toLine = line;
                parent = parent->parent.lock();
            }
        }
        if (blockStarted>0) {
            for (int i=0; i<blockStarted;i++) {
                PCodeBlock newBlock = std::make_shared<CodeBlock>(parent,line,line);
                if (parent != nullptr) {
                    parent->subBlocks.append(newBlock);
                }
                mCodeBlocks.append(newBlock);
                parent = newBlock;
            }
        }
        line++;
    }
    //remove all unfinished folds
    while (parent != nullptr) {
        mCodeBlocks.removeAll(parent);
        parent = parent->parent.lock();
    }
}

PCodeBlock QSynEdit::foldStartAtLine(int line) const
{
    foreach(const PCodeBlock& range, mCodeBlocks) {
        if (range->fromLine == line ){
            return range;
        } else if (range->fromLine>line)
            break; // sorted by line. don't bother scanning further
    }
    return PCodeBlock();
}

PCodeBlock QSynEdit::foldAroundLine(int line)
{
    return foldAroundLineEx(line,false,false,false);
}

PCodeBlock QSynEdit::foldAroundLineEx(int line, bool wantCollapsed, bool acceptFromLine, bool acceptToLine)
{
    // Check global list
    PCodeBlock result = checkFoldRange(mCodeBlocks, line, wantCollapsed, acceptFromLine, acceptToLine);

    // Found an item in the top level list?
    if (result) {
        while (true) {
            PCodeBlock ResultChild = checkFoldRange(result->subBlocks, line, wantCollapsed, acceptFromLine, acceptToLine);
            if (!ResultChild)
                break;
            result = ResultChild; // repeat for this one
        }
    }
    return result;
}

PCodeBlock QSynEdit::checkFoldRange(const QVector<PCodeBlock> &blocksToCheck, int line, bool wantCollapsed, bool acceptFromLine, bool acceptToLine)
{
    foreach(const PCodeBlock &block, blocksToCheck){
        if (((block->fromLine < line) || ((block->fromLine <= line) && acceptFromLine)) &&
          ((block->toLine > line) || ((block->toLine >= line) && acceptToLine))) {
            if (block->collapsed == wantCollapsed) {
                return block;
            }
        }
    }
    return PCodeBlock();
}

PCodeBlock QSynEdit::foldEndAtLine(int line)
{
    foreach(const PCodeBlock &block, mCodeBlocks){
        if (block->toLine == line ){
            return block;
        } else if (block->fromLine>line)
            break; // sorted by line. don't bother scanning further
    }
    return PCodeBlock();
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

bool QSynEdit::empty() const
{
    return mDocument->empty();
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
        invalidateLine(mCaretLine);
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
        bool updateAll =
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
        internalSetCaretPos(caretPos());
        if (mOptions.testFlag(EditorOption::ScrollPastEol)) {
            internalSetSelBegin(selBegin());
            internalSetSelEnd(selEnd());
        }
        updateHScrollbar();
        updateVScrollbar();
        if (updateAll)
            invalidate();
        endInternalChanges();
    }
}

int QSynEdit::tabSize() const
{
    return mDocument->tabSize();
}

inline bool isGroupChange(ChangeReason reason){
    return (reason == ChangeReason::Input
            || reason == ChangeReason::DeleteChar
            || reason == ChangeReason::DeletePreviousChar);
}

void QSynEdit::doUndo()
{
    if (mReadOnly)
        return;

    QSet<size_t> undoedNumbers;

    beginEditing();
    //Remove Group Break;
    while (mUndoList->lastChangeReason() ==  ChangeReason::GroupBreak) {
        PUndoItem item = mUndoList->popItem();
        undoedNumbers.insert(item->changeNumber());
        mRedoList->addRedo(item);
    }

    PCaretAndSelectionInfo endInfo;
    PUndoItem item = mUndoList->peekItem();
    if (item) {
        size_t oldChangeNumber = item->changeNumber();
        ChangeReason  lastChange = mUndoList->lastChangeReason();
        bool keepGoing;
        do {
            doUndoItem();
            endInfo = mUndoList->caretAndSelBeforeChange(item->changeNumber());
            undoedNumbers.insert(item->changeNumber());
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
        setActiveSelectionMode(endInfo->selMode);
        setCaretAndSelection(endInfo->caret,endInfo->selBegin,endInfo->selEnd);
    }
    foreach(size_t number, undoedNumbers) {
        mRedoList->addCaretAndSelectionInfo(
                    number,
                    mUndoList->caretAndSelBeforeChange(number),
                    mUndoList->caretAndSelAfterChange(number));
        mUndoList->removeCaretAndSelInfo(number);
    }
    updateModifiedStatusForUndoRedo();
    endEditing();
    onChanged();
}

void QSynEdit::doUndoItem()
{
    mUndoing = true;
    beginEditing();
    bool changeScrollPastEol = ! mOptions.testFlag(EditorOption::ScrollPastEol);
    mOptions.setFlag(EditorOption::ScrollPastEol);
    auto action = finally([&,this]{
        endEditing();
        if (changeScrollPastEol)
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
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            internalSetCaretPos(item->changeStartPos());
            break;
        case ChangeReason::LeftTop:
        {
            CharPos p;
            p.ch = leftPos();
            p.line = topPos();
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            setLeftPos(item->changeStartPos().ch);
            setTopPos(item->changeStartPos().line);
        }
            break;
        case ChangeReason::Selection:
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            setCaretAndSelection(caretPos(), item->changeStartPos(), item->changeEndPos());
            break;
        case ChangeReason::InsertLine:
            Q_ASSERT(mDocument->count()==1);
            properDeleteLine(0,true);
            setCaretPos(fileBegin());
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList{},
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        case ChangeReason::Input: {
            doUndoRedoInput(item->changeText()[0],
                          item->changeStartPos(),
                          item->changeEndPos(),
                          item->changeSelMode(),
                          item->changeNumber(),
                    true);
            break;
        }
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
            setCaretPos(item->changeStartPos());
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
            properMoveLine(fromLine, toLine,true);
            setCaretPos(item->changeStartPos());
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
        }
            break;
        case ChangeReason::MergeWithNextLine: {
            CharPos startPos = item->changeStartPos();
            QString tempStr = mDocument->getLine(startPos.line);
            QString leftStr = tempStr.left(startPos.ch);
            QString rightStr = tempStr.mid(startPos.ch);
            QStringList helper({"",""});
            beginEditing();
            if (shouldInsertAfterCurrentLine(startPos.line, leftStr, rightStr, false)) {
                properSetLine(startPos.line, leftStr, false);
                properInsertLine(startPos.line+1, rightStr, true);
            } else {
                properInsertLine(startPos.line, leftStr,false);
                properSetLine(startPos.line+1, rightStr, true);
            }
            setCaretPos(item->changeStartPos());
            endEditing();
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        }
        case ChangeReason::MergeWithPrevLine: {
            CharPos startPos = item->changeStartPos();
            QString tempStr = mDocument->getLine(startPos.line);
            QString leftStr = tempStr.left(startPos.ch);
            QString rightStr = tempStr.mid(startPos.ch);
            QStringList helper({"",""});
            beginEditing();
            if (shouldInsertAfterCurrentLine(startPos.line, leftStr, rightStr, false)) {
                properSetLine(startPos.line, leftStr, false);
                properInsertLine(startPos.line+1, rightStr, true);
            } else {
                properInsertLine(startPos.line, leftStr,false);
                properSetLine(startPos.line+1, rightStr, true);
            }
            setCaretPos(item->changeEndPos());
            endEditing();
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        }
        case ChangeReason::DeleteChar:
        case ChangeReason::DeletePreviousChar:
        case ChangeReason::Delete: {
            // If there's no selection, we have to set
            // the Caret's position manualy.
//            qDebug()<<"undo delete";
//            qDebug()<<Item->changeText();
//            qDebug()<<Item->changeStartPos().Line<<Item->changeStartPos().Char;
            CharPos startPos = item->changeStartPos();
            CharPos endPos = item->changeEndPos();
            if (item->changeSelMode()==SelectionMode::Column) {
                // The recorded positions refer to the pre-deletion document
                // and may point past the shortened lines. Column deletion
                // preserves the part left of the band, so the left edge's
                // display x is exact; clamping the right edge to the line
                // length is safe since it stays >= the left edge's x.
                int xFrom = charToGlyphLeft(startPos.line,
                        std::min(startPos.ch,(int)mDocument->getLine(startPos.line).length()));
                int xTo = charToGlyphLeft(endPos.line,
                        std::min(endPos.ch,(int)mDocument->getLine(endPos.line).length()));
                if (xFrom > xTo)
                    std::swap(xFrom, xTo);
                startPos.ch = xposToGlyphStartChar(startPos.line,xFrom);
            }
            doInsertText(startPos,item->changeText(),item->changeSelMode(),
                         startPos.line,
                         endPos.line);
            if (item->changeReason() == ChangeReason::DeleteChar
                    || item->changeReason() == ChangeReason::MergeWithNextLine)
                setCaretPos(item->changeStartPos());
            else
                setCaretPos(item->changeEndPos());
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
            CharPos pos = item->changeStartPos();
            QString tmpStr = mDocument->getLine(pos.line);

            if (shouldInsertAfterCurrentLine(pos.line,tmpStr,s,true)) {
                properDeleteLine(pos.line+1, false);
            } else {
                properDeleteLine(pos.line, false);
            }
            properSetLine(pos.line, tmpStr+s, true);
            setCaretPos(pos);
            mRedoList->addRedo(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        item->changeText(),
                        item->changeSelMode(),
                        item->changeNumber());
            break;
        }
        case ChangeReason::DuplicateLine:{
            QString s;
            properDeleteLine(item->changeStartPos().line+1,true);
            setCaretPos(item->changeStartPos());
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
            mRedoList->addRedo(item);
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
    QSet<size_t> redoedNumbers;

    beginEditing();
    //skip group chain breakers
    while (mRedoList->lastChangeReason()==ChangeReason::GroupBreak) {
        PUndoItem poppedItem = mRedoList->popItem();
        redoedNumbers.insert(poppedItem->changeNumber());
        mUndoList->restoreChange(poppedItem);
    }
    ChangeReason lastChange = mRedoList->lastChangeReason();
    bool keepGoing;
    item = mRedoList->peekItem();
    PCaretAndSelectionInfo endInfo;
    do {
      doRedoItem();
      redoedNumbers.insert(item->changeNumber());
      endInfo = mRedoList->caretAndSelAfterChange(item->changeNumber());
      item = mRedoList->peekItem();
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

    foreach(size_t number, redoedNumbers) {
        mUndoList->restoreCaretAndSelInfos(
                    number,
                    mRedoList->caretAndSelBeforeChange(number),
                    mRedoList->caretAndSelAfterChange(number));
        mRedoList->removeCaretAndSelInfo(number);
    }
    setActiveSelectionMode(endInfo->selMode);
    setCaretAndSelection(endInfo->caret,endInfo->selBegin,endInfo->selEnd);
    updateModifiedStatusForUndoRedo();    
    onChanged();
    endEditing();
}

void QSynEdit::doRedoItem()
{
    mUndoing = true;
    bool changeScrollPastEol = ! mOptions.testFlag(EditorOption::ScrollPastEol);
    mOptions.setFlag(EditorOption::ScrollPastEol);
    mUndoList->setInsideRedo(true);
    beginEditing();
    auto action = finally([&,this]{
        endEditing();
        mUndoList->setInsideRedo(false);
        if (changeScrollPastEol)
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
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(),
                        mActiveSelectionMode,
                        item->changeNumber());
            //setCaretPos(item->changeStartPos());
            break;
        case ChangeReason::LeftTop:
        {
            CharPos p;
            p.ch = leftPos();
            p.line = topPos();
            mUndoList->restoreChange(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(),
                        item->changeSelMode(),
                        item->changeNumber());
            setLeftPos(item->changeStartPos().ch);
            setTopPos(item->changeStartPos().line);
        }
            break;
        case ChangeReason::Selection:
            mUndoList->restoreChange(
                        item->changeReason(),
                        item->changeStartPos(),
                        item->changeEndPos(),
                        QStringList(),
                        mActiveSelectionMode,
                        item->changeNumber());
//            setCaretAndSelection(
//                        caretPos(),
//                        item->changeStartPos(),
//                        item->changeEndPos());
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
            setCaretPos(fileBegin());
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
        case ChangeReason::Input: {
            doUndoRedoInput(item->changeText()[0],
                          item->changeStartPos(),
                          item->changeEndPos(),
                          item->changeSelMode(),
                          item->changeNumber(),
                    false);
            break;
        }
        case ChangeReason::Insert:
            setCaretAndSelection(
                        item->changeStartPos(),
                        item->changeStartPos(),
                        item->changeStartPos());
            doInsertText(item->changeStartPos(),item->changeText(), item->changeSelMode(),
                         item->changeStartPos().line,
                         item->changeEndPos().line);
            setCaretPos(item->changeEndPos());
            mUndoList->restoreChange(item->changeReason(),
                                 item->changeStartPos(),
                                 item->changeEndPos(),
                                 QStringList(),
                                 item->changeSelMode(),
                                 item->changeNumber());
            break;
        case ChangeReason::MergeWithNextLine:
            setCaretPos(item->changeStartPos());
            doMergeWithNextLine();
            mUndoList->restoreChange(item->changeReason(), item->changeStartPos(),
                                 item->changeEndPos(),item->changeText(),
                                 item->changeSelMode(),item->changeNumber());
            break;
        case ChangeReason::MergeWithPrevLine:
            setCaretPos(item->changeEndPos());
            doMergeWithPrevLine();
            mUndoList->restoreChange(item->changeReason(), item->changeStartPos(),
                                 item->changeEndPos(),item->changeText(),
                                 item->changeSelMode(),item->changeNumber());
            break;
        case ChangeReason::DeleteChar:
        case ChangeReason::DeletePreviousChar:
        case ChangeReason::Delete:
            doDeleteText(item->changeStartPos(),item->changeEndPos(),item->changeSelMode());
            mUndoList->restoreChange(item->changeReason(), item->changeStartPos(),
                                 item->changeEndPos(),item->changeText(),
                                 item->changeSelMode(),item->changeNumber());
            setCaretPos(item->changeStartPos());
            break;
        case ChangeReason::LineBreak: {
            CharPos caret = item->changeStartPos();
            mUndoList->restoreChange(item->changeReason(), item->changeStartPos(),
                                 item->changeEndPos(),item->changeText(),
                                 item->changeSelMode(),item->changeNumber());
            setCaretAndSelection(caret, caret, caret);
            doBreakLine();
            break;
        }
        case ChangeReason::DuplicateLine: {
            mUndoList->restoreChange(item->changeReason(), item->changeStartPos(),
                                 item->changeEndPos(),item->changeText(),
                                 item->changeSelMode(),item->changeNumber());
            setCaretPos(item->changeStartPos());
            doDuplicateCurrentLine();
            break;
        }
        default:
            mUndoList->restoreChange(item);
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
    if (!hasSelContent()) {
        return "";
    } else {
        int firstLine = selBegin().line;
        int lastLine = selEnd().line;

        switch(mActiveSelectionMode) {
        case SelectionMode::Normal:{
            int charFrom = selBegin().ch;
            int charTo = selEnd().ch;
//            PCodeFoldingRange foldRange = foldStartAtLine(selEnd().line);
//            QString s = mDocument->getLine(lastLine);
//            if ((foldRange) && foldRange->collapsed && charTo>s.length()) {
//                s=s+mSyntaxer->foldString(s);
//                if (charTo>s.length()) {
//                    lastLine = foldRange->toLine;
//                    charTo = mDocument->getLine(lastLine).length();
//                }
//            }
            if (firstLine == lastLine)
                return  mDocument->getLine(firstLine).mid(charFrom, charTo - charFrom);
            else {
                QString result = mDocument->getLine(firstLine).mid(charFrom);
                result+= lineBreak();
                for (int i = firstLine + 1; i<=lastLine - 1; i++) {
                    result += mDocument->getLine(i);
                    result+=lineBreak();
                }
                const QString &lineText = mDocument->getLine(lastLine);
                result.append(lineText.constData(), charTo);
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

QStringList QSynEdit::selContent() const
{
    QStringList result;
    if (!hasSelContent()) {
        return result;
    } else {
        int firstLine = selBegin().line;
        int lastLine = selEnd().line;

        switch(mActiveSelectionMode) {
        case SelectionMode::Normal:{
            int charFrom = selBegin().ch;
            int charTo = selEnd().ch;
            if (firstLine == lastLine) {
                result.append(mDocument->getLine(firstLine).mid(charFrom, charTo - charFrom));
                return result;
            } else {
                result.append(mDocument->getLine(firstLine).mid(charFrom));
                for (int i = firstLine + 1; i<=lastLine - 1; i++) {
                    result += mDocument->getLine(i);
                }
                const QString &lineText = mDocument->getLine(lastLine);
                result.append(lineText.left(charTo));
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
            for (int i = firstLine; i <= lastLine; i++) {
                int l = xposToGlyphStartChar(i,xFrom);
                int r = xposToGlyphStartChar(i,xTo);
                QString s = mDocument->getLine(i);
                result.append(s.mid(l,r-l));
            }
            return result;
        }
        }
    }
    return result;
}

int QSynEdit::selCount() const
{
    if (!hasSelContent())
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
//        PCodeFoldingRange foldRange = foldStartAtLine(endPos.line);
//        QString s = mDocument->getLine(lastLine);
//        if ((foldRange) && foldRange->collapsed && chTo>s.length()) {
//            s=s+mSyntaxer->foldString(s);
//            if (chTo>s.length()) {
//                lastLine = foldRange->toLine;
//                chTo = s.length();
//            }
//        }
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

void QSynEdit::saveToFile(QFile &file, const QByteArray &encoding, QByteArray &realEncoding) const
{
    mDocument->saveToFile(file,encoding,realEncoding);
}

QString QSynEdit::displayLineText()
{
    return getDisplayStringAtLine(mCaretLine);
}

QString QSynEdit::lineText() const
{
    return mDocument->getLine(mCaretLine);
}

QString QSynEdit::lineText(int line) const
{
    return mDocument->getLine(line);
}

size_t QSynEdit::lineSeq(int line) const
{
    return mDocument->getLineSeq(line);
}

PSyntaxState QSynEdit::lineSyntaxState(int line) const
{
    return mDocument->getSyntaxState(line);
}

bool QSynEdit::findLineTextBySeq(size_t lineSeq,  QString& text) const
{
    PDocumentLine line = mDocument->findLineBySeq(lineSeq);
    if (!line)
        return false;

    Q_ASSERT(line->lineSeq() == lineSeq);
    text = line->lineText();
    return true;
}

const PConstSyntaxer QSynEdit::syntaxer() const
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

bool QSynEdit::isSpaceChar(const QChar &ch) const
{
    return mSyntaxer->isSpaceChar(ch);
}

void QSynEdit::startParseLine(Syntaxer *syntaxer, int lineIndex) const
{
    Q_ASSERT(validLine(lineIndex));
    if (lineIndex == 0) {
        syntaxer->resetState();
    } else {
        syntaxer->setState(mDocument->getSyntaxState(lineIndex-1));
    }
    syntaxer->setLine(lineIndex, mDocument->getLine(lineIndex), mDocument->getLineSeq(lineIndex));
}

void QSynEdit::startParseLine(Syntaxer *syntaxer, int lineIndex, const QString lineText) const
{
    Q_ASSERT(validLine(lineIndex));
    if (lineIndex == 0) {
        syntaxer->resetState();
    } else {
        syntaxer->setState(mDocument->getSyntaxState(lineIndex-1));
    }
    syntaxer->setLine(lineIndex, lineText, mDocument->getLineSeq(lineIndex));
}


void QSynEdit::moveCaretHorz(int deltaX, bool isSelection)
{
    CharPos dstPos = caretPos();
    QString s = lineText();
    int lineLen = s.length();
    if (!isSelection && selAvail() && (deltaX!=0)) {
        if (deltaX<0)
            dstPos = selBegin();
        else
            dstPos = selEnd();
    } else {
        if (mOptions.testFlag(EditorOption::AltSetsColumnMode)) {
            if (qApp->keyboardModifiers().testFlag(Qt::AltModifier) && !mReadOnly) {
                setActiveSelectionMode(SelectionMode::Column);
            } else
                setActiveSelectionMode(SelectionMode::Normal);
        }

        // only moving or selecting one char can change the line
        //bool changeY = !mOptions.testFlag(SynEditorOption::eoScrollPastEol);
        bool changeY=true;
        int glyphIndex = mDocument->charToGlyphIndex(dstPos.line,dstPos.ch);
        if (changeY && (deltaX == -1) && (glyphIndex==0) && (dstPos.line > 0)) {
            // end of previous line
            if (mActiveSelectionMode==SelectionMode::Column) {
                return;
            }
            int row = lineToRow(dstPos.line);
            row--;
            int line = rowToLine(row);
            if (line!=dstPos.line && line>=0) {
                dstPos.line = line;
                dstPos.ch = getDisplayStringAtLine(dstPos.line).length();
            }
        } else if (changeY && (deltaX == 1) && (glyphIndex >= mDocument->glyphCount(dstPos.line)) && (dstPos.line < mDocument->count()-1)) {
            // start of next line
            if (mActiveSelectionMode==SelectionMode::Column) {
                return;
            }
            int row = lineToRow(dstPos.line);
            row++;
            int line = rowToLine(row);
            if (line!=dstPos.line && line<=mDocument->count()) {
                dstPos.line = line;
                dstPos.ch = 0;
            }
        } else {
            dstPos.ch = std::max(0, mDocument->glyphStartChar(dstPos.line, glyphIndex + deltaX));
            // don't go past last char when ScrollPastEol option not set
            if ((deltaX > 0) && changeY)
              dstPos.ch = std::min(dstPos.ch, lineLen);
        }
    }
    // set caret and block begin / end
    beginInternalChanges();
    //ensureCaretVisible();
    moveCaretAndSelection(caretPos(), dstPos, isSelection);
    endInternalChanges();
}

void QSynEdit::moveCaretVert(int deltaY, bool isSelection)
{
    DisplayCoord originCoord = displayXY();
    DisplayCoord dstCoord = originCoord;
    if (!isSelection && selAvail()) {
        if (deltaY<0)
            dstCoord = bufferToDisplayPos(selBegin());
        else
            dstCoord = bufferToDisplayPos(selEnd());
    }
    dstCoord.row+=deltaY;

    if (deltaY >= 0) {
        dstCoord.row = std::min(dstCoord.row, displayLineCount()-1);
    } else {
        dstCoord.row = std::max(dstCoord.row, 0);
    }

    if (originCoord.row != dstCoord.row) {
        if (mOptions.testFlag(EditorOption::KeepCaretX))
            dstCoord.x = mLastCaretX;
    }
    if (mOptions.testFlag(EditorOption::AltSetsColumnMode)) {
        if (qApp->keyboardModifiers().testFlag(Qt::AltModifier) && !mReadOnly)
            setActiveSelectionMode(SelectionMode::Column);
        else
            setActiveSelectionMode(SelectionMode::Normal);
    }

    CharPos dstPos;
    if (dstCoord.row == originCoord.row && isSelection && deltaY!=0) {
        if (dstCoord.row==0 && mDocument->count()>1) {
            dstPos = fileBegin();
        } else {
            dstPos = fileEnd();
        }
    } else
        dstPos = displayToBufferPos(dstCoord);

    if (mActiveSelectionMode==SelectionMode::Column) {
        int w=mDocument->lineWidth(dstPos.line);
        if (w+1<originCoord.x)
            return;
    }

    int saveLastCaretX = mLastCaretX;

    // set caret and block begin / end
    beginInternalChanges();
    //ensureCaretVisible();
    moveCaretAndSelection(caretPos(), dstPos, isSelection);
    endInternalChanges();

    // Restore mLastCaretX after moving caret, since
    // updateLastCaretX, called by internalSetCaretPos, changes it. This is the
    // one case where we don't want that.
    mLastCaretX = saveLastCaretX;
}

void QSynEdit::moveCaretAndSelection(const CharPos &before, const CharPos &after, bool isSelection, bool ensureCaretVisible)
{
    beginInternalChanges();
    if (isSelection) {
        if (!selAvail())
          internalSetSelBegin(before);
        internalSetSelEnd(after);
    } else
        internalSetSelBegin(after);
    internalSetCaretPos(after,ensureCaretVisible);
    endInternalChanges();
}

void QSynEdit::moveCaretToLineStart(bool isSelection)
{
    int newX;
    // home key enhancement
    if (mOptions.testFlag(EditorOption::EnhanceHomeKey)) {
        QString s = mDocument->getLine(mCaretLine);

        int first_nonblank = 0;
        int maxX = s.length();
        while ((first_nonblank < maxX) && (s[first_nonblank] == ' ' || s[first_nonblank] == '\t')) {
            first_nonblank++;
        }
        newX = mCaretCh;

        if ((newX > first_nonblank)
                || (newX == 0))
            newX = first_nonblank;
        else
            newX = 0;
    } else
        newX = 0;
    moveCaretAndSelection(caretPos(), CharPos{newX, mCaretLine}, isSelection);
}

void QSynEdit::moveCaretToLineEnd(bool isSelection, bool ensureCaretVisible)
{
    int newX;
    if (mOptions.testFlag(EditorOption::EnhanceEndKey)) {
        QString text = lineText();
        int lastNonBlank = text.length()-1;
        int minX = 0;
        while ((lastNonBlank >= minX) && (text[lastNonBlank] == ' ' || text[lastNonBlank] =='\t'))
            lastNonBlank--;
        newX = mCaretCh;
        if ((newX <= lastNonBlank) || (newX == text.length()))
            newX = lastNonBlank+1;
        else
            newX = text.length();
    } else
        newX = lineText().length();
    moveCaretAndSelection(caretPos(), CharPos{newX, mCaretLine}, isSelection, ensureCaretVisible);
}

void QSynEdit::doGotoBlockStart(bool isSelection)
{
    //todo: handle block other than {}
    CharPos blockStart;
    CharPos currentPos = caretPos();
    if (currentPos.ch>0) {
        CharPos prevPos{currentPos.ch-1,currentPos.line};
        if (charAt(prevPos)=='{')
            currentPos = prevPos;
    }
    if (getContainingBlockStart(currentPos, blockStart)) {
        blockStart.ch++;
        if (!isSelection)
            setCaretPos(blockStart);
        else
            setCaretAndSelection(blockStart, blockStart, caretPos());
    }
}

void QSynEdit::doGotoBlockEnd(bool isSelection)
{
    //todo: handle block other than {}
    CharPos blockEnd;
    CharPos currentPos = caretPos();
    if (charAt(currentPos)=='}')
        currentPos.ch++;
    if (getContainingBlockEnd(currentPos, blockEnd)) {
        if (!isSelection)
            setCaretPos(blockEnd);
        else
            setCaretAndSelection(blockEnd, blockEnd, caretPos());
    }
}

void QSynEdit::doGotoEditorStart(bool isSelection)
{
    CharPos newPos{fileBegin()};
    if (!isSelection)
        setCaretPos(newPos);
    else
        setCaretAndSelection(newPos, newPos, caretPos());
}

void QSynEdit::doGotoEditorEnd(bool isSelection)
{
    CharPos newPos{fileEnd()};
    if (!isSelection)
        setCaretPos(newPos);
    else
        setCaretAndSelection(newPos, newPos, caretPos());
}

void QSynEdit::doDeleteSelection()
{
    if (readOnly())
        return;
    CharPos startPos=selBegin();
    CharPos endPos=selEnd();
    doDeleteText(startPos,endPos,mActiveSelectionMode);
}

void QSynEdit::doSetSelTextPrimitive(const QStringList &text)
{
    if (readOnly())
        return;
    SelectionMode mode = mActiveSelectionMode;
    beginInternalChanges();
    bool groupUndo=false;
    CharPos startPos = selBegin();
    CharPos endPos = selEnd();
    bool selChanged = false;
    if (hasSelContent()) {
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
                internalSetCaretPos(startPos);
                x=xBegin;
            } else {
                internalSetCaretPos(endPos);
                x=xEnd;
            }
            startPos.ch = xposToGlyphStartChar(startPos.line, x);
            endPos.ch = xposToGlyphStartChar(endPos.line, x);
            internalSetSelBegin(startPos);
            internalSetSelEnd(endPos);
        } else
            setCaretPos(startPos);
        selChanged = true;
    }
    if (!text.isEmpty()) {
        doInsertText(caretPos(),text,mode,mSelectionBegin.line,mSelectionEnd.line);
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
    if (hasSelContent()) {
      beginEditing();
      blockBeginned = true;
    }
    CharPos startOfBlock = selBegin();
    CharPos endOfBlock = selEnd();
    mSelectionBegin = startOfBlock;
    mSelectionEnd = endOfBlock;
    doSetSelTextPrimitive(splitStrings(value));
}

int QSynEdit::searchReplace(const QString &searchText, const QString &replaceWith,
                            const CharPos & scopeBegin,
                            const CharPos & scopeEnd,
                            CharPos& newScopeEnd,
                            SearchOptions options, Searcher *searchEngine,
                    SearchMatchedProc matchedCallback, SearchConfirmAroundProc confirmAroundCallback)
{
    if (!searchEngine)
        return 0;

    Q_ASSERT(scopeBegin<=scopeEnd);
    Q_ASSERT(validInDoc(scopeBegin));
    Q_ASSERT(validInDoc(scopeEnd));
    // can't search for or replace an empty string
    if (searchText.isEmpty()) {
        return 0;
    }
    int result = 0;

    bool backwards = options.testFlag(ssoBackwards);
    bool searchInWholeDoc = (
                scopeBegin == fileBegin()
                && scopeEnd == fileEnd());
    // from caret only in effect if searching in whole document
    bool fromCaret = options.testFlag(ssoFromCaret) && searchInWholeDoc;
    bool wrapped = false;
    CharPos posCurrent;
    CharPos posBegin = scopeBegin;
    CharPos posEnd = scopeEnd;
    newScopeEnd = scopeEnd;
    if (fromCaret) {
        if (selBegin() == caretPos() || selEnd() == caretPos()) {
            if (options.testFlag(ssoIncludeCurrentSelection)) {
                if (backwards)
                    posEnd = std::max(selBegin(), selEnd());
                else
                    posBegin = std::min(selBegin(), selEnd());
            } else {
                if (backwards)
                    posEnd = std::min(selBegin(), selEnd());
                else
                    posBegin = std::max(selBegin(), selEnd());
            }
        } else {
            if (backwards)
                posEnd = caretPos();
            else
                posBegin = caretPos();
        }
    }
    if (backwards)
        posCurrent = posEnd;
    else
        posCurrent = posBegin;
    CharPos origCurrent{posCurrent};
    // initialize the search engine
    searchEngine->setOptions(options);
    searchEngine->setPattern(searchText);
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
        while ((posCurrent.line >= posBegin.line) && (posCurrent.line <= posEnd.line)) {
            int countInLine = searchEngine->findAll(mDocument->getLine(posCurrent.line));
            int resultOffset = 0;
            int totalLineOffset = 0;
            if (backwards)
                i = searchEngine->resultCount()-1;
            else
                i = 0;
            // Operate on all results in this line.
            QSet<int> tokenBorders;
            if (countInLine>0 && options.testFlag(ssoWholeWord)) {
                tokenBorders = getTokenBorders(posCurrent.line);
            }
            while (countInLine > 0) {
                // An occurrence may have been replaced with a text of different length
                int chFound = searchEngine->result(i) + resultOffset;
                int searchLen = searchEngine->length(i);
                if (backwards)
                    i--;
                else
                    i++;
                countInLine--;
                // Is the search result entirely in the search range?
                bool isInValidSearchRange = true;
                int first = chFound;
                int last = chFound + searchLen;
                if  ((searchLen==0) &&
                     (((posCurrent.line == posBegin.line) && (first == posBegin.ch) && !backwards)
                      ||  ((posCurrent.line == posEnd.line) && (last == posEnd.ch) && backwards))
                     ) {
                    isInValidSearchRange = false;
                } else if (((posCurrent.line == posBegin.line) && (first < posBegin.ch)) ||
                        ((posCurrent.line == posEnd.line) && (last > posEnd.ch))) {
                    isInValidSearchRange = false;
                }
                if (!isInValidSearchRange)
                    continue;
                if (options.testFlag(ssoWholeWord) && first != last) {
                    if (!tokenBorders.contains(first)
                            || !tokenBorders.contains(last))
                        continue;
                }
                result++;
                // Select the text, so the user can see it in the OnReplaceText event
                // handler or as the search result.
                posCurrent.ch = chFound;
                internalSetSelBegin(posCurrent);
                posCurrent.ch += searchLen;
                internalSetSelEnd(posCurrent);

                if (backwards)
                    internalSetCaretPos(selBegin());
                else
                    internalSetCaretPos(selEnd());

                QString replaceText = searchEngine->replace(selText(), replaceWith);
                if (searchAction == SearchAction::ReplaceAndExit) {
                    searchAction = SearchAction::Exit;
                } else if (searchAction != SearchAction::ReplaceAll
                        && matchedCallback && !dobatchReplace) {
                    searchAction = matchedCallback(
                                selText(),replaceText,
                                CharPos{chFound,posCurrent.line},
                                searchLen);
                }
                if (searchAction == SearchAction::Exit) {
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
                    int replaceLen = replaceText.length();
                    totalLineOffset += replaceLen - searchLen;
                    // fix the caret position and the remaining results
                    if (backwards) {
                        setCaretCh(chFound);
                    } else {
                        if ((searchLen != replaceLen)) {
                            resultOffset += replaceLen - searchLen;
                            if (posEnd != fileEnd()  && posCurrent.line == posEnd.line) {
                                posEnd.ch += replaceLen - searchLen;
                            }
                        }
                    }
                    if (searchLen != replaceLen && newScopeEnd.line == posCurrent.line)
                        newScopeEnd.ch += replaceLen - searchLen;
                    mOptions.setFlag(EditorOption::AutoIndent,oldAutoIndent);
                }
            }
            if (fromCaret && totalLineOffset!=0 && origCurrent.line==posCurrent.line) {
                if (backwards != wrapped)
                    origCurrent.ch += totalLineOffset;
            }

            // search next / previous line
            if (backwards) {
                posCurrent.line--;
            } else {
                posCurrent.line++;
            }
            if ((posCurrent.line < posBegin.line) || (posCurrent.line > posEnd.line)) {
                if (fromCaret && !wrapped){ //wrap around, should happen only once
                    if (!options.testFlag(ssoWrapAround)) {
                        break;
                    } else {
                        if (confirmAroundCallback && !confirmAroundCallback())
                            break;
                    }
                    //search start from cursor, search has finished but no result founds
                    wrapped = true;
                    posBegin = scopeBegin;
                    posEnd = scopeEnd;
                    if (backwards) {
                        posBegin = origCurrent;
                        posCurrent = posEnd;
                    } else {
                        posEnd= origCurrent;
                        posCurrent = posBegin;
                    }
                } else
                    break;
            }
        }
    }
    return result;
}

void QSynEdit::properSetLine(int line, const QString &lineText, bool parseToEnd)
{
    mDocument->putLine(line,lineText);
    reparseLines(line, line + 1, parseToEnd && mSyntaxer->needsLineState());
}

void QSynEdit::properInsertLine(int line, const QString &lineText, bool parseToEnd)
{
    mDocument->insertLine(line, lineText);
    processCodeBlocksOnLinesInserted(line,1);
    reparseLines(line,line+1, parseToEnd && mSyntaxer->needsLineState());
    emit linesInserted(line, 1);
    updateVScrollbar();
    //we must invalidate whole editor to properly render contents
    invalidateLines(line,INT_MAX);
}

void QSynEdit::properDeleteLines(int line, int count, bool parseToEnd)
{
    if (count<=0)
        return;
    mDocument->deleteLines(line, count);
    processFoldsOnLinesDeleted(line, count);
    // Unlike properInsertLine(s)/properSetLine/properMoveLine, deletion has no
    // new lines to parse: with a line-local syntaxer the shifted lines' states
    // stay valid, so the reparse can be skipped entirely.
    if (parseToEnd && mSyntaxer->needsLineState())
        reparseLines(line, line + count, true);
    emit linesDeleted(line,count);
    updateVScrollbar();

    //we must invalidate whole editor to properly render contents
    invalidateLines(line,INT_MAX);
}

void QSynEdit::properInsertLines(int line, int count, bool parseToEnd)
{
    if (count<=0)
        return;
    mDocument->insertLines(line, count);
    processCodeBlocksOnLinesInserted(line, count);
    reparseLines(line,line+count, parseToEnd && mSyntaxer->needsLineState());
    emit linesInserted(line, count);
    updateVScrollbar();
    //we must invalidate whole editor to properly render contents
    invalidateLines(line,INT_MAX);
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
    updateVScrollbar();
}

void QSynEdit::doDeleteText(CharPos startPos, CharPos endPos, SelectionMode mode)
{
    Q_ASSERT(validInDoc(startPos));
    Q_ASSERT(validInDoc(endPos));
    Q_ASSERT(endPos >= startPos);
    if (mReadOnly || mDocument->empty())
        return;
    if (startPos == endPos)
        return;
//    if (mode == SelectionMode::Normal) {
//        PCodeFoldingRange foldRange = foldStartAtLine(endPos.line);
//        QString s = mDocument->getLine(endPos.line);
//        if ((foldRange) && foldRange->collapsed && endPos.ch>=s.length()) {
//            QString newS=s+mSyntaxer->foldString(s);
//            if ((startPos.ch<s.length() || startPos.line<endPos.line)
//                    && endPos.ch>=newS.length() ) {
//                //selection has whole block
//                endPos.line = foldRange->toLine;
//                endPos.ch = mDocument->getLine(endPos.line).length();
//            } else {
//                return;
//            }
//        }
//    }
    QStringList deleted=getContent(startPos,endPos,mode);
    beginEditing();
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
            properSetLine(startPos.line,newString,true);
            setCaretPos(startPos);
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
        // keep the column mode: deletion leaves a zero-width column
        // selection (i.e. a column cursor)
        beginInternalChanges();
        internalSetCaretPos(newStartPos);
        internalSetSelBegin(newStartPos);
        internalSetSelEnd(newEndPos);
        endInternalChanges();
        // Column deletion never removes a line entirely, so no mark
        // updating is needed here.
        break;
    }
    }
    addChangeToUndo(ChangeReason::Delete,
            startPos,
            endPos,
            deleted,
            mode);
    endEditing();
}

void QSynEdit::doInsertText(const CharPos& pos,
                           const QStringList& text,
                           SelectionMode mode, int startLine, int endLine) {
    if (text.isEmpty())
        return;
    if (startLine>endLine)
        std::swap(startLine,endLine);
//    if (mode == SelectionMode::Normal) {
//        PCodeFoldingRange foldRange = foldStartAtLine(pos.line);
//        QString s = mDocument->getLine(pos.line);
//        if ((foldRange) && foldRange->collapsed && pos.ch>s.length()+1)
//            return;
//    }
    CharPos newPos;
    switch(mode){
    case SelectionMode::Normal:
        doInsertTextByNormalMode(pos,text);
        break;
    case SelectionMode::Column:{
        CharPos blockBegin=selBegin();
        CharPos blockEnd=selEnd();
        int lenBefore = mDocument->getLine(blockEnd.line).length();
        doInsertTextByColumnMode(pos, text, startLine,endLine);
        if (!text.isEmpty()) {
            int textLen = mDocument->getLine(blockEnd.line).length()-lenBefore;
            blockBegin.ch+=textLen;
            blockEnd.ch+=textLen;
            // keep the column mode
            beginInternalChanges();
            internalSetCaretPos(blockBegin);
            internalSetSelBegin(blockBegin);
            internalSetSelEnd(blockEnd);
            endInternalChanges();
        }
    }
        break;
    }

}

void QSynEdit::doInsertTextByNormalMode(CharPos pos, const QStringList& text)
{
    Q_ASSERT(validInDoc(pos));
    QString leftSide;
    QString rightSide;
    QString str;
    bool changeScroll;
    beginEditing();
    QString lineText = mDocument->getLine(pos.line);
    leftSide = lineText.left(pos.ch);
    rightSide = lineText.mid(pos.ch);
    int currentLine=pos.line;
    // step1: insert the first line of text into current line
    if (text.length()>1) {
        if (!mUndoing) {
            QString s = text[0];
            if (leftSide.trimmed().isEmpty() && shouldRecalcIndent(currentLine)) {
                s=s.trimmed();
                addChangeToUndo(ChangeReason::Delete,
                                {0,pos.line},
                                {pos.ch,pos.line},
                                QStringList({leftSide}),
                                SelectionMode::Normal
                                );
                leftSide = genSpaces(calcIndentSpaces(currentLine,s,true));
                pos.ch = leftSide.length();
                addChangeToUndo(ChangeReason::Insert,
                                {0,pos.line},
                                {pos.ch,pos.line},
                                QStringList({}),
                                SelectionMode::Normal
                                );
            }
            str = leftSide + s;
        } else
            str = leftSide + text[0];
        if (leftSide.trimmed().isEmpty()) {
            properInsertLines(currentLine, text.length()-1, false);
        } else {
            properInsertLines(currentLine+1, text.length()-1, false);
        }
        properSetLine(currentLine, str, false);
        // step2: insert remaining lines of text
        for (int i=1;i<text.length();i++) {
            currentLine=pos.line+i;
            if (text[i].isEmpty()) {
                if (i==text.length()-1) {
                    str = rightSide;
                } else {
                    str = "";
                }
            } else {
                str = text[i];
                if (i==text.length()-1)
                    str += rightSide;
            }
            if (!mUndoing && mSyntaxer->language()==ProgrammingLanguage::CPP
                    && shouldRecalcIndent(currentLine)) {
                int indentSpaces = calcIndentSpaces(currentLine,str,true);
                str = genSpaces(indentSpaces)+trimLeft(str);
            }
            properSetLine(currentLine, str, i==text.length()-1);
        }
    } else {
        if (!mUndoing) {
            QString s = text[0];
            if (leftSide.trimmed().isEmpty() && shouldRecalcIndent(currentLine)) {
                s=s.trimmed();
                addChangeToUndo(ChangeReason::Delete,
                                {0,pos.line},
                                {pos.ch,pos.line},
                                QStringList({leftSide}),
                                SelectionMode::Normal
                                );
                leftSide = genSpaces(calcIndentSpaces(currentLine,s,true));
                pos.ch = leftSide.length();
                addChangeToUndo(ChangeReason::Insert,
                                {0,pos.line},
                                {pos.ch,pos.line},
                                QStringList({}),
                                SelectionMode::Normal
                                );
            }
            str = leftSide + s + rightSide;
        } else
            str = leftSide + text[0] + rightSide;
        properSetLine(currentLine, str, true);
    }

    changeScroll = !mOptions.testFlag(EditorOption::ScrollPastEol);
    mOptions.setFlag(EditorOption::ScrollPastEol);
    auto action = finally([&,this]{
        if (changeScroll)
            mOptions.setFlag(EditorOption::ScrollPastEol,false);
    });
    CharPos newPos=CharPos{(int)str.length() - (int)rightSide.length(),currentLine};
    //onLinesPutted(startLine-1,result+1);
    addChangeToUndo(ChangeReason::Insert,
            pos,newPos,
            QStringList(),SelectionMode::Normal);
    setCaretPos(newPos);
    endEditing();
}

void QSynEdit::doInsertTextByColumnMode(const CharPos& pos, const QStringList& text, int startLine, int endLine)
{
    QString str;
    QString tempString;
    int line;
    CharPos  lineBreakPos;
    // pixel x of the insertion column, consistent with doDeleteText's column branch
    int startX = charToGlyphLeft(pos.line,pos.ch);
    line = startLine;
    if (!mUndoing) {
        beginEditing();
    }
    int i=0;
    while(line<=endLine) {
        str = text[i];
        int insertPos = 0;
        if (line > mDocument->count()) {
            tempString = QString(startX/mCharWidth,' ') + str;
            properInsertLine(mDocument->count(), tempString, line==endLine);
            if (!mUndoing) {
                lineBreakPos.line = line - 1;
                lineBreakPos.ch = mDocument->getLine(line - 2).length() + 1;
                addChangeToUndo(ChangeReason::LineBreak,
                                 lineBreakPos,
                                 lineBreakPos,
                                 QStringList(), SelectionMode::Normal);
            }
        } else {
            tempString = mDocument->getLine(line);
            int lineWidth = mDocument->lineWidth(line);
            if (lineWidth < startX) {
                insertPos = tempString.length();
                tempString = tempString + QString((startX - lineWidth)/mCharWidth,' ') + str;
            } else {
                insertPos = xposToGlyphStartChar(line,startX);
                tempString.insert(insertPos,str);
            }
            properSetLine(line, tempString, line==endLine);
        }
        // Add undo change here from PasteFromClipboard
        if (!mUndoing) {
            addChangeToUndo(ChangeReason::Insert,
                    CharPos{insertPos, line},
                    CharPos{insertPos+(int)str.length(), line},
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
    return;
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

void QSynEdit::processCommand(EditCommand command, QVariant commandData,QVariant extraCommandData)
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
        int gap = lineToRow(caretLine()) * mTextHeight - topPos();
        moveCaretVert(counter, command == EditCommand::SelPageUp || command == EditCommand::SelPageDown);
        setTopPos(lineToRow(caretLine()) * mTextHeight - gap);
        endInternalChanges();
        break;
    }
    case EditCommand::PageTop:
    case EditCommand::SelPageTop:
        moveCaretVert(yposToRow(0)-lineToRow(mCaretLine), command == EditCommand::SelPageTop);
        break;
    case EditCommand::PageBottom:
    case EditCommand::SelPageBottom:
        moveCaretVert(yposToRow(0) + mLinesInWindow-1-lineToRow(mCaretLine), command == EditCommand::SelPageBottom);
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
        if (commandData.isValid())
            moveCaretAndSelection(caretPos(), commandData.value<CharPos>(), command == EditCommand::SelGotoXY);
        break;
    // word selection
    case EditCommand::PrevWordBegin:
    case EditCommand::SelPrevWordBegin:
    {
        CharPos newCaret = prevWordBegin(caretPos());
        moveCaretAndSelection(caretPos(), newCaret, command == EditCommand::SelPrevWordBegin);
        break;
    }
    case EditCommand::nextWordBegin:
    case EditCommand::SelNextWordBegin:
    {
        CharPos newCaret = nextWordBegin(caretPos());
        moveCaretAndSelection(caretPos(), newCaret, command == EditCommand::SelNextWordBegin);
        break;
    }
    case EditCommand::SelWord:
        selCurrentToken();
        break;
    case EditCommand::SelectAll:
        doSelectAll();
        break;
    case EditCommand::ExpandSelection:
        doExpandSelection(caretPos());
        break;
    case EditCommand::ShrinkSelection:
        doShrinkSelection(caretPos());
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
        doDeleteCurrentLine();
        break;
    case EditCommand::Duplicate:
        doDuplicate();
        break;
    case EditCommand::MoveSelUp:
        doMoveSelUp();
        break;
    case EditCommand::MoveSelDown:
        doMoveSelDown();
        break;
    case EditCommand::ClearAll:
        doClearAll();
        break;
    case EditCommand::LineBreak:
        doBreakLine();
        break;
    case EditCommand::LineBreakAtEnd:
        beginEditing();
        mCaretCh = mDocument->getLine(mCaretLine).length();
        doBreakLine();
        endEditing();
        break;
    case EditCommand::BlockIndent:
        doBlockIndent();
        break;
    case EditCommand::BlockUnindent:
        doBlockUnindent();
        break;
    case EditCommand::Tab:
        doTabKey();
        break;
    case EditCommand::ShiftTab:
        doShiftTabKey();
        break;
    case EditCommand::ImeStr:
    case EditCommand::Input:
        if (commandData.canConvert<QChar>()) {
            doInputStr(commandData.toChar());
        } else
            doInputStr(commandData.toString());
        break;
    case EditCommand::ReplaceLine:
        doReplaceLine(commandData.toInt(), extraCommandData.toString());
        break;
    case EditCommand::InsertMode:
        setInsertMode(true);
        break;
    case EditCommand::OverwriteMode:
        setInsertMode(false);
        break;
    case EditCommand::ToggleMode:
        setInsertMode(!mInserting);
        break;
    case EditCommand::Cut:
        doCutToClipboard();
        break;
    case EditCommand::Copy:
        doCopyToClipboard();
        break;
    case EditCommand::Paste:
        doPasteFromClipboard();
        break;
    case EditCommand::Undo:
        doUndo();
        break;
    case EditCommand::Redo:
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
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()-mCharWidth);
        break;
    case EditCommand::ScrollRight:
        horizontalScrollBar()->setValue(horizontalScrollBar()->value()+mCharWidth);
        break;
    case EditCommand::ScrollUp:
        verticalScrollBar()->setValue(verticalScrollBar()->value()-mTextHeight);
        break;
    case EditCommand::ScrollDown:
        verticalScrollBar()->setValue(verticalScrollBar()->value()+mTextHeight);
        break;
    case EditCommand::MatchBracket:
        {
        CharPos pos = getMatchingBracket();
        if (pos.isValid())
            setCaretPos(pos);
        }
        break;
    case EditCommand::TrimTrailingSpaces:
        doTrimTrailingSpaces();
        break;
    default:
        break;
    }
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
        CharPos pos;
        if (pointToCharLine(p,pos)) {
            if (inSelection(pos)) {
                setCursor(Qt::ArrowCursor);
                return;
            }
        }
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
        int firstRow, lastRow, firstX, lastX;
        // Compute the invalid area in lines / columns.
        // columns
        firstX = mLeftPos;
        if (rcClip.left() > mGutterWidth + 2 )
            firstX += (rcClip.left() - mGutterWidth - 2 ) ;
        lastX = mLeftPos + (rcClip.right() - mGutterWidth - 2);
        // lines
        firstRow = minMax(yposToRow(0) + rcClip.top() / mTextHeight, yposToRow(0), displayLineCount()-1);
        lastRow = minMax(yposToRow(0) + (rcClip.bottom() + mTextHeight - 1) / mTextHeight, 0, displayLineCount()-1);


        QPainter cachePainter(mContentImage.get());
        cachePainter.setFont(font());
        QSynEditPainter textPainter(this, &cachePainter,
                                       firstRow,lastRow,firstX,lastX);
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
        internalSetSelBegin(caretPos());
        internalSetSelEnd(caretPos());
        event->accept();
    } else {
        EditCommand cmd=TranslateKeyCode(event->key(),event->modifiers());
        if (cmd!=EditCommand::None) {
            processCommand(cmd);
            event->accept();
        } else if (!event->text().isEmpty()) {
            if (event->text().length()==1) {
                QChar c = event->text().at(0);
                if (c=='\t' || c.isPrint()) {
                    processCommand(EditCommand::Input,event->text());
                    event->accept();
                }
            } else  {
                processCommand(EditCommand::Input,event->text());
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
    bool wasSel = false;
    bool startDrag = false;
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
            wasSel = true;
            mMouseDownPos = event->pos();
        }
        computeCaret();
        mStateFlags.setFlag(StateFlag::WaitForDragging,false);
        if (wasSel && mOptions.testFlag(EditorOption::DragDropEditing) && (x >= mGutterWidth + 2)
                && (mActiveSelectionMode == SelectionMode::Normal) && inSelection(displayToBufferPos(pixelsToGlyphPos(x, y))) ) {
            startDrag = true;
        }
        if (startDrag && !mReadOnly) {
            mStateFlags.setFlag(StateFlag::WaitForDragging);
        } else {
            if (event->modifiers() == Qt::ShiftModifier) {
                //BlockBegin and BlockEnd are restored to their original position in the
                //code from above and SetBlockEnd will take care of proper invalidation
                internalSetSelEnd(caretPos());
            } else {
                if (mOptions.testFlag(EditorOption::AltSetsColumnMode)) {
                    if (event->modifiers() == Qt::AltModifier && !mReadOnly)
                        setActiveSelectionMode(SelectionMode::Column);
                    else
                        setActiveSelectionMode(SelectionMode::Normal);
                } else {
                    setActiveSelectionMode(SelectionMode::Normal);
                }
                //Selection mode must be set before calling SetBlockBegin
                internalSetSelBegin(caretPos());
            }
            computeScroll(false);
        }
    }
}

void QSynEdit::mouseReleaseEvent(QMouseEvent *event)
{
    beginInternalChanges();
    QAbstractScrollArea::mouseReleaseEvent(event);
    int x=event->pos().x();

    if (!mMouseMoved && (x < mGutterWidth )) {
        processGutterClick(event);
    }

    if (mStateFlags.testFlag(StateFlag::WaitForDragging) &&
            !mStateFlags.testFlag(StateFlag::DblClicked)) {
        computeCaret();
        if (! (event->modifiers() & Qt::ShiftModifier))
            internalSetSelBegin(caretPos());
        internalSetSelEnd(caretPos());
        mStateFlags.setFlag(StateFlag::WaitForDragging, false);
    }
    mStateFlags.setFlag(StateFlag::DblClicked,false);
    ensureLineAlignedWithTop();
    ensureCaretVisible();
    endInternalChanges();
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
    QPoint mousePos = event->pos();
    if (mousePos.x() >= mGutterWidth) {
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
            invalidateLine(mCaretLine);
    }
    QString s = event->commitString();
    if (!s.isEmpty()) {
        processCommand(EditCommand::ImeStr,s);
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
        mDragCaretSave = caretPos();
        mDragSelBeginSave = selBegin();
        mDragSelEndSave = selEnd();
        CharPos pos = displayToBufferPos(pixelsToNearestGlyphPos(event->pos().x(),
                                                                        event->pos().y()));
        internalSetCaretPos(pos);
        internalSetSelBegin(mDragSelBeginSave);
        internalSetSelEnd(mDragSelEndSave);
        properInsertLine(mDocument->count(),"",true); //add a line to handle drag to the last
        showCaret();
        computeScroll(true);
    }
}

void QSynEdit::dropEvent(QDropEvent *event)
{
    //mScrollTimer->stop();

    CharPos pos = displayToBufferPos(pixelsToNearestGlyphPos(event->pos().x(),
                                                                    event->pos().y()));
    if (
            (event->proposedAction() == Qt::DropAction::CopyAction
             && pos>mDragSelBeginSave && pos<mDragSelEndSave)
             ||
             (event->proposedAction() != Qt::DropAction::CopyAction
             && pos>=mDragSelBeginSave && pos<=mDragSelEndSave)
            ) {
        properDeleteLine(mDocument->count()-1,true);
        ensureLineAlignedWithTop();
        ensureCaretVisible();
        //do nothing if drag onto itself
        event->acceptProposedAction();
        mDropped = true;
        return;
    }
    bool lastLineUsed = (pos.line == mDocument->count()-1);
    int topPos = mTopPos;
    int leftPos = mLeftPos;
    QStringList text=splitStrings(event->mimeData()->text());
    //let beginEditing record old position;
    setCaretAndSelection(mDragCaretSave, mDragSelBeginSave, mDragSelEndSave);
    beginEditing();
    if (lastLineUsed) {
        int line=mDocument->count()-2;
        QString s=mDocument->getLine(line);
        addChangeToUndo(ChangeReason::LineBreak,
                         CharPos{(int)s.length(),line},
                         CharPos{(int)s.length(),line}, QStringList(), SelectionMode::Normal);
    } else
        properDeleteLine(mDocument->count()-1, true);
    addLeftTopToUndo();
    internalSetCaretPos(pos);
    if (event->proposedAction() == Qt::DropAction::CopyAction) {
        //just copy it
        doInsertText(pos,text,mActiveSelectionMode,pos.line,pos.line+text.length()-1);
    } else if (event->proposedAction() == Qt::DropAction::MoveAction)  {
        if (pos < mDragSelBeginSave ) {
            //delete old
            doDeleteText(mDragSelBeginSave,mDragSelEndSave,mActiveSelectionMode);
            //paste to new position
            doInsertText(pos,text,mActiveSelectionMode,pos.line,pos.line+text.length()-1);
        } else {
            if (pos.line>mDocument->count()) {
                int line=mDocument->count();
                QString s=mDocument->getLine(line);
                beginEditing();
                properInsertLine(mDocument->count(),"",true);
                addChangeToUndo(ChangeReason::LineBreak,
                                     CharPos{(int)s.length()+1,line},
                                     CharPos{(int)s.length()+1,line}, QStringList(), SelectionMode::Normal);
                endEditing();
                pos.line = line;
                pos.ch=0;
            } else {

            }
            //paste to new position
            doInsertText(pos,text,mActiveSelectionMode,pos.line,pos.line+text.length()-1);
            //delete old
            doDeleteText(mDragSelBeginSave,mDragSelEndSave,mActiveSelectionMode);
            //set caret to right pos
            if (mDragSelBeginSave.line == mDragSelEndSave.line) {
                if (pos.line == mDragSelEndSave.line) {
                    pos.ch -= mDragSelEndSave.ch-mDragSelBeginSave.ch;
                }
            } else {
                if (pos.line == mDragSelEndSave.line) {
                    pos.ch -= mDragSelEndSave.ch-1;
                } else {
                    pos.line -= mDragSelEndSave.line-mDragSelBeginSave.line;
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
    setCaretPos(pos);
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

    QPoint mousePos = QCursor::pos();
    mousePos = mapFromGlobal(mousePos);
    int x=mousePos.x();
    int y=mousePos.y();
    CharPos pos = displayToBufferPos(pixelsToNearestGlyphPos(x,y));
    internalSetCaretPos(pos);
    internalSetSelBegin(mDragSelBeginSave);
    internalSetSelEnd(mDragSelEndSave);
    showCaret();
}

void QSynEdit::dragLeaveEvent(QDragLeaveEvent *)
{
    mDragging = false;
    properDeleteLine(mDocument->count()-1,true);
//    setCaretPos(mDragCaretSave);
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

void QSynEdit::setGutterWidth(int value)
{
    value = std::max(value, 0);
    if (mGutterWidth != value) {
        mGutterWidth = value;
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
    // Note: no column-selection fixup here. It used to re-snap the column
    // selection to the caret's display x, but this slot also fires mid-edit
    // (per line of a column deletion/insertion), where the caret is stale
    // and the line may already be shortened. Editing operations are
    // responsible for the final selection state themselves.
    if (mGutter.showLineNumbers() && (mGutter.autoSize()))
        mGutter.autoSizeDigitCount(mDocument->count());
    setTopPos(mTopPos);
    endInternalChanges();
}

void QSynEdit::onLinesChanging()
{
    beginInternalChanges();
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

void QSynEdit::setActiveSelectionMode(const SelectionMode &value)
{
    if (mActiveSelectionMode != value) {
        if (selAvail())
            invalidateSelection();
        mActiveSelectionMode = value;
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
    internalSetSelBegin(caretPos());
}

void QSynEdit::internalSetSelEnd(const CharPos &value)
{
    Q_ASSERT(validInDoc(value));
    Q_ASSERT(validGlyphStart(value));

    beginInternalChanges();
    if (value.ch != mSelectionEnd.ch || value.line != mSelectionEnd.line) {
        if (mActiveSelectionMode == SelectionMode::Column && value.ch != mSelectionEnd.ch) {
            CharPos oldBlockEnd = mSelectionEnd;
            mSelectionEnd = value;
            invalidateLines(
                        std::min(mSelectionBegin.line, std::min(mSelectionEnd.line, oldBlockEnd.line)),
                        std::max(mSelectionBegin.line, std::max(mSelectionEnd.line, oldBlockEnd.line))+1);
        } else {
            CharPos oldBlockEnd = mSelectionEnd;
            mSelectionEnd = value;
            if (mActiveSelectionMode != SelectionMode::Column || mSelectionBegin.ch != mSelectionEnd.ch) {
                invalidateLines(
                            std::min(mSelectionEnd.line, oldBlockEnd.line),
                            std::max(mSelectionEnd.line, oldBlockEnd.line)+1);
            }
        }
        setStatusChanged(StatusChange::Selection);
    }
    endInternalChanges();
}

void QSynEdit::setSelBeginEnd(const CharPos &beginPos, const CharPos &endPos)
{
    beginInternalChanges();
    internalSetSelBegin(beginPos);
    internalSetSelEnd(endPos);
    endInternalChanges();
}

void QSynEdit::setSelLength(int len)
{
    if (mSelectionBegin.line>=mDocument->count() || mSelectionBegin.line<0)
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
        CharPos newEnd{x,y};
        setCaretAndSelection(newEnd, mSelectionBegin, newEnd);
    } else {
        int y = mSelectionBegin.line;
        int ch = mSelectionBegin.ch;
        int x = len;
        QString lineText = mDocument->getLine(y);
        while (true) {
            if (x<=ch)
                break;
            x-=(ch+1);
            y--;
            if (y<0)
                break;
            lineText = mDocument->getLine(y);
            ch =  lineText.length();
        }
        if (y<0) {
            y = 0;
            x = 0;
        } else {
            x = ch - x;
        }
        CharPos newStart{x,y};
        setCaretAndSelection(newStart, newStart, mSelectionBegin);
    }
}

void QSynEdit::setSelText(const QString &text)
{
    doSetSelText(text);
}

void QSynEdit::doReplaceLine(int line, const QString &lineText)
{
    beginEditing();
    CharPos pos{0,line};
    addChangeToUndo(ChangeReason::ReplaceLine,pos,pos,QStringList(mDocument->getLine(line)),SelectionMode::Normal);
    properSetLine(line, lineText,true);
    setCaretPos(ensureCharPosValid(caretPos()));
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

void QSynEdit::internalSetSelBegin(const CharPos &value)
{
    Q_ASSERT(validInDoc(value));
    Q_ASSERT(validGlyphStart(value));
    beginInternalChanges();
    if (selAvail()) {
        int invalLine1, invalLine2;
        if (mSelectionBegin.line < mSelectionEnd.line) {
            invalLine1 = std::min(value.line, mSelectionBegin.line);
            invalLine2 = std::max(value.line, mSelectionEnd.line);
        } else {
            invalLine1 = std::min(value.line, mSelectionEnd.line);
            invalLine2 = std::max(value.line, mSelectionBegin.line);
        };
        mSelectionBegin = value;
        mSelectionEnd = value;
        invalidateLines(invalLine1, invalLine2+1);
        setStatusChanged(StatusChange::Selection);
    } else {
        mSelectionBegin = value;
        mSelectionEnd = value;
    }
    endInternalChanges();
}

int QSynEdit::leftPos() const
{
    return mLeftPos;
}

void QSynEdit::setLeftPos(int value)
{
    value = std::max(value, 0);
    if (value != mLeftPos) {
        if (mDocument->maxLineWidth()<0)
            mLeftPos = value;
        setStatusChanged(StatusChange::LeftPos);
        if (mScrollBars == ScrollStyle::Both ||  mScrollBars == ScrollStyle::OnlyHorizontal)
            horizontalScrollBar()->setValue(value);
        else {
            if (mDocument->maxLineWidth()>0) {
                mLeftPos = std::max(value, 0);
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
    int newGutterWidth;
    if (mGutter.useFontStyle()) {
        QFontMetrics fm=QFontMetrics(mGutter.font());
        newGutterWidth = mGutter.realGutterWidth(fm.averageCharWidth());
    } else {
        newGutterWidth = mGutter.realGutterWidth(mCharWidth);
    }
    if (newGutterWidth == mGutterWidth)
        invalidateGutter();
    else
        setGutterWidth(newGutterWidth);
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
