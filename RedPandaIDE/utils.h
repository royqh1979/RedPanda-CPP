#ifndef UTILS_H
#define UTILS_H

#include <type_traits>
#include <utility>
#include <functional>
#include <QString>

class QByteArray;
class QString;
class QStringList;
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
    Other // any others
};

enum class FileEndingType {
    Windows,
    Linux,
    Mac
};// Windows: CRLF, UNIX: LF, Mac: CR

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

bool isTextAllAscii(const QString& text);

QByteArray runAndGetOutput(const QString& cmd, const QString& workingDir, const QStringList& arguments, bool inheritEnvironment = false);

bool isNonPrintableAsciiChar(char ch);

bool fileExists(const QString& file);
bool fileExists(const QString& dir, const QString& fileName);
bool directoryExists(const QString& file);
QString includeTrailingPathDelimiter(const QString& path);
QString excludeTrailingPathDelimiter(const QString& path);
FileType getFileType(const QString& filename);
QString getCompiledExecutableName(const QString filename);
void splitStringArguments(const QString& arguments, QStringList& argumentList);
bool programHasConsole(const QString& filename);
QString toLocalPath(const QString& filename);

using LineProcessFunc =  std::function<void(const QString&)>;

QStringList ReadStreamToLines(QTextStream* stream);

void ReadStreamToLines(QTextStream* stream, LineProcessFunc lineFunc);


QStringList TextToLines(const QString& text);

void TextToLines(const QString& text, LineProcessFunc lineFunc);

QStringList ReadFileToLines(const QString& fileName, QTextCodec* codec);

void ReadFileToLines(const QString& fileName, QTextCodec* codec, LineProcessFunc lineFunc);

void decodeKey(int combinedKey, int& key, Qt::KeyboardModifiers& modifiers);

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
