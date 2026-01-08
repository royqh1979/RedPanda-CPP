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
#include "compiler/compilerinfo.h"
#include "utils.h"
#include "utils/escape.h"
#include "utils/font.h"
#include "utils/parsearg.h"
#include <QDir>
#include "systemconsts.h"
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
    mFilename{filename},
    mPersistor{filename},
    mDirs{&mPersistor},
    mEditor{&mPersistor},
    mEnvironment{&mPersistor, &mDirs},
    mCompilerSets{&mPersistor, &mDirs},
    mExecutor{&mPersistor},
    mDebugger{&mPersistor},
    mCodeCompletion{&mPersistor},
    mCodeFormatter{&mPersistor},
    mCompile{&mPersistor},
    mUI{&mPersistor},
#ifdef ENABLE_VCS
    mVCS{&mPersistor},
#endif
    mLanguages{&mPersistor}
{
    //load();
}

void Settings::load()
{
    mCompilerSets.loadSets();
    mEnvironment.load();
    mEditor.load();
    mExecutor.load();
    mDebugger.load();
    mCodeCompletion.load();
    mCodeFormatter.load();
    mCompile.load();
    mUI.load();
    mDirs.load();
#ifdef ENABLE_VCS
    mVCS.load();
#endif
    mLanguages.load();
}

QSettings::Status Settings::sync()
{
    return mPersistor.sync();
}


DirSettings &Settings::dirs()
{
    return mDirs;
}

EditorSettings &Settings::editor()
{
    return mEditor;
}

CompilerSets &Settings::compilerSets()
{
    return mCompilerSets;
}

EnvironmentSettings &Settings::environment()
{
    return mEnvironment;
}

ExecutorSettings &Settings::executor()
{
    return mExecutor;
}

QString Settings::filename() const
{
    return mFilename;
}

LanguageSettings &Settings::languages()
{
    return mLanguages;
}

CodeCompletionSettings &Settings::codeCompletion()
{
    return mCodeCompletion;
}

CodeFormatterSettings &Settings::codeFormatter()
{
    return mCodeFormatter;
}

CompileSettings &Settings::compile()
{
    return mCompile;
}

UISettings &Settings::ui()
{
    return mUI;
}

#ifdef ENABLE_VCS
VCSSettings &Settings::vcs()
{
    return mVCS;
}
#endif

DebuggerSettings& Settings::debugger()
{
    return mDebugger;
}


















#ifdef ENABLE_VCS

#endif

