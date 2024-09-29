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
    ModuleDef, // Windows Module Definition
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

struct NonExclusiveTemporaryFileOwner {
    const QString filename;

    // take ownership
    explicit NonExclusiveTemporaryFileOwner(std::unique_ptr<QTemporaryFile> &tempFile);

    NonExclusiveTemporaryFileOwner(const NonExclusiveTemporaryFileOwner &) = delete;
    NonExclusiveTemporaryFileOwner(NonExclusiveTemporaryFileOwner &&) = delete;
    NonExclusiveTemporaryFileOwner& operator=(const NonExclusiveTemporaryFileOwner &) = delete;
    NonExclusiveTemporaryFileOwner& operator=(NonExclusiveTemporaryFileOwner &&) = delete;
    ~NonExclusiveTemporaryFileOwner();
};

using PNonExclusiveTemporaryFileOwner = std::unique_ptr<NonExclusiveTemporaryFileOwner>;

FileType getFileType(const QString& filename);

bool programHasConsole(const QString& filename);

QString parseMacros(const QString& s);
QString parseMacros(const QString& s, const QMap<QString, QString>& variables);
QMap<QString, QString> devCppMacroVariables();

class CppParser;
void resetCppParser(std::shared_ptr<CppParser> parser, int compilerSetIndex=-1);

int getNewFileNumber();

struct ProcessOutput
{
    QByteArray standardOutput;
    QByteArray standardError;
    QString errorMessage;
};

ProcessOutput runAndGetOutput(const QString& cmd, const QString& workingDir, const QStringList& arguments,
                           const QByteArray& inputContent = QByteArray(),
                           bool separateStderr = false,
                           bool inheritEnvironment = false,
                           const QProcessEnvironment& env = QProcessEnvironment() );

void openFileFolderInExplorer(const QString& path);

void executeFile(const QString& fileName,
                 const QStringList& params,
                 const QString& workingDir,
                 const QString& tempFile);

#ifdef Q_OS_WIN
bool isGreenEdition();
#else
constexpr bool isGreenEdition() { return false; }
#endif

#ifdef Q_OS_WIN
bool readRegistry(HKEY key, const QString& subKey, const QString& name, QString& value);
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

QStringList getExecutableSearchPaths();

QStringList platformCommandForTerminalArgsPreview();

QString appArch();
QString osArch();

QString byteArrayToString(const QByteArray &content, bool isUTF8);
QByteArray stringToByteArray(const QString& content, bool isUTF8);

#ifdef _MSC_VER
#define __builtin_unreachable() (__assume(0))
#endif

std::tuple<QString, QStringList, PNonExclusiveTemporaryFileOwner> wrapCommandForTerminalEmulator(const QString &terminal, const QStringList &argsPattern, const QStringList &payloadArgsWithArgv0);

std::tuple<QString, QStringList, PNonExclusiveTemporaryFileOwner> wrapCommandForTerminalEmulator(const QString &terminal, const QString &argsPattern, const QStringList &payloadArgsWithArgv0);

struct ExternalResource {
    ExternalResource();
    ~ExternalResource();
};

template <typename T, typename D>
std::unique_ptr<T, D> resourcePointer(T *pointer, D deleter)
{
    return {pointer, deleter};
}

#ifdef Q_OS_WINDOWS
bool applicationHasUtf8Manifest(const wchar_t *path);
bool osSupportsUtf8Manifest();
bool applicationIsUtf8(const QString &path);
#endif

#if QT_VERSION_MAJOR >= 6
// for xml.name() == "tag"
inline bool operator==(QStringView a, const char *b)
{
    return a.compare(b);
}
#endif

#endif // UTILS_H
