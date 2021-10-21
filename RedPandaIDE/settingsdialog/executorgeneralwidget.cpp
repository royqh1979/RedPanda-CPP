#include "executorgeneralwidget.h"
#include "ui_executorgeneralwidget.h"
#include "../settings.h"

#include <QFileDialog>

ExecutorGeneralWidget::ExecutorGeneralWidget(const QString& name, const QString& group, QWidget *parent):
    SettingsWidget(name,group,parent),
    ui(new Ui::ExecutorGeneralWidget)
{
    ui->setupUi(this);
}

ExecutorGeneralWidget::~ExecutorGeneralWidget()
{
    delete ui;
}

void ExecutorGeneralWidget::doLoad()
{
    ui->chkPauseConsole->setChecked(pSettings->executor().pauseConsole());
    ui->chkMinimizeOnRun->setChecked(pSettings->executor().minimizeOnRun());
    ui->grpExecuteParameters->setChecked(pSettings->executor().useParams());
    ui->txtExecuteParamaters->setText(pSettings->executor().params());
    ui->grpRedirectInput->setChecked(pSettings->executor().redirectInput());
    ui->txtRedirectInputFile->setText(pSettings->executor().inputFilename());
}

void ExecutorGeneralWidget::doSave()
{
    pSettings->executor().setPauseConsole(ui->chkPauseConsole->isChecked());
    pSettings->executor().setMinimizeOnRun(ui->chkMinimizeOnRun->isChecked());
    pSettings->executor().setUseParams(ui->grpExecuteParameters->isChecked());
    pSettings->executor().setParams(ui->txtExecuteParamaters->text());
    pSettings->executor().setRedirectInput(ui->grpRedirectInput->isChecked());
    pSettings->executor().setInputFilename(ui->txtRedirectInputFile->text());

    pSettings->executor().save();
}

void ExecutorGeneralWidget::on_btnBrowse_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Choose input file"),
                QString(),
                tr("All files (*.*)"));
    if (!filename.isEmpty() && fileExists(filename)) {
        ui->txtRedirectInputFile->setText(filename);
    }
}

