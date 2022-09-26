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
#include "composition.h"
#include "../Constants.h"

namespace QSynedit {

SynHighlightComposition::SynHighlightComposition()
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
    mMarkerAttribute->setForeground(Qt::yellow);
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

}
