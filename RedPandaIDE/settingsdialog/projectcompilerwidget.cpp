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
#include <QMessageBox>

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

    ui->tabOptions->resetUI(pSet,mOptions);

    ui->chkStaticLink->setChecked(mStaticLink);
    ui->chkAddCharset->setChecked(mAddCharset);

    QByteArray execEncoding = mExecCharset;
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
    mOptions = pMainWindow->project()->options().compilerOptions;
    Settings::PCompilerSet pSet = pSettings->compilerSets().getSet(ui->cbCompilerSet->currentIndex());
    if (mOptions.isEmpty() && pSet)
        mOptions = pSet->compileOptions();
    mStaticLink = pMainWindow->project()->options().staticLink;
    mAddCharset = pMainWindow->project()->options().addCharset;
    mExecCharset = pMainWindow->project()->options().execEncoding;
    ui->cbCompilerSet->blockSignals(true);
    ui->cbCompilerSet->setCurrentIndex(pMainWindow->project()->options().compilerSet);
    ui->cbCompilerSet->blockSignals(false);
    refreshOptions();
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
    mOptions = pMainWindow->project()->options().compilerOptions;
    mStaticLink = pMainWindow->project()->options().staticLink;
    mAddCharset = pMainWindow->project()->options().addCharset;
    mExecCharset = pMainWindow->project()->options().execEncoding;
    pMainWindow->project()->saveOptions();
}

void ProjectCompilerWidget::init()
{
    ui->cbCompilerSet->blockSignals(true);
    ui->cbCompilerSet->clear();
    for (size_t i=0;i<pSettings->compilerSets().size();i++) {
        ui->cbCompilerSet->addItem(pSettings->compilerSets().getSet(i)->name());
    }
    ui->cbCompilerSet->blockSignals(false);
    ui->cbEncodingDetails->setVisible(false);
    ui->cbEncoding->clear();
    ui->cbEncoding->addItem(tr("System Default(%1)").arg(QString(pCharsetInfoManager->getDefaultSystemEncoding())),ENCODING_SYSTEM_DEFAULT);
    ui->cbEncoding->addItem(tr("UTF-8"),ENCODING_UTF8);
    foreach (const QString& langName, pCharsetInfoManager->languageNames()) {
        ui->cbEncoding->addItem(langName,langName);
    }
    SettingsWidget::init();
}

void ProjectCompilerWidget::on_cbCompilerSet_currentIndexChanged(int index)
{
    if (index<0)
        return;
    std::shared_ptr<Project> project = pMainWindow->project();
    clearSettingsChanged();
    disconnectInputs();
    ui->cbCompilerSet->blockSignals(true);
    auto action = finally([this]{
        ui->cbCompilerSet->blockSignals(false);
        refreshOptions();
        connectInputs();
    });
    Settings::PCompilerSet pSet=pSettings->compilerSets().getSet(index);
#ifdef ENABLE_SDCC
    if (pSet) {
        if (project->options().type==ProjectType::MicroController) {
            if (pSet->compilerType()!=CompilerType::SDCC) {
                QMessageBox::information(this,
                                         tr("Wrong Compiler Type"),
                                         tr("Compiler %1 can't compile a microcontroller project.").arg(pSet->name())
                                         );
                ui->cbCompilerSet->setCurrentIndex(project->options().compilerSet);
                return;
            }
        } else {
            if (pSet->compilerType()==CompilerType::SDCC) {
                QMessageBox::information(this,
                                         tr("Wrong Compiler Type"),
                                         tr("Compiler %1 can only compile microcontroller project.").arg(pSet->name())
                                         );
                ui->cbCompilerSet->setCurrentIndex(project->options().compilerSet);
                return;
            }
        }
    }
#endif
    if (QMessageBox::warning(
                this,
                tr("Change Project Compiler Set"),
                tr("Change the project's compiler set will lose all custom compiler set options.")
                +"<br />"
                + tr("Do you really want to do that?"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes) {
        ui->cbCompilerSet->setCurrentIndex(project->options().compilerSet);
        return;
    }
    mOptions = pSet->compileOptions();
    mStaticLink = pSet->staticLink();
    mAddCharset = pSet->autoAddCharsetParams();
    mExecCharset = pSet->execCharset().toUtf8();

    setSettingsChanged();
    //project->saveOptions();
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

