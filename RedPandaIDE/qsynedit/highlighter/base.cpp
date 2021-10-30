#include "base.h"
#include "../Constants.h"

SynHighlighter::SynHighlighter() :
    mEnabled(true),
    mWordBreakChars{ SynWordBreakChars }
{

}

const QMap<QString, PSynHighlighterAttribute>& SynHighlighter::attributes() const
{
    return mAttributes;
}

const QSet<QChar>& SynHighlighter::wordBreakChars() const
{
    return mWordBreakChars;
}

PSynHighlighterAttribute SynHighlighter::commentAttribute() const
{
    return mCommentAttribute;
}

PSynHighlighterAttribute SynHighlighter::identifierAttribute() const
{
    return mIdentifierAttribute;
}

PSynHighlighterAttribute SynHighlighter::keywordAttribute() const
{
    return mKeywordAttribute;
}

PSynHighlighterAttribute SynHighlighter::stringAttribute() const
{
    return mStringAttribute;
}

PSynHighlighterAttribute SynHighlighter::whitespaceAttribute() const
{
    return mWhitespaceAttribute;
}

PSynHighlighterAttribute SynHighlighter::symbolAttribute() const
{
    return mSymbolAttribute;
}

SynHighlighterTokenType SynHighlighter::getTokenType()
{
    return SynHighlighterTokenType::Default;
}

bool SynHighlighter::isKeyword(const QString &)
{
    return false;
}

void SynHighlighter::nextToEol()
{
    while (!eol())
        next();
}

bool SynHighlighter::isSpaceChar(const QChar &ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

bool SynHighlighter::isWordBreakChar(const QChar &ch)
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

bool SynHighlighter::isIdentChar(const QChar &ch) const
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

void SynHighlighter::addAttribute(PSynHighlighterAttribute attribute)
{
    mAttributes[attribute->name()]=attribute;
}

void SynHighlighter::clearAttributes()
{
    mAttributes.clear();
}

int SynHighlighter::attributesCount() const
{
    return mAttributes.size();
}

PSynHighlighterAttribute SynHighlighter::getAttribute(const QString &name) const
{
    auto search = mAttributes.find(name);
    if (search!=mAttributes.end()) {
        return search.value();
    }
    return PSynHighlighterAttribute();
}

bool SynHighlighter::enabled() const
{
    return mEnabled;
}

void SynHighlighter::setEnabled(bool value)
{
    if (value != mEnabled) {
        mEnabled = value;
    }
}

SynFontStyles SynHighlighterAttribute::styles() const
{
    return mStyles;
}

void SynHighlighterAttribute::setStyles(const SynFontStyles &styles)
{
    if (mStyles!=styles) {
        mStyles = styles;
    }
}

QColor SynHighlighterAttribute::foreground() const
{
    return mForeground;
}

void SynHighlighterAttribute::setForeground(const QColor &color)
{
    mForeground = color;
}

QColor SynHighlighterAttribute::background() const
{
    return mBackground;
}

void SynHighlighterAttribute::setBackground(const QColor &background)
{
    mBackground = background;
}

QString SynHighlighterAttribute::name() const
{
    return mName;
}

void SynHighlighterAttribute::setName(const QString &name)
{
    if (mName!=name) {
        mName = name;
    }
}

SynHighlighterAttribute::SynHighlighterAttribute(const QString &name):
    mForeground(QColor()),
    mBackground(QColor()),
    mName(name),
    mStyles(SynFontStyle::fsNone)
{

}

bool SynRangeState::operator==(const SynRangeState &s2)
{
    // indents contains the information of brace/parenthesis/brackets embedded levels
    return (state == s2.state)
            && (spaceState == s2.spaceState)
            && (indents == s2.indents)
            ;
}

int SynRangeState::getLastIndent()
{
    if (indents.isEmpty())
        return -1;
    return indents.back();
}
