#include "colorschema.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "utils.h"

ColorSchema::ColorSchema()
{

}

QString ColorSchema::name() const
{
    return mName;
}

void ColorSchema::setName(const QString &name)
{
    mName = name;
}

void ColorSchema::read(const QJsonObject &json)
{
    if (json.contains("name") && json["name"].isString()) {
        setName(json["name"].toString());
    } else {
        setName("");
    }
    mItems.clear();
    if (json.contains("items") && json["items"].isObject()) {
        QJsonObject itemsList = json["items"].toObject();
        for (QString key:itemsList.keys()) {
            PColorSchemaItem item = std::make_shared<ColorSchemaItem>(key);
            item->read(itemsList[key].toObject());
        }
    }
}

void ColorSchema::write(QJsonObject &json)
{
    json["name"] = mName;
    QJsonObject itemsList;
    for (PColorSchemaItem item:mItems) {
        if (!item->name().isEmpty()) {
            QJsonObject itemObject;
            item->write(itemObject);
            itemsList[item->name()] = itemObject;
        }
    }
    json["items"]=itemsList;
}

void ColorSchema::load(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        throw new FileError(QObject::tr("Can't open file '%1' for read").arg(file.fileName()));
    }
    QByteArray content = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(content,&error);
    if (error.error!=QJsonParseError::NoError) {
        throw new FileError(QObject::tr("Can't parse json file '%1' at offset %2! Error Code: %3")
                            .arg(file.fileName()).arg(error.offset).arg(error.error));
    }
    if (!doc.isObject()) {
        throw new FileError(QObject::tr("Can't parse json file '%1' is not a color schema config file!")
                            .arg(file.fileName()));
    }
    read(doc.object());
}

void ColorSchema::save(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        throw new FileError(QObject::tr("Can't open file '%1' for write").arg(file.fileName()));
    }
    QJsonObject json;
    write(json);
    QJsonDocument doc(json);
    QByteArray content = doc.toJson();
    file.write(content);
}

ColorSchemaItem::ColorSchemaItem(const QString& name):
    mName(name),
    mForeground(),
    mBackground(),
    mBold(false),
    mItalic(false),
    mUnderlined(false),
    mStrikeout(false)
{

}

QString ColorSchemaItem::name() const
{
    return mName;
}

QColor ColorSchemaItem::foreground() const
{
    return mForeground;
}

void ColorSchemaItem::setForeground(const QColor &foreground)
{
    mForeground = foreground;
}

QColor ColorSchemaItem::background() const
{
    return mBackground;
}

void ColorSchemaItem::setBackground(const QColor &background)
{
    mBackground = background;
}

bool ColorSchemaItem::bold() const
{
    return mBold;
}

void ColorSchemaItem::setBold(bool bold)
{
    mBold = bold;
}

bool ColorSchemaItem::italic() const
{
    return mItalic;
}

void ColorSchemaItem::setItalic(bool italic)
{
    mItalic = italic;
}

bool ColorSchemaItem::underlined() const
{
    return mUnderlined;
}

void ColorSchemaItem::setUnderlined(bool underlined)
{
    mUnderlined = underlined;
}

bool ColorSchemaItem::strikeout() const
{
    return mStrikeout;
}

void ColorSchemaItem::setStrikeout(bool strikeout)
{
    mStrikeout = strikeout;
}

void ColorSchemaItem::read(const QJsonObject &json)
{
    if (json.contains("foreground") && json["foreground"].isString()) {
        setForeground(json["foreground"].toString());
    } else {
        setForeground(QColor());
    }
    if (json.contains("background") && json["background"].isString()) {
        setBackground(json["background"].toString());
    } else {
        setBackground(QColor());
    }
    if (json.contains("bold") && json["bold"].isBool()) {
        setBold(json["bold"].toBool());
    } else {
        setBold(false);
    }
    if (json.contains("italic") && json["italic"].isBool()) {
        setBold(json["italic"].toBool());
    } else {
        setItalic(false);
    }
    if (json.contains("underlined") && json["underlined"].isBool()) {
        setBold(json["underlined"].toBool());
    } else {
        setUnderlined(false);
    }
    if (json.contains("strikeout") && json["strikeout"].isBool()) {
        setBold(json["strikeout"].toBool());
    } else {
        setUnderlined(false);
    }

}

void ColorSchemaItem::write(QJsonObject &json)
{
    if (mForeground.isValid()) {
        json["foreground"] = mForeground.name();
    } else if (json.contains("foreground")){
        json.remove("foreground");
    }
    if (mBackground.isValid()) {
        json["background"] = mBackground.name();
    } else if (json.contains("background")){
        json.remove("background");
    }
    json["bold"] = mBold;
    json["italic"] = mItalic;
    json["underlined"] = mUnderlined;
    json["strikeout"] = mStrikeout;
}
