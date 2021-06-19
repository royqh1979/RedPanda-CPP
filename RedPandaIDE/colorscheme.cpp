#include "colorscheme.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "utils.h"
#include "settings.h"
#include "qsynedit/Constants.h"


ColorManager * pColorManager;
ColorScheme::ColorScheme()
{

}

PColorScheme ColorScheme::fromJson(const QJsonObject &json)
{
    PColorScheme scheme = std::make_shared<ColorScheme>();
    scheme->mItems.clear();
    for (QString key:json.keys()) {
        if (json[key].isObject()) {
            scheme->mItems[key]=ColorSchemeItem::fromJson(json[key].toObject());
        }
    }
    return scheme;
}

void ColorScheme::toJson(QJsonObject &json)
{
    for (QString key:mItems.keys()) {
        PColorSchemeItem item = mItems[key];
        if (item) {
            QJsonObject itemObject;
            item->toJson(itemObject);
            json[key] = itemObject;
        }
    }
}

PColorScheme ColorScheme::load(const QString &filename)
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
    return ColorScheme::fromJson(doc.object());
}

void ColorScheme::save(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        throw new FileError(QObject::tr("Can't open file '%1' for write").arg(file.fileName()));
    }
    QJsonObject json;
    toJson(json);
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

bool ColorScheme::customed() const
{
    return mCustomed;
}

void ColorScheme::setCustomed(bool customed)
{
    mCustomed = customed;
}

QString ColorScheme::preferThemeType() const
{
    return mPreferThemeType;
}

void ColorScheme::setPreferThemeType(const QString &preferThemeType)
{
    mPreferThemeType = preferThemeType;
}

ColorSchemeItem::ColorSchemeItem():
    mForeground(),
    mBackground(),
    mBold(false),
    mItalic(false),
    mUnderlined(false),
    mStrikeout(false)
{

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

PColorSchemeItem ColorSchemeItem::fromJson(const QJsonObject &json)
{
    PColorSchemeItem item = std::make_shared<ColorSchemeItem>();
    if (json.contains("foreground") && json["foreground"].isString()) {
        item->setForeground(json["foreground"].toString());
    } else {
        item->setForeground(QColor());
    }
    if (json.contains("background") && json["background"].isString()) {
        item->setBackground(json["background"].toString());
    } else {
        item->setBackground(QColor());
    }
    if (json.contains("bold") && json["bold"].isBool()) {
        item->setBold(json["bold"].toBool());
    } else {
        item->setBold(false);
    }
    if (json.contains("italic") && json["italic"].isBool()) {
        item->setBold(json["italic"].toBool());
    } else {
        item->setItalic(false);
    }
    if (json.contains("underlined") && json["underlined"].isBool()) {
        item->setBold(json["underlined"].toBool());
    } else {
        item->setUnderlined(false);
    }
    if (json.contains("strikeout") && json["strikeout"].isBool()) {
        item->setBold(json["strikeout"].toBool());
    } else {
        item->setUnderlined(false);
    }
    return item;
}

void ColorSchemeItem::toJson(QJsonObject &json)
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

ColorManager::ColorManager()
{
    mDefaultSchemeItemDefine = std::make_shared<ColorSchemeItemDefine>();
    initItemDefines();
    init();
}

void ColorManager::init()
{
    reload();
}

void ColorManager::reload()
{
    mSchemes.clear();
    //bundled schemes ( the lowest priority)
    loadSchemesInDir(pSettings->dirs().data(Settings::Dirs::DataType::ColorSheme),false);
    //config schemes ( higher priority)
    loadSchemesInDir(pSettings->dirs().config(Settings::Dirs::DataType::ColorSheme),false);
    //customed schemes ( highest priority)
    loadSchemesInDir(pSettings->dirs().config(Settings::Dirs::DataType::ColorSheme),true);
}

QStringList ColorManager::getSchemes(const QString &themeType)
{
    if (themeType.isEmpty()) {
        return mSchemes.keys();
    }
    QStringList lst;
    for (QString name:mSchemes.keys()) {
        PColorScheme scheme = mSchemes[name];
        if (scheme && scheme->preferThemeType() == themeType) {
            lst.append(name);
        }
    }
    return lst;
}

bool ColorManager::exists(const QString name)
{
    return mSchemes.contains(name);
}

PColorScheme ColorManager::copy(const QString &sourceName)
{
    if (!mSchemes.contains(sourceName))
        return PColorScheme();
    PColorScheme sourceScheme = mSchemes[sourceName];
    QString newName = sourceName+" Copy";
    if (mSchemes.contains(newName))
        return PColorScheme();
    // save source with the new name
    QString newFilepath = generateFullPathname(newName,false,false);
    sourceScheme->save(newFilepath);
    // then load it to the copied
    PColorScheme newScheme = ColorScheme::load(newFilepath);
    newScheme->setBundled(false);
    newScheme->setCustomed(false);
    mSchemes[newName]=newScheme;
    return newScheme;
}

QString ColorManager::generateFilename(const QString &name, bool isCustomed)
{
    QString newName = name;
    newName.replace(' ','_');
    if (isCustomed)
        newName += EXT_PREFIX_CUSTOM;
    return newName += EXT_COLOR_SCHEME;
}

void ColorManager::loadSchemesInDir(const QString &dirName, bool isCustomed)
{
    QDir dir(dirName);
    dir.setFilter(QDir::Files);
    QFileInfoList  list = dir.entryInfoList();
    QString suffix;
    if (isCustomed) {
        suffix = EXT_PREFIX_CUSTOM;
        suffix = suffix + EXT_COLOR_SCHEME;
    } else {
        suffix = EXT_COLOR_SCHEME;
    }
    for (int i=0;i<list.size();i++) {
        QFileInfo fileInfo = list[i];
        QString name = fileInfo.fileName().toLower();
        if (name.endsWith(suffix)) {
            PColorScheme scheme = ColorScheme::load(fileInfo.absoluteFilePath());
            name.remove(name.length()-suffix.length(),suffix.length());
            name.replace('_',' ');
            mSchemes[name]=scheme;
        }
    }
}

void ColorManager::initItemDefines()
{
    //Highlighter colors
    addDefine(SYNS_AttrAssembler,true,true,true);
    addDefine(SYNS_AttrCharacter,true,true,true);
    addDefine(SYNS_AttrComment,true,true,true);
    addDefine(SYNS_AttrClass,true,true,true);
    addDefine(SYNS_AttrFloat,true,true,true);
    addDefine(SYNS_AttrFunction,true,true,true);
    addDefine(SYNS_AttrGlobalVariable,true,true,true);
    addDefine(SYNS_AttrHexadecimal,true,true,true);
    addDefine(SYNS_AttrIdentifier,true,true,true);
    addDefine(SYNS_AttrIllegalChar,true,true,true);
    addDefine(SYNS_AttrLocalVariable,true,true,true);
    addDefine(SYNS_AttrNumber,true,true,true);
    addDefine(SYNS_AttrOctal,true,true,true);
    addDefine(SYNS_AttrPreprocessor,true,true,true);
    addDefine(SYNS_AttrReservedWord,true,true,true);
    addDefine(SYNS_AttrSpace,true,true,true);
    addDefine(SYNS_AttrString,true,true,true);
    addDefine(SYNS_AttrStringEscapeSequences,true,true,true);
    addDefine(SYNS_AttrSymbol,true,true,true);
    addDefine(SYNS_AttrVariable,true,true,true);

    //Gutter colors
    addDefine(COLOR_SCHEME_GUTTER,true,true,true);
    //Active Line
    addDefine(COLOR_SCHEME_ACTIVE_LINE,false,true,false);
    //Breakpoint Line
    addDefine(COLOR_SCHEME_BREAKPOINT,true,true,false);
    //Current Debug Line
    addDefine(COLOR_SCHEME_ACTIVE_BREAKPOINT,true,true,false);
    //Fold line
    addDefine(COLOR_SCHEME_FOLD_LINE,true,false,false);
    //Brace/Bracket/Parenthesis Level 1 2 3 4
    addDefine(COLOR_SCHEME_BRACE_1,true,false,false);
    addDefine(COLOR_SCHEME_BRACE_2,true,false,false);
    addDefine(COLOR_SCHEME_BRACE_3,true,false,false);
    addDefine(COLOR_SCHEME_BRACE_4,true,false,false);
    addDefine(COLOR_SCHEME_SELECTION,true,true,false);
    //Syntax Error
    addDefine(COLOR_SCHEME_ERROR,true,false,false);
    addDefine(COLOR_SCHEME_WARNING,true,false,false);
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
    mSchemeItemDefine[name]=define;
}

bool ColorManager::removeDefine(const QString &name)
{
    return mSchemeItemDefine.remove(name)==1;
}

PColorSchemeItemDefine ColorManager::getDefine(const QString &name)
{
    if (mSchemeItemDefine.contains(name))
        return mSchemeItemDefine[name];
    return PColorSchemeItemDefine();
}

QString ColorManager::generateFullPathname(const QString &name, bool isBundled, bool isCustomed)
{
    QString filename = generateFilename(name,isCustomed);
    if (isBundled) {
        return includeTrailingPathDelimiter(pSettings->dirs().data(Settings::Dirs::DataType::ColorSheme))+filename;
    } else {
        return includeTrailingPathDelimiter(pSettings->dirs().config(Settings::Dirs::DataType::ColorSheme))+filename;
    }
}

ColorSchemeItemDefine::ColorSchemeItemDefine()
{
    mHasBackground = true;
    mHasForeground = true;
    mHasFontStyle = true;
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

bool ColorSchemeItemDefine::hasFontStyle() const
{
    return mHasFontStyle;
}

void ColorSchemeItemDefine::setHasFontStyle(bool value)
{
    mHasFontStyle = value;
}
