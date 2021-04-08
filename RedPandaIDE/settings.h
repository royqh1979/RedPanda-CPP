#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

#define EDITOR_DEFAULT_ENCODING "editor/default_encoding"

class Settings
{
public:


public:
    Settings();

    void setDefault(const QString &key, const QVariant &value);
    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key) const;
private:
    QSettings mSettings;
};

extern Settings* pSettings;

#endif // SETTINGS_H
