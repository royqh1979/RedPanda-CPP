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
#include "autolinkmanager.h"
#include "settings.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "systemconsts.h"

AutolinkManager* pAutolinkManager;

AutolinkManager::AutolinkManager()
{
}

PAutolink AutolinkManager::getLink(const QString &header) const
{
    PAutolink link = mLinks.value(header,PAutolink());
    if (link)
        return link;
    foreach (QString key, mLinks.keys()) {
        if (header.endsWith("/"+key, PATH_SENSITIVITY)) {
            return mLinks.value(key);
        }
    }
    return PAutolink();
}

void AutolinkManager::load()
{
    QDir dir(pSettings->dirs().config());
    QString filename=dir.filePath(DEV_AUTOLINK_FILE);
    QFile file(filename);
    if (!file.exists()) {
#ifdef Q_OS_WIN
        QFile preFile(":/config/autolink.json");
        if (!preFile.open(QFile::ReadOnly)) {
            throw FileError(QObject::tr("Can't open file '%1' for read.")
                            .arg(":/config/autolink.json"));
        }
        QByteArray content=preFile.readAll();
        if (!file.open(QFile::WriteOnly|QFile::Truncate)) {
            throw FileError(QObject::tr("Can't open file '%1' for write.")
                            .arg(filename));
        }
        file.write(content);
        file.close();
        preFile.close();
#elif defined(Q_OS_MACOS)
        return;
#else // XDG desktop
        QFile preFile(":/config/autolink-xdg.json");
        if (!preFile.open(QFile::ReadOnly)) {
            throw FileError(QObject::tr("Can't open file '%1' for read.")
                            .arg(":/config/autolink-xdg.json"));
        }
        QByteArray content=preFile.readAll();
        if (!file.open(QFile::WriteOnly|QFile::Truncate)) {
            throw FileError(QObject::tr("Can't open file '%1' for write.")
                            .arg(filename));
        }
        file.write(content);
        file.close();
        preFile.close();
#endif
    }
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll().trimmed();
        if (content.isEmpty())
            return;
        QJsonDocument doc(QJsonDocument::fromJson(content));
        fromJson(doc.array());
        file.close();
    } else {
        throw FileError(QObject::tr("Can't open file '%1' for read.")
                        .arg(filename));
    }
}

void AutolinkManager::save()
{
    QDir dir(pSettings->dirs().config());
    QString filename=dir.filePath(DEV_AUTOLINK_FILE);
    QFile file(filename);
    if (file.open(QFile::WriteOnly|QFile::Truncate)) {
        QJsonDocument doc(toJson());
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    } else {
        throw FileError(QObject::tr("Can't open file '%1' for write.")
                        .arg(filename));
    }
}

void AutolinkManager::setLink(const QString &header, const QString &linkOption, bool execUseUTF8)
{
    PAutolink link = mLinks.value(header,PAutolink());
    if (link) {
        link->linkOption = linkOption;
        link->execUseUTF8 = execUseUTF8;
    } else {
        link = std::make_shared<Autolink>();
        link->header = header;
        link->linkOption = linkOption;
        link->execUseUTF8 = execUseUTF8;
        mLinks.insert(header,link);
    }
}

const QMap<QString, PAutolink> &AutolinkManager::links() const
{
    return mLinks;
}

void AutolinkManager::clear()
{
    mLinks.clear();
}

QJsonArray AutolinkManager::toJson()
{
    QJsonArray result;
    foreach (const QString& header, mLinks.keys()){
        QJsonObject autolink;
        autolink["header"]=header;
        autolink["links"]=mLinks[header]->linkOption;
        autolink["execUseUTF8"]=mLinks[header]->execUseUTF8;
        result.append(autolink);
    }
    return result;
}

void AutolinkManager::fromJson(QJsonArray json)
{
    clear();
    for (int i=0;i<json.size();i++) {
        QJsonObject obj = json[i].toObject();
        setLink(obj["header"].toString(),obj["links"].toString(),obj["execUseUTF8"].toBool());
    }
}
