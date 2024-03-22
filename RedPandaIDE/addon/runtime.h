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
#ifndef ADDON_RUNTIME_H
#define ADDON_RUNTIME_H

#include <chrono>

#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>

#include <qt_utils/utils.h>

struct lua_State;
struct lua_Debug;
typedef int (*lua_CFunction) (lua_State *L);
typedef void (*lua_Hook) (lua_State *L, lua_Debug *ar);

namespace AddOn {

void registerApiGroup(lua_State *L, const QString &groupName);

class LuaError : public BaseError {
public:
    explicit LuaError(const QString &reason);
};

struct LuaExtraState {
    QString name;
    std::chrono::microseconds timeLimit;
    std::chrono::time_point<std::chrono::system_clock> timeStart;
};

class RaiiLuaState {
public:
    RaiiLuaState(const QString &name, std::chrono::microseconds timeLimit);
    RaiiLuaState(const RaiiLuaState &) = delete;
    RaiiLuaState(RaiiLuaState &&rhs);
    RaiiLuaState &operator=(const RaiiLuaState &) = delete;
    RaiiLuaState &operator=(RaiiLuaState &&rhs);
    ~RaiiLuaState();

    bool fetchBoolean(int index);
    long long fetchInteger(int index);
    double fetchNumber(int index);
    QString fetchString(int index);
    QJsonArray fetchArray(int index);
    QJsonObject fetchObject(int index);
    QJsonValue fetch(int index);

    static bool fetchBoolean(lua_State *L, int index);
    static long long fetchInteger(lua_State *L, int index);
    static double fetchNumber(lua_State *L, int index);
    static QString fetchString(lua_State *L, int index);
    static QJsonArray fetchArray(lua_State *L, int index);
    static QJsonObject fetchObject(lua_State *L, int index);
    static QJsonValue fetch(lua_State *L, int index);

    bool popBoolean();
    long long popInteger();
    double popNumber();
    QString popString();
    QJsonArray popArray();
    QJsonObject popObject();
    QJsonValue pop();

    static QJsonValue pop(lua_State *L);

    void push(decltype(nullptr));
    void push(const QMap<QString, lua_CFunction> &value);

    static void push(lua_State *L, decltype(nullptr));
    static void push(lua_State *L, bool value);
    static void push(lua_State *L, const QString &value);
    static void push(lua_State *L, const QStringList &value);
    static void push(lua_State *L, const QJsonArray &value);
    static void push(lua_State *L, const QJsonObject &value);

    int getTop();
    static int getTop(lua_State *L);

    int loadBuffer(const QByteArray &buff, const QString &name);
    void openLibs();
    int pCall(int nargs, int nresults, int msgh);
    int getGlobal(const QString &name);
    void setGlobal(const QString &name);
    void setHook(lua_Hook f, int mask, int count);

    void setTimeStart();
    LuaExtraState &extraState();
    static LuaExtraState &extraState(lua_State *lua);

private:
    static QJsonValue fetchTableImpl(lua_State *L, int index, int depth);
    static QJsonValue fetchValueImpl(lua_State *L, int index, int depth);

    static void pushArrayImpl(lua_State *L, const QJsonArray &value, int depth);
    static void pushObjectImpl(lua_State *L, const QJsonObject &value, int depth);
    static void pushValueImpl(lua_State *L, const QJsonValue &value, int depth);

private:
    lua_State *mLua;
    static QHash<lua_State *, LuaExtraState> mExtraState;

    static constexpr int TABLE_MAX_DEPTH = 10;
    // each nesting level of table requires 2 slots in Lua stack
    // [3]      -- value, will not be used until next level returns
    // [2] [-1] -- key
    // [1] [-2] -- table
    static constexpr int LUA_STACK_SIZE = TABLE_MAX_DEPTH * 2 + 10;
};

}

#endif // ADDON_RUNTIME_H
