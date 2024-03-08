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
#ifndef GDBMI_DEBUGGER_H
#define GDBMI_DEBUGGER_H

#include "debugger.h"
#include <QProcess>
#include <memory>

class GDBMIDebuggerClient: public DebuggerClient {
    Q_OBJECT
public:
    explicit GDBMIDebuggerClient(Debugger* debugger, QObject *parent = nullptr);

    // DebuggerClient interface
public:
    void postCommand(const QString &Command, const QString &Params, DebugCommandSource Source) override;
    void registerInferiorStoppedCommand(const QString &Command, const QString &Params) override;
    void stopDebug() override;
    // QThread interface
protected:
    void run() override;
private:
    bool mStop;
    std::shared_ptr<QProcess> mProcess;
};

#endif // GDBMI_DEBUGGER_H
