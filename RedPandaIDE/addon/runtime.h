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

    static QString fetchString(lua_State *L, int index);

    bool popBoolean();
    long long popInteger();
    double popNumber();
    QString popString();
    QJsonArray popArray();
    QJsonObject popObject();
    QJsonValue pop();

    void push(decltype(nullptr));
    void push(const QMap<QString, lua_CFunction> &value);

    static void push(lua_State *L, const QString &value);
    static void push(lua_State *L, const QStringList &value);

    int loadBuffer(const QByteArray &buff, const QString &name);
    void openLibs();
    int pCall(int nargs, int nresults, int msgh);
    void setGlobal(const QString &name);
    void setHook(lua_Hook f, int mask, int count);

    void setTimeStart();
    LuaExtraState &extraState();
    static LuaExtraState &extraState(lua_State *lua);

private:
    QJsonValue fetchTableImpl(int index, int depth);
    QJsonValue fetchValueImpl(int index, int depth);

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
