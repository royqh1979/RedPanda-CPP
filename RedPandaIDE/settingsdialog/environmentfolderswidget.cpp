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
#include "environmentfolderswidget.h"
#include "ui_environmentfolderswidget.h"
#include "../settings.h"
#include "../mainwindow.h"
#include "../iconsmanager.h"

#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QUrl>

EnvironmentFoldersWidget::EnvironmentFoldersWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentFoldersWidget)
{
    ui->setupUi(this);
}

EnvironmentFoldersWidget::~EnvironmentFoldersWidget()
{
    delete ui;
}

void EnvironmentFoldersWidget::doLoad()
{
    ui->txtConfigFolder->setText(pSettings->dirs().config());
    ui->txtIconSetFolder->setEnabled(pSettings->environment().useCustomIconSet());
    ui->btnOpenIconSetFolderInFileBrowser->setEnabled(pSettings->environment().useCustomIconSet());
    if (pSettings->environment().useCustomIconSet()) {
        ui->txtIconSetFolder->setText(pSettings->dirs().config(Settings::Dirs::DataType::IconSet));
    }
    ui->txtThemeFolder->setText(pSettings->dirs().config(Settings::Dirs::DataType::Theme));
}

void EnvironmentFoldersWidget::doSave()
{
}

void EnvironmentFoldersWidget::on_btnOpenConfigFolderInBrowser_clicked()
{
    QDesktopServices::openUrl(
                QUrl("file:///"+
                     includeTrailingPathDelimiter(pSettings->dirs().config()),QUrl::TolerantMode));

}


void EnvironmentFoldersWidget::on_btnResetDefault_clicked()
{
    if (QMessageBox::question(this,tr("Confirm"),
                          tr("Do you really want to delete all custom settings?"),
                          QMessageBox::Yes|QMessageBox::No,
                          QMessageBox::No)!=QMessageBox::Yes)
        return;
    QDir dir(pSettings->dirs().config());
    if (!dir.removeRecursively()) {
        QMessageBox::critical(this,tr("Error"),
                              tr("Failed to delete custom settings."));
        return;
    }
    emit shouldQuitApp();
}

void EnvironmentFoldersWidget::updateIcons(const QSize &/*size*/)
{
    pIconsManager->setIcon(ui->btnOpenConfigFolderInBrowser,IconsManager::ACTION_FILE_OPEN_FOLDER);
    pIconsManager->setIcon(ui->btnOpenThemeFolderInFileBrowser,IconsManager::ACTION_FILE_OPEN_FOLDER);
    pIconsManager->setIcon(ui->btnOpenIconSetFolderInFileBrowser,IconsManager::ACTION_FILE_OPEN_FOLDER);
}


void EnvironmentFoldersWidget::on_btnOpenIconSetFolderInFileBrowser_clicked()
{
    QDesktopServices::openUrl(
                QUrl("file:///"+
                     includeTrailingPathDelimiter(pSettings->dirs().config(Settings::Dirs::DataType::IconSet)),QUrl::TolerantMode));

}


void EnvironmentFoldersWidget::on_btnOpenThemeFolderInFileBrowser_clicked()
{
    QString folderName = pSettings->dirs().config(Settings::Dirs::DataType::Theme);
    QDir folder=QDir{folderName};
    if (!folder.exists())
        folder.mkpath(folderName);
    QDesktopServices::openUrl(
                QUrl("file:///"+
                     includeTrailingPathDelimiter(folderName),QUrl::TolerantMode));
}

