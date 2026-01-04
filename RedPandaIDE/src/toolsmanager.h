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
#ifndef TOOLSMANAGER_H
#define TOOLSMANAGER_H

#include <QObject>
#include <memory>

enum class ToolItemInputOrigin {
    None,
    CurrentSelection,
    WholeDocument
};

enum class ToolItemOutputTarget {
    RedirectToNull,
    RedirectToToolsOutputPanel,
    ReplaceCurrentSelection,
    RepalceWholeDocument,
};

struct ToolItem {
    QString id;
    QString title;
    QString program;
    QString workingDirectory;
    QString parameters;
    ToolItemInputOrigin inputOrigin;
    ToolItemOutputTarget outputTarget;
    bool isUTF8;
};

using PToolItem = std::shared_ptr<ToolItem>;

class ToolsManager : public QObject
{
    Q_OBJECT
public:
    explicit ToolsManager(QObject *parent = nullptr);
    void load();
    void save();
    const QList<PToolItem> &tools() const;
    PToolItem findTool(const QString& title);
    void setTools(const QList<PToolItem> &newTools);
private:
    QList<PToolItem> mTools;
};

using PToolsManager = std::shared_ptr<ToolsManager>;

#endif // TOOLSMANAGER_H
