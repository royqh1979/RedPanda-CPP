#ifndef SYSTEMCONSTS_H
#define SYSTEMCONSTS_H

#include <QStringList>

#ifdef Q_OS_WIN
#define APP_SETTSINGS_FILENAME "redpandacpp.ini"
#define GCC_PROGRAM     "gcc.exe"
#define GPP_PROGRAM     "g++.exe"
#define GDB_PROGRAM     "gdb.exe"
#define GDB32_PROGRAM   "gdb32.exe"
#define MAKE_PROGRAM    "mingw32-make.exe"
#define WINDRES_PROGRAM "windres.exe"
#define GPROF_PROGRAM   "gprof.exe"
#define CLEAN_PROGRAM   "rm.exe"
#define CPP_PROGRAM     "cpp.exe"
#else
#error "Only support windows now!"
#endif

#ifdef Q_OS_WIN
#   define PATH_SENSITIVITY Qt::CaseInsensitive
#   define NULL_FILE       "NUL"
#   define EXECUTABE_EXT   "exe"
#elif Q_OS_LINUX
#   define PATH_SENSITIVITY Qt::CaseSensitive
#   define NULL_FILE       "/dev/null"
#   define EXECUTABE_EXT   ""
#else
#error "Only support windows and linux now!"
#endif

#define DEVCPP_VERSION "0.0.5"

class SystemConsts
{
public:
    SystemConsts();
    const QStringList& defaultFileFilters() const noexcept;
    const QString& defaultCFileFilter() const noexcept;
    const QString& defaultCPPFileFilter() const noexcept;
    const QString& defaultAllFileFilter() const noexcept;
    void addDefaultFileFilter(const QString& name, const QString& fileExtensions);
private:
    QStringList mDefaultFileFilters;
};

extern SystemConsts* pSystemConsts;
#endif // SYSTEMCONSTS_H
