#include "settings.h"
#include <QTextCodec>

Settings* pSettings;

Settings::Settings():
    mSettings(QSettings::IniFormat,QSettings::UserScope,"Red Panda C++")
{

    // default values for editors
    setDefault(EDITOR_DEFAULT_ENCODING, QTextCodec::codecForLocale()->name());
    setDefault(EDITOR_AUTO_INDENT,true);
}


void Settings::setDefault(const QString &key, const QVariant &value) {
    if (!mSettings.contains(key)) {
        mSettings.setValue(key,value);
    }
}

void Settings::setValue(const QString &key, const QVariant &value) {
    mSettings.setValue(key,value);
}

QVariant Settings::value(const QString &key) const {
    return mSettings.value(key);
}
