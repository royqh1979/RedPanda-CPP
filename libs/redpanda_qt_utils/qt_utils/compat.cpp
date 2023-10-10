#include "compat.h"

#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
# include <MinHook.h>
#endif

namespace Compat {

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)

QString QLocale_::formattedDataSize(qint64 bytes, int precision)
{
    if (-1024 < bytes && bytes < 1024)
        return toString(bytes) + " bytes";
    const QString units[] = {
        QStringLiteral("bytes"), QStringLiteral("KiB"), QStringLiteral("MiB"), QStringLiteral("GiB"),
        QStringLiteral("TiB"),   QStringLiteral("PiB"), QStringLiteral("EiB"),
    };
    int order = std::__lg(bytes > 0 ? bytes : -bytes) / 10;
    double value = double(bytes) / (1LL << (order * 10));
    return QString("%1 %2").arg(toString(value, 'f', precision)).arg(units[order]);
}

#endif


#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)

QString QUuid_::toString(StringFormat mode)
{
    QString repr = this->QUuid::toString();
    if (mode & 0x1)
        // remove braces
        repr = repr.mid(1, repr.length() - 2);
    if (mode & 0x2)
        // remove hyphen
        repr.remove('-');
    return repr;
}

#endif


#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)

bool QProcess_::startDetached(qint64 *pid)
{
    const QString &program = this->program();
    const QStringList &arguments = this->arguments();
    const QString &workingDirectory = this->workingDirectory();

    // calculate delta
    const QProcessEnvironment &expectedEnvironment = this->processEnvironment();
    const QProcessEnvironment &systemEnvironment = QProcessEnvironment::systemEnvironment();
    QSet<QString> addition = QSet<QString>::fromList(expectedEnvironment.keys()) - QSet<QString>::fromList(systemEnvironment.keys());
    QSet<QString> deletion = QSet<QString>::fromList(systemEnvironment.keys()) - QSet<QString>::fromList(expectedEnvironment.keys());
    QSet<QString> modification;
    for (const QString &key : expectedEnvironment.keys()) {
        if (systemEnvironment.contains(key) && systemEnvironment.value(key) != expectedEnvironment.value(key))
            modification.insert(key);
    }

    // set environment variables
    for (const QString &key : addition)
        qputenv(key.toLocal8Bit(), expectedEnvironment.value(key).toLocal8Bit());
    for (const QString &key : deletion)
        qunsetenv(key.toLocal8Bit());
    for (const QString &key : modification)
        qputenv(key.toLocal8Bit(), expectedEnvironment.value(key).toLocal8Bit());

    // run subprocess detached
    bool result = QProcess::startDetached(program, arguments, workingDirectory, pid);

    // restore environment variables
    for (const QString &key : addition)
        qunsetenv(key.toLocal8Bit());
    for (const QString &key : deletion)
        qputenv(key.toLocal8Bit(), systemEnvironment.value(key).toLocal8Bit());
    for (const QString &key : modification)
        qputenv(key.toLocal8Bit(), systemEnvironment.value(key).toLocal8Bit());

    return result;
}

# if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
struct MinHookManager {
    bool initialized;
    bool created;
    bool enabled;

    MinHookManager() {
        initialized = MH_OK == MH_Initialize();
        created = initialized && MH_OK == MH_CreateHook((void *)&CreateProcessW, (void *)&QProcess_::detourCreateProcessW, (void **)&QProcess_::gpCreateProcessW);
        enabled = created && MH_OK == MH_EnableHook((void *)&CreateProcessW);
    }
    ~MinHookManager() {
        if (enabled)
            MH_DisableHook((void *)&CreateProcessW);
        if (created)
            MH_RemoveHook((void *)&CreateProcessW);
        if (initialized)
            MH_Uninitialize();
    }

    operator bool() { return enabled; }
};

decltype(&CreateProcessW) QProcess_::gpCreateProcessW = nullptr;
QProcess_::CreateProcessArgumentModifier QProcess_::gProcessArgumentModifier{};

static MinHookManager gMinHookManager; // after `QProcess_::gpCreateProcessW` initialized

void QProcess_::setCreateProcessArgumentsModifier(CreateProcessArgumentModifier modifier)
{
    if (gMinHookManager) // dont remove: touch `gMinHookManager` so that the linker will keep it
        gProcessArgumentModifier = modifier;
}

void QProcess_::start(QIODevice::OpenMode mode)
{
    this->QProcess::start(mode);
    gProcessArgumentModifier = CreateProcessArgumentModifier{};
}

BOOL QProcess_::detourCreateProcessW(LPCWSTR applicationName,
                                     LPWSTR commandLine,
                                     LPSECURITY_ATTRIBUTES processAttributes,
                                     LPSECURITY_ATTRIBUTES threadAttributes,
                                     BOOL inheritHandles,
                                     DWORD creationFlags,
                                     LPVOID environment,
                                     LPCWSTR currentDirectory,
                                     LPSTARTUPINFOW startupInfo,
                                     LPPROCESS_INFORMATION processInformation)
{
    CreateProcessArguments args{applicationName,
                                commandLine,
                                processAttributes,
                                threadAttributes,
                                bool(inheritHandles),
                                creationFlags,
                                environment,
                                currentDirectory,
                                startupInfo,
                                processInformation};
    if (gProcessArgumentModifier)
        gProcessArgumentModifier(&args);
    return gpCreateProcessW(args.applicationName,
                            args.arguments,
                            args.processAttributes,
                            args.threadAttributes,
                            args.inheritHandles,
                            args.flags,
                            args.environment,
                            args.currentDirectory,
                            args.startupInfo,
                            args.processInformation);
}
# endif

#endif

}
