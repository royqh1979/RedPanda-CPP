#include "dapprotocol.h"
#include <QJsonDocument>
#include <QJsonObject>

QString createDAPRequestMessage(qint64 seq, const QString &command, const QVariantMap &arguments)
{
    QJsonObject obj;
    obj["seq"]=seq;
    obj["type"]="request";
    obj["command"]=command;
    if (!arguments.isEmpty()) {
        obj["arguments"]= QJsonObject::fromVariantMap(arguments);
    }
    QJsonDocument doc;
    doc.setObject(obj);
    QString contentPart = doc.toJson(QJsonDocument::JsonFormat::Indented);
    QString message = QString("Content-Length: %1\r\n").arg(contentPart.length())+contentPart;
    return message;
}

DAPRequest parseDAPRequestMessage(const QByteArray &contentPart)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(contentPart, &error);
    if (error.error != QJsonParseError::NoError) {
        throw DAPMessageError(QObject::tr("Failed to parse json content: %1").arg(QString::fromUtf8(contentPart)));
    }
    QJsonObject obj = doc.object();
    DAPRequest req;
    if (!obj.contains("seq")) {
        throw DAPMessageError(QObject::tr("The request message don't have '%1' field!").arg("seq"));
    }
    req.seq = obj["seq"].toDouble();
    req.type = "request";
    if (!obj.contains("command")) {
        throw DAPMessageError(QObject::tr("The request message don't have '%1' field!").arg("command"));
    }
    req.command = obj["command"].toString();
    if (obj.contains("arguments"))
    req.arguments = obj["arguments"].toObject().toVariantMap();
    return req;
}

DAPMessageError::DAPMessageError(const QString &reason) : BaseError{reason}
{

}
