#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <QColor>
#include <qsynedit/highlighter/base.h>

#define EXT_COLOR_SCHEME ".scheme"
#define EXT_PREFIX_CUSTOM ".custom"

#define COLOR_SCHEME_BREAKPOINT "breakpoint"
#define COLOR_SCHEME_ERROR  "error"
#define COLOR_SCHEME_ACTIVE_BREAKPOINT  "active breakpoint"
#define COLOR_SCHEME_GUTTER "gutter"
#define COLOR_SCHEME_SELECTION "selected text"
#define COLOR_SCHEME_FOLD_LINE "fold line"
#define COLOR_SCHEME_ACTIVE_LINE "active line"
#define COLOR_SCHEME_WARNING "warning"
#define COLOR_SCHEME_INDENT_GUIDE_LINE "indent guide line"
#define COLOR_SCHEME_BRACE_1 "brace/parenthesis/bracket level 1"
#define COLOR_SCHEME_BRACE_2 "brace/parenthesis/bracket level 2"
#define COLOR_SCHEME_BRACE_3 "brace/parenthesis/bracket level 3"
#define COLOR_SCHEME_BRACE_4 "brace/parenthesis/bracket level 4"

class ColorSchemeItem {

public:
    explicit ColorSchemeItem(const QString& name);
    QString name() const;

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

    void read(const QJsonObject& json);
    void write(QJsonObject& json);

private:
    QString mName;
    QColor mForeground;
    QColor mBackground;
    bool mBold;
    bool mItalic;
    bool mUnderlined;
    bool mStrikeout;
};

using PColorSchemeItem = std::shared_ptr<ColorSchemeItem>;

class ColorScheme;
using PColorScheme = std::shared_ptr<ColorScheme>;
class ColorScheme
{
public:
    explicit ColorScheme();

    static PColorScheme load(const QString& filename);

    QMap<QString,PColorSchemeItem> items();
    QString name() const;
    void setName(const QString &name);

    void read(const QJsonObject& json);
    void write(QJsonObject& json);

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
    QString mName;
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

private:
    bool mHasBackground;
    bool mHasForeground;
    bool mHasFontStyle;
};

using PColorSchemeItemDefine = std::shared_ptr<ColorSchemeItemDefine>;

class ColorManager {
public:
    explicit ColorManager();
    void init();
    void reload();
    QStringList getSchemes(const QString& themeType = QString());

    bool exists(const QString name);
    PColorScheme copy(const QString& source);
    bool rename(const QString& oldName, const QString& newName);
    PColorScheme remove(const QString& name);
    PColorScheme get(const QString& name);
    bool isValidName(const QString& name);
    void addDefine(const QString& name, bool hasForeground, bool hasBackground, bool hasFontStyle);
    bool removeDefine(const QString &name);
    PColorSchemeItemDefine getDefine(const QString& name);
private:
    QString generateFullPathname(const QString& name, bool isBundled, bool isCustomed);
    QString generateFilename(const QString& name, bool isCustomed);
    void loadSchemesInDir(const QString& dirName, bool isCustomed);
    void initItemDefines();
private:
    QMap<QString,PColorSchemeItemDefine> mSchemeItemDefine;
    QMap<QString,PColorScheme> mSchemes;
    PColorSchemeItemDefine mDefaultSchemeItemDefine;
};

extern ColorManager * pColorManager;

#endif // COLORSCHEME_H
