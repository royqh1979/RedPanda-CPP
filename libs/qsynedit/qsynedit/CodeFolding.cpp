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
#include "CodeFolding.h"
#include "Constants.h"


namespace QSynedit {

int CodeFoldingDefines::count()
{
    return fRegions.size();
}

PCodeFoldingDefine CodeFoldingDefines::add(bool addEnding, const QChar &openSymbol, const QChar &closeSymbol, const QString &highlight)
{
    PCodeFoldingDefine region = std::make_shared<CodeFoldingDefine>();
    region->addEnding = addEnding;
    region->openSymbol = openSymbol;
    region->closeSymbol = closeSymbol;
    region->highlight = highlight;
    fRegions.push_back(region);
    return region;
}

PCodeFoldingDefine CodeFoldingDefines::get(int index)
{
    return fRegions.at(index);
}

CodeFoldingOptions::CodeFoldingOptions():
    indentGuides(true),
    fillIndents(false),
    showCollapsedLine(true),
    collapsedLineColor(QColor("black")),
    folderBarLinesColor(QColor("black")),
    indentGuidesColor("gray")
{
    foldRegions.add(true,'{','}',SYNS_AttrSymbol);
}


bool CodeFoldingRange::parentCollapsed()
{
    PCodeFoldingRange parentFold = parent.lock();
    // Find first parent that is collapsed
    while (parentFold) {
        if (parentFold->collapsed) {
            return true;
        }
        parentFold = parentFold->parent.lock();
    }
    return false;
}

void CodeFoldingRange::move(int count)
{
    fromLine += count;
    toLine += count;
}

CodeFoldingRange::CodeFoldingRange(PCodeFoldingRange parent,
                                   int fromLine,
                                   int toLine):
    fromLine(fromLine),
    toLine(toLine),
    linesCollapsed(0),
    collapsed(false),
    hintMarkLeft(0),
    parent(parent)
{
    subFoldRanges = std::make_shared<CodeFoldingRanges>();
}


PCodeFoldingRange CodeFoldingRanges::range(int index)
{
    return mRanges[index];
}

void CodeFoldingRanges::clear()
{
    mRanges.clear();
}

int CodeFoldingRanges::count() const
{
    return mRanges.size();
}

CodeFoldingRanges::CodeFoldingRanges()
{

}

PCodeFoldingRange CodeFoldingRanges::addByParts(PCodeFoldingRange parent,
                                                PCodeFoldingRanges allFold,
                                                int fromLine,
                                                int toLine)
{
    PCodeFoldingRange range=std::make_shared<CodeFoldingRange>(parent, fromLine,toLine);
    mRanges.append(range);
    if (allFold && allFold.get()!=this) {
        allFold->add(range);
    }
    return range;
}

void CodeFoldingRanges::insert(int index, PCodeFoldingRange range)
{
    mRanges.insert(index,range);
}

void CodeFoldingRanges::remove(int index)
{
    mRanges.remove(index);
}

void CodeFoldingRanges::add(PCodeFoldingRange foldRange)
{
    mRanges.push_back(foldRange);
}

PCodeFoldingRange CodeFoldingRanges::operator[](int index) const
{
    return mRanges[index];
}

}
