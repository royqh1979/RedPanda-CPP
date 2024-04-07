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
#include "editorcodecompletionwidget.h"
#include "ui_editorcodecompletionwidget.h"
#include "../settings.h"
#include "../mainwindow.h"
#include "../symbolusagemanager.h"

EditorCodeCompletionWidget::EditorCodeCompletionWidget(const QString& name, const QString& group,
                                                       QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorCodeCompletionWidget)
{
    ui->setupUi(this);
    ui->chkClearWhenEditorHidden->setVisible(false);
}

EditorCodeCompletionWidget::~EditorCodeCompletionWidget()
{
    delete ui;
}

void EditorCodeCompletionWidget::doLoad()
{
    ui->grpEnabled->setChecked(pSettings->codeCompletion().enabled());

    ui->chkParseLocalFiles->setChecked(pSettings->codeCompletion().parseLocalHeaders());
    ui->chkParseSystemFiles->setChecked(pSettings->codeCompletion().parseGlobalHeaders());

    ui->spinWidth->setValue(pSettings->codeCompletion().widthInColumns());
    ui->spinHeight->setValue(pSettings->codeCompletion().heightInLines());

    ui->chkShowSuggestionWhileTyping->setChecked(pSettings->codeCompletion().showCompletionWhileInput());
    ui->chkRecordUsage->setChecked(pSettings->codeCompletion().recordUsage());
    ui->chkSortByScope->setChecked(pSettings->codeCompletion().sortByScope());
    ui->chkShowKeywords->setChecked(pSettings->codeCompletion().showKeywords());
    ui->chkIgnoreCases->setChecked(pSettings->codeCompletion().ignoreCase());
    ui->chkAppendFunc->setChecked(pSettings->codeCompletion().appendFunc());
    ui->chkShowCodeIns->setChecked(pSettings->codeCompletion().showCodeIns());
//    ui->chkClearWhenEditorHidden->setChecked(pSettings->codeCompletion().clearWhenEditorHidden());
    ui->chkHideSymbolsStartWithTwoUnderline->setChecked(pSettings->codeCompletion().hideSymbolsStartsWithTwoUnderLine());
    ui->chkHideSymbolsStartWithUnderline->setChecked(pSettings->codeCompletion().hideSymbolsStartsWithUnderLine());

    ui->chkEditorShareCodeParser->setChecked(pSettings->codeCompletion().shareParser());
    ui->spinMinCharRequired->setValue(pSettings->codeCompletion().minCharRequired());
}

void EditorCodeCompletionWidget::doSave()
{
    //font
    pSettings->codeCompletion().setEnabled(ui->grpEnabled->isChecked());

    pSettings->codeCompletion().setParseLocalHeaders(ui->chkParseLocalFiles->isChecked());
    pSettings->codeCompletion().setParseGlobalHeaders(ui->chkParseSystemFiles->isChecked());

    pSettings->codeCompletion().setWidthInColumns(ui->spinWidth->value());
    pSettings->codeCompletion().setHeightInLines(ui->spinHeight->value());

    pSettings->codeCompletion().setShowCompletionWhileInput(ui->chkShowSuggestionWhileTyping->isChecked());
    pSettings->codeCompletion().setRecordUsage(ui->chkRecordUsage->isChecked());
    pSettings->codeCompletion().setSortByScope(ui->chkSortByScope->isChecked());
    pSettings->codeCompletion().setShowKeywords(ui->chkShowKeywords->isChecked());
    pSettings->codeCompletion().setIgnoreCase(ui->chkIgnoreCases->isChecked());
    pSettings->codeCompletion().setAppendFunc(ui->chkAppendFunc->isChecked());
    pSettings->codeCompletion().setShowCodeIns(ui->chkShowCodeIns->isChecked());
    pSettings->codeCompletion().setMinCharRequired(ui->spinMinCharRequired->value());

    //pSettings->codeCompletion().setClearWhenEditorHidden(ui->chkClearWhenEditorHidden->isChecked());

    pSettings->codeCompletion().setHideSymbolsStartsWithTwoUnderLine(ui->chkHideSymbolsStartWithTwoUnderline->isChecked());
    pSettings->codeCompletion().setHideSymbolsStartsWithUnderLine(ui->chkHideSymbolsStartWithUnderline->isChecked());

    pSettings->codeCompletion().setShareParser(ui->chkEditorShareCodeParser->isChecked());

    pSettings->codeCompletion().save();
}


void EditorCodeCompletionWidget::on_btnClearUsageData_clicked()
{
    pMainWindow->symbolUsageManager()->reset();
}

