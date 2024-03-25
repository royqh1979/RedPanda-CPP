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
#include "editor.h"
#include "ui_editorfontwidget.h"
#include "../settings.h"
#include "../mainwindow.h"
#include "../iconsmanager.h"
#include "utils.h"
#include "utils/font.h"
#include "widgets/editorfontdialog.h"
#include <qabstractitemmodel.h>

Qt::ItemFlags EditorFontModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::NoItemFlags;
    if (index.isValid()) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable ;
    } else if (index.row() == -1) {
        // -1 means it's a drop target?
        flags = Qt::ItemIsDropEnabled;
    }
    return flags;
}

EditorFontWidget::EditorFontWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorFontWidget),
    mModel(pSettings->editor().fontFamilies())
{
    ui->setupUi(this);

    QItemSelectionModel *m = ui->lstFontList->selectionModel();
    ui->lstFontList->setModel(&mModel);
    delete m;
    ui->lstFontList->setDragEnabled(true);
    ui->lstFontList->setAcceptDrops(true);
    ui->lstFontList->setDropIndicatorShown(true);
    ui->lstFontList->setDragDropMode(QAbstractItemView::InternalMove);
}

EditorFontWidget::~EditorFontWidget()
{
    delete ui;
}

void EditorFontWidget::on_chkGutterOnlyMonospacedFonts_stateChanged(int)
{
    if (ui->chkGutterOnlyMonospacedFonts->isChecked()) {
        ui->cbGutterFont->setFontFilters(QFontComboBox::FontFilter::MonospacedFonts);
    } else {
        ui->cbGutterFont->setFontFilters(QFontComboBox::FontFilter::AllFonts);
    }
    ui->cbGutterFont->view()->reset();
}

void EditorFontWidget::on_btnAddFont_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    int insertPos = index.isValid() ? index.row() + 1 : mModel.rowCount();
    EditorFontDialog dlg(insertPos==0, this);
    if (dlg.exec() == QDialog::Accepted) {
        mModel.insertRow(insertPos);
        mModel.setData(mModel.index(insertPos), dlg.fontFamily());
        ui->lstFontList->setCurrentIndex(mModel.index(insertPos));
    }
}

void EditorFontWidget::on_btnRemoveFont_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    if (!index.isValid())
        return;
    mModel.removeRow(index.row());
}

void EditorFontWidget::on_btnModifyFont_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    modifyFont(index);
}

void EditorFontWidget::on_btnResetFonts_clicked()
{
    mModel.setStringList(defaultEditorFonts());
}

void EditorFontWidget::on_btnMoveFontToTop_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    if (!index.isValid())
        return;
    if (index.row() == 0)
        return;
    mModel.moveRow(QModelIndex(), index.row(), QModelIndex(), 0);
}

void EditorFontWidget::on_btnMoveFontUp_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    if (!index.isValid())
        return;
    if (index.row() == 0)
        return;
    mModel.moveRow(QModelIndex(), index.row(), QModelIndex(), index.row() - 1);
}

void EditorFontWidget::on_btnMoveFontDown_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    if (!index.isValid())
        return;
    if (index.row() == mModel.rowCount() - 1)
        return;
    mModel.moveRow(QModelIndex(), index.row(), QModelIndex(), index.row() + 2);
}

void EditorFontWidget::on_btnMoveFontToBottom_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    if (!index.isValid())
        return;
    if (index.row() == mModel.rowCount() - 1)
        return;
    mModel.moveRow(QModelIndex(), index.row(), QModelIndex(), mModel.rowCount());
}

void EditorFontWidget::doLoad()
{
    //pSettings->editor().load();
    //font
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
    pSettings->editor().setFontFamilies(mModel.stringList());
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

void EditorFontWidget::updateIcons(const QSize &/*size*/) {
    pIconsManager->setIcon(ui->btnAddFont, IconsManager::ACTION_MISC_ADD);
    pIconsManager->setIcon(ui->btnRemoveFont, IconsManager::ACTION_MISC_REMOVE);
    pIconsManager->setIcon(ui->btnModifyFont, IconsManager::ACTION_MISC_RENAME);
    pIconsManager->setIcon(ui->btnResetFonts, IconsManager::ACTION_MISC_RESET);
    pIconsManager->setIcon(ui->btnMoveFontToTop, IconsManager::ACTION_MISC_MOVETOP);
    pIconsManager->setIcon(ui->btnMoveFontUp, IconsManager::ACTION_MISC_MOVEUP);
    pIconsManager->setIcon(ui->btnMoveFontDown, IconsManager::ACTION_MISC_MOVEDOWN);
    pIconsManager->setIcon(ui->btnMoveFontToBottom, IconsManager::ACTION_MISC_MOVEBOTTOM);
}

void EditorFontWidget::on_lstFontList_doubleClicked(const QModelIndex &index)
{
    modifyFont(index);
}

void EditorFontWidget::modifyFont(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    EditorFontDialog dlg(index.row()==0, this);
    dlg.setFontFamily(mModel.data(index, Qt::DisplayRole).toString());
    if (dlg.exec() == QDialog::Accepted) {
        mModel.setData(index, dlg.fontFamily());
    }
}

