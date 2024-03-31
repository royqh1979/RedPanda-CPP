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
#ifndef QSYNEDIT_ASM_SYNTAXER_H
#define QSYNEDIT_ASM_SYNTAXER_H

#include "syntaxer.h"

namespace QSynedit {

class ASMSyntaxer : public Syntaxer
{
    enum class TokenId {
        Comment,
        Identifier,
        Instruction,        // add mov etc
        Directive, // .section .data etc
        Label,
        Register, //EAX EBX etc
        Null,
        Number,
        Space,
        String,
        Symbol,
        Unknown
    };
    enum class IdentPrefix {
        None,
        Period,
        Percent
    };

public:
    explicit ASMSyntaxer(bool isATT=false, bool isCppMixed=false);
    ASMSyntaxer(const ASMSyntaxer&)=delete;
    ASMSyntaxer& operator=(const ASMSyntaxer&)=delete;

    const PTokenAttribute &numberAttribute() const;
    const PTokenAttribute &directiveAttribute() const;
    const PTokenAttribute &labelAttribute() const;
    const PTokenAttribute &registerAttribute() const;

    static QMap<QString,QString> Instructions;
    static QSet<QString> InstructionNames;
    static const QSet<QString> Registers;
    static const QSet<QString> ATTRegisters;
    static const QSet<QString> Directives;
    static const QSet<QString> ATTDirectives;
private:
    QChar* mLine;
    QString mLineString;
    int mLineNumber;
    int mRun;
    int mStringLen;
    QChar mToIdent;
    int mTokenPos;
    TokenId mTokenID;
    bool mHasTrailingSpaces;
    PTokenAttribute mNumberAttribute;
    PTokenAttribute mDirectiveAttribute;
    PTokenAttribute mRegisterAttribute;
    PTokenAttribute mLabelAttribute;
    bool mATT;
    bool mCppMixed;
    QSet<QString> mKeywordsCache;

private:
    void CommentProc();
    void CRProc();
    void GreaterProc();
    void IdentProc(IdentPrefix prefix);
    void LFProc();
    void LowerProc();
    void NullProc();
    void NumberProc();
    void SingleQuoteStringProc();
    void SlashProc();
    void SpaceProc();
    void StringProc();
    void SymbolProc();
    void UnknownProc();
    bool isIdentStartChar(const QChar& ch) const override;
    static void initData();

    // SynHighlighter interface
public:
    bool eol() const override;

    QString languageName() override;
    ProgrammingLanguage language() override;
    QString getToken() const override;
    const PTokenAttribute &getTokenAttribute() const override;
    int getTokenPos() override;
    void next() override;
    void setLine(const QString &newLine, int lineNumber) override;

    bool isCommentNotFinished(int state) const override;
    bool isStringNotFinished(int state) const override;
    SyntaxState getState() const override;
    void setState(const SyntaxState& rangeState) override;
    void resetState() override;

    bool supportFolding() override;
    bool needsLineState() override;
    QSet<QString> keywords() override;

    bool isATT() const;
    void setATT(bool newATT);
    QString commentSymbol() override;
    QString blockCommentBeginSymbol() override;
    QString blockCommentEndSymbol() override;
};

}

#endif // QSYNEDIT_ASM_SYNTAXER_H
