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
    SimpleExecutor(const QString &kind, int major, int minor, const QList<QString> &apis);

    bool apiVersionCheck(const QJsonObject &addonApi);

    QJsonValue runScript(const QByteArray &script, const QString &name,
                         std::chrono::microseconds timeLimit);

private:
    QString mKind;
    int mMajor;
    int mMinor;
    QStringList mApis;
};

class ThemeExecutor : private SimpleExecutor {
public:
    ThemeExecutor();
    QJsonObject operator()(const QByteArray &script, const QString &name);
};

class CompilerHintExecutor : private SimpleExecutor {
public:
    CompilerHintExecutor();
    QJsonObject operator()(const QByteArray &script);
};

}

#endif // ADDON_EXECUTOR_H
