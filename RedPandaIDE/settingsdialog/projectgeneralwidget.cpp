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
#include "projectgeneralwidget.h"
#include "ui_projectgeneralwidget.h"
#include "../project.h"
#include "../mainwindow.h"
#include "settings.h"
#include "../systemconsts.h"
#include "../iconsmanager.h"

#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QTextCodec>

ProjectGeneralWidget::ProjectGeneralWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectGeneralWidget)
{
    ui->setupUi(this);
}

ProjectGeneralWidget::~ProjectGeneralWidget()
{
    delete ui;
}

void ProjectGeneralWidget::refreshIcon()
{
    QPixmap icon(mIconPath);
    ui->lbIcon->setPixmap(icon);
}

void ProjectGeneralWidget::doLoad()
{
    std::shared_ptr<Project> project = pMainWindow->project();
    if (!project)
        return;
    ui->txtName->setText(project->name());
    ui->txtFileName->setText(project->filename());
    ui->txtOutputFile->setText(project->executable());

    int srcCount=0,headerCount=0,resCount=0,otherCount=0, totalCount=0;
    foreach (const PProjectUnit& unit, project->units()) {
        switch(getFileType(unit->fileName())) {
        case FileType::CSource:
        case FileType::CppSource:
            srcCount++;
            break;
        case FileType::CppHeader:
        case FileType::CHeader:
            headerCount++;
            break;
        case FileType::WindowsResourceSource:
            resCount++;
            break;
        default:
            otherCount++;
        }
        totalCount++;
    }
    ui->lblFiles->setText(tr("%1 files [ %2 sources, %3 headers, %4 resources, %5 other files ]")
                          .arg(totalCount).arg(srcCount).arg(headerCount)
                          .arg(resCount).arg(otherCount));

    ui->cbDefaultEncoding->setCurrentText(project->options().encoding);

    ui->lstType->setCurrentRow( static_cast<int>(project->options().type));

    ui->cbDefaultCpp->setChecked(project->options().useGPP);
    ui->cbSupportXPTheme->setChecked(project->options().supportXPThemes);
    mIconPath = project->options().icon;
    QPixmap icon(mIconPath);
    refreshIcon();
}

void ProjectGeneralWidget::doSave()
{
    std::shared_ptr<Project> project = pMainWindow->project();
    if (!project)
        return;
    project->setName(ui->txtName->text().trimmed());

    project->options().encoding = ui->cbDefaultEncoding->currentText();

    int row = std::max(0,ui->lstType->currentRow());
    project->options().type = static_cast<ProjectType>(row);

    project->options().useGPP = ui->cbDefaultCpp->isChecked();
    project->options().supportXPThemes = ui->cbSupportXPTheme->isChecked();
    if (mIconPath.isEmpty()
            || !ui->lbIcon->pixmap() || ui->lbIcon->pixmap()->isNull()) {
        project->options().icon = "";
    } else {
        QString iconPath = changeFileExt(project->filename(),"ico");
        if (iconPath!=mIconPath) {
            if (QFile(iconPath).exists()) {
                if (!QFile::remove(iconPath)) {
                    QMessageBox::critical(this,
                                          tr("Can't remove old icon file"),
                                          tr("Can't remove old icon file '%1'")
                                          .arg(iconPath),
                                          QMessageBox::Ok);
                }
            }
            QFile::copy(mIconPath, iconPath);
            project->options().icon = iconPath;
            mIconPath = iconPath;
            refreshIcon();
        }
    }

    project->saveOptions();
}

void ProjectGeneralWidget::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select icon file"),
                                                    pSettings->dirs().appDir(),
                                                    tr("Image Files (*.ico *.png *.jpg)"));
    if (!fileName.isEmpty()) {
        mIconPath = fileName;
        QPixmap icon(mIconPath);
        refreshIcon();
        setSettingsChanged();
    }
    ui->btnRemove->setEnabled(!mIconPath.isEmpty());
}


void ProjectGeneralWidget::on_btnRemove_clicked()
{
    mIconPath = "";
    ui->lbIcon->setPixmap(QPixmap());
    ui->btnRemove->setEnabled(!mIconPath.isEmpty());
    setSettingsChanged();
}

void ProjectGeneralWidget::init()
{
    ui->cbDefaultEncoding->clear();
    ui->cbDefaultEncoding->addItems(pSystemConsts->codecNames());
    SettingsWidget::init();
}

void ProjectGeneralWidget::updateIcons(const QSize &)
{
    pIconsManager->setIcon(ui->btnBrowse,IconsManager::ACTION_FILE_OPEN_FOLDER);
    pIconsManager->setIcon(ui->btnRemove, IconsManager::ACTION_MISC_CROSS);
}

