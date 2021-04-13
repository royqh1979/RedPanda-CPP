#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

#define SETTING_DIRS "dirs"
#define SETTING_EDITOR "editor"
#define SETTING_EDITOR_DEFAULT_ENCODING "default_encoding"
#define SETTING_EDITOR_AUTO_INDENT "default_auto_indent"


class Settings
{
private:
    class _Base {
    public:
        explicit _Base(Settings* settings, const QString& groupName);
        void setDefault(const QString &key, const QVariant &value);
        void setValue(const QString &key, const QVariant &value);
        QVariant value(const QString &key);
    protected:
        Settings* mSettings;
        QString mGroup;
    };

public:
    class Dirs: public _Base {
    public:
        explicit Dirs(Settings * settings);
        const QString app() const;
    };

    class Editor: public _Base {
    public:
        explicit Editor(Settings * settings);
        QByteArray defaultEncoding();
        void setDefaultEncoding(const QByteArray& encoding);
        bool autoIndent();
        void setAutoIndent(bool indent);
    };

public:
    Settings();

    void setDefault(const QString& group, const QString &key, const QVariant &value);
    void setValue(const QString& group, const QString &key, const QVariant &value);
    QVariant value(const QString& group, const QString &key);

    Dirs& dirs();
    Editor& editor();
private:
    QSettings mSettings;
    Dirs mDirs;
    Editor mEditor;
};


extern Settings* pSettings;

#endif // SETTINGS_H
