#include "base.h"
#include "../Constants.h"

SynHighlighter::SynHighlighter(QObject *parent) : QObject(parent),
    mWordBreakChars{ SynWordBreakChars },
    mEnabled(true),
    mUpdateCount(0)
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

void SynHighlighter::onAttributeChanged()
{
    setAttributesChanged();
}

void SynHighlighter::setAttributesChanged()
{
    if (mUpdateCount == 0) {
        emit attributesChanged();
    }
}

void SynHighlighter::beginUpdate()
{
    mUpdateCount++;
}

void SynHighlighter::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount == 0) {
        setAttributesChanged();
    }
    if (mUpdateCount<0) {
        throw new std::out_of_range("mUpdateCount in SynHighlighterBase < 0");
    }
}

SynRangeState SynHighlighter::getRangeState() const
{
    return {0,0};
}

int SynHighlighter::getBraceLevel() const
{
    return 0;
}

int SynHighlighter::getBracketLevel() const
{
    return 0;
}

int SynHighlighter::getParenthesisLevel() const
{
    return 0;
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

bool SynHighlighter::isIdentChar(const QChar &ch) const
{
    if (ch == '_') {
        return true;
    }
    if (ch>='0' && ch <= '9') {
        return true;
    }
    if (ch>='a' && ch <= 'z') {
        return true;
    }
    if (ch>='A' && ch <= 'A') {
        return true;
    }

}

void SynHighlighter::addAttribute(PSynHighlighterAttribute attribute)
{
    mAttributes[attribute->name()]=attribute;
    connect(attribute.get(), &SynHighlighterAttribute::changed,
            this, &SynHighlighter::setAttributesChanged);
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
        setAttributesChanged();
    }
}

void SynHighlighterAttribute::setChanged()
{
    emit changed();
}

SynFontStyles SynHighlighterAttribute::styles() const
{
    return mStyles;
}

void SynHighlighterAttribute::setStyles(const SynFontStyles &styles)
{
    if (mStyles!=styles) {
        mStyles = styles;
        setChanged();
    }
}

QString SynHighlighterAttribute::name() const
{
    return mName;
}

void SynHighlighterAttribute::setName(const QString &name)
{
    if (mName!=name) {
        mName = name;
        setChanged();
    }
}

QColor SynHighlighterAttribute::foreground() const
{
    return mForeground;
}

void SynHighlighterAttribute::setForeground(const QColor &foreground)
{
    if (mForeground!=foreground) {
        mForeground = foreground;
        setChanged();
    }
}

SynHighlighterAttribute::SynHighlighterAttribute(const QString &name, QObject *parent):
    QObject(parent),
    mName(name),
    mForeground(QColorConstants::Black),
    mBackground(QColorConstants::White)
{

}

QColor SynHighlighterAttribute::background() const
{
    return mBackground;
}

void SynHighlighterAttribute::setBackground(const QColor &background)
{
    if (mBackground!=background) {
        mBackground = background;
        setChanged();
    }
}
