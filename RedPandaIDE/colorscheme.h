#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <QColor>
#include <qsynedit/highlighter/base.h>

#define EXT_COLOR_SCHEME ".scheme"
#define EXT_PREFIX_CUSTOM ".custom"

#define COLOR_SCHEME_BREAKPOINT "Breakpoint"
#define COLOR_SCHEME_ERROR  "Error"
#define COLOR_SCHEME_ACTIVE_BREAKPOINT  "Active Breakpoint"
#define COLOR_SCHEME_GUTTER "Gutter"
#define COLOR_SCHEME_GUTTER_ACTIVE_LINE "Gutter Active Line"
#define COLOR_SCHEME_SELECTION "Selected text"
#define COLOR_SCHEME_TEXT "Editor Text"
#define COLOR_SCHEME_CURRENT_HIGHLIGHTED_WORD "Current Highlighted Word"
#define COLOR_SCHEME_FOLD_LINE "Fold Line"
#define COLOR_SCHEME_ACTIVE_LINE "Active Line"
#define COLOR_SCHEME_WARNING "Warning"
#define COLOR_SCHEME_INDENT_GUIDE_LINE "Indent Guide Line"
#define COLOR_SCHEME_BRACE_1 "brace/parenthesis/bracket level 1"
#define COLOR_SCHEME_BRACE_2 "brace/parenthesis/bracket level 2"
#define COLOR_SCHEME_BRACE_3 "brace/parenthesis/bracket level 3"
#define COLOR_SCHEME_BRACE_4 "brace/parenthesis/bracket level 4"


class ColorSchemeItem;
using PColorSchemeItem = std::shared_ptr<ColorSchemeItem>;
class ColorSchemeItem {

public:
    explicit ColorSchemeItem();

    QColor foreground() const;
    void setForeground(const QColor &foreground);

    QColor background() const;
    void setBackground(const QColor &background);

    bool bold() const;
    void setBold(bool bold);

    bool italic() const;
    void setItalic(bool italic);

    bool underlined() const;
    void setUnderlined(bool underlined);

    bool strikeout() const;
    void setStrikeout(bool strikeout);

    static PColorSchemeItem fromJson(const QJsonObject& json);
    void toJson(QJsonObject& json);

private:
    QColor mForeground;
    QColor mBackground;
    bool mBold;
    bool mItalic;
    bool mUnderlined;
    bool mStrikeout;
};


class ColorScheme;
using PColorScheme = std::shared_ptr<ColorScheme>;
class ColorScheme
{
public:
    explicit ColorScheme();

    static PColorScheme load(const QString& filename);

    void addItem(const QString& name);

    QMap<QString,PColorSchemeItem> items();

    static PColorScheme fromJson(const QJsonObject& json);
    void toJson(QJsonObject& json);

    //void load();
    void save(const QString& filename);

    bool bundled() const;
    void setBundled(bool bundled);

    bool customed() const;
    void setCustomed(bool customed);

    QString preferThemeType() const;
    void setPreferThemeType(const QString &preferThemeType);
private:
    QMap<QString,PColorSchemeItem> mItems;
    QString mPreferThemeType;
    bool mBundled;
    bool mCustomed;
};

class ColorSchemeItemDefine {
public:
    explicit ColorSchemeItemDefine();
    bool hasBackground() const;
    void setHasBackground(bool hasBackground);

    bool hasForeground() const;
    void setHasForeground(bool hasForeground);

    bool hasFontStyle() const;
    void setHasFontStyle(bool value);

    QString group() const;
    void setGroup(const QString &group);

    QString displayName() const;
    void setDisplayName(const QString &displayName);

private:
    bool mHasBackground;
    bool mHasForeground;
    bool mHasFontStyle;
    QString mGroup;
    QString mDisplayName;
};

using PColorSchemeItemDefine = std::shared_ptr<ColorSchemeItemDefine>;

class ColorManager {
public:
    explicit ColorManager();
    void init();
    void reload();
    QStringList getSchemes(const QString& themeType = QString());
    QStringList getDefines();

    bool exists(const QString name);
    QString copy(const QString& source);
    bool restoreToDefault(const QString& name);
    bool remove(const QString& name);
    bool rename(const QString& oldName, const QString& newName);
    bool add(const QString& name, PColorScheme scheme);
    PColorScheme get(const QString& name);
    PColorSchemeItem getItem(const QString& schemeName, const QString& itemName);
    bool isValidName(const QString& name);
    void addDefine(const QString& name, const QString& displayName, const QString& group, bool hasForeground, bool hasBackground, bool hasFontStyle);
    bool removeDefine(const QString &name);
    PColorSchemeItemDefine getDefine(const QString& name);
    bool saveScheme(const QString &name);
private:
    QString generateFullPathname(const QString& name, bool isBundled, bool isCustomed);
    QString generateFilename(const QString& name, bool isCustomed);
    void loadSchemesInDir(const QString& dirName, bool isBundled, bool isCustomed);
    void initItemDefines();
private:
    QMap<QString,PColorSchemeItemDefine> mSchemeItemDefines;
    QMap<QString,PColorScheme> mSchemes;
    PColorSchemeItemDefine mDefaultSchemeItemDefine;
};

extern ColorManager * pColorManager;

#endif // COLORSCHEME_H
