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
#ifndef QSYNEDIT_CODEFOLDING_H
#define QSYNEDIT_CODEFOLDING_H
#include <QColor>
#include <vector>
#include <memory>
#include <QVector>

namespace QSynedit {
struct CodeFoldingOptions {
      bool indentGuides;
      bool fillIndents;
      bool rainbowIndentGuides;
      bool rainbowIndents;
      bool showCollapsedLine;
      QColor collapsedLineColor;
      QColor folderBarLinesColor;
      QColor indentGuidesColor;
      //CodeFoldingDefines foldRegions;
      CodeFoldingOptions();
};

class CodeBlock;
typedef std::shared_ptr<CodeBlock> PCodeBlock;

// A single fold
class CodeBlock {
public:
    explicit CodeBlock(PCodeBlock parent, int fromLine, int toLine);
    CodeBlock(const CodeBlock&)=delete;
    CodeBlock& operator=(const CodeBlock&)=delete;
    int fromLine; // Beginning line
    int toLine; // End line
    QVector<PCodeBlock> subBlocks; // Sub fold ranges
    bool collapsed; // Is collapsed?
    std::weak_ptr<CodeBlock> parent;
    int linesCollapsed() const { return collapsed?(toLine - fromLine):0; }
    bool parentCollapsed() const;
    void move(int count);
};

}
#endif // CODEFOLDING_H
