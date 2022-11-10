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
#include "projectmakefilewidget.h"
#include "ui_projectmakefilewidget.h"
#include "compilersetdirectorieswidget.h"
#include "../mainwindow.h"
#include "../project.h"
#include "../widgets/custommakefileinfodialog.h"
#include "../iconsmanager.h"
#include "../systemconsts.h"

#include <QFileDialog>

ProjectMakefileWidget::ProjectMakefileWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectMakefileWidget)
{
    ui->setupUi(this);

    mIncludesDirWidget = new CompilerSetDirectoriesWidget(this);
    ui->verticalLayout->addWidget(mIncludesDirWidget);
}

ProjectMakefileWidget::~ProjectMakefileWidget()
{
    delete ui;
}

void ProjectMakefileWidget::doLoad()
{
    ui->grpCustomMakefile->setChecked(pMainWindow->project()->options().useCustomMakefile);
    ui->txtCustomMakefile->setText(pMainWindow->project()->options().customMakefile);
    mIncludesDirWidget->setDirList(pMainWindow->project()->options().makeIncludes);
}

void ProjectMakefileWidget::doSave()
{
    pMainWindow->project()->options().useCustomMakefile = ui->grpCustomMakefile->isChecked();
    pMainWindow->project()->options().customMakefile = ui->txtCustomMakefile->text();
    pMainWindow->project()->options().makeIncludes = mIncludesDirWidget->dirList();
    pMainWindow->project()->saveOptions();

}

void ProjectMakefileWidget::on_btnBrowse_clicked()
{
    QString currentFile=ui->txtCustomMakefile->text();
    if (currentFile.isEmpty()) {
        currentFile = pMainWindow->project()->directory();
    }
    QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Custom makefile"),
                currentFile,
                tr("All files (%1)").arg(ALL_FILE_WILDCARD));
    if (!fileName.isEmpty() && QFileInfo(fileName).exists()) {
        ui->txtCustomMakefile->setText(fileName);
    }
}


void ProjectMakefileWidget::updateIcons(const QSize &)
{
    pIconsManager->setIcon(ui->btnBrowse, IconsManager::ACTION_FILE_OPEN_FOLDER);
    pIconsManager->setIcon(ui->btnInfo, IconsManager::ACTION_MISC_HELP);
}


void ProjectMakefileWidget::on_btnInfo_clicked()
{
    CustomMakefileInfoDialog dialog(this);
    dialog.exec();
}

