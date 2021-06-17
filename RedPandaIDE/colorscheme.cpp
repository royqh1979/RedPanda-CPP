#include "colorscheme.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "utils.h"

ColorScheme::ColorScheme()
{

}

QString ColorScheme::name() const
{
    return mName;
}

void ColorScheme::setName(const QString &name)
{
    mName = name;
}

void ColorScheme::read(const QJsonObject &json)
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
            PColorSchemeItem item = std::make_shared<ColorSchemeItem>(key);
            item->read(itemsList[key].toObject());
        }
    }
}

void ColorScheme::write(QJsonObject &json)
{
    json["name"] = mName;
    QJsonObject itemsList;
    for (PColorSchemeItem item:mItems) {
        if (!item->name().isEmpty()) {
            QJsonObject itemObject;
            item->write(itemObject);
            itemsList[item->name()] = itemObject;
        }
    }
    json["items"]=itemsList;
}

void ColorScheme::load(const QString &filename)
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

void ColorScheme::save(const QString &filename)
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

bool ColorScheme::bundled() const
{
    return mBundled;
}

void ColorScheme::setBundled(bool bundled)
{
    mBundled = bundled;
}

bool ColorScheme::modified() const
{
    return mModified;
}

void ColorScheme::setModified(bool modified)
{
    mModified = modified;
}

QString ColorScheme::preferThemeType() const
{
    return mPreferThemeType;
}

void ColorScheme::setPreferThemeType(const QString &preferThemeType)
{
    mPreferThemeType = preferThemeType;
}

ColorSchemeItem::ColorSchemeItem(const QString& name):
    mName(name),
    mForeground(),
    mBackground(),
    mBold(false),
    mItalic(false),
    mUnderlined(false),
    mStrikeout(false)
{

}

QString ColorSchemeItem::name() const
{
    return mName;
}

QColor ColorSchemeItem::foreground() const
{
    return mForeground;
}

void ColorSchemeItem::setForeground(const QColor &foreground)
{
    mForeground = foreground;
}

QColor ColorSchemeItem::background() const
{
    return mBackground;
}

void ColorSchemeItem::setBackground(const QColor &background)
{
    mBackground = background;
}

bool ColorSchemeItem::bold() const
{
    return mBold;
}

void ColorSchemeItem::setBold(bool bold)
{
    mBold = bold;
}

bool ColorSchemeItem::italic() const
{
    return mItalic;
}

void ColorSchemeItem::setItalic(bool italic)
{
    mItalic = italic;
}

bool ColorSchemeItem::underlined() const
{
    return mUnderlined;
}

void ColorSchemeItem::setUnderlined(bool underlined)
{
    mUnderlined = underlined;
}

bool ColorSchemeItem::strikeout() const
{
    return mStrikeout;
}

void ColorSchemeItem::setStrikeout(bool strikeout)
{
    mStrikeout = strikeout;
}

void ColorSchemeItem::read(const QJsonObject &json)
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

void ColorSchemeItem::write(QJsonObject &json)
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

void ColorManager::init()
{
    reload();
}

PColorScheme ColorManager::copy(const QString &sourceName)
{
    if (!mSchemes.contains(sourceName))
        return PColorScheme();
    PColorScheme sourceScheme = mSchemes[sourceName];
    // todo:save source with the new name
    // then load it to the copied

}

bool ColorManager::rename(const QString &oldName, const QString &newName)
{
    if (mSchemes.contains(newName))
        return false;
    if (!isValidName(newName))
        return false;
    PColorScheme scheme = get(oldName);
    if (!scheme)
        return false;
    mSchemes[newName] = scheme;

}

PColorScheme ColorManager::remove(const QString &name)
{
    PColorScheme scheme=get(name);
    if (scheme)
        mSchemes.remove(name);
    return scheme;
}

PColorScheme ColorManager::get(const QString &name)
{
    if (mSchemes.contains(name))
        return mSchemes[name];
    return PColorScheme();
}

bool ColorManager::isValidName(const QString &name)
{
    for (QChar ch:name) {
        if (!((ch == ' ') or  (ch>='a' && ch<='z') or (ch>='A' && ch <= 'Z')
            or (ch>='0' && ch<='9') or (ch == '-') ))
            return false;
    }
    return true;
}

void ColorManager::addDefine(const QString &name, bool hasForeground, bool hasBackground, bool hasFontStyle)
{
    PColorSchemeItemDefine define = std::make_shared<ColorSchemeItemDefine>();
    define->setHasForeground(hasForeground);
    define->setHasBackground(hasBackground);
    define->setHasFontStyle(hasFontStyle);
    mSchemeDefine[name]=define;
}

bool ColorManager::removeDefine(const QString &name)
{
    return mSchemeDefine.remove(name)==1;
}

PColorSchemeItemDefine ColorManager::getDefine(const QString &name)
{
    if (mSchemeDefine.contains(name))
        return mSchemeDefine[name];
    return PColorSchemeItemDefine();
}

bool ColorSchemeItemDefine::hasBackground() const
{
    return mHasBackground;
}

void ColorSchemeItemDefine::setHasBackground(bool hasBackground)
{
    mHasBackground = hasBackground;
}

bool ColorSchemeItemDefine::hasForeground() const
{
    return mHasForeground;
}

void ColorSchemeItemDefine::setHasForeground(bool hasForeground)
{
    mHasForeground = hasForeground;
}

bool ColorSchemeItemDefine::getHasFontStyle() const
{
    return hasFontStyle;
}

void ColorSchemeItemDefine::setHasFontStyle(bool value)
{
    hasFontStyle = value;
}
