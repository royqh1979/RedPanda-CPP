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
#ifndef TOOLSGENERALWIDGET_H
#define TOOLSGENERALWIDGET_H

#include <QAbstractListModel>
#include <QWidget>
#include "settingswidget.h"
#include "../widgets/macroinfomodel.h"
#include "../toolsmanager.h"

namespace Ui {
class ToolsGeneralWidget;
}

class ToolsModel: public QAbstractListModel {
public:
    explicit ToolsModel(QObject* parent = nullptr);

    const QList<PToolItem> &tools() const;
    void setTools(const QList<PToolItem> &newTools);
    void addTool(PToolItem item);
    PToolItem getTool(int row);
    void updateTool(int row, PToolItem item);
    void removeTool(int row);

private:
    QList<PToolItem> mTools;
    int mMoveTargetRow;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    // QAbstractItemModel interface
public:
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    bool insertRows(int row, int count, const QModelIndex &parent) override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;

};


class ToolsGeneralWidget : public SettingsWidget
{
    Q_OBJECT
public:
    explicit ToolsGeneralWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ToolsGeneralWidget();
private:
    void finishEditing(bool askSave);
    void cleanEditor();
    void prepareEdit(int row);
    void showEditPanel(bool isShow);
private slots:
    void onEdited();
    void editTool(const QModelIndex &index);
    void updateDemo();
    void on_btnAdd_clicked();

    void on_btnEditOk_clicked();

    void on_btnEditCancel_clicked();

    void on_btnRemove_clicked();

    void on_btnInsertMacro_clicked();

    void on_btnBrowseWorkingDirectory_clicked();

    void on_btnBrowseProgram_clicked();
    void on_btnEdit_clicked();

private:
    Ui::ToolsGeneralWidget *ui;
    MacroInfoModel mMacroInfoModel;
    ToolsModel mToolsModel;
    bool mEdited;
    int mCurrentEditingRow;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;

    // SettingsWidget interface
protected:
    void updateIcons(const QSize &size) override;
};

#endif // TOOLSGENERALWIDGET_H
