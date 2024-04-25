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
#ifndef SYSTEMCONSTS_H
#define SYSTEMCONSTS_H

#include <QStringList>

#define APP_SETTSINGS_FILENAME "redpandacpp.ini"
#ifdef Q_OS_WIN
#define CONSOLE_PAUSER  "consolepauser.exe"
#define GCC_PROGRAM     "gcc.exe"
#define GPP_PROGRAM     "g++.exe"
#define GDB_PROGRAM     "gdb.exe"
#define GDB_SERVER_PROGRAM     "gdbserver.exe"
#define GDB32_PROGRAM   "gdb32.exe"
#define MAKE_PROGRAM    "mingw32-make.exe"
#define WINDRES_PROGRAM "windres.exe"
#define CLEAN_PROGRAM   "del /q /f"
#define CD_PROGRAM   "cd /d"
#define CPP_PROGRAM     "cpp.exe"
#define GIT_PROGRAM     "git.exe"
#define CLANG_PROGRAM   "clang.exe"
#define CLANG_CPP_PROGRAM   "clang++.exe"
#define LLDB_MI_PROGRAM   "lldb-mi.exe"
#define LLDB_SERVER_PROGRAM   "lldb-server.exe"
#define SDCC_PROGRAM   "sdcc.exe"
#define PACKIHX_PROGRAM   "packihx.exe"
#define MAKEBIN_PROGRAM   "makebin.exe"
#define ASTYLE_PROGRAM     "astyle.exe"
#else // Unix
#define CONSOLE_PAUSER  "consolepauser"
#define GCC_PROGRAM     "gcc"
#define GPP_PROGRAM     "g++"
#define GDB_PROGRAM     "gdb"
#define GDB_SERVER_PROGRAM     "gdbserver"
#define GDB32_PROGRAM   "gdb32"
#define MAKE_PROGRAM    "make"
#define WINDRES_PROGRAM ""
#define GPROF_PROGRAM   "gprof"
#define CLEAN_PROGRAM   "rm -rf"
#define CD_PROGRAM   "cd"
#define CPP_PROGRAM     "cpp"
#define GIT_PROGRAM     "git"
#define CLANG_PROGRAM   "clang"
#define CLANG_CPP_PROGRAM   "clang++"
#define LLDB_MI_PROGRAM   "lldb-mi"
#define LLDB_SERVER_PROGRAM   "lldb-server"
#define SDCC_PROGRAM   "sdcc"
#define PACKIHX_PROGRAM   "packihx"
#define MAKEBIN_PROGRAM   "makebin"
#define ASTYLE_PROGRAM     "astyle"
#endif

#define DEV_PROJECT_EXT "dev"
#define PROJECT_BOOKMARKS_EXT "bookmarks"
#define PROJECT_DEBUG_EXT "debug"
#define RC_EXT "rc"
#define RES_EXT "res"
#define H_EXT "h"
#define OBJ_EXT "o"
#define LST_EXT "lst"
#define DEF_EXT "def"
#define LIB_EXT "a"
#define GCH_EXT "gch"
#define TEMPLATE_EXT "template"
#define TEMPLATE_INFO_FILE "info.template"
#define DEV_INTERNAL_OPEN "$__DEV_INTERNAL_OPEN"
#define DEV_LASTOPENS_FILE "lastopens.json"
#define DEV_SYMBOLUSAGE_FILE  "symbolusage.json"
#define DEV_CODESNIPPET_FILE  "codesnippets.json"
#define DEV_NEWFILETEMPLATES_FILE "newfiletemplate.txt"
#define DEV_NEWCFILETEMPLATES_FILE "newcfiletemplate.txt"
#define DEV_NEWGASFILETEMPLATES_FILE "newgasfiletemplate.txt"
#define DEV_AUTOLINK_FILE "autolink.json"
#define DEV_SHORTCUT_FILE "shortcuts.json"
#define DEV_TOOLS_FILE "tools.json"
#define DEV_BOOKMARK_FILE "bookmarks.json"
#define DEV_DEBUGGER_FILE "debugger.json"
#define DEV_HISTORY_FILE "history.json"
#define DEV_PROBLEM_SET_FILE "problemset.json"


#ifdef Q_OS_WIN
#   define PATH_SENSITIVITY Qt::CaseInsensitive
#   define PATH_SEPARATOR   ";"
#   define LINE_BREAKER     "\r\n"
#   define NULL_FILE       "NUL"
#   define DEFAULT_EXECUTABLE_SUFFIX   "exe"
#   define DEFAULT_PREPROCESSING_SUFFIX "p"
#   define DEFAULT_COMPILATION_SUFFIX "s"
#   define DEFAULT_ASSEMBLING_SUFFIX "o"
#   define STATIC_LIB_EXT   "a"
#   define DYNAMIC_LIB_EXT   "dll"
#   define MAKEFILE_NAME    "makefile.win"
#   define XMAKEFILE_NAME    "xmake.lua"
#   define ALL_FILE_WILDCARD "*.*"
#else // Unix
#   define PATH_SENSITIVITY Qt::CaseSensitive
#   define PATH_SEPARATOR   ":"
#   define LINE_BREAKER     "\n"
#   define NULL_FILE       "/dev/null"
#   define DEFAULT_EXECUTABLE_SUFFIX   ""
#   define DEFAULT_PREPROCESSING_SUFFIX "p"
#   define DEFAULT_COMPILATION_SUFFIX "s"
#   define DEFAULT_ASSEMBLING_SUFFIX "o"
#   define STATIC_LIB_EXT   "a"
#   define DYNAMIC_LIB_EXT   "d"
#   define MAKEFILE_NAME    "makefile"
#   define XMAKEFILE_NAME    "xmake.lua"
#   define ALL_FILE_WILDCARD "*"
#endif

#define SDCC_IHX_SUFFIX "ihx"
#define SDCC_BIN_SUFFIX "bin"
#define SDCC_HEX_SUFFIX "hex"
#define SDCC_REL_SUFFIX "rel"

class SystemConsts
{
public:
    SystemConsts();
    const QStringList& defaultFileFilters() const noexcept;
    QString defaultCFileFilter() const noexcept;
    QString defaultCPPFileFilter() const noexcept;
    QString defaultAllFileFilter() const noexcept;
    QString executableFileFilter() const noexcept;
    QString fileFilterFor(const QString& suffix);
    void addDefaultFileFilter(const QString& name, const QString& fileExtensions);
    const QStringList &iconFileFilters() const;
    const QString& iconFileFilter() const;
    void setIconFileFilters(const QStringList &newIconFileFilters);

    const QStringList &codecNames() const;

    const QStringList &defaultFileNameFilters() const;

private:
    void addFileFilter(QStringList& filters, const QString& name, const QString& fileExtensions);
    QStringList mDefaultFileFilters;
    QStringList mIconFileFilters;
    QStringList mDefaultFileNameFilters;
    QStringList mCodecNames;
};

extern SystemConsts* pSystemConsts;
#endif // SYSTEMCONSTS_H
