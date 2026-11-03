/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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

#ifndef TERMINALPROCESS_H
#define TERMINALPROCESS_H

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QStringList>
#include <QDir>

// Lightweight command-panel backend using QProcess.
// Executes one command at a time. Built-in commands (cd, pwd, clear)
// are handled internally since they need to affect terminal state.
//
// Not a full PTY terminal. For vim/htop/ssh support, implement a
// PTY-based ITerminalBackend.

class TerminalProcess : public QObject
{
    Q_OBJECT
public:
    explicit TerminalProcess(QObject *parent = nullptr);
    ~TerminalProcess();

    // Execute a command line. Returns true if accepted.
    // Handles built-in commands internally.
    bool execute(const QString &commandLine);

    // Stop the currently running process.
    void stop();

    // Current working directory of the terminal session.
    QString workingDirectory() const;
    void setWorkingDirectory(const QString &path);

    QByteArray parseAnsiToHtml(const QByteArray &data);

signals:
    void outputReady(const QString &html);
    void commandFinished(int exitCode);
    void workingDirectoryChanged(const QString &path);

private slots:
    void onProcessStarted();
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    bool handleBuiltin(const QString &line);
    void runExternal(const QString &program, const QStringList &args);

    QProcess *mProcess = nullptr;
    QDir mCurrentDir;
    QStringList mHistory;
    int mHistoryIndex = -1;
    QByteArray mPendingOutput;
    bool mRunning = false;
};

#endif // TERMINALPROCESS_H
