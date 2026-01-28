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
#include "compilersetsettings.h"
#include "../utils.h"
#include "../settings.h"
#include "../systemconsts.h"
#include "../utils/escape.h"
#include "../utils/os.h"
#include "../utils/parsearg.h"
#include <QDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include "src/addon/luaexecutor.h"
#include "src/addon/luaruntime.h"

static QStringList CompilerCompatibleIndex; // index for old settings compatibility

static void prepareCompatibleIndex()
{

    //old settings compatibility, don't reorder, add or remove items
    CompilerCompatibleIndex.append(CC_CMD_OPT_ANSI);
    CompilerCompatibleIndex.append(CC_CMD_OPT_NO_ASM);
    CompilerCompatibleIndex.append(CC_CMD_OPT_TRADITIONAL_CPP);

    CompilerCompatibleIndex.append(CC_CMD_OPT_ARCH);
    CompilerCompatibleIndex.append(CC_CMD_OPT_TUNE);
    CompilerCompatibleIndex.append(CC_CMD_OPT_INSTRUCTION);
    CompilerCompatibleIndex.append(CC_CMD_OPT_OPTIMIZE);
    CompilerCompatibleIndex.append(CC_CMD_OPT_POINTER_SIZE);
    CompilerCompatibleIndex.append(CC_CMD_OPT_STD);

    CompilerCompatibleIndex.append(CC_CMD_OPT_INHIBIT_ALL_WARNING);
    CompilerCompatibleIndex.append(CC_CMD_OPT_WARNING_ALL);
    CompilerCompatibleIndex.append(CC_CMD_OPT_WARNING_EXTRA);
    CompilerCompatibleIndex.append(CC_CMD_OPT_CHECK_ISO_CONFORMANCE);
    CompilerCompatibleIndex.append(CC_CMD_OPT_SYNTAX_ONLY);
    CompilerCompatibleIndex.append(CC_CMD_OPT_WARNING_AS_ERROR);
    CompilerCompatibleIndex.append(CC_CMD_OPT_ABORT_ON_ERROR);
    CompilerCompatibleIndex.append(CC_CMD_OPT_ENABLE_GCC_IMPORT_STD);

    CompilerCompatibleIndex.append(CC_CMD_OPT_PROFILE_INFO);

    CompilerCompatibleIndex.append(LINK_CMD_OPT_LINK_OBJC);
    CompilerCompatibleIndex.append(LINK_CMD_OPT_NO_LINK_STDLIB);
    CompilerCompatibleIndex.append(LINK_CMD_OPT_NO_CONSOLE);
    CompilerCompatibleIndex.append(LINK_CMD_OPT_STRIP_EXE);
    CompilerCompatibleIndex.append(CC_CMD_OPT_DEBUG_INFO);

    CompilerCompatibleIndex.append(CC_CMD_OPT_VERBOSE_ASM);
    CompilerCompatibleIndex.append(CC_CMD_OPT_ONLY_GEN_ASM_CODE);
    CompilerCompatibleIndex.append(CC_CMD_OPT_USE_PIPE);
}

QString CompilerSets::getKeyFromCompilerCompatibleIndex(int idx)
{
    if (CompilerCompatibleIndex.isEmpty())
        prepareCompatibleIndex();
    if (idx<0 || idx >= CompilerCompatibleIndex.length())
        return QString();
    return CompilerCompatibleIndex[idx];
}


CompilerSet::CompilerSet():
    mFullLoaded{false},
    mCompilerType{CompilerType::Unknown},
    mAutoAddCharsetParams{false},
    mExecCharset{ENCODING_SYSTEM_DEFAULT},
    mStaticLink{false},
    mPersistInAutoFind{false},
    mForceEnglishOutput{false},
    mPreprocessingSuffix{DEFAULT_PREPROCESSING_SUFFIX},
    mCompilationProperSuffix{DEFAULT_COMPILATION_SUFFIX},
    mAssemblingSuffix{DEFAULT_ASSEMBLING_SUFFIX},
    mExecutableSuffix{DEFAULT_EXECUTABLE_SUFFIX},
    mGccSupportNLS{false},
    mGccSupportNLSInitialized{false}
#ifdef Q_OS_WINDOWS
    , mGccIsUtf8Initialized{false}
    , mGDBIsUtf8Initialized{false}
    , mGccSupportConvertingCharsetInitialized{false}
#endif
{

}


CompilerSet::CompilerSet(const QString& compilerFolder, const QString& c_prog):
    mAutoAddCharsetParams{true},
    mExecCharset{ENCODING_SYSTEM_DEFAULT},
    mStaticLink{true},
    mPersistInAutoFind{false},
    mForceEnglishOutput{false},
    mPreprocessingSuffix{DEFAULT_PREPROCESSING_SUFFIX},
    mCompilationProperSuffix{DEFAULT_COMPILATION_SUFFIX},
    mAssemblingSuffix{DEFAULT_ASSEMBLING_SUFFIX},
    mExecutableSuffix{DEFAULT_EXECUTABLE_SUFFIX},
    mGccSupportNLS{false},
    mGccSupportNLSInitialized{false}
#ifdef Q_OS_WINDOWS
    , mGccIsUtf8Initialized(false)
    , mGDBIsUtf8Initialized{false}
    , mGccSupportConvertingCharsetInitialized(false)
#endif
{
    QDir dir(compilerFolder);
    if (dir.exists(c_prog)) {

        setProperties(compilerFolder,c_prog);

        if (mName.isEmpty()) {
            mFullLoaded = false;
            return;
        }

        //manually set the directories
        setDirectories(compilerFolder);

        setExecutables();

        setUserInput();

#ifdef ENABLE_SDCC
        if (mCompilerType == CompilerType::SDCC) {
            mExecutableSuffix = SDCC_HEX_SUFFIX;
        }
#endif
        mFullLoaded = true;
    } else {
        mFullLoaded = false;
    }
}

CompilerSet::CompilerSet(const CompilerSet &set):
    mFullLoaded{set.mFullLoaded},
    mCCompiler{set.mCCompiler},
    mCppCompiler{set.mCppCompiler},
    mMake{set.mMake},
    mDebugger{set.mDebugger},
    mResourceCompiler{set.mResourceCompiler},
    mDebugServer{set.mDebugServer},

    mBinDirs{set.mBinDirs},
    mCIncludeDirs{set.mCIncludeDirs},
    mCppIncludeDirs{set.mCppIncludeDirs},
    mLibDirs{set.mLibDirs},
    mDefaultLibDirs{set.mDefaultLibDirs},
    mDefaultCIncludeDirs{set.mDefaultCIncludeDirs},
    mDefaultCppIncludeDirs{set.mDefaultCppIncludeDirs},

    mDumpMachine{set.mDumpMachine},
    mVersion{set.mVersion},
    mName{set.mName},
    mTarget{set.mTarget},
    mCompilerType{set.mCompilerType},


    mUseCustomCompileParams{set.mUseCustomCompileParams},
    mUseCustomLinkParams{set.mUseCustomLinkParams},
    mCustomCompileParams{set.mCustomCompileParams},
    mCustomLinkParams{set.mCustomLinkParams},
    mAutoAddCharsetParams{set.mAutoAddCharsetParams},
    mExecCharset{set.mExecCharset},
    mStaticLink{set.mStaticLink},
    mPersistInAutoFind{set.mPersistInAutoFind},
    mForceEnglishOutput{set.mForceEnglishOutput},

    mPreprocessingSuffix{set.mPreprocessingSuffix},
    mCompilationProperSuffix{set.mCompilationProperSuffix},
    mAssemblingSuffix{set.mAssemblingSuffix},
    mExecutableSuffix{set.mExecutableSuffix},
    mCompileOptions{set.mCompileOptions},
    mGccSupportNLS{set.mGccSupportNLS},
    mGccSupportNLSInitialized{set.mGccSupportNLSInitialized}
#ifdef Q_OS_WINDOWS
    , mGccIsUtf8(set.mGccIsUtf8)
    , mGccIsUtf8Initialized(set.mGccIsUtf8Initialized)
    , mGDBIsUtf8(set.mGDBIsUtf8)
    , mGDBIsUtf8Initialized(set.mGDBIsUtf8Initialized)
    , mGccSupportConvertingCharset(set.mGccSupportConvertingCharset)
    , mGccSupportConvertingCharsetInitialized(set.mGccSupportConvertingCharsetInitialized)
#endif
{

}

CompilerSet::CompilerSet(const QJsonObject &set) :
    mFullLoaded{true},
    mCCompiler{set["cCompiler"].toString()},
    mCppCompiler{set["cxxCompiler"].toString()},
    mMake{set["make"].toString()},
    mDebugger{set["debugger"].toString()},
    mResourceCompiler{set["resourceCompiler"].toString()},
    mDebugServer{set["debugServer"].toString()},

    mBinDirs{},               // handle later
    mCIncludeDirs{},          // handle later
    mCppIncludeDirs{},        // handle later
    mLibDirs{},               // handle later
    mDefaultLibDirs{},        // handle later
    mDefaultCIncludeDirs{},   // handle later
    mDefaultCppIncludeDirs{}, // handle later

    mDumpMachine{set["dumpMachine"].toString()},
    mVersion{set["version"].toString()},
    mName{set["name"].toString()},
    mTarget{set["target"].toString()},
    mCompilerType{}, // handle later

    mUseCustomCompileParams{!set["customCompileParams"].toArray().isEmpty()},
    mUseCustomLinkParams{!set["customLinkParams"].toArray().isEmpty()},
    mCustomCompileParams{}, // handle later
    mCustomLinkParams{},    // handle later
    mAutoAddCharsetParams{!set["execCharset"].toString().isEmpty()},
    mExecCharset{}, // handle later
    mStaticLink{set["staticLink"].toBool()},
    mPersistInAutoFind{false},
    mForceEnglishOutput{false},

    mPreprocessingSuffix{set["preprocessingSuffix"].toString()},
    mCompilationProperSuffix{set["compilationProperSuffix"].toString()},
    mAssemblingSuffix{set["assemblingSuffix"].toString()},
    mExecutableSuffix{set["executableSuffix"].toString()},
    mCompileOptions{}, // handle later
    mGccSupportNLS{false},
    mGccSupportNLSInitialized{false}
#ifdef Q_OS_WINDOWS
    , mGccIsUtf8Initialized(false)
    , mGDBIsUtf8Initialized(false)
    , mGccSupportConvertingCharsetInitialized(false)
#endif
{
    foreach (const QJsonValue &dir, set["binDirs"].toArray())
        mBinDirs.append(dir.toString());
    foreach (const QJsonValue &dir, set["cIncludeDirs"].toArray())
        mCIncludeDirs.append(dir.toString());
    foreach (const QJsonValue &dir, set["cxxIncludeDirs"].toArray())
        mCppIncludeDirs.append(dir.toString());
    foreach (const QJsonValue &dir, set["libDirs"].toArray())
        mLibDirs.append(dir.toString());
    foreach (const QJsonValue &dir, set["defaultLibDirs"].toArray())
        mDefaultLibDirs.append(dir.toString());
    foreach (const QJsonValue &dir, set["defaultCIncludeDirs"].toArray())
        mDefaultCIncludeDirs.append(dir.toString());
    foreach (const QJsonValue &dir, set["defaultCxxIncludeDirs"].toArray())
        mDefaultCppIncludeDirs.append(dir.toString());

    QString compilerType = set["compilerType"].toString();
    if (compilerType == "GCC") {
        mCompilerType = CompilerType::GCC;
    } else if (compilerType == "GCC_UTF8") {
        mCompilerType = CompilerType::GCC;
    } else if (compilerType == "Clang") {
        mCompilerType = CompilerType::Clang;
    }
#if ENABLE_SDCC
    else if (compilerType == "SDCC") {
        mCompilerType = CompilerType::SDCC;
    }
#endif
    else {
        mCompilerType = CompilerType::Unknown;
        mFullLoaded = false;
    }

    QStringList compileParams;
    foreach (const QJsonValue &param, set["customCompileParams"].toArray())
        compileParams << param.toString();
    mCustomCompileParams = escapeArgumentsForInputField(compileParams);
    QStringList linkParams;
    foreach (const QJsonValue &param, set["customLinkParams"].toArray())
        linkParams << param.toString();
    mCustomLinkParams = escapeArgumentsForInputField(linkParams);

    if (!mAutoAddCharsetParams)
        mExecCharset = "UTF-8";
    else
        mExecCharset = set["execCharset"].toString();

    const static QMap<QString, QString> optionMap = {
                                                     {CC_CMD_OPT_OPTIMIZE, "ccCmdOptOptimize"},
                                                     {CC_CMD_OPT_STD, "ccCmdOptStd"},
                                                     {C_CMD_OPT_STD, "cCmdOptStd"},
                                                     {CC_CMD_OPT_INSTRUCTION, "ccCmdOptInstruction"},

                                                     {CC_CMD_OPT_POINTER_SIZE, "ccCmdOptPointerSize"},

                                                     {CC_CMD_OPT_DEBUG_INFO, "ccCmdOptDebugInfo"},
                                                     {CC_CMD_OPT_PROFILE_INFO, "ccCmdOptProfileInfo"},
                                                     {CC_CMD_OPT_SYNTAX_ONLY, "ccCmdOptSyntaxOnly"},
                                                     {CC_CMD_OPT_ENABLE_GCC_IMPORT_STD, "ccCmdOptEnableGccImportStd"},

                                                     {CC_CMD_OPT_INHIBIT_ALL_WARNING, "ccCmdOptInhibitAllWarning"},
                                                     {CC_CMD_OPT_WARNING_ALL, "ccCmdOptWarningAll"},
                                                     {CC_CMD_OPT_WARNING_EXTRA, "ccCmdOptWarningExtra"},
                                                     {CC_CMD_OPT_CHECK_ISO_CONFORMANCE, "ccCmdOptCheckIsoConformance"},
                                                     {CC_CMD_OPT_NO_MS_EXTENSIONS, "ccCmdOptNoMSExtensions"},
                                                     {CC_CMD_OPT_WARNING_AS_ERROR, "ccCmdOptWarningAsError"},
                                                     {CC_CMD_OPT_ABORT_ON_ERROR, "ccCmdOptAbortOnError"},
                                                     {CC_CMD_OPT_STACK_PROTECTOR, "ccCmdOptStackProtector"},
                                                     {CC_CMD_OPT_ADDRESS_SANITIZER, "ccCmdOptAddressSanitizer"},

                                                     {CC_CMD_OPT_ERROR_RETURN_TYPE, "ccCmdOptErrorReturnType"},
                                                     {CC_CMD_OPT_ERROR_VLA, "ccCmdOptErrorVLA"},
                                                     {CC_CMD_OPT_ERROR_IMPLICIT_INT, "ccCmdOptErrorImplicitInt"},

                                                     {CC_CMD_OPT_USE_PIPE, "ccCmdOptUsePipe"},
                                                     {LINK_CMD_OPT_NO_LINK_STDLIB, "linkCmdOptNoLinkStdlib"},
                                                     {LINK_CMD_OPT_NO_CONSOLE, "linkCmdOptNoConsole"},
                                                     {LINK_CMD_OPT_STRIP_EXE, "linkCmdOptStripExe"},
                                                     };
    foreach (const QString &key, optionMap.keys()) {
        const QString &jsonKey = optionMap[key];
        QString value = set[jsonKey].toString();
        if (!value.isEmpty())
            setCompileOption(key, value);
    }
}

void CompilerSet::resetCompileOptionts()
{
      mCompileOptions.clear();
}

bool CompilerSet::setCompileOption(const QString &key, int valIndex)
{
    PCompilerOption op = CompilerInfoManager::getCompilerOption(mCompilerType, key);
    if (!op)
        return false;
    if (op->choices.isEmpty()) {
        if (valIndex==1)
            mCompileOptions.insert(key,COMPILER_OPTION_ON);
        else
            mCompileOptions.remove(key);
        return true;
    } else if (valIndex>0 && valIndex <= op->choices.length()) {
        mCompileOptions.insert(key,op->choices[valIndex-1].second);
        return true;
    } else {
        mCompileOptions.remove(key);
        return true;
    }
    return false;
}

bool CompilerSet::setCompileOption(const QString &key, const QString &value)
{
    PCompilerOption op = CompilerInfoManager::getCompilerOption(mCompilerType,key);
    if (!op)
        return false;
    mCompileOptions.insert(key,value);
    return true;
}

void CompilerSet::unsetCompileOption(const QString &key)
{
    mCompileOptions.remove(key);
}

void CompilerSet::setCompileOptions(const QMap<QString, QString> options)
{
    mCompileOptions=options;
}

QString CompilerSet::getCompileOptionValue(const QString &key) const
{
    return mCompileOptions.value(key,QString());
}

//static void checkDirs(const QStringList& dirlist, QString& gooddirs, QString& baddirs) {
//    gooddirs = "";
//    baddirs = "";

//    for (int i=0; i<dirlist.count();i++) {
//        QDir dir(dirlist[i]);
//        if (!dir.exists()) {
//            if (baddirs.isEmpty()) {
//                baddirs = dirlist[i];
//            } else {
//                baddirs += ";" + dirlist[i];
//            }
//        } else {
//            if (gooddirs.isEmpty()) {
//                gooddirs = dirlist[i];
//            } else {
//                gooddirs += ";" + dirlist[i];
//            }
//        }
//    }
//}


//bool CompilerSet::dirsValid(QString &msg)
//{
//    QString goodbin, badbin, goodlib, badlib, goodinc, badinc, goodinccpp, badinccpp;
//    msg = "";

//    if (mBinDirs.count()>0) {// we need some bin dir, so treat count=0 as an error too
//        checkDirs(mBinDirs,goodbin,badbin);
//        if (!badbin.isEmpty()) {
//            msg += QObject::tr("The following %1 directories don't exist:").arg(
//                        QObject::tr("binary")
//                        );
//            msg += "<br />";
//            msg += badbin.replace(';',"<br />");
//            msg += "<br />";
//            msg += "<br />";
//            return false;
//        }
//    } else {
//        msg += QObject::tr("No %1 directories have been specified.").arg(
//                    QObject::tr("binary")
//                    );
//        msg += "<br />";
//        msg += "<br />";
//        return false;
//    }
//    checkDirs(mCIncludeDirs,goodbin,badbin);
//    if (!badbin.isEmpty()) {
//        msg += QObject::tr("The following %1 directories don't exist:").arg(
//                    QObject::tr("C include")
//                    );
//        msg += "<br />";
//        msg += badbin.replace(';',"<br />");
//        msg += "<br />";
//        msg += "<br />";
//        return false;
//    }

//    checkDirs(mCppIncludeDirs,goodbin,badbin);
//    if (!badbin.isEmpty()) {
//        msg += QObject::tr("The following %1 directories don't exist:").arg(
//                    QObject::tr("C++ include")
//                    );
//        msg += "<br />";
//        msg += badbin.replace(';',"<br />");
//        msg += "<br />";
//        msg += "<br />";
//        return false;
//    }

//    checkDirs(mLibDirs,goodbin,badbin);
//    if (!badbin.isEmpty()) {
//        msg += QObject::tr("The following %1 directories don't exist:").arg(
//                    QObject::tr("C++ include")
//                    );
//        msg += "<br />";
//        msg += badbin.replace(';',"<br />");
//        msg += "<br />";
//        msg += "<br />";
//        return false;
//    }

//    if (!msg.isEmpty())
//        return false;
//    else
//        return true;
//}

//bool CompilerSet::validateExes(QString &msg)
//{
//    msg ="";
//    if (!fileExists(mCCompiler)) {
//        msg += QObject::tr("Cannot find the %1 \"%2\"")
//                .arg(QObject::tr("C Compiler"))
//                .arg(mCCompiler);
//    }
//    if (!fileExists(mCppCompiler)) {
//        msg += QObject::tr("Cannot find the %1 \"%2\"")
//                .arg(QObject::tr("C++ Compiler"))
//                .arg(mCppCompiler);
//    }
//    if (!mMake.isEmpty() && !fileExists(mMake)) {
//        msg += QObject::tr("Cannot find the %1 \"%2\"")
//                .arg(QObject::tr("Maker"))
//                .arg(mMake);
//    }
//    if (!fileExists(mDebugger)) {
//        msg += QObject::tr("Cannot find the %1 \"%2\"")
//                .arg(QObject::tr("Debugger"))
//                .arg(mDebugger);
//    }
//    if (!msg.isEmpty())
//        return false;
//    else
//        return true;
//}

const QString &CompilerSet::CCompiler() const
{
    return mCCompiler;
}

void CompilerSet::setCCompiler(const QString &name)
{
    if (mCCompiler!=name) {
#ifdef Q_OS_WIN
        mGccIsUtf8Initialized = false;
#endif
        mCCompiler = name;
        if (mCompilerType == CompilerType::Unknown) {
            QString temp=extractFileName(mCCompiler);
            if (temp == CLANG_PROGRAM) {
                setCompilerType(CompilerType::Clang);
            } else if (temp == GCC_PROGRAM) {
                setCompilerType(CompilerType::GCC);
            }
        }
    }
}

const QString &CompilerSet::cppCompiler() const
{
    return mCppCompiler;
}

void CompilerSet::setCppCompiler(const QString &name)
{
    mCppCompiler = name;
}

const QString &CompilerSet::make() const
{
    return mMake;
}

void CompilerSet::setMake(const QString &name)
{
    mMake = name;
}

const QString &CompilerSet::debugger() const
{
    return mDebugger;
}

void CompilerSet::setDebugger(const QString &name)
{
    if (mDebugger!=name) {
#ifdef Q_OS_WIN
        mGDBIsUtf8Initialized = false;
#endif
        mDebugger = name;
    }
}

const QString &CompilerSet::resourceCompiler() const
{
    return mResourceCompiler;
}

void CompilerSet::setResourceCompiler(const QString &name)
{
    mResourceCompiler = name;
}

QStringList &CompilerSet::binDirs()
{
    return mBinDirs;
}

QStringList &CompilerSet::CIncludeDirs()
{
    return mCIncludeDirs;
}

QStringList &CompilerSet::CppIncludeDirs()
{
    return mCppIncludeDirs;
}

QStringList &CompilerSet::libDirs()
{
    return mLibDirs;
}

QStringList &CompilerSet::defaultCIncludeDirs()
{
    if (!mFullLoaded && !binDirs().isEmpty()) {
        mFullLoaded=true;
        setDirectories(binDirs()[0]);
    }
    return mDefaultCIncludeDirs;
}

QStringList &CompilerSet::defaultCppIncludeDirs()
{
    if (!mFullLoaded && !binDirs().isEmpty()) {
        mFullLoaded=true;
        setDirectories(binDirs()[0]);
    }
    return mDefaultCppIncludeDirs;
}

QStringList &CompilerSet::defaultLibDirs()
{
    if (!mFullLoaded && !binDirs().isEmpty()) {
        mFullLoaded=true;
        setDirectories(binDirs()[0]);
    }
    return mLibDirs;
}

const QString &CompilerSet::dumpMachine() const
{
    return mDumpMachine;
}

void CompilerSet::setDumpMachine(const QString &value)
{
    mDumpMachine = value;
}

const QString &CompilerSet::version() const
{
    return mVersion;
}

void CompilerSet::setVersion(const QString &value)
{
    mVersion = value;
}

const QString &CompilerSet::name() const
{
    return mName;
}

void CompilerSet::setName(const QString &value)
{
    mName = value;
}

const QString &CompilerSet::target() const
{
    return mTarget;
}

void CompilerSet::setTarget(const QString &value)
{
    mTarget = value;
}

void CompilerSet::setUseCustomCompileParams(bool value)
{
    mUseCustomCompileParams = value;
}

bool CompilerSet::useCustomLinkParams() const
{
    return mUseCustomLinkParams;
}

void CompilerSet::setUseCustomLinkParams(bool value)
{
    mUseCustomLinkParams = value;
}

const QString &CompilerSet::customCompileParams() const
{
    return mCustomCompileParams;
}

void CompilerSet::setCustomCompileParams(const QString &value)
{
    mCustomCompileParams = value;
}

const QString &CompilerSet::customLinkParams() const
{
    return mCustomLinkParams;
}

void CompilerSet::setCustomLinkParams(const QString &value)
{
    mCustomLinkParams = value;
}

bool CompilerSet::autoAddCharsetParams() const
{
    return mAutoAddCharsetParams;
}

void CompilerSet::setAutoAddCharsetParams(bool value)
{
    mAutoAddCharsetParams = value;
}

int CompilerSet::charToValue(char valueChar)
{
    if (valueChar == '1') {
        return 1;
    } else if ( (valueChar>='a') && (valueChar<='z')) {
        return (valueChar-'a')+2;
    } else {
        return 0;
    }
}

char CompilerSet::valueToChar(int val)
{
    static const char ValueToChar[28] = {'0', '1', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                                  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                                  's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};
    return ValueToChar[val];
}

static void addExistingDirectory(QStringList& dirs, const QString& directory) {
    if (!directoryExists(directory))
        return;
    QFileInfo dirInfo(directory);
    QString dirPath = dirInfo.absoluteFilePath();
    if (dirs.contains(dirPath))
        return;
    dirs.append(dirPath);
}

static void addExistingDirectory(QStringList& dirs, const QByteArray& directory) {
    QString dir = QString::fromLocal8Bit(directory);
    if (!directoryExists(dir))
        dir = QString::fromUtf8(directory);
    addExistingDirectory(dirs,dir);
}

void CompilerSet::setProperties(const QString& binDir, const QString& c_prog)
{
#ifdef ENABLE_SDCC
    if (c_prog == SDCC_PROGRAM) {
        setSDCCProperties(binDir,c_prog);
    } else {
#endif

        setGCCProperties(binDir,c_prog);
#ifdef ENABLE_SDCC
    }
#endif
}

void CompilerSet::setGCCProperties(const QString& binDir, const QString& c_prog)
{
    // We have tested before the call
//    if (!fileExists(c_prog))
//        return;

    // Obtain basic info
    QByteArray dumpMachine = getCompilerOutput(binDir, c_prog, {"-dumpmachine"});
    mDumpMachine = QString(dumpMachine).trimmed();
    if (mDumpMachine.isEmpty())
        // unknown binary
        return;
    if (mDumpMachine == "mingw32") {
        // MinGW.org uses bare `mingw32` “triplet”, not conforming to GCC's document.
        // here we change it to MinGW-w64’s compatibility mode triplet.
        // ref 1. https://gcc.gnu.org/install/specific.html
        // ref 2. https://sourceforge.net/p/mingw-w64/wiki2/Feature%20list/
        mDumpMachine = "i386-pc-mingw32";
    }
    mTarget = mDumpMachine.mid(0, mDumpMachine.indexOf('-'));
    QByteArray version = getCompilerOutput(binDir, c_prog, {"-dumpfullversion"}).trimmed();
    if (version.isEmpty())
        version = getCompilerOutput(binDir, c_prog, {"-dumpversion"}).trimmed();
    // QRegularExpression versionPattern = QRegularExpression("^[\\d]+\\.[\\d]+.*$");
    // if (auto m = versionPattern.match(version); !m.hasMatch()) {
    //     version = getCompilerOutput(binDir, c_prog, {"-dumpfullversion"}).trimmed();
    // }
    mVersion = QString(version).trimmed();

    // Obtain compiler distro
    bool outputFormatIsPe = false;
    QByteArray verboseOut = getCompilerOutput(binDir, c_prog, {"-v"});
    QByteArray targetStr = "clang version ";
    int clangVersionPos = verboseOut.indexOf(targetStr);
    if (clangVersionPos >= 0) {
        mCompilerType = CompilerType::Clang;
        QRegularExpression ntPosixPattern = QRegularExpression("^(.*)-pc-windows-msys$");
        QRegularExpression mingwW64Pattern = QRegularExpression("^(.*)-w64-windows-gnu$");
        if (auto m = ntPosixPattern.match(mDumpMachine); m.hasMatch()) {
            outputFormatIsPe = true;
            if (mName.isEmpty())
                mName = "MSYS2 Clang " + mVersion;
        } else if (m = mingwW64Pattern.match(mDumpMachine); m.hasMatch()) {
            outputFormatIsPe = true;
            if (mName.isEmpty())
                mName = "MinGW-w64 Clang " + mVersion;
        } else {
            if (mName.isEmpty())
                mName = "Clang " + mVersion;
        }
    } else {
        mCompilerType = CompilerType::GCC;
        QRegularExpression ntPosixPattern = QRegularExpression("^(.*)-(.*)-(msys|cygwin)$");
        QRegularExpression mingwW64Pattern = QRegularExpression("^(.*)-w64-mingw32$");
        QRegularExpression mingwOrgPattern("^(.*)-(.*)-mingw32$");
        if (auto m = ntPosixPattern.match(mDumpMachine); m.hasMatch()) {
            outputFormatIsPe = true;
            if (mName.isEmpty()) {
                if (m.captured(3) == "msys")
                    mName = "MSYS2 GCC " + mVersion;
                else
                    mName = "Cygwin GCC " + mVersion;
            }
        } else if (m = mingwW64Pattern.match(mDumpMachine); m.hasMatch()) {
            outputFormatIsPe = true;
            if (mName.isEmpty())
                mName = "MinGW-w64 GCC " + mVersion;
        } else if (m = mingwOrgPattern.match(mDumpMachine); m.hasMatch()) {
            outputFormatIsPe = true;
            if (mName.isEmpty())
                mName = "MinGW.org GCC " + mVersion;
        } else {
            if (mName.isEmpty())
                mName = "GCC " + mVersion;
        }
    }
    if (outputFormatIsPe)
        setCompileOption(LINK_CMD_OPT_STACK_SIZE, "12");

    // Set compiler folder
    QDir tmpDir(binDir);
    tmpDir.cdUp();
    QString folder = tmpDir.path();

    // Add the default directories
    addExistingDirectory(mBinDirs, getFilePath(folder,"bin"));
    // if (!mDumpMachine.isEmpty()) {
    //     //mingw-w64 bin folder
    //     addExistingDirectory(mBinDirs,
    //                         generateSubfolderPath(
    //                             folder,
    //                                 {"lib" , "gcc" , mDumpMachine, mVersion} ));
    // }
}

#ifdef ENABLE_SDCC
void CompilerSet::setSDCCProperties(const QString& binDir, const QString& c_prog)
{
    // We have tested before the call
//    if (!fileExists(c_prog))
//        return;
    // Obtain version number and compiler distro etc
    QStringList arguments;
    arguments.append("-v");
    QByteArray output = getCompilerOutput(binDir, c_prog,arguments);

    if (!output.startsWith("SDCC"))
        return;

    //Target
    int delimPos = 0;
    while (delimPos<output.length() && (output[delimPos]>=((char)32)))
        delimPos++;
    QString triplet = output.mid(0,delimPos);

    QRegularExpression re("\\s+(\\d+\\.\\d+\\.\\d+)\\s+");
    QRegularExpressionMatch match = re.match(triplet);
    if (match.hasMatch())
        mVersion = match.captured(1);
    if (mVersion.isEmpty())
        mName = "SDCC";
    else
        mName = "SDCC "+mVersion;
    mCompilerType=CompilerType::SDCC;

    addExistingDirectory(mBinDirs, binDir);
}
#endif

QStringList CompilerSet::x86MultilibList(const QString &folder, const QString &c_prog) const
{
    QByteArray multilibOutput = getCompilerOutput(folder, c_prog, {"-print-multi-lib"});
    QStringList result;
    foreach (const QByteArray& rawLine, multilibOutput.split('\n')) {
        // man gcc:
        //   -print-multi-lib
        //     Print the mapping from multilib directory names to compiler switches that enable them.
        //     The directory name is separated from the switches by `;`, and each switch starts with
        //     an `@` instead of the `-`, without spaces between multiple switches.

        // Example 1 (GCC):
        //   .;
        //   32;@m32

        // Example 2 (GCC Debian):
        //   .;
        //   32;@m32
        //   x32;@mx32

        // Example 3 (Clang):
        //   .;@m64
        //   32;@m32

        QString line = QString(rawLine).trimmed();
        int sep = line.indexOf(';');
        if (sep < 0)
            continue;
        QString dir = line.left(sep);
        if (dir == ".")
            // native ABI
            continue;
        QString switches = line.mid(sep+1);
        if (switches == "@m32")
            result.append("32");
        else if (switches == "@mx32")
            result.append("x32");
        else if (switches == "@m64")
            // possible for Debian x32 port
            result.append("64");
    }
    return result;
}

QStringList CompilerSet::defines(bool isCpp) {
    // get default defines
    QStringList arguments;
    arguments.append("-dM");
    arguments.append("-E");
    arguments.append("-x");
    QString key;
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC) {
        arguments.append("c");
        arguments.append("-V");
        key=SDCC_CMD_OPT_PROCESSOR;
        //language standard
        PCompilerOption pOption = CompilerInfoManager::getCompilerOption(compilerType(), key);
        if (pOption) {
            if (!mCompileOptions[key].isEmpty())
                arguments.append(pOption->setting + mCompileOptions[key]);
        }
        key=SDCC_CMD_OPT_STD;
        //language standard
        pOption = CompilerInfoManager::getCompilerOption(compilerType(), key);
        if (pOption) {
            if (!mCompileOptions[key].isEmpty())
                arguments.append(pOption->setting + mCompileOptions[key]);
        }
    } else {
#endif
        if (isCpp) {
            arguments.append("c++");
            key=CC_CMD_OPT_STD;
        } else {
            arguments.append("c");
            key=C_CMD_OPT_STD;
        }
        //language standard
        PCompilerOption pOption = CompilerInfoManager::getCompilerOption(compilerType(), key);
        if (pOption) {
            if (!mCompileOptions[key].isEmpty())
                arguments.append(pOption->setting + mCompileOptions[key]);
        }
        pOption = CompilerInfoManager::getCompilerOption(compilerType(), CC_CMD_OPT_ENABLE_GCC_IMPORT_STD);
        if (pOption && mCompileOptions.contains(CC_CMD_OPT_ENABLE_GCC_IMPORT_STD)) {
            arguments.append(pOption->setting);
        }
        pOption = CompilerInfoManager::getCompilerOption(compilerType(), CC_CMD_OPT_DEBUG_INFO);
        if (pOption && mCompileOptions.contains(CC_CMD_OPT_DEBUG_INFO)) {
            arguments.append(pOption->setting);
        }
#ifdef ENABLE_SDCC
    }
#endif

    if (mUseCustomCompileParams) {
        QStringList extraParams = parseArgumentsWithoutVariables(mCustomCompileParams);
        arguments.append(extraParams);
    }
    if (arguments.contains("-g3"))
        arguments.append("-D_DEBUG");
    arguments.append(NULL_FILE);

    QFileInfo ccompiler(mCCompiler);
    QByteArray output = getCompilerOutput(ccompiler.absolutePath(),ccompiler.fileName(),arguments);
    // 'cpp.exe -dM -E -x c++ -std=c++17 NUL'
//    qDebug()<<"------------------";
    QStringList result;
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC) {
        QList<QByteArray> lines = output.split('\n');
        QByteArray currentLine;
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("+")) {
                currentLine = line;
                break;
            }
        }
        lines = currentLine.split(' ');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("-D")) {
                trimmedLine = trimmedLine.mid(2);
                if (trimmedLine.contains("=")) {
                    QList<QByteArray> items=trimmedLine.split('=');
                    result.append(QString("#define %1 %2").arg(QString(items[0]),QString(items[1])));
                } else {
                    result.append("#define "+trimmedLine);
                }
            }
        }
    } else {
#else
    {
#endif
        QList<QByteArray> lines = output.split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                result.append(trimmedLine);
            }
        }
    }
    return result;
}

void CompilerSet::setExecutables()
{
    if (mCompilerType == CompilerType::Clang) {
        mCCompiler =  findProgramInBinDirs(CLANG_PROGRAM);
        mCppCompiler = findProgramInBinDirs(CLANG_CPP_PROGRAM);
        mDebugger = findProgramInBinDirs(GDB_PROGRAM);
        if (mDebugger.isEmpty()) {
            mDebugger = findProgramInBinDirs(LLDB_MI_PROGRAM);
            mDebugServer = findProgramInBinDirs(LLDB_SERVER_PROGRAM);
        } else {
            mDebugServer = findProgramInBinDirs(GDB_SERVER_PROGRAM);
        }
        if (mCCompiler.isEmpty())
            mCCompiler =  findProgramInBinDirs(GCC_PROGRAM);
        if (mCppCompiler.isEmpty())
            mCppCompiler = findProgramInBinDirs(GPP_PROGRAM);
#ifdef ENABLE_SDCC
    } else if (mCompilerType == CompilerType::SDCC) {
        mCCompiler =  findProgramInBinDirs(SDCC_PROGRAM);
        if (mCCompiler.isEmpty())
            mCCompiler =  findProgramInBinDirs(SDCC_PROGRAM);
#endif
    } else {
        mCCompiler =  findProgramInBinDirs(GCC_PROGRAM);
        mCppCompiler = findProgramInBinDirs(GPP_PROGRAM);
        mDebugger = findProgramInBinDirs(GDB_PROGRAM);
        mDebugServer = findProgramInBinDirs(GDB_SERVER_PROGRAM);
    }
    mMake = findProgramInBinDirs(MAKE_PROGRAM);
#ifdef Q_OS_WIN
    mResourceCompiler = findProgramInBinDirs(WINDRES_PROGRAM);
#endif
}

void CompilerSet::setDirectories(const QString& binDir)
{
#ifdef ENABLE_SDCC
    if (mCompilerType == CompilerType::SDCC) {
        setSDCCDirectories(binDir);
    } else {
#endif
        setGCCDirectories(binDir);
#ifdef ENABLE_SDCC
    }
#endif
}

void CompilerSet::setGCCDirectories(const QString& binDir)
{
    QString c_prog;
    if (mCompilerType==CompilerType::Clang)
        c_prog = CLANG_PROGRAM;
    else
        c_prog = GCC_PROGRAM;
    // Find default directories
    // C include dirs
    QStringList arguments;
    arguments.clear();
    arguments.append("-xc");
    arguments.append("-v");
    arguments.append("-E");
    arguments.append(NULL_FILE);
    QByteArray output = getCompilerOutput(binDir,c_prog,arguments);

    int delimPos1 = output.indexOf("#include <...> search starts here:");
    int delimPos2 = output.indexOf("End of search list.");
    if (delimPos1 >0 && delimPos2>0 ) {
        delimPos1 += QByteArray("#include <...> search starts here:").length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mDefaultCIncludeDirs,trimmedLine);
            }
        }
    }

    // Find default directories
    // C++ include dirs
    arguments.clear();
    arguments.append("-xc++");
    arguments.append("-E");
    arguments.append("-v");
    arguments.append(NULL_FILE);
    output = getCompilerOutput(binDir,c_prog,arguments);
    //gcc -xc++ -E -v NUL

    delimPos1 = output.indexOf("#include <...> search starts here:");
    delimPos2 = output.indexOf("End of search list.");
    if (delimPos1 >0 && delimPos2>0 ) {
        delimPos1 += QByteArray("#include <...> search starts here:").length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mDefaultCppIncludeDirs,trimmedLine);
            }
        }
    }

    // Find default directories
    arguments.clear();
    arguments.append("-print-search-dirs");
    arguments.append(NULL_FILE);
    output = getCompilerOutput(binDir,c_prog,arguments);
    // bin dirs
    QByteArray targetStr = QByteArray("programs: =");
    delimPos1 = output.indexOf(targetStr);
    if (delimPos1>=0) {
        delimPos1+=targetStr.length();
        delimPos2 = delimPos1;
        while (delimPos2 < output.length() && output[delimPos2]!='\n')
            delimPos2+=1;
        QList<QByteArray> lines = output.mid(delimPos1,delimPos2-delimPos1).split(';');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty())
                addExistingDirectory(mBinDirs,trimmedLine);
        }
    }
    // lib dirs
    targetStr = QByteArray("libraries: =");
    delimPos1 = output.indexOf(targetStr);
    if (delimPos1>=0) {
        delimPos1+=targetStr.length();
        delimPos2 = delimPos1;
        while (delimPos2 < output.length() && output[delimPos2]!='\n')
            delimPos2+=1;
        QList<QByteArray> lines = output.mid(delimPos1,delimPos2-delimPos1).split(';');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty())
                addExistingDirectory(mDefaultLibDirs,trimmedLine);
        }
    }
}

#ifdef ENABLE_SDCC
void CompilerSet::setSDCCDirectories(const QString& binDir)
{
    QString c_prog = SDCC_PROGRAM;
    // Find default directories
    // C include dirs
    QStringList arguments;
    arguments.clear();
    arguments.append("--print-search-dirs");
    QString key = SDCC_CMD_OPT_PROCESSOR;
    PCompilerOption pOption = CompilerInfoManager::getCompilerOption(compilerType(), key);
    if (pOption) {
        if (!mCompileOptions[key].isEmpty())
            arguments.append(pOption->setting + mCompileOptions[key]);
    }
    QByteArray output = getCompilerOutput(binDir,c_prog,arguments);

    //bindirs
    QByteArray targetStr = QByteArray("programs:");
    int delimPos1 = output.indexOf(targetStr);
    int delimPos2 = output.indexOf("datadir:");
    if (delimPos1 >0 && delimPos2>delimPos1 ) {
        delimPos1 += targetStr.length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mBinDirs,trimmedLine);
            }
        }
    }

    targetStr = QByteArray("includedir:");
    delimPos1 = output.indexOf(targetStr);
    delimPos2 = output.indexOf("libdir:");
    if (delimPos1 >0 && delimPos2>delimPos1 ) {
        delimPos1 += targetStr.length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mDefaultCIncludeDirs,trimmedLine);
            }
        }
    }

    targetStr = QByteArray("libdir:");
    delimPos1 = output.indexOf(targetStr);
    delimPos2 = output.indexOf("libpath:");
    if (delimPos1 >0 && delimPos2>delimPos1 ) {
        delimPos1 += targetStr.length();
        QList<QByteArray> lines = output.mid(delimPos1, delimPos2-delimPos1).split('\n');
        for (QByteArray& line:lines) {
            QByteArray trimmedLine = line.trimmed();
            if (!trimmedLine.isEmpty()) {
                addExistingDirectory(mDefaultLibDirs,trimmedLine);
            }
        }
    }

}
#endif

// int CompilerSet::mainVersion() const
// {
//     int i = mVersion.indexOf('.');
//     if (i<0)
//         return -1;
//     bool ok;
//     int num = mVersion.left(i).toInt(&ok);
//     if (!ok)
//         return -1;
//     return num;

// }

bool CompilerSet::canCompileC() const
{
    return fileExists(mCCompiler);
}

bool CompilerSet::canCompileCPP() const
{
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC)
        return false;
#endif
    return fileExists(mCppCompiler);
}

bool CompilerSet::canMake() const
{
    return fileExists(mMake);
}

bool CompilerSet::canDebug() const
{
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC)
        return false;
#endif
    return fileExists(mDebugger);
}

void CompilerSet::setUserInput()
{
    mUseCustomCompileParams = false;
    mUseCustomLinkParams = false;
#ifdef ENABLE_SDCC
    if (mCompilerType==CompilerType::SDCC) {
        mAutoAddCharsetParams = false;
        mStaticLink = false;
    } else {
#else
    {
#endif
        mAutoAddCharsetParams = true;
        mStaticLink = true;
    }
}


QString CompilerSet::findProgramInBinDirs(const QString name) const
{
    for (const QString& dir : mBinDirs) {
        QFileInfo f(getAbsoluteFilePath(dir, name));
        if (f.exists() && f.isExecutable()) {
            return f.absoluteFilePath();
        }
    }
    return QString();
}

void CompilerSet::setIniOptions(const QByteArray &value)
{
   if (value.isEmpty())
       return;
   mCompileOptions.clear();
   for (int i=0;i<value.length();i++) {
       QString key = CompilerSets::getKeyFromCompilerCompatibleIndex(i);
       setCompileOption(key,charToValue(value[i]));
   }
}

QByteArray CompilerSet::getCompilerOutput(const QString &binDir, const QString &binFile, const QStringList &arguments) const
{
    QProcessEnvironment env;
    env.insert("LANGUAGE","");
    env.insert("LC_ALL", "C");
    QString path = binDir;
    env.insert("PATH",path);
    auto [result, _, errorMessage] = runAndGetOutput(
                getFilePath(binDir, binFile),
                binDir,
                arguments,
                QByteArray(),
                false,
                false,
                env);
    return result.trimmed();
}

bool CompilerSet::forceEnglishOutput() const
{
    return mForceEnglishOutput;
}

void CompilerSet::setForceEnglishOutput(bool newForceEnglishOutput)
{
    mForceEnglishOutput = newForceEnglishOutput;
}

bool CompilerSet::persistInAutoFind() const
{
    return mPersistInAutoFind;
}

void CompilerSet::setPersistInAutoFind(bool newPersistInAutoFind)
{
    mPersistInAutoFind = newPersistInAutoFind;
}

QString CompilerSet::getOutputFilename(const QString &sourceFilename)
{
    return getOutputFilename(sourceFilename, CompilationStage::GenerateExecutable);
}

QString CompilerSet::getOutputFilename(const QString &sourceFilename, CompilationStage stage)
{
    switch(stage) {
    case CompilerSet::CompilationStage::PreprocessingOnly:
        return changeFileExt(sourceFilename, preprocessingSuffix());
    case CompilerSet::CompilationStage::CompilationProperOnly:
        return changeFileExt(sourceFilename, compilationProperSuffix());
    case CompilerSet::CompilationStage::GenerateGimple:
        return changeFileExt(sourceFilename, "gimple");
    case CompilerSet::CompilationStage::AssemblingOnly:
        return changeFileExt(sourceFilename, assemblingSuffix());
    case CompilerSet::CompilationStage::GenerateExecutable:
        return changeFileExt(sourceFilename, executableSuffix());
    }
    return changeFileExt(sourceFilename,DEFAULT_EXECUTABLE_SUFFIX);
}

bool CompilerSet::isOutputExecutable()
{
    return isOutputExecutable(CompilationStage::GenerateExecutable);
}

bool CompilerSet::isOutputExecutable(CompilationStage stage)
{
    return stage == CompilationStage::GenerateExecutable;
}

#ifdef Q_OS_WINDOWS
bool CompilerSet::isDebuggerUsingUTF8() const
{
    switch(mCompilerType) {
    case CompilerType::Clang:
        return true;
    case CompilerType::GCC:
        if (!mGDBIsUtf8Initialized) {
            mGDBIsUtf8 = applicationIsUtf8(mDebugger);
            mGDBIsUtf8Initialized = true;
        }
        return mGDBIsUtf8;
    default:
        return false;
    }
}

bool CompilerSet::isCompilerUsingUTF8() const
{
    switch(mCompilerType) {
    case CompilerType::Clang:
        return true;
    case CompilerType::GCC:
        if (!mGccIsUtf8Initialized) {
            mGccIsUtf8 = applicationIsUtf8(mCCompiler);
            mGccIsUtf8Initialized = true;
        }
        return mGccIsUtf8;
    default:
        return false;
    }
}
#endif

bool CompilerSet::supportConvertingCharset()
{
#ifdef Q_OS_WIN
    if (mCompilerType != CompilerType::GCC)
        return false;
    if (!mGccSupportConvertingCharsetInitialized) {
        mGccSupportConvertingCharset = [this] () {
            QByteArray verboseOut = getCompilerOutput(QFileInfo(mCCompiler).dir().path(), mCCompiler, {"-v"});
            QByteArray targetStr = "Configured with: ";
            int configurationPos = verboseOut.indexOf(targetStr);
            if (configurationPos < 0)
                return false;
            int endPos = verboseOut.indexOf('\n', configurationPos);
            if (endPos < 0)
                return false;
            QByteArray configuration = verboseOut.mid(configurationPos + targetStr.size(), endPos - configurationPos - targetStr.size());
            return configuration.contains("--with-libiconv") && !configuration.contains("--with-libiconv=no");
        } ();
        mGccSupportConvertingCharsetInitialized = true;
    }
    return mGccSupportConvertingCharset;
#else
    // Unix: iconv is a part of POSIX standard.
    return mCompilerType == CompilerType::GCC;
#endif
}

bool CompilerSet::supportNLS()
{
    if (mCompilerType != CompilerType::GCC)
        return false;
    if (!mGccSupportNLSInitialized) {
        mGccSupportNLS = [this] () {
            QByteArray verboseOut = getCompilerOutput(QFileInfo(mCCompiler).dir().path(), mCCompiler, {"-v"});
            QByteArray targetStr = "Configured with: ";
            int configurationPos = verboseOut.indexOf(targetStr);
            if (configurationPos < 0)
                return false;
            int endPos = verboseOut.indexOf('\n', configurationPos);
            if (endPos < 0)
                return false;
            QByteArray configuration = verboseOut.mid(configurationPos + targetStr.size(), endPos - configurationPos - targetStr.size());
            return configuration.contains("--enable-nls") && !configuration.contains("--disable-nls");
        } ();
        mGccSupportNLSInitialized = true;
    }
    return mGccSupportNLS;

}

const QString &CompilerSet::assemblingSuffix() const
{
    return mAssemblingSuffix;
}

void CompilerSet::setAssemblingSuffix(const QString &newAssemblingSuffix)
{
    mAssemblingSuffix = newAssemblingSuffix;
}

const QString &CompilerSet::compilationProperSuffix() const
{
    return mCompilationProperSuffix;
}

void CompilerSet::setCompilationProperSuffix(const QString &newCompilationProperSuffix)
{
    mCompilationProperSuffix = newCompilationProperSuffix;
}

const QString &CompilerSet::preprocessingSuffix() const
{
    return mPreprocessingSuffix;
}

void CompilerSet::setPreprocessingSuffix(const QString &newPreprocessingSuffix)
{
    mPreprocessingSuffix = newPreprocessingSuffix;
}

const QString &CompilerSet::executableSuffix() const
{
    return mExecutableSuffix;
}

void CompilerSet::setExecutableSuffix(const QString &newExecutableSuffix)
{
    mExecutableSuffix = newExecutableSuffix;
}

const QMap<QString, QString> &CompilerSet::compileOptions() const
{
    return mCompileOptions;
}

const QString &CompilerSet::execCharset() const
{
    return mExecCharset;
}

void CompilerSet::setExecCharset(const QString &newExecCharset)
{
    mExecCharset = newExecCharset;
}

const QString &CompilerSet::debugServer() const
{
    return mDebugServer;
}

void CompilerSet::setDebugServer(const QString &newDebugServer)
{
    mDebugServer = newDebugServer;
}

QStringList CompilerSet::findErrors()
{
    QStringList errors;
    if (!mCCompiler.isEmpty() && !fileExists(mCCompiler)) {
        errors.append(QObject::tr("C Compiler \"%1\" is missing!").arg(mCCompiler));
    }
    if (!mCppCompiler.isEmpty() && !fileExists(mCppCompiler)) {
        errors.append(QObject::tr("C++ Compiler \"%1\" is missing!").arg(mCppCompiler));
    }
    if (!mDebugger.isEmpty() && !fileExists(mDebugger)) {
        errors.append(QObject::tr("Debugger \"%1\" is missing!").arg(mDebugger));
    }
    if (!mMake.isEmpty() && !fileExists(mMake)) {
        errors.append(QObject::tr("Make program \"%1\" is missing!").arg(mMake));
    }
    return errors;
}

void CompilerSet::setCompilerType(CompilerType newCompilerType)
{
    mCompilerType = newCompilerType;
}

CompilerType CompilerSet::compilerType() const
{
    return mCompilerType;
}

bool CompilerSet::staticLink() const
{
    return mStaticLink;
}

void CompilerSet::setStaticLink(bool newStaticLink)
{
    mStaticLink = newStaticLink;
}

bool CompilerSet::useCustomCompileParams() const
{
    return mUseCustomCompileParams;
}

CompilerSets::CompilerSets(SettingsPersistor *persistor, DirSettings *dirSettings):
    mDirSettings{dirSettings},
    mDefaultIndex{-1},
    mPersistor{persistor}
{
    Q_ASSERT(mDirSettings!=nullptr);
    Q_ASSERT(mPersistor!=nullptr);
    prepareCompatibleIndex();
}

PCompilerSet CompilerSets::addSet()
{
    PCompilerSet p=std::make_shared<CompilerSet>();
    mList.push_back(p);
    return p;
}

PCompilerSet CompilerSets::addSet(const QString &folder, const QString& c_prog)
{
    PCompilerSet p=std::make_shared<CompilerSet>(folder,c_prog);
    if (c_prog==GCC_PROGRAM && p->compilerType()==CompilerType::Clang)
        return PCompilerSet();
    mList.push_back(p);
    return p;
}

PCompilerSet CompilerSets::addSet(const PCompilerSet &pSet)
{
    PCompilerSet p=std::make_shared<CompilerSet>(*pSet);
    mList.push_back(p);
    return p;
}

PCompilerSet CompilerSets::addSet(const QJsonObject &set)
{
    PCompilerSet p = std::make_shared<CompilerSet>(set);
    mList.push_back(p);
    return p;
}

static void setX86MultilibOptions(PCompilerSet pSet, const QString &value) {
    pSet->setCompileOption(CC_CMD_OPT_POINTER_SIZE, value);
}

static void setReleaseOptions(PCompilerSet pSet) {
    pSet->setCompileOption(CC_CMD_OPT_OPTIMIZE,"2");
    pSet->setCompileOption(LINK_CMD_OPT_STRIP_EXE, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_USE_PIPE, COMPILER_OPTION_ON);

    pSet->setCompileOption(CC_CMD_OPT_NO_MS_EXTENSIONS, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_ERROR_RETURN_TYPE, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_ERROR_VLA, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_ERROR_IMPLICIT_INT, COMPILER_OPTION_ON);
    pSet->setStaticLink(true);
}

static void setDebugOptions(PCompilerSet pSet, bool enableAsan = false) {
    //pSet->setCompileOption(CC_CMD_OPT_OPTIMIZE,"g");
    pSet->setCompileOption(CC_CMD_OPT_DEBUG_INFO, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_WARNING_ALL, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_NO_MS_EXTENSIONS, COMPILER_OPTION_ON);
    //pSet->setCompileOption(CC_CMD_OPT_WARNING_EXTRA, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_USE_PIPE, COMPILER_OPTION_ON);

    pSet->setCompileOption(CC_CMD_OPT_ERROR_RETURN_TYPE, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_ERROR_VLA, COMPILER_OPTION_ON);
    pSet->setCompileOption(CC_CMD_OPT_ERROR_IMPLICIT_INT, COMPILER_OPTION_ON);

    if (enableAsan) {
#ifdef __aarch64__
        pSet->setCompileOption(CC_CMD_OPT_ADDRESS_SANITIZER, "hwaddress");
#else
        pSet->setCompileOption(CC_CMD_OPT_ADDRESS_SANITIZER, "address");
#endif
    }
    //Some windows gcc don't correctly support this
    //pSet->setCompileOption(CC_CMD_OPT_STACK_PROTECTOR, "-strong");
    pSet->setStaticLink(false);

}

bool CompilerSets::addSets(const QString &folder, const QString& c_prog) {
    for (const PCompilerSet& set:mList) {
        if (set->binDirs().contains(folder) && extractFileName(set->CCompiler())==c_prog)
            return false;
    }
    // Default, release profile
    PCompilerSet baseSet = addSet(folder,c_prog);
    if (!baseSet || baseSet->name().isEmpty())
        return false;
#if ENABLE_SDCC
    if (c_prog == SDCC_PROGRAM) {
        baseSet->setCompileOption(SDCC_OPT_NOSTARTUP,COMPILER_OPTION_ON);
    } else {
#else
    {
#endif
        QString baseName = baseSet->name();
        QString platformName;
        if (isTarget64Bit(baseSet->target())) {
            platformName = "64-bit";
        } else {
            platformName = "32-bit";
        }

        // handling x86 multilib
        if (baseSet->target() == "x86_64") {
            auto multilibs = baseSet->x86MultilibList(folder, c_prog);
            for (const QString &value : multilibs) {
                PCompilerSet set= addSet(baseSet);
                set->setName(baseName + " multilib " + value + " Release");
                setX86MultilibOptions(set, value);
                setReleaseOptions(set);

                set = addSet(baseSet);
                set->setName(baseName + " multilib " + value + " Debug");
                setX86MultilibOptions(set, value);
                setDebugOptions(set);
            }
        }


        PCompilerSet debugSet = addSet(baseSet);
        debugSet->setName(baseName + " " + platformName + " Debug");
        setDebugOptions(debugSet);

        // Enable ASan compiler set if it is supported and gdb works with ASan.
#ifdef Q_OS_LINUX
        PCompilerSet debugAsanSet = addSet(baseSet);
        debugAsanSet->setName(baseName + " " + platformName + " Debug with ASan");
        setDebugOptions(debugAsanSet, true);
#endif

        baseSet->setName(baseName + " " + platformName + " Release");
        setReleaseOptions(baseSet);
    }

#ifdef Q_OS_LINUX
# if defined(__x86_64__) || defined(__aarch64__) || __SIZEOF_POINTER__ == 4
    mDefaultIndex = (int)mList.size() - 1; // x86-64, AArch64 Linux or 32-bit Unix, default to "debug with ASan"
# else
    mDefaultIndex = (int)mList.size() - 2; // other Unix, where ASan can be very slow, default to "debug"
# endif
#else
    mDefaultIndex = (int)mList.size() - 1;
#endif

    return true;

}

bool CompilerSets::addSets(const QString &folder)
{
    bool found = false;
    if (!directoryExists(folder))
        return found;
    if (fileExists(folder, GCC_PROGRAM)) {
        addSets(folder,GCC_PROGRAM);
        found=true;
    }
    if (fileExists(folder, CLANG_PROGRAM)) {
        addSets(folder,CLANG_PROGRAM);
        found=true;
    }
#ifdef ENABLE_SDCC
    //qDebug()<<folder;
    if (fileExists(folder, SDCC_PROGRAM)) {
        addSets(folder,SDCC_PROGRAM);
        found=true;
    }
#endif
    return found;
}

CompilerSetList CompilerSets::clearSets()
{
    CompilerSetList persisted;
    for (size_t i=0;i<mList.size();i++) {
        mPersistor->beginGroup(QString(SETTING_COMPILTER_SET).arg(i));
        mPersistor->remove("");
        mPersistor->endGroup();
        if (mList[i]->persistInAutoFind())
            persisted.push_back(std::move(mList[i]));
    }
    mList.clear();
    mDefaultIndex = -1;
    return persisted;
}

void CompilerSets::findSets()
{
    CompilerSetList persisted = clearSets();
    // canonical paths that has been searched.
    // use canonical paths here to resolve symbolic links.
    QSet<QString> searched;

#ifdef ENABLE_LUA_ADDON
    QJsonObject compilerHint;
    if (
        QFile scriptFile(pSettings->dirs().appLibexecDir() + "/compiler_hint.lua");
        scriptFile.exists() && scriptFile.open(QFile::ReadOnly)
    ) {
        QByteArray script = scriptFile.readAll();
        try {
            compilerHint = AddOn::Lua::CompilerHintExecutor{}(script);
        } catch (const AddOn::LuaError &e) {
            QMessageBox::critical(nullptr,
                                  QObject::tr("Error executing platform compiler hint add-on"),
                                  e.reason());
        }
        if (!compilerHint.empty()) {
            QJsonArray compilerList = compilerHint["compilerList"].toArray();
            for (const QJsonValue &value : compilerList) {
                addSet(value.toObject());
            }
            QJsonArray noSearch = compilerHint["noSearch"].toArray();
            QString canonicalPath;
            for (const QJsonValue &value : noSearch) {
                canonicalPath = QDir(value.toString()).canonicalPath();
                if (!canonicalPath.isEmpty())
                    searched.insert(canonicalPath);
            }
        }
    }
#endif

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathList = path.split(PATH_SEPARATOR);
#ifdef Q_OS_WIN
    pathList = QStringList{
        mDirSettings->appDir() + "/clang64/bin",
        mDirSettings->appDir() + "/mingw64/bin",
        mDirSettings->appDir() + "/mingw32/bin",

        // cross toolchain targeting Linux
        // directory names follow dynamic linker (ld-linux-x86-64.so -> gcc-linux-x86-64)
        mDirSettings->appDir() + "/gcc-linux-x86-64/bin",
        mDirSettings->appDir() + "/gcc-linux-aarch64/bin",
    } + pathList;
#endif
    QString folder, canonicalFolder;
    for (int i=pathList.count()-1;i>=0;i--) {
        folder = QDir(pathList[i]).absolutePath();
        canonicalFolder = QDir(pathList[i]).canonicalPath();
        if (canonicalFolder.isEmpty())
            continue;
        if (searched.contains(canonicalFolder))
            continue;
        searched.insert(canonicalFolder);
        // but use absolute path to search so compiler set can survive system upgrades.
        // during search:
        //   /opt/gcc-13 -> /opt/gcc-13.1.0
        // after upgrade:
        //   /opt/gcc-13 -> /opt/gcc-13.2.0
        addSets(folder);
    }

#ifdef ENABLE_LUA_ADDON
    if (
        // note that array index starts from 1 in Lua
        int preferCompilerInLua = compilerHint["preferCompiler"].toInt();
        preferCompilerInLua >= 1 && preferCompilerInLua <= (int)mList.size()
    ) {
        mDefaultIndex = preferCompilerInLua - 1;
    }
#endif

    for (PCompilerSet &set: persisted)
        addSet(set);
}

void CompilerSets::saveSets()
{
    for (size_t i=0;i<mList.size();i++) {
        saveSet(i);
    }
    if (mDefaultIndex>=(int)mList.size()) {
        setDefaultIndex( mList.size()-1 );
    }
    mPersistor->beginGroup(SETTING_COMPILTER_SETS);
    mPersistor->saveValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX,mDefaultIndex);
    mPersistor->saveValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX_TIMESTAMP,mDefaultIndexTimeStamp);
    mPersistor->saveValue(SETTING_COMPILTER_SETS_COUNT,(int)mList.size());

    mPersistor->endGroup();
}

void CompilerSets::loadSets()
{
    mList.clear();
    mPersistor->beginGroup(SETTING_COMPILTER_SETS);
    mDefaultIndex = mPersistor->value(SETTING_COMPILTER_SETS_DEFAULT_INDEX,-1).toInt();
    mDefaultIndexTimeStamp = mPersistor->value(SETTING_COMPILTER_SETS_DEFAULT_INDEX_TIMESTAMP,0).toLongLong();
    //fix error time
    if (mDefaultIndexTimeStamp > QDateTime::currentMSecsSinceEpoch())
        mDefaultIndexTimeStamp = QDateTime::currentMSecsSinceEpoch();
    int listSize = mPersistor->value(SETTING_COMPILTER_SETS_COUNT,0).toInt();
    mPersistor->endGroup();
    bool loadError = false;
    for (int i=0;i<listSize;i++) {
        PCompilerSet pSet=loadSet(i);
        if (!pSet) {
            loadError = true;
            break;
        }
        mList.push_back(pSet);
    }
    if (loadError) {
        mList.clear();
        setDefaultIndex(-1);
    }
    PCompilerSet pCurrentSet = defaultSet();
    if (pCurrentSet) {
//        if (!pCurrentSet->dirsValid(msg)) {
//            if (QMessageBox::warning(nullptr,QObject::tr("Confirm"),
//                       QObject::tr("The following problems were found during validation of compiler set \"%1\":")
//                                     .arg(pCurrentSet->name())
//                                     +"<br /><br />"
//                                     +msg
//                                     +"<br /><br />"
//                                     +QObject::tr("Leaving those directories will lead to problems during compilation.")
//                                     +"<br /><br />"
//                                     +QObject::tr("Would you like Red Panda C++ to remove them for you and add the default paths to the valid paths?")
//                                     ,
//                                     QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
//                return;
//            }
//            findSets();
//            if ( (int)mList.size() <= mDefaultIndex)
//                mDefaultIndex =  mList.size()-1;
//            pCurrentSet = defaultSet();
//            if (!pCurrentSet) {
//                mList.clear();
//                mDefaultIndex = -1;
//                saveSets();
//                return;
//            }
//            saveSets();
//            pCurrentSet->setProperties(pCurrentSet->CCompiler());
//        } else {
//            return;
//        }
        return;
    } else {
#ifdef Q_OS_WIN
        QString msg = QObject::tr("Compiler set not configuared.")
                +"<br /><br />"
                +QObject::tr("Would you like Red Panda C++ to search for compilers in the following locations: <BR />'%1'<BR />'%2'? ")
                .arg(getFilePath(mDirSettings->appDir(), "mingw32"))
                .arg(getFilePath(mDirSettings->appDir(), + "mingw64"));
#else
        QString msg = QObject::tr("Compiler set not configuared.")
                +"<br /><br />"
                +QObject::tr("Would you like Red Panda C++ to search for compilers in PATH?");
#endif
        if (QMessageBox::warning(nullptr,QObject::tr("Confirm"),
                   msg,
                                 QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        findSets();
        if (size()==0) {
            QMessageBox::warning(
                nullptr,
                QObject::tr("No Compiler Set"),
                QObject::tr("Can't find a C/C++ compiler.")
                    +"<br/>"
                    +QObject::tr("You must have a compiler to compile and execute C/C++ files."));
        }
        pCurrentSet = defaultSet();
        if (!pCurrentSet) {
            mList.clear();
            setDefaultIndex(-1);
            saveSets();
            return;
        }
        saveSets();

    }

}

void CompilerSets::saveDefaultIndex()
{
    mPersistor->beginGroup(SETTING_COMPILTER_SETS);
    mPersistor->saveValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX,mDefaultIndex);
    mPersistor->saveValue(SETTING_COMPILTER_SETS_DEFAULT_INDEX_TIMESTAMP,mDefaultIndexTimeStamp);
    mPersistor->endGroup();
}

void CompilerSets::deleteSet(int index)
{
    // Erase all sections at and above from disk
    for (size_t i=index;i<mList.size();i++) {
        mPersistor->beginGroup(QString(SETTING_COMPILTER_SET).arg(i));
        mPersistor->remove("");
        mPersistor->endGroup();
    }
    mList.erase(std::begin(mList)+index);
    saveSets();
}

size_t CompilerSets::size() const
{
    return mList.size();
}

int CompilerSets::defaultIndex() const
{
    return mDefaultIndex;
}

qint64 CompilerSets::defaultIndexTimestamp() const
{
    return mDefaultIndexTimeStamp;
}

void CompilerSets::setDefaultIndex(int value)
{
    mDefaultIndex = value;
    mDefaultIndexTimeStamp = QDateTime::currentMSecsSinceEpoch();
}

PCompilerSet CompilerSets::defaultSet()
{
    return getSet(mDefaultIndex);
}

PCompilerSet CompilerSets::getSet(int index)
{
    if (index>=0 && index<(int)mList.size()) {
        return mList[index];
    }
    return PCompilerSet();
}

void CompilerSets::savePath(const QString& name, const QString& path) {
    if (!isGreenEdition()) {
        mPersistor->saveValue(name, path);
        return;
    }

    QString s;
    QString prefix1 = excludeTrailingPathDelimiter(mDirSettings->appDir()) + "/";
    QString prefix2 = excludeTrailingPathDelimiter(mDirSettings->appDir()) + QDir::separator();
    if (path.startsWith(prefix1, PATH_SENSITIVITY)) {
        s = "%AppPath%/"+ path.mid(prefix1.length());
    } else if (path.startsWith(prefix2, PATH_SENSITIVITY)) {
        s = "%AppPath%/"+ path.mid(prefix2.length());
    } else {
        s= path;
    }
    mPersistor->saveValue(name,s);
}

void CompilerSets::savePathList(const QString& name, const QStringList& pathList) {
    if (!isGreenEdition()) {
        mPersistor->saveValue(name, pathList);
        return;
    }

    QStringList sl;
    for (const QString& path: pathList) {
        QString s;
        QString prefix1 = excludeTrailingPathDelimiter(mDirSettings->appDir()) + "/";
        QString prefix2 = excludeTrailingPathDelimiter(mDirSettings->appDir()) + QDir::separator();
        if (path.startsWith(prefix1, PATH_SENSITIVITY)) {
            s = "%AppPath%/"+ path.mid(prefix1.length());
        } else if (path.startsWith(prefix2, PATH_SENSITIVITY)) {
            s = "%AppPath%/" + path.mid(prefix2.length());
        } else {
            s= path;
        }
        sl.append(s);
    }
    mPersistor->saveValue(name,sl);
}

void CompilerSets::saveSet(int index)
{
    PCompilerSet pSet = mList[index];
    mPersistor->beginGroup(QString(SETTING_COMPILTER_SET).arg(index));

    savePath("ccompiler", pSet->CCompiler());
    savePath("cppcompiler", pSet->cppCompiler());
    savePath("debugger", pSet->debugger());
    savePath("debug_server", pSet->debugServer());
    savePath("make", pSet->make());
    savePath("windres", pSet->resourceCompiler());

    mPersistor->remove("Options");
    foreach(const PCompilerOption& option, CompilerInfoManager::getInstance()->getCompilerOptions(pSet->compilerType())) {
        mPersistor->remove(option->key);
    }
    // Save option string
    foreach (const QString& optionKey, pSet->compileOptions().keys()) {
        mPersistor->saveValue(optionKey, pSet->compileOptions().value(optionKey));
    }

    // Save extra 'general' options
    mPersistor->saveValue("useCustomCompileParams", pSet->useCustomCompileParams());
    mPersistor->saveValue("customCompileParams", pSet->customCompileParams());
    mPersistor->saveValue("useCustomLinkParams", pSet->useCustomLinkParams());
    mPersistor->saveValue("customLinkParams", pSet->customLinkParams());
    mPersistor->saveValue("AddCharset", pSet->autoAddCharsetParams());
    mPersistor->saveValue("StaticLink", pSet->staticLink());
    mPersistor->saveValue("ExecCharset", pSet->execCharset());
    mPersistor->saveValue("PersistInAutoFind", pSet->persistInAutoFind());
    mPersistor->saveValue("forceEnglishOutput", pSet->forceEnglishOutput());

    mPersistor->saveValue("preprocessingSuffix", pSet->preprocessingSuffix());
    mPersistor->saveValue("compilationProperSuffix", pSet->compilationProperSuffix());
    mPersistor->saveValue("assemblingSuffix", pSet->assemblingSuffix());
    mPersistor->saveValue("executableSuffix", pSet->executableSuffix());

    // Misc. properties
    mPersistor->saveValue("DumpMachine", pSet->dumpMachine());
    mPersistor->saveValue("Version", pSet->version());
    mPersistor->saveValue("Name", pSet->name());
    mPersistor->saveValue("Target", pSet->target());
    mPersistor->saveValue("CompilerType", (int)pSet->compilerType());

    // Paths
    savePathList("Bins",pSet->binDirs());
    savePathList("C",pSet->CIncludeDirs());
    savePathList("Cpp",pSet->CppIncludeDirs());
    savePathList("Libs",pSet->libDirs());

    mPersistor->endGroup();
}

QString CompilerSets::loadPath(const QString &name)
{
    QString s =  mPersistor->value(name).toString();
    // replace libexec dir for forward compatibility
    QString prefix = "%*APP_LIBEXEC_DIR*%/";
    if (s.startsWith(prefix)) {
        s = getFilePath(mDirSettings->appLibexecDir(), s.mid(prefix.length()));
    }
    prefix = "%AppPath%/";
    if (s.startsWith(prefix)) {
        s = getFilePath(mDirSettings->appDir(), s.mid(prefix.length()));
    }
    return QFileInfo(s).absoluteFilePath();
}

void CompilerSets::loadPathList(const QString &name, QStringList& list)
{
    list.clear();
    QStringList sl = mPersistor->value(name).toStringList();
    // replace libexec dir for forward compatibility
    QString prefix1 = "%*APP_LIBEXEC_DIR*%/";
    QString prefix2 = "%AppPath%/";
    for (QString& s:sl) {
        if (s.startsWith(prefix1)) {
            s = getFilePath(mDirSettings->appLibexecDir(), s.mid(prefix1.length()));
        } else if (s.startsWith(prefix2)) {
            s = getFilePath(mDirSettings->appDir(), s.mid(prefix2.length()));
        }
        list.append(QFileInfo(s).absoluteFilePath());
    }
}

PCompilerSet CompilerSets::loadSet(int index)
{
    PCompilerSet pSet = std::make_shared<CompilerSet>();
    mPersistor->beginGroup(QString(SETTING_COMPILTER_SET).arg(index));

    pSet->setCCompiler(loadPath("ccompiler"));
    pSet->setCppCompiler(loadPath("cppcompiler"));
    pSet->setDebugger(loadPath("debugger"));
    pSet->setDebugServer(loadPath("debug_server"));
    pSet->setMake(loadPath("make"));
    pSet->setResourceCompiler(loadPath("windres"));

    pSet->setDumpMachine(mPersistor->value("DumpMachine").toString());
    pSet->setVersion(mPersistor->value("Version").toString());
    pSet->setName(mPersistor->value("Name").toString());
    pSet->setTarget(mPersistor->value("Target").toString());
    //compatibility
    QString temp = mPersistor->value("CompilerType").toString();
    if (temp==COMPILER_CLANG) {
        pSet->setCompilerType(CompilerType::Clang);
    } else if (temp==COMPILER_GCC) {
        pSet->setCompilerType(CompilerType::GCC);
    } else if (temp==COMPILER_GCC_UTF8) {
        pSet->setCompilerType(CompilerType::GCC);
#ifdef ENABLE_SDCC
    } else if (temp==COMPILER_SDCC) {
        pSet->setCompilerType(CompilerType::SDCC);
#endif
    } else {
        pSet->setCompilerType((CompilerType)mPersistor->value("CompilerType").toInt());
    }

    // Load extra 'general' options
    pSet->setUseCustomCompileParams(mPersistor->value("useCustomCompileParams", false).toBool());
    pSet->setCustomCompileParams(mPersistor->value("customCompileParams").toString());
    pSet->setUseCustomLinkParams(mPersistor->value("useCustomLinkParams", false).toBool());
    pSet->setCustomLinkParams(mPersistor->value("customLinkParams").toString());
    pSet->setAutoAddCharsetParams(mPersistor->value("AddCharset", true).toBool());
    pSet->setStaticLink(mPersistor->value("StaticLink", false).toBool());
    pSet->setPersistInAutoFind(mPersistor->value("PersistInAutoFind", false).toBool());
    bool forceEnglishOutput=QLocale::system().name().startsWith("zh")?false:true;
    pSet->setForceEnglishOutput(mPersistor->value("forceEnglishOutput", forceEnglishOutput).toBool());

    pSet->setExecCharset(mPersistor->value("ExecCharset", ENCODING_SYSTEM_DEFAULT).toString());
    if (pSet->execCharset().isEmpty()) {
        pSet->setExecCharset(ENCODING_SYSTEM_DEFAULT);
    }
    pSet->setPreprocessingSuffix(mPersistor->value("preprocessingSuffix", DEFAULT_PREPROCESSING_SUFFIX).toString());
    pSet->setCompilationProperSuffix(mPersistor->value("compilationProperSuffix",DEFAULT_COMPILATION_SUFFIX).toString());
    pSet->setAssemblingSuffix(mPersistor->value("assemblingSuffix", DEFAULT_ASSEMBLING_SUFFIX).toString());
    pSet->setExecutableSuffix(mPersistor->value("executableSuffix", DEFAULT_EXECUTABLE_SUFFIX).toString());

    // Load options
    QByteArray iniOptions = mPersistor->value("Options","").toByteArray();
    if (!iniOptions.isEmpty())
        pSet->setIniOptions(iniOptions);
    else {
        foreach (const QString &optionKey, mPersistor->allKeys()) {
            if (CompilerInfoManager::hasCompilerOption(pSet->compilerType(),optionKey)) {
                pSet->setCompileOption(optionKey, mPersistor->value(optionKey).toString());
            }
        }
    }

    // Paths
    loadPathList("Bins",pSet->binDirs());
    loadPathList("C",pSet->CIncludeDirs());
    loadPathList("Cpp",pSet->CppIncludeDirs());
    loadPathList("Libs",pSet->libDirs());

    mPersistor->endGroup();

//    if (pSet->binDirs().isEmpty())
//        return PCompilerSet();

    return pSet;
}

bool CompilerSets::isTarget64Bit(const QString &target)
{
    /* Fetched from LLVM 15.0.6's arch parser,
     *   `Triple::ArchType parseArch(StringRef ArchName)`
     *   in `llvm/lib/Support/Triple.cpp`.
     * The following non-CPU targets are not included:
     *   nvptx64, le64, amdil64, hsail64, spir64, spirv64, renderscript64.
     */
    QSet<QString> targets {
        // x86_64
        "amd64", "x86_64", "x86_64h",
        // ppc64
        "powerpc64", "ppu", "ppc64",
        // ppc64le
        "powerpc64le", "ppc64le",
        // aarch64
        "aarch64", "arm64", "arm64e",
        // aarch64_be
        "aarch64_be",
        // aarch64_32
        "aarch64_32", "arm64_32",
        // mips64
        "mips64", "mips64eb", "mipsn32", "mipsisa64r6", "mips64r6", "mipsn32r6",
        // mips64el
        "mips64el", "mipsn32el", "mipsisa64r6el", "mips64r6el", "mipsn32r6el",
        // riscv64
        "riscv64",
        // systemz
        "s390x", "systemz",
        // sparcv9
        "sparcv9", "sparc64",
        // wasm64
        "wasm64",
        // loongarch64
        "loongarch64",
    };
    return targets.contains(target);
}
