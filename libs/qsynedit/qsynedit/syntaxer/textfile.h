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
#ifndef QSYNEDIT_TEXTFILE_SYNTAXER_H
#define QSYNEDIT_TEXTFILE_SYNTAXER_H


#include "syntaxer.h"
#include <QVector>

namespace QSynedit {

class TextSyntaxer : public Syntaxer
{
    enum class TokenId {
        Null,
        Space,
        Text
    };

    enum RangeState {
        Unknown,
    };

public:
    explicit TextSyntaxer();
    TextSyntaxer(const TextSyntaxer&)=delete;
    TextSyntaxer& operator=(const TextSyntaxer&)=delete;

    static const QSet<QString> Directives;
private:
    const QChar* mLine;
    QString mLineString;
    int mLineNumber;
    int mRun;
    int mStringLen;
    int mTokenPos;
    RangeState mState;
    TokenId mTokenID;
    bool mHasTrailingSpaces;

    PTokenAttribute mTextAttribute;


private:
    void procSpace();
    void procText();
    void procNull();

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
    QSet<QString> keywords() override;
    QString commentSymbol() override;
    bool supportFolding() override;
    bool needsLineState() override;

};

}
#endif // QSYNEDIT_TEXTFILE_SYNTAXER_H
