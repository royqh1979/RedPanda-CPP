#include "formattergeneralwidget.h"
#include "ui_formattergeneralwidget.h"

FormatterGeneralWidget::FormatterGeneralWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormatterGeneralWidget)
{
    ui->setupUi(this);
}

FormatterGeneralWidget::~FormatterGeneralWidget()
{
    delete ui;
}
