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
#ifndef PAINTER_H
#define PAINTER_H

#include <QColor>
#include <QPainter>
#include <QString>
#include "types.h"
#include "syntaxer/syntaxer.h"
#include "gutter.h"

namespace QSynedit {
struct TokenTextInfo {
    int left;
    QString token;
    QFont font;
    QColor foreground;
};

struct TokenBackgroundInfo {
    int left;
    int width;
    QColor background;
};


class QSynEdit;
class QSynEditPainter
{
    struct SynTokenAccu {
        int width;
        int left;
        int startGlyph;
        int endGlyph;
//        QString s;
        QColor foreground;
        QColor background;
        FontStyles style;
        QFont font;
        bool showSpecialGlyphs;
    };

public:
    QSynEditPainter(QSynEdit * edit,QPainter* painter,
                    int firstRow,
                    int lastRow,
                    int left, int right);
    QSynEditPainter(const QSynEditPainter&)=delete;
    QSynEditPainter& operator=(const QSynEditPainter&)=delete;

    void paintEditingArea(const QRect& clip);
    void paintGutter(const QRect& clip);

private:
    QColor colEditorBG();
    void computeSelectionInfo();
    void getDrawingColors(bool selected, QColor& foreground , QColor& background);
    int fixXValue(int xpos);
    void paintLine();
    void paintToken(
            const QString& lineText,
            const QList<int> &glyphStartCharList,
            const QList<int> &glyphStartPositionList,
            int startGlyph,
            int endGlyph,
            int tokenWidth, int tokenLeft,
            int first, int last,
            QColor foreground,
            QColor background,
            const QFont& font, bool showGlyphs
            );
    void paintEditAreas(const EditingAreaList& areaList);
    void paintHighlightToken(const QString& lineText,
                             const QList<int> &glyphStartCharList,
                             const QList<int> &glyphStartPositionsList,
                             bool bFillToEOL);
    void addHighlightToken(
            const QString& lineText,
            const QString& token, int tokenLeft,
            int line, PTokenAttribute p_Attri, bool showGlyphs,
            const QList<int> glyphStartCharList,
            int tokenStartChar,
            int tokenEndChar,
            bool calcGlyphPosition,
            QList<int> &glyphStartPositionList,
            int &tokenWidth
            );

    void paintFoldAttributes();
    void getBraceColorAttr(int level, PTokenAttribute &attr);
    void paintLines();

private:
    QSynEdit* mEdit;
    QPainter* mPainter;
    bool bDoRightEdge; // right edge
    int nRightEdge;
    // selection info
    bool bAnySelection; // any selection visible?
    DisplayCoord mSelStart; // start of selected area
    DisplayCoord mSelEnd; // end of selected area
    // info about normal and selected text and background colors
    bool mIsSpecialLine, mIsCurrentLine, mIsLineEndSelected;
    QColor colFG, colBG;
    QColor colSelFG, colSelBG;
    QColor colSpFG, colSpBG;
    // info about selection of the current line
    int mLineSelStart, mLineSelEnd;
    bool mHasSelectionInLine;
    int mCurrentLineWidth;
    // painting the background and the text
    QRect mRcLine, mRcToken;
    int mFirstLine, mLastLine;
    QList<TokenTextInfo> mLineTokens;
    QList<TokenBackgroundInfo> mLineTokenBackgrounds;

    QRect mClip;
    int mFirstRow, mLastRow, mLeft, mRight;
    SynTokenAccu mTokenAccu;
};

}

#endif // PAINTER_H
