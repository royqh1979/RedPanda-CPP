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
#include "editortooltipswidget.h"
#include "ui_editortooltipswidget.h"
#include "../settings.h"

EditorTooltipsWidget::EditorTooltipsWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorTooltipsWidget)
{
    ui->setupUi(this);
}

EditorTooltipsWidget::~EditorTooltipsWidget()
{
    delete ui;
}

void EditorTooltipsWidget::doLoad()
{
    ui->chkShowFunctionTips->setChecked(pSettings->editor().showFunctionTips());
    ui->grpEnableTooltips->setChecked(pSettings->editor().enableTooltips());
    ui->chkIssueTooltips->setChecked(pSettings->editor().enableIssueToolTips());
    ui->chkIdentifierTooltips->setChecked(pSettings->editor().enableIdentifierToolTips());
    ui->chkHeaderTooltips->setChecked(pSettings->editor().enableHeaderToolTips());
    ui->chkDebugTooltips->setChecked(pSettings->editor().enableDebugTooltips());

}

void EditorTooltipsWidget::doSave()
{
    pSettings->editor().setShowFunctionTips(ui->chkShowFunctionTips->isChecked());
    pSettings->editor().setEnableTooltips(ui->grpEnableTooltips->isChecked());
    pSettings->editor().setEnableIssueToolTips(ui->chkIssueTooltips->isChecked());
    pSettings->editor().setEnableIdentifierToolTips(ui->chkIdentifierTooltips->isChecked());
    pSettings->editor().setEnableHeaderToolTips(ui->chkHeaderTooltips->isChecked());
    pSettings->editor().setEnableDebugTooltips(ui->chkDebugTooltips->isChecked());
    pSettings->editor().save();
}
