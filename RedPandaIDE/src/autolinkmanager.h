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
#ifndef AUTOLINKMANAGER_H
#define AUTOLINKMANAGER_H

#include <QObject>
#include <QString>
#include <memory>
#include <QVector>
#include <QMap>

struct Autolink {
    QString header;
    QString linkOption;
    bool execUseUTF8;
};
using PAutolink = std::shared_ptr<Autolink>;

class AutolinkManager
{
public:
    explicit AutolinkManager();
    PAutolink getLink(const QString& header) const;
    void load();
    void save();
    void setLink(const QString& header,
                 const QString& linkOption,
                 bool execUseUTF8);
    void removeLink(const QString& header);
    const QMap<QString,PAutolink>& links() const;
    void clear();
    QJsonArray toJson();
    void fromJson(QJsonArray json);
private:
    QMap<QString,PAutolink> mLinks;
};

extern AutolinkManager* pAutolinkManager;

#endif // AUTOLINKMANAGER_H
