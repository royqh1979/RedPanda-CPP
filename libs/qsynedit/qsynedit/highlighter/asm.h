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
#ifndef SYNEDITASMHIGHLIGHTER_H
#define SYNEDITASMHIGHLIGHTER_H

#include "base.h"

namespace QSynedit {

class ASMHighlighter : public Highlighter
{
    enum TokenId {
        Comment,
        Identifier,
        Key,
        Null,
        Number,
        Space,
        String,
        Symbol,
        Unknown
    };
public:
    explicit ASMHighlighter();
    PHighlighterAttribute numberAttribute();

    static const QSet<QString> Keywords;
private:
    QChar* mLine;
    QString mLineString;
    int mLineNumber;
    int mRun;
    int mStringLen;
    QChar mToIdent;
    int mTokenPos;
    TokenKind mTokenID;
    PHighlighterAttribute mNumberAttribute;

private:
    void CommentProc();
    void CRProc();
    void GreaterProc();
    void IdentProc();
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


    // SynHighlighter interface
public:
    bool eol() const override;

    QString languageName() override;
    HighlighterLanguage language() override;
    QString getToken() const override;
    PHighlighterAttribute getTokenAttribute() const override;
    TokenKind getTokenKind() override;
    TokenType getTokenType() override;
    int getTokenPos() override;
    void next() override;
    void setLine(const QString &newLine, int lineNumber) override;

    // SynHighlighter interface
public:
    HighlighterClass getClass() const override;
    QString getName() const override;

    // SynHighlighter interface
public:
    bool getTokenFinished() const override;
    bool isLastLineCommentNotFinished(int state) const override;
    bool isLastLineStringNotFinished(int state) const override;
    HighlighterState getState() const override;
    void setState(const HighlighterState& rangeState) override;
    void resetState() override;

    // SynHighlighter interface
public:
    QSet<QString> keywords() const override;
};

}

#endif // SYNEDITASMHIGHLIGHTER_H
