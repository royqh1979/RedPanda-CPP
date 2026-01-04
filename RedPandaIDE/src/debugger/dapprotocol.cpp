#include "dapprotocol.h"
#include <QJsonDocument>
#include <QJsonObject>


DAPMessageError::DAPMessageError(const QString &reason) : BaseError{reason}
{

}

static QJsonObject createDAPMessageObj(qint64 seq, const QString& type) {
    QJsonObject obj;
    obj["seq"]=seq;
    obj["type"]=type;
    return obj;
}

static QString jsonToDAPMessageString(const QJsonObject &jsonObj)
{
    QJsonDocument doc;
    doc.setObject(jsonObj);
    QString contentPart = doc.toJson(QJsonDocument::JsonFormat::Indented);
    QString message = QString("Content-Length: %1\r\n").arg(contentPart.length())+contentPart;
    return message;
}

static QJsonObject contentPartToDAPMessageObj(const QByteArray &contentPart) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(contentPart, &error);
    if (error.error != QJsonParseError::NoError) {
        throw DAPMessageError(QObject::tr("Failed to parse json content: %1").arg(QString::fromUtf8(contentPart)));
    }
    QJsonObject obj = doc.object();
    return obj;
}

static bool parseJsonObjBoolProperty(const QJsonObject& obj, const QString propName) {
    if (!obj.contains(propName)) {
        throw DAPMessageError(QObject::tr("The request message don't have '%1' field!").arg(propName));
    }
    return obj[propName].toBool();
}

static int parseJsonObjIntProperty(const QJsonObject& obj, const QString propName) {
    if (!obj.contains(propName)) {
        throw DAPMessageError(QObject::tr("The request message don't have '%1' field!").arg(propName));
    }
    return obj[propName].toInt();
}

static double parseJsonObjNumberProperty(const QJsonObject& obj, const QString propName) {
    if (!obj.contains(propName)) {
        throw DAPMessageError(QObject::tr("The request message don't have '%1' field!").arg(propName));
    }
    return obj[propName].toDouble();
}

static QString parseJsonObjStringProperty(const QJsonObject& obj, const QString propName) {
    if (!obj.contains(propName)) {
        throw DAPMessageError(QObject::tr("The request message don't have '%1' field!").arg(propName));
    }
    return obj[propName].toString();
}

static QJsonObject parseJsonObjObjectProperty(const QJsonObject& obj, const QString propName) {
    if (!obj.contains(propName)) {
        throw DAPMessageError(QObject::tr("The request message don't have '%1' field!").arg(propName));
    }
    return obj[propName].toObject();
}


QString createDAPRequestMessage(qint64 seq, const QString &command, const QJsonObject &arguments)
{
    QJsonObject obj = createDAPMessageObj(seq, "request");
    obj["command"]=command;
    if (!arguments.isEmpty()) {
        obj["arguments"] = arguments;
    }
    return jsonToDAPMessageString(obj);
}

QString createDAPResponseMessage(qint64 seq, qint64 request_seq, bool success, const QString &command, const QString &message, const QJsonObject &body)
{
    QJsonObject obj = createDAPMessageObj(seq, "response");
    obj["request_seq"]=request_seq;
    obj["success"]=success;
    obj["command"]=command;
    if (!message.isEmpty())
        obj["message"]=message;
    if (!body.isEmpty())
        obj["body"]=body;
    return jsonToDAPMessageString(obj);
}

QString createDAPEventMessage(qint64 seq, const QString &event, const QJsonObject &body)
{
    QJsonObject obj = createDAPMessageObj(seq, "event");
    obj["event"] = event;
    if (!body.isEmpty())
        obj["body"] = body;
    return jsonToDAPMessageString(obj);
}

std::shared_ptr<DAPProtocolMessage> parseDAPMessage(const QByteArray &contentPart)
{
    QJsonObject obj = contentPartToDAPMessageObj(contentPart);
    DAPRequest req;
    qint64 seq = parseJsonObjNumberProperty(obj, "seq");
    QString type = parseJsonObjStringProperty(obj, "type");
    if (type == "request") {
        std::shared_ptr<DAPRequest> req = std::make_shared<DAPRequest>();
        req->seq = seq;
        req->type = "request";
        req->command = parseJsonObjStringProperty(obj, "command");
        req->arguments = obj["arguments"].toObject();
        return req;
    } else if (type == "event") {
        std::shared_ptr<DAPEvent> event = std::make_shared<DAPEvent>();
        event->seq = seq;
        event->type = "event";
        event->event = parseJsonObjStringProperty(obj, "event");
        event->body = obj["body"].toObject();
        return event;
    } else if (type == "response") {
        std::shared_ptr<DAPResponse> response = std::make_shared<DAPResponse>();
        response->seq = seq;
        response->type = "response";
        response->request_seq = parseJsonObjNumberProperty(obj, "request_seq");
        response->success = parseJsonObjBoolProperty(obj, "success");
        response->command = parseJsonObjStringProperty(obj, "command");
        response->message = obj["message"].toString();
        response->body = obj["body"].toObject();
        return response;
    }
    return std::shared_ptr<DAPProtocolMessage>();
}
