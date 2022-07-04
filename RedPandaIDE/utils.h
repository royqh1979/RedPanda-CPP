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
#include "SimpleIni.h"

using SimpleIni = CSimpleIniA;
using PSimpleIni = std::shared_ptr<SimpleIni>;

class QByteArray;
class QTextStream;
class QTextCodec;

#define ENCODING_AUTO_DETECT "AUTO"
#define ENCODING_UTF8   "UTF-8"
#define ENCODING_UTF8_BOM "UTF-8 BOM"
#define ENCODING_SYSTEM_DEFAULT   "SYSTEM"
#define ENCODING_ASCII  "ASCII"

enum class FileType{
    CSource, // c source file (.c)
    CppSource, // c++ source file (.cpp)
    CHeader, // c header (.h)
    CppHeader, // c++ header (.hpp)
    WindowsResourceSource, // resource source (.res)
    Project, //Red Panda C++ Project (.dev)
    Other // any others
};

enum class FileEndingType {
    Windows,
    Linux,
    Mac
};// Windows: CRLF, UNIX: LF, Mac: CR

enum class SearchFileScope {
    currentFile,
    wholeProject,
    openedFiles
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

class BaseError{
public:
    explicit BaseError(const QString& reason);
    QString reason() const;

protected:
    QString mReason;
};

class IndexOutOfRange:public BaseError {
public:
    explicit IndexOutOfRange(int Index);
};

class FileError: public BaseError {
public:
    explicit FileError(const QString& reason);
};

typedef void (*LineOutputFunc) (const QString& line);
typedef bool (*CheckAbortFunc) ();
bool isGreenEdition();

const QByteArray GuessTextEncoding(const QByteArray& text);

bool isTextAllAscii(const QByteArray& text);
bool isTextAllAscii(const QString& text);

QByteArray runAndGetOutput(const QString& cmd, const QString& workingDir, const QStringList& arguments,
                           const QByteArray& inputContent = QByteArray(),
                           bool inheritEnvironment = false,
                           const QProcessEnvironment& env = QProcessEnvironment() );

void executeFile(const QString& fileName,
                 const QString& params,
                 const QString& workingDir,
                 const QString& tempFile);

bool isNonPrintableAsciiChar(char ch);

bool fileExists(const QString& file);
bool fileExists(const QString& dir, const QString& fileName);
bool directoryExists(const QString& file);
bool removeFile(const QString& filename);
QString includeTrailingPathDelimiter(const QString& path);
QString excludeTrailingPathDelimiter(const QString& path);
FileType getFileType(const QString& filename);
QString changeFileExt(const QString& filename, QString ext);
QString extractRelativePath(const QString& base, const QString& dest);
QString genMakePath(const QString& fileName,bool escapeSpaces, bool encloseInQuotes);
QString genMakePath1(const QString& fileName);
QString genMakePath2(const QString& fileName);
QString getCompiledExecutableName(const QString& filename);
void splitStringArguments(const QString& arguments, QStringList& argumentList);
bool programHasConsole(const QString& filename);
using LineProcessFunc =  std::function<void(const QString&)>;

QStringList readStreamToLines(QTextStream* stream);
void readStreamToLines(QTextStream* stream, LineProcessFunc lineFunc);

QStringList textToLines(const QString& text);
void textToLines(const QString& text, LineProcessFunc lineFunc);
QString linesToText(const QStringList& lines);

QList<QByteArray> splitByteArrayToLines(const QByteArray& content);

QString parseMacros(const QString& s);

/**
 * @brief readFileToLines
 * @param fileName
 * @param codec
 * @return
 */
QStringList readFileToLines(const QString& fileName, QTextCodec* codec);
QStringList readFileToLines(const QString& fileName);
QByteArray readFileToByteArray(const QString& fileName);
void readFileToLines(const QString& fileName, QTextCodec* codec, LineProcessFunc lineFunc);
void stringsToFile(const QStringList& list, const QString& fileName);
void stringToFile(const QString& str, const QString& fileName);
int countLeadingWhitespaceChars(const QString& line);

void decodeKey(int combinedKey, int& key, Qt::KeyboardModifiers& modifiers);
void inflateRect(QRect& rect, int delta);
void inflateRect(QRect& rect, int dx, int dy);
QString trimRight(const QString& s);
QString trimLeft(const QString& s);
bool stringIsBlank(const QString& s);
int compareFileModifiedTime(const QString& filename1, const QString& filename2);
QByteArray getHTTPBody(const QByteArray& content);
bool haveGoodContrast(const QColor& c1, const QColor &c2);
qulonglong stringToHex(const QString& str, bool &isOk);

//void changeTheme(const QString& themeName);

bool findComplement(const QString& s,
                       const QChar& fromToken,
                       const QChar& toToken,
                       int& curPos,
                       int increment);
void logToFile(const QString& s, const QString& filename, bool append=true);

QString localizePath(const QString& path);
QString extractFileName(const QString& fileName);
QString extractFileDir(const QString& fileName);
QString extractFilePath(const QString& filePath);
QString extractAbsoluteFilePath(const QString& filePath);
QString getSizeString(int size);
bool isReadOnly(const QString& filename);


QByteArray toByteArray(const QString& s);
QString fromByteArray(const QByteArray& s);

int getNewFileNumber();

enum class SplitProcessCommandQuoteType {
    None,
    Single,
    Double
};

QStringList splitProcessCommand(const QString& cmd);


#ifdef Q_OS_WIN
bool readRegistry(HKEY key,const QByteArray& subKey, const QByteArray& name, QString& value);
#endif

class CppParser;
void resetCppParser(std::shared_ptr<CppParser> parser, int compilerSetIndex=-1);

int screenDPI();
void setScreenDPI(int dpi);
float pointToPixel(float point);
float pointToPixel(float point, float dpi);
float pixelToPoint(float pixel);

void copyFolder(const QString &fromDir, const QString& toDir);


/**
 * from https://github.com/Microsoft/GSL
 **/

template <class F>
class final_action
{
public:
    static_assert(!std::is_reference<F>::value && !std::is_const<F>::value &&
                      !std::is_volatile<F>::value,
                  "Final_action should store its callable by value");

    explicit final_action(F f) noexcept : f_(std::move(f)) {}

    final_action(final_action&& other) noexcept
        : f_(std::move(other.f_)), invoke_(std::exchange(other.invoke_, false))
    {}

    final_action(const final_action&) = delete;
    final_action& operator=(const final_action&) = delete;
    final_action& operator=(final_action&&) = delete;

    ~final_action() noexcept
    {
        if (invoke_) f_();
    }

private:
    F f_;
    bool invoke_{true};
};

template <class F> final_action<typename std::remove_cv<typename std::remove_reference<F>::type>::type>
finally(F&& f) noexcept
{
    return final_action<typename std::remove_cv<typename std::remove_reference<F>::type>::type>(
        std::forward<F>(f));
}
#endif // UTILS_H
