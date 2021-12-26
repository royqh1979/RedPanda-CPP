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
#include "editorsymbolcompletionwidget.h"
#include "ui_editorsymbolcompletionwidget.h"
#include "../settings.h"

EditorSymbolCompletionWidget::EditorSymbolCompletionWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorSymbolCompletionWidget)
{
    ui->setupUi(this);
}

EditorSymbolCompletionWidget::~EditorSymbolCompletionWidget()
{
    delete ui;
}

void EditorSymbolCompletionWidget::doLoad()
{
    ui->grpCompleSymbols->setChecked(pSettings->editor().completeSymbols());
    ui->chkCompleteBrace->setChecked(pSettings->editor().completeBrace());
    ui->chkCompleteBracket->setChecked(pSettings->editor().completeBracket());
    ui->chkCompleteComments->setChecked(pSettings->editor().completeComment());
    ui->chkCompleteDoubleQuotation->setChecked(pSettings->editor().completeDoubleQuote());
    ui->chkCompleteGlobalInclude->setChecked(pSettings->editor().completeGlobalInclude());
    ui->chkCompleteParenthesis->setChecked(pSettings->editor().completeParenthese());
    ui->chkCompleteSingleQuotation->setChecked(pSettings->editor().completeSingleQuote());
    ui->chkRemoveSymbolPairs->setChecked(pSettings->editor().removeSymbolPairs());
    ui->chkSkipMathingSymbols->setChecked(pSettings->editor().overwriteSymbols());
}

void EditorSymbolCompletionWidget::doSave()
{
    pSettings->editor().setCompleteSymbols(ui->grpCompleSymbols->isChecked());
    pSettings->editor().setCompleteBrace(ui->chkCompleteBrace->isChecked());
    pSettings->editor().setCompleteBracket(ui->chkCompleteBracket->isChecked());
    pSettings->editor().setCompleteComment(ui->chkCompleteComments->isChecked());
    pSettings->editor().setCompleteDoubleQuote(ui->chkCompleteDoubleQuotation->isChecked());
    pSettings->editor().setCompleteGlobalInclude(ui->chkCompleteGlobalInclude->isChecked());
    pSettings->editor().setCompleteParenthese(ui->chkCompleteParenthesis->isChecked());
    pSettings->editor().setCompleteSingleQuote(ui->chkCompleteSingleQuotation->isChecked());
    pSettings->editor().setRemoveSymbolPairs(ui->chkRemoveSymbolPairs->isChecked());
    pSettings->editor().setOverwriteSymbols(ui->chkSkipMathingSymbols->isChecked());

    pSettings->editor().save();
}
