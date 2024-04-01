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
#include "thememanager.h"
#include <QApplication>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QMetaObject>
#include <QStyle>
#include "utils.h"
#include "settings.h"
#include "systemconsts.h"

#ifdef ENABLE_LUA_ADDON
#include "addon/executor.h"
#include "addon/runtime.h"
#endif

ThemeManager::ThemeManager(QObject *parent) : QObject(parent),
    mUseCustomTheme(false)
{

}

PAppTheme ThemeManager::theme(const QString &themeName)
{
    if (mUseCustomTheme)
        prepareCustomeTheme();
    QString themeDir;
    if (mUseCustomTheme) {
        themeDir = pSettings->dirs().config(Settings::Dirs::DataType::Theme);
    } else {
        themeDir = pSettings->dirs().data(Settings::Dirs::DataType::Theme);
    }
#ifdef ENABLE_LUA_ADDON
    PAppTheme appTheme = std::make_shared<AppTheme>(QString("%1/%2.lua").arg(themeDir, themeName), AppTheme::ThemeType::Lua);
#else
    PAppTheme appTheme = std::make_shared<AppTheme>(QString("%1/%2.json").arg(themeDir, themeName), AppTheme::ThemeType::JSON);
#endif
    return appTheme;
}

bool ThemeManager::useCustomTheme() const
{
    return mUseCustomTheme;
}

void ThemeManager::setUseCustomTheme(bool newUseCustomTheme)
{
    mUseCustomTheme = newUseCustomTheme;
}

void ThemeManager::prepareCustomeTheme()
{

    if (QFile(pSettings->dirs().config(Settings::Dirs::DataType::Theme)).exists())
        return;
    copyFolder(pSettings->dirs().data(Settings::Dirs::DataType::Theme),pSettings->dirs().config(Settings::Dirs::DataType::Theme));
}

QList<PAppTheme> ThemeManager::getThemes()
{
    if (mUseCustomTheme)
        prepareCustomeTheme();

    QList<PAppTheme> result;
    QString themeDir;
    QString themeExtension;
    AppTheme::ThemeType themeType;
    if (mUseCustomTheme) {
        themeDir = pSettings->dirs().config(Settings::Dirs::DataType::Theme);
        themeExtension = "json";
        themeType = AppTheme::ThemeType::JSON;
    } else {
        themeDir = pSettings->dirs().data(Settings::Dirs::DataType::Theme);
#ifdef ENABLE_LUA_ADDON
        themeExtension = "lua";
        themeType = AppTheme::ThemeType::Lua;
#else
        themeExtension = "json";
        themeType = AppTheme::ThemeType::JSON;
#endif
    }
    QDirIterator it(themeDir);
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        if (fileInfo.suffix().compare(themeExtension, PATH_SENSITIVITY)==0) {
            try {
                PAppTheme appTheme = std::make_shared<AppTheme>(fileInfo.absoluteFilePath(), themeType);
                result.append(appTheme);
            } catch(FileError e) {
                //just skip it
            }
#ifdef ENABLE_LUA_ADDON
            catch(AddOn::LuaError e) {
                qDebug() << e.reason();
            }
#endif
        }
    }
    std::sort(result.begin(),result.end(),[](const PAppTheme &theme1, const PAppTheme &theme2){
        return QFileInfo(theme1->filename()).baseName() <  QFileInfo(theme2->filename()).baseName();
    });
    return result;
}


QColor AppTheme::color(ColorRole role) const
{
    return mColors.value(role,QColor());
}

QPalette AppTheme::palette() const
{
    QPalette pal = initialPalette();

    const static struct {
        ColorRole themeColor;
        QPalette::ColorRole paletteColorRole;
        QPalette::ColorGroup paletteColorGroup;
        bool setColorRoleAsBrush;
    } mapping[] = {
        {ColorRole::PaletteWindow,                    QPalette::Window,           QPalette::All,      false},
        {ColorRole::PaletteWindowDisabled,            QPalette::Window,           QPalette::Disabled, false},
        {ColorRole::PaletteWindowText,                QPalette::WindowText,       QPalette::All,      true},
        {ColorRole::PaletteWindowTextDisabled,        QPalette::WindowText,       QPalette::Disabled, true},
        {ColorRole::PaletteBase,                      QPalette::Base,             QPalette::All,      false},
        {ColorRole::PaletteBaseDisabled,              QPalette::Base,             QPalette::Disabled, false},
        {ColorRole::PaletteAlternateBase,             QPalette::AlternateBase,    QPalette::All,      false},
        {ColorRole::PaletteAlternateBaseDisabled,     QPalette::AlternateBase,    QPalette::Disabled, false},
        {ColorRole::PaletteToolTipBase,               QPalette::ToolTipBase,      QPalette::All,      true},
        {ColorRole::PaletteToolTipBaseDisabled,       QPalette::ToolTipBase,      QPalette::Disabled, true},
        {ColorRole::PaletteToolTipText,               QPalette::ToolTipText,      QPalette::All,      false},
        {ColorRole::PaletteToolTipTextDisabled,       QPalette::ToolTipText,      QPalette::Disabled, false},
        {ColorRole::PaletteText,                      QPalette::Text,             QPalette::All,      true},
        {ColorRole::PaletteTextDisabled,              QPalette::Text,             QPalette::Disabled, true},
        {ColorRole::PaletteButton,                    QPalette::Button,           QPalette::All,      false},
        {ColorRole::PaletteButtonDisabled,            QPalette::Button,           QPalette::Disabled, false},
        {ColorRole::PaletteButtonText,                QPalette::ButtonText,       QPalette::All,      true},
        {ColorRole::PaletteButtonTextDisabled,        QPalette::ButtonText,       QPalette::Disabled, true},
        {ColorRole::PaletteBrightText,                QPalette::BrightText,       QPalette::All,      false},
        {ColorRole::PaletteBrightTextDisabled,        QPalette::BrightText,       QPalette::Disabled, false},
        {ColorRole::PaletteHighlight,                 QPalette::Highlight,        QPalette::All,      true},
        {ColorRole::PaletteHighlightDisabled,         QPalette::Highlight,        QPalette::Disabled, true},
        {ColorRole::PaletteHighlightedText,           QPalette::HighlightedText,  QPalette::All,      true},
        {ColorRole::PaletteHighlightedTextDisabled,   QPalette::HighlightedText,  QPalette::Disabled, true},
        {ColorRole::PaletteLink,                      QPalette::Link,             QPalette::All,      false},
        {ColorRole::PaletteLinkDisabled,              QPalette::Link,             QPalette::Disabled, false},
        {ColorRole::PaletteLinkVisited,               QPalette::LinkVisited,      QPalette::All,      false},
        {ColorRole::PaletteLinkVisitedDisabled,       QPalette::LinkVisited,      QPalette::Disabled, false},
        {ColorRole::PaletteLight,                     QPalette::Light,            QPalette::All,      false},
        {ColorRole::PaletteLightDisabled,             QPalette::Light,            QPalette::Disabled, false},
        {ColorRole::PaletteMidlight,                  QPalette::Midlight,         QPalette::All,      false},
        {ColorRole::PaletteMidlightDisabled,          QPalette::Midlight,         QPalette::Disabled, false},
        {ColorRole::PaletteDark,                      QPalette::Dark,             QPalette::All,      false},
        {ColorRole::PaletteDarkDisabled,              QPalette::Dark,             QPalette::Disabled, false},
        {ColorRole::PaletteMid,                       QPalette::Mid,              QPalette::All,      false},
        {ColorRole::PaletteMidDisabled,               QPalette::Mid,              QPalette::Disabled, false},
        {ColorRole::PaletteShadow,                    QPalette::Shadow,           QPalette::All,      false},
        {ColorRole::PaletteShadowDisabled,            QPalette::Shadow,           QPalette::Disabled, false}
    };

    for (auto entry: mapping) {
        const QColor themeColor = color(entry.themeColor);
        // Use original color if color is not defined in theme.
        if (themeColor.isValid()) {
//            if (entry.setColorRoleAsBrush)
//                // TODO: Find out why sometimes setBrush is used
//                pal.setBrush(entry.paletteColorGroup, entry.paletteColorRole, themeColor);
//            else
//                pal.setColor(entry.paletteColorGroup, entry.paletteColorRole, themeColor);
            pal.setBrush(entry.paletteColorGroup, entry.paletteColorRole, themeColor);
            pal.setColor(entry.paletteColorGroup, entry.paletteColorRole, themeColor);
        }
    }

    return pal;
}

AppTheme::AppTheme(const QString &filename, ThemeType type, QObject *parent):QObject(parent)
{
    mFilename = filename;
    QFile file(filename);
    if (!file.exists()) {
        throw FileError(tr("Theme file '%1' doesn't exist!")
                        .arg(filename));
    }
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll().trimmed();
        if (content.isEmpty())
            return;
        QJsonObject obj;

        switch (type) {
        case ThemeType::JSON: {
            QJsonParseError error;
            QJsonDocument doc(QJsonDocument::fromJson(content, &error));
            if (error.error  != QJsonParseError::NoError) {
                throw FileError(tr("Error in json file '%1':%2 : %3")
                                    .arg(filename)
                                    .arg(error.offset)
                                    .arg(error.errorString()));
            }
            obj = doc.object();

            // In Lua-based theme, the "style" key has replaced "isDark" and "useQtFusionStyle" keys.
            // The following part handles old "isDark" and "useQtFusionStyle" keys.
            if (!obj.contains("style")) {
                bool useQtFusionStyle = obj["useQtFusionStyle"].toBool(true);
                if (useQtFusionStyle) {
                    bool isDark = obj["isDark"].toBool(false);
                    obj["style"] = isDark ? "RedPandaDarkFusion" : "RedPandaLightFusion";
                } else {
                    obj["style"] = AppTheme::initialStyle();
                }
            }

            // In Lua-based theme, the script handles name localization.
            // The following part handles old "name_xx_XX" keys.
            QString localeName = obj["name_"+pSettings->environment().language()].toString();
            if (!localeName.isEmpty())
                obj["name"] = localeName;

            break;
        }
#ifdef ENABLE_LUA_ADDON
        case ThemeType::Lua: {
            obj = AddOn::ThemeExecutor{}(content, filename);
            break;
        }
#endif
        }

        QFileInfo fileInfo(filename);
        mName = fileInfo.baseName();
        mDisplayName = obj["name"].toString();
        mStyle = obj["style"].toString();
        mDefaultColorScheme = obj["default scheme"].toString();
        mDefaultIconSet = obj["default iconset"].toString();
        QJsonObject colors = obj["palette"].toObject();
        const QMetaObject &m = *metaObject();
        QMetaEnum e = m.enumerator(m.indexOfEnumerator("ColorRole"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            if (colors.contains(key)) {
                QString val=colors[key].toString();
                mColors.insert(i, QColor(val));
            }
        }

    } else {
        throw FileError(tr("Can't open the theme file '%1' for read.")
                        .arg(filename));
    }
}

bool AppTheme::isSystemInDarkMode() {
    // https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5
    // compare the window color with the text color to determine whether the palette is dark or light
    return initialPalette().color(QPalette::WindowText).lightness() > initialPalette().color(QPalette::Window).lightness();
}

// If you copy QPalette, default values stay at default, even if that default is different
// within the context of different widgets. Create deep copy.
static QPalette copyPalette(const QPalette &p)
{
    QPalette res;
    for (int group = 0; group < QPalette::NColorGroups; ++group) {
        for (int role = 0; role < QPalette::NColorRoles; ++role) {
            res.setBrush(QPalette::ColorGroup(group),
                         QPalette::ColorRole(role),
                         p.brush(QPalette::ColorGroup(group), QPalette::ColorRole(role)));
            res.setColor(QPalette::ColorGroup(group),
                         QPalette::ColorRole(role),
                         p.color(QPalette::ColorGroup(group), QPalette::ColorRole(role)));
        }
    }
    return res;
}

QPalette AppTheme::initialPalette()
{
    static QPalette palette = copyPalette(QApplication::palette());
    return palette;
}

QString AppTheme::initialStyle()
{
    static QString style = QApplication::style()->objectName();
    return style;
}

const QString &AppTheme::filename() const
{
    return mFilename;
}

const QString &AppTheme::defaultIconSet() const
{
    return mDefaultIconSet;
}

void AppTheme::setDefaultIconSet(const QString &newDefaultIconSet)
{
    mDefaultIconSet = newDefaultIconSet;
}

const QString &AppTheme::name() const
{
    return mName;
}

const QString &AppTheme::displayName() const
{
    return mDisplayName;
}

const QString &AppTheme::defaultColorScheme() const
{
    return mDefaultColorScheme;
}

void AppTheme::setDefaultColorScheme(const QString &newDefaultColorScheme)
{
    mDefaultColorScheme = newDefaultColorScheme;
}

const QString &AppTheme::style() const
{
    return mStyle;
}
