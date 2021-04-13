#include "settings.h"
#include <QApplication>
#include <QTextCodec>
#include "utils.h"

Settings* pSettings;

Settings::Settings(const QString &filename):
    mSettings(filename,QSettings::IniFormat),
    mDirs(this),
    mEditor(this)
{
    // default values for editors
    mEditor.setDefault(SETTING_EDITOR_DEFAULT_ENCODING, QTextCodec::codecForLocale()->name());
    mEditor.setDefault(SETTING_EDITOR_AUTO_INDENT,true);

}

void Settings::setDefault(const QString&group,const QString &key, const QVariant &value) {
    mSettings.beginGroup(group);
    auto act = finally([this] {
        this->mSettings.endGroup();
    });
    if (!mSettings.contains(key)) {
        mSettings.setValue(key,value);
    }
}

void Settings::setValue(const QString& group, const QString &key, const QVariant &value) {
    mSettings.beginGroup(group);
    auto act = finally([this] {
        this->mSettings.endGroup();
    });
    mSettings.setValue(key,value);
}

QVariant Settings::value(const QString& group, const QString &key) {
    mSettings.beginGroup(group);
    auto act = finally([this] {
        this->mSettings.endGroup();
    });
    return mSettings.value(key);
}

Settings::Dirs &Settings::dirs()
{
    return mDirs;
}

Settings::Editor &Settings::editor()
{
    return mEditor;
}

Settings::Dirs::Dirs(Settings *settings):
    _Base(settings, SETTING_DIRS)
{
}

const QString Settings::Dirs::app() const
{
    QApplication::instance()->applicationDirPath();
}

Settings::_Base::_Base(Settings *settings, const QString &groupName):
    mSettings(settings),
    mGroup(groupName)
{

}

void Settings::_Base::setDefault(const QString &key, const QVariant &value)
{
    mSettings->setDefault(mGroup,key,value);
}

void Settings::_Base::setValue(const QString &key, const QVariant &value)
{
    mSettings->setValue(mGroup,key,value);
}

QVariant Settings::_Base::value(const QString &key)
{
    return mSettings->value(mGroup,key);
}

Settings::Editor::Editor(Settings *settings): _Base(settings, SETTING_EDITOR)
{

}

QByteArray Settings::Editor::defaultEncoding()
{
    return value(SETTING_EDITOR_DEFAULT_ENCODING).toByteArray();
}

void Settings::Editor::setDefaultEncoding(const QByteArray &encoding)
{
    setValue(SETTING_EDITOR_DEFAULT_ENCODING,encoding);
}

bool Settings::Editor::autoIndent()
{
    return value(SETTING_EDITOR_AUTO_INDENT).toBool();
}

void Settings::Editor::setAutoIndent(bool indent)
{
    setValue(SETTING_EDITOR_AUTO_INDENT,indent);
}



Settings::CompilerSet::CompilerSet(Settings *settings, int index, const QString& compilerFolder):
    _Base(settings, "CompilerSet_"+QString(index)),

    mIndex(index)
{

}
