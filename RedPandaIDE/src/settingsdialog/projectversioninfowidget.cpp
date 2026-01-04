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
#include "projectversioninfowidget.h"
#include "ui_projectversioninfowidget.h"
#include <windows.h>
#include <iterator>
#include "../mainwindow.h"
#include "../project.h"

static QStringList languageNames;
static QList<int> languageIDs;

static BOOL CALLBACK localeEnumProc(
  _In_ wchar_t *lpLocaleString
        ) {

    QString s = QString::fromWCharArray(lpLocaleString);
    bool ok;
    int aid = s.mid(4,4).toInt(&ok,16);
    if (ok) {
        wchar_t buffer [1024];
        GetLocaleInfoW(aid, LOCALE_SLANGUAGE, buffer, sizeof(buffer) / sizeof(wchar_t));
        languageNames.append(QString::fromWCharArray(buffer));
        languageIDs.append(aid);
    }
    return TRUE;
}

ProjectVersionInfoWidget::ProjectVersionInfoWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectVersionInfoWidget)
{
    ui->setupUi(this);
    if (languageNames.isEmpty()) {
        EnumSystemLocalesW(localeEnumProc, LCID_SUPPORTED);
    }
    ui->cbLanguage->addItems(languageNames);
}

ProjectVersionInfoWidget::~ProjectVersionInfoWidget()
{
    delete ui;
}

void ProjectVersionInfoWidget::doLoad()
{
    std::shared_ptr<Project> project = pMainWindow->project();
    ui->grpVersionInfo->setChecked(project->options().includeVersionInfo);
    ui->spinMajor->setValue(project->options().versionInfo.major);
    ui->spinMinor->setValue(project->options().versionInfo.minor);
    ui->spinRelease->setValue(project->options().versionInfo.release);
    ui->spinBuild->setValue(project->options().versionInfo.build);
    ui->chkAutoIncreaseBuildNumber->setChecked(project->options().versionInfo.autoIncBuildNr);
    ui->chkSyncProductWithFile->setChecked(project->options().versionInfo.syncProduct);
    for (int i=0;i<languageIDs.count();i++) {
        if (languageIDs[i] == project->options().versionInfo.languageID) {
            ui->cbLanguage->setCurrentIndex(i);
            break;
        }
    }
    ui->txtFileDescription->setText(project->options().versionInfo.fileDescription);
    ui->txtFileVersion->setText(project->options().versionInfo.fileVersion);
    ui->txtProductName->setText(project->options().versionInfo.productName);
    ui->txtProductVersion->setText(project->options().versionInfo.productVersion);
    ui->txtOriginalFilename->setText(project->options().versionInfo.originalFilename);
    ui->txtInternalFilename->setText(project->options().versionInfo.internalName);
    ui->txtCompanyName->setText(project->options().versionInfo.companyName);
    ui->txtLegalCopyright->setText(project->options().versionInfo.legalCopyright);
    ui->txtLegalTrademarks->setText(project->options().versionInfo.legalTrademarks);
}

void ProjectVersionInfoWidget::doSave()
{
    std::shared_ptr<Project> project = pMainWindow->project();
    project->options().includeVersionInfo = ui->grpVersionInfo->isChecked();
    project->options().versionInfo.major = ui->spinMajor->value();
    project->options().versionInfo.minor = ui->spinMinor->value();
    project->options().versionInfo.release = ui->spinRelease->value();
    project->options().versionInfo.build = ui->spinBuild->value();
    project->options().versionInfo.autoIncBuildNr = ui->chkAutoIncreaseBuildNumber->isChecked();
    project->options().versionInfo.syncProduct = ui->chkSyncProductWithFile->isChecked();
    if (ui->cbLanguage->currentIndex()>=0)
        project->options().versionInfo.languageID = languageIDs[ui->cbLanguage->currentIndex()];

    project->options().versionInfo.fileDescription = ui->txtFileDescription->text();
    project->options().versionInfo.fileVersion = ui->txtFileVersion->text();
    project->options().versionInfo.productName = ui->txtProductName->text();
    project->options().versionInfo.productVersion = ui->txtProductVersion->text();
    project->options().versionInfo.originalFilename = ui->txtOriginalFilename->text();
    project->options().versionInfo.internalName = ui->txtInternalFilename->text();
    project->options().versionInfo.companyName = ui->txtCompanyName->text();
    project->options().versionInfo.legalCopyright = ui->txtLegalCopyright->text();
    project->options().versionInfo.legalTrademarks = ui->txtLegalTrademarks->text();
    project->saveOptions();
}
