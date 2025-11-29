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

    enum class ProcessStage {
        Normal,
        LastBackSlash,
        SpacesAfterLastSlash,
    };

    enum class TokenId {
        Comment,
        Directive,
        Identifier,
        Key,
        Null,
        BinInteger,
        DecInteger,
        OctInteger,
        HexInteger,
        Space,
        String,
        StringEscapeSeq,
        Symbol,
        Unknown,
        Char,
        Float,
        HexFloat,
        RawString,
        LastBackSlash,
        SpaceAfterBackSlash
    };

    enum RangeState {
        rsUnknown, rsAnsiC, rsDirective, rsDirectiveComment,
        rsString,
        rsMultiLineDirective, rsCppComment,
        rsDocstring,
        rsStringEscapeSeq,
        rsRawString, rsSpace,rsRawStringNotEscaping,rsRawStringEnd,
        rsChar, rsCharEscaping,
        rsDefineIdentifier, rsDefineRemaining,
    };


    struct CppSyntaxState: SyntaxState {
        QString initialDCharSeq;
        bool inAttribute;
        QList<size_t> ancestorsForIf;
        bool mergeWithNextLine;
        QString lastToken;
        RangeState stateBeforeLastToken;

        bool equals(const std::shared_ptr<SyntaxState>& s2) const override;
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

    bool isRawStringStart(const PSyntaxState &state) const { return state->state == RangeState::rsRawString; }
    bool isRawStringNoEscape(const PSyntaxState &state) const { return state->state == RangeState::rsRawStringNotEscaping; }
    bool isRawStringEnd(const PSyntaxState &state) const { return state->state == RangeState::rsRawStringEnd; }
    bool isCharNotFinished(const PSyntaxState &state) const { return state->state == RangeState::rsChar || state->state == RangeState::rsCharEscaping; }
    bool isCharEscaping(const PSyntaxState &state) const { return state->state == RangeState::rsCharEscaping; }
    bool isStringEscaping(const PSyntaxState &state) const { return state->state == RangeState::rsStringEscapeSeq; }

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
    void pushIndents(IndentType indentType, size_t lineSeq, const QString& keyword = QString());
    void popStatementIndents();

private:
    CppSyntaxState mRange;
    QString mLine;
    int mLineSize;
    size_t mLineSeq;

    int mPrevLineLastTokenSize;
    QString mOrigLine;
    bool mMergeWithNextLine;
    QString mSpacesAfterLastBackSlash;
    ProcessStage mProcessStage;

    int mRun;
    int mStringLen;
    int mToIdent;
    int mTokenPos;
    TokenId mTokenId;
    int mLineNumber;
    int mLeftBraces;
    int mRightBraces;

    bool mHandleLastBackSlash;

    QString mLastKeyword;

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
    bool isCommentNotFinished(const PSyntaxState &state) const override;
    bool isStringNotFinished(const PSyntaxState &state) const override;
    bool isDocstringNotFinished(const PSyntaxState &state) const override;
    bool eol() const override;
    QString getToken() const override;
    const PTokenAttribute &getTokenAttribute() const override;
    int getTokenPos() override;
    void next() override;
    void setLine(int lineNumber, const QString &newLine, size_t lineSeq) override;
    bool isKeyword(const QString &word) override;
    void setState(const PSyntaxState& rangeState) override;
    void resetState() override;

    QString languageName() override;
    ProgrammingLanguage language() override;

    PSyntaxState getState() const override;
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
    bool handleLastBackSlash() const;
    void setHandleLastBackSlash(bool newHandleLastBackSlash);
};

}

#endif // QSYNEDIT_CPP_SYNTAXER_H
