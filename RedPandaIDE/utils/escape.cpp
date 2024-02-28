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

#include "utils/escape.h"

#include <QSet>
#include <QRegularExpression>

#ifdef _MSC_VER
#define __builtin_unreachable() (__assume(0))
#endif

static QString contextualBackslashEscaping(const QString &arg, const QSet<QChar> &needsEscaping, bool escapeFinal = true)
{
    QString result;
    for (auto it = arg.begin(); ; ++it) {
        int nBackSlash = 0;
        while (it != arg.end() && *it == '\\') {
            ++it;
            ++nBackSlash;
        }
        if (it == arg.end()) {
            if (escapeFinal) {
                // escape all backslashes, but leave following character unescaped
                // (terminating double quote for CreateProcess, or LF or space in makefile)
                result.append(QString('\\').repeated(nBackSlash * 2));
            } else {
                // leave all backslashes unescaped, and add a space to protect LF
                result.append(QString('\\').repeated(nBackSlash));
                if (nBackSlash > 0)
                    result.push_back(' ');
            }
            break;
        } else if (needsEscaping.contains(*it)) {
            // escape all backslashes and the following character
            result.append(QString('\\').repeated(nBackSlash * 2 + 1));
            result.push_back(*it);
        } else {
            // backslashes aren't special here
            result.append(QString('\\').repeated(nBackSlash));
            result.push_back(*it);
        }
    }
    return result;
}

QString escapeArgumentImplBourneAgainShellPretty(const QString &arg, bool isFirstArg)
{
    // ref. https://pubs.opengroup.org/onlinepubs/9699919799.2018edition/utilities/V3_chap02.html

    if (arg.isEmpty())
        return R"("")";

    /* POSIX say the following reserved words (may) have special meaning:
     *   !, {, }, case, do, done, elif, else, esac, fi, for, if, in, then, until, while,
     *   [[, ]], function, select,
     * only if used as the _first word_ of a command (or somewhere we dot not care).
     */
    const static QSet<QString> reservedWord{
        "!", "{", "}", "case", "do", "done", "elif", "else", "esac", "fi", "for", "if", "in", "then", "until", "while",
        "[[", "]]", "function", "select",
    };
    if (isFirstArg && reservedWord.contains(arg))
        return QString(R"("%1")").arg(arg);

    /* POSIX say “shall quote”:
     *   '|', '&', ';', '<', '>', '(', ')', '$', '`', '\\', '"', '\'', ' ', '\t', '\n';
     * and “may need to be quoted”:
     *   '*', '?', '[', '#', '~', '=', '%'.
     * among which “may need to be quoted” there are 4 kinds:
     *   - wildcards '*', '?', '[' are “danger anywhere” (handle it as if “shall quote”);
     *   - comment '#', home '~', is “danger at first char in any word”;
     *   - (environment) variable '=' is “danger at any char in first word”;
     *   - foreground '%' is “danger at first char in first word”.
     * although not mentioned by POSIX, bash’s brace expansion '{', '}' are also “danger anywhere”.
     */

    static QRegularExpression doubleQuotingDangerChars(R"([`$\\"])");
    static QRegularExpression otherDangerChars(R"([|&;<>() \t\n*?\[\{\}])");
    bool isDoubleQuotingDanger = arg.contains(doubleQuotingDangerChars);
    bool isSingleQuotingDanger = arg.contains('\'');
    bool isDangerAnyChar = isDoubleQuotingDanger || isSingleQuotingDanger || arg.contains(otherDangerChars);
    bool isDangerFirstChar = (arg[0] == '#') || (arg[0] == '~');
    if (isFirstArg) {
        isDangerAnyChar = isDangerAnyChar || arg.contains('=');
        isDangerFirstChar = isDangerFirstChar || (arg[0] == '%');
    }

    // a “safe” string
    if (!isDangerAnyChar && !isDangerFirstChar)
        return arg;

    // prefer more-common double quoting
    if (!isDoubleQuotingDanger)
        return QString(R"("%1")").arg(arg);

    // and then check the opportunity of single quoting
    if (!isSingleQuotingDanger)
        return QString("'%1'").arg(arg);

    // escaping is necessary
    // use double quoting since it’s less tricky
    QString result = "\"";
    for (auto ch : arg) {
        if (ch == '$' || ch == '`' || ch == '\\' || ch == '"')
            result.push_back('\\');
        result.push_back(ch);
    }
    result.push_back('"');
    return result;
}

QString escapeArgumentImplBourneAgainShellFast(QString arg)
{
    /* 1. replace each single quote with `'\''`, which contains
     *    - a single quote to close quoting,
     *    - an escaped single quote representing the single quote itself, and
     *    - a single quote to open quoting again. */
    arg.replace('\'', R"('\'')");
    /* 2. enclose the string with a pair of single quotes. */
    return '\'' + arg + '\'';
}

QString escapeArgumentImplWindowsCreateProcess(const QString &arg, bool forceQuote)
{
    // See https://stackoverflow.com/questions/31838469/how-do-i-convert-argv-to-lpcommandline-parameter-of-createprocess ,
    // and https://learn.microsoft.com/en-gb/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way .

    static QRegularExpression needQuoting(R"([ \t\n\v"])");
    if (!arg.isEmpty() && !forceQuote && !arg.contains(needQuoting))
        return arg;

    return '"' + contextualBackslashEscaping(arg, {'"'}) + '"';
}

QString escapeArgumentImplWindowsCommandPrompt(const QString &arg)
{
    static QRegularExpression metaChars(R"([()%!^"<>&|])");
    bool containsMeta = arg.contains(metaChars);
    if (containsMeta) {
        QString quoted = escapeArgumentImplWindowsCreateProcess(arg, false);
        quoted.replace('^', "^^"); // handle itself first
        quoted.replace('(', "^(");
        quoted.replace(')', "^)");
        quoted.replace('%', "^%");
        quoted.replace('!', "^!");
        quoted.replace('"', "^\"");
        quoted.replace('<', "^<");
        quoted.replace('>', "^>");
        quoted.replace('&', "^&");
        quoted.replace('|', "^|");
        return quoted;
    } else
        return escapeArgumentImplWindowsCreateProcess(arg, false);
}

QString escapeArgument(const QString &arg, bool isFirstArg, EscapeArgumentRule rule)
{
    switch (rule) {
    case EscapeArgumentRule::BourneAgainShellPretty:
        return escapeArgumentImplBourneAgainShellPretty(arg, isFirstArg);
    case EscapeArgumentRule::BourneAgainShellFast:
        return escapeArgumentImplBourneAgainShellFast(arg);
    case EscapeArgumentRule::WindowsCreateProcess:
        return escapeArgumentImplWindowsCreateProcess(arg, false);
    case EscapeArgumentRule::WindowsCreateProcessForceQuote:
        return escapeArgumentImplWindowsCreateProcess(arg, true);
    case EscapeArgumentRule::WindowsCommandPrompt:
        return escapeArgumentImplWindowsCommandPrompt(arg);
    default:
        __builtin_unreachable();
    }
}

EscapeArgumentRule platformShellEscapeArgumentRule()
{
#ifdef Q_OS_WIN
    return EscapeArgumentRule::WindowsCommandPrompt;
#else
    return EscapeArgumentRule::BourneAgainShellPretty;
#endif
}

QString escapeArgumentForPlatformShell(const QString &arg, bool isFirstArg)
{
    return escapeArgument(arg, isFirstArg, platformShellEscapeArgumentRule());
}

QString escapeCommandForPlatformShell(const QString &prog, const QStringList &args)
{
    QStringList escapedArgs{escapeArgumentForPlatformShell(prog, true)};
    for (int i = 0; i < args.size(); ++i)
        escapedArgs << escapeArgumentForPlatformShell(args[i], false);
    return escapedArgs.join(' ');
}

EscapeArgumentRule makefileRecipeEscapeArgumentRule()
{
#ifdef Q_OS_WIN
    /* Lord knows why.

      standard CreateProcess or CMD escaping:
        child.exe -c main'.c -o main'.o
      yielding:
        0: [child.exe]
        1: [-c]
        2: [main.c -o main.o]
      that's not what we want.

      however, if CMD escaping a malformed argument
        child.exe -c main'.c -o main'.o ^"mal \^" ^& calc^"
      yielding:
        0: [child.exe]
        1: [-c]
        2: [main'.c]
        3: [-o]
        4: [main'.o]
        5: [mal " & calc]
      it works?!?!?!

      force-quoted CreateProcess escaping seems work on most cases.
    */
    return EscapeArgumentRule::WindowsCreateProcessForceQuote;
#else
    return EscapeArgumentRule::BourneAgainShellPretty;
#endif
}

QString escapeArgumentForMakefileVariableValue(const QString &arg, bool isFirstArg)
{
    static QSet<QChar> needsMfEscaping = {'#'};
    QString recipeEscaped = escapeArgument(arg, isFirstArg, makefileRecipeEscapeArgumentRule());
    QString mfEscaped = contextualBackslashEscaping(recipeEscaped, needsMfEscaping);
    return mfEscaped.replace('$', "$$");
}

QString escapeArgumentsForMakefileVariableValue(const QStringList &args)
{
    QStringList escapedArgs;
    for (int i = 0; i < args.size(); ++i)
        escapedArgs << escapeArgumentForMakefileVariableValue(args[i], false);
    return escapedArgs.join(' ');
}

QString escapeFilenameForMakefileInclude(const QString &filename)
{
    static QSet<QChar> needsEscaping{'#', ' '};
    QString result = contextualBackslashEscaping(filename, needsEscaping);
    return result.replace('$', "$$");
}

QString escapeFilenameForMakefileTarget(const QString &filename)
{
    static QSet<QChar> needsEscaping{'#', ' ', ':', '%'};
    QString result = contextualBackslashEscaping(filename, needsEscaping);
    return result.replace('$', "$$");
}

QString escapeFilenameForMakefilePrerequisite(const QString &filename)
{
    static QSet<QChar> needsEscaping{'#', ' ', ':', '?', '*'};
    QString result = contextualBackslashEscaping(filename, needsEscaping, false);
    return result.replace('$', "$$");
}

QString escapeFilenamesForMakefilePrerequisite(const QStringList &filenames)
{
    QStringList escapedFilenames;
    for (int i = 0; i < filenames.size(); ++i)
        escapedFilenames << escapeFilenameForMakefilePrerequisite(filenames[i]);
    return escapedFilenames.join(' ');
}

QString escapeArgumentForMakefileRecipe(const QString &arg, bool isFirstArg)
{
    QString shellEscaped = escapeArgument(arg, isFirstArg, makefileRecipeEscapeArgumentRule());
    return shellEscaped.replace('$', "$$");
}

QString escapeArgumentForInputField(const QString &arg, bool isFirstArg)
{
    return escapeArgument(arg, isFirstArg, EscapeArgumentRule::BourneAgainShellPretty);
}

QString escapeArgumentsForInputField(const QStringList &args)
{
    QStringList escapedArgs;
    for (int i = 0; i < args.size(); ++i)
        escapedArgs << escapeArgumentForInputField(args[i], false);
    return escapedArgs.join(' ');
}
