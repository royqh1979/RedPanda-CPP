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
#ifndef QSYNEDIT_LUA_SYNTAXER_H
#define QSYNEDIT_LUA_SYNTAXER_H
#include "syntaxer.h"
#include <QSet>
#include <QMap>

namespace QSynedit {

class LuaSyntaxer: public Syntaxer
{
    enum class TokenId {
        Comment,
        Identifier,
        Key,
        Null,
        Number,
        Space,
        String,
        StringEscapeSeq,
        Symbol,
        Unknown,
        Float,
        Hex,
        HexFloat
    };

    enum RangeState {
        rsUnknown,
        rsComment,
        rsLongComment,
        rsString,
        rsMultiLineDirective,
        rsStringEscapeSeq,
        rsSpace,
        rsRawStringNotEscaping,
        rsRawStringEnd,

        rsLongString /* long string must be placed at end to record levels */
    };

public:
    explicit LuaSyntaxer();
    LuaSyntaxer(const LuaSyntaxer&)=delete;
    LuaSyntaxer operator=(const LuaSyntaxer&)=delete;

    const PTokenAttribute &invalidAttribute() const;

    const PTokenAttribute &numberAttribute() const;

    const PTokenAttribute &floatAttribute() const;

    const PTokenAttribute &hexAttribute() const;

    const PTokenAttribute &stringEscapeSequenceAttribute() const;

    const PTokenAttribute &charAttribute() const;

    static const QSet<QString> Keywords;

    static const QSet<QString> StdLibFunctions;

    static const QMap<QString,QSet<QString>> StdLibTables;

    static const QSet<QString> XMakeLibFunctions;


    TokenId getTokenId();
private:
    void braceCloseProc();
    void braceOpenProc();
    void colonProc();
    void commentProc();
    void equalProc();
    void greaterProc();
    void identProc();
    void longCommentProc();
    void lowerProc();
    void minusProc();
    void nullProc();
    void pointProc();
    void processChar();
    void roundCloseProc();
    void roundOpenProc();
    void slashProc();
    void stringProc();
    void tildeProc();

    void numberProc();
    void spaceProc();
    void squareCloseProc();
    void squareOpenProc();
//    void stringEndProc();
    void stringEscapeSeqProc();
    void unknownProc();

    void popIndents(IndentType indentType);
    void pushIndents(IndentType indentType, int line=-1);

private:
    bool mAsmStart;
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
    bool mUseXMakeLibs;

    QSet<QString> mCustomTypeKeywords;
    QSet<QString> mKeywordsCache;

    PTokenAttribute mInvalidAttribute;
    PTokenAttribute mNumberAttribute;
    PTokenAttribute mFloatAttribute;
    PTokenAttribute mHexAttribute;
    PTokenAttribute mStringEscapeSequenceAttribute;
    PTokenAttribute mCharAttribute;


public:
    bool isCommentNotFinished(int state) const override;
    bool isStringNotFinished(int state) const override;
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
    bool isIdentStartChar(const QChar& ch) const override;
    QSet<QString> keywords() override;
    QString foldString(QString startLine) override;
    const QSet<QString> &customTypeKeywords() const;
    void setCustomTypeKeywords(const QSet<QString> &newCustomTypeKeywords);
    bool supportBraceLevel() override;
    QMap<QString, QSet<QString> > scopedKeywords() override;
    bool useXMakeLibs() const;
    void setUseXMakeLibs(bool newUseXMakeLibs);
    QString commentSymbol() override;
    QString blockCommentBeginSymbol() override;
    QString blockCommentEndSymbol() override;
    bool supportFolding() override;
    bool needsLineState() override;

};

}

#endif // QSYNEDIT_LUA_SYNTAXER_H
