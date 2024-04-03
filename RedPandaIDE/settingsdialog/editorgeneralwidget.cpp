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
#include "editorgeneralwidget.h"
#include "ui_editorgeneralwidget.h"
#include "../settings.h"
#include "../mainwindow.h"

#include <QStandardItemModel>

EditorGeneralWidget::EditorGeneralWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::editorgeneralwidget)
{
    ui->setupUi(this);
    QStringList caretTypes;
    caretTypes.append(tr("Vertical Line"));
    caretTypes.append(tr("Horizontal Line"));
    caretTypes.append(tr("Half Block"));
    caretTypes.append(tr("Block"));
    ui->cbCaretForInsert->addItems(caretTypes);
    ui->cbCaretForOverwrite->addItems(caretTypes);
}

EditorGeneralWidget::~EditorGeneralWidget()
{
    delete ui;
}

static void setCaretTypeIndex(QComboBox* combo, QSynedit::EditCaretType caretType) {
    int t = static_cast<int>(caretType);
    combo->setCurrentIndex(t);
}

static QSynedit::EditCaretType getCaretTypeIndex(QComboBox* combo) {
    if (combo->currentIndex()<0)
        return QSynedit::EditCaretType::VerticalLine;
    return static_cast<QSynedit::EditCaretType>(combo->currentIndex());
}
void EditorGeneralWidget::doLoad()
{
    pSettings->editor().load();
    //indents
    ui->chkAutoIndent->setChecked(pSettings->editor().autoIndent());
    ui->chkTabToSpaces->setChecked(pSettings->editor().tabToSpaces());
    ui->spTabWidth->setValue(pSettings->editor().tabWidth());
    ui->chkShowIndentLines->setChecked(pSettings->editor().showIndentLines());
    ui->colorIndentLine->setColor(pSettings->editor().indentLineColor());
    ui->chkFillIndents->setChecked(pSettings->editor().fillIndents());
    //carets
    ui->chkEnhanceHome->setChecked(pSettings->editor().enhanceHomeKey());
    ui->chkEnhanceEndKey->setChecked(pSettings->editor().enhanceEndKey());
    ui->chkKeepCaretX->setChecked(pSettings->editor().keepCaretX());
    setCaretTypeIndex(ui->cbCaretForInsert,pSettings->editor().caretForInsert());
    setCaretTypeIndex(ui->cbCaretForOverwrite,pSettings->editor().caretForOverwrite());
    ui->chkCaretUseTextColor->setChecked(pSettings->editor().caretUseTextColor());
    ui->colorCaret->setColor(pSettings->editor().caretColor());
    //highlight
    ui->chkHighlightCurrentWord->setChecked(pSettings->editor().highlightCurrentWord());
    ui->chkHighlightMatchingBraces->setChecked(pSettings->editor().highlightMathingBraces());
    //scrolls;
    ui->chkAutoHideScrollBars->setChecked(pSettings->editor().autoHideScrollbar());
    ui->chkScrollPastEOF->setChecked(pSettings->editor().scrollPastEof());
    ui->chkScrollPastEOL->setChecked(pSettings->editor().scrollPastEol());
    ui->chkScrollHalfPage->setChecked(pSettings->editor().halfPageScroll());
    ui->spinMouseWheelScrollSpeed->setValue(pSettings->editor().mouseWheelScrollSpeed());
    ui->spinMouseSelectionScrollSpeed->setValue(pSettings->editor().mouseSelectionScrollSpeed());

    //right margin line;
    ui->grpRightEdge->setChecked(pSettings->editor().showRightEdgeLine());
    ui->spRightEdge->setValue(pSettings->editor().rightEdgeWidth());
    ui->colorRightEdgeLine->setColor(pSettings->editor().rightEdgeLineColor());
}

void EditorGeneralWidget::doSave()
{
    //indents
    pSettings->editor().setAutoIndent(ui->chkAutoIndent->isChecked());
    pSettings->editor().setTabToSpaces(ui->chkTabToSpaces->isChecked());
    pSettings->editor().setTabWidth(ui->spTabWidth->value());
    pSettings->editor().setShowIndentLines(ui->chkShowIndentLines->isChecked());
    pSettings->editor().setIndentLineColor(ui->colorIndentLine->color());
    pSettings->editor().setFillIndents(ui->chkFillIndents->isChecked());

    //carets
    pSettings->editor().setEnhanceHomeKey(ui->chkEnhanceHome->isChecked());
    pSettings->editor().setEnhanceEndKey(ui->chkEnhanceEndKey->isChecked());
    pSettings->editor().setKeepCaretX(ui->chkKeepCaretX->isChecked());
    pSettings->editor().setCaretForInsert(getCaretTypeIndex(ui->cbCaretForInsert));
    pSettings->editor().setCaretForOverwrite(getCaretTypeIndex(ui->cbCaretForOverwrite));
    pSettings->editor().setCaretUseTextColor(ui->chkCaretUseTextColor->isChecked());
    pSettings->editor().setCaretColor(ui->colorCaret->color());
    //highlight
    pSettings->editor().setHighlightCurrentWord(ui->chkHighlightCurrentWord->isChecked());
    pSettings->editor().setHighlightMathingBraces(ui->chkHighlightMatchingBraces->isChecked());

    //scrolls;
    pSettings->editor().setAutoHideScrollbar(ui->chkAutoHideScrollBars->isChecked());
    pSettings->editor().setScrollPastEof(ui->chkScrollPastEOF->isChecked());
    pSettings->editor().setScrollPastEol(ui->chkScrollPastEOL->isChecked());
    pSettings->editor().setHalfPageScroll(ui->chkScrollHalfPage->isChecked());
    pSettings->editor().setMouseWheelScrollSpeed(ui->spinMouseWheelScrollSpeed->value());
    pSettings->editor().setMouseSelectionScrollSpeed(ui->spinMouseSelectionScrollSpeed->value());
    //right margin line;
    pSettings->editor().setShowRightEdgeLine(ui->grpRightEdge->isChecked());
    pSettings->editor().setRightEdgeWidth(ui->spRightEdge->value());
    pSettings->editor().setRightEdgeLineColor(ui->colorRightEdgeLine->color());
    pSettings->editor().save();
    pMainWindow->updateEditorSettings();
}

void EditorGeneralWidget::on_chkCaretUseTextColor_stateChanged(int )
{
    ui->lbCaretColor->setVisible(!ui->chkCaretUseTextColor->isChecked());
    ui->colorCaret->setVisible(!ui->chkCaretUseTextColor->isChecked());
}


void EditorGeneralWidget::on_chkShowIndentLines_stateChanged(int)
{
    ui->lbIndentLineColor->setVisible(ui->chkShowIndentLines->isChecked());
    ui->colorIndentLine->setVisible(ui->chkShowIndentLines->isChecked());
}

