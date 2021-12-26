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
#include "editormiscwidget.h"
#include "ui_editormiscwidget.h"
#include "../settings.h"

EditorMiscWidget::EditorMiscWidget(const QString& name, const QString& group,
                                   QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorMiscWidget)
{
    ui->setupUi(this);
}

EditorMiscWidget::~EditorMiscWidget()
{
    delete ui;
}

void EditorMiscWidget::doLoad()
{
    ui->chkReadonlySystemHeaders->setChecked(pSettings->editor().readOnlySytemHeader());
    ui->chkLoadLastFiles->setChecked(pSettings->editor().autoLoadLastFiles());
    if (pSettings->editor().defaultFileCpp()) {
        ui->rbCppFile->setChecked(true);
    } else {
        ui->rbCFile->setChecked(true);
    }
    ui->chkUseUTF8ByDefault->setChecked(pSettings->editor().useUTF8ByDefault());
}

void EditorMiscWidget::doSave()
{
    pSettings->editor().setReadOnlySytemHeader(ui->chkReadonlySystemHeaders->isChecked());
    pSettings->editor().setAutoLoadLastFiles(ui->chkLoadLastFiles->isChecked());
    pSettings->editor().setDefaultFileCpp(ui->rbCppFile->isChecked());
    pSettings->editor().setUseUTF8ByDefault(ui->chkUseUTF8ByDefault->isChecked());
    pSettings->editor().save();
}
