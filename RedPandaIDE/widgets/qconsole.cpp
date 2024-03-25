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
#include "qconsole.h"

#include <QEvent>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QScrollBar>
#include <cmath>
#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QClipboard>
#include "../utils.h"

QConsole::QConsole(QWidget *parent):
    QAbstractScrollArea{parent},
    mContents{this},
    mContentImage{}
{
    mMaxHistory = 500;
    mHistoryIndex = -1;
    mCommand = "";
    mCurrentEditableLine = "";
    mRowHeight = 0;
    mTopRow = 1;
    mRowsInWindow = 0;
    mColumnsPerRow = 0;
    mColumnWidth = 0;
    mReadonly = false;
    mSelectionBegin = {0,0};
    mSelectionEnd = {0,0};
    mCaretChar = 0;
    mBackground = palette().color(QPalette::Base);
    mForeground = palette().color(QPalette::Text);
    mSelectionBackground = palette().color(QPalette::Highlight);
    mSelectionForeground = palette().color(QPalette::HighlightedText);
    mInactiveSelectionBackground = palette().color(QPalette::Inactive,QPalette::Highlight);
    mInactiveSelectionForeground = palette().color(QPalette::Inactive,QPalette::HighlightedText);
    mTabSize = 4;
    mBlinkTimerId = 0;
    mBlinkStatus = 0;
    //enable input method
    setAttribute(Qt::WA_InputMethodEnabled);
//    setMouseTracking(false);
    recalcCharExtent();
    mScrollTimer = new QTimer(this);
    mScrollTimer->setInterval(100);
    connect(mScrollTimer,&QTimer::timeout,this, &QConsole::scrollTimerHandler);
    connect(&mContents,&ConsoleLines::layoutFinished,this, &QConsole::contentsLayouted);
    connect(&mContents,&ConsoleLines::rowsAdded,this, &QConsole::contentsRowsAdded);
    connect(&mContents,&ConsoleLines::lastRowsChanged,this, &QConsole::contentsLastRowsChanged);
    connect(&mContents,&ConsoleLines::lastRowsRemoved,this, &QConsole::contentsLastRowsRemoved);
    connect(verticalScrollBar(),&QScrollBar::valueChanged,
            this, &QConsole::doScrolled);

}

int QConsole::maxHistory() const
{
    return mMaxHistory;
}

void QConsole::setMaxHistory(int historySize)
{
    mMaxHistory = historySize;
}

int QConsole::tabSize() const
{
    return mTabSize;
}

int QConsole::columnsPerRow() const
{
    return mColumnsPerRow;
}

int QConsole::rowsInWindow() const
{
    return mRowsInWindow;
}

int QConsole::charColumns(QChar ch, int columnsBefore) const
{
    if (ch == '\t') {
        return mTabSize - (columnsBefore % mTabSize);
    }
    if (ch == ' ')
        return 1;
    return std::ceil((int)(fontMetrics().horizontalAdvance(ch)) / (double) mColumnWidth);
}

void QConsole::invalidate()
{
    viewport()->update();
}

void QConsole::invalidateRows(int startRow, int endRow)
{
    if (!isVisible())
        return;
    if (startRow == -1 && endRow == -1) {
        invalidate();
    } else {
        startRow = std::max(startRow, 1);
        endRow = std::max(endRow, 1);
        // find the visible lines first
        if (startRow > endRow)
            std::swap(startRow, endRow);

        if (endRow >= mContents.rows())
            endRow = INT_MAX; // paint empty space beyond last line

        // mTopLine is in display coordinates, so FirstLine and LastLine must be
        // converted previously.
        startRow = std::max(startRow, mTopRow);
        endRow = std::min(endRow, mTopRow + mRowsInWindow-1);

        // any line visible?
        if (endRow >= startRow) {
            QRect rcInval = {
                0,
                mRowHeight * (startRow - mTopRow),
                clientWidth(), mRowHeight * (endRow - mTopRow + 1)
            };
            invalidateRect(rcInval);
        }
    }

}

void QConsole::invalidateRect(const QRect &rect)
{
    viewport()->update(rect);
}

void QConsole::addLine(const QString &line)
{
    mCurrentEditableLine = "";
    mCaretChar=0;
    mSelectionBegin = caretPos();
    mSelectionEnd = caretPos();
    mContents.addLine(line);
}

void QConsole::addText(const QString &text)
{
    QStringList lst = textToLines(text);
    for (const QString& line:lst) {
        addLine(line);
    }
}

void QConsole::removeLastLine()
{
    mCurrentEditableLine = "";
    mCaretChar=0;
    mSelectionBegin = caretPos();
    mSelectionEnd = caretPos();
    mContents.RemoveLastLine();
}

void QConsole::changeLastLine(const QString &line)
{
    mContents.changeLastLine(line);
}

QString QConsole::getLastLine()
{
    return mContents.getLastLine();
}

void QConsole::clear()
{
    mContents.clear();
    mCommand = "";
    mCurrentEditableLine = "";
    mTopRow = 1;
    mSelectionBegin = {0,0};
    mSelectionEnd = {0,0};
    mCaretChar = 0;
    updateScrollbars();
}

void QConsole::copy()
{
    if (!this->hasSelection())
        return;
    QString s = selText();
    QClipboard* clipboard=QGuiApplication::clipboard();
    clipboard->clear();
    clipboard->setText(s);
}

void QConsole::paste()
{
    if (mReadonly)
        return;
    QClipboard* clipboard=QGuiApplication::clipboard();
    textInputed(clipboard->text());
}

void QConsole::selectAll()
{
    if (mContents.lines()>0) {
        mSelectionBegin = {1,1};
        mSelectionEnd = { mContents.getLastLine().length()+1,mContents.lines()};
        invalidate();
    }
}

QString QConsole::selText()
{
    if (!hasSelection())
        return "";
    int ColFrom = selectionBegin().ch;
    int First = selectionBegin().line;
    int ColTo = selectionEnd().ch;
    int Last = selectionEnd().line;
    if (First == Last) {
        QString s = mContents.getLine(First);
        if (First == mContents.lines()) {
            s += this->mCommand;
        }
        return s.mid(ColFrom, ColTo - ColFrom);

    } else  {
        QString result = mContents.getLine(First).mid(ColFrom);
        result+= lineBreak();
        for (int i = First + 1; i<=Last - 1; i++) {
            result += mContents.getLine(i);
            result+= lineBreak();
        }
        QString s = mContents.getLine(Last);
        if (Last == mContents.lines())
            s+= this->mCommand;
        result += s.leftRef(ColTo);
        return result;
    }
}

void QConsole::recalcCharExtent() {
    mRowHeight = fontMetrics().lineSpacing();
    mColumnWidth = fontMetrics().horizontalAdvance("M");
}

void QConsole::sizeOrFontChanged(bool)
{
    if (mColumnWidth != 0) {
        mColumnsPerRow = std::max(clientWidth()-2,0) / mColumnWidth;
        mRowsInWindow = clientHeight() / mRowHeight;
        mContents.layout();
    }

}

int QConsole::clientWidth()
{
    return viewport()->size().width();
}

int QConsole::clientHeight()
{
    return viewport()->size().height();
}

void QConsole::setTopRow(int value)
{
    value = std::min(value,maxScrollHeight());
    value = std::max(value, 1);
    if (value != mTopRow) {
        verticalScrollBar()->setValue(value);
    }
}

int QConsole::maxScrollHeight()
{
    return std::max(mContents.rows()-mRowsInWindow+1,1);
}

void QConsole::updateScrollbars()
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    int nMaxScroll = maxScrollHeight();
    int nMin = 1;
    int nMax = std::max(1, nMaxScroll);
    int nPage = mRowsInWindow;
    int nPos = std::min(std::max(mTopRow,nMin),nMax);
    verticalScrollBar()->setMinimum(nMin);
    verticalScrollBar()->setMaximum(nMax);
    verticalScrollBar()->setPageStep(nPage);
    verticalScrollBar()->setSingleStep(1);
    if (nPos != verticalScrollBar()->value())
        verticalScrollBar()->setValue(nPos);
    else
        invalidate();
}

void QConsole::paintRows(QPainter &painter, int row1, int row2)
{
    if (row1>row2)
        return;
    QRect rect(0,(row1-mTopRow)*mRowHeight,clientWidth(),(row2-row1+1)*mRowHeight);
    painter.fillRect(rect,mBackground);
    QStringList lst = mContents.getRows(row1,row2);
    int startRow = row1-mTopRow;
    painter.setPen(mForeground);
    RowColumn selBeginRC = mContents.lineCharToRowColumn(selectionBegin());
    RowColumn selEndRC = mContents.lineCharToRowColumn(selectionEnd());
    LineChar editBegin = {
        mContents.getLastLine().length() - mCurrentEditableLine.length(),
        mContents.lines()-1
    };
    RowColumn editBeginRC = mContents.lineCharToRowColumn(editBegin);
    bool isSelection = false;
    painter.setPen(mForeground);
    for (int i=0; i< lst.size(); i++) {
        int currentRow = i+row1-1;
        int left=2;
        int top = (startRow+i) * mRowHeight;
        int baseLine = (startRow+i+1)*mRowHeight - painter.fontMetrics().descent();
        QString s = lst[i];
        int columnsBefore = 0;
        for (QChar ch:s) {
            int charCol = charColumns(ch,columnsBefore);
            int width = charCol * mColumnWidth;
            if  ((currentRow > selBeginRC.row ||
                    (currentRow == selBeginRC.row && columnsBefore>=selBeginRC.column))
                 &&
                 (currentRow < selEndRC.row ||
                                     (currentRow == selEndRC.row && columnsBefore+charCol<=selEndRC.column))) {
                if (!isSelection) {
                    isSelection = true;
                }
                if (!mReadonly &&(currentRow>editBeginRC.row ||
                        columnsBefore >= editBeginRC.column)) {
                    painter.setPen(mSelectionForeground);
                    painter.fillRect(left,top,width,mRowHeight,mSelectionBackground);
                } else {
                    painter.setPen(mInactiveSelectionForeground);
                    painter.fillRect(left,top,width,mRowHeight,mInactiveSelectionBackground);
                }
            } else {
                if (isSelection) {
                    isSelection = false;
                    painter.setPen(mForeground);
                }
            }
            painter.drawText(left,baseLine,ch);
            left+= width;
            columnsBefore += charCol;
        }
    }
}

void QConsole::ensureCaretVisible()
{
    int caretRow = mContents.rows();
    if (caretRow < mTopRow) {
        mTopRow = caretRow;
        return;
    }
    if (caretRow >= mTopRow + mRowsInWindow) {
        mTopRow = caretRow + 1 -mRowsInWindow;
    }
}

void QConsole::showCaret()
{
    if (mBlinkTimerId==0)
        mBlinkTimerId = startTimer(500);
}

void QConsole::hideCaret()
{
    if (mBlinkTimerId!=0) {
        killTimer(mBlinkTimerId);
        mBlinkTimerId = 0;
        mBlinkStatus = 0;
        updateCaret();
    }
}

void QConsole::updateCaret()
{
    QRect rcCaret = getCaretRect();
    invalidateRect(rcCaret);
}

LineChar QConsole::caretPos()
{
    QString lastLine = mContents.getLastLine();
    int line = std::max(mContents.lines()-1,0);
    int charIndex = 0;
    if (mCaretChar>=mCurrentEditableLine.length()) {
        charIndex = lastLine.length();
    } else {
        charIndex = lastLine.length()-mCurrentEditableLine.length()+mCaretChar;
    }
    return {charIndex,line};
}

RowColumn QConsole::caretRowColumn()
{
    return mContents.lineCharToRowColumn(caretPos());
}

QPoint QConsole::rowColumnToPixels(const RowColumn &rowColumn)
{
    /*
     mTopRow is 1-based; rowColumn.row is 0-based
     */
    int row =rowColumn.row+1 - mTopRow;
    int col =rowColumn.column;
    return QPoint(2+col*mColumnWidth, row*mRowHeight);
}

QRect QConsole::getCaretRect()
{
    LineChar caret = caretPos();
    QChar caretChar = mContents.getChar(caret);
    RowColumn caretRC = mContents.lineCharToRowColumn(caret);
    QPoint caretPos = rowColumnToPixels(caretRC);
    int caretWidth=mColumnWidth;
    //qDebug()<<"caret"<<mCaretX<<mCaretY;
    int columnsBefore = caretRC.column;
    if (!caretChar.isNull()) {
        caretWidth = charColumns(caretChar, columnsBefore)*mColumnWidth;
    }
    return QRect(caretPos.x(),caretPos.y(),caretWidth,
                  mRowHeight);
}

void QConsole::doScrolled()
{
    mTopRow = verticalScrollBar()->value();
    invalidate();
}

void QConsole::contentsLayouted()
{
    updateScrollbars();
}

void QConsole::contentsRowsAdded(int )
{
    ensureCaretVisible();
    updateScrollbars();
}

void QConsole::contentsLastRowsRemoved(int )
{
    ensureCaretVisible();
    updateScrollbars();
}

void QConsole::contentsLastRowsChanged(int rowCount)
{
    ensureCaretVisible();
    invalidateRows(mContents.rows()-rowCount+1,mContents.rows());
}

void QConsole::scrollTimerHandler()

{
    QPoint iMousePos = QCursor::pos();
    iMousePos = mapFromGlobal(iMousePos);
    RowColumn mousePosRC = pixelsToNearestRowColumn(iMousePos.x(),iMousePos.y());

    if (mScrollDeltaY != 0) {
        if (QApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier))
          setTopRow(mTopRow + mScrollDeltaY * mRowsInWindow);
        else
          setTopRow(mTopRow + mScrollDeltaY);
        int row = mTopRow;
        if (mScrollDeltaY > 0)  // scrolling down?
            row+=mRowsInWindow - 1;
        mousePosRC.row = row - 1;
        int oldStartRow = mContents.lineCharToRowColumn(selectionBegin()).row+1;
        int oldEndRow = mContents.lineCharToRowColumn(selectionEnd()).row+1;
        invalidateRows(oldStartRow,oldEndRow);
        mSelectionEnd = mContents.rowColumnToLineChar(mousePosRC);
        invalidateRows(row,row);
    }

//    computeScrollY(Y);
}

void QConsole::mousePressEvent(QMouseEvent *event)
{
    Qt::MouseButton button = event->button();
    int X=event->pos().x();
    int Y=event->pos().y();

    QAbstractScrollArea::mousePressEvent(event);

    //fKbdHandler.ExecuteMouseDown(Self, Button, Shift, X, Y);

    if (button == Qt::LeftButton) {
//        setMouseTracking(true);
        RowColumn mousePosRC = pixelsToNearestRowColumn(X,Y);
        LineChar mousePos = mContents.rowColumnToLineChar(mousePosRC);
        //I couldn't track down why, but sometimes (and definitely not all the time)
        //the block positioning is lost.  This makes sure that the block is
        //maintained in case they started a drag operation on the block
        int oldStartRow = mContents.lineCharToRowColumn(selectionBegin()).row+1;
        int oldEndRow = mContents.lineCharToRowColumn(selectionEnd()).row+1;
        invalidateRows(oldStartRow,oldEndRow);
        mSelectionBegin = mousePos;
        mSelectionEnd = mousePos;
    }
}

void QConsole::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseReleaseEvent(event);
    mScrollTimer->stop();
//    setMouseTracking(false);

}

void QConsole::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractScrollArea::mouseMoveEvent(event);
    Qt::MouseButtons buttons = event->buttons();
    int x=event->pos().x();
    int y=event->pos().y();

    if ((buttons == Qt::LeftButton)) {
      // should we begin scrolling?
      computeScrollY(y);
      RowColumn mousePosRC = pixelsToNearestRowColumn(x, y);
      LineChar mousePos = mContents.rowColumnToLineChar(mousePosRC);
      //qDebug()<<x<<y<<mousePosRC.row<<mousePosRC.column<<mousePos.line<<mousePos.ch;
      if (mScrollDeltaY == 0) {
          int oldStartRow = mContents.lineCharToRowColumn(selectionBegin()).row+1;
          int oldEndRow = mContents.lineCharToRowColumn(selectionEnd()).row+1;
          invalidateRows(oldStartRow,oldEndRow);
          mSelectionEnd = mousePos;
          int row = mContents.lineCharToRowColumn(mSelectionEnd).row+1;
          invalidateRows(row,row);
      }
    }
}

void QConsole::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        event->accept();
        if (mReadonly)
            return;
        emit commandInput(mCommand);
        if (mMaxHistory>0 && !mCommand.trimmed().isEmpty()) {
            if (mCommandHistory.isEmpty()
                || mCommandHistory.last()!=mCommand) {
                if (mCommandHistory.length()==mMaxHistory) {
                    mCommandHistory.pop_front();
                }
                mCommandHistory.append(mCommand);
            }
            mHistoryIndex = mCommandHistory.length();
        }
        mCommand="";
        addLine("");
        return;
    case Qt::Key_Up:
        event->accept();
        mHistoryIndex--;
        loadCommandFromHistory();
        return;
    case Qt::Key_Down:
        event->accept();
        mHistoryIndex++;
        loadCommandFromHistory();
        return;
    case Qt::Key_Left:
        event->accept();
        if (mReadonly)
            return;
        if (mCaretChar>0 && mCaretChar<=mCurrentEditableLine.size()) {
            setCaretChar(mCaretChar-1, !(event->modifiers() & Qt::ShiftModifier));
        }
        return;
    case Qt::Key_Right:
        event->accept();
        if (mReadonly)
            return;
        if (mCaretChar<mCurrentEditableLine.size()) {
            setCaretChar(mCaretChar+1, !(event->modifiers() & Qt::ShiftModifier));
        }
        return;
    case Qt::Key_Home:
        event->accept();
        if (mReadonly)
            return;
        if (mCaretChar>0 && mCaretChar<=mCurrentEditableLine.size()) {
            setCaretChar(0, !(event->modifiers() & Qt::ShiftModifier));
        }
        return;
    case Qt::Key_End:
        event->accept();
        if (mReadonly)
            return;
        if (mCaretChar<mCurrentEditableLine.size()) {
            setCaretChar(mCurrentEditableLine.size(), !(event->modifiers() & Qt::ShiftModifier));
        }
        return;
    case Qt::Key_Backspace:
        event->accept();
        if (mReadonly)
            return;
        if (!mCurrentEditableLine.isEmpty() && (mCaretChar-1)<mCurrentEditableLine.size()
                &&(mCaretChar-1>=0)) {
            QString lastLine;
            if (caretInSelection()) {
                lastLine = removeSelection();
            } else {
                lastLine = mContents.getLastLine();
                int len=mCurrentEditableLine.length();
                mCaretChar--;
                mCommand.remove(mCommand.length()-len+mCaretChar,1);
                lastLine.remove(lastLine.length()-len+mCaretChar,1);
                mCurrentEditableLine.remove(mCaretChar,1);
            }
            mContents.changeLastLine(lastLine);
            mSelectionBegin = caretPos();
            mSelectionEnd = caretPos();
        } else {
            if (hasSelection()) {
                mSelectionBegin = caretPos();
                mSelectionEnd = caretPos();
                invalidate();
            }
        }
        return;
    case Qt::Key_Delete:
        event->accept();
        if (mReadonly)
            return;
        if (!mCurrentEditableLine.isEmpty() && (mCaretChar)<mCurrentEditableLine.size()
                &&(mCaretChar>=0)) {
            QString lastLine;
            if (caretInSelection()) {
                lastLine = removeSelection();
            } else {
                lastLine = mContents.getLastLine();
                int len=mCurrentEditableLine.length();
                mCommand.remove(mCommand.length()-len+mCaretChar,1);
                lastLine.remove(lastLine.length()-len+mCaretChar,1);
                mCurrentEditableLine.remove(mCaretChar,1);
            }
            mContents.changeLastLine(lastLine);
            mSelectionBegin = caretPos();
            mSelectionEnd = caretPos();
        } else {
            if (hasSelection()) {
                mSelectionBegin = caretPos();
                mSelectionEnd = caretPos();
                invalidate();
            }
        }
        return;
    default:
        if (!event->text().isEmpty()) {
            event->accept();
            if (mReadonly)
                return;
            textInputed(event->text());
            return;
        }
    }
    QAbstractScrollArea::keyPressEvent(event);
}

void QConsole::focusInEvent(QFocusEvent *)
{
    showCaret();
}

void QConsole::focusOutEvent(QFocusEvent *)
{
    hideCaret();
}

void QConsole::paintEvent(QPaintEvent *event)
{
    if (mRowHeight==0)
        return;
    // Now paint everything while the caret is hidden.
    QPainter painter(viewport());
    //Get the invalidated rect.
    QRect rcClip = event->rect();
    QRect rcCaret= getCaretRect();

    if (rcCaret == rcClip) {
        // only update caret
        painter.drawImage(rcCaret,*mContentImage,rcCaret);
    } else {
        int nL1, nL2;
        // Compute the invalid area in lines
        // lines
        nL1 = std::min(std::max(mTopRow + rcClip.top() / mRowHeight, mTopRow), maxScrollHeight() + mRowsInWindow  - 1 );
        nL2 = std::min(std::max(mTopRow + (rcClip.bottom() + mRowHeight - 1) / mRowHeight, 1), maxScrollHeight() + mRowsInWindow  - 1);
        QPainter cachePainter(mContentImage.get());
        cachePainter.setFont(font());
        if (viewport()->rect() == rcClip) {
            cachePainter.fillRect(rcClip, mBackground);
        }
        paintRows(cachePainter,nL1,nL2);
        painter.drawImage(rcClip,*mContentImage,rcClip);
    }
    paintCaret(painter, rcCaret);
}

void QConsole::paintCaret(QPainter &painter, const QRect rcClip)
{
    if (mBlinkStatus!=1)
        return;
    painter.setClipRect(rcClip);
    ConsoleCaretType ct = ConsoleCaretType::ctHorizontalLine;
    QColor caretColor = mForeground;
    switch(ct) {
    case ConsoleCaretType::ctVerticalLine:
        painter.fillRect(rcClip.left()+1,rcClip.top(),rcClip.left()+2,rcClip.bottom(),caretColor);
        break;
    case ConsoleCaretType::ctHorizontalLine:
        painter.fillRect(rcClip.left(),rcClip.bottom()-2,rcClip.right(),rcClip.bottom()-1,caretColor);
        break;
    case ConsoleCaretType::ctBlock:
        painter.fillRect(rcClip, caretColor);
        break;
    case ConsoleCaretType::ctHalfBlock:
        QRect rc=rcClip;
        rc.setTop(rcClip.top()+rcClip.height() / 2);
        painter.fillRect(rcClip, caretColor);
        break;
    }
}

void QConsole::textInputed(const QString &text)
{
    if (mContents.rows()<=0) {
        mContents.addLine("");
    }
    QString lastLine;
    if (caretInSelection()) {
        lastLine = removeSelection();
    } else {
        lastLine = mContents.getLastLine();
    }
    if (mCaretChar>=mCurrentEditableLine.size()) {
        mCommand += text;
        mCurrentEditableLine += text;
        mCaretChar=mCurrentEditableLine.size();
        mContents.changeLastLine(lastLine+text);
    } else {        
        int len=mCurrentEditableLine.length();
        mCommand.insert(mCommand.length()-len+mCaretChar,text);
        lastLine.insert(lastLine.length()-len+mCaretChar,text);
        mCurrentEditableLine.insert(mCaretChar,text);
        mContents.changeLastLine(lastLine);
        mCaretChar+=text.length();
    }
    mSelectionBegin = caretPos();
    mSelectionEnd = caretPos();
}

void QConsole::loadCommandFromHistory()
{
    if (mMaxHistory<=0)
        return;
    if (mHistoryIndex<0)
        mHistoryIndex=0;
    if (mHistoryIndex<mCommandHistory.length())
        mCommand = mCommandHistory[mHistoryIndex];
    else
        mCommand = "";
    if (mHistoryIndex>mCommandHistory.length())
        mHistoryIndex=mCommandHistory.length();
    QString lastLine = mContents.getLastLine();
    int len=mCurrentEditableLine.length();
    lastLine.remove(lastLine.length()-len,INT_MAX);
    mCurrentEditableLine=mCommand;
    mCaretChar = mCurrentEditableLine.length();
    mSelectionBegin = caretPos();
    mSelectionEnd = caretPos();
    mContents.changeLastLine(lastLine + mCommand);
}

LineChar QConsole::selectionBegin()
{
    if (mSelectionBegin.line < mSelectionEnd.line ||
            (mSelectionBegin.line == mSelectionEnd.line &&
             mSelectionBegin.ch < mSelectionEnd.ch))
        return mSelectionBegin;
    return mSelectionEnd;
}

LineChar QConsole::selectionEnd()
{
    if (mSelectionBegin.line < mSelectionEnd.line ||
            (mSelectionBegin.line == mSelectionEnd.line &&
             mSelectionBegin.ch < mSelectionEnd.ch))
        return mSelectionEnd;
    return mSelectionBegin;
}

void QConsole::setCaretChar(int newCaretChar, bool resetSelection)
{
    RowColumn oldPosRC = caretRowColumn();
    RowColumn oldSelBegin = mContents.lineCharToRowColumn(selectionBegin());
    RowColumn oldSelEnd = mContents.lineCharToRowColumn(selectionEnd());
    int oldStartRow = std::min(std::min(oldPosRC.row,oldSelBegin.row),oldSelEnd.row);
    int oldEndRow = std::max(std::max(oldPosRC.row,oldSelBegin.row),oldSelEnd.row);
    mCaretChar = newCaretChar;
    LineChar newPos = caretPos();
    RowColumn newPosRC = mContents.lineCharToRowColumn(newPos);
    if (resetSelection)
        mSelectionBegin = newPos;
    mSelectionEnd = newPos;

    int startRow = std::min(newPosRC.row, oldStartRow)+1;
    int endRow = std::max(newPosRC.row, oldEndRow)+1;
    invalidateRows(startRow, endRow);
}

bool QConsole::caretInSelection()
{
    if (!hasSelection())
        return false;
    //LineChar selBegin = selectionBegin();
    LineChar selEnd = selectionEnd();
    QString lastline = mContents.getLastLine();
    int editBeginChar = lastline.length() - mCurrentEditableLine.length();
    if (selEnd.line == mContents.lines()-1 && selEnd.ch > editBeginChar ) {
        return true;
    }
    return false;
}

QString QConsole::removeSelection()
{

    QString lastLine = mContents.getLastLine();
    int len=mCurrentEditableLine.length();
    LineChar selBegin = selectionBegin();
    LineChar selEnd = selectionEnd();
    int selLen = selEnd.ch -selBegin.ch;
    int ch = selBegin.ch -(lastLine.length()-len);
    if (selBegin.line < mContents.lines()-1) {
        mCaretChar = 0;
        selLen = selEnd.ch - (lastLine.length()-len);
    }  else if (ch<0) {
        mCaretChar = 0;
        selLen = selLen + ch;
    } else {
        mCaretChar=ch;
    }
    mCommand.remove(mCommand.length()-len+mCaretChar,selLen);
    lastLine.remove(lastLine.length()-len+mCaretChar,selLen);
    mCurrentEditableLine.remove(mCaretChar,selLen);
    return lastLine;
}

bool QConsole::hasSelection()
{
    return (mSelectionBegin.line != mSelectionEnd.line)
            || (mSelectionBegin.ch != mSelectionEnd.ch);
}

int QConsole::computeScrollY(int y)
{
    QRect iScrollBounds = viewport()->rect();
    if (y < iScrollBounds.top())
        mScrollDeltaY = (y - iScrollBounds.top()) / mRowHeight - 1;
    else if (y >= iScrollBounds.bottom())
        mScrollDeltaY = (y - iScrollBounds.bottom()) / mRowHeight + 1;
    else
        mScrollDeltaY = 0;

    if (mScrollDeltaY)
        mScrollTimer->start();
    return mScrollDeltaY;
}

RowColumn QConsole::pixelsToNearestRowColumn(int x, int y)
{
    // Result is in display coordinates
    // don't return a partially visible last line
    if (y >= mRowsInWindow * mRowHeight) {
        y = mRowsInWindow * mRowHeight - 1;
        if (y < 0)
            y = 0;
    }
    return {
      std::max(0, (x - 2) / mColumnWidth),
      mTopRow + (y / mRowHeight)-1
    };
}

QString QConsole::lineBreak()
{
    return "\r\n";
}


void QConsole::fontChanged()
{
    recalcCharExtent();
    sizeOrFontChanged(true);
}

bool QConsole::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::FontChange:
        fontChanged();
        break;
    case QEvent::PaletteChange:
        mBackground = palette().color(QPalette::Base);
        mForeground = palette().color(QPalette::Text);
        mSelectionBackground = palette().color(QPalette::Highlight);
        mSelectionForeground = palette().color(QPalette::HighlightedText);
        mInactiveSelectionBackground = palette().color(QPalette::Inactive,QPalette::Highlight);
        mInactiveSelectionForeground = palette().color(QPalette::Inactive,QPalette::HighlightedText);
        break;
    default:
        break;
    }
    return QAbstractScrollArea::event(event);
}

void QConsole::resizeEvent(QResizeEvent *)
{
    //resize the cache image
    std::shared_ptr<QImage> image = std::make_shared<QImage>(clientWidth(),clientHeight(),
                                                            QImage::Format_ARGB32);
    if (mContentImage) {
        //QRect newRect = image->rect().intersected(mContentImage->rect());
        QPainter painter(image.get());
        painter.fillRect(viewport()->rect(),mBackground);
//        painter.drawImage(newRect,*mContentImage);
    }

    mContentImage = image;

    sizeOrFontChanged(false);
}

void QConsole::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == mBlinkTimerId) {
        mBlinkStatus = 1- mBlinkStatus;
        updateCaret();
    }
}

void QConsole::inputMethodEvent(QInputMethodEvent *event)
{
    if (mReadonly)
        return;
    QString s=event->commitString();
    if (!s.isEmpty())
        textInputed(s);
}

void QConsole::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y()>0) {
        verticalScrollBar()->setValue(verticalScrollBar()->value()-1);
        event->accept();
        return;
    } else if (event->angleDelta().y()<0)  {
        verticalScrollBar()->setValue(verticalScrollBar()->value()+1);
        event->accept();
        return;
    }
}

int ConsoleLines::rows() const
{
    return mRows;
}

int ConsoleLines::lines() const
{
    return mLines.count();
}

void ConsoleLines::layout()
{
    if (!mConsole)
        return;
    if (mLayouting) {
        mNeedRelayout = true;
        return;
    }
    mLayouting = true;
    mNeedRelayout = false;
    emit layoutStarted();
    mRows = 0;
    bool forceUpdate = (mOldTabSize!=mConsole->tabSize());
    for (PConsoleLine consoleLine: mLines) {
        if (forceUpdate || consoleLine->maxColumns > mConsole->columnsPerRow()) {
            consoleLine->maxColumns = breakLine(consoleLine->text,consoleLine->fragments);
        }
        mRows+=consoleLine->fragments.count();
    }
    emit layoutFinished();
    mLayouting = false;
    if (mNeedRelayout)
        emit needRelayout();
}

ConsoleLines::ConsoleLines(QConsole *console)
{
    mConsole = console;
    mRows = 0;
    mLayouting = false;
    mNeedRelayout = false;
    mOldTabSize = -1;
    mMaxLines = 1000;
    connect(this,&ConsoleLines::needRelayout,this,&ConsoleLines::layout);
}

void ConsoleLines::addLine(const QString &line)
{
    PConsoleLine consoleLine=std::make_shared<ConsoleLine>();
    consoleLine->text = line;
    consoleLine->maxColumns = breakLine(line,consoleLine->fragments);
    if (mLines.count()<mMaxLines || mMaxLines <= 0) {
        mLines.append(consoleLine);
        mRows += consoleLine->fragments.count();
        emit rowsAdded(consoleLine->fragments.count());
    } else {
        PConsoleLine firstLine = mLines[0];
        mLines.pop_front();
        mRows -= firstLine->fragments.count();
        mLines.append(consoleLine);
        mRows += consoleLine->fragments.count();
        emit layoutStarted();
        emit layoutFinished();
    }
}

void ConsoleLines::RemoveLastLine()
{
    if (mLines.count()<=0)
        return;
    PConsoleLine consoleLine = mLines[mLines.count()-1];
    mLines.pop_back();
    mRows -= consoleLine->fragments.count();
    emit lastRowsRemoved(consoleLine->fragments.count());
}

void ConsoleLines::changeLastLine(const QString &newLine)
{
    if (mLines.count()<=0) {
        return;
    }
    PConsoleLine consoleLine = mLines[mLines.count()-1];
    int oldRows = consoleLine->fragments.count();
    consoleLine->text = newLine;
    breakLine(newLine,consoleLine->fragments);
    int newRows = consoleLine->fragments.count();
    if (newRows == oldRows) {
        emit lastRowsChanged(oldRows);
        return ;
    } else {
        mRows -= oldRows;
        mRows += newRows;
        emit layoutStarted();
        emit layoutFinished();
    }
}

QString ConsoleLines::getLastLine()
{
    if (mLines.count()<=0)
        return "";
    return mLines[mLines.count()-1]->text;
}

QString ConsoleLines::getLine(int line)
{
    if (line>=0 && line < mLines.count()) {
        return mLines[line]->text;
    }
    return "";
}

QChar ConsoleLines::getChar(int line, int ch)
{
    QString s = getLine(line);
    if (ch>=0 && ch<s.length()) {
        return s[ch];
    } else {
        return QChar();
    }
}

QChar ConsoleLines::getChar(const LineChar &lineChar)
{
    return getChar(lineChar.line,lineChar.ch);
}

QStringList ConsoleLines::getRows(int startRow, int endRow)
{
    if (startRow>mRows)
        return QStringList();
    if (startRow > endRow)
        return QStringList();
    QStringList lst;
    int row = 0;
    for (PConsoleLine line:mLines) {
        for (const QString& s:line->fragments) {
            row+=1;
            if (row>endRow) {
                return lst;
            }
            if (row>=startRow) {
                lst.append(s);
            }
        }
    }
    return lst;
}

LineChar ConsoleLines::rowColumnToLineChar(const RowColumn &rowColumn)
{
    return rowColumnToLineChar(rowColumn.row,rowColumn.column);
}

LineChar ConsoleLines::rowColumnToLineChar(int row, int column)
{
    LineChar result{column,mLines.size()-1};
    int rows=0;
    for (int i=0;i<mLines.size();i++) {
        PConsoleLine line = mLines[i];
        if (row >= rows && row<rows+line->fragments.size()) {
            int r=row - rows;
            QString fragment = line->fragments[r];
            int columnsBefore = 0;
            int charsBefore = 0;
            for (int j=0;j<r;j++) {
                charsBefore += line->fragments[j].length();
            }
            for (int j=0;j<fragment.size();j++) {
                QChar ch = fragment[j];
                int charColumns= mConsole->charColumns(ch, columnsBefore);
                if (column>=columnsBefore && column<columnsBefore+charColumns) {
                    result.ch = charsBefore + j;
                    break;
                }
                columnsBefore += charColumns;
            }
            result.line = i;
            break;
        }
        rows += line->fragments.size();
    }
    return result;
}

RowColumn ConsoleLines::lineCharToRowColumn(const LineChar &lineChar)
{
    return lineCharToRowColumn(lineChar.line,lineChar.ch);
}

RowColumn ConsoleLines::lineCharToRowColumn(int line, int ch)
{
    RowColumn result{ch,std::max(0,mRows-1)};
    int rowsBefore = 0;
    if (line>=0 && line < mLines.size()) {
        for (int i=0;i<line;i++) {
            int rows = mLines[i]->fragments.size();
            rowsBefore += rows;
        }
        PConsoleLine consoleLine = mLines[line];
        int charsBefore = 0;
        for (int r=0;r<consoleLine->fragments.size();r++) {
            int chars = consoleLine->fragments[r].size();
            if (r==consoleLine->fragments.size()-1 || (ch>=charsBefore && ch<charsBefore+chars)) {
                QString fragment = consoleLine->fragments[r];
                int columnsBefore = 0;
                int len = std::min(ch-charsBefore,fragment.size());
                for (int j=0;j<len;j++) {
                    QChar ch = fragment[j];
                    int charColumns = mConsole->charColumns(ch,columnsBefore);
                    columnsBefore += charColumns;
                }
                result.column=columnsBefore;
                result.row = rowsBefore + r;
                break;
            }
            charsBefore += chars;
        }
    }
    return result;
}

bool ConsoleLines::layouting() const
{
    return mLayouting;
}

int ConsoleLines::breakLine(const QString &line, QStringList &fragments)
{
    fragments.clear();
    QString s = "";
    int maxColLen = 0;
    int columnsBefore = 0;
    for (QChar ch:line) {
        int charColumn = mConsole->charColumns(ch,columnsBefore);
        if (charColumn + columnsBefore > mConsole->columnsPerRow()) {
            if (ch == '\t') {
                if  (columnsBefore != mConsole->columnsPerRow()) {
                    charColumn = 0;
                } else
                    charColumn = mConsole->tabSize();
            }
            fragments.append(s);
            if (columnsBefore > maxColLen) {
                maxColLen = columnsBefore;
            }
            s = "";
            columnsBefore = 0;
        }
        if (charColumn > 0) {
            columnsBefore += charColumn;
            s += ch;
        }
    }
    if (fragments.count() == 0 || !s.isEmpty()) {
        fragments.append(s);
        if (columnsBefore > maxColLen) {
            maxColLen = columnsBefore;
        }
    }
    return maxColLen;
}

int ConsoleLines::getMaxLines() const
{
    return mMaxLines;
}

int ConsoleLines::maxLines() const
{
    return mMaxLines;
}

void ConsoleLines::setMaxLines(int maxLines)
{
    mMaxLines = maxLines;
    if (mMaxLines > 0) {
        while (mLines.count()>mMaxLines) {
            mLines.pop_front();
        }
    }
}

void ConsoleLines::clear()
{
    mLines.clear();
    mRows = 0;
}
