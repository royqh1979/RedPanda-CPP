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
#include "projectoutputwidget.h"
#include "ui_projectoutputwidget.h"
#include "../mainwindow.h"
#include "../project.h"
#include "../iconsmanager.h"
#include "../systemconsts.h"

#include <QFileDialog>


ProjectOutputWidget::ProjectOutputWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectOutputWidget)
{
    ui->setupUi(this);
}

ProjectOutputWidget::~ProjectOutputWidget()
{
    delete ui;
}

void ProjectOutputWidget::doLoad()
{
    ui->txtOutputDir->setText(pMainWindow->project()->options().folderForOutput);
    ui->txtObjOutputDir->setText(pMainWindow->project()->options().folderForObjFiles);
    ui->grpAutosaveCompileLog->setChecked(pMainWindow->project()->options().logOutput);
    ui->txtCompileLog->setText(pMainWindow->project()->options().logFilename);
    ui->grpOverrideOutput->setChecked(pMainWindow->project()->options().useCustomOutputFilename);
    ui->txtOutputFilename->setText(pMainWindow->project()->options().customOutputFilename);
}

void ProjectOutputWidget::doSave()
{
    pMainWindow->project()->options().folderForOutput = ui->txtOutputDir->text();
    pMainWindow->project()->options().folderForObjFiles = ui->txtObjOutputDir->text();
    pMainWindow->project()->options().logOutput = ui->grpAutosaveCompileLog->isChecked();
    pMainWindow->project()->options().logFilename = ui->txtCompileLog->text();
    pMainWindow->project()->options().useCustomOutputFilename = ui->grpOverrideOutput->isChecked();
    pMainWindow->project()->options().customOutputFilename = ui->txtOutputFilename->text();
    pMainWindow->project()->saveOptions();
}

void ProjectOutputWidget::on_btnOutputDir_clicked()
{
    QString currentName = ui->txtOutputDir->text();
    if (currentName.isEmpty())
        currentName = pMainWindow->project()->directory();
    QString dirName = QFileDialog::getExistingDirectory(
                this,
                tr("Executable output directory"),
                currentName);
    if (!dirName.isEmpty())
        ui->txtOutputDir->setText(dirName);
}


void ProjectOutputWidget::on_btnObjOutputDir_clicked()
{
    QString currentName = ui->txtObjOutputDir->text();
    if (currentName.isEmpty())
        currentName = pMainWindow->project()->directory();
    QString dirName = QFileDialog::getExistingDirectory(
                this,
                tr("Object files output directory"),
                currentName);
    if (!dirName.isEmpty())
        ui->txtObjOutputDir->setText(dirName);
}


void ProjectOutputWidget::on_btnCompileLog_clicked()
{
    QString currentFile=ui->txtCompileLog->text();
    if (currentFile.isEmpty()) {
        currentFile = pMainWindow->project()->directory();
    }
    QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Log file"),
                currentFile,
                tr("All files (%1)").arg(ALL_FILE_WILDCARD),
                nullptr,
                QFileDialog::Options() | QFileDialog::DontConfirmOverwrite);
    if (!fileName.isEmpty() ) {
        ui->txtCompileLog->setText(fileName);
    }
}

void ProjectOutputWidget::updateIcons(const QSize &/*size*/)
{
    pIconsManager->setIcon(ui->btnCompileLog, IconsManager::ACTION_FILE_OPEN_FOLDER);
    pIconsManager->setIcon(ui->btnObjOutputDir, IconsManager::ACTION_FILE_OPEN_FOLDER);
    pIconsManager->setIcon(ui->btnOutputDir, IconsManager::ACTION_FILE_OPEN_FOLDER);
}

