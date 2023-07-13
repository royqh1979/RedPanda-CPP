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
#ifndef LINENUMBERTEXTEDITOR_H
#define LINENUMBERTEXTEDITOR_H

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>

class LineNumberTextEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    LineNumberTextEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    const QColor &lineNumberAreaForeground() const;
    void setLineNumberAreaForeground(const QColor &newLineNumberAreaForeground);

    const QColor &lineNumberAreaBackground() const;
    void setLineNumberAreaBackground(const QColor &newLineNumberAreaBackground);

    const QColor &lineNumberAreaCurrentLine() const;
    void setLineNumberAreaCurrentLine(const QColor &newLineNumberAreaCurrentLine);

    void clearFormat();

    void clearAll();

    void highlightLine(int line, QColor highlightColor);

    void locateLine(int line);

signals:
    void lineNumberAreaCurrentLineChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
private:
    void clearStartFormat();

private:
    QWidget *lineNumberArea;
    QColor mLineNumberAreaForeground;
    QColor mLineNumberAreaBackground;
    QColor mLineNumberAreaCurrentLine;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(LineNumberTextEditor *editor) : QWidget(editor), mParentEditor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(mParentEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        mParentEditor->lineNumberAreaPaintEvent(event);
    }

private:
    LineNumberTextEditor *mParentEditor;
};



#endif // LINENUMBERTEXTEDITOR_H
