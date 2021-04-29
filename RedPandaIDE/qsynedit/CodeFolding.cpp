#include "CodeFolding.h"

int QSynFoldRegions::count()
{
    return fRegions.size();
}

int QSynFoldRegions::add(bool addEnding, const QChar &openSymbol, const QChar &closeSymbol, const QString &highlight)
{
    PSynFoldRegion region = std::make_shared<QSynFoldRegion>();
    region->addEnding = addEnding;
    region->openSymbol = openSymbol;
    region->closeSymbol = closeSymbol;
    region->highlight = highlight;
    fRegions.push_back(region);
}

PSynFoldRegion QSynFoldRegions::get(int index)
{
    return fRegions.at(index);
}

QSynCodeFolding::QSynCodeFolding():
    indentGuides(true),
    showCollapsedLine(true),
    collapsedLineColor(QColor("black")),
    folderBarLinesColor(QColor("black")),
    indentGuidesColor("gray")
{
    foldRegions.add(true,'{','}',"Symbol");
}


bool QSynEditFoldRange::parentCollapsed()
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

void QSynEditFoldRange::move(int count)
{
    fromLine += count;
    toLine += count;
}

QSynEditFoldRange::QSynEditFoldRange(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold, int aFromLine, PSynFoldRegion aFoldRegion, int aToLine):
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


PSynEditFoldRange QSynEditFoldRanges::foldRange(int index)
{
    return ranges[index];
}

int QSynEditFoldRanges::count()
{
    return ranges.size();
}

QSynEditFoldRanges::QSynEditFoldRanges()
{

}

PSynEditFoldRange QSynEditFoldRanges::addByParts(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold, int aFromLine, PSynFoldRegion aFoldRegion, int aToLine)
{
    PSynEditFoldRange range=std::make_shared<QSynEditFoldRange>(aParent,aAllFold, aFromLine,aFoldRegion,aToLine);
    return range;
}

int QSynEditFoldRanges::remove(int index)
{
    ranges.erase(ranges.begin()+index);
}

void QSynEditFoldRanges::addObject(PSynEditFoldRange foldRange)
{
    ranges.push_back(foldRange);
}
