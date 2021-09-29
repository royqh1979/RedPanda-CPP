#include "codecompletionlistview.h"
#include "../mainwindow.h"
#include "../editor.h"
#include "../editorlist.h"

CodeCompletionListView::CodeCompletionListView(QWidget *parent) : QListView(parent)
{

}

void CodeCompletionListView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Up
            || event->key() == Qt::Key_Down) {
        QListView::keyPressEvent(event);
        return;
    }
    if (!mKeypressedCallback || !mKeypressedCallback(event)) {
        QListView::keyPressEvent(event);
    }
}

void CodeCompletionListView::focusInEvent(QFocusEvent *event)
{
    Editor *editor = pMainWindow->editorList()->getEditor();
    if (editor) {
        qDebug()<<"popup:show caret";
        editor->showCaret();
    }
}

const KeyPressedCallback &CodeCompletionListView::keypressedCallback() const
{
    return mKeypressedCallback;
}

void CodeCompletionListView::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mKeypressedCallback = newKeypressedCallback;
}

