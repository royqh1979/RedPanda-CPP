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
public:
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
        rsUnknown, rsAnsiC, rsDirective, rsDirectiveComment,
        rsString, rsStringNextLine, rsStringUnfinished,
        rsMultiLineString, rsMultiLineDirective, rsCppComment,
        rsDocstring,
        rsStringEscapeSeq,
        rsRawString, rsSpace,rsRawStringNotEscaping,rsRawStringEnd,
        rsChar, rsCharEscaping,
        rsDefineIdentifier, rsDefineRemaining,
    };

    explicit CppSyntaxer();
    CppSyntaxer(const CppSyntaxer&)=delete;
    CppSyntaxer operator=(const CppSyntaxer&)=delete;

    const PTokenAttribute &preprocessorAttribute() const { return mPreprocessorAttribute; }

    const PTokenAttribute &invalidAttribute() const { return mInvalidAttribute; }

    const PTokenAttribute &numberAttribute() const { return mNumberAttribute; }

    const PTokenAttribute &floatAttribute() const { return mFloatAttribute; }

    const PTokenAttribute &hexAttribute() const { return mHexAttribute; }

    const PTokenAttribute &octAttribute() const { return mOctAttribute; }

    const PTokenAttribute &stringEscapeSequenceAttribute() const { return mStringEscapeSequenceAttribute; }

    const PTokenAttribute &charAttribute() const { return mCharAttribute; }

    const PTokenAttribute &variableAttribute() const { return mVariableAttribute; }

    const PTokenAttribute &functionAttribute() const { return mFunctionAttribute; }

    const PTokenAttribute &classAttribute() const { return mClassAttribute; }

    const PTokenAttribute &globalVarAttribute() const { return mGlobalVarAttribute; }

    const PTokenAttribute &localVarAttribute() const { return mLocalVarAttribute; }

    static const QSet<QString> Keywords;

    static const QSet<QString> ValidIntegerSuffixes;

    static const QSet<QString> StandardAttributes;

    bool isStringToNextLine(int state) { return state == RangeState::rsStringNextLine; }
    bool isRawStringStart(int state) { return state == RangeState::rsRawString; }
    bool isRawStringNoEscape(int state) { return state == RangeState::rsRawStringNotEscaping; }
    bool isRawStringEnd(int state) { return state == RangeState::rsRawStringEnd; }
    bool isCharNotFinished(int state) { return state == RangeState::rsChar || state == RangeState::rsCharEscaping; }
    bool isCharEscaping(int state) { return state == RangeState::rsCharEscaping; }
    bool isInAttribute(const SyntaxState &state);

    TokenId getTokenId() { return mTokenId; }
private:
    void procAndSymbol();
    void procCppStyleComment();
    void procDocstring();
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

    // Syntaxer interface
public:
    bool isCommentNotFinished(int state) const override;
    bool isStringNotFinished(int state) const override;
    bool isDocstringNotFinished(int state) const override;
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

    SyntaxState getState() const override;
    bool isIdentChar(const QChar &ch) const override;
    bool isIdentStartChar(const QChar &ch) const override;
    QSet<QString> keywords() override;
    QString foldString(QString startLine) override;
    const QSet<QString> &customTypeKeywords() const;
    void setCustomTypeKeywords(const QSet<QString> &newCustomTypeKeywords);

    bool supportBraceLevel() override;

    QString commentSymbol() override;
    QString blockCommentBeginSymbol() override;
    QString blockCommentEndSymbol() override;
    virtual bool supportFolding() override;
    virtual bool needsLineState() override;
};

}

#endif // QSYNEDIT_CPP_SYNTAXER_H
