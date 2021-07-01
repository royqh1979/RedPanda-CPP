#include "executorgeneralwidget.h"
#include "ui_executorgeneralwidget.h"
#include "../settings.h"

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
}

void ExecutorGeneralWidget::doSave()
{
    pSettings->executor().setPauseConsole(ui->chkPauseConsole->isChecked());
    pSettings->executor().setMinimizeOnRun(ui->chkMinimizeOnRun->isChecked());

    pSettings->executor().save();
}
