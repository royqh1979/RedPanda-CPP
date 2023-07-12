/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
    ui->chkConvertInputHTML->setChecked(pSettings->executor().convertHTMLToTextForInput());
    ui->chkConvertExpectedHTML->setChecked(pSettings->executor().convertHTMLToTextForExpected());

    ui->chkIgnoreSpacesWhenValidatingCases->setChecked(pSettings->executor().ignoreSpacesWhenValidatingCases());
    ui->chkRedirectStderr->setChecked(pSettings->executor().redirectStderrToToolLog());

    ui->cbFont->setCurrentFont(QFont(pSettings->executor().caseEditorFontName()));
    ui->spinFontSize->setValue(pSettings->executor().caseEditorFontSize());
    ui->chkOnlyMonospaced->setChecked(pSettings->executor().caseEditorFontOnlyMonospaced());
    ui->grpEnableTimeout->setChecked(pSettings->executor().enableCaseLimit());

    ui->spinCaseTimeout->setValue(pSettings->executor().caseTimeout());
    ui->spinMemoryLimit->setValue(pSettings->executor().caseMemoryLimit());
}

void ExecutorProblemSetWidget::doSave()
{
    pSettings->executor().setEnableProblemSet(ui->grpProblemSet->isChecked());
    pSettings->executor().setEnableCompetitiveCompanion(ui->grpCompetitiveCompanion->isChecked());
    pSettings->executor().setCompetivieCompanionPort(ui->spinPortNumber->value());
    pSettings->executor().setConvertHTMLToTextForInput(ui->chkConvertInputHTML->isChecked());
    pSettings->executor().setConvertHTMLToTextForExpected(ui->chkConvertExpectedHTML->isChecked());
    pSettings->executor().setIgnoreSpacesWhenValidatingCases(ui->chkIgnoreSpacesWhenValidatingCases->isChecked());
    pSettings->executor().setRedirectStderrToToolLog(ui->chkRedirectStderr->isChecked());
    pSettings->executor().setCaseEditorFontName(ui->cbFont->currentFont().family());
    pSettings->executor().setCaseEditorFontOnlyMonospaced(ui->chkOnlyMonospaced->isChecked());
    pSettings->executor().setCaseEditorFontSize(ui->spinFontSize->value());
    pSettings->executor().setEnableCaseLimit(ui->grpEnableTimeout->isChecked());
    pSettings->executor().setCaseTimeout(ui->spinCaseTimeout->value());
    pSettings->executor().setCaseMemoryLimit(ui->spinMemoryLimit->value());

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

