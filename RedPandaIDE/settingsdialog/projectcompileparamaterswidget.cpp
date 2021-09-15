#include "projectcompileparamaterswidget.h"
#include "ui_projectcompileparamaterswidget.h"

ProjectCompileParamatersWidget::ProjectCompileParamatersWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProjectCompileParamatersWidget)
{
    ui->setupUi(this);
}

ProjectCompileParamatersWidget::~ProjectCompileParamatersWidget()
{
    delete ui;
}
