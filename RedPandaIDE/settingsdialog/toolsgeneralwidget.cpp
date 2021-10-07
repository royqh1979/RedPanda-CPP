#include "toolsgeneralwidget.h"
#include "ui_toolsgeneralwidget.h"

ToolsGeneralWidget::ToolsGeneralWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ToolsGeneralWidget)
{
    ui->setupUi(this);
}

ToolsGeneralWidget::~ToolsGeneralWidget()
{
    delete ui;
}
