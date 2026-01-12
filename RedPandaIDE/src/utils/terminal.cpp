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
#include "terminal.h"
#include <QCoreApplication>
#include <QDir>
#include "escape.h"
#include "parsearg.h"
#include "../settings/dirsettings.h"
#include "../settings/environmentsettings.h"
#include <qt_utils/utils.h>

std::tuple<QString, QStringList, PNonExclusiveTemporaryFileOwner> wrapCommandForTerminalEmulator(const QString &terminal, const QStringList &argsPattern, const QStringList &payloadArgsWithArgv0, DirSettings *dirSettings)
{
    QStringList wrappedArgs;
    std::unique_ptr<QTemporaryFile> temproryFile;
    for (const QString &patternItem : argsPattern) {
        if (patternItem == "$term")
            wrappedArgs.append(terminal);
        else if (patternItem == "$integrated_term")
            wrappedArgs.append(includeTrailingPathDelimiter(dirSettings->appDir())+terminal);
        else if (patternItem == "$argv")
            wrappedArgs.append(payloadArgsWithArgv0);
        else if (patternItem == "$command" || patternItem == "$unix_command") {
            // “$command” is for compatibility; previously used on multiple Unix terms
            QStringList escapedArgs;
            for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                auto &arg = payloadArgsWithArgv0[i];
                auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::BourneAgainShellPretty);
                escapedArgs.append(escaped);
            }
            wrappedArgs.push_back(escapedArgs.join(' '));
        } else if (patternItem == "$dos_command") {
            QStringList escapedArgs;
            for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                auto &arg = payloadArgsWithArgv0[i];
                auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::WindowsCommandPrompt);
                escapedArgs.append(escaped);
            }
            wrappedArgs.push_back(escapedArgs.join(' '));
        } else if (patternItem == "$lpCommandLine") {
            QStringList escapedArgs;
            for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                auto &arg = payloadArgsWithArgv0[i];
                auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::WindowsCreateProcess);
                escapedArgs.append(escaped);
            }
            wrappedArgs.push_back(escapedArgs.join(' '));
        } else if (patternItem == "$tmpfile" || patternItem == "$tmpfile.command") {
            // “$tmpfile” is for compatibility; previously used on macOS Terminal.app
            temproryFile = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/redpanda_XXXXXX.command");
            if (temproryFile->open()) {
                QStringList escapedArgs;
                for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                    auto &arg = payloadArgsWithArgv0[i];
                    auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::BourneAgainShellPretty);
                    escapedArgs.append(escaped);
                }
                temproryFile->write(escapedArgs.join(' ').toUtf8());
                temproryFile->write("\n");
                QFile(temproryFile->fileName()).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
            }
            wrappedArgs.push_back(temproryFile->fileName());
        } else if (patternItem == "$tmpfile.sh") {
            temproryFile = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/redpanda_XXXXXX.command");
            if (temproryFile->open()) {
                QStringList escapedArgs = {"exec"};
                for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                    auto &arg = payloadArgsWithArgv0[i];
                    auto escaped = escapeArgument(arg, false, EscapeArgumentRule::BourneAgainShellPretty);
                    escapedArgs.append(escaped);
                }
                temproryFile->write("#!/bin/sh\n");
                temproryFile->write(escapedArgs.join(' ').toUtf8());
                temproryFile->write("\n");
                QFile(temproryFile->fileName()).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
            }
            wrappedArgs.push_back(temproryFile->fileName());
        } else if (patternItem == "$tmpfile.bat") {
            temproryFile = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/redpanda_XXXXXX.bat");
            if (temproryFile->open()) {
                QStringList escapedArgs;
                for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                    auto &arg = payloadArgsWithArgv0[i];
                    auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::WindowsCommandPrompt);
                    escapedArgs.append(escaped);
                }
                temproryFile->write(escapedArgs.join(' ').toLocal8Bit());
                temproryFile->write("\r\n");
                QFile(temproryFile->fileName()).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
            }
            wrappedArgs.push_back(temproryFile->fileName());
        } else if (patternItem == "$sequential_app_id") {
            static QString prefix = QStringLiteral("io.redpanda.term_%1_").arg(QCoreApplication::applicationPid());
            static std::atomic<int> appIdCounter = 0;
            QString appId = prefix + QString::number(++appIdCounter);
            wrappedArgs.push_back(appId);
        } else
            wrappedArgs.push_back(patternItem);
    }
    if (wrappedArgs.empty())
        return {QString(""), QStringList{}, std::make_unique<NonExclusiveTemporaryFileOwner>(temproryFile)};
    return {wrappedArgs[0], wrappedArgs.mid(1), std::make_unique<NonExclusiveTemporaryFileOwner>(temproryFile)};
}

std::tuple<QString, QStringList, PNonExclusiveTemporaryFileOwner> wrapCommandForTerminalEmulator(const QString &terminal, const QString &argsPattern, const QStringList &payloadArgsWithArgv0, DirSettings *dirSettings)
{
    return wrapCommandForTerminalEmulator(terminal, parseArguments(argsPattern, EnvironmentSettings::terminalArgsPatternMagicVariables(), false), payloadArgsWithArgv0, dirSettings);
}
