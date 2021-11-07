#include "thememanager.h"
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QMetaObject>
#include "utils.h"

ThemeManager::ThemeManager(QObject *parent) : QObject(parent)
{

}

PAppTheme ThemeManager::theme(const QString &themeName)
{
   PAppTheme appTheme = std::make_shared<AppTheme>();
   appTheme->load(QString(":/themes/%1.json").arg(themeName));
   return appTheme;
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
    if (!file.exists())
        return;
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
        mName = obj["name"].toString();
        mIsDark = obj["isDark"].toBool(false);
        mDefaultColorScheme = obj["default scheme"].toString();
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
        throw FileError(tr("Can't open file '%1' for read.")
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

const QString &AppTheme::defaultColorScheme() const
{
    return mDefaultColorScheme;
}

void AppTheme::setDefaultColorScheme(const QString &newDefaultColorScheme)
{
    mDefaultColorScheme = newDefaultColorScheme;
}

bool AppTheme::isDark() const
{
    return mIsDark;
}
