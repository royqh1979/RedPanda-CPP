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
#ifndef ENVIRONMENTSHORTCUTWIDGET_H
#define ENVIRONMENTSHORTCUTWIDGET_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QWidget>
#include "settingswidget.h"
#include "../shortcutmanager.h"

namespace Ui {
class EnvironmentShortcutWidget;
}

class QMenu;
class EnvironmentShortcutModel: public QAbstractTableModel {
    Q_OBJECT
    // QAbstractItemModel interface
public:
    explicit EnvironmentShortcutModel(QObject* parent=nullptr);
    void reload();

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    const QList<PEnvironmentShortcut> &shortcuts() const;
signals:
    void shortcutChanged();
public slots:
    void shortcutsUpdated();
private:
    void loadShortCutsOfMenu(const QMenu * menu, QList<QAction*>& globalActions);
private:
    QList<PEnvironmentShortcut> mShortcuts;

};

class EnvironmentShortcutDelegate: public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit EnvironmentShortcutDelegate(QObject *parent = nullptr);

public:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
protected slots:
    void onEditingFinished(QWidget* editor);
};

class EnvironmentShortcutWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentShortcutWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentShortcutWidget();

private:
    Ui::EnvironmentShortcutWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_txtKeyword_textChanged(const QString &arg1);

private:
    EnvironmentShortcutModel mModel;
    QSortFilterProxyModel* mFilterProxy;
    EnvironmentShortcutDelegate* mDelegate;
};

#endif // ENVIRONMENTSHORTCUTWIDGET_H
