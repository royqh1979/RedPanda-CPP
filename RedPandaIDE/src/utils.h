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
#include "utils/types.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

using SimpleIni = CSimpleIniA;
using PSimpleIni = std::shared_ptr<SimpleIni>;

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
QString fileTypeToName(FileType fileType);
FileType nameToFileType(const QString& name);
constexpr bool isASMSourceFile(FileType fileType) {
    return fileType == FileType::NASM || fileType == FileType::GAS;
}
constexpr bool isC_CPPSourceFile(FileType fileType) {
    return fileType == FileType::CSource || fileType == FileType::CppSource;
}
constexpr bool isC_CPPHeaderFile(FileType fileType) {
    return fileType == FileType::CCppHeader;
}
constexpr bool isC_CPP_ASMSourceFile(FileType fileType) {
    return isC_CPPSourceFile(fileType) || isASMSourceFile(fileType);
}

bool programIsWin32GuiApp(const QString& filename);

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
void setComboTextAndHistory(QComboBox *cb, const QString& newText, QStringList &historyList);
void updateComboHistory(QStringList &historyList, const QString &newKey);

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

#endif // UTILS_H
