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
#include "qt_utils/charsetinfo.h"

#include <QFileDialog>
#include <QIcon>
#include <QImageWriter>
#include <QImageWriter>
#include <QMessageBox>

ProjectGeneralWidget::ProjectGeneralWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectGeneralWidget)
{
    ui->setupUi(this);
    ui->cbType->addItems(
                {
                    "Win32 GUI",
                    "Win32 Console",
                    "Win32 Static Library",
                    "Win32 DLL",
                });
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
#ifdef ENABLE_SDCC
    bool isMicroControllerProject = (project->options().type==ProjectType::MicroController);
#else
    bool isMicroControllerProject = false;
#endif
    ui->cbType->setVisible(!isMicroControllerProject);
    ui->cbDefaultCpp->setVisible(!isMicroControllerProject);
    ui->cbSupportXPTheme->setVisible(!isMicroControllerProject);
    ui->grpIcon->setVisible(!isMicroControllerProject);
    ui->lblEncoding->setVisible(!isMicroControllerProject);
    ui->panelEncoding->setVisible(!isMicroControllerProject);

    ui->txtName->setText(project->name());
    ui->txtFileName->setText(project->filename());
    ui->txtOutputFile->setText(project->outputFilename());

    int srcCount=0,headerCount=0,resCount=0,otherCount=0, totalCount=0;
    foreach (const PProjectUnit& unit, project->unitList()) {
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

    QByteArray defaultEncoding = project->options().encoding;
    if (defaultEncoding == ENCODING_AUTO_DETECT
            || defaultEncoding == ENCODING_SYSTEM_DEFAULT
            || defaultEncoding == ENCODING_UTF8
            || defaultEncoding == ENCODING_UTF8_BOM) {
        int index =ui->cbEncoding->findData(defaultEncoding);
        ui->cbEncoding->setCurrentIndex(index);
        ui->cbEncodingDetail->clear();
        ui->cbEncodingDetail->setVisible(false);
    } else {
        QString language = pCharsetInfoManager->findLanguageByCharsetName(defaultEncoding);
        ui->cbEncoding->setCurrentText(language);
        ui->cbEncodingDetail->setVisible(true);
        ui->cbEncodingDetail->clear();
        QList<PCharsetInfo> infos = pCharsetInfoManager->findCharsetsByLanguageName(language);
        foreach (const PCharsetInfo& info, infos) {
            ui->cbEncodingDetail->addItem(info->name);
        }
        ui->cbEncodingDetail->setCurrentText(defaultEncoding);
    }

    ui->cbType->setCurrentIndex(static_cast<int>(project->options().type));

    ui->cbDefaultCpp->setChecked(project->options().isCpp);
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

    if (ui->cbEncodingDetail->isVisible()) {
        project->setEncoding(ui->cbEncodingDetail->currentText().toUtf8());
    } else {
        project->setEncoding(ui->cbEncoding->currentData().toByteArray());
    }

    int row = std::max(0,ui->cbType->currentIndex());
    project->options().type = static_cast<ProjectType>(row);

    project->options().isCpp = ui->cbDefaultCpp->isChecked();
    project->options().supportXPThemes = ui->cbSupportXPTheme->isChecked();
    if (mIconPath.isEmpty() || ui->lbIcon->pixmap(Qt::ReturnByValue).isNull()) {
        project->options().icon = "";
    } else {
        QString iconPath = generateAbsolutePath(project->directory(),"app.ico");
        if (iconPath!=mIconPath) {
            if (fileExists(iconPath)) {
                if (!QFile::remove(iconPath)) {
                    QMessageBox::critical(this,
                                          tr("Can't remove old icon file"),
                                          tr("Can't remove old icon file '%1'")
                                          .arg(iconPath),
                                          QMessageBox::Ok);
                    return;
                }
            }
            if (!mIconPath.endsWith(".ico",PATH_SENSITIVITY) && QImageWriter::supportedImageFormats().contains("ico")) {
                ui->lbIcon->pixmap(Qt::ReturnByValue).save(iconPath,"ico");
            } else
                copyFile(mIconPath, iconPath,true);
        }
        project->options().icon = iconPath;
        mIconPath = iconPath;
        refreshIcon();
    }

    project->saveOptions();
}

void ProjectGeneralWidget::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select icon file"),
                                                    pMainWindow->project()->directory(),
                                                    tr("Image Files (*.ico *.png *.jpg)"));
    if (!fileName.isEmpty()) {
        mIconPath = fileName;
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

void ProjectGeneralWidget::on_cbEncoding_currentTextChanged(const QString &/*arg1*/)
{
    QString userData = ui->cbEncoding->currentData().toString();
    if (userData == ENCODING_AUTO_DETECT
            || userData == ENCODING_SYSTEM_DEFAULT
            || userData == ENCODING_UTF8
            || userData == ENCODING_UTF8_BOM) {
        ui->cbEncodingDetail->setVisible(false);
        ui->cbEncodingDetail->clear();
    } else {
        ui->cbEncodingDetail->setVisible(true);
        ui->cbEncodingDetail->clear();
        QList<PCharsetInfo> infos = pCharsetInfoManager->findCharsetsByLanguageName(userData);
        foreach (const PCharsetInfo& info, infos) {
            ui->cbEncodingDetail->addItem(info->name);
        }
    }
}

void ProjectGeneralWidget::init()
{
    ui->cbEncodingDetail->setVisible(false);
    ui->cbEncoding->clear();
    ui->cbEncoding->addItem(tr("System Default(%1)").arg(QString(pCharsetInfoManager->getDefaultSystemEncoding())),ENCODING_SYSTEM_DEFAULT);
    ui->cbEncoding->addItem(tr("UTF-8"),ENCODING_UTF8);
    ui->cbEncoding->addItem(tr("UTF-8 BOM"),ENCODING_UTF8_BOM);
    foreach (const QString& langName, pCharsetInfoManager->languageNames()) {
        ui->cbEncoding->addItem(langName,langName);
    }
    SettingsWidget::init();
}

void ProjectGeneralWidget::updateIcons(const QSize &)
{
    pIconsManager->setIcon(ui->btnBrowse,IconsManager::ACTION_FILE_OPEN_FOLDER);
    pIconsManager->setIcon(ui->btnRemove, IconsManager::ACTION_MISC_CROSS);
}


void ProjectGeneralWidget::on_cbType_currentIndexChanged(int /*index*/)
{
    std::shared_ptr<Project> project = pMainWindow->project();
    if (!project)
        return;
    ui->txtOutputFile->setText(project->outputFilename());
}

