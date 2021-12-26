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
#ifndef TEXTPAINTER_H
#define TEXTPAINTER_H

#include <QColor>
#include <QPainter>
#include <QString>
#include "Types.h"
#include "highlighter/base.h"
#include "../utils.h"
#include "MiscClasses.h"

class SynEdit;
class SynEditTextPainter
{
    struct SynTokenAccu {
        int Columns;
        int ColumnsBefore;
        QString s;
        QColor FG;
        QColor BG;
        SynFontStyles Style;
    };

public:
    SynEditTextPainter(SynEdit * edit,QPainter* painter,int FirstRow, int LastRow,
                       int FirstCol, int LastCol);
    void paintTextLines(const QRect& clip);
    void paintGutter(const QRect& clip);

private:
    QColor colEditorBG();
    void ComputeSelectionInfo();
    void setDrawingColors(bool Selected);
    int ColumnToXValue(int Col);
    void PaintToken(const QString& Token, int TokenLen, int ColumnsBefore,
                    int First, int Last, bool isSelection);
    void PaintEditAreas(const SynEditingAreaList& areaList);
    void PaintHighlightToken(bool bFillToEOL);
    bool TokenIsSpaces(bool& bSpacesTest, const QString& Token, bool& bIsSpaces);
    void AddHighlightToken(const QString& Token, int ColumnsBefore, int TokenColumns,
                           int cLine, PSynHighlighterAttribute p_Attri);

    void PaintFoldAttributes();
    void GetBraceColorAttr(int level, PSynHighlighterAttribute &attr);
    void PaintLines();
    void drawMark(PSynEditMark aMark,int& aGutterOff, int aMarkRow);

private:
    SynEdit* edit;
    QPainter* painter;
    bool bDoRightEdge; // right edge
    int nRightEdge;
    // selection info
    bool bAnySelection; // any selection visible?
    DisplayCoord vSelStart; // start of selected area
    DisplayCoord vSelEnd; // end of selected area
    // info about normal and selected text and background colors
    bool bSpecialLine, bLineSelected, bCurrentLine;
    QColor colFG, colBG;
    QColor colSelFG, colSelBG;
    QColor colSpFG, colSpBG;
    // info about selection of the current line
    int nLineSelStart, nLineSelEnd;
    bool bComplexLine;
    // painting the background and the text
    QRect rcLine, rcToken;
    int vFirstLine, vLastLine;

    QRect AClip;
    int aFirstRow, aLastRow, FirstCol, LastCol;
    SynTokenAccu TokenAccu;
};

#endif // TEXTPAINTER_H
