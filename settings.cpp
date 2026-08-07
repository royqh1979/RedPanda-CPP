/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "settings.h"
#include <QApplication>
#include <algorithm>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QStandardPaths>
#include <QScreen>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#ifdef Q_OS_LINUX
#include <sys/sysinfo.h>
#endif
#ifdef ENABLE_LUA_ADDON
# include "addon/luaexecutor.h"
# include "addon/luaruntime.h"
#endif

Settings* pSettings;

Settings::Settings(const QString &filename):
    mFilename(filename),
    mSettings(filename,QSettings::IniFormat),
    mDirs(this),
    mUI(this),
    mView(this),
    mHistory(this)
{
    //load();
}

Settings::~Settings()
{
    //mEditor.save();
}

void Settings::beginGroup(const QString &group)
{
    mSettings.beginGroup(group);
}

void Settings::endGroup()
{
    mSettings.endGroup();
}

void Settings::remove(const QString &key)
{
    mSettings.remove(key);
}

void Settings::saveValue(const QString& group, const QString &key, const QVariant &value) {
    mSettings.beginGroup(group);
    mSettings.setValue(key,value);
    mSettings.endGroup();
}

void Settings::saveValue(const QString &key, const QVariant &value)
{
    mSettings.setValue(key,value);
}

QVariant Settings::value(const QString &group, const QString &key, const QVariant &defaultValue)
{
    mSettings.beginGroup(group);
    return mSettings.value(key,defaultValue);
    this->mSettings.endGroup();
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue)
{
    return mSettings.value(key,defaultValue);
}

void Settings::load()
{
    mDirs.load();
    mUI.load();
    mView.load();
    mHistory.load();
}

void Settings::save()
{
    mDirs.save();
    mUI.save();
    mView.save();
    mHistory.save();
}

QSettings::Status Settings::sync()
{
    mSettings.sync();
    return mSettings.status();
}

Settings::Dirs &Settings::dirs()
{
    return mDirs;
}

Settings::UI &Settings::ui()
{
    return mUI;
}

Settings::View &Settings::view()
{
    return mView;
}

Settings::History &Settings::history()
{
    return mHistory;
}

Settings::Dirs::Dirs(Settings *settings):
    _Base(settings, SETTING_DIRS)
{
}

QString Settings::Dirs::appDir() const
{
    return QApplication::instance()->applicationDirPath();
}

QString Settings::Dirs::executable() const
{
    QString s = QApplication::instance()->applicationFilePath();
    s.replace("/",QDir::separator());
    return s;
}

void Settings::Dirs::doSave()
{
}

void Settings::Dirs::doLoad()
{
}

Settings::_Base::_Base(Settings *settings, const QString &groupName):
    mSettings(settings),
    mGroup(groupName)
{

}

void Settings::_Base::beginGroup()
{
    mSettings->beginGroup(mGroup);
}

void Settings::_Base::endGroup()
{
    mSettings->endGroup();
}

void Settings::_Base::remove(const QString &key)
{
    mSettings->remove(key);
}

void Settings::_Base::saveValue(const QString &key, const QVariant &value)
{
    mSettings->saveValue(key,value);
}

void Settings::_Base::saveValue(const QString &key, const QSet<QString> &set)
{
    QStringList val;
    foreach(const QString& s,set) {
        val.append(s);
    }
    mSettings->saveValue(key,val);
}

QVariant Settings::_Base::value(const QString &key, const QVariant &defaultValue)
{
    return mSettings->value(key,defaultValue);
}

bool Settings::_Base::boolValue(const QString &key, bool defaultValue)
{
    return value(key,defaultValue).toBool();
}

QSize Settings::_Base::sizeValue(const QString &key, const QSize& size)
{
    return value(key,size).toSize();
}

int Settings::_Base::intValue(const QString &key, int defaultValue)
{
    return value(key,defaultValue).toInt();
}

double Settings::_Base::doubleValue(const QString &key, double defaultValue)
{
    return value(key,defaultValue).toDouble();
}

unsigned int Settings::_Base::uintValue(const QString &key, unsigned int defaultValue)
{
    return value(key,defaultValue).toUInt();
}

QStringList Settings::_Base::stringListValue(const QString &key, const QStringList &defaultValue)
{
    return value(key,defaultValue).toStringList();
}

QSet<QString> Settings::_Base::stringSetValue(const QString &key)
{
    QStringList lst=value(key,QStringList()).toStringList();
    QSet<QString> result;
    foreach(const QString& s, lst)
        result.insert(s);
    return result;
}

QColor Settings::_Base::colorValue(const QString &key, const QColor& defaultValue)
{
    return value(key,defaultValue).value<QColor>();
}

QString Settings::_Base::stringValue(const QString &key, const QString& defaultValue)
{
    return value(key,defaultValue).toString();
}

void Settings::_Base::save()
{
    beginGroup();
    doSave();
    endGroup();
}

void Settings::_Base::load()
{
    beginGroup();
    doLoad();
    endGroup();
}


Settings::UI::UI(Settings *settings) :
    _Base(settings, SETTING_UI)
{

}

int Settings::UI::settingsDialogWidth() const
{
    return mSettingsDialogWidth;
}

void Settings::UI::setSettingsDialogWidth(int newSettingsDialogWidth)
{
    mSettingsDialogWidth = newSettingsDialogWidth;
}

int Settings::UI::settingsDialogHeight() const
{
    return mSettingsDialogHeight;
}

void Settings::UI::setSettingsDialogHeight(int newSettingsDialogHeight)
{
    mSettingsDialogHeight = newSettingsDialogHeight;
}

int Settings::UI::settingsDialogSplitterPos() const
{
    return mSettingsDialogSplitterPos;
}

void Settings::UI::setSettingsDialogSplitterPos(int newSettingsDialogSplitterPos)
{
    mSettingsDialogSplitterPos = newSettingsDialogSplitterPos;
}

void Settings::UI::doSave()
{
    saveValue("SettingsDialogWidth",mSettingsDialogWidth);
    saveValue("SettingsDialogHeight",mSettingsDialogHeight);
    saveValue("SettingsDialogSplitterPos",mSettingsDialogSplitterPos);
    saveValue("FontName", mFontName);
    saveValue("FontSize",mFontSize);
    saveValue("ShowContentsPanel", mShowContentsPanel);
    saveValue("MainWindowWidth", mMainWindowWidth);
    saveValue("MainWindowHeight", mMainWindowHeight);
    saveValue("MainWindowLeft", mMainWindowLeft);
    saveValue("MainWindowTop", mMainWindowTop);
    saveValue("ContentsPanelWidth", mContentsPanelWidth);
}

void Settings::UI::doLoad()
{
    QRect geometry = qApp->primaryScreen()->geometry();
    int width = geometry.width();
    int height = geometry.height();
    mSettingsDialogWidth = intValue("SettingsDialogWidth", 977 * width / 1920);
    mSettingsDialogHeight = intValue("SettingsDialogHeight", 622 * height / 1080);
    mSettingsDialogSplitterPos = intValue("SettingsDialogSplitterPos", 300 * width / 1920);

    mFontName = stringValue("FontName", qApp->font().family());
    mFontSize = intValue("FontSize", 12);

    mShowContentsPanel = boolValue("ShowContentsPanel", true);

    mMainWindowWidth = intValue("MainWindowWidth", 1350 * width / 1920);
    mMainWindowHeight = intValue("MainWindowHeight", 810 * height / 1080);
    mContentsPanelWidth = intValue("ContentsPanelWidth", 270 * width / 1920);
    mMainWindowLeft = intValue("MainWindowLeft", 100 * width / 1920);
    mMainWindowTop = intValue("MainWindowTop", 30 * height / 1080 );
}

int Settings::UI::mainWindowTop() const
{
    return mMainWindowTop;
}

void Settings::UI::setMainWindowTop(int newMainWindowTop)
{
    mMainWindowTop = newMainWindowTop;
}

int Settings::UI::mainWindowLeft() const
{
    return mMainWindowLeft;
}

void Settings::UI::setMainWindowLeft(int newMainWindowLeft)
{
    mMainWindowLeft = newMainWindowLeft;
}

int Settings::UI::mainWindowHeight() const
{
    return mMainWindowHeight;
}

void Settings::UI::setMainWindowHeight(int newMainWindowHeight)
{
    mMainWindowHeight = newMainWindowHeight;
}

int Settings::UI::mainWindowWidth() const
{
    return mMainWindowWidth;
}

void Settings::UI::setMainWindowWidth(int newMainWindowWidth)
{
    mMainWindowWidth = newMainWindowWidth;
}

bool Settings::UI::showContentsPanel() const
{
    return mShowContentsPanel;
}

void Settings::UI::setShowContentsPanel(bool newShowContentsPanel)
{
    mShowContentsPanel = newShowContentsPanel;
}

int Settings::UI::fontSize() const
{
    return mFontSize;
}

void Settings::UI::setFontSize(int newFontSize)
{
    mFontSize = newFontSize;
}

const QString &Settings::UI::fontName() const
{
    return mFontName;
}

void Settings::UI::setFontName(const QString &newFontName)
{
    mFontName = newFontName;
}

Settings::View::View(Settings *settings):
        _Base(settings, SETTING_VIEW)
{

}

const QString &Settings::View::fitMode() const
{
    return mFitMode;
}

void Settings::View::setFitMode(const QString &newFitMode)
{
    mFitMode = newFitMode;
}

void Settings::View::doSave()
{
    saveValue("FitMode",mFitMode);
    saveValue("SlideShowDelayTime", mSlideShowDelayTime);
}

void Settings::View::doLoad()
{
    mFitMode = stringValue("FitMode","None");
    mSlideShowDelayTime = intValue("SlideShowDelayTime", 5000);
}

int Settings::View::slideShowDelayTime() const
{
    return mSlideShowDelayTime;
}

void Settings::View::setSlideShowDelayTime(int newSlideShowDelayTime)
{
    mSlideShowDelayTime = newSlideShowDelayTime;
}

int Settings::UI::contentsPanelWidth() const
{
    return mContentsPanelWidth;
}

void Settings::UI::setContentsPanelWidth(int newContentsPanelWidth)
{
    mContentsPanelWidth = newContentsPanelWidth;
}

Settings::History::History(Settings *settings):
    _Base(settings, SETTING_HISTORY)
{
}

QStringList Settings::History::recentDirs() const
{
    return mRecentDirs;
}

void Settings::History::setRecentDirs(const QStringList &newRecentDirs)
{
    mRecentDirs = newRecentDirs;
}

void Settings::History::addRecentDir(const QString &dir)
{
    QString normalizedDir = QDir(dir).absolutePath();
    mRecentDirs.removeAll(normalizedDir);
    mRecentDirs.prepend(normalizedDir);
    while (mRecentDirs.size() > 10) {
        mRecentDirs.removeLast();
    }
}

void Settings::History::clearRecentDirs()
{
    mRecentDirs.clear();
}

void Settings::History::doSave()
{
    saveValue("RecentDirs", mRecentDirs);
}

void Settings::History::doLoad()
{
    mRecentDirs = stringListValue("RecentDirs", QStringList());
}
