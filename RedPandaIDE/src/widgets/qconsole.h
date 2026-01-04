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
#ifndef QCONSOLE_H
#define QCONSOLE_H

#include <QAbstractScrollArea>
#include <QVector>
#include <memory>

struct ConsoleLine {
    QString text;
    QStringList fragments;
    int maxColumns;
};

enum class ConsoleCaretType {
    ctVerticalLine,ctHorizontalLine,ctBlock,ctHalfBlock
};

using PConsoleLine = std::shared_ptr<ConsoleLine>;

using ConsoleLineList = QVector<PConsoleLine>;

/**
 * @brief The RowColumn struct
 * column and row are 0-based
 */
struct RowColumn {
    int column;
    int row;
};

/**
 * @brief The LineChar struct
 * line and ch are 0-based
 */
struct LineChar {
    int ch;
    int line;

    LineChar() = default;
    constexpr LineChar(qsizetype ch_, qsizetype line_) : ch(ch_), line(line_) {}
};

class QConsole;
class ConsoleLines : public QObject{
    Q_OBJECT
public:
    explicit ConsoleLines(QConsole* console);
    void addLine(const QString& line);
    void RemoveLastLine();
    void changeLastLine(const QString& newLine);
    QString getLastLine();
    QString getLine(int line);
    QChar getChar(int line,int ch);
    QChar getChar(const LineChar& lineChar);
    /**
     * @brief getRows
     * @param startRow 1-based
     * @param endRow 1-based
     * @return
     */
    QStringList getRows(int startRow, int endRow);

    LineChar rowColumnToLineChar(const RowColumn& rowColumn);
    LineChar rowColumnToLineChar(int row ,int column);
    RowColumn lineCharToRowColumn(const LineChar& lineChar);
    RowColumn lineCharToRowColumn(int line, int ch);
    int rows() const;
    int lines() const;
    bool layouting() const;
    int maxLines() const;
    void setMaxLines(int maxLines);
    void clear();

    int getMaxLines() const;
public slots:
    void layout();
signals:
    void layoutStarted();
    void layoutFinished();
    void needRelayout();
    void rowsAdded(int rowCount);
    void lastRowsRemoved(int rowCount);
    void lastRowsChanged(int rowCount);
private:
    int breakLine(const QString& line, QStringList& fragments);
private:
    ConsoleLineList mLines;
    int mRows;
    bool mLayouting;
    bool mNeedRelayout;
    int mOldTabSize;
    QConsole* mConsole;
    int mMaxLines;
};


class QConsole : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit QConsole(QWidget* parent = nullptr);
    int maxHistory() const;
    void setMaxHistory(int historySize);

    int tabSize() const;

    int columnsPerRow() const;

    int rowsInWindow() const;
    int charColumns(QChar ch, int columnsBefore) const;

    void invalidate();
    void invalidateRows(int startRow,int endRow);
    void invalidateRect(const QRect& rect);

    void addLine(const QString& line);
    void addText(const QString& text);
    void removeLastLine();
    void changeLastLine(const QString& line);
    QString getLastLine();
    void clear();
    void copy();
    void paste();
    void selectAll();
    QString selText();

signals:
    void commandInput(const QString& command);
private:
    ConsoleLines mContents;
    QStringList mCommandHistory;
    int mMaxHistory;
    int mHistoryIndex;
    QString mCommand;
    QString mCurrentEditableLine;
//    bool mIndex;
    int mRowHeight;
    int mTopRow; // 1-based
    int mRowsInWindow;
    int mColumnsPerRow;
    int mColumnWidth;
    bool mReadonly;
    LineChar mSelectionBegin;
    LineChar mSelectionEnd;
    int mCaretChar;
    QColor mBackground;
    QColor mForeground;
    QColor mSelectionBackground;
    QColor mSelectionForeground;
    QColor mInactiveSelectionBackground;
    QColor mInactiveSelectionForeground;
    int mTabSize;
    std::shared_ptr<QImage> mContentImage;
    int mBlinkTimerId;
    int mBlinkStatus;
    QTimer* mScrollTimer;
    int mScrollDeltaY;
private:
    void fontChanged();
    void recalcCharExtent();
    void sizeOrFontChanged(bool bFont);
    int clientWidth();
    int clientHeight();
    /**
     * @brief setTopRow
     * @param value 1-based
     */
    void setTopRow(int value);
    int maxScrollHeight();
    void updateScrollbars();
    void paintRows(QPainter& painter, int row1,int row2);
    void ensureCaretVisible();
    void showCaret();
    void hideCaret();
    void updateCaret();
    LineChar caretPos();
    RowColumn caretRowColumn();
    QPoint rowColumnToPixels(const RowColumn& rowColumn);
    QRect getCaretRect();
    void paintCaret(QPainter &painter, const QRect rcClip);
    void textInputed(const QString& text);
    void loadCommandFromHistory();
    LineChar selectionBegin();
    LineChar selectionEnd();
    void setCaretChar(int newCaretChar, bool resetSelection);
    bool caretInSelection();
    QString removeSelection();
    bool hasSelection();
    int computeScrollY(int y);
    RowColumn pixelsToNearestRowColumn(int x,int y);
    QString lineBreak();


private slots:
    void doScrolled();
    void contentsLayouted();
    void contentsRowsAdded(int rowCount);
    void contentsLastRowsRemoved(int rowCount);
    void contentsLastRowsChanged(int rowCount);
    void scrollTimerHandler();

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;


    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event) override;

    // QObject interface
protected:
    void timerEvent(QTimerEvent *event) override;

    // QWidget interface
protected:
    void inputMethodEvent(QInputMethodEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
};


#endif // QCONSOLE_H
