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
#include "syntaxer.h"
#include "../constants.h"

namespace QSynedit {
Syntaxer::Syntaxer() :
    mWordBreakChars{ WordBreakChars }
{
    mCommentAttribute = std::make_shared<TokenAttribute>(SYNS_AttrComment,
                                                               TokenType::Comment);
    addAttribute(mCommentAttribute);
    mIdentifierAttribute = std::make_shared<TokenAttribute>(SYNS_AttrIdentifier,
                                                                  TokenType::Identifier);
    addAttribute(mIdentifierAttribute);
    mKeywordAttribute = std::make_shared<TokenAttribute>(SYNS_AttrReservedWord,
                                                               TokenType::Keyword);
    addAttribute(mKeywordAttribute);
    mStringAttribute = std::make_shared<TokenAttribute>(SYNS_AttrString,
                                                              TokenType::String);
    addAttribute(mStringAttribute);
    mWhitespaceAttribute = std::make_shared<TokenAttribute>(SYNS_AttrSpace,
                                                                   TokenType::Space);
    addAttribute(mWhitespaceAttribute);
    mSymbolAttribute = std::make_shared<TokenAttribute>(SYNS_AttrSymbol,
                                                                 TokenType::Operator);
    addAttribute(mSymbolAttribute);
}

bool Syntaxer::isKeyword(const QString &)
{
    return false;
}

void Syntaxer::nextToEol()
{
    while (!eol())
        next();
}

QSet<QString> Syntaxer::keywords()
{
    return QSet<QString>();
}

QMap<QString, QSet<QString> > Syntaxer::scopedKeywords()
{
    return QMap<QString, QSet<QString> >();
}

QString Syntaxer::foldString(QString /*startLine*/)
{
    return " ... }";
}

bool Syntaxer::supportBraceLevel()
{
    return false;
}

bool Syntaxer::isSpaceChar(const QChar &ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch.isSpace();
}

bool Syntaxer::isWordBreakChar(const QChar &ch)
{
    switch (ch.unicode()) {
    case '.':
    case ',':
    case ';':
    case ':':
    case '"':
    case '\'':
    case '!':
    case '?':
    case '[':
    case ']':
    case '(':
    case ')':
    case '{':
    case '}':
    case '<':
    case '>':
    case '^':
    case '|':
    case '&':
    case '-':
    case '=':
    case '+':
    case '*':
    case '/':
    case '\\':
        return true;
    default:
        return false;
    }
}

bool Syntaxer::isIdentChar(const QChar &ch) const
{
    return (ch == '_')
        || ((ch>='0') && (ch <= '9'))
        || ((ch>='a') && (ch <= 'z'))
           || ((ch>='A') && (ch <= 'Z'));
}

bool Syntaxer::isIdentStartChar(const QChar &ch) const
{
    return (ch == '_')
           || ((ch>='a') && (ch <= 'z'))
           || ((ch>='A') && (ch <= 'Z'));
}

PTokenAttribute Syntaxer::getAttribute(const QString& name) const
{
    return mAttributes.value(name,PTokenAttribute());
}

QString Syntaxer::commentSymbol()
{
    return QString();
}

QString Syntaxer::blockCommentBeginSymbol()
{
    return QString();
}

QString Syntaxer::blockCommentEndSymbol()
{
    return QString();
}

TokenAttribute::TokenAttribute(const QString &name, TokenType tokenType):
    mForeground(QColor()),
    mBackground(QColor()),
    mName(name),
    mStyles(FontStyle::fsNone),
    mTokenType(tokenType)
{

}

bool SyntaxState::operator==(const SyntaxState &s2)
{
    // indents contains the information of brace/parenthesis/brackets embedded levels
    return (state == s2.state)
            && (blockLevel == s2.blockLevel) // needed by block folding
            && (blockStarted == s2.blockStarted)  // needed by block folding
            && (blockEnded == s2.blockEnded)    // needed by block folding;
            && (blockEndedLastLine == s2.blockEndedLastLine) //needed by block folding;
            && (braceLevel == s2.braceLevel) // current braces embedding level (needed by rainbow color)
            && (bracketLevel == s2.bracketLevel) // current brackets embedding level (needed by rainbow color)
            && (parenthesisLevel == s2.parenthesisLevel) // current parenthesis embedding level (needed by rainbow color)
            && (indents == s2.indents)
            && (lastUnindent == s2.lastUnindent)
            && (extraData == s2.extraData)
            ;
}

IndentInfo SyntaxState::getLastIndent()
{
    if (indents.isEmpty())
        return IndentInfo{IndentType::None,0};
    return indents.back();
}

IndentType SyntaxState::getLastIndentType()
{
    if (indents.isEmpty())
        return IndentType::None;
    return indents.back().type;
}

SyntaxState::SyntaxState():
    state{0},
    blockLevel{0},
    blockStarted{0},
    blockEnded{0},
    blockEndedLastLine{0},
    braceLevel{0},
    bracketLevel{0},
    parenthesisLevel{0},
//    leftBraces(0),
//    rightBraces(0),
    lastUnindent{IndentType::None,0},
    hasTrailingSpaces{false}
{
}

bool IndentInfo::operator==(const IndentInfo &i2) const
{
    return type==i2.type && line==i2.line;
}

}
