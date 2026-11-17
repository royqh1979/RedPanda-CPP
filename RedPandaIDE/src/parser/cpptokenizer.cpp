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
#include "cpptokenizer.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QStringList>
#include <utility>

namespace {
struct TokenizerProfile {
    bool enabled = qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE");
    const bool traceEnabled = qEnvironmentVariableIsSet("REDPANDA_PARSER_TRACE");
    qint64 identifierNs = 0;
    qint64 numberNs = 0;
    qint64 literalNs = 0;
    qint64 preprocessorNs = 0;
    qint64 operatorNs = 0;
    qint64 addTokenNs = 0;
    qint64 startsWithNs = 0;
    qint64 advanceNs = 0;
    qint64 identifierCalls = 0;
    qint64 numberCalls = 0;
    qint64 literalCalls = 0;
    qint64 preprocessorCalls = 0;
    qint64 operatorCalls = 0;
    qint64 addTokenCalls = 0;
    qint64 startsWithCalls = 0;
    qint64 advanceCalls = 0;
    QHash<QString, qint64> startsWithTextCalls;
    QHash<QString, qint64> tokenizeTextCalls;
    void reset()
    {
        identifierNs = numberNs = literalNs = preprocessorNs = operatorNs = 0;
        addTokenNs = startsWithNs = advanceNs = 0;
        identifierCalls = numberCalls = literalCalls = preprocessorCalls = 0;
        operatorCalls = addTokenCalls = startsWithCalls = advanceCalls = 0;
        startsWithTextCalls.clear();
        tokenizeTextCalls.clear();
    }
    ~TokenizerProfile()
    {
        if (enabled) {
            qWarning() << "TOKENIZER_PROFILE"
                       << "identifier" << identifierNs / 1000000.0
                       << "number" << numberNs / 1000000.0
                       << "literal" << literalNs / 1000000.0
                       << "preprocessor" << preprocessorNs / 1000000.0
                       << "operator" << operatorNs / 1000000.0
                       << "addToken" << addTokenNs / 1000000.0
                       << "startsWith" << startsWithNs / 1000000.0
                       << "advance" << advanceNs / 1000000.0
                       << "calls" << identifierCalls << numberCalls << literalCalls
                       << preprocessorCalls << operatorCalls << addTokenCalls
                       << startsWithCalls << advanceCalls;
            qWarning() << "TOKENIZER_STARTS_WITH" << startsWithTextCalls;
        }
        for (auto it = tokenizeTextCalls.constBegin(); it != tokenizeTextCalls.constEnd(); ++it)
            qWarning().noquote() << "TOKENIZE_TEXT" << it.value()
                                 << it.key().toUtf8().toBase64();
    }
};

TokenizerProfile tokenizerProfile;

void traceTokenization(const QString &text)
{
    if (tokenizerProfile.traceEnabled)
        ++tokenizerProfile.tokenizeTextCalls[text];
}

struct TokenizerProfileScope {
    TokenizerProfileScope(qint64 &target, qint64 &calls)
        : target(target), calls(calls)
    {
        if (tokenizerProfile.enabled) {
            ++calls;
            timer.start();
        }
    }
    ~TokenizerProfileScope()
    {
        if (tokenizerProfile.enabled)
            target += timer.nsecsElapsed();
    }
    qint64 &target;
    qint64 &calls;
    QElapsedTimer timer;
};
}

CppTokenizer::CppTokenizer() = default;

void CppTokenizer::clear()
{
    mBufferLines.clear();
    mBuffer.clear();
    mCurrent = nullptr;
    mCurrentPosition = SourcePosition{0, 0};
    mCurrentFile.clear();
    mTokenList.clear();
    mUnmatchedBraces.clear();
    mUnmatchedBrackets.clear();
    mUnmatchedParentheses.clear();
    mFileBoundaries.clear();
    mStopForParserReset = false;
}

void CppTokenizer::relocateTokens(const QString &fileName, const SourcePosition &start)
{
    for (Token &token : mTokenList) {
        token.fileName = fileName;
        if (token.position.line == 0)
            token.position.ch += start.ch;
        token.position.line += start.line;
    }
}

void CppTokenizer::tokenize(const QStringList &buffer)
{
    if (qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE_RESET_TOKENIZER")) {
        tokenizerProfile.reset();
        qunsetenv("REDPANDA_PARSER_PROFILE_RESET_TOKENIZER");
    }
    clear();
    mBufferLines = buffer;
    mBuffer = buffer.join('\n');
    traceTokenization(mBuffer);
    mCurrent = mBuffer.constData();

    while (true) {
        skipWhitespace();
        if (mStopForParserReset) {
            mTokenList.clear();
            return;
        }
        if (atEnd())
            break;

        SourcePosition position = mCurrentPosition;
        if (*mCurrent == '[' && startsWith("[[")) {
            skipAttribute();
            continue;
        }

        if (*mCurrent == '#') {
            QString text = readPreprocessor();
            TokenKind kind = text.startsWith("#include ")
                    ? TokenKind::FileBoundary : TokenKind::Preprocessor;
            addToken(std::move(text), kind, position);
        } else if (*mCurrent == '"' || *mCurrent == '\''
                   || ((*mCurrent == 'R' || *mCurrent == 'L'
                        || *mCurrent == 'u' || *mCurrent == 'U')
                       && (startsWith("R\"") || startsWith("LR\"")
                           || startsWith("uR\"") || startsWith("UR\"")
                           || startsWith("u8R\"") || startsWith("L\"")
                           || startsWith("u\"") || startsWith("U\"")
                           || startsWith("u8\"") || startsWith("L'")
                           || startsWith("u'") || startsWith("U'")
                           || startsWith("u8'")))) {
            TokenKind kind = TokenKind::StringLiteral;
            QString text = readLiteral(kind);
            addToken(std::move(text), kind, position);
        } else if (isIdentifierStart(*mCurrent)) {
            QString identifier = readIdentifier();
            if (identifier == "__extension__")
                continue;
            if (identifier == "__attribute__" || identifier == "__attribute"
                    || identifier == "alignas" || identifier == "_Pragma") {
                skipWhitespace();
                if (*mCurrent == '(') {
                    skipGnuAttribute();
                    continue;
                }
            }
            addToken(std::move(identifier), TokenKind::Identifier, position);
        } else if (mCurrent->isDigit()
                   || (*mCurrent == '.' && (mCurrent + 1)->isDigit())) {
            TokenKind kind = TokenKind::IntegerLiteral;
            QString text = readNumber(kind);
            addToken(std::move(text), kind, position);
        } else {
            addToken(readOperator(), TokenKind::Operator, position);
        }
    }
    resolveUnmatchedPairs();
}

void CppTokenizer::addToken(QString text, TokenKind kind, const SourcePosition &position)
{
    TokenizerProfileScope profileScope(tokenizerProfile.addTokenNs,
                                       tokenizerProfile.addTokenCalls);
    int index = mTokenList.count();
    int opening = -1;
    if (kind == TokenKind::Operator) {
        if (text == "{") {
            mUnmatchedBraces.append(index);
        } else if (text == "}" && !mUnmatchedBraces.isEmpty()) {
            opening = mUnmatchedBraces.takeLast();
            mTokenList[opening].matchIndex = index;
        } else if (text == "[") {
            mUnmatchedBrackets.append(index);
        } else if (text == "]" && !mUnmatchedBrackets.isEmpty()) {
            opening = mUnmatchedBrackets.takeLast();
            mTokenList[opening].matchIndex = index;
        } else if (text == "(") {
            mUnmatchedParentheses.append(index);
        } else if (text == ")" && !mUnmatchedParentheses.isEmpty()) {
            opening = mUnmatchedParentheses.takeLast();
            mTokenList[opening].matchIndex = index;
        }
    }

    Token token;
    token.text = std::move(text);
    token.position = position;
    token.kind = kind;
    token.fileName = mCurrentFile;
    token.matchIndex = opening;
    mTokenList.append(std::move(token));

    if (kind == TokenKind::FileBoundary) {
        readFileBoundary(mTokenList.last().text, index);
        mTokenList.last().fileName = mCurrentFile;
        mFileBoundaries.append(index);
    }
}

void CppTokenizer::resolveUnmatchedPairs()
{
    QVector<QVector<int>*> stacks{&mUnmatchedBraces, &mUnmatchedBrackets,
                                  &mUnmatchedParentheses};
    for (QVector<int> *stack : stacks) {
        for (int index : *stack)
            mTokenList[index].matchIndex = nextFileBoundary(index);
    }
}

int CppTokenizer::nextFileBoundary(int tokenIndex) const
{
    for (int boundary : mFileBoundaries) {
        if (boundary > tokenIndex)
            return boundary;
    }
    return mTokenList.count();
}

void CppTokenizer::skipWhitespace()
{
    while (mCurrent->isSpace()) {
        if (*mCurrent == '\n') {
            advance();
            continue;
        }
        const QChar *whitespaceStart = mCurrent;
        while (mCurrent->isSpace() && *mCurrent != '\n')
            ++mCurrent;
        mCurrentPosition.ch += mCurrent - whitespaceStart;
    }
}

void CppTokenizer::skipAttribute()
{
    int level = 0;
    while (!atEnd()) {
        if (startsWith("[[")) {
            ++level;
            advance();
            advance();
        } else if (startsWith("]]")) {
            --level;
            advance();
            advance();
            if (level == 0)
                return;
        } else if (*mCurrent == '"' || *mCurrent == '\'') {
            skipQuoted(*mCurrent);
        } else {
            advance();
        }
    }
}

void CppTokenizer::skipGnuAttribute()
{
    int level = 0;
    while (!atEnd()) {
        if (*mCurrent == '"' || *mCurrent == '\'') {
            skipQuoted(*mCurrent);
            continue;
        }
        if (*mCurrent == '(')
            ++level;
        else if (*mCurrent == ')')
            --level;
        advance();
        if (level == 0)
            return;
    }
}

void CppTokenizer::skipQuoted(QChar quote)
{
    advance();
    while (!atEnd() && *mCurrent != quote && *mCurrent != '\n' && *mCurrent != '\r') {
        if (*mCurrent == '\\') {
            advance();
            if (!atEnd())
                advance();
        } else {
            advance();
        }
    }
    if (*mCurrent == quote)
        advance();
}

void CppTokenizer::skipRawString()
{
    while (!atEnd() && *mCurrent != '"')
        advance();
    if (!atEnd())
        advance();

    QString delimiter;
    while (!atEnd() && *mCurrent != '(' && *mCurrent != '\n' && *mCurrent != '\r') {
        delimiter += *mCurrent;
        advance();
    }
    if (*mCurrent == '(')
        advance();

    QString ending = ")" + delimiter + "\"";
    while (!atEnd()) {
        if (startsWith(ending)) {
            for (int i = 0; i < ending.length(); ++i)
                advance();
            return;
        }
        advance();
    }
}

QString CppTokenizer::readIdentifier()
{
    TokenizerProfileScope profileScope(tokenizerProfile.identifierNs,
                                       tokenizerProfile.identifierCalls);
    int start = mCurrent - mBuffer.constData();
    const QChar *identifierStart = mCurrent;
    while (isIdentifierPart(*mCurrent))
        ++mCurrent;
    mCurrentPosition.ch += mCurrent - identifierStart;
    return mBuffer.mid(start, mCurrent - mBuffer.constData() - start);
}

QString CppTokenizer::readNumber(TokenKind &kind)
{
    TokenizerProfileScope profileScope(tokenizerProfile.numberNs,
                                       tokenizerProfile.numberCalls);
    int start = mCurrent - mBuffer.constData();
    bool hexadecimal = startsWith("0x") || startsWith("0X");
    bool floating = *mCurrent == '.';
    QChar previous;
    while (!atEnd()) {
        QChar ch = *mCurrent;
        if (ch.isLetterOrNumber() || ch == '_' || ch == '\'') {
            if ((!hexadecimal && (ch == 'e' || ch == 'E' || ch == 'f' || ch == 'F'))
                    || (hexadecimal && (ch == 'p' || ch == 'P')))
                floating = true;
            previous = ch;
            advance();
        } else if (ch == '.') {
            floating = true;
            previous = ch;
            advance();
        } else if ((ch == '+' || ch == '-')
                   && (previous == 'e' || previous == 'E'
                       || (hexadecimal && (previous == 'p' || previous == 'P')))) {
            previous = ch;
            advance();
        } else {
            break;
        }
    }
    if (floating)
        kind = TokenKind::FloatingLiteral;
    return mBuffer.mid(start, mCurrent - mBuffer.constData() - start);
}

QString CppTokenizer::readLiteral(TokenKind &kind)
{
    TokenizerProfileScope profileScope(tokenizerProfile.literalNs,
                                       tokenizerProfile.literalCalls);
    int start = mCurrent - mBuffer.constData();
    if (startsWith("'") || startsWith("L'") || startsWith("u'")
            || startsWith("U'") || startsWith("u8'"))
        kind = TokenKind::CharacterLiteral;

    bool raw = startsWith("R\"") || startsWith("LR\"") || startsWith("uR\"")
            || startsWith("UR\"") || startsWith("u8R\"");
    if (raw) {
        skipRawString();
    } else {
        QChar quote = *mCurrent;
        if (quote != '"' && quote != '\'') {
            while (!atEnd() && *mCurrent != '"' && *mCurrent != '\'')
                advance();
            quote = *mCurrent;
        }
        if (!atEnd())
            skipQuoted(quote);
    }

    while (isIdentifierPart(*mCurrent))
        advance();
    return mBuffer.mid(start, mCurrent - mBuffer.constData() - start);
}

QString CppTokenizer::readPreprocessor()
{
    TokenizerProfileScope profileScope(tokenizerProfile.preprocessorNs,
                                       tokenizerProfile.preprocessorCalls);
    int start = mCurrent - mBuffer.constData();
    while (!atEnd()) {
        if (*mCurrent == '\n' || *mCurrent == '\r') {
            int beforeNewline = mCurrent - mBuffer.constData();
            // Do not use operator[] here: it may detach mBuffer and invalidate mCurrent.
            if (beforeNewline > start && mBuffer.at(beforeNewline - 1) == '\\') {
                advance();
                continue;
            }
            break;
        }
        advance();
    }
    return mBuffer.mid(start, mCurrent - mBuffer.constData() - start);
}

QString CppTokenizer::readOperator()
{
    TokenizerProfileScope profileScope(tokenizerProfile.operatorNs,
                                       tokenizerProfile.operatorCalls);
    int start = mCurrent - mBuffer.constData();
    int length = 1;
    QChar second = mCurrent[1];
    switch (mCurrent->unicode()) {
    case '<':
        if (second == '<' || second == '=') {
            length = ((second == '<' && mCurrent[2] == '=')
                      || (second == '=' && mCurrent[2] == '>')) ? 3 : 2;
        }
        break;
    case '>':
        if (second == '>' || second == '=')
            length = (second == '>' && mCurrent[2] == '=') ? 3 : 2;
        break;
    case '.':
        if (second == '.' && mCurrent[2] == '.')
            length = 3;
        else if (second == '*')
            length = 2;
        break;
    case '-':
        if (second == '>') {
            length = mCurrent[2] == '*' ? 3 : 2;
        } else if (second == '-' || second == '=') {
            length = 2;
        }
        break;
    case ':':
    case '#':
        if (second == mCurrent[0])
            length = 2;
        break;
    case '+':
    case '&':
    case '|':
        if (second == mCurrent[0] || second == '=')
            length = 2;
        break;
    case '*':
    case '/':
    case '%':
    case '=':
    case '!':
    case '^':
        if (second == '=')
            length = 2;
        break;
    default:
        break;
    }
    QString result = mBuffer.mid(start, length);
    mCurrentPosition.ch += length;
    mCurrent += length;
    return result;
}

void CppTokenizer::readFileBoundary(const QString &text, int tokenIndex)
{
    Q_UNUSED(tokenIndex);
    int delimiter = text.lastIndexOf(':');
    if (delimiter < 0)
        return;
    mCurrentFile = text.mid(QString("#include ").length(),
                            delimiter - QString("#include ").length());
    bool ok = false;
    int line = QStringView(text.constBegin() + delimiter + 1, text.constEnd()).toInt(&ok);
    if (ok)
        mCurrentPosition = SourcePosition{line, 0};
}

bool CppTokenizer::startsWith(const char *text) const
{
    TokenizerProfileScope profileScope(tokenizerProfile.startsWithNs,
                                       tokenizerProfile.startsWithCalls);
    if (tokenizerProfile.enabled)
        ++tokenizerProfile.startsWithTextCalls[text];
    QLatin1String latinText(text);
    for (int i = 0; i < latinText.size(); ++i) {
        if (mCurrent[i] != latinText[i])
            return false;
    }
    return true;
}

bool CppTokenizer::startsWith(const QString &text) const
{
    TokenizerProfileScope profileScope(tokenizerProfile.startsWithNs,
                                       tokenizerProfile.startsWithCalls);
    if (tokenizerProfile.enabled)
        ++tokenizerProfile.startsWithTextCalls[text];
    for (int i = 0; i < text.length(); ++i) {
        if (mCurrent[i] == 0 || mCurrent[i] != text[i])
            return false;
    }
    return true;
}

void CppTokenizer::advance()
{
    TokenizerProfileScope profileScope(tokenizerProfile.advanceNs,
                                       tokenizerProfile.advanceCalls);
    if (*mCurrent == '\n') {
        ++mCurrentPosition.line;
        mCurrentPosition.ch = 0;
    } else {
        ++mCurrentPosition.ch;
    }
    if (!atEnd())
        ++mCurrent;
}
