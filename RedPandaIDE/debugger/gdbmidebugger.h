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
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <memory>
#include <QRegularExpression>

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
    void runNextCmd() override;
private:
    QStringList tokenize(const QString& s) const;
    bool outputTerminated(const QByteArray& text) const;
    void handleBreakpoint(const GDBMIResultParser::ParseObject& breakpoint);
    void handleCreateVar(const GDBMIResultParser::ParseObject &multiVars);
    void handleFrame(const GDBMIResultParser::ParseValue &frame);
    void handleStack(const QList<GDBMIResultParser::ParseValue> & stack);
    void handleLocalVariables(const QList<GDBMIResultParser::ParseValue> & variables);
    void handleEvaluation(const QString& value);
    void handleMemory(const QList<GDBMIResultParser::ParseValue> & rows);
    void handleRegisterNames(const QList<GDBMIResultParser::ParseValue> & names);
    void handleRegisterValue(const QList<GDBMIResultParser::ParseValue> & values);
    void handleListVarChildren(const GDBMIResultParser::ParseObject& multiVars);
    void handleUpdateVarValue(const QList<GDBMIResultParser::ParseValue> &changes);
    void processConsoleOutput(const QByteArray& line);
    void processLogOutput(const QByteArray& line);
    void processResult(const QByteArray& result);
    void processExecAsyncRecord(const QByteArray& line);
    void processError(const QByteArray& errorLine);
    void processResultRecord(const QByteArray& line);
    void processDebugOutput(const QByteArray& debugOutput);
    QByteArray removeToken(const QByteArray& line) const;
private slots:
    void asyncUpdate();
private:
    bool mStop;
    std::shared_ptr<QProcess> mProcess;
    QMap<QString,QStringList> mFileCache;
    int mCurrentLine;
    qulonglong mCurrentAddress;
    QString mCurrentFunc;
    QString mCurrentFile;

    bool mAsyncUpdated;

    static const QRegularExpression REGdbSourceLine;
};

#endif // GDBMI_DEBUGGER_H
