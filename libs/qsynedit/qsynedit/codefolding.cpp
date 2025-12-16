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
#include "codefolding.h"
#include "constants.h"


namespace QSynedit {

CodeFoldingOptions::CodeFoldingOptions():
    indentGuides{true},
    fillIndents{false},
    rainbowIndentGuides{true},
    rainbowIndents{true},
    showCollapsedLine{true},
    collapsedLineColor{"black"},
    folderBarLinesColor{"black"},
    indentGuidesColor("gray")
{

}


CodeBlock::CodeBlock(PCodeBlock parent, int fromLine, int toLine)
{
    this->parent = parent;
    this->fromLine = fromLine;
    this->toLine = toLine;
    this->collapsed = false;
}

bool CodeBlock::parentCollapsed() const
{
    PCodeBlock parentFold = parent.lock();
    // Find first parent that is collapsed
    while (parentFold) {
        if (parentFold->collapsed) {
            return true;
        }
        parentFold = parentFold->parent.lock();
    }
    return false;
}

void CodeBlock::move(int count)
{
    fromLine += count;
    toLine += count;
}

}
