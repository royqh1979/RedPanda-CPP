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
#include <qt_utils/charsetinfo.h>

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
    ui->panelAddCharset->setVisible(pSet->compilerType()!=CompilerType::Clang);
    //ui->chkAddCharset->setEnabled(pSet->compilerType()!=COMPILER_CLANG);
    mOptions = pMainWindow->project()->options().compilerOptions;
    if (mOptions.isEmpty())
        mOptions = pSet->compileOptions();

    ui->tabOptions->resetUI(pSet,mOptions);

    ui->chkStaticLink->setChecked(pSet->staticLink());

    QByteArray execEncoding = pMainWindow->project()->options().execEncoding;
    if (execEncoding == ENCODING_AUTO_DETECT
            || execEncoding == ENCODING_SYSTEM_DEFAULT
            || execEncoding == ENCODING_UTF8) {
        int index =ui->cbEncoding->findData(execEncoding);
        ui->cbEncoding->setCurrentIndex(index);
        ui->cbEncodingDetails->clear();
        ui->cbEncodingDetails->setVisible(false);
    } else {
        QString encoding = execEncoding;
        QString language = pCharsetInfoManager->findLanguageByCharsetName(encoding);
        ui->cbEncoding->setCurrentText(language);
        ui->cbEncodingDetails->setVisible(true);
        ui->cbEncodingDetails->clear();
        QList<PCharsetInfo> infos = pCharsetInfoManager->findCharsetsByLanguageName(language);
        foreach (const PCharsetInfo& info, infos) {
            ui->cbEncodingDetails->addItem(info->name);
        }
        ui->cbEncodingDetails->setCurrentText(encoding);
    }
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
    if (pSet->compilerType()!=CompilerType::Clang)
        pMainWindow->project()->options().addCharset = ui->chkAddCharset->isChecked();
    pMainWindow->project()->options().staticLink = ui->chkStaticLink->isChecked();

    if (ui->cbEncodingDetails->isVisible()) {
        pMainWindow->project()->options().execEncoding = ui->cbEncodingDetails->currentText().toLocal8Bit();
    } else {
        pMainWindow->project()->options().execEncoding = ui->cbEncoding->currentData().toString().toLocal8Bit();
    }
    pMainWindow->project()->saveOptions();
}

void ProjectCompilerWidget::init()
{
    ui->cbCompilerSet->clear();
    for (size_t i=0;i<pSettings->compilerSets().size();i++) {
        ui->cbCompilerSet->addItem(pSettings->compilerSets().getSet(i)->name());
    }
    ui->cbEncodingDetails->setVisible(false);
    ui->cbEncoding->clear();
    ui->cbEncoding->addItem(tr("ANSI"),ENCODING_SYSTEM_DEFAULT);
    ui->cbEncoding->addItem(tr("UTF-8"),ENCODING_UTF8);
    foreach (const QString& langName, pCharsetInfoManager->languageNames()) {
        ui->cbEncoding->addItem(langName,langName);
    }
    SettingsWidget::init();
}

void ProjectCompilerWidget::on_cbCompilerSet_currentIndexChanged(int)
{
    refreshOptions();
}

void ProjectCompilerWidget::on_cbEncoding_currentTextChanged(const QString &/*arg1*/)
{
    QString userData = ui->cbEncoding->currentData().toString();
    if (userData == ENCODING_AUTO_DETECT
            || userData == ENCODING_SYSTEM_DEFAULT
            || userData == ENCODING_UTF8) {
        ui->cbEncodingDetails->setVisible(false);
        ui->cbEncodingDetails->clear();
    } else {
        ui->cbEncodingDetails->setVisible(true);
        ui->cbEncodingDetails->clear();
        QList<PCharsetInfo> infos = pCharsetInfoManager->findCharsetsByLanguageName(userData);
        foreach (const PCharsetInfo& info, infos) {
            ui->cbEncodingDetails->addItem(info->name);
        }
    }
}


void ProjectCompilerWidget::on_cbEncodingDetails_currentTextChanged(const QString &/*arg1*/)
{

}

