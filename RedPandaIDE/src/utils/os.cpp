/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#include "os.h"
#include <qt_utils/utils.h>
#include <QApplication>
#include <QDomDocument>
#include <QOperatingSystemVersion>
#include <QSysInfo>

#ifdef Q_OS_WIN
using pIsWow64Process2_t = BOOL (WINAPI *)(
    HANDLE hProcess, USHORT *pProcessMachine, USHORT *pNativeMachine
);
#endif

#ifdef Q_OS_WIN
static bool gIsGreenEdition = true;
static bool gIsGreenEditionInited = false;

bool isGreenEdition()
{
    if (!gIsGreenEditionInited) {
        QString appPath = QApplication::instance()->applicationDirPath();
        appPath = excludeTrailingPathDelimiter(localizePath(appPath));
        QString keyString = R"(Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++)";
        QString systemInstallPath;
        readRegistry(HKEY_LOCAL_MACHINE, keyString, "InstallLocation", systemInstallPath);
        if (systemInstallPath.isEmpty()) {
            readRegistry(HKEY_LOCAL_MACHINE, keyString, "UninstallString", systemInstallPath);
            systemInstallPath = excludeTrailingPathDelimiter(extractFileDir(systemInstallPath));
        } else
            systemInstallPath = excludeTrailingPathDelimiter(systemInstallPath);
        QString userInstallPath;
        readRegistry(HKEY_CURRENT_USER, keyString, "InstallLocation", userInstallPath);
        if (userInstallPath.isEmpty()) {
            readRegistry(HKEY_CURRENT_USER, keyString, "UninstallString", userInstallPath);
            userInstallPath = excludeTrailingPathDelimiter(extractFileDir(userInstallPath));
        } else
            userInstallPath = excludeTrailingPathDelimiter(userInstallPath);
        systemInstallPath = localizePath(systemInstallPath);
        userInstallPath = localizePath(userInstallPath);
        gIsGreenEdition = appPath.compare(systemInstallPath, Qt::CaseInsensitive) != 0 &&
                appPath.compare(userInstallPath, Qt::CaseInsensitive) != 0;
        gIsGreenEditionInited = true;
    }
    return gIsGreenEdition;
}
#endif

#ifdef Q_OS_WIN
bool readRegistry(HKEY key,const QString& subKey, const QString& name, QString& value) {
    LONG result;
    HKEY hkey;
    result = RegOpenKeyExW(key, subKey.toStdWString().c_str(), 0, KEY_READ, &hkey);
    if (result != ERROR_SUCCESS)
        return false;

    DWORD dataType;
    DWORD dataSize;
    result = RegQueryValueExW(hkey, name.toStdWString().c_str(), NULL, &dataType, NULL, &dataSize);
    if (result != ERROR_SUCCESS || (dataType != REG_SZ && dataType != REG_MULTI_SZ)) {
        RegCloseKey(hkey);
        return false;
    }

    wchar_t * buffer = new wchar_t[dataSize / sizeof(wchar_t) + 10];
    result = RegQueryValueExW(hkey, name.toStdWString().c_str(), NULL, &dataType, (LPBYTE)buffer, &dataSize);
    RegCloseKey(hkey);
    if (result != ERROR_SUCCESS) {
        delete[] buffer;
        return false;
    }

    value = QString::fromWCharArray(buffer);
    delete [] buffer;
    return true;
}
#endif

ExternalResource::ExternalResource() {
    Q_INIT_RESOURCE(qsynedit_qmake_qmake_qm_files);
    Q_INIT_RESOURCE(redpanda_qt_utils_qmake_qmake_qm_files);
}

ExternalResource::~ExternalResource() {
    Q_CLEANUP_RESOURCE(qsynedit_qmake_qmake_qm_files);
    Q_CLEANUP_RESOURCE(redpanda_qt_utils_qmake_qmake_qm_files);
}

#ifdef Q_OS_WINDOWS
bool applicationHasUtf8Manifest(const wchar_t *path)
{
    auto module = resourcePointer(LoadLibraryExW(path, nullptr, LOAD_LIBRARY_AS_DATAFILE), &FreeLibrary);
    if (!module)
        return false;
    HRSRC resInfo = FindResourceW(
        module.get(),
        MAKEINTRESOURCEW(1) /* CREATEPROCESS_MANIFEST_RESOURCE_ID */,
        MAKEINTRESOURCEW(24) /* RT_MANIFEST */);
    if (!resInfo)
        return false;
    auto res = resourcePointer(LoadResource(module.get(), resInfo), &FreeResource);
    if (!res)
        return false;
    char *data = (char *)LockResource(res.get());
    DWORD size = SizeofResource(module.get(), resInfo);
    QByteArray manifest(data, size);
    QDomDocument doc;
    if (!doc.setContent(manifest))
        return false;
    QDomNodeList acpNodes = doc.elementsByTagName("activeCodePage");
    if (acpNodes.isEmpty())
        return false;
    QDomElement acpNode = acpNodes.item(0).toElement();
    // case sensitive
    // ref. https://devblogs.microsoft.com/oldnewthing/20220531-00/?p=106697
    return acpNode.text() == QStringLiteral("UTF-8");
}

bool osSupportsUtf8Manifest()
{
    // since Windows 10 1903 (accurate build number unknown)
    // ref. https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
    return QOperatingSystemVersion::current().microVersion() >= 18362;
}

bool applicationIsUtf8(const QString &path)
{
    static bool systemIsUtf8 = GetACP() == CP_UTF8;
    if (systemIsUtf8)
        return true;
    if (!fileExists(path))
        return false;
    return osSupportsUtf8Manifest() && applicationHasUtf8Manifest((wchar_t *)path.constData());
}
#endif

QString appArch()
{
    return QSysInfo::buildCpuArchitecture();
}

QString osArch()
{
#ifdef Q_OS_WINDOWS
    pIsWow64Process2_t pIsWow64Process2 =
        reinterpret_cast<pIsWow64Process2_t>(GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process2"));
    if (pIsWow64Process2) {
        // IsWow64Process2 returns real native architecture under xtajit, while GetNativeSystemInfo does not.
        USHORT processMachineResult, nativeMachineResult;
        if (pIsWow64Process2(GetCurrentProcess(), &processMachineResult, &nativeMachineResult))
            switch (nativeMachineResult) {
            case IMAGE_FILE_MACHINE_I386:
                return "i386";
            case IMAGE_FILE_MACHINE_AMD64:
                return "x86_64";
            case IMAGE_FILE_MACHINE_ARM64:
                return "arm64";
            }
        return QSysInfo::currentCpuArchitecture();
    } else
        return QSysInfo::currentCpuArchitecture();
#else
    return QSysInfo::currentCpuArchitecture();
#endif
}

bool programIsWin32GuiApp(const QString & filename)
{
#ifdef Q_OS_WIN
    bool result = false;
    HANDLE handle = CreateFileW(filename.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle != INVALID_HANDLE_VALUE) {
        result = [handle] {
            IMAGE_DOS_HEADER dos_header;
            DWORD signature;
            DWORD bytesread;
            IMAGE_FILE_HEADER pe_header;
            IMAGE_OPTIONAL_HEADER opt_header;

            ReadFile(handle, &dos_header, sizeof(dos_header), &bytesread, NULL);
            if (bytesread != sizeof(dos_header))
                return false;
            if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
                return false;

            SetFilePointer(handle, dos_header.e_lfanew, NULL, 0);
            ReadFile(handle, &signature, sizeof(signature), &bytesread, NULL);
            if (bytesread != sizeof(signature))
                return false;
            if (signature != IMAGE_NT_SIGNATURE)
                return false;

            ReadFile(handle, &pe_header, sizeof(pe_header), &bytesread, NULL);
            if (bytesread != sizeof(pe_header))
                return false;

            ReadFile(handle, &opt_header, sizeof(opt_header), &bytesread, NULL);
            if (bytesread != sizeof(opt_header))
                return false;
            if (opt_header.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC && opt_header.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                return false;

            return opt_header.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI;
        } ();
    }
    CloseHandle(handle);
    return result;
#else
    return false;
#endif
}
