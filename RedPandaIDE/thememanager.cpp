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
#include "utils.h"
#include "settings.h"
#include "systemconsts.h"

ThemeManager::ThemeManager(QObject *parent) : QObject(parent),
    mUseCustomTheme(false)
{

}

PAppTheme ThemeManager::theme(const QString &themeName)
{
    if (mUseCustomTheme)
        prepareCustomeTheme();
    PAppTheme appTheme = std::make_shared<AppTheme>();
    QString themeDir;
    if (mUseCustomTheme)
        themeDir = pSettings->dirs().config(Settings::Dirs::DataType::Theme);
    else
        themeDir = pSettings->dirs().data(Settings::Dirs::DataType::Theme);
    appTheme->load(QString("%1/%2.json").arg(themeDir, themeName));
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
    if (mUseCustomTheme)
        themeDir = pSettings->dirs().config(Settings::Dirs::DataType::Theme);
    else
        themeDir = pSettings->dirs().data(Settings::Dirs::DataType::Theme);
    QDirIterator it(themeDir);
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        if (fileInfo.suffix().compare("json", PATH_SENSITIVITY)==0) {
            try {
                PAppTheme appTheme = std::make_shared<AppTheme>();
                appTheme->load(fileInfo.absoluteFilePath());
                result.append(appTheme);
            } catch(FileError e) {
                //just skip it
            }
        }
    }
    return result;
}

AppTheme::AppTheme(QObject *parent):QObject(parent)
{

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

void AppTheme::load(const QString &filename)
{
    QFile file(filename);
    if (!file.exists()) {
        throw FileError(tr("Theme file '%1' doesn't exist!")
                        .arg(filename));
    }
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll();
        QJsonParseError error;
        QJsonDocument doc(QJsonDocument::fromJson(content,&error));
        if (error.error  != QJsonParseError::NoError) {
            throw FileError(tr("Error in json file '%1':%2 : %3")
                            .arg(filename)
                            .arg(error.offset)
                            .arg(error.errorString()));
        }
        QJsonObject obj=doc.object();
        QFileInfo fileInfo(filename);
        mName = fileInfo.baseName();
        mDisplayName = obj["name"].toString();
        QString localeName = obj["name_"+pSettings->environment().language()].toString();
        if (!localeName.isEmpty())
            mDisplayName = localeName;
        mUseQtFusionStyle = obj["useQtFusionStyle"].toBool(true);
        mIsDark = obj["isDark"].toBool(false);
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

bool AppTheme::useQtFusionStyle() const
{
    return mUseQtFusionStyle;
}

bool AppTheme::isDark() const
{
    return mIsDark;
}
