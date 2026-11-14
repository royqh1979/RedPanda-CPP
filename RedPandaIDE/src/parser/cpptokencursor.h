/*
 * Copyright (C) 2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef CPPTOKENCURSOR_H
#define CPPTOKENCURSOR_H

#include "cpptokenizer.h"

class CppTokenCursor
{
public:
    struct Checkpoint {
        int index = 0;
        int greaterConsumed = 0;
    };

    enum class MatchStatus {
        SameFile,
        FileEnd,
        CrossFile,
        Unmatched
    };

    explicit CppTokenCursor(const CppTokenizer &tokenizer);

    bool atEnd() const { return mIndex >= mTokens->count(); }
    int index() const { return mIndex; }
    int greaterConsumed() const { return mGreaterConsumed; }
    const CppTokenizer::Token &current() const;
    const CppTokenizer::Token &peek(int offset = 1) const;
    const QString &currentText() const;
    const QStringList &sourceLines() const { return *mSourceLines; }

    void advance();
    bool consume(const QString &text);
    bool consumeGreater();
    Checkpoint checkpoint() const { return Checkpoint{mIndex, mGreaterConsumed}; }
    void restore(Checkpoint checkpoint);

    MatchStatus matchStatus() const;
    MatchStatus skipToMatch();
    void advanceToFileEnd();

private:
    const CppTokenizer::TokenList *mTokens;
    const QStringList *mSourceLines;
    int mIndex = 0;
    int mGreaterConsumed = 0;
};

#endif // CPPTOKENCURSOR_H
