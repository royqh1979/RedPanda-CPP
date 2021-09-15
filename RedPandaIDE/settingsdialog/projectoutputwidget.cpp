#include "projectoutputwidget.h"
#include "ui_projectoutputwidget.h"
#include "../mainwindow.h"
#include "../project.h"

#include <QFileDialog>


ProjectOutputWidget::ProjectOutputWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectOutputWidget)
{
    ui->setupUi(this);
}

ProjectOutputWidget::~ProjectOutputWidget()
{
    delete ui;
}

void ProjectOutputWidget::doLoad()
{
    ui->txtOutputDir->setText(pMainWindow->project()->options().exeOutput);
    ui->txtObjOutputDir->setText(pMainWindow->project()->options().objectOutput);
    ui->grpAutosaveCompileLog->setChecked(pMainWindow->project()->options().logOutputEnabled);
    ui->txtCompileLog->setText(pMainWindow->project()->options().logOutput);
    ui->grpOverrideOutput->setChecked(pMainWindow->project()->options().overrideOutput);
    ui->txtOutputFilename->setText(pMainWindow->project()->options().overridenOutput);
}

void ProjectOutputWidget::doSave()
{
    pMainWindow->project()->options().exeOutput = ui->txtOutputDir->text();
    pMainWindow->project()->options().objectOutput = ui->txtObjOutputDir->text();
    pMainWindow->project()->options().logOutputEnabled = ui->grpAutosaveCompileLog->isChecked();
    pMainWindow->project()->options().logOutput = ui->txtCompileLog->text();
    pMainWindow->project()->options().overrideOutput = ui->grpOverrideOutput->isChecked();
    pMainWindow->project()->options().overridenOutput = ui->txtOutputFilename->text();
    pMainWindow->project()->saveOptions();
}

void ProjectOutputWidget::on_btnOutputDir_triggered(QAction *)
{
    QString dirName = QFileDialog::getExistingDirectory(
                this,
                tr("Executable output directory"),
                pMainWindow->project()->directory());
    if (!dirName.isEmpty())
        ui->txtOutputDir->setText(dirName);
}


void ProjectOutputWidget::on_btnObjOutputDir_triggered(QAction *)
{
    QString dirName = QFileDialog::getExistingDirectory(
                this,
                tr("Object files output directory"),
                pMainWindow->project()->directory());
    if (!dirName.isEmpty())
        ui->txtObjOutputDir->setText(dirName);
}


void ProjectOutputWidget::on_btnCompileLog_triggered(QAction *)
{
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Log file"),
                pMainWindow->project()->directory(),
                tr("All files (*.*)"));
    if (!fileName.isEmpty() ) {
        ui->txtCompileLog->setText(fileName);
    }
}

