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
#ifndef UTILS_OS_H
#define UTILS_OS_H
#include <QString>
#include <memory>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

bool programIsWin32GuiApp(const QString& filename);

#ifdef Q_OS_WIN
bool isGreenEdition();
#else
constexpr bool isGreenEdition() { return false; }
#endif

#ifdef Q_OS_WIN
bool readRegistry(HKEY key, const QString& subKey, const QString& name, QString& value);
#endif

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

QString appArch();
QString osArch();

#endif // UTILS_OS_H
