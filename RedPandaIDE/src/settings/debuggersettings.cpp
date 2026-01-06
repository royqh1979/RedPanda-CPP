#include "debuggersettings.h"
#include "../utils/font.h"

DebuggerSettings::DebuggerSettings(SettingsPersistor *persistor):
    BaseSettings{persistor, SETTING_DEBUGGER}
{

}

bool DebuggerSettings::enableDebugConsole() const
{
    return mEnableDebugConsole;
}

void DebuggerSettings::setEnableDebugConsole(bool showCommandLog)
{
    mEnableDebugConsole = showCommandLog;
}

bool DebuggerSettings::showDetailLog() const
{
    return mShowDetailLog;
}

void DebuggerSettings::setShowDetailLog(bool showAnnotations)
{
    mShowDetailLog = showAnnotations;
}

QString DebuggerSettings::fontName() const
{
    return mFontName;
}

void DebuggerSettings::setFontName(const QString &fontName)
{
    mFontName = fontName;
}

bool DebuggerSettings::blendMode() const
{
    return mBlendMode;
}

void DebuggerSettings::setBlendMode(bool blendMode)
{
    mBlendMode = blendMode;
}

bool DebuggerSettings::skipSystemLibraries() const
{
    return mSkipSystemLibraries;
}

void DebuggerSettings::setSkipSystemLibraries(bool newSkipSystemLibraries)
{
    mSkipSystemLibraries = newSkipSystemLibraries;
}

bool DebuggerSettings::skipProjectLibraries() const
{
    return mSkipProjectLibraries;
}

void DebuggerSettings::setSkipProjectLibraries(bool newSkipProjectLibraries)
{
    mSkipProjectLibraries = newSkipProjectLibraries;
}

bool DebuggerSettings::skipCustomLibraries() const
{
    return mSkipCustomLibraries;
}

void DebuggerSettings::setSkipCustomLibraries(bool newSkipCustomLibraries)
{
    mSkipCustomLibraries = newSkipCustomLibraries;
}

bool DebuggerSettings::openCPUInfoWhenSignaled() const
{
    return mOpenCPUInfoWhenSignaled;
}

void DebuggerSettings::setOpenCPUInfoWhenSignaled(bool newOpenCPUInfoWhenSignaled)
{
    mOpenCPUInfoWhenSignaled = newOpenCPUInfoWhenSignaled;
}

bool DebuggerSettings::useGDBServer() const
{
    return mUseGDBServer;
}

void DebuggerSettings::setUseGDBServer(bool newUseGDBServer)
{
    mUseGDBServer = newUseGDBServer;
}

int DebuggerSettings::GDBServerPort() const
{
    return mGDBServerPort;
}

void DebuggerSettings::setGDBServerPort(int newGDBServerPort)
{
    mGDBServerPort = newGDBServerPort;
}

int DebuggerSettings::memoryViewRows() const
{
    return mMemoryViewRows;
}

void DebuggerSettings::setMemoryViewRows(int newMemoryViewRows)
{
    mMemoryViewRows = newMemoryViewRows;
}

int DebuggerSettings::memoryViewColumns() const
{
    return mMemoryViewColumns;
}

void DebuggerSettings::setMemoryViewColumns(int newMemoryViewColumns)
{
    mMemoryViewColumns = newMemoryViewColumns;
}

bool DebuggerSettings::autosave() const
{
    return mAutosave;
}

void DebuggerSettings::setAutosave(bool newAutosave)
{
    mAutosave = newAutosave;
}

int DebuggerSettings::arrayElements() const
{
    return mArrayElements;
}

void DebuggerSettings::setArrayElements(int newArrayElements)
{
    mArrayElements = newArrayElements;
}

int DebuggerSettings::characters() const
{
    return mCharacters;
}

void DebuggerSettings::setCharacters(int newCharacters)
{
    mCharacters = newCharacters;
}

bool DebuggerSettings::useIntelStyle() const
{
    return mUseIntelStyle;
}

void DebuggerSettings::setUseIntelStyle(bool useIntelStyle)
{
    mUseIntelStyle = useIntelStyle;
}

int DebuggerSettings::fontSize() const
{
    return mFontSize;
}

void DebuggerSettings::setFontSize(int fontSize)
{
    mFontSize = fontSize;
}

bool DebuggerSettings::onlyShowMono() const
{
    return mOnlyShowMono;
}

void DebuggerSettings::setOnlyShowMono(bool onlyShowMono)
{
    mOnlyShowMono = onlyShowMono;
}

void DebuggerSettings::doSave()
{
    saveValue("enable_debug_console", mEnableDebugConsole);
    saveValue("show_detail_log", mShowDetailLog);
    saveValue("font_name",mFontName);
    saveValue("only_show_mono",mOnlyShowMono);
    saveValue("font_size",mFontSize);
    saveValue("use_intel_style",mUseIntelStyle);
    saveValue("blend_mode",mBlendMode);
    saveValue("skip_system_lib", mSkipSystemLibraries);
    saveValue("skip_project_lib", mSkipProjectLibraries);
    saveValue("skip_custom_lib", mSkipCustomLibraries);
    saveValue("autosave",mAutosave);
    saveValue("open_cpu_info_when_signaled",mOpenCPUInfoWhenSignaled);
    saveValue("use_gdb_server", mUseGDBServer);
    saveValue("gdb_server_port",mGDBServerPort);
    saveValue("memory_view_rows",mMemoryViewRows);
    saveValue("memory_view_columns",mMemoryViewColumns);
    saveValue("array_elements",mArrayElements);
    saveValue("string_characters",mCharacters);
}

void DebuggerSettings::doLoad()
{
    mEnableDebugConsole = boolValue("enable_debug_console",true);
    mShowDetailLog = boolValue("show_detail_log",false);
    mFontName = stringValue("font_name", defaultMonoFont());
    mOnlyShowMono = boolValue("only_show_mono",true);
    mFontSize = intValue("font_size",14);
    mUseIntelStyle = boolValue("use_intel_style",false);
    mBlendMode = boolValue("blend_mode",true);
    mSkipSystemLibraries = boolValue("skip_system_lib",true);
    mSkipProjectLibraries = boolValue("skip_project_lib",true);
    mSkipCustomLibraries = boolValue("skip_custom_lib",false);
    mAutosave = boolValue("autosave",true);
    mOpenCPUInfoWhenSignaled = boolValue("open_cpu_info_when_signaled",true);
#ifdef Q_OS_WIN
    mUseGDBServer = boolValue("use_gdb_server", false);
#else
    mUseGDBServer = true;
#endif
    mGDBServerPort = intValue("gdb_server_port",41234);
    mMemoryViewRows = intValue("memory_view_rows",16);
    mMemoryViewColumns = intValue("memory_view_columns",16);
    mArrayElements = intValue("array_elements",100);
    mCharacters = intValue("string_characters",300);
}
