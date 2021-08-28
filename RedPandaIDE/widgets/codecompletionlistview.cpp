#include "codecompletionlistview.h"

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

const KeyPressedCallback &CodeCompletionListView::keypressedCallback() const
{
    return mKeypressedCallback;
}

void CodeCompletionListView::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mKeypressedCallback = newKeypressedCallback;
}

