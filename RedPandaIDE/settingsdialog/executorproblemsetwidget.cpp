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

    ui->cbFont->setCurrentFont(QFont(pSettings->executor().caseEditorFontName()));
    ui->spinFontSize->setValue(pSettings->executor().caseEditorFontSize());
    ui->chkOnlyMonospaced->setChecked(pSettings->executor().caseEditorFontOnlyMonospaced());
}

void ExecutorProblemSetWidget::doSave()
{
    pSettings->executor().setEnableProblemSet(ui->grpProblemSet->isChecked());
    pSettings->executor().setEnableCompetitiveCompanion(ui->grpCompetitiveCompanion->isChecked());
    pSettings->executor().setCompetivieCompanionPort(ui->spinPortNumber->value());
    pSettings->executor().setIgnoreSpacesWhenValidatingCases(ui->chkIgnoreSpacesWhenValidatingCases->isChecked());
    pSettings->executor().setCaseEditorFontName(ui->cbFont->currentFont().family());
    pSettings->executor().setCaseEditorFontOnlyMonospaced(ui->chkOnlyMonospaced->isChecked());
    pSettings->executor().setCaseEditorFontSize(ui->spinFontSize->value());
    pSettings->executor().save();
    pMainWindow->applySettings();
}

void ExecutorProblemSetWidget::on_chkOnlyMonospaced_stateChanged(int )
{
    if (ui->chkOnlyMonospaced->isChecked()) {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::MonospacedFonts);
    } else {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::AllFonts);
    }
}

