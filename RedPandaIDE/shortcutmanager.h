#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QMap>
#include <QObject>
#include <memory>

class QAction;

struct EnvironmentShortcut {
    QString name;
    QString fullPath;
    QString shortcut;
    QAction* action;
};

using PEnvironmentShortcut = std::shared_ptr<EnvironmentShortcut>;

class ShortcutManager : public QObject
{
    Q_OBJECT
public:
    explicit ShortcutManager(QObject *parent = nullptr);
    void load();
    void save();
    void setShortcuts(QList<PEnvironmentShortcut> shortcuts);
    void applyTo(QList<QAction*> actions);
    void applyTo(QAction* action);
signals:
private:

private:
    QMap<QString,PEnvironmentShortcut> mShortcuts;
};

using PShortcutManager = std::shared_ptr<ShortcutManager>;

#endif // SHORTCUTMANAGER_H
