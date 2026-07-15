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
        Text,
        Symbol,
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
    void procSymbol();

public:
    bool eol() const override;

    QString languageName() const override;
    ProgrammingLanguage language() const override;
    QString getToken() const override;
    const PTokenAttribute &getTokenAttribute() const override;
    int getTokenPos() override;
    void next() override;
    void setLine(int lineNumber, const QString &newLine, size_t lineSeq) override;
    bool isCommentNotFinished(const PSyntaxState &) const override;
    bool isStringNotFinished(const PSyntaxState &) const override;
    PSyntaxState getState() const override;
    void setState(const PSyntaxState& rangeState) override;
    void resetState() override;
    QString lineCommentSymbol() const override;
    bool supportFolding() const override;
    bool needsLineState() const override;
    virtual PSyntaxer createInstance() const override;
};

}
#endif // QSYNEDIT_TEXTFILE_SYNTAXER_H
