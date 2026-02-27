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

#ifndef UTILS_PE_H
#define UTILS_PE_H

#include <QtGlobal>

#ifdef Q_OS_WIN

#include <QString>

struct PortableExecutable
{
    QString path;

    PortableExecutable(const QString &path);
    ~PortableExecutable() = default;

    bool isWin32GuiApp();
    bool isUnicodeAware();
    bool hasUtf8Manifest();
    bool isUtf8();

    static bool osSupportsUtf8Manifest();
};

#else // Q_OS_WIN

struct PortableExecutable
{
    template <typename... Ts>
    constexpr PortableExecutable(Ts&&...) {}

    constexpr bool isWin32GuiApp() { return false; }
};

#endif // Q_OS_WIN

#endif // UTILS_PE_H
