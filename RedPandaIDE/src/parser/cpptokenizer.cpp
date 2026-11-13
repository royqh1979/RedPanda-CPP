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

#include <QStringList>

CppTokenizer::CppTokenizer() = default;

void CppTokenizer::clear()
{
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
    clear();
    mBuffer = buffer.join('\n');
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
        if (startsWith("[[")) {
            skipAttribute();
            continue;
        }

        if (*mCurrent == '#') {
            QString text = readPreprocessor();
            TokenKind kind = text.startsWith("#include ")
                    ? TokenKind::FileBoundary : TokenKind::Preprocessor;
            addToken(text, kind, position);
        } else if (*mCurrent == '"' || *mCurrent == '\''
                   || startsWith("R\"") || startsWith("LR\"")
                   || startsWith("uR\"") || startsWith("UR\"")
                   || startsWith("u8R\"") || startsWith("L\"")
                   || startsWith("u\"") || startsWith("U\"")
                   || startsWith("u8\"") || startsWith("L'")
                   || startsWith("u'") || startsWith("U'")
                   || startsWith("u8'")) {
            TokenKind kind = TokenKind::StringLiteral;
            QString text = readLiteral(kind);
            addToken(text, kind, position);
        } else if (isIdentifierStart(*mCurrent)) {
            addToken(readIdentifier(), TokenKind::Identifier, position);
        } else if (mCurrent->isDigit()
                   || (*mCurrent == '.' && (mCurrent + 1)->isDigit())) {
            TokenKind kind = TokenKind::IntegerLiteral;
            QString text = readNumber(kind);
            addToken(text, kind, position);
        } else {
            addToken(readOperator(), TokenKind::Operator, position);
        }
    }
    resolveUnmatchedPairs();
}

void CppTokenizer::addToken(const QString &text, TokenKind kind, const SourcePosition &position)
{
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
    token.text = text;
    token.position = position;
    token.kind = kind;
    token.fileName = mCurrentFile;
    token.matchIndex = opening;
    mTokenList.append(token);

    if (kind == TokenKind::FileBoundary) {
        readFileBoundary(text, index);
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
    while (true) {
        if (mCurrent->isSpace()) {
            advance();
        } else if (startsWith("//")) {
            while (!atEnd() && *mCurrent != '\n' && *mCurrent != '\r')
                advance();
        } else if (startsWith("/*")) {
            advance();
            advance();
            while (!atEnd() && !startsWith("*/"))
                advance();
            if (startsWith("*/")) {
                advance();
                advance();
            }
        } else {
            break;
        }
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
    int start = mCurrent - mBuffer.constData();
    while (isIdentifierPart(*mCurrent))
        advance();
    return mBuffer.mid(start, mCurrent - mBuffer.constData() - start);
}

QString CppTokenizer::readNumber(TokenKind &kind)
{
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
    int start = mCurrent - mBuffer.constData();
    while (!atEnd()) {
        if (*mCurrent == '\n' || *mCurrent == '\r') {
            int beforeNewline = mCurrent - mBuffer.constData();
            if (beforeNewline > start && mBuffer[beforeNewline - 1] == '\\') {
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
    static const QStringList operators{
        "<<=", ">>=", "<=>", "...", "->*", "##",
        "::", "++", "--", "+=", "-=", "*=", "/=", "%=", "==", "!=",
        "<=", ">=", "&&", "||", "&=", "|=", "^=", "<<", ">>", "->", ".*"
    };
    for (const QString &text : operators) {
        if (startsWith(text)) {
            for (int i = 0; i < text.length(); ++i)
                advance();
            return text;
        }
    }
    QString result(*mCurrent);
    advance();
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

bool CppTokenizer::startsWith(const QString &text) const
{
    for (int i = 0; i < text.length(); ++i) {
        if (mCurrent[i] == 0 || mCurrent[i] != text[i])
            return false;
    }
    return true;
}

void CppTokenizer::advance()
{
    if (*mCurrent == '\n') {
        ++mCurrentPosition.line;
        mCurrentPosition.ch = 0;
    } else {
        ++mCurrentPosition.ch;
    }
    if (!atEnd())
        ++mCurrent;
}
