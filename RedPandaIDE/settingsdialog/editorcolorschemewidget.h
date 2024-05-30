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
#ifndef EDITORCOLORSCHEMEWIDGET_H
#define EDITORCOLORSCHEMEWIDGET_H

#include "settingswidget.h"
#include "../colorscheme.h"

#include <QMenu>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

namespace Ui {
class EditorColorSchemeWidget;
}

class ColorSchemeItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    ColorSchemeItemDelegate(QObject *parent=nullptr);


    // QStyledItemDelegate interface
protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class EditorColorSchemeWidget : public SettingsWidget
{
    Q_OBJECT

public:
    enum {
        NameRole = Qt::UserRole+1
    };
    explicit EditorColorSchemeWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorColorSchemeWidget();

public slots:
    void onItemSelectionChanged();
    void onSettingChanged();
    void onForegroundChanged();
    void onBackgroundChanged();
    void onFontStyleChanged();
    void changeSchemeComboFont();

private:
    void addDefine(const QString& name, PColorSchemeItemDefine define);
    PColorSchemeItem getCurrentItem();
    PColorScheme getCurrentScheme();
    void connectModificationSlots();
    void disconnectModificationSlots();
    void setCurrentSchemeModified();

private:
    Ui::EditorColorSchemeWidget *ui;
    QStandardItemModel mDefinesModel;
    QFont mDefaultSchemeComboFont;
    QFont mModifiedSchemeComboFont;
    QSet<QString> mModifiedSchemes;
    QMenu mMenu;
    QStyledItemDelegate *mItemDelegate;
    std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > mStatementColors;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;

private slots:
    void on_actionCopy_Scheme_triggered();
    void on_btnSchemeMenu_pressed();
    void on_actionImport_Scheme_triggered();
    void on_actionRename_Scheme_triggered();
    void on_actionReset_Scheme_triggered();
    void on_actionExport_Scheme_triggered();
    void on_actionDelete_Scheme_triggered();
};

#endif // EDITORCOLORSCHEMEWIDGET_H
