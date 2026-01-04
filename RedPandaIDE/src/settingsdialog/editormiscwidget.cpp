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
#include "editormiscwidget.h"
#include "ui_editormiscwidget.h"
#include "../settings.h"
#include "qt_utils/charsetinfo.h"
#include "../mainwindow.h"

EditorMiscWidget::EditorMiscWidget(const QString& name, const QString& group,
                                   QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorMiscWidget)
{
    ui->setupUi(this);
}

EditorMiscWidget::~EditorMiscWidget()
{
    delete ui;
}

void EditorMiscWidget::doLoad()
{
    ui->chkReadonlySystemHeaders->setChecked(pSettings->editor().readOnlySytemHeader());
    ui->chkLoadLastFiles->setChecked(pSettings->editor().autoLoadLastFiles());
    if (pSettings->editor().defaultFileCpp()) {
        ui->rbCppFile->setChecked(true);
    } else {
        ui->rbCFile->setChecked(true);
    }
    ui->chkAutoDetectFileEncoding->setChecked(pSettings->editor().autoDetectFileEncoding());

    QByteArray defaultEncoding = pSettings->editor().defaultEncoding();
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
    if (pSettings->editor().removeTrailingSpacesWhenSaved())
        ui->rbRemoveTrailingSpaces->setChecked(true);
    else if (pSettings->editor().autoFormatWhenSaved())
        ui->rbAutoReformat->setChecked(true);
    else
        ui->rbNone->setChecked(true);

    ui->chkParseTodos->setChecked(pSettings->editor().parseTodos());
}

void EditorMiscWidget::doSave()
{
    pSettings->editor().setReadOnlySytemHeader(ui->chkReadonlySystemHeaders->isChecked());
    pSettings->editor().setAutoLoadLastFiles(ui->chkLoadLastFiles->isChecked());
    pSettings->editor().setDefaultFileCpp(ui->rbCppFile->isChecked());
    pSettings->editor().setAutoDetectFileEncoding(ui->chkAutoDetectFileEncoding->isChecked());

    if (ui->cbEncodingDetail->isVisible()) {
        pSettings->editor().setDefaultEncoding(ui->cbEncodingDetail->currentText().toLocal8Bit());
    } else {
        pSettings->editor().setDefaultEncoding(ui->cbEncoding->currentData().toByteArray());
    }
    pSettings->editor().setAutoFormatWhenSaved(ui->rbAutoReformat->isChecked());
    pSettings->editor().setRemoveTrailingSpacesWhenSaved(ui->rbRemoveTrailingSpaces->isChecked());
    pSettings->editor().setParseTodos(ui->chkParseTodos->isChecked());


    pSettings->editor().save();
    pMainWindow->updateEditorSettings();
}

void EditorMiscWidget::init()
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

void EditorMiscWidget::on_cbEncoding_currentTextChanged(const QString &/*arg1*/)
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

