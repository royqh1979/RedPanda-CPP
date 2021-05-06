#include "CodeFolding.h"

int SynEditFoldRegions::count()
{
    return fRegions.size();
}

int SynEditFoldRegions::add(bool addEnding, const QChar &openSymbol, const QChar &closeSymbol, const QString &highlight)
{
    PSynEditFoldRegion region = std::make_shared<SynEditFoldRegion>();
    region->addEnding = addEnding;
    region->openSymbol = openSymbol;
    region->closeSymbol = closeSymbol;
    region->highlight = highlight;
    fRegions.push_back(region);
}

PSynEditFoldRegion SynEditFoldRegions::get(int index)
{
    return fRegions.at(index);
}

SynEditCodeFolding::SynEditCodeFolding():
    indentGuides(true),
    showCollapsedLine(true),
    collapsedLineColor(QColor("black")),
    folderBarLinesColor(QColor("black")),
    indentGuidesColor("gray")
{
    foldRegions.add(true,'{','}',"Symbol");
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

}


PSynEditFoldRange SynEditFoldRanges::foldRange(int index)
{
    return ranges[index];
}

int SynEditFoldRanges::count()
{
    return ranges.size();
}

SynEditFoldRanges::SynEditFoldRanges()
{

}

PSynEditFoldRange SynEditFoldRanges::addByParts(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold, int aFromLine, PSynEditFoldRegion aFoldRegion, int aToLine)
{
    PSynEditFoldRange range=std::make_shared<SynEditFoldRange>(aParent,aAllFold, aFromLine,aFoldRegion,aToLine);
    return range;
}

int SynEditFoldRanges::remove(int index)
{
    ranges.erase(ranges.begin()+index);
}

void SynEditFoldRanges::addObject(PSynEditFoldRange foldRange)
{
    ranges.push_back(foldRange);
}
