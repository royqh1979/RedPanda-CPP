#ifndef COLORSCHEMA_H
#define COLORSCHEMA_H

#include <QColor>
#include <qsynedit/highlighter/base.h>

class ColorSchemaItem {

public:
    explicit ColorSchemaItem(const QString& name);
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

using PColorSchemaItem = std::shared_ptr<ColorSchemaItem>;

class ColorSchema
{
public:
    ColorSchema();

    QMap<QString,PColorSchemaItem> items();
    QString name() const;
    void setName(const QString &name);

    void read(const QJsonObject& json);
    void write(QJsonObject& json);

    void load(const QString& filename);
    void save(const QString& filename);

private:
    QMap<QString,PColorSchemaItem> mItems;
    QString mName;
};

#endif // COLORSCHEMA_H
