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

}

const QMap<QString, PHighlighterAttribute>& Highlighter::attributes() const
{
    return mAttributes;
}

const QSet<QChar>& Highlighter::wordBreakChars() const
{
    return mWordBreakChars;
}

PHighlighterAttribute Highlighter::commentAttribute() const
{
    return mCommentAttribute;
}

PHighlighterAttribute Highlighter::identifierAttribute() const
{
    return mIdentifierAttribute;
}

PHighlighterAttribute Highlighter::keywordAttribute() const
{
    return mKeywordAttribute;
}

PHighlighterAttribute Highlighter::stringAttribute() const
{
    return mStringAttribute;
}

PHighlighterAttribute Highlighter::whitespaceAttribute() const
{
    return mWhitespaceAttribute;
}

PHighlighterAttribute Highlighter::symbolAttribute() const
{
    return mSymbolAttribute;
}

TokenType Highlighter::getTokenType()
{
    return TokenType::Default;
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

PHighlighterAttribute Highlighter::getAttribute(const QString &name) const
{
    auto search = mAttributes.find(name);
    if (search!=mAttributes.end()) {
        return search.value();
    }
    return PHighlighterAttribute();
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

QColor HighlighterAttribute::foreground() const
{
    return mForeground;
}

void HighlighterAttribute::setForeground(const QColor &color)
{
    mForeground = color;
}

QColor HighlighterAttribute::background() const
{
    return mBackground;
}

void HighlighterAttribute::setBackground(const QColor &background)
{
    mBackground = background;
}

QString HighlighterAttribute::name() const
{
    return mName;
}

void HighlighterAttribute::setName(const QString &name)
{
    if (mName!=name) {
        mName = name;
    }
}

HighlighterAttribute::HighlighterAttribute(const QString &name):
    mForeground(QColor()),
    mBackground(QColor()),
    mName(name),
    mStyles(FontStyle::fsNone)
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
