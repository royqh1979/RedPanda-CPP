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
#include "api.h"

#include <QtCore>
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QJsonValue>

#ifdef Q_OS_WINDOWS
# include <windows.h>
#endif

#include <lua/lua.hpp>

#include "utils.h"
#include "settings.h"
#include "thememanager.h"
#include "runtime.h"

#ifdef Q_OS_WINDOWS
// added in Windows 11 21H2, declare our version to support old SDKs.
constexpr int win32_flag_UserEnabled = 0x00000001;

// added in Windows 11 21H2, use GetProcAddress to detect availability.
using pGetMachineTypeAttributes_t = HRESULT (WINAPI *)(
	USHORT Machine, int *MachineTypeAttributes
);

// added in Windows 10 1709, use GetProcAddress to detect availability.
using pIsWow64GuestMachineSupported_t = HRESULT (WINAPI *)(
	USHORT WowGuestMachine, BOOL *MachineIsSupported
);
using pIsWow64Process2_t = BOOL (WINAPI *)(
	HANDLE hProcess, USHORT *pProcessMachine, USHORT *pNativeMachine
);
#endif

// C_Debug.debug(string) -> ()
extern "C" int luaApi_Debug_debug(lua_State *L) noexcept {
    QString info = AddOn::RaiiLuaState::fetchString(L, 1);
    qDebug() << info;
    return 0;
}

// C_Desktop.desktopEnvironment() -> string
extern "C" int luaApi_Desktop_desktopEnvironment(lua_State *L) noexcept {
#if defined(Q_OS_WIN32)
    // exclude WinRT intentionally
    lua_pushliteral(L, "windows");
#elif defined(Q_OS_MACOS)
    lua_pushliteral(L, "macos");
#elif (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_HURD) || defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD) || defined(Q_OS_OPENBSD) || defined(Q_OS_SOLARIS)
    // desktops that follows to freedesktop.org specs, i.e. GNU/Linux, GNU/Hurd, BSD, Solaris (illumos)
    lua_pushliteral(L, "xdg");
#else
    lua_pushliteral(L, "unknown");
#endif
    return 1;
}

// C_Desktop.language() -> string
extern "C" int luaApi_Desktop_language(lua_State *L) noexcept {
    QString lang = pSettings->environment().language();
    AddOn::RaiiLuaState::push(L, lang);
    return 1;
}

// C_Desktop.qtStyleList() -> [string]
extern "C" int luaApi_Desktop_qtStyleList(lua_State *L) noexcept {
    QStringList styles = QStyleFactory::keys();
    AddOn::RaiiLuaState::push(L, styles);
    return 1;
}

// C_Desktop.systemAppMode() -> string
extern "C" int luaApi_Desktop_systemAppMode(lua_State *L) noexcept {
    if (AppTheme::isSystemInDarkMode())
        lua_pushliteral(L, "dark");
    else
        lua_pushliteral(L, "light");
    return 1;
}

// C_Desktop.systemStyle() -> string
extern "C" int luaApi_Desktop_systemStyle(lua_State *L) noexcept {
    AddOn::RaiiLuaState::push(L, AppTheme::initialStyle());
    return 1;
}

// C_FileSystem.exists(string) -> bool
extern "C" int luaApi_FileSystem_exists(lua_State *L) noexcept {
    QString path = AddOn::RaiiLuaState::fetchString(L, 1);
    QFileInfo fileInfo(path);
    AddOn::RaiiLuaState::push(L, fileInfo.exists());
    return 1;
}

// C_FileSystem.isExecutable(string) -> bool
extern "C" int luaApi_FileSystem_isExecutable(lua_State *L) noexcept {
    QString path = AddOn::RaiiLuaState::fetchString(L, 1);
    QFileInfo fileInfo(path);
    bool isExecutable = fileInfo.isFile() && fileInfo.isReadable() && fileInfo.isExecutable();
    AddOn::RaiiLuaState::push(L, isExecutable);
    return 1;
}

// C_System.appArch() -> string
extern "C" int luaApi_System_appArch(lua_State *L) noexcept {
    AddOn::RaiiLuaState::push(L, appArch());
    return 1;
}

// C_System.appDir() -> string
extern "C" int luaApi_System_appDir(lua_State *L) noexcept {
    QString appDir = pSettings->dirs().appDir();
    AddOn::RaiiLuaState::push(L, appDir);
    return 1;
}

// C_System.appLibexecDir() -> string
extern "C" int luaApi_System_appLibexecDir(lua_State *L) noexcept {
    QString appLibexecDir = pSettings->dirs().appLibexecDir();
    AddOn::RaiiLuaState::push(L, appLibexecDir);
    return 1;
}

// C_System.appResourceDir() -> string
extern "C" int luaApi_System_appResourceDir(lua_State *L) noexcept {
    QString appResourceDir = pSettings->dirs().appResourceDir();
    AddOn::RaiiLuaState::push(L, appResourceDir);
    return 1;
}

// C_System.osArch() -> string
extern "C" int luaApi_System_osArch(lua_State *L) noexcept {
    AddOn::RaiiLuaState::push(L, osArch());
    return 1;
}

static QString qtArchitectureNormalization(const QString &arch) {
    // adjust QEMU user emulation arches to match QSysInfo::currentCpuArchitecture
    if (arch == "aarch64")
        return "arm64";
    if (arch.startsWith("ppc"))
        return "power" + arch.mid(3);
    if (arch == "sparc64")
        return "sparcv9";
    return arch;
}

// C_System.supportedAppArchList() -> [string]
extern "C" int luaApi_System_supportedAppArchList(lua_State *L) noexcept {
#ifdef Q_OS_WINDOWS
    QSet<QString> arches;
    pGetMachineTypeAttributes_t pGetMachineTypeAttributes =
        reinterpret_cast<pGetMachineTypeAttributes_t>(GetProcAddress(
            GetModuleHandleW(L"kernel32"), "GetMachineTypeAttributes"));
    if (pGetMachineTypeAttributes) {
        // the direct way
        int result;
        if (pGetMachineTypeAttributes(IMAGE_FILE_MACHINE_I386, &result) == 0 && result & win32_flag_UserEnabled)
            arches.insert("i386");
        if (pGetMachineTypeAttributes(IMAGE_FILE_MACHINE_AMD64, &result) == 0 && result & win32_flag_UserEnabled)
            arches.insert("x86_64");
        if (pGetMachineTypeAttributes(IMAGE_FILE_MACHINE_ARMNT, &result) == 0 && result & win32_flag_UserEnabled)
            arches.insert("arm");
        if (pGetMachineTypeAttributes(IMAGE_FILE_MACHINE_ARM64, &result) == 0 && result & win32_flag_UserEnabled)
            arches.insert("arm64");
    } else {
        pIsWow64GuestMachineSupported_t pIsWow64GuestMachineSupported =
            reinterpret_cast<pIsWow64GuestMachineSupported_t>(GetProcAddress(
                GetModuleHandleW(L"kernel32"), "IsWow64GuestMachineSupported"));
        pIsWow64Process2_t pIsWow64Process2 = 
        reinterpret_cast<pIsWow64Process2_t>(GetProcAddress(
            GetModuleHandleW(L"kernel32"), "IsWow64Process2"));
        if (pIsWow64GuestMachineSupported && pIsWow64Process2) {
            // recent Windows 10, native + WoW64 arches
            // IsWow64Process2 returns real native architecture under xtajit, while GetNativeSystemInfo does not.
            USHORT processMachineResult, nativeMachineResult;
            if (pIsWow64Process2(GetCurrentProcess(), &processMachineResult, &nativeMachineResult))
                switch (nativeMachineResult) {
                case IMAGE_FILE_MACHINE_I386:
                    arches.insert("i386");
                    break;
                case IMAGE_FILE_MACHINE_AMD64:
                    arches.insert("x86_64");
                    break;
                case IMAGE_FILE_MACHINE_ARM64:
                    arches.insert("arm64");
                    break;
                }
            BOOL wow64SupportResult;
            if (pIsWow64GuestMachineSupported(IMAGE_FILE_MACHINE_I386, &wow64SupportResult) == S_OK && wow64SupportResult)
                arches.insert("i386");
            if (pIsWow64GuestMachineSupported(IMAGE_FILE_MACHINE_ARMNT, &wow64SupportResult) == S_OK && wow64SupportResult)
                arches.insert("arm");
        } else {
            // legacy Windows, hardcode
            SYSTEM_INFO si;
            GetNativeSystemInfo(&si);
            switch (si.wProcessorArchitecture) {
            case PROCESSOR_ARCHITECTURE_INTEL:
                arches.insert("i386");
                break;
            case PROCESSOR_ARCHITECTURE_AMD64:
                arches.insert("i386");
                arches.insert("x86_64");
                break;
            case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64: // undocumented but found in Qt source
            case PROCESSOR_ARCHITECTURE_IA64:
                // does Red Panda C++ run on IA-64?
                arches.insert("i386");
                arches.insert("ia64");
                break;
            }
        }
    }
    QStringList result{arches.begin(), arches.end()};
    AddOn::RaiiLuaState::push(L, result);
    return 1;
#elif defined(Q_OS_MACOS)
    QStringList result;
    if (QSysInfo::currentCpuArchitecture() == "arm64")
        result = QStringList{"arm64", "x86_64"};
    else if (QVersionNumber::fromString(QSysInfo::kernelVersion()) >= QVersionNumber(19, 0))
        // macOS 10.15+
        result = QStringList{"x86_64"};
    else
        result = QStringList{"i386", "x86_64"};
    AddOn::RaiiLuaState::push(L, result);
    return 1;
#else
    QSet<QString> arches;
    arches.insert(QSysInfo::currentCpuArchitecture());
    arches.insert(QSysInfo::buildCpuArchitecture());

    // read binfmt_misc registry
    QDir binfmt_misc("/proc/sys/fs/binfmt_misc");
    if (binfmt_misc.exists()) {
        QFileInfoList entries = binfmt_misc.entryInfoList(QDir::Files);
        for (const QFileInfo &entry : entries) {
            if (entry.fileName().startsWith("qemu-")) {
                QString arch = entry.fileName().mid(5);
                arches.insert(qtArchitectureNormalization(arch));
            }
        }
    }

    // workaround for Debian 10 Qt 5.11, better to be
    //   QStringList result{arches.begin(), arches.end()};
    QStringList result = arches.toList();
    AddOn::RaiiLuaState::push(L, result);
    return 1;
#endif
}

#ifdef Q_OS_WINDOWS
// C_System.readRegistry(string, string) -> string|nil
extern "C" int luaApi_System_readRegistry(lua_State *L) noexcept
{
    QString subKey = AddOn::RaiiLuaState::fetchString(L, 1);
    QString name = AddOn::RaiiLuaState::fetchString(L, 2);
    QString value;
    bool success = readRegistry(HKEY_CURRENT_USER, subKey, name, value);
    if (success)
        AddOn::RaiiLuaState::push(L, value);
    else {
        success = readRegistry(HKEY_LOCAL_MACHINE, subKey, name, value);
        if (success)
            AddOn::RaiiLuaState::push(L, value);
        else
            AddOn::RaiiLuaState::push(L, nullptr);
    }
    return 1;
}
#endif

// C_Util.format(string, ...) -> string
extern "C" int luaApi_Util_format(lua_State *L) noexcept
{
    QString s = AddOn::RaiiLuaState::fetchString(L, 1);
    int nArgs = lua_gettop(L);
    for (int i = 2; i <= nArgs; ++i) {
        QJsonValue arg = AddOn::RaiiLuaState::fetch(L, i);
        s = s.arg(arg.toString());
    }
    AddOn::RaiiLuaState::push(L, s);
    return 1;
}
