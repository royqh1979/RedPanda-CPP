#ifndef ENVIRONMENTSHORTCUTWIDGET_H
#define ENVIRONMENTSHORTCUTWIDGET_H

#include <QAbstractTableModel>
#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EnvironmentShortcutWidget;
}

struct EnvironmentShortCut {
    QString name;
    QString fullPath;
    QString shortcut;
    QAction* action;
};

using PEnvironmentShortCut = std::shared_ptr<EnvironmentShortCut>;
class QMenu;
class EnvironmentShortcutModel: public QAbstractTableModel {
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
    const QList<PEnvironmentShortCut> &shortcuts() const;
private:
    void loadShortCutsOfMenu(const QMenu * menu, QList<QAction*>& globalActions);
private:
    QList<PEnvironmentShortCut> mShortcuts;
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
