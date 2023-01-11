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
    mEnabled(true),
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

const QMap<QString, PTokenAttribute>& Syntaxer::attributes() const
{
    return mAttributes;
}

const QSet<QChar>& Syntaxer::wordBreakChars() const
{
    return mWordBreakChars;
}

const PTokenAttribute& Syntaxer::identifierAttribute() const
{
    return mIdentifierAttribute;
}

const PTokenAttribute &Syntaxer::keywordAttribute() const
{
    return mKeywordAttribute;
}

const PTokenAttribute &Syntaxer::commentAttribute() const
{
    return mCommentAttribute;
}

const PTokenAttribute& Syntaxer::stringAttribute() const
{
    return mStringAttribute;
}

const PTokenAttribute& Syntaxer::whitespaceAttribute() const
{
    return mWhitespaceAttribute;
}

const PTokenAttribute& Syntaxer::symbolAttribute() const
{
    return mSymbolAttribute;
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

QSet<QString> Syntaxer::keywords() const
{
    return QSet<QString>();
}

QString Syntaxer::foldString()
{
    return " ... }";
}

bool Syntaxer::supportBraceLevel()
{
    return false;
}

bool Syntaxer::isSpaceChar(const QChar &ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
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
    if (ch == '_') {
        return true;
    }
    if ((ch>='0') && (ch <= '9')) {
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

void Syntaxer::addAttribute(PTokenAttribute attribute)
{
    mAttributes[attribute->name()]=attribute;
}

void Syntaxer::clearAttributes()
{
    mAttributes.clear();
}

int Syntaxer::attributesCount() const
{
    return mAttributes.size();
}

PTokenAttribute Syntaxer::getAttribute(const QString& name) const
{
    return mAttributes.value(name,PTokenAttribute());
}

bool Syntaxer::enabled() const
{
    return mEnabled;
}

void Syntaxer::setEnabled(bool value)
{
    if (value != mEnabled) {
        mEnabled = value;
    }
}

FontStyles TokenAttribute::styles() const
{
    return mStyles;
}

void TokenAttribute::setStyles(const FontStyles &styles)
{
    if (mStyles!=styles) {
        mStyles = styles;
    }
}

const QColor& TokenAttribute::foreground() const
{
    return mForeground;
}

void TokenAttribute::setForeground(const QColor &color)
{
    mForeground = color;
}

const QColor &TokenAttribute::background() const
{
    return mBackground;
}

void TokenAttribute::setBackground(const QColor &background)
{
    mBackground = background;
}

TokenType TokenAttribute::tokenType() const
{
    return mTokenType;
}

QString TokenAttribute::name() const
{
    return mName;
}

TokenAttribute::TokenAttribute(const QString &name, TokenType tokenType):
    mForeground(QColor()),
    mBackground(QColor()),
    mName(name),
    mStyles(FontStyle::fsNone),
    mTokenType(tokenType)
{

}

bool SyntaxerState::operator==(const SyntaxerState &s2)
{
    // indents contains the information of brace/parenthesis/brackets embedded levels
    return (state == s2.state)
            && (indents == s2.indents)
            ;
}

int SyntaxerState::getLastIndent()
{
    if (indents.isEmpty())
        return -1;
    return indents.back();
}

SyntaxerState::SyntaxerState():
    state(0),
    blockLevel(0),
    blockStarted(0),
    blockEnded(0),
    blockEndedLastLine(0),
    braceLevel(0),
    bracketLevel(0),
    parenthesisLevel(0),
//    leftBraces(0),
//    rightBraces(0),
    firstIndentThisLine(0),
    hasTrailingSpaces(false)
{
}
}
