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
#ifndef SYNEDITCPPHIGHLIGHTER_H
#define SYNEDITCPPHIGHLIGHTER_H
#include "base.h"
#include <QSet>

namespace QSynedit {

class CppHighlighter: public Highlighter
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
        rsStringEscapeSeq, rsMultiLineStringEscapeSeq,
        rsRawString, rsSpace,rsRawStringEscaping,rsRawStringNotEscaping,rsChar,
        rsCppCommentEnded, rsDefineStart, rsDefineIdentifier, rsDefineRemaining
    };

public:
    explicit CppHighlighter();

    const PHighlighterAttribute &asmAttribute() const;

    const PHighlighterAttribute &preprocessorAttribute() const;

    const PHighlighterAttribute &invalidAttribute() const;

    const PHighlighterAttribute &numberAttribute() const;

    const PHighlighterAttribute &floatAttribute() const;

    const PHighlighterAttribute &hexAttribute() const;

    const PHighlighterAttribute &octAttribute() const;

    const PHighlighterAttribute &stringEscapeSequenceAttribute() const;

    const PHighlighterAttribute &charAttribute() const;

    const PHighlighterAttribute &variableAttribute() const;

    const PHighlighterAttribute &functionAttribute() const;

    const PHighlighterAttribute &classAttribute() const;

    const PHighlighterAttribute &globalVarAttribute() const;

    const PHighlighterAttribute &localVarAttribute() const;

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
    void stringEndProc();
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
    HighlighterState mRange;
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

    PHighlighterAttribute mAsmAttribute;
    PHighlighterAttribute mPreprocessorAttribute;
    PHighlighterAttribute mInvalidAttribute;
    PHighlighterAttribute mNumberAttribute;
    PHighlighterAttribute mFloatAttribute;
    PHighlighterAttribute mHexAttribute;
    PHighlighterAttribute mOctAttribute;
    PHighlighterAttribute mStringEscapeSequenceAttribute;
    PHighlighterAttribute mCharAttribute;
    PHighlighterAttribute mVariableAttribute;
    PHighlighterAttribute mFunctionAttribute;
    PHighlighterAttribute mClassAttribute;
    PHighlighterAttribute mGlobalVarAttribute;
    PHighlighterAttribute mLocalVarAttribute;

    // SynHighligterBase interface
public:
    bool getTokenFinished() const override;
    bool isLastLineCommentNotFinished(int state) const override;
    bool isLastLineStringNotFinished(int state) const override;
    bool eol() const override;
    QString getToken() const override;
    const PHighlighterAttribute &getTokenAttribute() const override;
    int getTokenPos() override;
    void next() override;
    void setLine(const QString &newLine, int lineNumber) override;
    bool isKeyword(const QString &word) override;
    void setState(const HighlighterState& rangeState) override;
    void resetState() override;

    QString languageName() override;
    HighlighterLanguage language() override;

    // SynHighlighter interface
public:
    HighlighterState getState() const override;

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
