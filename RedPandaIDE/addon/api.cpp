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

#include <lua/lua.hpp>

#include "settings.h"
#include "thememanager.h"
#include "runtime.h"

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
