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
#include "environmentshortcutwidget.h"
#include "ui_environmentshortcutwidget.h"
#include "../mainwindow.h"
#include "../widgets/shortcutinputedit.h"
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>

EnvironmentShortcutWidget::EnvironmentShortcutWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentShortcutWidget)
{
    ui->setupUi(this);
    mFilterProxy = new QSortFilterProxyModel(this);
    mFilterProxy->setSourceModel(&mModel);
    mFilterProxy->setFilterKeyColumn(0);
    mDelegate =new EnvironmentShortcutDelegate(this);
    QItemSelectionModel* m=ui->tblShortcut->selectionModel();
    ui->tblShortcut->setModel(mFilterProxy);
    delete m;
    ui->tblShortcut->setItemDelegate(mDelegate);
    connect(&mModel, &EnvironmentShortcutModel::shortcutChanged,
            this, &SettingsWidget::setSettingsChanged);
    mDelegate =new EnvironmentShortcutDelegate(this);
}

EnvironmentShortcutWidget::~EnvironmentShortcutWidget()
{
    delete ui;
}

void EnvironmentShortcutWidget::doLoad()
{
    mModel.reload();
}

void EnvironmentShortcutWidget::doSave()
{
    ShortcutManager manager;
    manager.setShortcuts(mModel.shortcuts());
    manager.save();
    pMainWindow->updateShortcuts();
    mModel.reload();
}

EnvironmentShortcutModel::EnvironmentShortcutModel(QObject *parent):QAbstractTableModel(parent)
{

}

void EnvironmentShortcutModel::reload()
{
    beginResetModel();
    mShortcuts.clear();
    QList<QMenu*> menus = pMainWindow->menuBar()->findChildren<QMenu*>();
    QList<QAction*> actions = pMainWindow->listShortCutableActions();
    foreach( const QMenu* menu, menus) {
        if (menu->title().isEmpty())
            continue;
        loadShortCutsOfMenu(menu, actions);
    }
    foreach (QAction* action,actions) {
        if (!action->text().isEmpty()) {
            PEnvironmentShortcut item = std::make_shared<EnvironmentShortcut>();
            item->name = action->objectName();
            item->fullPath = QString("%1 : %2").arg(tr("action"),action->text());
            item->action = action;
            item->shortcut = action->shortcut().toString().trimmed();
            item->isAction = true;
            mShortcuts.append(item);
        }
    }
    endResetModel();
}

int EnvironmentShortcutModel::rowCount(const QModelIndex &) const
{
    return mShortcuts.count();
}

int EnvironmentShortcutModel::columnCount(const QModelIndex &) const
{
    return 2;
}

QVariant EnvironmentShortcutModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (role==Qt::DisplayRole || role == Qt::EditRole || role == Qt::ToolTipRole) {
        PEnvironmentShortcut item = mShortcuts[index.row()];
        switch( index.column()) {
        case 0:
            return item->fullPath;
        case 1:
            return item->shortcut;
        }
    }
    return QVariant();
}

bool EnvironmentShortcutModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }
    if (role == Qt::EditRole) {
        if (index.column()!=1)
            return false;
        PEnvironmentShortcut item = mShortcuts[index.row()];
        QString s = value.toString().trimmed();
        if (s!=item->shortcut) {
            if (s.isEmpty()) {
                item->shortcut="";
            } else {
                for (int i=0;i<mShortcuts.length();i++) {
                    if (i==index.row())
                        continue;
                    if (s==mShortcuts[i]->shortcut)  {
                        QMessageBox::critical(nullptr,
                                              tr("Error"),
                                              tr("Shortcut \"%1\" is used by \"%2\".")
                                              .arg(s,mShortcuts[i]->fullPath));
                        return false;
                    }
                }
                item->shortcut = value.toString();
            }
            emit shortcutChanged();
        }
        return true;
    }
    return false;
}

QVariant EnvironmentShortcutModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch(section) {
            case 0:
                return tr("Function");
            case 1:
                return tr("Shortcut");
            }
        }
    }
    return QVariant();
}

Qt::ItemFlags EnvironmentShortcutModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled;
    if (index.isValid() && index.column()==1) {
        flags.setFlag(Qt::ItemIsEditable);
    }
    return flags;
}

const QList<PEnvironmentShortcut> &EnvironmentShortcutModel::shortcuts() const
{
    return mShortcuts;
}

void EnvironmentShortcutModel::shortcutsUpdated()
{
    beginResetModel();
    endResetModel();
}

void EnvironmentShortcutModel::loadShortCutsOfMenu(const QMenu *menu, QList<QAction *> &globalActions)
{
    QList<QAction*> actions = menu->actions();
    foreach (QAction* action,actions) {
        if (!action->text().isEmpty() && action->menu()==nullptr) {
            PEnvironmentShortcut item = std::make_shared<EnvironmentShortcut>();
            item->name = action->objectName();
            item->fullPath = QString("%1 > %2").arg(menu->title(),action->text());
            item->action = action;
            item->shortcut = action->shortcut().toString().trimmed();
            item->isAction = true;
            mShortcuts.append(item);
        }
        globalActions.removeAll(action);
    }
}

EnvironmentShortcutDelegate::EnvironmentShortcutDelegate(
        QObject *parent) : QStyledItemDelegate(parent)
{
}

QWidget *EnvironmentShortcutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.isValid() && index.column()==1) {
        ShortcutInputEdit *editor=new ShortcutInputEdit(dynamic_cast<QWidget*>(parent));
        connect(editor,&ShortcutInputEdit::inputFinished,
                this, &EnvironmentShortcutDelegate::onEditingFinished);
        return editor;
    }
    return QStyledItemDelegate::createEditor(parent,option,index);
}

void EnvironmentShortcutDelegate::onEditingFinished(QWidget* editor)
{
    emit commitData(editor);
    emit closeEditor(editor, QAbstractItemDelegate::SubmitModelCache);
}

void EnvironmentShortcutWidget::on_txtKeyword_textChanged(const QString &arg1)
{
    mFilterProxy->setFilterFixedString(arg1);
}

