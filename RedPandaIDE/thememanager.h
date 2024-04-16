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
#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H
#include <QPalette>
#include <QHash>
#include <QColor>
#include <memory>
#include <QObject>
#include <set>

class AppTheme;
using PAppTheme = std::shared_ptr<AppTheme>;

class AppTheme : public QObject{
    Q_OBJECT
public:
    enum ColorRole {
        /* Color for QPalette */

        PaletteWindow,
        PaletteWindowText,
        PaletteBase,
        PaletteAlternateBase,
        PaletteToolTipBase,
        PaletteToolTipText,
        PaletteText,
        PaletteButton,
        PaletteButtonText,
        PaletteBrightText,
        PaletteHighlight,
        PaletteHighlightedText,
        PaletteLink,
        PaletteLinkVisited,

        PaletteLight,
        PaletteMidlight,
        PaletteDark,
        PaletteMid,
        PaletteShadow,

        PaletteWindowDisabled,
        PaletteWindowTextDisabled,
        PaletteBaseDisabled,
        PaletteAlternateBaseDisabled,
        PaletteToolTipBaseDisabled,
        PaletteToolTipTextDisabled,
        PaletteTextDisabled,
        PaletteButtonDisabled,
        PaletteButtonTextDisabled,
        PaletteBrightTextDisabled,
        PaletteHighlightDisabled,
        PaletteHighlightedTextDisabled,
        PaletteLinkDisabled,
        PaletteLinkVisitedDisabled,

        PaletteLightDisabled,
        PaletteMidlightDisabled,
        PaletteDarkDisabled,
        PaletteMidDisabled,
        PaletteShadowDisabled
    };

    Q_ENUM(ColorRole)

    enum class ThemeType {
        JSON,
#ifdef ENABLE_LUA_ADDON
        Lua,
#endif
    };

    AppTheme(const QString& filename, ThemeType type, QObject* parent=nullptr);

    QColor color(ColorRole role) const;
    QPalette palette() const;

    const QString &style() const;

    const QString &defaultColorScheme() const;
    void setDefaultColorScheme(const QString &newDefaultColorScheme);

    const QString &displayName() const;
    void setDisplayName(const QString &newDisplayName);

    const QString &category() const;
    void setCategory(const QString &newCategory);

    const QString &name() const;

    const QString &defaultIconSet() const;
    void setDefaultIconSet(const QString &newDefaultIconSet);

    static bool isSystemInDarkMode();
    static QString initialStyle();

    const QString& filename() const;

public:
    static PAppTheme fallbackTheme();

private:
    AppTheme();

private:
    static QPalette initialPalette();
private:
    QHash<int,QColor> mColors;
    QString mName;
    QString mDisplayName;
    QString mCategory;
    QString mStyle;
    QString mDefaultColorScheme;
    QString mDefaultIconSet;
    QString mFilename;
};

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);
    PAppTheme theme(const QString& themeName);
    QList<PAppTheme> getThemes();

private:
    struct ThemeCompare {
        bool operator()(const PAppTheme &lhs, const PAppTheme &rhs) const;
    };

private:
    bool tryLoadThemeFromDir(const QString &dir, const QString &dirType, const QString &themeName, PAppTheme &theme);
    void loadThemesFromDir(const QString &dir, const QString &dirType, std::set<PAppTheme, ThemeCompare> &themes);

    // lua overrides json
    inline static const std::pair<QString, AppTheme::ThemeType> searchTypes[] = {
#ifdef ENABLE_LUA_ADDON
        { "lua", AppTheme::ThemeType::Lua },
#endif
        { "json", AppTheme::ThemeType::JSON },
    };
};

#endif // THEMEMANAGER_H
