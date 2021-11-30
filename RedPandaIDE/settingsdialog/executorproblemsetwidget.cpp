#include "executorproblemsetwidget.h"
#include "ui_executorproblemsetwidget.h"
#include "../settings.h"
#include "../mainwindow.h"

ExecutorProblemSetWidget::ExecutorProblemSetWidget(const QString& name, const QString& group, QWidget *parent):
    SettingsWidget(name,group,parent),
    ui(new Ui::ExecutorProblemSetWidget)
{
    ui->setupUi(this);
}

ExecutorProblemSetWidget::~ExecutorProblemSetWidget()
{
    delete ui;
}

void ExecutorProblemSetWidget::doLoad()
{
    ui->grpProblemSet->setChecked(pSettings->executor().enableProblemSet());
    ui->grpCompetitiveCompanion->setChecked(pSettings->executor().enableCompetitiveCompanion());
    ui->spinPortNumber->setValue(pSettings->executor().competivieCompanionPort());
    ui->chkIgnoreSpacesWhenValidatingCases->setChecked(pSettings->executor().ignoreSpacesWhenValidatingCases());
}

void ExecutorProblemSetWidget::doSave()
{
    pSettings->executor().setEnableProblemSet(ui->grpProblemSet->isChecked());
    pSettings->executor().setEnableCompetitiveCompanion(ui->grpCompetitiveCompanion->isChecked());
    pSettings->executor().setCompetivieCompanionPort(ui->spinPortNumber->value());
    pSettings->executor().setIgnoreSpacesWhenValidatingCases(ui->chkIgnoreSpacesWhenValidatingCases->isChecked());
    pSettings->executor().save();
    pMainWindow->applySettings();
}
