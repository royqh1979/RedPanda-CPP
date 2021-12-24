#include "environmentshortcutwidget.h"
#include "ui_environmentshortcutwidget.h"
#include "../mainwindow.h"
#include <QMenuBar>

EnvironmentShortcutWidget::EnvironmentShortcutWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentShortcutWidget)
{
    ui->setupUi(this);
    ui->tblShortcut->setModel(&mModel);
    connect(&mModel, &EnvironmentShortcutModel::shortcutChanged,
            this, &SettingsWidget::setSettingsChanged);
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
    QList<QAction*> actions = pMainWindow->findChildren<QAction*>(QString(),Qt::FindDirectChildrenOnly);
    QList<QMenu*> menus = pMainWindow->menuBar()->findChildren<QMenu*>();
    foreach( const QMenu* menu, menus) {
        if (menu->title().isEmpty())
            continue;
        loadShortCutsOfMenu(menu, actions);
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
    if (role==Qt::DisplayRole || role == Qt::EditRole) {
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
            item->shortcut = value.toString();
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
        if (!action->text().isEmpty()) {
            PEnvironmentShortcut item = std::make_shared<EnvironmentShortcut>();
            item->name = action->objectName();
            item->fullPath = QString("%1 > %2").arg(menu->title(),action->text());
            item->action = action;
            item->shortcut = action->shortcut().toString().trimmed();
            mShortcuts.append(item);
        }
        globalActions.removeAll(action);
    }
}
