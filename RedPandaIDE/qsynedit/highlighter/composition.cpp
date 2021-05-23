#include "composition.h"
#include "../Constants.h"

SynHighlightComposition::SynHighlightComposition(QObject *parent):
    SynHighlighter(parent)
{

}

SynHighlighterClass SynHighlightComposition::getClass() const
{
    return SynHighlighterClass::Composition;
}

QString SynHighlightComposition::getName() const
{
    return "SynHighlightComposition";
}

SynScheme::SynScheme(QObject *parent):
    QObject(parent),
    mCaseSensitive(true)
{
    mMarkerAttribute = std::make_shared<SynHighlighterAttribute>(SYNS_AttrMarker);
    connect(mMarkerAttribute.get(),&SynHighlighterAttribute::changed,
            this, &SynScheme::MarkerAttriChanged);
    mMarkerAttribute->setBackground(QColorConstants::Yellow);
    mMarkerAttribute->setStyles(SynFontStyle::fsBold);
}

QString SynScheme::endExpr() const
{
    return mEndExpr;
}

void SynScheme::setEndExpr(const QString &endExpr)
{
    mEndExpr = endExpr;
}

QString SynScheme::getStartExpr() const
{
    return StartExpr;
}

void SynScheme::setStartExpr(const QString &value)
{
    StartExpr = value;
}

PSynHighlighter SynScheme::getHighlighter() const
{
    return mHighlighter;
}

void SynScheme::setHighlighter(const PSynHighlighter &highlighter)
{
    mHighlighter = highlighter;
}

PSynHighlighterAttribute SynScheme::getMarkerAttribute() const
{
    return mMarkerAttribute;
}

QString SynScheme::getSchemeName() const
{
    return mSchemeName;
}

void SynScheme::setSchemeName(const QString &schemeName)
{
    mSchemeName = schemeName;
}

int SynScheme::getCaseSensitive() const
{
    return mCaseSensitive;
}

void SynScheme::setCaseSensitive(int caseSensitive)
{
    mCaseSensitive = caseSensitive;
}

QString SynScheme::ConvertExpression(const QString &Value)
{
    if (!mCaseSensitive) {
        return Value.toUpper();
    } else {
        return Value;
    }
}

void SynScheme::MarkerAttriChanged() {
}
