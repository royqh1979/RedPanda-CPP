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

QJsonValue RaiiLuaState::fetch(int index)
{
    return fetchValueImpl(index, 0);
}

QString RaiiLuaState::fetchString(lua_State *L, int index)
{
    return lua_tostring(L, index);
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

QJsonValue RaiiLuaState::pop()
{
    QJsonValue value = fetch(-1);
    lua_pop(mLua, 1);
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

QJsonValue RaiiLuaState::fetchTableImpl(int index, int depth)
{
    if (depth == 1)
        // check stack size at first recursion to avoid multiple reallocations
        lua_checkstack(mLua, LUA_STACK_SIZE);
    if (depth > TABLE_MAX_DEPTH)
        throw LuaError("Lua runtime error: table nested too deeply");

    push(nullptr); // make sure lua_next starts at beginning
    int newIndex = index < 0 ? index - 1 : index; // after push negative index changes

    QJsonObject hashPart;
    QJsonArray arrayPart;

    // here we take the fact that Lua iterates array part first
    bool processingArrayPart = true;
    while (lua_next(mLua, newIndex)) {
        QJsonValue v = pop();
        if (processingArrayPart && lua_isinteger(mLua, -1) && fetchInteger(-1) == arrayPart.size() + 1)
            // we are still in array part
            arrayPart.push_back(v);
        else {
            // we have stepped in hash part
            processingArrayPart = false;
            QString k = fetchString(-1);
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

QJsonValue RaiiLuaState::fetchValueImpl(int index, int depth)
{
    if (lua_isnil(mLua, index))
        return {};
    else if (lua_isboolean(mLua, index))
        return fetchBoolean(index);
    else if (lua_isinteger(mLua, index))
        return fetchInteger(index);
    else if (lua_isnumber(mLua, index))
        return fetchNumber(index);
    else if (lua_isstring(mLua, index))
        return fetchString(index);
    else if (lua_istable(mLua, index))
        return fetchTableImpl(index, depth + 1);
    else
        throw LuaError(QString("Lua type error: unknown type %1.").arg(lua_typename(mLua, index)));
}

QHash<lua_State *, LuaExtraState> RaiiLuaState::mExtraState;

} // namespace AddOn
