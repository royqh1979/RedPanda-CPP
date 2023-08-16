#ifndef ADDON_EXECUTOR_H
#define ADDON_EXECUTOR_H

#include <QJsonValue>
#include <QJsonObject>
#include <QStringList>
#include <chrono>

namespace AddOn {

// simple, stateless Lua executor
class SimpleExecutor {
protected:
    SimpleExecutor(const QList<QString> &apis): mApis(apis) {}

    // run a Lua script and fetch its return value as type R
    QJsonValue runScript(const QByteArray &script, const QString &name,
                         std::chrono::microseconds timeLimit);

private:
    QStringList mApis;
};

class ThemeExecutor : private SimpleExecutor {
public:
    ThemeExecutor();
    QJsonObject operator()(const QByteArray &script, const QString &name);
};

}

#endif // ADDON_EXECUTOR_H
