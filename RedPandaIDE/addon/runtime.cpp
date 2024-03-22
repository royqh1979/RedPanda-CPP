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
#include "runtime.h"

#include <lua/lua.hpp>

namespace AddOn {

LuaError::LuaError(const QString &reason): BaseError(reason) {}

RaiiLuaState::RaiiLuaState(const QString &name, std::chrono::microseconds timeLimit)
    : mLua(luaL_newstate()) {
    mExtraState.insert(mLua, {name, timeLimit, /* .timeStart = */ {}});
}

RaiiLuaState::RaiiLuaState(RaiiLuaState &&rhs)
    : mLua(rhs.mLua) {
    rhs.mLua = nullptr;
}

RaiiLuaState &RaiiLuaState::operator=(RaiiLuaState &&rhs) {
    // do not check self assignment intentionally, following STL semantics
    this->~RaiiLuaState();
    // if self assignment, move constructor guarantees to place the object
    // in a valid (but unspecified) state
    new (this) RaiiLuaState(std::move(rhs));
    return *this;
}

RaiiLuaState::~RaiiLuaState() {
    if (mLua) {
        mExtraState.remove(mLua);
        lua_close(mLua);
    }
}

bool RaiiLuaState::fetchBoolean(int index)
{
    return lua_toboolean(mLua, index);
}

long long RaiiLuaState::fetchInteger(int index)
{
    return lua_tointeger(mLua, index);
}

double RaiiLuaState::fetchNumber(int index)
{
    return lua_tonumber(mLua, index);
}

QString RaiiLuaState::fetchString(int index)
{
    return lua_tostring(mLua, index);
}

QJsonObject RaiiLuaState::fetchObject(int index)
{
    return fetchTableImpl(mLua, index, 0).toObject();
}

QJsonValue RaiiLuaState::fetch(int index)
{
    return fetchValueImpl(mLua, index, 0);
}

bool RaiiLuaState::fetchBoolean(lua_State *L, int index)
{
    return lua_toboolean(L, index);
}

long long RaiiLuaState::fetchInteger(lua_State *L, int index)
{
    return lua_tointeger(L, index);
}

double RaiiLuaState::fetchNumber(lua_State *L, int index)
{
    return lua_tonumber(L, index);
}

QString RaiiLuaState::fetchString(lua_State *L, int index)
{
    return lua_tostring(L, index);
}

QJsonArray RaiiLuaState::fetchArray(lua_State *L, int index)
{
    return fetchTableImpl(L, index, 0).toArray();
}

QJsonObject RaiiLuaState::fetchObject(lua_State *L, int index)
{
    return fetchTableImpl(L, index, 0).toObject();
}

QJsonValue RaiiLuaState::fetch(lua_State *L, int index)
{
    return fetchValueImpl(L, index, 0);
}

bool RaiiLuaState::popBoolean()
{
    bool value = lua_toboolean(mLua, -1);
    lua_pop(mLua, 1);
    return value;
}

QString RaiiLuaState::popString()
{
    QString value = fetchString(-1);
    lua_pop(mLua, 1);
    return value;
}

QJsonObject RaiiLuaState::popObject()
{
    QJsonObject value = fetchObject(-1);
    lua_pop(mLua, 1);
    return value;
}

QJsonValue RaiiLuaState::pop()
{
    QJsonValue value = fetch(-1);
    lua_pop(mLua, 1);
    return value;
}

QJsonValue RaiiLuaState::pop(lua_State *L)
{
    QJsonValue value = fetch(L, -1);
    lua_pop(L, 1);
    return value;
}

void RaiiLuaState::push(decltype(nullptr))
{
    lua_pushnil(mLua);
}

void RaiiLuaState::push(const QMap<QString, lua_CFunction> &value)
{
    lua_newtable(mLua);
    for (auto it = value.cbegin(); it != value.cend(); ++it) {
        lua_pushstring(mLua, it.key().toUtf8().constData());
        lua_pushcfunction(mLua, it.value());
        lua_settable(mLua, -3);
    }
}

void RaiiLuaState::push(lua_State *L, decltype(nullptr))
{
    lua_pushnil(L);
}

void RaiiLuaState::push(lua_State *L, bool value)
{
    lua_pushboolean(L, value);
}

void RaiiLuaState::push(lua_State *L, const QString &value)
{
    lua_pushstring(L, value.toUtf8().constData());
}

void RaiiLuaState::push(lua_State *L, const QStringList &value)
{
    lua_newtable(L);
    for (int i = 0; i < value.length(); i++) {
        lua_pushinteger(L, i + 1);
        lua_pushstring(L, value[i].toUtf8().constData());
        lua_settable(L, -3);
    }
}

void RaiiLuaState::push(lua_State *L, const QJsonArray &value)
{
    pushArrayImpl(L, value, 0);
}

void RaiiLuaState::push(lua_State *L, const QJsonObject &value)
{
    pushObjectImpl(L, value, 0);
}

int RaiiLuaState::getTop()
{
    return lua_gettop(mLua);
}

int RaiiLuaState::getTop(lua_State *L)
{
    return lua_gettop(L);
}

int RaiiLuaState::loadBuffer(const QByteArray &buff, const QString &name)
{
    return luaL_loadbuffer(mLua, buff.constData(), buff.size(), name.toUtf8().constData());
}

void RaiiLuaState::openLibs()
{
    luaL_openlibs(mLua);
}

int RaiiLuaState::pCall(int nargs, int nresults, int msgh)
{
    return lua_pcall(mLua, nargs, nresults, msgh);
}

int RaiiLuaState::getGlobal(const QString &name)
{
    return lua_getglobal(mLua, name.toUtf8().constData());
}

void RaiiLuaState::setGlobal(const QString &name)
{
    return lua_setglobal(mLua, name.toUtf8().constData());
}

void RaiiLuaState::setHook(lua_Hook f, int mask, int count)
{
    lua_sethook(mLua, f, mask, count);
}

void RaiiLuaState::setTimeStart() {
    extraState().timeStart = std::chrono::system_clock::now();
}

LuaExtraState &RaiiLuaState::extraState() {
    return mExtraState[mLua];
}

LuaExtraState &RaiiLuaState::extraState(lua_State *lua) {
    return mExtraState[lua];
}

QJsonValue RaiiLuaState::fetchTableImpl(lua_State *L, int index, int depth)
{
    if (depth == 1)
        // check stack size at first recursion to avoid multiple reallocations
        lua_checkstack(L, LUA_STACK_SIZE);
    if (depth > TABLE_MAX_DEPTH)
        throw LuaError("Lua runtime error: table nested too deeply");

    push(L, nullptr); // make sure lua_next starts at beginning
    int newIndex = index < 0 ? index - 1 : index; // after push negative index changes

    QJsonObject hashPart;
    QJsonArray arrayPart;

    // here we take the fact that Lua iterates array part first
    bool processingArrayPart = true;
    while (lua_next(L, newIndex)) {
        QJsonValue v;
        try {
            v = fetchValueImpl(L, -1, depth);
        } catch (const LuaError &e) {
            QString key = fetchString(L, -2);
            QString reason = e.reason() + QString(" (at table key '%1')").arg(key);
            lua_pop(L, 2);
            throw LuaError(reason);
        }
        lua_pop(L, 1);
        if (processingArrayPart && lua_isinteger(L, -1) && fetchInteger(L, -1) == arrayPart.size() + 1)
            // we are still in array part
            arrayPart.push_back(v);
        else {
            // we have stepped in hash part
            processingArrayPart = false;
            QString k = fetchString(L, -1);
            hashPart[k] = v;
        }
    }

    if (!arrayPart.empty())
        if (!hashPart.empty())
            // table contains both array part and hash part, malformed
            throw LuaError("Lua type error: table contains both array part and hash part.");
        else
            // is array
            return arrayPart;
    else
        if (!hashPart.empty())
            // is object
            return hashPart;
        else
            // empty table, okay
            // return null since we cannot determine
            return {};
}

QJsonValue RaiiLuaState::fetchValueImpl(lua_State *L, int index, int depth)
{
    if (lua_isnil(L, index))
        return {};
    else if (lua_isboolean(L, index))
        return fetchBoolean(L, index);
    else if (lua_isinteger(L, index))
        return fetchInteger(L, index);
    // lua_isnumber treats strings that can be converted to numbers as numbers
    // use lua_type to detect numbers
    else if (lua_type(L, index) == LUA_TNUMBER)
        return fetchNumber(L, index);
    else if (lua_isstring(L, index))
        return fetchString(L, index);
    else if (lua_istable(L, index))
        return fetchTableImpl(L, index, depth + 1);
    else {
        int type = lua_type(L, index);
        const char *name = lua_typename(L, type);
        throw LuaError(QString("Lua type error: unknown type %1.").arg(name));
    }
}

void RaiiLuaState::pushArrayImpl(lua_State *L, const QJsonArray &value, int depth)
{
    if (depth == 1)
        // check stack size at first recursion to avoid multiple reallocations
        lua_checkstack(L, LUA_STACK_SIZE);
    if (depth > TABLE_MAX_DEPTH)
        throw LuaError("Lua runtime error: table nested too deeply");

    lua_newtable(L);
    for (int i = 0; i < value.size(); i++) {
        push(L, i + 1);
        pushValueImpl(L, value[i], depth);
        lua_settable(L, -3);
    }
}

void RaiiLuaState::pushObjectImpl(lua_State *L, const QJsonObject &value, int depth)
{
    if (depth == 1)
        // check stack size at first recursion to avoid multiple reallocations
        lua_checkstack(L, LUA_STACK_SIZE);
    if (depth > TABLE_MAX_DEPTH)
        throw LuaError("Lua runtime error: table nested too deeply");

    lua_newtable(L);
    for (auto it = value.begin(); it != value.end(); ++it) {
        push(L, it.key());
        pushValueImpl(L, it.value(), depth);
        lua_settable(L, -3);
    }
}

void RaiiLuaState::pushValueImpl(lua_State *L, const QJsonValue &value, int depth)
{
    if (value.isNull())
        lua_pushnil(L);
    else if (value.isBool())
        lua_pushboolean(L, value.toBool());
    else if (value.isDouble())
        lua_pushnumber(L, value.toDouble());
    else if (value.isString())
        lua_pushstring(L, value.toString().toUtf8().constData());
    else if (value.isObject())
        pushObjectImpl(L, value.toObject(), depth + 1);
    else if (value.isArray())
        pushArrayImpl(L, value.toArray(), depth + 1);
    else
        throw LuaError("Lua type error: unknown type.");
}

QHash<lua_State *, LuaExtraState> RaiiLuaState::mExtraState;

} // namespace AddOn
