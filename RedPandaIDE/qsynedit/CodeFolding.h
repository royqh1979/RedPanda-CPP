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
#ifndef CODEFOLDING_H
#define CODEFOLDING_H
#include <QColor>
#include <vector>
#include <memory>
#include <QVector>

struct SynEditFoldRegion;
typedef std::shared_ptr<SynEditFoldRegion> PSynEditFoldRegion;

class SynEditFoldRegions {
private:
    std::vector<PSynEditFoldRegion> fRegions;
public:
    int count();
    PSynEditFoldRegion add(bool addEnding, const QChar& openSymbol, const QChar& closeSymbol, const QString& highlight);
    PSynEditFoldRegion get(int index);
};
typedef std::shared_ptr<SynEditFoldRegions> PSynFoldRegions;


struct SynEditFoldRegion {
    bool addEnding;
    SynEditFoldRegions subFoldRegions;
    QChar openSymbol;
    QChar closeSymbol;
    QString highlight;
};

struct SynEditCodeFolding {
      bool indentGuides;
      bool fillIndents;
      bool showCollapsedLine;
      QColor collapsedLineColor;
      QColor folderBarLinesColor;
      QColor indentGuidesColor;
      SynEditFoldRegions foldRegions;
      SynEditCodeFolding();
};

class SynEditFoldRange;
typedef std::shared_ptr<SynEditFoldRange> PSynEditFoldRange;
class SynEditFoldRanges;
typedef std::shared_ptr<SynEditFoldRanges> PSynEditFoldRanges;

class SynEditFoldRanges{
private:
    QVector<PSynEditFoldRange> mRanges;
public:
    PSynEditFoldRange range(int index);
    void clear();
    int count() const;
    SynEditFoldRanges();
    PSynEditFoldRange addByParts(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold,
                               int aFromLine, PSynEditFoldRegion aFoldRegion, int aToLine);

    void insert(int index, PSynEditFoldRange range);
    void remove(int index);
    void add(PSynEditFoldRange foldRange);
    PSynEditFoldRange operator[](int index) const;
};

// A single fold
class SynEditFoldRange {
public:
    int fromLine; // Beginning line
    int toLine; // End line
    int linesCollapsed; // Number of collapsed lines
    PSynEditFoldRanges subFoldRanges; // Sub fold ranges
    bool collapsed; // Is collapsed?
    PSynEditFoldRanges allFoldRanges;// TAllFoldRanges pointer
    PSynEditFoldRegion foldRegion; // FoldRegion
    int hintMarkLeft;
    PSynEditFoldRange parent;
    bool parentCollapsed();
    void move(int count);
    explicit SynEditFoldRange(PSynEditFoldRange aParent, PSynEditFoldRanges aAllFold,
                      int aFromLine, PSynEditFoldRegion aFoldRegion, int aToLine);
};

#endif // CODEFOLDING_H
