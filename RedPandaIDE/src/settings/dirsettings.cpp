/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#include "dirsettings.h"
#include "../utils.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

using DataType = DirSettings::DataType;

DirSettings::DirSettings(SettingsPersistor *persistor):
    BaseSettings{persistor, SETTING_DIRS}
{
}

QString DirSettings::appDir()
{
    return QApplication::instance()->applicationDirPath();
}

QString DirSettings::appResourceDir()
{
#ifdef Q_OS_WIN
    return appDir();
#elif defined(Q_OS_MACOS)
//    return QApplication::instance()->applicationDirPath();
    return "";
#else // XDG desktop
    // in AppImage or tarball PREFIX is not true, resolve from relative path
    const static QString absoluteResourceDir(QDir(appDir()).absoluteFilePath("../share/" APP_NAME));
    return absoluteResourceDir;
#endif
}


QString DirSettings::appLibexecDir()
{
#ifdef Q_OS_WIN
    return appDir();
#elif defined(Q_OS_MACOS)
    return QApplication::instance()->applicationDirPath();
#else // XDG desktop
    const static QString libExecDir(QDir(appDir()).absoluteFilePath("../" LIBEXECDIR "/" APP_NAME));
    return libExecDir;
#endif
}

QString DirSettings::projectDir() const
{
    return mProjectDir;
}

QString DirSettings::data(DirSettings::DataType dataType)
{
    QString dataDir = getFilePath(appDir(), +"data");
    switch (dataType) {
    case DataType::None:
        return dataDir;
    case DataType::ColorScheme:
        return ":/resources/colorschemes";
    case DataType::IconSet:
        return ":/resources/iconsets";
    case DataType::Theme:
        return ":/resources/themes";
    case DataType::Template:
        return getFilePath(appResourceDir(),"templates");
    }
    return "";
}

QString DirSettings::config(DirSettings::DataType dataType) const
{

    QFileInfo configFile(mPersistor->filename());
    QString configDir = configFile.path();
    switch (dataType) {
    case DataType::None:
        return configDir;
    case DataType::ColorScheme:
        return getAbsoluteFilePath(configDir, "scheme");
    case DataType::IconSet:
        return getAbsoluteFilePath(configDir, "iconsets");
    case DataType::Theme:
        return getAbsoluteFilePath(configDir, "themes");
    case DataType::Template:
        return getAbsoluteFilePath(configDir, "templates");
    }
    return "";
}

QString DirSettings::executable()
{
    QString s = QApplication::instance()->applicationFilePath();
    s.replace("/",QDir::separator());
    return s;
}

void DirSettings::doSave()
{
    saveValue("projectDir",mProjectDir);
}

void DirSettings::doLoad()
{
    QString defaultProjectDir;
    if (isGreenEdition()) {
        defaultProjectDir = getFilePath(appDir(), "projects");
    } else {
        QStringList docLocations = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
        defaultProjectDir = getFilePath(
                                docLocations.first(),
                                "projects");
    }
    mProjectDir = stringValue("projectDir",defaultProjectDir);
}

void DirSettings::setProjectDir(const QString &newProjectDir)
{
    mProjectDir = newProjectDir;
}
