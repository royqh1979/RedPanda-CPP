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
protected:
    enum class TokenId {
        Comment,
        Identifier,
        Instruction,        // add mov etc
        Directive, // .section .data etc
        Label,
        Register, //EAX EBX etc
        Null,
        BinInteger,
        DecInteger,
        OctInteger,
        HexInteger,
        Space,
        String,
        Symbol,
        PreprocessorDirective,
        Unknown
    };

public:
    explicit ASMSyntaxer();
    ASMSyntaxer(const ASMSyntaxer&)=delete;
    ASMSyntaxer& operator=(const ASMSyntaxer&)=delete;

    const PTokenAttribute &numberAttribute() const;
    const PTokenAttribute &directiveAttribute() const;
    const PTokenAttribute &labelAttribute() const;
    const PTokenAttribute &registerAttribute() const;

    static QMap<QString,QString> Instructions;
    static QSet<QString> InstructionNames;
    static const QSet<QString> Registers;
    static QSet<QString> PrefixedRegisters;

protected:
    const QChar* mLine;
    QString mLineString;
    int mLineNumber;
    int mRun;
    int mStringLen;
    QChar mToIdent;
    int mTokenPos;
    TokenId mTokenID;
    bool mHasTrailingSpaces;
    PTokenAttribute mNumberAttribute;
    PTokenAttribute mHexAttribute;
    PTokenAttribute mOctAttribute;
    PTokenAttribute mDirectiveAttribute;
    PTokenAttribute mRegisterAttribute;
    PTokenAttribute mLabelAttribute;
    PTokenAttribute mPreprocessDirectiveAttribute;

private:
    void procComment();
    void procCR();
    void procGreaterThan();
    void procIdent(const QString& prefix);
    void procLF();
    void proceLowerThan();
    void procNumber();
    void procNumberType();
    void procSingleQuoteString();
    void procSlash();
    void procSpace();
    void procString();
    void procSymbol();
    void procUnknown();
    static void initData();
    TokenId getIdentType(const QString& ident,  QChar nextChar);
protected:
    virtual bool isCommentStartChar(QChar ch);
    virtual void procNull();
    virtual bool isDirective(const QString& ident);
    virtual bool isPreprocessDirective(const QString& ident);
    virtual void handleDirective(int line, const QString& directive);
    virtual void handleIdent(int line, const QString& ident);
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

    QString commentSymbol() override;
    QString blockCommentBeginSymbol() override;
    QString blockCommentEndSymbol() override;
    const PTokenAttribute &preprocessDirectiveAttribute() const;
    const PTokenAttribute &hexAttribute() const;
    const PTokenAttribute &octAttribute() const;
};

}

#endif // QSYNEDIT_ASM_SYNTAXER_H
