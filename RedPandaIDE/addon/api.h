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
