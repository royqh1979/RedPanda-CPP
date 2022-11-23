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

HighlighterSchema::HighlighterSchema(QObject *parent):
    QObject(parent),
    mCaseSensitive(true)
{
    mMarkerAttribute = std::make_shared<HighlighterAttribute>(SYNS_AttrMarker,
                                                              QSynedit::TokenType::Default);
    mMarkerAttribute->setForeground(Qt::yellow);
    mMarkerAttribute->setStyles(FontStyle::fsBold);
}

QString HighlighterSchema::endExpr() const
{
    return mEndExpr;
}

void HighlighterSchema::setEndExpr(const QString &endExpr)
{
    mEndExpr = endExpr;
}

QString HighlighterSchema::getStartExpr() const
{
    return StartExpr;
}

void HighlighterSchema::setStartExpr(const QString &value)
{
    StartExpr = value;
}

PHighlighter HighlighterSchema::getHighlighter() const
{
    return mHighlighter;
}

void HighlighterSchema::setHighlighter(const PHighlighter &highlighter)
{
    mHighlighter = highlighter;
}

PHighlighterAttribute HighlighterSchema::getMarkerAttribute() const
{
    return mMarkerAttribute;
}

QString HighlighterSchema::getSchemeName() const
{
    return mSchemeName;
}

void HighlighterSchema::setSchemeName(const QString &schemeName)
{
    mSchemeName = schemeName;
}

int HighlighterSchema::getCaseSensitive() const
{
    return mCaseSensitive;
}

void HighlighterSchema::setCaseSensitive(int caseSensitive)
{
    mCaseSensitive = caseSensitive;
}

QString HighlighterSchema::ConvertExpression(const QString &Value)
{
    if (!mCaseSensitive) {
        return Value.toUpper();
    } else {
        return Value;
    }
}

void HighlighterSchema::MarkerAttriChanged() {
}

}
