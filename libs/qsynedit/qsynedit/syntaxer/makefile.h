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
#ifndef QSYNEDIT_MAKEFILE_SYNTAXER_H
#define QSYNEDIT_MAKEFILE_SYNTAXER_H


#include "syntaxer.h"
#include <QVector>

namespace QSynedit {

class MakefileSyntaxer : public Syntaxer
{
    enum class TokenId {
        Null,
        Comment,
        Target,
        Command,
        CommandParam,
        Number,
        Space,
        String,
        Identifier,
        Variable,
        Expression,
        Symbol,
        Directive
    };

    enum RangeState {
        Unknown,
        DQString, // Double Quoted "blahblah"
        SQString, // Single Quoted 'blahblah'
        /* Targets, */
        Prequisitions,
        Command,
        CommandParameters,
        ParenthesisExpression,
        BraceExpression,
        Assignment,
    };

    enum class ExpressionStartType {
        Parenthesis,
        Brace
    };

    enum class StringStartType {
        SingleQuoted,
        DoubleQuoted
    };


public:
    explicit MakefileSyntaxer();
    MakefileSyntaxer(const MakefileSyntaxer&)=delete;
    MakefileSyntaxer& operator=(const MakefileSyntaxer&)=delete;

    static const QSet<QString> Directives;
private:
    const QChar* mLine;
    QString mLineString;
    int mLineNumber;
    int mRun;
    int mStringLen;
    int mTokenPos;
    QVector<RangeState> mStates;
    RangeState mState;
    TokenId mTokenID;
    bool mHasTrailingSpaces;

    PTokenAttribute mTargetAttribute;
    PTokenAttribute mCommandAttribute;
    PTokenAttribute mCommandParamAttribute;
    PTokenAttribute mNumberAttribute;
    PTokenAttribute mVariableAttribute;
    PTokenAttribute mExpressionAttribute;

private:
    void procSpace();
    void procNumber();
    void procNull();
    void procString(bool inExpression );
    void procStringStart(StringStartType type, bool inExpression);
    void procExpressionStart(ExpressionStartType type);
    void procExpressionEnd();
    void procSymbol();
    void procVariableExpression();
    void procAutoVariable();
    void procAssignment();
    void procDollar();
    void procComment();
    void procIdentifier();

    void pushState();
    void popState();
    bool isIdentStartChar(const QChar& ch)  const override{
        if ((ch>='a') && (ch <= 'z')) {
            return true;
        }
        if ((ch>='A') && (ch <= 'Z')) {
            return true;
        }
        switch(ch.unicode()) {
        case '_':
        case '%':
        case '.':
        case '*':
        case '/':
            return true;
        }

        return false;
    }

    bool isNumberChar(const QChar& ch) {
        return (ch>='0') && (ch<='9');
    }

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
    bool isIdentChar(const QChar& ch) const override;
    QSet<QString> keywords() override;
    QString commentSymbol() override;
    bool supportFolding() override;
    bool needsLineState() override;

};

}
#endif // QSYNEDIT_MAKEFILE_SYNTAXER_H
