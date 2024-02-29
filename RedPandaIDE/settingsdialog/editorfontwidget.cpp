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
    ui->cbFallbackFont2->setEnabled(false);
    ui->cbFallbackFont3->setEnabled(false);
    connect(ui->chkFallbackFont2, &QCheckBox::stateChanged,
            this, &EditorFontWidget::onFallbackFontsCheckStateChanged);
    connect(ui->chkFallbackFont3, &QCheckBox::stateChanged,
            this, &EditorFontWidget::onFallbackFontsCheckStateChanged);
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
    ui->cbFallbackFont->setCurrentFont(QFont(pSettings->editor().fallbackFontName()));
    ui->cbFallbackFont2->setCurrentFont(QFont(pSettings->editor().fallbackFontName2()));
    ui->cbFallbackFont3->setCurrentFont(QFont(pSettings->editor().fallbackFontName3()));
    ui->chkFallbackFont2->setChecked(pSettings->editor().useFallbackFont2());
    ui->chkFallbackFont3->setChecked(pSettings->editor().useFallbackFont3());

    ui->spinFontSize->setValue(pSettings->editor().fontSize());
    ui->spinLineSpacing->setValue(pSettings->editor().lineSpacing());
    ui->chkLigature->setChecked(pSettings->editor().enableLigaturesSupport());
    ui->chkForceFixedFontWidth->setChecked(pSettings->editor().forceFixedFontWidth());
    ui->chkLeadingSpaces->setChecked(pSettings->editor().showLeadingSpaces());
    ui->chkInnerSpaces->setChecked(pSettings->editor().showInnerSpaces());
    ui->chkTrailingSpaces->setChecked(pSettings->editor().showTrailingSpaces());
    ui->chkLineBreaks->setChecked(pSettings->editor().showLineBreaks());
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
    pSettings->editor().setFallbackFontName(ui->cbFallbackFont->currentFont().family());
    pSettings->editor().setFallbackFontName2(ui->cbFallbackFont2->currentFont().family());
    pSettings->editor().setFallbackFontName3(ui->cbFallbackFont3->currentFont().family());
    pSettings->editor().setUseFallbackFont2(ui->chkFallbackFont2->isChecked());
    pSettings->editor().setUseFallbackFont3(ui->chkFallbackFont3->isChecked());
    pSettings->editor().setFontSize(ui->spinFontSize->value());
    pSettings->editor().setLineSpacing(ui->spinLineSpacing->value());

    pSettings->editor().setEnableLigaturesSupport(ui->chkLigature->isChecked());
    pSettings->editor().setForceFixedFontWidth(ui->chkForceFixedFontWidth->isChecked());
    pSettings->editor().setShowLeadingSpaces(ui->chkLeadingSpaces->isChecked());
    pSettings->editor().setShowInnerSpaces(ui->chkInnerSpaces->isChecked());
    pSettings->editor().setShowTrailingSpaces(ui->chkTrailingSpaces->isChecked());
    pSettings->editor().setShowLineBreaks(ui->chkLineBreaks->isChecked());
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
    QFont::cleanup();
    pMainWindow->updateEditorSettings();
}

void EditorFontWidget::onFallbackFontsCheckStateChanged()
{
    ui->cbFallbackFont2->setEnabled(ui->chkFallbackFont2->isChecked());
    ui->cbFallbackFont3->setEnabled(ui->chkFallbackFont3->isChecked());
}


// void EditorFontWidget::on_chkLigature_toggled(bool checked)
// {
//     if (ui->chkLigature->isChecked())
//         ui->chkForceFixedFontWidth->setChecked(false);
// }


// void EditorFontWidget::on_chkForceFixedFontWidth_toggled(bool checked)
// {
//     if (ui->chkForceFixedFontWidth->isChecked())
//         ui->chkLigature->setChecked(false);
// }

