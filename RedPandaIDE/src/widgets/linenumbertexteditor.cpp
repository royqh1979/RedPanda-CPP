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
#include "linenumbertexteditor.h"

#include <QPainter>
#include <QTextBlock>
#include <QDebug>

LineNumberTextEditor::LineNumberTextEditor(QWidget *parent):QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &LineNumberTextEditor::blockCountChanged, this, &LineNumberTextEditor::updateLineNumberAreaWidth);
    connect(this, &LineNumberTextEditor::updateRequest, this, &LineNumberTextEditor::updateLineNumberArea);
    connect(this, &LineNumberTextEditor::cursorPositionChanged, this, &LineNumberTextEditor::highlightCurrentLine);
    updateLineNumberAreaWidth(0);
    //highlightCurrentLine();
}

int LineNumberTextEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}

void LineNumberTextEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void LineNumberTextEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void LineNumberTextEditor::clearStartFormat()
{
    moveCursor(QTextCursor::Start);
    QTextCursor cursor = textCursor();
    cursor.setCharFormat(QTextCharFormat());
}

const QColor &LineNumberTextEditor::lineNumberAreaCurrentLine() const
{
    return mLineNumberAreaCurrentLine;
}

void LineNumberTextEditor::setLineNumberAreaCurrentLine(const QColor &newLineNumberAreaCurrentLine)
{
    if (mLineNumberAreaCurrentLine == newLineNumberAreaCurrentLine)
        return;
    mLineNumberAreaCurrentLine = newLineNumberAreaCurrentLine;
    emit lineNumberAreaCurrentLineChanged();
}

void LineNumberTextEditor::clearFormat()
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(QTextCharFormat());
    cursor.clearSelection();
}

void LineNumberTextEditor::clearAll()
{
    clear();
    clearStartFormat();
}

void LineNumberTextEditor::highlightLine(int line, QColor highlightColor)
{
    QTextBlock block = document()->findBlockByLineNumber(line);
    if (!block.isValid())
        return;
    QTextCursor cur(block);
    if (cur.isNull())
        return;
    QTextCharFormat oldFormat = cur.charFormat();
    QTextCharFormat format = QTextCharFormat(cur.charFormat());
    cur.select(QTextCursor::LineUnderCursor);
    format.setUnderlineColor(highlightColor);
    format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    format.setTextOutline(highlightColor);
    cur.setCharFormat(format);
    cur.clearSelection();
    cur.setCharFormat(oldFormat);
    setTextCursor(cur);
    moveCursor(QTextCursor::MoveOperation::StartOfLine);
}

void LineNumberTextEditor::locateLine(int line)
{
    QTextBlock block = document()->findBlockByLineNumber(line);
    if (!block.isValid())
        return;
    QTextCursor cur(block);
    if (cur.isNull())
        return;
    setTextCursor(cur);
    moveCursor(QTextCursor::MoveOperation::StartOfLine);
}

const QColor &LineNumberTextEditor::lineNumberAreaBackground() const
{
    return mLineNumberAreaBackground;
}

void LineNumberTextEditor::setLineNumberAreaBackground(const QColor &newLineNumberAreaBackground)
{
    mLineNumberAreaBackground = newLineNumberAreaBackground;
}

const QColor &LineNumberTextEditor::lineNumberAreaForeground() const
{
    return mLineNumberAreaForeground;
}

void LineNumberTextEditor::setLineNumberAreaForeground(const QColor &newLineNumberAreaForeground)
{
    mLineNumberAreaForeground = newLineNumberAreaForeground;
}

void LineNumberTextEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void LineNumberTextEditor::highlightCurrentLine()
{
    lineNumberArea->update();
//    QList<QTextEdit::ExtraSelection> extraSelections;

//    if (!isReadOnly()) {
//        QTextEdit::ExtraSelection selection;

//        QColor lineColor = QColor(Qt::yellow).lighter(160);

//        selection.format.setBackground(lineColor);
//        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
//        selection.cursor = textCursor();
//        selection.cursor.clearSelection();
//        extraSelections.append(selection);
//    }

//    setExtraSelections(extraSelections);
}

void LineNumberTextEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.setFont(font());
    if (isEnabled())
        painter.fillRect(event->rect(), mLineNumberAreaBackground);
    else
        painter.fillRect(event->rect(), palette().color(QPalette::Disabled, QPalette::Button));
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            if (!isEnabled())
                painter.setPen(palette().color(QPalette::Disabled,QPalette::ButtonText));
            else if (textCursor().blockNumber()==blockNumber)
                painter.setPen(mLineNumberAreaCurrentLine);
            else
                painter.setPen(mLineNumberAreaForeground);
            // int y=top+std::max(0,bottom-top-fontMetrics().lineSpacing());
            painter.drawText(5, top, lineNumberArea->width()-10, fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
