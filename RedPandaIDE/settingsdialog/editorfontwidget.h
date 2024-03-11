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
#ifndef EDITORFONTWIDGET_H
#define EDITORFONTWIDGET_H

#include <QWidget>
#include <QAbstractListModel>
#include "settingswidget.h"
#include "utils/font.h"

namespace Ui {
class EditorFontWidget;
}

class EditorFontModel : public QAbstractListModel
{
    Q_OBJECT
public:
    void addFont(const QString& font);
    void remove(int index);
    void clear();
    void moveUp(int index);
    void moveDown(int index);
    QModelIndex lastFont();
    const QStringList &fonts() const;
    void updateFonts(const QStringList& fonts);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

private:
    QStringList mFonts;
};

class EditorFontWidget : public SettingsWidget
{
    Q_OBJECT
public:
    explicit EditorFontWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorFontWidget();


private slots:
    void on_chkGutterOnlyMonospacedFonts_stateChanged(int arg1);
    void on_btnAddFont_clicked();
    void on_btnRemoveFont_clicked();
    void on_btnMoveFontUp_clicked();
    void on_btnMoveFontDown_clicked();
    void on_btnResetFonts_clicked();

    // void on_chkLigature_toggled(bool checked);

    // void on_chkForceFixedFontWidth_toggled(bool checked);

private:
    Ui::EditorFontWidget *ui;
    EditorFontModel mModel;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
    void updateIcons(const QSize &size) override;
};

#endif // EDITORFONTWIDGET_H
