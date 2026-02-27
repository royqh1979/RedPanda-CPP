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

#include "pe.h"

#include <QtGlobal>

#ifdef Q_OS_WIN

#include <QDomDocument>
#include <QOperatingSystemVersion>
#include <QString>

#if QT_VERSION_MAJOR >= 6
#include <QByteArrayView>
#else
#include <QByteArray>
#endif

#include <string.h>

#include <windows.h>
#include <dbghelp.h>

PortableExecutable::PortableExecutable(const QString &path) : path(path)
{
}

bool PortableExecutable::isWin32GuiApp()
{
    HANDLE hFile = CreateFileW(
        (const wchar_t *)path.constData(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    HANDLE hMapping = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    CloseHandle(hFile);
    if (!hMapping)
        return false;

    void *base = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMapping);
    if (!base)
        return false;

    bool result = false;
    do {
        IMAGE_NT_HEADERS *ntHeader = ImageNtHeader(base);
        bool is64Bit = ntHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;

        if (is64Bit) {
            auto *optHeader64 = (IMAGE_OPTIONAL_HEADER64 *)&ntHeader->OptionalHeader;
            result = optHeader64->Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI;
        } else {
            auto *optHeader32 = (IMAGE_OPTIONAL_HEADER32 *)&ntHeader->OptionalHeader;
            result = optHeader32->Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI;
        }
    } while (false);

    UnmapViewOfFile(base);
    return result;
}

bool PortableExecutable::isUnicodeAware()
{
    HANDLE hFile = CreateFileW(
        (const wchar_t *)path.constData(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    HANDLE hMapping = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    CloseHandle(hFile);
    if (!hMapping)
        return false;

    void *base = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMapping);
    if (!base)
        return false;

    bool result = false;
    IMAGE_SECTION_HEADER *lastRvaSection = nullptr;
    do {
        IMAGE_NT_HEADERS *ntHeader = ImageNtHeader(base);
        bool is64Bit = ntHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;

        ULONG importDirSize;
        auto *importDesc = (IMAGE_IMPORT_DESCRIPTOR *)ImageDirectoryEntryToData(
            base, FALSE, IMAGE_DIRECTORY_ENTRY_IMPORT, &importDirSize);
        if (!importDesc || importDirSize == 0)
            break;

        for (; importDesc && importDesc->OriginalFirstThunk; importDesc++) {
            auto *dllNamePtr = (char *)ImageRvaToVa(ntHeader, base, importDesc->Name, &lastRvaSection);
            if (!dllNamePtr)
                continue;

            // POSIX-style, always UTF-8
            if (_stricmp(dllNamePtr, "cygwin1.dll") == 0 ||
                _stricmp(dllNamePtr, "msys-2.0.dll") == 0) {
                result = true;
                break;
            }

            // If the program is Unicode-aware (gets wargv somehow), it's very likely UTF-8
            const char *wargvFunction = nullptr;
            if (_stricmp(dllNamePtr, "kernel32.dll") == 0)
                wargvFunction = "GetCommandLineW";
            else if (_stricmp(dllNamePtr, "msvcrt.dll") == 0)
                wargvFunction = "__wgetmainargs";
            else if (_stricmp(dllNamePtr, "api-ms-win-crt-runtime-l1-1-0.dll") == 0)
                wargvFunction = "_configure_wide_argv";
            else
                continue;

            void *thunkData = ImageRvaToVa(ntHeader, base, importDesc->OriginalFirstThunk, &lastRvaSection);
            if (!thunkData)
                continue;

            if (is64Bit) {
                for (const uint64_t *thunk = (const uint64_t *)thunkData; *thunk; thunk++) {
                    // import by ordinal
                    if (*thunk & 0x8000'0000'0000'0000ULL)
                        continue;

                    uint32_t nameOffset = *thunk & 0x7fff'ffffUL;
                    auto *importByName = (IMAGE_IMPORT_BY_NAME *)
                        ImageRvaToVa(ntHeader, base, nameOffset, &lastRvaSection);
                    if (!importByName)
                        continue;
                    if (strcmp(wargvFunction, importByName->Name) == 0) {
                        result = true;
                        break;
                    }
                }
            } else {
                for (const uint32_t *thunk = (const uint32_t *)thunkData; *thunk; thunk++) {
                    // import by ordinal
                    if (*thunk & 0x8000'0000UL)
                        continue;

                    uint32_t nameOffset = *thunk & 0x7fff'ffffUL;
                    auto *importByName = (IMAGE_IMPORT_BY_NAME *)
                        ImageRvaToVa(ntHeader, base, nameOffset, &lastRvaSection);
                    if (!importByName)
                        continue;
                    if (strcmp(wargvFunction, importByName->Name) == 0) {
                        result = true;
                        break;
                    }
                }
            }

            if (result)
                break;
        }
    } while (false);

    UnmapViewOfFile(base);
    return result;
}

bool PortableExecutable::hasUtf8Manifest()
{
    HMODULE hModule = LoadLibraryExW(
        (const wchar_t *)path.constData(),
        nullptr,
        LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule || hModule == INVALID_HANDLE_VALUE)
        return false;

    HRSRC resInfo = FindResourceW(
        hModule,
        MAKEINTRESOURCEW(1) /* CREATEPROCESS_MANIFEST_RESOURCE_ID */,
        MAKEINTRESOURCEW(24) /* RT_MANIFEST */);
    if (!resInfo)
        return false;
    HGLOBAL res = LoadResource(hModule, resInfo);
    if (!res)
        return false;
    char *data = (char *)LockResource(res);
    DWORD size = SizeofResource(hModule, resInfo);

    bool result = false;
    do {
        QDomDocument doc;
#if QT_VERSION_MAJOR >= 6
        if (!doc.setContent(QByteArrayView(data, size)))
            break;
#else
        if (!doc.setContent(QByteArray(data, size)))
            break;
#endif
        QDomNodeList acpNodes = doc.elementsByTagName("activeCodePage");
        if (acpNodes.isEmpty())
            break;
        QDomElement acpNode = acpNodes.item(0).toElement();
        // case sensitive
        // ref. https://devblogs.microsoft.com/oldnewthing/20220531-00/?p=106697
        result = (acpNode.text() == QStringLiteral("UTF-8"));
    } while (false);

    FreeResource(res);
    return result;
}

bool PortableExecutable::isUtf8()
{
    if (GetACP() == CP_UTF8)
        return true;

    return (osSupportsUtf8Manifest() && hasUtf8Manifest()) ||
           isUnicodeAware();
}

bool PortableExecutable::osSupportsUtf8Manifest()
{
    // since Windows 10 1903 (accurate build number unknown)
    // ref. https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
    return QOperatingSystemVersion::current().microVersion() >= 18362;
}

#endif
