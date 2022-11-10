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
#include "projectprecompilewidget.h"
#include "ui_projectprecompilewidget.h"
#include "../mainwindow.h"
#include "../project.h"
#include "../iconsmanager.h"

#include <QFileDialog>

ProjectPreCompileWidget::ProjectPreCompileWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectPreCompileWidget)
{
    ui->setupUi(this);
}

ProjectPreCompileWidget::~ProjectPreCompileWidget()
{
    delete ui;
}

void ProjectPreCompileWidget::doLoad()
{
    ui->grpPrecompileHeader->setChecked(pMainWindow->project()->options().usePrecompiledHeader);
    ui->txtPrecompileHeader->setText(pMainWindow->project()->options().precompiledHeader);
}

void ProjectPreCompileWidget::doSave()
{
    pMainWindow->project()->options().usePrecompiledHeader = ui->grpPrecompileHeader->isChecked();
    pMainWindow->project()->options().precompiledHeader = ui->txtPrecompileHeader->text();
    pMainWindow->project()->saveOptions();
}

void ProjectPreCompileWidget::on_btnBrowse_clicked()
{
    QString currentFile=ui->txtPrecompileHeader->text();
    QString currentDir;
    if (currentFile.isEmpty()) {
        currentDir = pMainWindow->project()->directory();
    } else {
        currentDir = extractFilePath(currentFile);
    }
    QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Precompiled header"),
                currentDir,
                tr("precompiled header files (*.pch)"),
                nullptr,
                QFileDialog::Options()|QFileDialog::DontConfirmOverwrite);
    if (!fileName.isEmpty()) {
        ui->txtPrecompileHeader->setText(fileName);
    }
}

void ProjectPreCompileWidget::updateIcons(const QSize &/*size*/)
{
    pIconsManager->setIcon(ui->btnBrowse, IconsManager::ACTION_FILE_OPEN_FOLDER);
}

