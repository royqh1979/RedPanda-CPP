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
#ifndef CPPTOKENIZER_H
#define CPPTOKENIZER_H

#include <QObject>
#include "parserutils.h"

class CppTokenizer
{
    enum class TokenType {
        Normal,
        LeftBrace,
        RightBrace,
        LeftParenthesis,
        RightParenthesis,
        LeftBracket,
        RightBracket,
        Assignment,
        LambdaCaptures
    };

public:
    struct Token {
      QString text;
      int line;
      int matchIndex;
    };
    using PToken = std::shared_ptr<Token>;
    using TokenList = QVector<PToken>;
    explicit CppTokenizer();
    CppTokenizer(const CppTokenizer&)=delete;
    CppTokenizer& operator=(const CppTokenizer&)=delete;

    void clear();
    void tokenize(const QStringList& buffer);
    void dumpTokens(const QString& fileName);
    const PToken& operator[](int i) const { return mTokenList[i]; }
    int tokenCount() const { return mTokenList.count(); }
    static bool isIdentChar(const QChar& ch) { return ch=='_' || ch.isLetter(); }
    int lambdasCount() const { return mLambdas.count(); }

    int indexOfFirstLambda() const { return mLambdas.front(); }
    void removeFirstLambda() { mLambdas.pop_front(); }

private:
    void addToken(const QString& sText, int iLine, TokenType tokenType);
    void advance();
    void countLines();

//    QString getForInit();
    QString getNextToken(
            TokenType *pTokenType);
    QString getNumber();
    QString getPreprocessor();
    QString getWord();
    bool isArguments() { return *mCurrent == '('; }
//    bool isForInit();
    bool isNumber() { return isDigitChar(*mCurrent); }
    bool isPreprocessor() { return *mCurrent=='#'; }
    bool isWord() { return isIdentChar(*mCurrent) && (*(mCurrent+1) != '"') && (*(mCurrent+1) != '\''); }
    void simplify(QString& output);
    void simplifyArgs(QString& output);
//    void skipAssignment();
    void skipDoubleQuotes();
    void skipPair(const QChar& cStart, const QChar cEnd);
    void skipParenthesis();
    bool skipAngleBracketPair();
    void skipRawString();
    void skipSingleQuote();
    void skipSplitLine();
    void skipTemplateArgs();
    void skipToEOL();
    void skipToNextToken();
    bool openFile(const QString& fileName);
    static bool isLetterChar(const QChar& ch) {
        return isIdentChar(ch)
                    || ch == '_'
                    || ch == '*'
                    || ch == '&'
                    || ch == '~';
    }
    static bool isHexChar(const QChar& ch) {
        return (ch >= 'A' && ch<='F')
                || (ch>='a' && ch<='f')
                || ch == 'x'
                || ch == 'L';
    }
    static bool isDigitChar(const QChar& ch) {
        return (ch>='0' && ch<='9');
    }

    static bool isSpaceChar(const QChar& ch) {
        return (ch == ' ' || ch == '\t');
    }

    static bool isLineChar(const QChar& ch) {
        return (ch=='\n' || ch=='\r');
    }

    static bool isBlankChar(const QChar& ch) {
        return (ch.unicode() <= 32) && (ch.unicode() > 0);
    }

//    static bool isOperatorChar(const QChar& ch) {
//        switch (ch.unicode()) {
//        case '+':
//        case '-':
//        case '/':
//        case '*':
//        case '[':
//        case ']':
//        case '=':
//        case '%':
//        case '!':
//        case '&':
//        case '|':
//        case '>':
//        case '<':
//        case '^':
//            return true;
//        default:
//            return false;
//        }
//    }

    static bool currentWordEquals(QChar* wordStart, QChar *wordEnd, const QString& text) {
        QString currentWord(wordStart, wordEnd-wordStart);
        return currentWord == text;
    }

private:
    QStringList mBuffer;
    QString mBufferStr;
    const QChar* mStart;
    const QChar* mCurrent;
    const QChar* mLineCount;
    int mCurrentLine;
    QString mLastToken;
    TokenList mTokenList;
    QList<int> mLambdas;
    QVector<int> mUnmatchedBraces; // stack of indices for unmatched '{'
    QVector<int> mUnmatchedBrackets; // stack of indices for unmatched '['
    QVector<int> mUnmatchedParenthesis;// stack of indices for unmatched '('
};

using PCppTokenizer = std::shared_ptr<CppTokenizer>;

#endif // CPPTOKENIZER_H
