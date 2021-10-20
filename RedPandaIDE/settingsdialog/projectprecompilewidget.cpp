#include "projectprecompilewidget.h"
#include "ui_projectprecompilewidget.h"
#include "../mainwindow.h"
#include "../project.h"

#include <QFileDialog>

ProjectPreCompileWidget::ProjectPreCompileWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectPreCompileWidget)
{
    ui->setupUi(this);
}

ProjectPreCompileWidget::~ProjectPreCompileWidget()
{
    delete ui;
}

void ProjectPreCompileWidget::doLoad()
{
    ui->grpPrecompileHeader->setChecked(pMainWindow->project()->options().usePrecompiledHeader);
    ui->txtPrecompileHeader->setText(pMainWindow->project()->options().precompiledHeader);
}

void ProjectPreCompileWidget::doSave()
{
    pMainWindow->project()->options().usePrecompiledHeader = ui->grpPrecompileHeader->isChecked();
    pMainWindow->project()->options().precompiledHeader = ui->txtPrecompileHeader->text();
    pMainWindow->project()->saveOptions();
}

void ProjectPreCompileWidget::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Precompiled header"),
                pMainWindow->project()->directory(),
                tr("header files (*.h)"));
    if (!fileName.isEmpty() && QFileInfo(fileName).exists()) {
        ui->txtPrecompileHeader->setText(fileName);
    }
}

