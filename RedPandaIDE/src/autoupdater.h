/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef AUTOUPDATER_H
#define AUTOUPDATER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVersionNumber>
#include <functional>

using UpdateCallback = std::function<void(const QString &version, const QString &downloadUrl)>;

class AutoUpdater : public QObject
{
public:
    explicit AutoUpdater(QObject *parent = nullptr);

    void checkForUpdates(bool silent, UpdateCallback callback);

private:
    void doReply(QNetworkReply *reply, bool silent, UpdateCallback callback);
    static bool isNewerThan(const QString &latest, const QString &current);

    QNetworkAccessManager *mNetworkManager;
};

extern void setAutoUpdaterCurrentVersion(const QString &version);

#endif // AUTOUPDATER_H
