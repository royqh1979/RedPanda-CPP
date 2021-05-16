#ifndef TEXTPAINTER_H
#define TEXTPAINTER_H

#include <QColor>
#include <QPainter>
#include <QString>
#include "Types.h"
#include "highlighter/base.h"

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
    SynEditTextPainter(SynEdit * edit);

    QColor colEditorBG();
    void ComputeSelectionInfo();
    void setDrawingColors(bool Selected);
    int ColumnToXValue(int Col);
    void PaintToken(const QString& Token, int TokenLen, int ColumnsBefore,
                    int First, int Last, bool isSelection);
    void PaintEditAreas(PSynEditingAreaList areaList);
    void PaintHighlightToken(bool bFillToEOL);
    bool TokenIsSpaces(bool& bSpacesTest, const QString& Token, bool& bIsSpaces);
    void AddHighlightToken(const QString& Token, int ColumnsBefore, int TokenColumns,
                           int cLine, PSynHighlighterAttribute p_Attri);

    void PaintFoldAttributes();
    void GetBraceColorAttr(int level, const PSynHighlighterAttribute& attr);
    void PaintLines();
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
