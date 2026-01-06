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
#ifndef SETTINGS_H
#define SETTINGS_H

#include "settings/basesettings.h"
#include "settings/codecompletionsettings.h"
#include "settings/codeformattersettings.h"
#include "settings/compilersetsettings.h"
#include "settings/compilesettings.h"
#include "settings/debuggersettings.h"
#include "settings/dirsettings.h"
#include "settings/editorsettings.h"
#include "settings/environmentsettings.h"
#include "settings/executorsettings.h"
#include "settings/languagesettings.h"
#include "settings/uisettings.h"
#ifdef ENABLE_VCS
#include "settings/vcssettings.h"
#endif

/**
 * use the following command to get gcc's default bin/library folders:
 * gcc -print-search-dirs
 */






#define SETTING_HISTORY "History"




class Settings
{
public:
    explicit Settings(const QString& filename);
    explicit Settings(Settings&& settings) = delete;
    explicit Settings(const Settings& settings) = delete;

    Settings& operator= (const Settings& settings) = delete;
    Settings& operator= (const Settings&& settings) = delete;
    void load();
    QSettings::Status sync();

    DirSettings& dirs();
    EditorSettings& editor();
    CompilerSets& compilerSets();
    EnvironmentSettings& environment();
    ExecutorSettings& executor();
    DebuggerSettings& debugger();
    CodeCompletionSettings &codeCompletion();
    CodeFormatterSettings &codeFormatter();
    CompileSettings &compile();
    UISettings &ui();
#ifdef ENABLE_VCS
    VCS &vcs();
#endif
    LanguageSettings &languages();
    QString filename() const;

private:
    QString mFilename;
    SettingsPersistor mPersistor;
    DirSettings mDirs;
    EditorSettings mEditor;
    EnvironmentSettings mEnvironment;
    CompilerSets mCompilerSets;
    ExecutorSettings mExecutor;
    DebuggerSettings mDebugger;
    CodeCompletionSettings mCodeCompletion;
    CodeFormatterSettings mCodeFormatter;
    CompileSettings mCompile;
    UISettings mUI;
#ifdef ENABLE_VCS
    VCSSetting mVCS;
#endif
    LanguageSettings mLanguages;
};

extern Settings* pSettings;

#endif // SETTINGS_H
