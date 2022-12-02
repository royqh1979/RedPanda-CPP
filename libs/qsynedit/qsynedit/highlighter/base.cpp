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
#include "base.h"
#include "../Constants.h"

namespace QSynedit {
Highlighter::Highlighter() :
    mEnabled(true),
    mWordBreakChars{ WordBreakChars }
{
    mCommentAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrComment,
                                                               TokenType::Comment);
    addAttribute(mCommentAttribute);
    mIdentifierAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrIdentifier,
                                                                  TokenType::Identifier);
    addAttribute(mIdentifierAttribute);
    mKeywordAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrReservedWord,
                                                               TokenType::Keyword);
    addAttribute(mKeywordAttribute);
    mStringAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrString,
                                                              TokenType::String);
    addAttribute(mStringAttribute);
    mWhitespaceAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrSpace,
                                                                   TokenType::Space);
    addAttribute(mWhitespaceAttribute);
    mSymbolAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrSymbol,
                                                                 TokenType::Operator);
    addAttribute(mSymbolAttribute);
}

const QMap<QString, PHighlighterAttribute>& Highlighter::attributes() const
{
    return mAttributes;
}

const QSet<QChar>& Highlighter::wordBreakChars() const
{
    return mWordBreakChars;
}

const PHighlighterAttribute& Highlighter::identifierAttribute() const
{
    return mIdentifierAttribute;
}

const PHighlighterAttribute &Highlighter::keywordAttribute() const
{
    return mKeywordAttribute;
}

const PHighlighterAttribute &Highlighter::commentAttribute() const
{
    return mCommentAttribute;
}

const PHighlighterAttribute& Highlighter::stringAttribute() const
{
    return mStringAttribute;
}

const PHighlighterAttribute& Highlighter::whitespaceAttribute() const
{
    return mWhitespaceAttribute;
}

const PHighlighterAttribute& Highlighter::symbolAttribute() const
{
    return mSymbolAttribute;
}

bool Highlighter::isKeyword(const QString &)
{
    return false;
}

void Highlighter::nextToEol()
{
    while (!eol())
        next();
}

QSet<QString> Highlighter::keywords() const
{
    return QSet<QString>();
}

QString Highlighter::foldString()
{
    return " ... }";
}

bool Highlighter::supportBraceLevel()
{
    return false;
}

bool Highlighter::isSpaceChar(const QChar &ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

bool Highlighter::isWordBreakChar(const QChar &ch)
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

bool Highlighter::isIdentChar(const QChar &ch) const
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

void Highlighter::addAttribute(PHighlighterAttribute attribute)
{
    mAttributes[attribute->name()]=attribute;
}

void Highlighter::clearAttributes()
{
    mAttributes.clear();
}

int Highlighter::attributesCount() const
{
    return mAttributes.size();
}

PHighlighterAttribute Highlighter::getAttribute(const QString& name) const
{
    return mAttributes.value(name,PHighlighterAttribute());
}

bool Highlighter::enabled() const
{
    return mEnabled;
}

void Highlighter::setEnabled(bool value)
{
    if (value != mEnabled) {
        mEnabled = value;
    }
}

FontStyles HighlighterAttribute::styles() const
{
    return mStyles;
}

void HighlighterAttribute::setStyles(const FontStyles &styles)
{
    if (mStyles!=styles) {
        mStyles = styles;
    }
}

const QColor& HighlighterAttribute::foreground() const
{
    return mForeground;
}

void HighlighterAttribute::setForeground(const QColor &color)
{
    mForeground = color;
}

const QColor &HighlighterAttribute::background() const
{
    return mBackground;
}

void HighlighterAttribute::setBackground(const QColor &background)
{
    mBackground = background;
}

TokenType HighlighterAttribute::tokenType() const
{
    return mTokenType;
}

QString HighlighterAttribute::name() const
{
    return mName;
}

HighlighterAttribute::HighlighterAttribute(const QString &name, TokenType tokenType):
    mForeground(QColor()),
    mBackground(QColor()),
    mName(name),
    mStyles(FontStyle::fsNone),
    mTokenType(tokenType)
{

}

bool HighlighterState::operator==(const HighlighterState &s2)
{
    // indents contains the information of brace/parenthesis/brackets embedded levels
    return (state == s2.state)
            && (indents == s2.indents)
            ;
}

int HighlighterState::getLastIndent()
{
    if (indents.isEmpty())
        return -1;
    return indents.back();
}

HighlighterState::HighlighterState():
    state(0),
    braceLevel(0),
    bracketLevel(0),
    parenthesisLevel(0),
    leftBraces(0),
    rightBraces(0),
    firstIndentThisLine(0)
{
}
}
