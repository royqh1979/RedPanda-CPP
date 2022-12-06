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
#ifndef MAKEFILEHIGHLIGHTER_H
#define MAKEFILEHIGHLIGHTER_H


#include "base.h"
#include <QVector>

namespace QSynedit {

class MakefileHighlighter : public Highlighter
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
        Unknown, String,
        /* Targets, */
        Prequisitions,
        Command,
        CommandParameters,
        ParenthesisExpression,
        BraceExpression,
        Assignment,
    };

    enum ExpressionStartType {
        Parenthesis,
        Brace
    };


public:
    explicit MakefileHighlighter();

    static const QSet<QString> Directives;
private:
    QChar* mLine;
    QString mLineString;
    int mLineNumber;
    int mRun;
    int mStringLen;
    int mTokenPos;
    QVector<RangeState> mStates;
    RangeState mState;
    TokenId mTokenID;

    PHighlighterAttribute mTargetAttribute;
    PHighlighterAttribute mCommandAttribute;
    PHighlighterAttribute mCommandParamAttribute;
    PHighlighterAttribute mNumberAttribute;
    PHighlighterAttribute mVariableAttribute;
    PHighlighterAttribute mExpressionAttribute;

private:
    void procSpace();
    void procNumber();
    void procNull();
    void procString(bool inExpression );
    void procStringStart();
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
    bool isIdentStartChar(const QChar& ch) {
        if (ch == '_') {
            return true;
        }
        if ((ch>='a') && (ch <= 'z')) {
            return true;
        }
        if ((ch>='A') && (ch <= 'Z')) {
            return true;
        }
        return false;
    }

    bool isNumberChar(const QChar& ch) {
        return (ch>='0') && (ch<='9');
    }

    // SynHighlighter interface
public:
    bool eol() const override;

    QString languageName() override;
    HighlighterLanguage language() override;
    QString getToken() const override;
    const PHighlighterAttribute &getTokenAttribute() const override;
    int getTokenPos() override;
    void next() override;
    void setLine(const QString &newLine, int lineNumber) override;

    // SynHighlighter interface
public:
    bool getTokenFinished() const override;
    bool isLastLineCommentNotFinished(int state) const override;
    bool isLastLineStringNotFinished(int state) const override;
    HighlighterState getState() const override;
    void setState(const HighlighterState& rangeState) override;
    void resetState() override;

    bool isIdentChar(const QChar& ch) const override;
public:
    QSet<QString> keywords() const override;

};

}
#endif // MAKEFILEHIGHLIGHTER_H
