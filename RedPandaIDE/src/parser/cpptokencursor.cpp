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
#include "cpptokencursor.h"

CppTokenCursor::CppTokenCursor(const CppTokenizer &tokenizer)
    : mTokens(&tokenizer.tokens()), mSourceLines(&tokenizer.sourceLines())
{
}

const CppTokenizer::Token &CppTokenCursor::current() const
{
    if (atEnd())
        return CppTokenizer::fileEndToken();
    return (*mTokens)[mIndex];
}

const CppTokenizer::Token &CppTokenCursor::peek(int offset) const
{
    int index = mIndex + offset;
    if (index < 0 || index >= mTokens->count())
        return CppTokenizer::fileEndToken();
    return (*mTokens)[index];
}

const QString &CppTokenCursor::currentText() const
{
    static const QString empty;
    static const QString greater = QStringLiteral(">");
    if (atEnd())
        return empty;
    if (mGreaterConsumed > 0)
        return greater;
    return current().text;
}

void CppTokenCursor::advance()
{
    if (atEnd())
        return;
    ++mIndex;
    mGreaterConsumed = 0;
}

bool CppTokenCursor::consume(const char *text)
{
    QLatin1String latinText(text);
    if (latinText == QLatin1String(">"))
        return consumeGreater();
    if (atEnd() || currentText() != latinText)
        return false;
    advance();
    return true;
}

bool CppTokenCursor::consume(const QString &text)
{
    if (text == ">")
        return consumeGreater();
    if (atEnd() || currentText() != text)
        return false;
    advance();
    return true;
}

bool CppTokenCursor::consumeGreater()
{
    if (atEnd())
        return false;
    if (current().text == ">>" && mGreaterConsumed == 0) {
        mGreaterConsumed = 1;
        return true;
    }
    if (currentText() == ">") {
        advance();
        return true;
    }
    return false;
}

void CppTokenCursor::restore(Checkpoint checkpoint)
{
    mIndex = checkpoint.index;
    mGreaterConsumed = checkpoint.greaterConsumed;
}

CppTokenCursor::MatchStatus CppTokenCursor::matchStatus() const
{
    if (atEnd())
        return MatchStatus::Unmatched;
    int target = current().matchIndex;
    if (target < 0 || target > mTokens->count())
        return MatchStatus::Unmatched;
    if (target == mTokens->count()
            || (*mTokens)[target].kind == CppTokenizer::TokenKind::FileBoundary)
        return MatchStatus::FileEnd;
    if ((*mTokens)[target].fileName == current().fileName)
        return MatchStatus::SameFile;
    return MatchStatus::CrossFile;
}

CppTokenCursor::MatchStatus CppTokenCursor::skipToMatch()
{
    MatchStatus status = matchStatus();
    switch (status) {
    case MatchStatus::SameFile:
    case MatchStatus::FileEnd:
        mIndex = current().matchIndex;
        mGreaterConsumed = 0;
        break;
    case MatchStatus::CrossFile:
        advanceToFileEnd();
        break;
    case MatchStatus::Unmatched:
        break;
    }
    return status;
}

void CppTokenCursor::advanceToFileEnd()
{
    int target = mTokens->count();
    for (int i = mIndex + 1; i < mTokens->count(); ++i) {
        if ((*mTokens)[i].kind == CppTokenizer::TokenKind::FileBoundary) {
            target = i;
            break;
        }
    }
    mIndex = target;
    mGreaterConsumed = 0;
}
