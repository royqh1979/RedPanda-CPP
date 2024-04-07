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
#include "environmentperformancewidget.h"
#include "ui_environmentperformancewidget.h"
#include "../settings.h"

EnvironmentPerformanceWidget::EnvironmentPerformanceWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentPerformanceWidget)
{
    ui->setupUi(this);
    //ui->chkClearWhenEditorHidden->setVisible(false);
}

EnvironmentPerformanceWidget::~EnvironmentPerformanceWidget()
{
    delete ui;
}

void EnvironmentPerformanceWidget::doLoad()
{
//    ui->chkClearWhenEditorHidden->setChecked(pSettings->codeCompletion().clearWhenEditorHidden());
//#ifdef Q_OS_WIN
//    MEMORYSTATUSEX statex;

//    statex.dwLength = sizeof (statex);

//    GlobalMemoryStatusEx (&statex);
//    if (statex.ullTotalPhys < (long long int)2*1024*1024*1024) {
//        ui->chkClearWhenEditorHidden->setEnabled(false);
//        ui->chkClearWhenEditorHidden->setChecked(true);
//        pSettings->codeCompletion().setClearWhenEditorHidden(true);
//        pSettings->codeCompletion().save();
//    }
//    if (statex.ullTotalPhys < (long long int)1024*1024*1024) {
//        ui->chkEditorsShareParser->setEnabled(false);
//        ui->chkEditorsShareParser->setChecked(true);
//        pSettings->codeCompletion().setShareParser(true);
//        pSettings->codeCompletion().save();
//    }
//#endif
    ui->chkEditorsShareParser->setChecked(pSettings->codeCompletion().shareParser());
}

void EnvironmentPerformanceWidget::doSave()
{
    //pSettings->codeCompletion().setClearWhenEditorHidden(ui->chkClearWhenEditorHidden->isChecked());
    pSettings->codeCompletion().setShareParser(ui->chkEditorsShareParser->isChecked());

    pSettings->codeCompletion().save();
    pSettings->editor().save();
}
