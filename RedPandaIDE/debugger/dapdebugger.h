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
#ifndef DAP_DEBUGGER_H
#define DAP_DEBUGGER_H

#include "debugger.h"

class DAPDebuggerClient : public DebuggerClient {
    Q_OBJECT
public:
    explicit DAPDebuggerClient(Debugger* debugger, QObject *parent = nullptr);


    // QThread interface
protected:
    void run() override;

    // DebuggerClient interface
public:
    DebuggerType clientType() override;

private:
    void initializeRequest();
private:
    std::shared_ptr<QProcess> mProcess;
    bool mStop;
};

#endif
