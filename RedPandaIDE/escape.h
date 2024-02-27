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
#ifndef ESCAPE_H
#define ESCAPE_H

#include <QString>

enum class EscapeArgumentRule {
    BourneAgainShellPretty,
    BourneAgainShellFast,
    WindowsCreateProcess,
    WindowsCreateProcessForceQuote,
    WindowsCommandPrompt,
};

QString escapeArgument(const QString &arg, bool isFirstArg, EscapeArgumentRule rule);

EscapeArgumentRule platformShellEscapeArgumentRule();
QString escapeArgumentForPlatformShell(const QString &arg, bool isFirstArg);
QString escapeCommandForPlatformShell(const QString &prog, const QStringList &args);

EscapeArgumentRule makefileRecipeEscapeArgumentRule();
QString escapeArgumentForMakefileVariableValue(const QString &arg, bool isFirstArg);
QString escapeArgumentsForMakefileVariableValue(const QStringList &args);
QString escapeFilenameForMakefileInclude(const QString &filename);
QString escapeFilenameForMakefileTarget(const QString &filename);
QString escapeFilenameForMakefilePrerequisite(const QString &filename);
QString escapeFilenamesForMakefilePrerequisite(const QStringList &filenames);
QString escapeArgumentForMakefileRecipe(const QString &arg, bool isFirstArg);

QString escapeArgumentForInputField(const QString &arg, bool isFirstArg);
QString escapeArgumentsForInputField(const QStringList &args);

#endif // ESCAPE_H
