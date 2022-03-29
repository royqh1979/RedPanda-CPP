#ifndef LINENUMBERTEXTEDITOR_H
#define LINENUMBERTEXTEDITOR_H

#include <QPlainTextEdit>

class LineNumberTextEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    LineNumberTextEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
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
