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

void EditorFontModel::addFont(const QString& font)
{
    beginInsertRows(QModelIndex(),mFonts.size(),mFonts.size());
    mFonts.append(font);
    endInsertRows();
}

void EditorFontModel::remove(int index)
{
    beginRemoveRows(QModelIndex(),index,index);
    mFonts.removeAt(index);
    endRemoveRows();
}

void EditorFontModel::clear()
{
    beginResetModel();
    mFonts.clear();
    endResetModel();
}

void EditorFontModel::moveUp(int index)
{
    if (index == 0)
        return;
    beginMoveRows(QModelIndex(),index,index,QModelIndex(),index - 1);
    mFonts.move(index,index - 1);
    endMoveRows();
}

void EditorFontModel::moveDown(int index)
{
    if (index == mFonts.size() - 1)
        return;
    beginMoveRows(QModelIndex(),index,index,QModelIndex(),index + 2);
    mFonts.move(index,index + 1);
    endMoveRows();
}

QModelIndex EditorFontModel::lastFont()
{
    return index(mFonts.size() - 1,0);
}

const QStringList &EditorFontModel::fonts() const
{
    return mFonts;
}

void EditorFontModel::updateFonts(const QStringList& fonts)
{
    beginResetModel();
    mFonts = fonts;
    endResetModel();
}

int EditorFontModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return mFonts.size();
}

int EditorFontModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

QVariant EditorFontModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole)
        return mFonts.at(index.row());
    return QVariant();
}

bool EditorFontModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role == Qt::EditRole) {
        mFonts[index.row()] = value.toString();
        emit dataChanged(index,index);
        return true;
    }
    return false;
}

EditorFontWidget::EditorFontWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorFontWidget)
{
    ui->setupUi(this);

    QItemSelectionModel *m = ui->lstFontList->selectionModel();
    ui->lstFontList->setModel(&mModel);
    delete m;
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
}

void EditorFontWidget::on_btnAddFont_clicked()
{
    mModel.addFont(ui->cbNewFont->currentFont().family());
}

void EditorFontWidget::on_btnRemoveFont_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    if (!index.isValid())
        return;
    mModel.remove(index.row());
}

void EditorFontWidget::on_btnMoveFontUp_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    if (!index.isValid())
        return;
    mModel.moveUp(index.row());
}

void EditorFontWidget::on_btnMoveFontDown_clicked()
{
    QModelIndex index = ui->lstFontList->currentIndex();
    if (!index.isValid())
        return;
    mModel.moveDown(index.row());
}

void EditorFontWidget::on_btnResetFonts_clicked()
{
    mModel.updateFonts(defaultEditorFonts());
}

void EditorFontWidget::doLoad()
{
    //pSettings->editor().load();
    //font
    ui->cbNewFont->setCurrentFont(QFont(defaultMonoFont()));
    mModel.updateFonts(pSettings->editor().fontFamilies());

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
    pSettings->editor().setFontFamilies(mModel.fonts());
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
    pIconsManager->setIcon(ui->btnMoveFontUp, IconsManager::ACTION_MISC_MOVEUP);
    pIconsManager->setIcon(ui->btnMoveFontDown, IconsManager::ACTION_MISC_MOVEDOWN);
    pIconsManager->setIcon(ui->btnResetFonts, IconsManager::ACTION_MISC_RESET);
}
