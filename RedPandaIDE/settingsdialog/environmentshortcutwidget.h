#ifndef ENVIRONMENTSHORTCUTWIDGET_H
#define ENVIRONMENTSHORTCUTWIDGET_H

#include <QAbstractTableModel>
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
private:
    EnvironmentShortcutModel mModel;
};

#endif // ENVIRONMENTSHORTCUTWIDGET_H
