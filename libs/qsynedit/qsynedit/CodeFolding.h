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

namespace QSynedit {
struct CodeFoldingDefine;
using PCodeFoldingDefine = std::shared_ptr<CodeFoldingDefine> ;

class CodeFoldingDefines {
private:
    std::vector<PCodeFoldingDefine> fRegions;
public:
    int count();
    PCodeFoldingDefine add(bool addEnding, const QChar& openSymbol, const QChar& closeSymbol, const QString& highlight);
    PCodeFoldingDefine get(int index);
};
typedef std::shared_ptr<CodeFoldingDefines> PSynFoldRegions;


struct CodeFoldingDefine {
    bool addEnding;
    QChar openSymbol;
    QChar closeSymbol;
    QString highlight;
};

struct CodeFoldingOptions {
      bool indentGuides;
      bool fillIndents;
      bool showCollapsedLine;
      QColor collapsedLineColor;
      QColor folderBarLinesColor;
      QColor indentGuidesColor;
      CodeFoldingDefines foldRegions;
      CodeFoldingOptions();
};

class CodeFoldingRange;
typedef std::shared_ptr<CodeFoldingRange> PCodeFoldingRange;
class CodeFoldingRanges;
typedef std::shared_ptr<CodeFoldingRanges> PCodeFoldingRanges;

class CodeFoldingRanges{
private:
    QVector<PCodeFoldingRange> mRanges;
public:
    PCodeFoldingRange range(int index);
    void clear();
    int count() const;
    CodeFoldingRanges();
    PCodeFoldingRange addByParts(PCodeFoldingRange parent, PCodeFoldingRanges allFold,
                               int fromLine, int toLine);

    void insert(int index, PCodeFoldingRange range);
    void remove(int index);
    void add(PCodeFoldingRange foldRange);
    PCodeFoldingRange operator[](int index) const;
};

// A single fold
class CodeFoldingRange {
public:
    int fromLine; // Beginning line
    int toLine; // End line
    int linesCollapsed; // Number of collapsed lines
    PCodeFoldingRanges subFoldRanges; // Sub fold ranges
    bool collapsed; // Is collapsed?
    int hintMarkLeft;
    std::weak_ptr<CodeFoldingRange> parent;
    bool parentCollapsed();
    void move(int count);
    explicit CodeFoldingRange(PCodeFoldingRange parent, int fromLine, int toLine);
};

}
#endif // CODEFOLDING_H
