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

#include "parsearg.h"

namespace ParseArgumentsDetail
{

/*Before:
    'blahblah'
     ^pos
  After:
    'blahblah'
              ^pos */
QString singleQuoted(const QString &command, int &pos)
{
    QString result;
    while (pos < command.length() && command[pos] != '\'') {
        result.push_back(command[pos]);
        ++pos;
    }
    if (pos < command.length())
        ++pos; // eat closing quote
    return result;
}

// read up to 3 octal digits
QString readOctal(const QString &command, int &pos)
{
    QString result;
    for (int i = 0; i < 3; ++i) {
        if (pos < command.length() && command[pos] >= '0' && command[pos] <= '7') {
            result.push_back(command[pos]);
            ++pos;
        } else
            break;
    }
    return result;
}

// read up to maxDigits hex digits
QString readHex(const QString &command, int &pos, int maxDigits)
{
    QString result;
    for (int i = 0; i < maxDigits; ++i) {
        if (pos < command.length() && (command[pos].isDigit() || (command[pos].toLower() >= 'a' && command[pos].toLower() <= 'f'))) {
            result.push_back(command[pos]);
            ++pos;
        } else
            break;
    }
    return result;
}

/*Case 1: braced variable name (ingore POSIX operators, no nested braces)
    Before:
      ${VARNAME.abc$}
       ^pos
    After:
      ${VARNAME.abc$}
                     ^pos
    Returns: value of `VARNAME.abc$`, "" if not found
  Case 2: command or arithmetic (no expansion, nested parentheses ok)
    Before:
      $(echo 1)
       ^pos
      $((1+1))
       ^pos
    After:
      $(echo 1)
               ^pos
      $((1+1))
              ^pos
    Returns: as is
  Case 3: ANSI-C quoting
    Before:
      $'blah\nblah\'blah'
       ^pos
    After:
      $'blah\nblah\'blah'
                         ^pos
    Returns: unescaped string
  Case 4: normal variable name
    Before:
      $VAR_NAME-x
       ^pos
    After:
      $VAR_NAME-x
               ^pos
    Returns: value of `VAR_NAME`, "" if not found
  Case 5: all other invalid cases (though they may be valid in shell)
    Before:
      $123
       ^pos
    After:
      $123
       ^pos
    Returns: as is */
QString variableExpansion(const QString &command, int &pos, const QMap<QString, QString> &variables, bool ansiCQuotingPermitted)
{
    if (pos >= command.length())
        return "$";
    if (command[pos] == '{') {
        // case 1, read to closing brace
        QString varName;
        QString result;
        ++pos; // eat opening brace
        while (pos < command.length() && command[pos] != '}') {
            varName.push_back(command[pos]);
            ++pos;
        }
        if (pos < command.length()) {
            ++pos; // eat closing brace
            if (variables.contains(varName))
                return variables[varName];
            else
                return {};
        } else {
            // unterminated
            return {};
        }
    } else if (command[pos] == '(') {
        // case 2, read to matching closing paren
        QString result = "$(";
        ++pos; // eat opening paren
        int level = 1;
        while (pos < command.length() && level > 0) {
            if (command[pos] == '(')
                ++level;
            else if (command[pos] == ')')
                --level;
            result.push_back(command[pos]);
            ++pos;
        }
        return result;
    } else if (ansiCQuotingPermitted && command[pos] == '\'') {
        // case 3, parse ANSI-C quoting
        QByteArray unescaped;
        ++pos; // eat opening quote
        while (pos < command.length()) {
            if (command[pos] == '\\') {
                ++pos;
                if (pos < command.length()) {
                    switch (command[pos].unicode()) {
                    case 'a':
                        ++pos;
                        unescaped.push_back('\a');
                        break;
                    case 'b':
                        ++pos;
                        unescaped.push_back('\b');
                        break;
                    case 'e':
                    case 'E':
                        ++pos;
                        unescaped.push_back('\x1B');
                        break;
                    case 'f':
                        ++pos;
                        unescaped.push_back('\f');
                        break;
                    case 'n':
                        ++pos;
                        unescaped.push_back('\n');
                        break;
                    case 'r':
                        ++pos;
                        unescaped.push_back('\r');
                        break;
                    case 't':
                        ++pos;
                        unescaped.push_back('\t');
                        break;
                    case 'v':
                        ++pos;
                        unescaped.push_back('\v');
                        break;
                    case '\\':
                        ++pos;
                        unescaped.push_back('\\');
                        break;
                    case '\'':
                        ++pos;
                        unescaped.push_back('\'');
                        break;
                    case '"':
                        ++pos;
                        unescaped.push_back('"');
                        break;
                    case '?':
                        ++pos;
                        unescaped.push_back('?');
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7': {
                        QString digits = readOctal(command, pos);
                        int octal = digits.toUInt(nullptr, 8);
                        unescaped.push_back(octal);
                        break;
                    }
                    case 'x': {
                        ++pos; // eat 'x'
                        QString digits = readHex(command, pos, 2);
                        if (digits.isEmpty()) {
                            // normal character
                            unescaped.append("\\x");
                        } else {
                            int hex = digits.toUInt(nullptr, 16);
                            unescaped.push_back(hex);
                        }
                        break;
                    }
                    case 'u': {
                        ++pos; // eat 'u'
                        QString digits = readHex(command, pos, 4);
                        if (digits.isEmpty()) {
                            // normal character
                            unescaped.append("\\u");
                        } else {
                            int hex = digits.toUInt(nullptr, 16);
                            QByteArray encoded = QString(QChar(hex)).toUtf8();
                            unescaped.append(encoded);
                        }
                        break;
                    }
                    case 'U': {
                        ++pos; // eat 'U'
                        QString digits = readHex(command, pos, 8);
                        if (digits.isEmpty()) {
                            // normal character
                            unescaped.append("\\U");
                        } else {
                            int hex = digits.toUInt(nullptr, 16);
                            char32_t s = hex;
                            QByteArray encoded = QString::fromUcs4(&s, 1).toUtf8();
                            unescaped.append(encoded);
                        }
                        break;
                    }
                    default:
                        // normal character
                        unescaped.push_back('\\');
                        break;
                    }
                }
            } else if (command[pos] == '\'') {
                ++pos; // eat closing quote
                return unescaped;
            } else {
                QChar c = command[pos];
                QByteArray encoded = QString(c).toUtf8();
                unescaped.append(encoded);
                ++pos;
            }
        }
        // unterminated
        return unescaped;
    } else if (command[pos].isLetter() || command[pos] == '_') {
        // case 4, read variable name
        QString varName;
        while (pos < command.length() && (command[pos].isLetterOrNumber() || command[pos] == '_')) {
            varName.push_back(command[pos]);
            ++pos;
        }
        if (variables.contains(varName))
            return variables[varName];
        else
            return {};
    } else {
        // case 5, return as is
        return "$";
    }
}

/*Before:
    <VARNAME.abc$>
     ^pos
  After:
    <VARNAME.abc$>
                  ^pos */
QString devCppExpansion(const QString &command, int &pos, const QMap<QString, QString> &variables)
{
    QString varName;
    QString result;
    while (pos < command.length() && command[pos] != '>') {
        varName.push_back(command[pos]);
        ++pos;
    }
    if (pos < command.length()) {
        ++pos; // eat closing bracket
        if (variables.contains(varName))
            return variables[varName];
        else
            // not a variable
            return '<' + varName + '>';
    } else {
        // unterminated, treat it as a normal string
        return '<' + varName;
    }
}

/*Before:
    "blah\"blah"
     ^pos
  After:
    "blah\"blah"
                ^pos */
QString doubleQuoted(const QString &command, int &pos, const QMap<QString, QString> &variables, bool enableDevCppVariableExpansion)
{
    QString result;
    while (pos < command.length()) {
        switch (command[pos].unicode()) {
        case '$':
            ++pos; // eat '$'
            result += variableExpansion(command, pos, variables, false);
            break;
        case '\\':
            ++pos; // eat backslash
            if (pos < command.length()) {
                switch (command[pos].unicode()) {
                case '$':
                case '`':
                case '"':
                case '\\':
                    result.push_back(command[pos]);
                    ++pos;
                    break;
                case '\n':
                    ++pos; // eat newline
                    break;
                default:
                    // normal character
                    result.push_back('\\');
                }
            } else {
                // unterminated
                result.push_back('\\');
            }
            break;
        case '"':
            ++pos; // eat closing quote
            return result;
        case '<':
            if (enableDevCppVariableExpansion) {
                ++pos; // eat '<'
                result += devCppExpansion(command, pos, variables);
                break;
            }
            [[fallthrough]];
        case '`': // not supported
        default:
            result.push_back(command[pos]);
            ++pos;
        }
    }
    // unterminated
    return result;
}

} // namespace ParseArgumentsDetail

QStringList parseArguments(const QString &command, const QMap<QString, QString> &variables, bool enableDevCppVariableExpansion)
{
    using namespace ParseArgumentsDetail;

    QStringList result;
    QString current;
    bool currentPolluted = false;

    int pos = 0;
    while (pos < command.length()) {
        switch (command[pos].unicode()) {
        case ' ':
        case '\t':
        case '\n':
            if (currentPolluted) {
                result.push_back(current);
                current.clear();
                currentPolluted = false;
            }
            ++pos;
            break;
        case '#':
            if (currentPolluted) {
                // normal character
                current.push_back(command[pos]);
                ++pos;
            } else {
                // comment, eat to newline
                while (pos < command.length() && command[pos] != '\n')
                    ++pos;
            }
            break;
        case '\'':
            ++pos; // eat opening quote
            current += singleQuoted(command, pos);
            currentPolluted = true;
            break;
        case '"':
            ++pos; // eat opening quote
            current += doubleQuoted(command, pos, variables, enableDevCppVariableExpansion);
            currentPolluted = true;
            break;
        case '$':
            ++pos; // eat '$'
            current += variableExpansion(command, pos, variables, true);
            currentPolluted = true;
            break;
        case '\\':
            ++pos; // eat backslash
            if (pos < command.length()) {
                if (command[pos] != '\n')
                    ++pos; // eat newline
                else {
                    // normal character
                    current.push_back(command[pos]);
                }
                ++pos;
                currentPolluted = true;
            } else {
                // unterminated
                current.push_back('\\');
            }
            break;
        case '<':
            if (enableDevCppVariableExpansion) {
                ++pos; // eat '<'
                current += devCppExpansion(command, pos, variables);
                currentPolluted = true;
                break;
            }
            [[fallthrough]];
        default:
            current.push_back(command[pos]);
            ++pos;
            currentPolluted = true;
        }
    }

    if (currentPolluted)
        result.push_back(current);

    return result;
}

QStringList parseArgumentsWithoutVariables(const QString &command)
{
    return parseArguments(command, {}, false);
}
