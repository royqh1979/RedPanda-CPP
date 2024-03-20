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
#include "debuggeneralwidget.h"
#include "ui_debuggeneralwidget.h"
#include "../settings.h"
#include "../mainwindow.h"

DebugGeneralWidget::DebugGeneralWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::DebugGeneralWidget)
{
    ui->setupUi(this);
    ui->panelCharacters->setVisible(false);
}

DebugGeneralWidget::~DebugGeneralWidget()
{
    delete ui;
}

void DebugGeneralWidget::doLoad()
{
    ui->chkOnlyMono->setChecked(pSettings->debugger().onlyShowMono());
    ui->cbFont->setCurrentFont(QFont(pSettings->debugger().fontName()));
    ui->sbFontSize->setValue(pSettings->debugger().fontSize());
    ui->grpEnableDebugConsole->setChecked(pSettings->debugger().enableDebugConsole());
    ui->chkShowDetailLog->setChecked(pSettings->debugger().showDetailLog());
    if (pSettings->debugger().useIntelStyle()) {
        ui->rbIntel->setChecked(true);
    } else {
        ui->rbATT->setChecked(true);
    }
    ui->chkShowCPUWhenSignaled->setChecked(pSettings->debugger().openCPUInfoWhenSignaled());
    ui->chkBlendMode->setChecked(pSettings->debugger().blendMode());
    ui->chkSkipSystemLib->setChecked(pSettings->debugger().skipSystemLibraries());
    ui->chkSkipProjectLib->setChecked(pSettings->debugger().skipProjectLibraries());
    ui->chkSkipCustomLib->setChecked(pSettings->debugger().skipCustomLibraries());
    ui->chkAutosave->setChecked(pSettings->debugger().autosave());
#ifdef Q_OS_WIN
    ui->grpUseGDBServer->setCheckable(true);
    ui->grpUseGDBServer->setChecked(pSettings->debugger().useGDBServer());
#else
    ui->grpUseGDBServer->setCheckable(false);
#endif
    ui->spinGDBServerPort->setValue(pSettings->debugger().GDBServerPort());
    ui->spinMemoryViewRows->setValue(pSettings->debugger().memoryViewRows());
    ui->spinMemoryViewColumns->setValue(pSettings->debugger().memoryViewColumns());
    ui->spinArrayElements->setValue(pSettings->debugger().arrayElements());
    ui->spinCharacters->setValue(pSettings->debugger().characters());
}

void DebugGeneralWidget::doSave()
{
    pSettings->debugger().setOnlyShowMono(ui->chkOnlyMono->isChecked());
    pSettings->debugger().setFontName(ui->cbFont->currentFont().family());
    pSettings->debugger().setFontSize(ui->sbFontSize->value());
    pSettings->debugger().setEnableDebugConsole(ui->grpEnableDebugConsole->isChecked());
    pSettings->debugger().setShowDetailLog(ui->chkShowDetailLog->isChecked());
    pSettings->debugger().setOpenCPUInfoWhenSignaled(ui->chkShowCPUWhenSignaled->isChecked());
    pSettings->debugger().setUseIntelStyle(ui->rbIntel->isChecked());
    pSettings->debugger().setBlendMode(ui->chkBlendMode->isChecked());
    pSettings->debugger().setSkipSystemLibraries(ui->chkSkipSystemLib->isChecked());
    pSettings->debugger().setSkipProjectLibraries(ui->chkSkipProjectLib->isChecked());
    pSettings->debugger().setSkipCustomLibraries(ui->chkSkipCustomLib->isChecked());
    pSettings->debugger().setAutosave(ui->chkAutosave->isChecked());
#ifdef Q_OS_WIN
    pSettings->debugger().setUseGDBServer(ui->grpUseGDBServer->isChecked());
#endif
    pSettings->debugger().setGDBServerPort(ui->spinGDBServerPort->value());

    pSettings->debugger().setMemoryViewRows(ui->spinMemoryViewRows->value());
    pSettings->debugger().setMemoryViewColumns(ui->spinMemoryViewColumns->value());
    pSettings->debugger().setArrayElements(ui->spinArrayElements->value());
    pSettings->debugger().setCharacters(ui->spinCharacters->value());

    pSettings->debugger().save();
    pMainWindow->updateDebuggerSettings();
}

void DebugGeneralWidget::on_chkOnlyMono_stateChanged(int)
{
    if (ui->chkOnlyMono->isChecked()) {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::MonospacedFonts);
    } else {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::AllFonts);
    }
    ui->cbFont->view()->reset();
}
