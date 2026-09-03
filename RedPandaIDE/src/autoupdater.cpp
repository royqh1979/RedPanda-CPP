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
#include "autoupdater.h"
#include <QJsonDocument>
#include <QJsonObject>

static const QString GITHUB_API_URL =
    QStringLiteral("https://api.github.com/repos/royqh1979/RedPanda-CPP/releases/latest");
static const QString GITEE_API_URL =
    QStringLiteral("https://gitee.com/api/v5/repos/royqh1979/RedPanda-CPP/releases/latest");

static QString sCurrentVersion;

void setAutoUpdaterCurrentVersion(const QString &version)
{
    sCurrentVersion = version;
}

AutoUpdater::AutoUpdater(QObject *parent)
    : QObject(parent)
    , mNetworkManager(new QNetworkAccessManager(this))
{
}

void AutoUpdater::checkForUpdates(bool silent, UpdateCallback callback)
{
    if (!callback) return;

    QNetworkRequest request(GITHUB_API_URL);
    request.setTransferTimeout(8000);

    QNetworkReply *reply = mNetworkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, silent, callback]() {
        doReply(reply, silent, callback);
    });
}

void AutoUpdater::doReply(QNetworkReply *reply, bool silent, UpdateCallback callback)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QNetworkRequest request(GITEE_API_URL);
        request.setTransferTimeout(8000);
        QNetworkReply *r2 = mNetworkManager->get(request);
        connect(r2, &QNetworkReply::finished, this, [this, r2, silent, callback]() {
            r2->deleteLater();
            QString tag, url;
            if (r2->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(r2->readAll());
                tag = doc.object()["tag_name"].toString();
                url = doc.object()["html_url"].toString();
            }
            if (isNewerThan(tag, sCurrentVersion))
                callback(tag, url);
            else if (!silent)
                callback(QString(), QString());
        });
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QString tag = doc.object()["tag_name"].toString();
    QString url = doc.object()["html_url"].toString();

    if (isNewerThan(tag, sCurrentVersion))
        callback(tag, url);
    else if (!silent)
        callback(QString(), QString());
}

bool AutoUpdater::isNewerThan(const QString &latest, const QString &current)
{
    if (latest.isEmpty() || current.isEmpty()) return false;
    QString lv = latest.startsWith('v', Qt::CaseInsensitive) ? latest.mid(1) : latest;
    QVersionNumber lvn = QVersionNumber::fromString(lv);
    QVersionNumber cvn = QVersionNumber::fromString(current);
    if (!lvn.isNull() && !cvn.isNull()) return lvn > cvn;
    return lv != current;
}
