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
#ifndef ADDON_API_H
#define ADDON_API_H

struct lua_State;

extern "C" int luaApi_Debug_debug(lua_State *L) noexcept;
extern "C" int luaApi_Debug_messageBox(lua_State *L) noexcept;

extern "C" int luaApi_Desktop_desktopEnvironment(lua_State *L) noexcept;
extern "C" int luaApi_Desktop_language(lua_State *L) noexcept;
extern "C" int luaApi_Desktop_qtStyleList(lua_State *L) noexcept;
extern "C" int luaApi_Desktop_systemAppMode(lua_State *L) noexcept;
extern "C" int luaApi_Desktop_systemStyle(lua_State *L) noexcept;

extern "C" int luaApi_FileSystem_exists(lua_State *L) noexcept;
extern "C" int luaApi_FileSystem_isExecutable(lua_State *L) noexcept;
extern "C" int luaApi_FileSystem_matchFiles(lua_State *L) noexcept;

extern "C" int luaApi_System_appArch(lua_State *L) noexcept;
extern "C" int luaApi_System_appDir(lua_State *L) noexcept;
extern "C" int luaApi_System_appLibexecDir(lua_State *L) noexcept;
extern "C" int luaApi_System_appResourceDir(lua_State *L) noexcept;
extern "C" int luaApi_System_osArch(lua_State *L) noexcept;
extern "C" int luaApi_System_popen(lua_State *L) noexcept;
extern "C" int luaApi_System_supportedAppArchList(lua_State *L) noexcept;
#ifdef Q_OS_WINDOWS
extern "C" int luaApi_System_readRegistry(lua_State *L) noexcept;
#endif

extern "C" int luaApi_Util_format(lua_State *L) noexcept;

#endif // ADDON_API_H
