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

extern "C" int luaApi_Desktop_desktopEnvironment(lua_State *L) noexcept;
extern "C" int luaApi_Desktop_language(lua_State *L) noexcept;
extern "C" int luaApi_Desktop_qtStyleList(lua_State *L) noexcept;
extern "C" int luaApi_Desktop_systemAppMode(lua_State *L) noexcept;
extern "C" int luaApi_Desktop_systemStyle(lua_State *L) noexcept;

#endif // ADDON_API_H
