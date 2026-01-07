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
#ifndef COMPILE_SET_SETTINGS_H
#define COMPILE_SET_SETTINGS_H
#include "basesettings.h"
#include "../compiler/compilerinfo.h"
#include "dirsettings.h"

#define SETTING_COMPILTER_SETS "CompilerSets"
#define SETTING_COMPILTER_SETS_DEFAULT_INDEX "defaultIndex"
#define SETTING_COMPILTER_SETS_DEFAULT_INDEX_TIMESTAMP "defaultIndexTimestamp"
#define SETTING_COMPILTER_SETS_COUNT "count"
#define SETTING_COMPILTER_SET "CompilerSet_%1"

class CompilerSet {
public:
    enum class CompilationStage {
        PreprocessingOnly,
        CompilationProperOnly,
        AssemblingOnly,
        GenerateExecutable,
        GenerateGimple,
    };

    explicit CompilerSet();
    explicit CompilerSet(const QString& compilerFolder, const QString& c_prog);
    explicit CompilerSet(const CompilerSet& set);
    explicit CompilerSet(const QJsonObject& set);

    CompilerSet& operator= (const CompilerSet& ) = delete;
    CompilerSet& operator= (const CompilerSet&& ) = delete;

    // Initialization
    void setProperties(const QString& binDir, const QString& c_prog);
    QStringList x86MultilibList(const QString &folder, const QString &c_prog) const;

    void resetCompileOptionts();
    bool setCompileOption(const QString& key, int valIndex);
    bool setCompileOption(const QString& key, const QString& value);
    void unsetCompileOption(const QString& key);
    void setCompileOptions(const QMap<QString, QString> options);

    QString getCompileOptionValue(const QString& key) const;

    // int mainVersion() const;
    QString findProgramInBinDirs(const QString name) const;

    bool canCompileC() const;
    bool canCompileCPP() const;
    bool canMake() const;
    bool canDebug() const;
//        bool dirsValid(QString& msg);
//        bool validateExes(QString& msg);
    //properties
    const QString& CCompiler() const;
    void setCCompiler(const QString& name);
    const QString& cppCompiler() const;
    void setCppCompiler(const QString& name);
    const QString& make() const;
    void setMake(const QString& name);
    const QString& debugger() const;
    void setDebugger(const QString& name);
    const QString& resourceCompiler() const;
    void setResourceCompiler(const QString& name);
    const QString &debugServer() const;
    void setDebugServer(const QString &newDebugServer);

    QStringList findErrors();

    QStringList& binDirs();
    QStringList& CIncludeDirs();
    QStringList& CppIncludeDirs();
    QStringList& libDirs();
    QStringList& defaultCIncludeDirs();
    QStringList& defaultCppIncludeDirs();
    QStringList& defaultLibDirs();

    const QString& dumpMachine() const;
    void setDumpMachine(const QString& value);
    const QString& version() const;
    void setVersion(const QString& value);
    const QString& name() const;
    void setName(const QString& value);
    QStringList defines(bool isCpp);
    const QString& target() const;
    void setTarget(const QString& value);

    bool useCustomCompileParams() const;
    void setUseCustomCompileParams(bool value);
    bool useCustomLinkParams() const;
    void setUseCustomLinkParams(bool value);
    const QString& customCompileParams() const;
    void setCustomCompileParams(const QString& value);
    const QString& customLinkParams() const;
    void setCustomLinkParams(const QString& value);
    bool autoAddCharsetParams() const;
    void setAutoAddCharsetParams(bool value);

    //Converts options to and from memory format ( for old settings compatibility)
    void setIniOptions(const QByteArray& value);

    bool staticLink() const;
    void setStaticLink(bool newStaticLink);


    static int charToValue(char valueChar);
    static char valueToChar(int val);
    CompilerType compilerType() const;

    void setCompilerType(CompilerType newCompilerType);

    const QString &execCharset() const;
    void setExecCharset(const QString &newExecCharset);

    const QMap<QString, QString> &compileOptions() const;

    const QString &executableSuffix() const;
    void setExecutableSuffix(const QString &newExecutableSuffix);

    const QString &preprocessingSuffix() const;
    void setPreprocessingSuffix(const QString &newPreprocessingSuffix);

    const QString &compilationProperSuffix() const;
    void setCompilationProperSuffix(const QString &newCompilationProperSuffix);

    const QString &assemblingSuffix() const;
    void setAssemblingSuffix(const QString &newAssemblingSuffix);

    QString getOutputFilename(const QString& sourceFilename);
    QString getOutputFilename(const QString& sourceFilename,CompilationStage stage);
    bool isOutputExecutable();
    bool isOutputExecutable(CompilationStage stage);

#ifdef Q_OS_WINDOWS
    bool isDebuggerUsingUTF8() const;
    bool isCompilerUsingUTF8() const;
#else
    constexpr bool isDebuggerUsingUTF8() const { return true; }
    constexpr bool isCompilerUsingUTF8() const { return true; }
#endif

    bool supportConvertingCharset();
    bool supportNLS();

    bool persistInAutoFind() const;
    void setPersistInAutoFind(bool newPersistInAutoFind);

    bool forceEnglishOutput() const;
    void setForceEnglishOutput(bool newForceEnglishOutput);

private:
    void setGCCProperties(const QString& binDir, const QString& c_prog);
    void setDirectories(const QString& binDir);
    void setGCCDirectories(const QString& binDir);
#ifdef ENABLE_SDCC
    void setSDCCProperties(const QString& binDir, const QString& c_prog);
    void setSDCCDirectories(const QString& binDir);
#endif
    //load hard defines
    void setExecutables();
    void setUserInput();


    QByteArray getCompilerOutput(const QString& binDir, const QString& binFile,
                                 const QStringList& arguments) const;
private:
    bool mFullLoaded;
    // Executables, most are hardcoded
    QString mCCompiler;
    QString mCppCompiler;
    QString mMake;
    QString mDebugger;
    QString mResourceCompiler;
    QString mDebugServer;

    // Directories, mostly hardcoded too
    QStringList mBinDirs;
    QStringList mCIncludeDirs;
    QStringList mCppIncludeDirs;
    QStringList mLibDirs;
    QStringList mDefaultLibDirs;
    QStringList mDefaultCIncludeDirs;
    QStringList mDefaultCppIncludeDirs;

    // Misc. properties
    QString mDumpMachine; // "x86_64-w64-mingw32", "x86_64-pc-linux-gnu", etc
    QString mVersion; // "14.2.0", "14" (--with-gcc-major-version-only)
    QString mName; // "MinGW-w64 GCC 14.2.0 Release"
    QString mTarget; // "x86_64", "i686", etc
    CompilerType mCompilerType;

    // User settings
    bool mUseCustomCompileParams;
    bool mUseCustomLinkParams;
    QString mCustomCompileParams;
    QString mCustomLinkParams;
    bool mAutoAddCharsetParams;
    QString mExecCharset;
    bool mStaticLink;
    bool mPersistInAutoFind;
    bool mForceEnglishOutput;

    QString mPreprocessingSuffix;
    QString mCompilationProperSuffix;
    QString mAssemblingSuffix;
    QString mExecutableSuffix;

    CompilationStage mCompilationStage;

    // Options
    QMap<QString,QString> mCompileOptions;

    bool mGccSupportNLS;
    bool mGccSupportNLSInitialized;
#ifdef Q_OS_WINDOWS
    // GCC ACP detection cache
    mutable bool mGccIsUtf8;
    mutable bool mGccIsUtf8Initialized;
    mutable bool mGDBIsUtf8;
    mutable bool mGDBIsUtf8Initialized;
    bool mGccSupportConvertingCharset;
    bool mGccSupportConvertingCharsetInitialized;
#endif
};

typedef std::shared_ptr<CompilerSet> PCompilerSet;
typedef std::vector<PCompilerSet> CompilerSetList;

class CompilerSets {
public:
    explicit CompilerSets(SettingsPersistor* settings, DirSettings *dirSettings);
    PCompilerSet addSet();
    PCompilerSet addSet(const PCompilerSet &pSet);
    bool addSets(const QString& folder);
    bool addSets(const QString& folder, const QString& c_prog);
    CompilerSetList clearSets();
    void findSets();
    void saveSets();
    void loadSets();
    void saveDefaultIndex();
    void deleteSet(int index);
    void saveSet(int index);
    size_t size() const;
    int defaultIndex() const;
    void setDefaultIndex(int value);
    qint64 defaultIndexTimestamp() const;
    PCompilerSet defaultSet();
    PCompilerSet getSet(int index);

    static QString getKeyFromCompilerCompatibleIndex(int idx);
    static bool isTarget64Bit(const QString &target);
private:
    PCompilerSet addSet(const QString& folder, const QString& c_prog);
    PCompilerSet addSet(const QJsonObject &set);
    void savePath(const QString& name, const QString& path);
    void savePathList(const QString& name, const QStringList& pathList);

    QString loadPath(const QString& name);
    void loadPathList(const QString& name, QStringList& list);
    PCompilerSet loadSet(int index);
private:
    CompilerSetList mList;
    DirSettings *mDirSettings;
    int mDefaultIndex;
    qint64 mDefaultIndexTimeStamp;
    SettingsPersistor* mPersistor;
};

#endif
//COMPILE_SET_SETTINGS_H
