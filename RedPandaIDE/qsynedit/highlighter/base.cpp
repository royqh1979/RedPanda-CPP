#include "base.h"
#include "../Constants.h"

SynHighligterBase::SynHighligterBase(QObject *parent) : QObject(parent),
    mWordBreakChars{ SynWordBreakChars},
    mEnabled(true),
    mUpdateCount(0)
{

}

std::map<QString, PSynHighlighterAttribute> SynHighligterBase::attributes() const
{
    return mAttributes;
}

std::set<QChar> SynHighligterBase::wordBreakChars() const
{
    return mWordBreakChars;
}

PSynHighlighterAttribute SynHighligterBase::commentAttribute() const
{
    return mCommentAttribute;
}

PSynHighlighterAttribute SynHighligterBase::identifierAttribute() const
{
    return mIdentifierAttribute;
}

PSynHighlighterAttribute SynHighligterBase::keywordAttribute() const
{
    return mKeywordAttribute;
}

PSynHighlighterAttribute SynHighligterBase::stringAttribute() const
{
    return mStringAttribute;
}

PSynHighlighterAttribute SynHighligterBase::whitespaceAttribute() const
{
    return mWhitespaceAttribute;
}

PSynHighlighterAttribute SynHighligterBase::symbolAttribute() const
{
    return mSymbolAttribute;
}

void SynHighligterBase::onAttributeChanged()
{
    setAttributesChanged();
}

void SynHighligterBase::setAttributesChanged()
{
    if (mUpdateCount == 0) {
        emit attributesChanged();
    }
}

void SynHighligterBase::beginUpdate()
{
    mUpdateCount++;
}

void SynHighligterBase::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount == 0) {
        setAttributesChanged();
    }
    if (mUpdateCount<0) {
        throw new std::out_of_range("mUpdateCount in SynHighlighterBase < 0");
    }
}

SynRangeState SynHighligterBase::getRangeState() const
{
    return 0;
}

SynRangeState SynHighligterBase::getSpaceRangeState() const
{

}

int SynHighligterBase::getBraceLevel() const
{
    return 0;
}

int SynHighligterBase::getBracketLevel() const
{
    return 0;
}

int SynHighligterBase::getParenthesisLevel() const
{
    return 0;
}

SynHighlighterTokenType SynHighligterBase::getTokenType()
{
    return SynHighlighterTokenType::httDefault;
}

bool SynHighligterBase::isKeyword(const QString &)
{
    return false;
}

bool SynHighligterBase::isSpaceChar(const QChar &ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

bool SynHighligterBase::isIdentChar(const QChar &ch) const
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

void SynHighligterBase::addAttribute(PSynHighlighterAttribute attribute)
{
    mAttributes[attribute->name()]=attribute;
}

void SynHighligterBase::clearAttributes()
{
    mAttributes.clear();
}

int SynHighligterBase::attributesCount() const
{
    return mAttributes.size();
}

PSynHighlighterAttribute SynHighligterBase::getAttribute(const QString &name) const
{
    auto search = mAttributes.find(name);
    if (search!=mAttributes.end()) {
        return search->second;
    }
    return PSynHighlighterAttribute();
}

void SynHighlighterAttribute::setChanged()
{
    emit changed();
}

bool SynHighlighterAttribute::strikeOut() const
{
    return mStrikeOut;
}

void SynHighlighterAttribute::setStrikeOut(bool strikeOut)
{
    if (mStrikeOut!=strikeOut) {
        mStrikeOut = strikeOut;
        setChanged();
    }
}

bool SynHighlighterAttribute::underline() const
{
    return mUnderline;
}

void SynHighlighterAttribute::setUnderline(bool underline)
{
    mUnderline = underline;
}

bool SynHighlighterAttribute::italic() const
{
    return mItalic;
}

void SynHighlighterAttribute::setItalic(bool italic)
{
    if (mItalic!=italic) {
        mItalic = italic;
        setChanged();
    }
}

bool SynHighlighterAttribute::bold() const
{
    return mBold;
}

void SynHighlighterAttribute::setBold(bool bold)
{
    if (mBold!=bold) {
        mBold = bold;
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
