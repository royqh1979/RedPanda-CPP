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
#ifndef QSYNEDIT_CPP_SYNTAXER_H
#define QSYNEDIT_CPP_SYNTAXER_H
#include "syntaxer.h"
#include <QSet>

namespace QSynedit {

class CppSyntaxer: public Syntaxer
{
    enum class TokenId {
        Comment,
        Directive,
        Identifier,
        Key,
        Null,
        Number,
        Space,
        String,
        StringEscapeSeq,
        Symbol,
        Unknown,
        Char,
        Float,
        Hex,
        HexFloat,
        Octal,
        Binary,
        RawString
    };

    enum RangeState {
        rsUnknown, rsAnsiC, rsDirective, rsDirectiveComment, rsString,
        rsMultiLineString, rsMultiLineDirective, rsCppComment,
        rsStringEscapeSeq,
        rsRawString, rsSpace,rsRawStringNotEscaping,rsRawStringEnd,rsChar,
        rsDefineIdentifier, rsDefineRemaining
    };

public:
    explicit CppSyntaxer();
    CppSyntaxer(const CppSyntaxer&)=delete;
    CppSyntaxer operator=(const CppSyntaxer&)=delete;

    const PTokenAttribute &preprocessorAttribute() const;

    const PTokenAttribute &invalidAttribute() const;

    const PTokenAttribute &numberAttribute() const;

    const PTokenAttribute &floatAttribute() const;

    const PTokenAttribute &hexAttribute() const;

    const PTokenAttribute &octAttribute() const;

    const PTokenAttribute &stringEscapeSequenceAttribute() const;

    const PTokenAttribute &charAttribute() const;

    const PTokenAttribute &variableAttribute() const;

    const PTokenAttribute &functionAttribute() const;

    const PTokenAttribute &classAttribute() const;

    const PTokenAttribute &globalVarAttribute() const;

    const PTokenAttribute &localVarAttribute() const;

    static const QSet<QString> Keywords;

    static const QSet<QString> ValidIntegerSuffixes;

    TokenId getTokenId();
private:
    void procAndSymbol();
    void procCppStyleComment();
    void procAnsiCStyleComment();
    void procAsciiChar();
    void procBraceClose();
    void procBraceOpen();
    void procColon();
    void procComma();
    void procDirective();
    void procDefineIdent();
    void procDefineRemaining();
    void procDirectiveEnd();
    void procEqual();
    void procGreater();
    void procIdentifier();
    void procLower();
    void procMinus();
    void procMod();
    void procNot();
    void procNull();
    void procNumber(bool isFloat=false);
    void procDecNumber();
    void procHexNumber();
    void procOctNumber();
    void procBinNumber();
    void procNumberSuffix();
    void procIntegerSuffix();
    void procFloatSuffix();

    void procOr();
    void procPlus();
    void procPoint();
    void procQuestion();
    void procRawString();
    void procRoundClose();
    void procRoundOpen();
    void procSemiColon();
    void procSlash();
    void procBackSlash();
    void procSpace();
    void procSquareClose();
    void procSquareOpen();
    void procStar();
//    void stringEndProc();
    void procStringEscapeSeq();
    void procString();
    void procStringStart();
    void procTilde();
    void procUnknown();
    void procXor();
    void processChar();
    void popIndents(IndentType indentType);
    void pushIndents(IndentType indentType, int line=-1);

private:
    SyntaxState mRange;
//    SynRangeState mSpaceRange;
    QString mLine;
    int mLineSize;
    int mRun;
    int mStringLen;
    int mToIdent;
    int mTokenPos;
    TokenId mTokenId;
    int mLineNumber;
    int mLeftBraces;
    int mRightBraces;

    QSet<QString> mCustomTypeKeywords;

    PTokenAttribute mPreprocessorAttribute;
    PTokenAttribute mInvalidAttribute;
    PTokenAttribute mNumberAttribute;
    PTokenAttribute mFloatAttribute;
    PTokenAttribute mHexAttribute;
    PTokenAttribute mOctAttribute;
    PTokenAttribute mStringEscapeSequenceAttribute;
    PTokenAttribute mCharAttribute;
    PTokenAttribute mVariableAttribute;
    PTokenAttribute mFunctionAttribute;
    PTokenAttribute mClassAttribute;
    PTokenAttribute mGlobalVarAttribute;
    PTokenAttribute mLocalVarAttribute;

    // SynHighligterBase interface
public:
    bool getTokenFinished() const override;
    bool isLastLineCommentNotFinished(int state) const override;
    bool isLastLineStringNotFinished(int state) const override;
    bool eol() const override;
    QString getToken() const override;
    const PTokenAttribute &getTokenAttribute() const override;
    int getTokenPos() override;
    void next() override;
    void setLine(const QString &newLine, int lineNumber) override;
    bool isKeyword(const QString &word) override;
    void setState(const SyntaxState& rangeState) override;
    void resetState() override;

    QString languageName() override;
    ProgrammingLanguage language() override;

    // SynHighlighter interface
public:
    SyntaxState getState() const override;

    // SynHighlighter interface
public:
    bool isIdentChar(const QChar &ch) const override;

    // SynHighlighter interface
public:
    QSet<QString> keywords() override;

    // SynHighlighter interface
public:
    QString foldString(QString startLine) override;
    const QSet<QString> &customTypeKeywords() const;
    void setCustomTypeKeywords(const QSet<QString> &newCustomTypeKeywords);

    // Highlighter interface
public:
    bool supportBraceLevel() override;

    // Syntaxer interface
public:
    QString commentSymbol() override;
    QString blockCommentBeginSymbol() override;
    QString blockCommentEndSymbol() override;
};

}

#endif // SYNEDITCPPHIGHLIGHTER_H
