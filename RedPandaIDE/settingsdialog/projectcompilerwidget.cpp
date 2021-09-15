#include "projectcompilerwidget.h"
#include "ui_projectcompilerwidget.h"

ProjectCompilerWidget::ProjectCompilerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProjectCompilerWidget)
{
    ui->setupUi(this);
}

ProjectCompilerWidget::~ProjectCompilerWidget()
{
    delete ui;
}
