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
#include <QObject>
#include <QPalette>
#include <QHash>
#include <QColor>
#include <memory>

class AppTheme:public QObject {
    Q_OBJECT
public:
    explicit AppTheme(QObject* parent = nullptr);

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

    QColor color(ColorRole role) const;
    QPalette palette() const;

    void load(const QString& filename);

    bool useQtFusionStyle() const;
    bool isDark() const;

    const QString &defaultColorScheme() const;
    void setDefaultColorScheme(const QString &newDefaultColorScheme);

    const QString &displayName() const;

    const QString &name() const;

    const QString &defaultIconSet() const;
    void setDefaultIconSet(const QString &newDefaultIconSet);

private:
    static QPalette initialPalette();
private:
    QHash<int,QColor> mColors;
    QString mName;
    QString mDisplayName;
    bool mUseQtFusionStyle;
    bool mIsDark;
    QString mDefaultColorScheme;
    QString mDefaultIconSet;
};

using PAppTheme = std::shared_ptr<AppTheme>;

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);
    PAppTheme theme(const QString& themeName);
    bool useCustomTheme() const;
    void setUseCustomTheme(bool newUseCustomTheme);
    void prepareCustomeTheme();
    QList<PAppTheme> getThemes();

private:
    bool mUseCustomTheme;
};

#endif // THEMEMANAGER_H
