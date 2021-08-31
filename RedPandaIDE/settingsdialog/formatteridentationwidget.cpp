#include "formatteridentationwidget.h"
#include "ui_formatteridentationwidget.h"

FormatterIdentationWidget::FormatterIdentationWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormatterIdentationWidget)
{
    ui->setupUi(this);
}

FormatterIdentationWidget::~FormatterIdentationWidget()
{
    delete ui;
}
