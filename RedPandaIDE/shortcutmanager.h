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
#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QMap>
#include <QObject>
#include <memory>

class QAction;
class QToolButton;

struct EnvironmentShortcut {
    QString name;
    QString fullPath;
    QString shortcut;
    QAction* action;
    QToolButton* button;
    bool isAction;
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

private:
    QMap<QString,PEnvironmentShortcut> mShortcuts;
};

using PShortcutManager = std::shared_ptr<ShortcutManager>;

#endif // SHORTCUTMANAGER_H
