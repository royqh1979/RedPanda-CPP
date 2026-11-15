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
#include "pptoken.h"

PpTokenizer::PpTokenizer(const QStringList &lines, int startLine, int startCol)
    : mLines(&lines), mLine(startLine), mCol(startCol)
{
}

QString PpTokenizer::prefixBefore(const PpToken &token) const
{
    if (mLine < 0 || mLine >= mLines->count())
        return QString();
    int prefixLength = mCol - token.text.length() - token.whitespaceBefore.length();
    return (*mLines)[mLine].left(qMax(0, prefixLength));
}

QString PpTokenizer::lineText(int line) const
{
    return line >= 0 && line < mLines->count() ? (*mLines)[line] : QString();
}

PpToken PpTokenizer::makeSimple(PpTokenKind kind, const QString &text) const
{
    PpToken token;
    token.kind = kind;
    token.text = text;
    token.whitespaceBefore = mWhitespace;
    return token;
}

PpToken PpTokenizer::next()
{
    while (true) {
        if (mLine >= mLines->count())
            return makeSimple(PpTokenKind::EndOfInput, QString());
        const QString& line = (*mLines)[mLine];
        if (mCol >= line.length()) {
            // end of the physical line; a raw string state survives the
            // boundary and continues on the next line. Whitespace is
            // carried by the boundary token (whitespace-only lines and
            // trailing whitespace must survive).
            mLine++;
            mCol = 0;
            PpToken token = makeSimple(PpTokenKind::EndOfLine, QString());
            mWhitespace.clear();
            return token;
        }
        if (mState == State::Raw) {
            // raw string content: consume verbatim up to the closing
            // )delimiter" sequence, or to the end of the line
            QString terminator = ")" + mRawDelimiter + "\"";
            int end = line.indexOf(terminator, mCol);
            QString text;
            if (end < 0) {
                text = line.mid(mCol);
                mCol = line.length();
            } else {
                text = line.mid(mCol, end - mCol + terminator.length());
                mCol = end + terminator.length();
                mState = State::Other;
            }
            PpToken token = makeSimple(PpTokenKind::RawStringFragment, text);
            mWhitespace.clear();
            return token;
        }
        // accumulate whitespace
        if (line[mCol] == ' ' || line[mCol] == '\t') {
            int start = mCol++;
            while (mCol < line.length()
                   && (line[mCol] == ' ' || line[mCol] == '\t'))
                ++mCol;
            mWhitespace += line.mid(start, mCol - start);
            continue;
        }
        break;
    }

    const QString& line = (*mLines)[mLine];
    QChar ch = line[mCol];
    PpToken token;
    if (isIdentStartChar(ch)) {
        token = readWord(line);
        // raw string literal: R"delim(...)delim"
        // (only a bare R, not an R inside an identifier)
        if (token.text == QLatin1String("R")
                && mCol < line.length() && line[mCol] == '"') {
            int p = mCol + 1;
            QString delimiter;
            while (p < line.length() && line[p] != '('
                   && delimiter.length() <= 16) {
                delimiter += line[p];
                p++;
            }
            if (p < line.length() && line[p] == '(') {
                token.kind = PpTokenKind::RawStringFragment;
                token.text = line.mid(mCol - 1, p - mCol + 2); // R"delim(
                mCol = p + 1;
                mState = State::Raw;
                mRawDelimiter = delimiter;
            }
            // otherwise ill-formed; keep it an identifier token
        }
    } else if (ch.isDigit()) {
        token = readNumber(line);
    } else if (ch == '"' || ch == '\'') {
        token = readStringOrChar(line, ch);
    } else if (ch == '#' && mCol + 1 < line.length() && line[mCol + 1] == '#') {
        token = makeSimple(PpTokenKind::Punct, QStringLiteral("##"));
        mCol += 2;
    } else {
        token = makeSimple(PpTokenKind::Punct, QString(ch));
        mCol++;
    }
    mWhitespace.clear();
    return token;
}

PpToken PpTokenizer::readWord(const QString &line)
{
    int start = mCol;
    while (mCol < line.length() && isWordChar(line[mCol]))
        mCol++;
    return makeSimple(PpTokenKind::Identifier, line.mid(start, mCol - start));
}

PpToken PpTokenizer::readNumber(const QString &line)
{
    // pp-number: digits plus letters/underscore/dot, and a sign after
    // an exponent marker (e/E/p/P)
    int start = mCol;
    while (mCol < line.length()) {
        QChar ch = line[mCol];
        if (isWordChar(ch) || ch == '.') {
            mCol++;
        } else if ((ch == '+' || ch == '-')
                   && mCol > start
                   && (line[mCol - 1] == 'e' || line[mCol - 1] == 'E'
                       || line[mCol - 1] == 'p' || line[mCol - 1] == 'P')) {
            mCol++;
        } else {
            break;
        }
    }
    return makeSimple(PpTokenKind::Number, line.mid(start, mCol - start));
}

PpToken PpTokenizer::readStringOrChar(const QString &line, QChar quote)
{
    // strings and character literals do not cross line boundaries; an
    // unterminated literal runs to the end of the line
    int start = mCol;
    mCol++; // opening quote
    while (mCol < line.length()) {
        QChar ch = line[mCol];
        if (ch == '\\' && mCol + 1 < line.length()) {
            mCol += 2;
            continue;
        }
        mCol++;
        if (ch == quote)
            break;
    }
    PpTokenKind kind = (quote == '"')
            ? PpTokenKind::StringLiteral
            : PpTokenKind::CharLiteral;
    return makeSimple(kind, line.mid(start, mCol - start));
}

static QVector<PpToken> tokenizeAll(const QString& text)
{
    QStringList lines{text};
    PpTokenizer tokenizer(lines);
    QVector<PpToken> tokens;
    while (true) {
        PpToken token = tokenizer.next();
        if (token.kind == PpTokenKind::EndOfInput)
            break;
        if (token.kind == PpTokenKind::EndOfLine)
            continue;
        tokens.append(token);
    }
    return tokens;
}

QStringList splitMacroParams(const QString &args)
{
    QStringList result;
    QVector<PpToken> tokens = tokenizeAll(args);
    int depth = 0;
    QString current;
    bool hasContent = false;
    bool sawComma = false;
    auto flush = [&]() {
        result.append(current.trimmed());
        current.clear();
        hasContent = false;
    };
    for (const PpToken& token : tokens) {
        if (token.kind == PpTokenKind::Punct) {
            if (token.text == "(" || token.text == "{"
                    || token.text == "[") {
                depth++;
            } else if (token.text == ")" || token.text == "}"
                       || token.text == "]") {
                depth--;
            } else if (token.text == "," && depth == 0) {
                flush();
                sawComma = true;
                continue;
            }
        }
        if (hasContent)
            current += token.whitespaceBefore;
        current += token.text;
        hasContent = true;
    }
    if (hasContent || sawComma)
        flush();
    return result;
}

ReplTokenList compileReplacementList(const QString &value, const QStringList &argNames)
{
    ReplTokenList result;
    QVector<PpToken> tokens = tokenizeAll(value);
    bool pendingStringize = false;
    QString stringizeWhitespace;
    bool pendingPaste = false;
    int varOptDepth = -1; // >= 0 while inside __VA_OPT__( ... )
    for (const PpToken& token : tokens) {
        if (token.kind == PpTokenKind::Punct && token.text == "#") {
            pendingStringize = true;
            stringizeWhitespace = token.whitespaceBefore;
            pendingPaste = false;
            continue;
        }
        if (token.kind == PpTokenKind::Punct && token.text == "##") {
            pendingPaste = true;
            pendingStringize = false;
            continue;
        }
        // __VA_OPT__(content): emits content only when the variadic
        // arguments are non-empty (C++20)
        if (token.kind == PpTokenKind::Identifier
                && token.text == QLatin1String("__VA_OPT__")) {
            ReplToken entry;
            entry.kind = ReplToken::Kind::VarOptBegin;
            result.append(entry);
            varOptDepth = 0;
            pendingStringize = false;
            pendingPaste = false;
            continue;
        }
        if (varOptDepth >= 0 && token.kind == PpTokenKind::Punct) {
            // inside __VA_OPT__( ... ): watch for the closing paren
            if (token.text == "(") {
                varOptDepth++;
                if (varOptDepth == 1) {
                    // the opening paren of __VA_OPT__( itself
                    continue;
                }
            } else if (token.text == ")") {
                varOptDepth--;
                if (varOptDepth == 0) {
                    ReplToken entry;
                    entry.kind = ReplToken::Kind::VarOptEnd;
                    result.append(entry);
                    pendingStringize = false;
                    pendingPaste = false;
                    continue;
                }
            }
        }
        int argIndex = -1;
        if (token.kind == PpTokenKind::Identifier) {
            argIndex = argNames.indexOf(token.text);
            if (argIndex < 0 && token.text == QLatin1String("__VA_ARGS__"))
                argIndex = argNames.indexOf("...");
        }
        if (argIndex >= 0) {
            ReplToken entry;
            entry.kind = ReplToken::Kind::Argument;
            entry.argIndex = argIndex;
            entry.stringize = pendingStringize;
            entry.pasteBefore = pendingPaste;
            // a pasted token merges with the previous output, so any
            // whitespace before it is dropped
            entry.whitespaceBefore = pendingPaste ? QString()
                    : (pendingStringize ? stringizeWhitespace
                                        : token.whitespaceBefore);
            result.append(entry);
        } else {
            // every body token becomes its own Text entry, so that
            // identifiers stay re-expandable during rescan
            ReplToken entry;
            entry.kind = ReplToken::Kind::Text;
            entry.token = token;
            entry.pasteBefore = pendingPaste;
            if (pendingPaste)
                entry.token.whitespaceBefore.clear();
            result.append(entry);
        }
        pendingStringize = false;
        pendingPaste = false;
    }
    return result;
}
