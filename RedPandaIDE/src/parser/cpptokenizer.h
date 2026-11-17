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
#ifndef CPPTOKENIZER_H
#define CPPTOKENIZER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <memory>
#include "cpptypes.h"

class CppTokenizer
{
public:
    enum class TokenKind {
        Identifier,
        IntegerLiteral,
        FloatingLiteral,
        StringLiteral,
        CharacterLiteral,
        Operator,
        Preprocessor,
        FileBoundary,
        FileEnd
    };

    struct Token {
        QString text;
        SourcePosition position;
        int matchIndex = -1;
        TokenKind kind = TokenKind::Operator;
        QString fileName;
    };

    using TokenList = QVector<Token>;

    explicit CppTokenizer();
    CppTokenizer(const CppTokenizer&) = delete;
    CppTokenizer& operator=(const CppTokenizer&) = delete;

    void clear();
    void tokenize(const QStringList &buffer);
    void relocateTokens(const QString &fileName, const SourcePosition &start);
    const Token &operator[](int index) const { return mTokenList[index]; }
    int tokenCount() const { return mTokenList.count(); }
    const TokenList &tokens() const { return mTokenList; }
    const QStringList &sourceLines() const { return mBufferLines; }
    static const Token &fileEndToken() {
        static const Token token = [] {
            Token result;
            result.kind = TokenKind::FileEnd;
            return result;
        }();
        return token;
    }
    void stopForParserReset() { mStopForParserReset = true; }

private:
    void addToken(QString text, TokenKind kind, const SourcePosition &position);
    void resolveUnmatchedPairs();
    int nextFileBoundary(int tokenIndex) const;

    void skipWhitespace();
    void skipAttribute();
    void skipGnuAttribute();
    void skipQuoted(QChar quote);
    void skipRawString();
    QString readIdentifier();
    QString readNumber(TokenKind &kind);
    QString readLiteral(TokenKind &kind);
    QString readPreprocessor();
    QString readOperator();
    void readFileBoundary(const QString &text, int tokenIndex);

    bool atEnd() const { return *mCurrent == 0; }
    bool startsWith(const char *text) const;
    bool startsWith(const QString &text) const;
    void advance();
    static bool isIdentifierStart(QChar ch) { return ch == '_' || ch.isLetter(); }
    static bool isIdentifierPart(QChar ch) { return ch == '_' || ch.isLetterOrNumber(); }

private:
    QStringList mBufferLines;
    QString mBuffer;
    const QChar *mCurrent = nullptr;
    SourcePosition mCurrentPosition;
    QString mCurrentFile;
    TokenList mTokenList;
    QVector<int> mUnmatchedBraces;
    QVector<int> mUnmatchedBrackets;
    QVector<int> mUnmatchedParentheses;
    QVector<int> mFileBoundaries;
    bool mStopForParserReset = false;
};

Q_DECLARE_TYPEINFO(CppTokenizer::Token, Q_RELOCATABLE_TYPE);

using PCppTokenizer = std::shared_ptr<CppTokenizer>;

#endif // CPPTOKENIZER_H
