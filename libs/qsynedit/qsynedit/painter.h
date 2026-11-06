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
#ifndef QSYNEDIT_PAINTER_H
#define QSYNEDIT_PAINTER_H

#include <QColor>
#include <QPainter>
#include <QString>
#include <climits>
#include "types.h"

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


class Syntaxer;
class Document;
using PSyntaxer = std::shared_ptr<Syntaxer>;
class TokenAttribute;
using PTokenAttribute = std::shared_ptr<TokenAttribute>;
class QSynEdit;
class QSynEditPainter
{
    struct SynTokenAccu {
        int width = 0;
        int left = 0;
        int startGlyph = 0;
        int endGlyph = 0;
//        QString s;
        QColor foreground;
        QColor background;
        FontStyles style = FontStyle::fsNone;
        QFont font;
        bool showSpecialGlyphs = false;
    };

    // per-row scratch state, created as a local in paintLines' row loop and
    // passed down by reference; grouping it here keeps the painter's
    // per-paint configuration (the members far below) untouched during the
    // row loop
    struct RowPaintContext {
        // bounds of the wrap segment being painted (absolute glyph x);
        // [0, INT_MAX] when word wrap is off (the single segment spans the
        // whole line)
        int segmentStartX = 0;
        int segmentEndX = INT_MAX;
        // token/selection clipping: the intersection of the visible region
        // (mLeft..mRight) and the wrap segment bounds. With word wrap the
        // segment bounds are the tighter side; without it the visible
        // region rules (horizontal scrolling)
        int tokenClipLeft = 0;
        int tokenClipRight = INT_MAX;
        // info about normal and selected text and background colors
        bool isSpecialLine = false;
        bool isCurrentLine = false;
        bool isRowEndSelected = false;
        QColor foreground, background;
        QColor selectionForeground, selectionBackground;
        QColor specialLineForeground, specialLineBackground;
        // selection of the current row
        int rowSelStart = 0, rowSelEnd = 0;
        bool hasSelectionInRow = false;
        int lineWidth = 0;
        // painting the background and the text
        QRect rcRow, rcToken;
        QList<TokenTextInfo> tokens;
        QList<TokenBackgroundInfo> tokenBackgrounds;
        SynTokenAccu tokenAccu;
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
    QColor lineBackgroundColor(bool isCurrentLine);
    void computeSelectionInfo();
    void getDrawingColors(const RowPaintContext &row, bool selected, QColor& foreground , QColor& background);
    int lineXToWidgetX(int segmentStartX, int xpos);
    void flushRowTokens(RowPaintContext &row);
    void paintToken(
            RowPaintContext &row,
            const QString& lineText,
            const QList<int> &glyphStartCharList,
            const QList<int> &glyphStartXList,
            int startGlyph,
            int endGlyph,
            int tokenWidth, int tokenLeft,
            int first, int last,
            QColor foreground,
            QColor background,
            const QFont& font, bool showGlyphs
            );
    void paintEditAreas(RowPaintContext &row, const EditingAreaList& areaList);
    void paintHighlightToken(RowPaintContext &row,
                             const QString& lineText,
                             const QList<int> &glyphStartCharList,
                             const QList<int> &glyphStartPositionsList,
                             bool fillToEOL);
    // recomputeGlyphPositions: the painted text differs from the document
    // line (IME preedit), so glyph x positions are computed while scanning
    // instead of read from the layout cache (and never written back)
    void addHighlightToken(
            RowPaintContext &row,
            const QString& lineText,
            const QString& token, int tokenLeft,
            int line, PTokenAttribute p_Attri, bool showGlyphs,
            const QList<int> &glyphStartCharList,
            int tokenStartChar,
            int tokenEndChar,
            bool recomputeGlyphPositions,
            QList<int> &glyphStartXList,
            int &tokenWidth
            );

    void paintFoldAttributes();
    void getRainbowColorAttr(int level, PTokenAttribute &attr);
    void getBraceColorAttr(int level, PTokenAttribute &attr);
    void paintLines();
    // resolve the (line, segment) of a display row by walking from the
    // anchor; never touches the row index
    void seekFromAnchor(int row, int &line, int &segment) const;

private:
    QSynEdit* mEdit;
    QPainter* mPainter;
    // read-only view of the document: the painter never edits, so only
    // const access is allowed (compile-enforced)
    const Document *mDocument;
    bool doRightEdge; // right edge
    int rightEdge;
    // selection info
    bool anySelection; // any selection visible?
    VisualPos mSelStart; // start of selected area
    VisualPos mSelEnd; // end of selected area
    int mFirstLine, mLastLine;

    // per-row scratch state lives in RowPaintContext (defined above): a
    // local in paintLines' row loop, passed down by reference
    QRect mClip;
    // visible horizontal region in absolute glyph x (set once by
    // paintEvent from the clip rect and mLeftPos; the horizontal scroll
    // offset lives here). Never overwritten per row.
    int mFirstRow, mLastRow, mLeft, mRight;
};
}

#endif // PAINTER_H
