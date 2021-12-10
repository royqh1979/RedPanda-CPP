#include "initchooselanguagewidget.h"
#include "ui_initchooselanguagewidget.h"

InitChooseLanguageWidget::InitChooseLanguageWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InitChooseLanguageWidget)
{
    ui->setupUi(this);
}

InitChooseLanguageWidget::~InitChooseLanguageWidget()
{
    delete ui;
}
