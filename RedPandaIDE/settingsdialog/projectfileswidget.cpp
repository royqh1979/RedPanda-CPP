#include "projectfileswidget.h"
#include "ui_projectfileswidget.h"

ProjectFilesWidget::ProjectFilesWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProjectFilesWidget)
{
    ui->setupUi(this);
}

ProjectFilesWidget::~ProjectFilesWidget()
{
    delete ui;
}
