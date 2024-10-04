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
#include "editorclipboardwidget.h"
#include "ui_editorclipboardwidget.h"
#include "../settings.h"
#include "../mainwindow.h"
#include "../colorscheme.h"

EditorClipboardWidget::EditorClipboardWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorClipboardWidget)
{
    ui->setupUi(this);
    ui->cbCopyWithFormatAs->addItem("None");
#ifdef Q_OS_WIN
    ui->cbCopyWithFormatAs->addItem("HTML");
#endif

    for (QString name: pColorManager->getSchemes()) {
        ui->cbHTMLColorScheme->addItem(name);
        ui->cbRTFColorScheme->addItem(name);
    }
    connect(ui->chkCopyRTFUseEditorColor,
            &QCheckBox::stateChanged,
            this,
            &EditorClipboardWidget::onUseSchemeChanged);
    connect(ui->chkCopyHTMLUseEditorColor,
            &QCheckBox::stateChanged,
            this,
            &EditorClipboardWidget::onUseSchemeChanged);
    connect(ui->chkCopyHTMLWithLineNumber,
            &QCheckBox::stateChanged,
            ui->chkCopyHTMLRecalcLineNumber,
            &QCheckBox::setEnabled);
}

EditorClipboardWidget::~EditorClipboardWidget()
{
    delete ui;
}

void EditorClipboardWidget::onUseSchemeChanged()
{
    ui->cbRTFColorScheme->setEnabled(!ui->chkCopyRTFUseEditorColor->isChecked());
    ui->cbHTMLColorScheme->setEnabled(!ui->chkCopyHTMLUseEditorColor->isChecked());
}

void EditorClipboardWidget::doLoad()
{
    //pSettings->editor().load();
    //copy
    QString mCopyHTMLColorScheme;
    ui->cbCopyWithFormatAs->setCurrentIndex(std::max(0,std::min(ui->cbCopyWithFormatAs->count(),
                                                                pSettings->editor().copyWithFormatAs())) );
    ui->chkCopyRTFUseBackground->setChecked(pSettings->editor().copyRTFUseBackground());
    ui->chkCopyRTFUseEditorColor->setChecked(pSettings->editor().copyRTFUseEditorColor());
    ui->cbRTFColorScheme->setCurrentText(pSettings->editor().copyRTFColorScheme());
    ui->chkCopyHTMLUseBackground->setChecked(pSettings->editor().copyHTMLUseBackground());
    ui->chkCopyHTMLUseEditorColor->setChecked(pSettings->editor().copyHTMLUseEditorColor());
    ui->chkCopyHTMLWithLineNumber->setChecked(pSettings->editor().copyHTMLWithLineNumber());
    ui->chkCopyHTMLRecalcLineNumber->setEnabled(pSettings->editor().copyHTMLWithLineNumber());
    ui->chkCopyHTMLRecalcLineNumber->setChecked(pSettings->editor().copyHTMLRecalcLineNumber());
    ui->cbHTMLColorScheme->setCurrentText(pSettings->editor().copyHTMLColorScheme());
    onUseSchemeChanged();
}

void EditorClipboardWidget::doSave()
{
    //copy
    pSettings->editor().setCopyWithFormatAs(ui->cbCopyWithFormatAs->currentIndex());

    pSettings->editor().setCopyRTFUseBackground(ui->chkCopyRTFUseBackground->isChecked());
    pSettings->editor().setCopyRTFUseEditorColor(ui->chkCopyRTFUseEditorColor->isChecked());
    pSettings->editor().setCopyRTFColorScheme(ui->cbRTFColorScheme->currentText());

    pSettings->editor().setCopyHTMLUseBackground(ui->chkCopyHTMLUseBackground->isChecked());
    pSettings->editor().setCopyHTMLUseEditorColor(ui->chkCopyHTMLUseEditorColor->isChecked());
    pSettings->editor().setCopyHTMLColorScheme(ui->cbHTMLColorScheme->currentText());
    pSettings->editor().setCopyHTMLWithLineNumber(ui->chkCopyHTMLWithLineNumber->isChecked());
    if (ui->chkCopyHTMLRecalcLineNumber->isEnabled())
        pSettings->editor().setCopyHTMLRecalcLineNumber(ui->chkCopyHTMLRecalcLineNumber->isChecked());

    pSettings->editor().save();
    pMainWindow->updateEditorSettings();
}
