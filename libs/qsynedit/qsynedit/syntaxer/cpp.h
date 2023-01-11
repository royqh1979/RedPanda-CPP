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
        Asm,
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
        RawString
    };

    enum RangeState {
        rsUnknown, rsAnsiC, rsAnsiCAsm, rsAnsiCAsmBlock, rsAsm,
        rsAsmBlock, rsDirective, rsDirectiveComment, rsString,
        rsMultiLineString, rsMultiLineDirective, rsCppComment,
        rsStringEscapeSeq,
        rsRawString, rsSpace,rsRawStringNotEscaping,rsRawStringEnd,rsChar,
        rsDefineIdentifier, rsDefineRemaining
    };

public:
    explicit CppSyntaxer();
    CppSyntaxer(const CppSyntaxer&)=delete;
    CppSyntaxer operator=(const CppSyntaxer&)=delete;

    const PTokenAttribute &asmAttribute() const;

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

    TokenId getTokenId();
private:
    void andSymbolProc();
    void ansiCppProc();
    void ansiCProc();
    void asciiCharProc();
    void atSymbolProc();
    void braceCloseProc();
    void braceOpenProc();
    void colonProc();
    void commaProc();
    void directiveProc();
    void defineIdentProc();
    void defineRemainingProc();
    void directiveEndProc();
    void equalProc();
    void greaterProc();
    void identProc();
    void lowerProc();
    void minusProc();
    void modSymbolProc();
    void notSymbolProc();
    void nullProc();
    void numberProc();
    void orSymbolProc();
    void plusProc();
    void pointProc();
    void questionProc();
    void rawStringProc();
    void roundCloseProc();
    void roundOpenProc();
    void semiColonProc();
    void slashProc();
    void backSlashProc();
    void spaceProc();
    void squareCloseProc();
    void squareOpenProc();
    void starProc();
//    void stringEndProc();
    void stringEscapeSeqProc();
    void stringProc();
    void stringStartProc();
    void tildeProc();
    void unknownProc();
    void xorSymbolProc();
    void processChar();
    void popIndents(int indentType);
    void pushIndents(int indentType);

private:
    bool mAsmStart;
    SyntaxerState mRange;
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

    PTokenAttribute mAsmAttribute;
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
    void setState(const SyntaxerState& rangeState) override;
    void resetState() override;

    QString languageName() override;
    ProgrammingLanguage language() override;

    // SynHighlighter interface
public:
    SyntaxerState getState() const override;

    // SynHighlighter interface
public:
    bool isIdentChar(const QChar &ch) const override;

    // SynHighlighter interface
public:
    QSet<QString> keywords() const override;

    // SynHighlighter interface
public:
    QString foldString() override;
    const QSet<QString> &customTypeKeywords() const;
    void setCustomTypeKeywords(const QSet<QString> &newCustomTypeKeywords);

    // Highlighter interface
public:
    bool supportBraceLevel() override;
};

}

#endif // SYNEDITCPPHIGHLIGHTER_H
