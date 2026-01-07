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
#ifndef DEBUGGER_SETTINGS_H
#define DEBUGGER_SETTINGS_H
#include "basesettings.h"

#define SETTING_DEBUGGER "Debugger"

class DebuggerSettings: public BaseSettings {
public:
    explicit DebuggerSettings(SettingsPersistor* persistor);
    bool enableDebugConsole() const;
    void setEnableDebugConsole(bool showCommandLog);

    bool showDetailLog() const;
    void setShowDetailLog(bool showAnnotations);

    bool onlyShowMono() const;
    void setOnlyShowMono(bool onlyShowMono);

    int fontSize() const;
    void setFontSize(int fontSize);

    bool useIntelStyle() const;
    void setUseIntelStyle(bool useIntelStyle);

    QString fontName() const;
    void setFontName(const QString &fontName);

    bool blendMode() const;
    void setBlendMode(bool blendMode);

    bool skipSystemLibraries() const;
    void setSkipSystemLibraries(bool newSkipSystemLibraries);
    bool skipProjectLibraries() const;
    void setSkipProjectLibraries(bool newSkipProjectLibraries);
    bool skipCustomLibraries() const;
    void setSkipCustomLibraries(bool newSkipCustomLibraries);

    bool openCPUInfoWhenSignaled() const;
    void setOpenCPUInfoWhenSignaled(bool newOpenCPUInfoWhenSignaled);

    bool useGDBServer() const;
    void setUseGDBServer(bool newUseGDBServer);
    int GDBServerPort() const;
    void setGDBServerPort(int newGDBServerPort);

    int memoryViewRows() const;
    void setMemoryViewRows(int newMemoryViewRows);

    int memoryViewColumns() const;
    void setMemoryViewColumns(int newMemoryViewColumns);

    bool autosave() const;
    void setAutosave(bool newAutosave);

    int arrayElements() const;
    void setArrayElements(int newArrayElements);

    int characters() const;
    void setCharacters(int newCharacters);

private:
    bool mEnableDebugConsole;
    bool mShowDetailLog;
    QString mFontName;
    bool mOnlyShowMono;
    int mFontSize;
    bool mUseIntelStyle;
    bool mBlendMode;
    bool mSkipSystemLibraries;
    bool mSkipProjectLibraries;
    bool mSkipCustomLibraries;
    bool mAutosave;
    bool mOpenCPUInfoWhenSignaled;
    bool mUseGDBServer;
    int mGDBServerPort;
    int mMemoryViewRows;
    int mMemoryViewColumns;
    int mArrayElements;
    int mCharacters;

    // _Base interface
protected:
    void doSave() override;
    void doLoad() override;
};


#endif
//DEBUGGER_SETTINGS_H
