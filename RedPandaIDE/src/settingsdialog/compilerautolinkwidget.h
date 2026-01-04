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
#ifndef COMPILERAUTOLINKWIDGET_H
#define COMPILERAUTOLINKWIDGET_H

#include <QAbstractTableModel>
#include <QWidget>
#include "settingswidget.h"
#include "../autolinkmanager.h"

namespace Ui {
class CompilerAutolinkWidget;
}

class CompilerAutolinkWidget;
class AutolinkModel: public QAbstractTableModel {
    Q_OBJECT
public:
    explicit AutolinkModel(CompilerAutolinkWidget* widget,QObject* parent=nullptr);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool insertRows(int row, int count, const QModelIndex &parent) override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;

    const QList<PAutolink> &links() const;
    void setLinks(const QMap<QString, PAutolink> &newLinks);
private:
    int findLink(const QString& header);
private:
    QList<PAutolink> mLinks;
    CompilerAutolinkWidget * mWidget;

};

class CompilerAutolinkWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit CompilerAutolinkWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~CompilerAutolinkWidget();

private:
    AutolinkModel mModel;
private:
    Ui::CompilerAutolinkWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnAdd_pressed();
    void on_btnRemove_pressed();

    // SettingsWidget interface
protected:
    void updateIcons(const QSize &size) override;
};

#endif // COMPILERAUTOLINKWIDGET_H
