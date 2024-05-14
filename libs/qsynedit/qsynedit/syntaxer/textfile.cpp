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
#include "textfile.h"
#include "../constants.h"
#include <qt_utils/utils.h>
//#include <QDebug>

namespace QSynedit {

TextSyntaxer::TextSyntaxer()
{
    mTextAttribute = std::make_shared<TokenAttribute>(SYNS_AttrText, TokenType::Default);
    addAttribute(mTextAttribute);
}

void TextSyntaxer::procSpace()
{
    mTokenID = TokenId::Space;
    while (mLine[mRun]!=0 && isSpaceChar(mLine[mRun]))
        mRun++;
    if (mRun>=mStringLen)
        mHasTrailingSpaces = true;
}

void TextSyntaxer::procText()
{
    mTokenID = TokenId::Text;
    while (mLine[mRun]!=0 && !mLine[mRun].isSpace())
        mRun++;
}

void TextSyntaxer::procNull()
{
    mTokenID = TokenId::Null;
    mState = RangeState::Unknown;
}

bool TextSyntaxer::eol() const
{
    return mTokenID == TokenId::Null;
}

QString TextSyntaxer::languageName()
{
    return "textfile";
}

ProgrammingLanguage TextSyntaxer::language()
{
    return ProgrammingLanguage::Textfile;
}

QString TextSyntaxer::getToken() const
{
    return mLineString.mid(mTokenPos,mRun-mTokenPos);
}

const PTokenAttribute &TextSyntaxer::getTokenAttribute() const
{
    /*
        Directive,
        Unknown
    */
    switch(mTokenID) {
    case TokenId::Space:
        return mWhitespaceAttribute;
    default:
        return mTextAttribute;
    }
}

int TextSyntaxer::getTokenPos()
{
    return mTokenPos;
}

void TextSyntaxer::next()
{
    mTokenPos = mRun;
    if (mLine[mRun].unicode()==0) {
        procNull();
    } else if (isSpaceChar(mLine[mRun])) {
        procSpace();
    } else {
        procText();
    }
}

void TextSyntaxer::setLine(const QString &newLine, int lineNumber)
{
    mLineString = newLine;
    mLine = getNullTerminatedStringData(mLineString);
    mLineNumber = lineNumber;
    mRun = 0;
    next();
}

bool TextSyntaxer::isCommentNotFinished(int /*state*/) const
{
    return false;
}

bool TextSyntaxer::isStringNotFinished(int /*state*/) const
{
    return false;
}

SyntaxState TextSyntaxer::getState() const
{
    SyntaxState state;
    state.state = (int)mState;
    state.hasTrailingSpaces = mHasTrailingSpaces;
    return state;
}

void TextSyntaxer::setState(const SyntaxState & rangeState)
{
    mState = (RangeState)rangeState.state;
    mHasTrailingSpaces = false;
}

void TextSyntaxer::resetState()
{
    mState = RangeState::Unknown;
    mHasTrailingSpaces = false;
}

QSet<QString> TextSyntaxer::keywords()
{
    return QSet<QString>();
}

QString TextSyntaxer::commentSymbol()
{
    return "";
}

bool TextSyntaxer::supportFolding()
{
    return false;
}

bool TextSyntaxer::needsLineState()
{
    return false;
}

}
