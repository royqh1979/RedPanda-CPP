/*
 * Copyright (C) 2020-2024 Roy Qu (royqh1979@gmail.com)
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
#ifndef DAP_PROTOCAL_H
#define DAP_PROTOCAL_H
#include <QString>
#include <QMap>
#include <QVariant>
#include <QJsonObject>
#include <qt_utils/utils.h>

class DAPMessageError : public BaseError {
public:
    explicit DAPMessageError(const QString& reason);
};

struct DAPProtocolMessage{
    qint64 seq;
    QString type;
};

struct DAPRequest: public DAPProtocolMessage {
    QString command;
    QJsonObject arguments;
};

struct DAPEvent: public DAPProtocolMessage {
    QString event;
    QJsonObject body;
};

struct DAPResponse: public DAPProtocolMessage {
    qint64 request_seq;
    bool success;
    QString command;
    QString message;
    QJsonObject body;
};

QString createDAPRequestMessage(
        qint64 seq, const QString &command, const QJsonObject& arguments);

QString createDAPResponseMessage(
        qint64 seq, qint64 request_seq, bool success,
        const QString& command, const QString& message, const QJsonObject& body);

QString createDAPEventMessage(
        qint64 seq, const QString& event, const QJsonObject& body);

std::shared_ptr<DAPProtocolMessage> parseDAPMessage(const QByteArray& contentPart);

#endif
