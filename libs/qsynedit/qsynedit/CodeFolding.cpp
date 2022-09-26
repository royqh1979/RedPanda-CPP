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

int SynEditFoldRegions::count()
{
    return fRegions.size();
}

PSynEditFoldRegion SynEditFoldRegions::add(bool addEnding, const QChar &openSymbol, const QChar &closeSymbol, const QString &highlight)
{
    PSynEditFoldRegion region = std::make_shared<SynEditFoldRegion>();
    region->addEnding = addEnding;
    region->openSymbol = openSymbol;
    region->closeSymbol = closeSymbol;
    region->highlight = highlight;
    fRegions.push_back(region);
    return region;
}

PSynEditFoldRegion SynEditFoldRegions::get(int index)
{
    return fRegions.at(index);
}

SynEditCodeFolding::SynEditCodeFolding():
    indentGuides(true),
    fillIndents(false),
    showCollapsedLine(true),
    collapsedLineColor(QColor("black")),
    folderBarLinesColor(QColor("black")),
    indentGuidesColor("gray")
{
    foldRegions.add(true,'{','}',SYNS_AttrSymbol);
}


bool SynEditFoldRange::parentCollapsed()
{
    PSynEditFoldRange parentFold = parent;
    // Find first parent that is collapsed
    while (parentFold) {
        if (parentFold->collapsed) {
            return true;
        }
        parentFold = parentFold->parent;
    }
    return false;
}

void SynEditFoldRange::move(int count)
{
    fromLine += count;
    toLine += count;
}

SynEditFoldRange::SynEditFoldRange(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold, int aFromLine, PSynEditFoldRegion aFoldRegion, int aToLine):
    fromLine(aFromLine),
    toLine(aToLine),
    linesCollapsed(0),
    collapsed(false),
    allFoldRanges(aAllFold),
    foldRegion(aFoldRegion),
    hintMarkLeft(0),
    parent(aParent)
{
    subFoldRanges = std::make_shared<SynEditFoldRanges>();
}


PSynEditFoldRange SynEditFoldRanges::range(int index)
{
    return mRanges[index];
}

void SynEditFoldRanges::clear()
{
    mRanges.clear();
}

int SynEditFoldRanges::count() const
{
    return mRanges.size();
}

SynEditFoldRanges::SynEditFoldRanges()
{

}

PSynEditFoldRange SynEditFoldRanges::addByParts(PSynEditFoldRange aParent,
                                                PSynEditFoldRanges aAllFold,
                                                int aFromLine,
                                                PSynEditFoldRegion aFoldRegion,
                                                int aToLine)
{
    PSynEditFoldRange range=std::make_shared<SynEditFoldRange>(aParent,aAllFold, aFromLine,aFoldRegion,aToLine);
    mRanges.append(range);
    if (aAllFold && aAllFold.get()!=this) {
        aAllFold->add(range);
    }
    return range;
}

void SynEditFoldRanges::insert(int index, PSynEditFoldRange range)
{
    mRanges.insert(index,range);
}

void SynEditFoldRanges::remove(int index)
{
    mRanges.remove(index);
}

void SynEditFoldRanges::add(PSynEditFoldRange foldRange)
{
    mRanges.push_back(foldRange);
}

PSynEditFoldRange SynEditFoldRanges::operator[](int index) const
{
    return mRanges[index];
}

}
