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
#include "projectdirectorieswidget.h"
#include "ui_projectdirectorieswidget.h"
#include "compilersetdirectorieswidget.h"
#include "../project.h"
#include "../mainwindow.h"

ProjectDirectoriesWidget::ProjectDirectoriesWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectDirectoriesWidget)
{
    ui->setupUi(this);

    mBinDirWidget = new CompilerSetDirectoriesWidget();
    ui->tabDirs->addTab(mBinDirWidget,tr("Binaries"));
    mLibDirWidget = new CompilerSetDirectoriesWidget();
    ui->tabDirs->addTab(mLibDirWidget,tr("Libraries"));
    mIncludeDirWidget = new CompilerSetDirectoriesWidget();
    ui->tabDirs->addTab(mIncludeDirWidget,tr("Includes"));
    mResourceDirWidget = new CompilerSetDirectoriesWidget();
    ui->tabDirs->addTab(mResourceDirWidget,tr("Resources"));
}


ProjectDirectoriesWidget::~ProjectDirectoriesWidget()
{
    delete ui;
}

void ProjectDirectoriesWidget::doLoad()
{
    mBinDirWidget->setDirList(pMainWindow->project()->options().binDirs);
    mLibDirWidget->setDirList(pMainWindow->project()->options().libDirs);
    mIncludeDirWidget->setDirList(pMainWindow->project()->options().includeDirs);
    mResourceDirWidget->setDirList(pMainWindow->project()->options().resourceIncludes);

}

void ProjectDirectoriesWidget::doSave()
{
    pMainWindow->project()->options().binDirs = mBinDirWidget->dirList();
    pMainWindow->project()->options().libDirs = mLibDirWidget->dirList();
    pMainWindow->project()->options().includeDirs = mIncludeDirWidget->dirList();
    pMainWindow->project()->options().resourceIncludes = mResourceDirWidget->dirList();
    pMainWindow->project()->saveOptions();
}
