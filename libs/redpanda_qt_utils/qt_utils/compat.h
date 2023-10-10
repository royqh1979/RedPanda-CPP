#ifndef COMPAT_H
#define COMPAT_H

/* For compatibility with Qt 5.6 -- 5.11, where `CONFIG += c++17`
 * does not work, only library features up to C++14 can be used.
 * i.e. `std::xxx_t` (since C++14) can be used while `std::xxx_v`
 * (since C++17) should not be used.
 *
 * Note that Red Panda C++ still requires C++17 core language support.
 */
#include <type_traits>
#include <utility>
#include <functional>

#include <QtCore>
#include <QFontMetrics>
#include <QWidget>

#ifdef Q_OS_WIN
# include <windows.h>
#endif

/* Missing macro:
 *   QT_CONFIG (undocumented)
 */

#ifndef QT_CONFIG
# define QT_CONFIG(feature) QT_SUPPORTS(feature)
#endif

/* Missing class:
 *   QOverload (5.7+)
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)

template <typename... Args>
struct QOverload
{
    template <typename C, typename R>
    static constexpr auto of(R (C::*pmf)(Args...)) -> decltype(pmf) { return pmf; }
};

#endif

/* Missing overloaded operator:
 *   bool operator==(QChar, const QString &) (undocumented)
 *   bool operator==(const QString &, QChar) (undocumented)
 *   bool operator!=(QChar, const QString &) (undocumented)
 *   bool operator!=(const QString &, QChar) (undocumented)
 * Since they are undocumented, here we implement these functions as templates to avoid conflicts.
 */

template <typename T, typename U>
auto operator==(T ch, const U &str)
    -> std::enable_if_t<std::is_convertible<T, QChar>::value && std::is_base_of<QString, U>::value, bool>
{
    return str.length() == 1 && ch == str[0];
}

template <typename T, typename U>
auto operator==(const U &str, T ch)
    -> std::enable_if_t<std::is_convertible<T, QChar>::value && std::is_base_of<QString, U>::value, bool>
{
    return ch == str;
}

template <typename T, typename U>
auto operator!=(T ch, const U &str)
    -> std::enable_if_t<std::is_convertible<T, QChar>::value && std::is_base_of<QString, U>::value, bool>
{
    return !(ch == str);
}

template <typename T, typename U>
auto operator!=(const U &str, T ch)
    -> std::enable_if_t<std::is_convertible<T, QChar>::value && std::is_base_of<QString, U>::value, bool>
{
    return !(ch == str);
}

/* Wrapper classes that add missing member functions.
 *
 * A wrapper class `Compat::T_` is implemented by inheriting `T` and adding missing
 * menber functions. `Compat::T_` inherits all constructors of `T` and adds a “move
 * constructor” `T_(T &&)` for convenience. Note that “copy constructor”
 * `T_(const T &)` is intentionally not provided to avoid implicit copy.
 */

namespace Compat {

/* Missing member function:
 *   QFlags<E>::setFlag (5.7+)
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)

template <typename E>
class QFlags_ : public QFlags<E> {
public:
    using QFlags<E>::QFlags;
    QFlags_(QFlags<E> &&f) : QFlags<E>(std::move(f)) {}

    QFlags_ &setFlag(E flag, bool on = true) {
        if (on)
            *this |= QFlags<E>(flag);
        else
            *this &= ~QFlags<E>(flag);
        return *this;
    }
};

#else

template <typename E>
using QFlags_ = QFlags<E>;

#endif

/* Missing member function:
 *   QFontMetrics::horizontalAdvance (5.11+)
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)

class QFontMetrics_ : public QFontMetrics {
public:
    using QFontMetrics::QFontMetrics;
    using QFontMetrics::operator=;
    QFontMetrics_(QFontMetrics &&fm) : QFontMetrics(std::move(fm)) {}

    int horizontalAdvance(QChar ch) const { return width(ch); }
    int horizontalAdvance(const QString &text, int len = -1) const { return width(text, len); }
};

#else

using QFontMetrics_ = QFontMetrics;

#endif


/* Missing static member function:
 *   QDateTime::currentSecsSinceEpoch (5.8+)
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)

class QDateTime_ : public QDateTime {
public:
    using QDateTime::QDateTime;
    QDateTime_(QDateTime &&dt) : QDateTime(std::move(dt)) {}

    static qint64 currentSecsSinceEpoch() { return currentMSecsSinceEpoch() / 1000; }
};

#else

using QDateTime_ = QDateTime;

#endif


/* Missing member function:
 *   QLocale::formattedDataSize (5.10+)
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)

class QLocale_ : public QLocale {
public:
    using QLocale::QLocale;
    QLocale_(QLocale &&l) : QLocale(std::move(l)) {}

    QString formattedDataSize(qint64 bytes, int precision = 2);
};

#else

using QLocale_ = QLocale;

#endif


/* Missing member function:
 *   QDir::isEmpty (5.9+)
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 9, 0)

class QDir_ : public QDir {
public:
    using QDir::QDir;
    QDir_(QDir &&d) : QDir(std::move(d)) {}

    bool isEmpty() const { return count() == 0; }
};

#else

using QDir_ = QDir;

#endif


/* Missing type:
 *   QUuid::StringFormat (5.11+)
 * Missing member function overload:
 *   QUuid::toString(StringFormat) (5.11+)
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)

class QUuid_ : QUuid {
public:
    enum StringFormat {
        WithBraces = 0,
        WithoutBraces = 1,
        Id128 = 3,
    };

public:
    using QUuid::QUuid;
    QUuid_(QUuid &&u) : QUuid(std::move(u)) {}

    QString toString(StringFormat mode);

public:
    static QUuid_ createUuid() { return QUuid::createUuid(); }
};

#else

using QUuid_ = QUuid;

#endif


/* Missing type:
 *   CreateProcessArguments        (5.7+)
 *   CreateProcessArgumentModifier (5.7+)
 * Missing member function:
 *   setCreateProcessArgumentsModifier (5.7+)
 *   startDetached                     (5.10+)
 *
 * `setCreateProcessArgumentsModifier` is so low level that a conformant implemetation
 * requires almost full rewrite of QProcess. Here we hook `CreateProcessW`.
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)

# if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
struct MinHookManager;
#endif

class QProcess_ : public QProcess {

public:
    using QProcess::QProcess;

    bool startDetached(qint64 *pid = nullptr);

# if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
public:
    struct CreateProcessArguments
    {
        const wchar_t *applicationName;
        wchar_t *arguments;
        SECURITY_ATTRIBUTES *processAttributes;
        SECURITY_ATTRIBUTES *threadAttributes;
        bool inheritHandles;
        unsigned long flags;
        void *environment;
        const wchar_t *currentDirectory;
        STARTUPINFO *startupInfo;
        PROCESS_INFORMATION *processInformation;
    };
    using CreateProcessArgumentModifier = std::function<void(CreateProcessArguments *)>;

public:
    void setCreateProcessArgumentsModifier(CreateProcessArgumentModifier modifier);
    void start(QIODevice::OpenMode mode = ReadWrite);

private:
    static BOOL detourCreateProcessW(LPCWSTR applicationName,
                                     LPWSTR commandLine,
                                     LPSECURITY_ATTRIBUTES processAttributes,
                                     LPSECURITY_ATTRIBUTES threadAttributes,
                                     BOOL inheritHandles,
                                     DWORD creationFlags,
                                     LPVOID environment,
                                     LPCWSTR currentDirectory,
                                     LPSTARTUPINFOW startupInfo,
                                     LPPROCESS_INFORMATION processInformation);

private:
    static decltype(&CreateProcessW) gpCreateProcessW;
    static CreateProcessArgumentModifier gProcessArgumentModifier;

    friend struct MinHookManager;
#endif

};

#else

using QProcess_ = QProcess;

#endif

}

#endif // COMPAT_H
