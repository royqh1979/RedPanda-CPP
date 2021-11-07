#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H
#include <QObject>
#include <QPalette>
#include <QHash>
#include <QColor>


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

    bool isDark() const;

    const QString &defaultColorScheme() const;
    void setDefaultColorScheme(const QString &newDefaultColorScheme);

private:
    static QPalette initialPalette();
private:
    QHash<int,QColor> mColors;
    QString mName;
    bool mIsDark;
    QString mDefaultColorScheme;
};

using PAppTheme = std::shared_ptr<AppTheme>;

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);
    PAppTheme theme(const QString& themeName);
signals:
};

#endif // THEMEMANAGER_H
