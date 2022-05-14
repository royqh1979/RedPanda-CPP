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
#include "projectcompilerwidget.h"
#include "ui_projectcompilerwidget.h"
#include "../settings.h"
#include "../project.h"
#include "../mainwindow.h"

ProjectCompilerWidget::ProjectCompilerWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectCompilerWidget)
{
    ui->setupUi(this);
}

ProjectCompilerWidget::~ProjectCompilerWidget()
{
    delete ui;
}

void ProjectCompilerWidget::refreshOptions()
{
    Settings::PCompilerSet pSet = pSettings->compilerSets().getSet(ui->cbCompilerSet->currentIndex());
    if (!pSet)
        return;
    ui->chkAddCharset->setVisible(pSet->compilerType()!=COMPILER_CLANG);
    ui->chkAddCharset->setEnabled(pSet->compilerType()!=COMPILER_CLANG);
    mOptions = pMainWindow->project()->options().compilerOptions;
    if (mOptions.isEmpty())
        mOptions = pSet->compileOptions();

    ui->tabOptions->resetUI(pSet,mOptions);

    ui->chkStaticLink->setChecked(pSet->staticLink());
}

void ProjectCompilerWidget::doLoad()
{
    ui->chkAddCharset->setChecked(pMainWindow->project()->options().addCharset);
    ui->chkStaticLink->setChecked(pMainWindow->project()->options().staticLink);

    mOptions = pMainWindow->project()->options().compilerOptions;
    ui->cbCompilerSet->setCurrentIndex(pMainWindow->project()->options().compilerSet);
}

void ProjectCompilerWidget::doSave()
{
    Settings::PCompilerSet pSet = pSettings->compilerSets().getSet(ui->cbCompilerSet->currentIndex());
    if (!pSet)
        return;

    pMainWindow->project()->setCompilerSet(ui->cbCompilerSet->currentIndex());
    pMainWindow->project()->options().compilerOptions = ui->tabOptions->arguments(true);
    if (pSet->compilerType()!=COMPILER_CLANG)
        pMainWindow->project()->options().addCharset = ui->chkAddCharset->isChecked();
    pMainWindow->project()->options().staticLink = ui->chkStaticLink->isChecked();
    pMainWindow->project()->saveOptions();
}

void ProjectCompilerWidget::init()
{
    ui->cbCompilerSet->clear();
    for (size_t i=0;i<pSettings->compilerSets().size();i++) {
        ui->cbCompilerSet->addItem(pSettings->compilerSets().getSet(i)->name());
    }
    SettingsWidget::init();
}

void ProjectCompilerWidget::on_cbCompilerSet_currentIndexChanged(int)
{
    refreshOptions();
}

