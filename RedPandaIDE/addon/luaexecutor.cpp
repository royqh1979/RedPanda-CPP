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
         {"debug", &luaApi_Debug_debug}, // (string, ...) -> ()
         {"messageBox", &luaApi_Debug_messageBox}, // (string, ...) -> ()
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
    {"C_FileSystem",
     {
         {"exists", &luaApi_FileSystem_exists},             // (string) -> bool
         {"isExecutable", &luaApi_FileSystem_isExecutable}, // (string) -> bool
         {"matchFiles", &luaApi_FileSystem_matchFiles},     // (string, string) -> [string]
     }},
    {"C_System",
     {
         {"appArch", &luaApi_System_appArch},               // () -> string
         {"appDir", &luaApi_System_appDir},                 // () -> string
         {"appLibexecDir", &luaApi_System_appLibexecDir},   // () -> string
         {"appResourceDir", &luaApi_System_appResourceDir}, // () -> string
         {"osArch", &luaApi_System_osArch},                 // () -> string
         {"popen", &luaApi_System_popen},
         {"supportedAppArchList",
          &luaApi_System_supportedAppArchList}, // () -> string
#ifdef Q_OS_WINDOWS
         {"readRegistry",
          &luaApi_System_readRegistry}, // (string, string) -> string|nil
#endif
     }},
    {"C_Util",
     {
         {"format", &luaApi_Util_format}, // (string, ...) -> string
     }}};

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

ThemeExecutor::ThemeExecutor() : SimpleExecutor(
    "theme", 0, 1,
    {"C_Debug", "C_Desktop", "C_Util"})
{
}

QJsonObject ThemeExecutor::operator()(const QByteArray &script,
                                      const QString &name) {
    using namespace std::chrono_literals;
    QJsonValue result = SimpleExecutor::runScript(script, "theme:" + name, 100ms);
    if (result.isObject() || result.isNull())
        return result.toObject();
    else
        throw LuaError("Theme script must return an object.");
}

SimpleExecutor::SimpleExecutor(const QString &kind, int major, int minor, const QList<QString> &apis)
    : mKind(kind), mMajor(major), mMinor(minor), mApis(apis)
{
}

bool SimpleExecutor::apiVersionCheck(const QJsonObject &addonApi) {
    const QString &addonKind = addonApi["kind"].toString();
    if (addonKind != mKind)
        return false;
    if (!addonApi.contains("major") || !addonApi.contains("minor"))
        return false;
    int addonMajor = addonApi["major"].toInt();
    int addonMinor = addonApi["minor"].toInt();
    if (mMajor == 0)
        return addonMajor == mMajor && addonMinor == mMinor;
    else
        return addonMajor == mMajor && addonMinor <= mMinor;
}

QJsonValue SimpleExecutor::runScript(const QByteArray &script,
                                     const QString &name,
                                     std::chrono::microseconds timeLimit) {
    RaiiLuaState L(name, timeLimit);
    int retLoad = L.loadBuffer(script, name);
    if (retLoad != 0)
        throw LuaError(QString("Lua load error: %1.").arg(L.popString()));
    L.setHook(&luaHook_timeoutKiller, LUA_MASKCOUNT, 1'000'000); // ~5ms on early 2020s desktop CPUs
    L.setTimeStart();
    int callResult = L.pCall(0, 0, 0);
    if (callResult != 0) {
        throw LuaError(QString("Lua error: %1.").arg(L.popString()));
    }

    // call `apiVersion()` to check compatibility
    int type = L.getGlobal("apiVersion");
    if (type != LUA_TFUNCTION) {
        throw LuaError("Add-on interface error: `apiVersion` is not a function.");
    }
    callResult = L.pCall(0, 1, 0);
    if (callResult != 0) {
        throw LuaError(QString("Lua error: %1.").arg(L.popString()));
    }
    QJsonObject addonApi = L.popObject();
    if (!apiVersionCheck(addonApi)) {
        throw LuaError(QString("Add-on interface error: API version incompatible with %1:%2.%3.")
            .arg(mKind).arg(mMajor).arg(mMinor));
    }

    // inject APIs and call `main()`
    L.openLibs();
    for (auto &api : mApis)
        registerApiGroup(L, api);
    type = L.getGlobal("main");
    if (type != LUA_TFUNCTION) {
        throw LuaError("Add-on interface error: `main` is not a function.");
    }
    callResult = L.pCall(0, 1, 0);
    if (callResult != 0) {
        throw LuaError(QString("Lua error: %1.").arg(L.popString()));
    }
    return L.fetch(1);
}

CompilerHintExecutor::CompilerHintExecutor() : SimpleExecutor(
    "compiler_hint", 0, 2,
    {"C_Debug", "C_Desktop", "C_FileSystem", "C_System", "C_Util"})
{
}

QJsonObject CompilerHintExecutor::operator()(const QByteArray &script) {
    using namespace std::chrono_literals;
    QJsonValue result = SimpleExecutor::runScript(script, "compiler_hint.lua", 1s);
    if (result.isObject() || result.isNull())
        return result.toObject();
    else
        throw LuaError("Compiler hint script must return an object.");
}

} // namespace AddOn
