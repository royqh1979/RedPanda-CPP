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
#include "executor.h"

#include <lua/lua.hpp>

#include "api.h"
#include "runtime.h"

namespace AddOn {

static QMap<QString, QMap<QString, lua_CFunction>> apiGroups{
                                                             {"C_Debug",
                                                              {
                                                                  {"debug", &luaApi_Debug_debug}, // (string) -> ()
                                                              }},
                                                             {"C_Desktop",
                                                              {
                                                                  {"desktopEnvironment",
                                                                   &luaApi_Desktop_desktopEnvironment},             // () -> string
                                                                  {"language", &luaApi_Desktop_language},           // () -> string
                                                                  {"qtStyleList", &luaApi_Desktop_qtStyleList},     // () -> [string]
                                                                  {"systemAppMode", &luaApi_Desktop_systemAppMode}, // () -> string
                                                                  {"systemStyle", &luaApi_Desktop_systemStyle},     // () -> string
                                                              }},
                                                             };

static void registerApiGroup(RaiiLuaState &L, const QString &name) {
    L.push(apiGroups[name]);
    L.setGlobal(name);
}

extern "C" void luaHook_timeoutKiller(lua_State *L, lua_Debug *ar [[maybe_unused]]) noexcept {
    using namespace std::chrono;
    AddOn::LuaExtraState &extraState = AddOn::RaiiLuaState::extraState(L);
    auto duration = system_clock::now() - extraState.timeStart;
    if (duration > extraState.timeLimit) {
        lua_pushfstring(L,
                        "timeout in script '%s' (%d/%d ms)",
                        extraState.name.toUtf8().constData(),
                        int(duration_cast<milliseconds>(duration).count()),
                        int(duration_cast<milliseconds>(extraState.timeLimit).count()));
        lua_error(L);
    }
};

ThemeExecutor::ThemeExecutor() : SimpleExecutor({"C_Debug", "C_Desktop"}) {}

QJsonObject ThemeExecutor::operator()(const QByteArray &script,
                                      const QString &name) {
    using namespace std::chrono_literals;
    QJsonValue result = SimpleExecutor::runScript(script, "theme:" + name, 100ms);
    if (result.isObject() || result.isNull())
        return result.toObject();
    else
        throw LuaError("Theme script must return an object.");
}

QJsonValue SimpleExecutor::runScript(const QByteArray &script,
                                     const QString &name,
                                     std::chrono::microseconds timeLimit) {
    RaiiLuaState L(name, timeLimit);
    L.openLibs();
    for (auto &api : mApis)
        registerApiGroup(L, api);

    int retLoad = L.loadBuffer(script, name);
    if (retLoad != 0)
        throw LuaError("Lua script load error.");
    L.setHook(&luaHook_timeoutKiller, LUA_MASKCOUNT, 1'000'000); // ~5ms on early 2020s desktop CPUs
    L.setTimeStart();
    int callResult = L.pCall(0, 1, 0);
    if (callResult != 0) {
        throw LuaError(QString("Lua error: %1.").arg(L.popString()));
    }

    return L.fetch(1);
}

} // namespace AddOn
