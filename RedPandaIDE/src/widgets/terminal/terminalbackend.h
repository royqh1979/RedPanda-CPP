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

#ifndef TERMINALBACKEND_H
#define TERMINALBACKEND_H

#include <QByteArray>
#include <QObject>
#include <QProcessEnvironment>

// Design doc: This is the abstract interface for terminal backends.
// Currently only PipeProcessBackend (QProcess-based) is implemented.
// Future backends (UnixPtyBackend, ConPtyBackend) should implement
// this same interface for full PTY terminal support.
//
// Key contract:
//   1. Backend sends/receives raw QByteArray — no ANSI/VT parsing here
//   2. Backend reports capabilities — UI adapts accordingly
//   3. Backend does NOT handle rendering or parsing
//   4. resize() and sendSignal() may be no-ops for low-capability backends

class ITerminalBackend : public QObject
{
public:
    struct StartOptions {
        QString program;
        QStringList arguments;
        QString workingDirectory;
        QProcessEnvironment environment;
        bool mergeStdErr = true;
    };

    explicit ITerminalBackend(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ITerminalBackend() = default;

    virtual void start(const StartOptions &options) = 0;
    virtual void writeBytes(const QByteArray &data) = 0;
    virtual bool isRunning() const = 0;
    virtual void stop() = 0;
    virtual void kill() = 0;

    // Signals are defined as virtual callback setters for the pure interface.
    // Concrete backends emit Qt signals for these callbacks.
    std::function<void()> onStarted;
    std::function<void(const QByteArray &)> onReadyRead;
    std::function<void(int)> onFinished;
    std::function<void(const QString &)> onError;
};

#endif // TERMINALBACKEND_H
