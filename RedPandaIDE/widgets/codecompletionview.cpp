#include "codecompletionview.h"
#include "ui_codecompletionview.h"

#include <QKeyEvent>

CodeCompletionView::CodeCompletionView(QWidget *parent) :
    QWidget(parent)
{
    setWindowFlags(Qt::Popup);
    mListView = new CodeCompletionListView(this);
    setLayout(new QVBoxLayout());
    layout()->addWidget(mListView);
    layout()->setMargin(0);
}

CodeCompletionView::~CodeCompletionView()
{

}

void CodeCompletionView::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mListView->setKeypressedCallback(newKeypressedCallback);
}

CodeCompletionListView::CodeCompletionListView(QWidget *parent) : QListView(parent)
{

}

void CodeCompletionListView::keyPressEvent(QKeyEvent *event)
{
    if (!mKeypressedCallback(event)) {
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
