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
#ifndef UTILS_H
#define UTILS_H
#include <type_traits>
#include <utility>
#include <functional>
#include <QString>
#include <QRect>
#include <QStringList>
#include <memory>
#include <QThread>
#include <QProcessEnvironment>
#include <QTemporaryFile>
#define SI_NO_CONVERSION
#include "SimpleIni.h"
#include "qt_utils/utils.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

using SimpleIni = CSimpleIniA;
using PSimpleIni = std::shared_ptr<SimpleIni>;
using TemporaryFileOwner = std::unique_ptr<QTemporaryFile>;

enum class FileType{
    GAS, // GNU assembler source file (.s)
    LUA, // lua file (.lua)
    CSource, // c source file (.c)
    CppSource, // c++ source file (.cpp)
    CHeader, // c header (.h)
    CppHeader, // c++ header (.hpp)
    WindowsResourceSource, // resource source (.res)
    Project, //Red Panda C++ Project (.dev)
    Text, // text file
    FragmentShader,
    VerticeShader,
    Other // any others
};

enum class SearchFileScope {
    currentFile,
    wholeProject,
    openedFiles,
    Folder
};

enum AutoSaveTarget {
    astCurrentFile,
    astAllOpennedFiles,
    astAllProjectFiles
};

enum AutoSaveStrategy {
    assOverwrite,
    assAppendUnixTimestamp,
    assAppendFormatedTimeStamp
};

enum FormatterBraceStyle {
    fbsDefault,
    fbsAllman,
    fbsJava,
    fbsKR,
    fbsStroustrup,
    fbsWitesmith,
    fbsVtk,
    fbsRatliff,
    fbsGNU,
    fbsLinux,
    fbsHorstmann,
    fbs1TBS,
    fbsGoogle,
    fbsMozilla,
    fbsWebkit,
    fbsPico,
    fbsLisp
};

enum FormatterOperatorAlign {
    foaNone,
    foaType,
    foaMiddle,
    foaName
};

enum FormatterIndentType {
    fitSpace,
    fitTab
};

enum class SplitProcessCommandQuoteType {
    None,
    Single,
    Double
};

enum class ProblemCaseValidateType {
    Exact,
    IgnoreLeadingTrailingSpaces,
    IgnoreSpaces
};

enum class UnixExecSemantics {
    Absolute,
    RelativeToCwd,
    SearchInPath,
};

enum class TerminalEmulatorArgumentsPattern {
    ImplicitSystem = 0,      //          bash -c  "echo hello, world; sleep 3"
    MinusEAppendArgs,        // term -e  bash -c  "echo hello, world; sleep 3"   # xterm-compatible
    MinusXAppendArgs,        // term -x  bash -c  "echo hello, world; sleep 3"   # some VTE-based
    MinusMinusAppendArgs,    // term --  bash -c  "echo hello, world; sleep 3"   # gnome-terminal, kgx
    MinusEAppendCommandLine, // term -e "bash -c \"echo hello, world; sleep 3\"" # some lightweighted; alternative form for VTE-based

    WriteCommandLineToTempFileThenTempFilename = 6226700, // macOS Terminal.app and iTerm2.app; 6226700 is how you dial “macOS00”
};

FileType getFileType(const QString& filename);
QStringList splitProcessCommand(const QString& cmd);

QString genMakePath(const QString& fileName,bool escapeSpaces, bool encloseInQuotes);
QString genMakePath1(const QString& fileName);
QString genMakePath2(const QString& fileName);
bool programHasConsole(const QString& filename);

QString parseMacros(const QString& s);

class CppParser;
void resetCppParser(std::shared_ptr<CppParser> parser, int compilerSetIndex=-1);

int getNewFileNumber();

QByteArray runAndGetOutput(const QString& cmd, const QString& workingDir, const QStringList& arguments,
                           const QByteArray& inputContent = QByteArray(),
                           bool inheritEnvironment = false,
                           const QProcessEnvironment& env = QProcessEnvironment() );

void openFileFolderInExplorer(const QString& path);

void executeFile(const QString& fileName,
                 const QString& params,
                 const QString& workingDir,
                 const QString& tempFile);

bool isGreenEdition();

#ifdef Q_OS_WIN
bool readRegistry(HKEY key,const QByteArray& subKey, const QByteArray& name, QString& value);
#endif

qulonglong stringToHex(const QString& str, bool &isOk);

bool findComplement(const QString& s,
                       const QChar& fromToken,
                       const QChar& toToken,
                       int& curPos,
                       int increment);

bool haveGoodContrast(const QColor& c1, const QColor &c2);

QByteArray getHTTPBody(const QByteArray& content);

QString getSizeString(int size);

class QComboBox;
void saveComboHistory(QComboBox* cb,const QString& text);

QColor alphaBlend(const QColor &lower, const QColor &upper);

UnixExecSemantics getPathUnixExecSemantics(const QString &path);

QStringList getExecutableSearchPaths();

QString escapeArgument(const QString &arg, bool isFirstArg);

auto wrapCommandForTerminalEmulator(const QString &terminal, const TerminalEmulatorArgumentsPattern &argsPattern, const QStringList &argsWithArgv0)
    -> std::tuple<QString, QStringList, std::unique_ptr<QTemporaryFile>>;

QString defaultShell();

#endif // UTILS_H
