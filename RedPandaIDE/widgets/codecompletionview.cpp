#include "codecompletionview.h"
#include "ui_codecompletionview.h"

CodeCompletionView::CodeCompletionView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CodeCompletionView)
{
    ui->setupUi(this);
}

CodeCompletionView::~CodeCompletionView()
{
    delete ui;
}
