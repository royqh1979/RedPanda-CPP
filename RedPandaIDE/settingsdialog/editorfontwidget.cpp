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
#include "editorfontwidget.h"
#include "ui_editorfontwidget.h"
#include "../settings.h"
#include "../mainwindow.h"

EditorFontWidget::EditorFontWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorFontWidget)
{
    ui->setupUi(this);
}

EditorFontWidget::~EditorFontWidget()
{
    delete ui;
}


void EditorFontWidget::on_chkOnlyMonospacedFonts_stateChanged(int)
{
    if (ui->chkOnlyMonospacedFonts->isChecked()) {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::MonospacedFonts);
    } else {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::AllFonts);
    }
}

void EditorFontWidget::on_chkGutterOnlyMonospacedFonts_stateChanged(int)
{
    if (ui->chkGutterOnlyMonospacedFonts->isChecked()) {
        ui->cbGutterFont->setFontFilters(QFontComboBox::FontFilter::MonospacedFonts);
    } else {
        ui->cbGutterFont->setFontFilters(QFontComboBox::FontFilter::AllFonts);
    }
}

void EditorFontWidget::doLoad()
{
    //pSettings->editor().load();
    //font
    ui->chkOnlyMonospacedFonts->setChecked(pSettings->editor().fontOnlyMonospaced());
    ui->cbFont->setCurrentFont(QFont(pSettings->editor().fontName()));
    ui->cbNonAsciiFont->setCurrentFont(QFont(pSettings->editor().nonAsciiFontName()));
    ui->spinFontSize->setValue(pSettings->editor().fontSize());
    ui->chkLigature->setChecked(pSettings->editor().enableLigaturesSupport());

    //gutter
    ui->chkGutterVisible->setChecked(pSettings->editor().gutterVisible());
    ui->chkAutoSizeGutter->setChecked(pSettings->editor().gutterAutoSize());
    ui->spinGutterLeftOffset->setValue(pSettings->editor().gutterLeftOffset());
    ui->spinGutterRightOffset->setValue(pSettings->editor().gutterRightOffset());
    ui->spinGutterDigitsCount->setValue(pSettings->editor().gutterDigitsCount());
    ui->grpGutterShowLineNumbers->setChecked(pSettings->editor().gutterShowLineNumbers());
    ui->chkAddLeadingZeros->setChecked(pSettings->editor().gutterAddLeadingZero());
    ui->chkLineNumbersStartsZero->setChecked(pSettings->editor().gutterLineNumbersStartZero());
    ui->grpUseCustomFont->setChecked(pSettings->editor().gutterUseCustomFont());
    ui->chkGutterOnlyMonospacedFonts->setChecked(pSettings->editor().gutterFontOnlyMonospaced());
    ui->cbGutterFont->setCurrentFont(QFont(pSettings->editor().gutterFontName()));
    ui->spinGutterFontSize->setValue(pSettings->editor().gutterFontSize());
}

void EditorFontWidget::doSave()
{
    //font
    pSettings->editor().setFontOnlyMonospaced(ui->chkOnlyMonospacedFonts->isChecked());
    pSettings->editor().setFontName(ui->cbFont->currentFont().family());
    pSettings->editor().setNonAsciiFontName(ui->cbNonAsciiFont->currentFont().family());
    pSettings->editor().setFontSize(ui->spinFontSize->value());
    pSettings->editor().setEnableLigaturesSupport(ui->chkLigature->isChecked());

    //gutter
    pSettings->editor().setGutterVisible(ui->chkGutterVisible->isChecked());
    pSettings->editor().setGutterAutoSize(ui->chkAutoSizeGutter->isChecked());
    pSettings->editor().setGutterLeftOffset(ui->spinGutterLeftOffset->value());
    pSettings->editor().setGutterRightOffset(ui->spinGutterRightOffset->value());
    pSettings->editor().setGutterDigitsCount(ui->spinGutterDigitsCount->value());
    pSettings->editor().setGutterShowLineNumbers(ui->grpGutterShowLineNumbers->isChecked());
    pSettings->editor().setGutterAddLeadingZero(ui->chkAddLeadingZeros->isChecked());
    pSettings->editor().setGutterLineNumbersStartZero(ui->chkLineNumbersStartsZero->isChecked());
    pSettings->editor().setGutterUseCustomFont(ui->grpUseCustomFont->isChecked());
    pSettings->editor().setGutterFontOnlyMonospaced(ui->chkGutterOnlyMonospacedFonts->isChecked());
    pSettings->editor().setGutterFontName(ui->cbGutterFont->currentFont().family());
    pSettings->editor().setGutterFontSize(ui->spinGutterFontSize->value());

    pSettings->editor().save();
    pMainWindow->updateEditorSettings();
}
