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
#include "editorsyntaxcheckwidget.h"
#include "ui_editorsyntaxcheckwidget.h"
#include "../settings.h"

EditorSyntaxCheckWidget::EditorSyntaxCheckWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorSyntaxCheckWidget)
{
    ui->setupUi(this);
}

EditorSyntaxCheckWidget::~EditorSyntaxCheckWidget()
{
    delete ui;
}

void EditorSyntaxCheckWidget::doLoad()
{
    //Auto Syntax Check
    ui->grpEnableAutoSyntaxCheck->setChecked(pSettings->editor().syntaxCheck());
    ui->chkSyntaxCheckWhenSave->setChecked(pSettings->editor().syntaxCheckWhenSave());
    ui->chkSyntaxCheckWhenLineChanged->setChecked(pSettings->editor().syntaxCheckWhenLineChanged());
}

void EditorSyntaxCheckWidget::doSave()
{
    //Auto Syntax Check
    pSettings->editor().setSyntaxCheck(ui->grpEnableAutoSyntaxCheck->isChecked());
    pSettings->editor().setSyntaxCheckWhenSave(ui->chkSyntaxCheckWhenSave->isChecked());
    pSettings->editor().setSyntaxCheckWhenLineChanged(ui->chkSyntaxCheckWhenLineChanged->isChecked());

    pSettings->editor().save();
}
