#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <QColor>
#include <qsynedit/highlighter/base.h>

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

    QMap<QString,PColorSchemeItem> items();
    QString name() const;
    void setName(const QString &name);

    void read(const QJsonObject& json);
    void write(QJsonObject& json);

    void load(const QString& filename);
    void save(const QString& filename);

    bool bundled() const;
    void setBundled(bool bundled);

    bool modified() const;
    void setModified(bool modified);

    QString preferThemeType() const;
    void setPreferThemeType(const QString &preferThemeType);
private:
    QMap<QString,PColorSchemeItem> mItems;
    QString mName;
    QString mPreferThemeType;
    bool mBundled;
    bool mModified;
};

class ColorSchemeItemDefine {
public:
    bool hasBackground() const;
    void setHasBackground(bool hasBackground);

    bool hasForeground() const;
    void setHasForeground(bool hasForeground);

    bool getHasFontStyle() const;
    void setHasFontStyle(bool value);

private:
    bool mHasBackground;
    bool mHasForeground;
    bool hasFontStyle;
};

using PColorSchemeItemDefine = std::shared_ptr<ColorSchemeItemDefine>;

class ColorManager {
public:
    void init();
    void reload();
    QMap<QString,PColorScheme> getSchemesForTheme(QString themeName);
    QMap<QString,PColorScheme> getSchemes();

    bool exists(const QString name);
    PColorScheme create(const QString& name);
    PColorScheme copy(const QString& source);
    bool rename(const QString& oldName, const QString& newName);
    PColorScheme remove(const QString& name);
    PColorScheme get(const QString& name);
    bool isValidName(const QString& name);
    void addDefine(const QString& name, bool hasForeground, bool hasBackground, bool hasFontStyle);
    bool removeDefine(const QString &name);
    PColorSchemeItemDefine getDefine(const QString& name);
private:
    QMap<QString,PColorSchemeItemDefine> mSchemeDefine;
    QMap<QString,PColorScheme> mSchemes;
};

#endif // COLORSCHEME_H
