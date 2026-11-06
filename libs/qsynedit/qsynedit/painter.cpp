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
#include "painter.h"
#include "qsynedit.h"
#include "document.h"
#include "constants.h"
#include "syntaxer/syntaxer.h"
#include <cmath>
#include <climits>
#include <QDebug>

namespace QSynedit {

QSynEditPainter::QSynEditPainter(QSynEdit *edit, QPainter *painter, int firstRow, int lastRow, int left, int right):
    mEdit{edit},
    mPainter{painter},
    mDocument{edit->document().get()},
    mFirstRow{firstRow},
    mLastRow{lastRow},
    mLeft{left},
    mRight{right}
{
}

void QSynEditPainter::paintEditingArea(const QRect& clip)
{
    mPainter->fillRect(clip, mEdit->mBackgroundColor);
    mClip = clip;
    mFirstLine = mEdit->rowToLine(mFirstRow);
    mLastLine = mEdit->rowToLine(mLastRow);
    // If the right edge is visible and in the invalid area, prepare to paint it.
    // Do this first to realize the pen when getting the dc variable.
    doRightEdge = false;
    if (mEdit->mRightEdge > 0) { // column value
        rightEdge = mEdit->textOffset() + mEdit->mRightEdge * mEdit->mCharWidth; // pixel value
        if (rightEdge >= mClip.left() &&rightEdge <= mClip.right()) {
            doRightEdge = true;
        }
    }

    // Paint the visible text lines. To make this easier, compute first the
    // necessary information about the selected area: is there any visible
    // selected area, and what are its lines / columns?
    if (mLastLine >= mFirstLine) {
        computeSelectionInfo();
        paintLines();
    }
    //painter->setClipping(false);

    // If anything of the two pixel space before the text area is visible, then
    // fill it with the component background color.
    if (mClip.left() <mEdit->mGutterWidth + 2) {
        QRect rcToken = mClip;
        rcToken.setLeft( std::max(mClip.left(), mEdit->mGutterWidth));
        rcToken.setRight(mEdit->mGutterWidth + 2);
        // Paint whole left edge of the text with same color.
        // (value of WhiteAttribute can vary in e.g. MultiSyn)
        mPainter->fillRect(rcToken,lineBackgroundColor(false));
        // Adjust the invalid area to not include this area.
        mClip.setLeft(rcToken.right());
    }
    // If there is anything visible below the last line, then fill this as well.
    QRect rcTail = mClip;
    rcTail.setTop((mLastRow - mEdit->yposToRow(0) + 1) * mEdit->mTextHeight);
    if (rcTail.top() < rcTail.bottom()) {
        mPainter->fillRect(rcTail,lineBackgroundColor(false));
        // Draw the right edge if necessary.
        if (doRightEdge) {
            QPen pen(mEdit->mRightEdgeColor,1);
            mPainter->setPen(pen);
            mPainter->drawLine(rightEdge, rcTail.top(),rightEdge, rcTail.bottom() + 1);
        }
    }

    // This messes with pen colors, so draw after right margin has been drawn
    paintFoldAttributes();
}

void QSynEditPainter::paintGutter(const QRect& clip)
{
    QRectF rcLine, rcFold;
    int x;

    mClip = clip;

    mPainter->fillRect(mClip,mEdit->mGutter.color());

    rcLine=mClip;
    if (mEdit->mGutter.showLineNumbers()) {
        // prepare the rect initially
        rcLine = mClip;
        rcLine.setRight( std::max(rcLine.right(), mEdit->mGutterWidth - 2.0));
        rcLine.setBottom(rcLine.top());

        if (mEdit->mGutter.useFontStyle()) {
            mPainter->setFont(mEdit->mGutter.font());
        } else {
            QFont newFont = mEdit->font();
            newFont.setBold(false);
            newFont.setItalic(false);
            newFont.setStrikeOut(false);
            newFont.setUnderline(false);
            mPainter->setFont(newFont);
        }
        QColor textColor;
        if (mEdit->mGutter.textColor().isValid()) {
            textColor = mEdit->mGutter.textColor();
        } else {
            textColor = mEdit->mForegroundColor;
        }
        // draw each line if it is not hidden by a fold
        CharPos selectionStart = mEdit->selBegin();
        CharPos selectionEnd = mEdit->selEnd();
        for (int row = mFirstRow; row <= mLastRow; row++) {
            int line, segment;
            mEdit->rowToLineSegment(row, line, segment);
            // line numbers are only drawn on a line's first wrap segment
            if (segment != 0)
                continue;
            if ((line >= mDocument->count()) && (mDocument->count() > 0 ))
                break;
            if (mEdit->mGutter.activeLineTextColor().isValid()) {
                if (
                        (mEdit->mCaretLine==line)     ||
                        (mEdit->mActiveSelectionMode == SelectionMode::Column && line >= selectionStart.line && line <= selectionEnd.line)
                        )
                    mPainter->setPen(mEdit->mGutter.activeLineTextColor());
                else
                    mPainter->setPen(textColor);
            } else {
                mPainter->setPen(textColor);
            }
            int lineTop = row * mEdit->mTextHeight - mEdit->mTopPos;

            // next line rect
            rcLine.setTop(lineTop);
            rcLine.setBottom(rcLine.top() + mEdit->mTextHeight);

            QString s = mEdit->mGutter.formatLineNumber(line+1);

            mEdit->onGutterGetText(line,s);
            QRectF textRect;
            textRect = mPainter->boundingRect(textRect, Qt::AlignLeft,s);
            mPainter->drawText(
                        (mEdit->mGutterWidth - mEdit->mGutter.rightOffset() - 2) - textRect.width(),
                        rcLine.bottom() - (mEdit->mTextHeight - int(textRect.height())) / 2 - mPainter->fontMetrics().descent(),
                        s
                        );
        }
    }

    // Draw the folding lines and squares
    if (mEdit->useCodeFolding()) {
      int lineWidth = std::max(0.0,std::ceil(mEdit->font().pixelSize() / 15));
      for (int row = mLastRow; row>= mFirstRow; row--) {
          int line, segment;
          mEdit->rowToLineSegment(row, line, segment);
          // fold vertical lines are drawn on EVERY wrap segment (continuous
          // across the fold); only the +/- button is first-segment-only
          if ((line >= mDocument->count()) && (mDocument->count() != 0))
            continue;

        // Form a rectangle for the square the user can click on
          rcFold.setLeft(mEdit->mGutterWidth - mEdit->mGutter.rightOffset());
          rcFold.setTop(row * mEdit->mTextHeight - mEdit->mTopPos);
          rcFold.setRight(rcFold.left() + mEdit->mGutter.rightOffset() - 4);
          rcFold.setBottom(rcFold.top() + mEdit->mTextHeight);

          mPainter->setPen(QPen(mEdit->mCodeFolding.folderBarLinesColor,lineWidth));

        // Need to paint a line?
          if (mEdit->foldAroundLine(line)) {
          x = rcFold.left() + (rcFold.width() / 2);
          mPainter->drawLine(x,rcFold.top(), x, rcFold.bottom());
        }

        // Need to paint a line end?
          if (mEdit->foldEndAtLine(line)) {
            x = rcFold.left() + (rcFold.width() / 2);
            mPainter->drawLine(x,rcFold.top(), x, rcFold.top() + rcFold.height() / 2);
            mPainter->drawLine(x,
                              rcFold.top() + rcFold.height() / 2,
                              rcFold.right() - 2 ,
                              rcFold.top() + rcFold.height() / 2);
        }
        // Any fold ranges beginning on this line?
          PCodeBlock foldRange = mEdit->foldStartAtLine(line);
        if (foldRange && segment == 0) {
            // Draw the bottom part of a line
            if (!foldRange->collapsed) {
                x = rcFold.left() + (rcFold.width() / 2);
                mPainter->drawLine(x, rcFold.top() + rcFold.height() / 2,
                                  x, rcFold.bottom());
            }

            // make a square rect
            int size = std::min(mEdit->font().pixelSize() * 4 / 5, mEdit->mGutter.rightOffset()) - lineWidth;
            float centerX = rcFold.left() + rcFold.width() / 2.0;
            float centerY = rcFold.top() + rcFold.height() / 2.0;
            float halfSize = size / 2.0;
            rcFold.setLeft(centerX - halfSize);
            rcFold.setRight(centerX + halfSize);
            rcFold.setTop(centerY - halfSize);
            rcFold.setBottom(centerY + halfSize);

            // Paint the square the user can click on
            mPainter->setBrush(mEdit->mGutter.color());
            //mPainter->setPen(mEdit->mCodeFolding.folderBarLinesColor);
            mPainter->drawRect(rcFold);

            // Paint minus sign
            mPainter->drawLine(
                        rcFold.left() + lineWidth * 2 + 1 , centerY,
                        rcFold.right() - lineWidth * 2 , centerY );
            // Paint vertical line of plus sign
            if (foldRange->collapsed) {
                mPainter->drawLine(centerX, rcFold.top() + lineWidth * 2,
                                  centerX, rcFold.bottom() - lineWidth * 2 );
            }
        }
      }
    }

    for (int row = mFirstRow; row <= mLastRow; row++) {
        int line, segment;
        mEdit->rowToLineSegment(row, line, segment);
        // gutter marks (breakpoints/bookmarks) only on a line's first segment
        if (segment != 0)
            continue;
        if ((line >= mDocument->count()) && (mDocument->count() != 0))
            break;
        mEdit->onGutterPaint(*mPainter,line, 0, (row - mEdit->yposToRow(0)) * mEdit->mTextHeight);
    }
}

QColor QSynEditPainter::lineBackgroundColor(bool isCurrentLine)
{
    if (mEdit->mActiveLineColor.isValid() && isCurrentLine) {
        return mEdit->mActiveLineColor;
    } else {
        return mEdit->mBackgroundColor;
    }
}

void QSynEditPainter::computeSelectionInfo()
{
    CharPos selStart;
    CharPos selEnd;
    anySelection = false;
    // Only if selection is visible anyway.
    anySelection = true;
    // Get the *real* start of the selected area.
    if (mEdit->mSelectionBegin.line < mEdit->mSelectionEnd.line) {
        selStart = mEdit->mSelectionBegin;
        selEnd = mEdit->mSelectionEnd;
    } else if (mEdit->mSelectionBegin.line > mEdit->mSelectionEnd.line) {
        selEnd = mEdit->mSelectionBegin;
        selStart = mEdit->mSelectionEnd;
    } else if (mEdit->mSelectionBegin.ch != mEdit->mSelectionEnd.ch) {
        // it is only on this line.
        selStart.line = mEdit->mSelectionBegin.line;
        selEnd.line = selStart.line;
        if (mEdit->mSelectionBegin.ch < mEdit->mSelectionEnd.ch) {
            selStart.ch = mEdit->mSelectionBegin.ch;
            selEnd.ch = mEdit->mSelectionEnd.ch;
        } else {
            selStart.ch = mEdit->mSelectionEnd.ch;
            selEnd.ch = mEdit->mSelectionBegin.ch;
        }
    } else
        anySelection = false;
    if (mEdit->mInputPreeditString.length()>0) {
        if (selStart.line == mEdit->mCaretLine && selStart.ch >= mEdit->mCaretCh) {
            selStart.ch+=mEdit->mInputPreeditString.length();
        }
        if (selEnd.line == mEdit->mCaretLine && selEnd.ch > mEdit->mCaretCh) {
            selEnd.ch+=mEdit->mInputPreeditString.length();
        }
    }
    // If there is any visible selection so far, then test if there is an
    // intersection with the area to be painted.
    if (anySelection) {
        // Don't care if the selection is not visible.
        anySelection = (selEnd.line >= mFirstLine) && (selStart.line <= mLastLine);
        if (anySelection) {
            // Transform the selection from text space into screen space
            mSelStart.row = mEdit->lineSegmentToRow(selStart.line,
                                                    mEdit->wrapSegmentOfChar(selStart.line, selStart.ch));
            mSelEnd.row = mEdit->lineSegmentToRow(selEnd.line,
                                                  mEdit->wrapSegmentOfChar(selEnd.line, selEnd.ch));
            if (mEdit->mInputPreeditString.length() > 0
                    && selStart.line == mEdit->mCaretLine) {
                QString lineTextWithPreedit = mEdit->lineText().left(mEdit->mCaretCh)
                        + mEdit->mInputPreeditString
                        + mEdit->lineText().mid(mEdit->mCaretCh);
                mSelStart.x = mEdit->charToGlyphLeft(selStart.line, lineTextWithPreedit,selStart.ch);
            } else {
                mSelStart.x = mEdit->charToGlyphLeft(selStart.line,selStart.ch);
            }
            if (mEdit->mInputPreeditString.length() > 0
                    && selEnd.line == mEdit->mCaretLine) {
                QString lineTextWithPreedit = mEdit->lineText().left(mEdit->mCaretCh)
                        + mEdit->mInputPreeditString
                        + mEdit->lineText().mid(mEdit->mCaretCh);
                mSelEnd.x = mEdit->charToGlyphLeft(selEnd.line, lineTextWithPreedit,selEnd.ch);
            } else {
                mSelEnd.x = mEdit->charToGlyphLeft(selEnd.line, selEnd.ch);
            }
            // In the column selection mode sort the begin and end of the selection,
            // this makes the painting code simpler.
            if (mEdit->mActiveSelectionMode == SelectionMode::Column && mSelStart.x > mSelEnd.x)
                std::swap(mSelStart.x, mSelEnd.x);
        }
    }
}

void QSynEditPainter::getDrawingColors(const RowPaintContext &ctx, bool selected, QColor &foreground, QColor &background)
{
    if (selected) {
        if (ctx.selectionForeground.isValid())
            foreground = ctx.selectionForeground;
        else
            foreground = ctx.foreground;
        if (ctx.selectionBackground.isValid())
            background = ctx.selectionBackground;
        else
            background = ctx.background;
    } else {
        foreground = ctx.foreground;
        background = ctx.background;
    }
}

int QSynEditPainter::lineXToWidgetX(int segmentStartX, int xpos)
{
    return mEdit->textOffset() + xpos - segmentStartX;
}

void QSynEditPainter::flushRowTokens(RowPaintContext &ctx)
{
    QRect rect = ctx.rcRow;
    for (int i=0;i<ctx.tokenBackgrounds.length();i++) {
        rect.setX(ctx.tokenBackgrounds[i].left);
        rect.setWidth(ctx.tokenBackgrounds[i].width);
        mPainter->fillRect(rect, ctx.tokenBackgrounds[i].background);
    }

    QFont font;
    QFontMetrics fm{font};
    int lineHeight = ctx.rcRow.height();
    for (int i=0;i<ctx.tokens.length();i++) {
        if (font!=ctx.tokens[i].font) {
            font = ctx.tokens[i].font;
            fm = QFontMetrics{font};
        }
        int fontHeight = fm.descent() + fm.ascent();
        int linePadding = (lineHeight - fontHeight) / 2;
        int y = ctx.rcRow.bottom() - linePadding - fm.descent();
        if (font!=mPainter->font())
            mPainter->setFont(font);
        QPen pen(ctx.tokens[i].foreground);
        if (pen!=mPainter->pen())
            mPainter->setPen(pen);
        mPainter->drawText(ctx.tokens[i].left,y, ctx.tokens[i].token);
    }
    ctx.tokens.clear();
    ctx.tokenBackgrounds.clear();
}

void QSynEditPainter::paintToken(
        RowPaintContext &ctx,
        const QString& lineText,
        const QList<int> &glyphStartCharList,
        const QList<int> &glyphStartXList,
        int startGlyph,
        int endGlyph,
        int tokenWidth, int tokenLeft,
        int first, int last,
        QColor foreground,
        QColor background,
        const QFont& font, bool showGlyphs)
{
    bool startPaint;
    int tokenRight = tokenWidth+tokenLeft;

    if (last >= first && ctx.rcToken.right() > ctx.rcToken.left()) {
        int x = lineXToWidgetX(ctx.segmentStartX, first);
        first -= tokenLeft;
        last -= tokenLeft;
        QRect rcTokenBack = ctx.rcToken;
        TokenBackgroundInfo backInfo;
        backInfo.left=rcTokenBack.left();
        backInfo.width=rcTokenBack.width();
        backInfo.background=background;
        ctx.tokenBackgrounds.append(backInfo);
        //mPainter->fillRect(rcTokenBack,mPainter->brush());
        if (first > tokenWidth) {
        } else {
            int accWidth=0;
            startPaint = false;
            for (int i=startGlyph; i<endGlyph;i++) {
                int glyphStart = glyphStartCharList[i];
                int glyphLen = calcSegmentInterval(glyphStartCharList,lineText.length(),i);
                QString glyph = lineText.mid(glyphStart,glyphLen);
                int glyphWidth = calcSegmentInterval(glyphStartXList, tokenRight, i);
                if (accWidth+glyphWidth>first) {
                    if (!startPaint ) {
                        x-= (first - accWidth - 1) ;
                        startPaint = true;
                    }
                }
                if (startPaint) {
                    bool drawed = false;
                    if (mEdit->mOptions.testFlag(EditorOption::LigatureSupport))  {
                        bool tryLigature = false;
                        if (glyph.length()==0) {
                        } else if (glyph.length()==1 && glyph.front().unicode()<=32){
                        } else if (mEdit->mOptions.testFlag(EditorOption::ForceMonospace)
                                   && glyphWidth != mPainter->fontMetrics().horizontalAdvance(glyph)) {
                        } else {
                            tryLigature = true;
                        }
                        if (tryLigature) {
                            QString textToPaint = glyph;
                            while(i+1<endGlyph && accWidth + glyphWidth < last) {
                                int nextGlyphStart = glyphStartCharList[i+1];
                                int nextGlyphLen = calcSegmentInterval(glyphStartCharList,lineText.length(),i+1);
                                QString glyph2 = lineText.mid(nextGlyphStart,nextGlyphLen);
                                // if (!OperatorGlyphs.contains(glyph))
                                //     break;
                                if ( glyph2.length()<1
                                     ||
                                     (glyph2.length()==1
                                      && glyph2.front().unicode()<=32))
                                    break;
                                int glyph2Width = calcSegmentInterval(glyphStartXList, tokenRight, i+1);
                                if (mEdit->mOptions.testFlag(EditorOption::ForceMonospace)) {
                                    if (glyph2Width != mPainter->fontMetrics().horizontalAdvance(glyph2)) {
                                        break;
                                    }
                                }
                                i++;
                                glyphWidth += glyph2Width;
                                textToPaint += glyph2;
                            }
                            TokenTextInfo tokenInfo;
                            tokenInfo.left = x;
                            tokenInfo.token = textToPaint;
                            tokenInfo.foreground=foreground;
                            tokenInfo.font = font;
                            ctx.tokens.append(tokenInfo);
                            drawed = true;
                        }
                    }
                    if (!drawed) {
                        if (glyph.length()>0) {
                            QString textToPaint = glyph;
                            int padding=0;
                            if (showGlyphs) {
                                switch(glyph.front().unicode()) {
                                case '\t':
                                    textToPaint=TabGlyph;
                                    padding=(glyphWidth-mPainter->fontMetrics().horizontalAdvance(TabGlyph))/2;
                                    break;
                                case ' ':
                                    textToPaint=SpaceGlyph;
                                    break;
                                default:
                                    break;
                                }
                            }
                            if (textToPaint!=" " && textToPaint!="\t") {
                                TokenTextInfo tokenInfo;
                                tokenInfo.left = x+padding;
                                tokenInfo.token = textToPaint;
                                tokenInfo.foreground=foreground;
                                tokenInfo.font = font;
                                ctx.tokens.append(tokenInfo);
                            }
                        }
                    }
                    x += glyphWidth;
                }
                accWidth += glyphWidth;
                if (accWidth >= last)
                    break;
            }
        }

        ctx.rcToken.setLeft(ctx.rcToken.right()+1);
    }
}

void QSynEditPainter::paintEditAreas(RowPaintContext &ctx, const EditingAreaList &areaList)
{
    QRect rc;
    int x1,x2;
    rc=ctx.rcRow;
    rc.setBottom(rc.bottom()-1);
    for (const PEditingArea& p:areaList) {
        int penWidth;
        if (p->type == EditingAreaType::eatWaveUnderLine) {
            if (mEdit->font().pixelSize()>=16)
                penWidth = mEdit->font().pixelSize() / 16;
            else
                penWidth = 1;
        } else {
            if (mEdit->font().pixelSize()>=32)
                penWidth = mEdit->font().pixelSize() / 16;
            else if (mEdit->font().pixelSize()>=14)
                penWidth = 2;
            else
                penWidth = 1;
        }
        if (p->beginX > mRight)
          continue;
        if (p->endX < mLeft)
          continue;
        if (p->beginX < mLeft)
          x1 = mLeft;
        else
          x1 = p->beginX;
        if (p->endX > mRight)
          x2 = mRight;
        else
          x2 = p->endX;
        rc.setLeft(lineXToWidgetX(ctx.segmentStartX, x1));
        rc.setRight(lineXToWidgetX(ctx.segmentStartX, x2));
        QPen pen;
        pen.setColor(p->color);
        pen.setWidthF(penWidth);
        mPainter->setPen(pen);
        mPainter->setBrush(Qt::NoBrush);
        int lineHeight = rc.height();
        int fontHeight = mPainter->fontMetrics().descent() + mPainter->fontMetrics().ascent();
        int linePadding = (lineHeight - fontHeight) / 2;
        switch(p->type) {
        case EditingAreaType::eatRectangleBorder:
            rc.setTop(rc.top()+penWidth/2);
            rc.setRight(rc.right() + penWidth );
            rc.setBottom(rc.bottom()-penWidth/2);
            mPainter->drawRect(rc);
            break;
        case EditingAreaType::eatUnderLine:
            mPainter->drawLine(rc.left(),rc.bottom()-linePadding-pen.width(),rc.right(),rc.bottom()-linePadding-pen.width());
            break;
        case EditingAreaType::eatWaveUnderLine: {
            if (linePadding>mPainter->fontMetrics().descent())
                linePadding -= mPainter->fontMetrics().descent();
            else
                linePadding = 0;
            int maxOffset = std::min(3*penWidth, mPainter->fontMetrics().descent());
            maxOffset = std::max(3, maxOffset);
            int offset = maxOffset;
            int lastX=rc.left();
            int lastY=rc.bottom()-offset-linePadding;
            int t = rc.left();
            while (t<rc.right()) {
                t+=maxOffset;
                if (t>=rc.right()) {
                    int diff = t - rc.right();
                    offset = (offset==0)?(maxOffset-diff):diff;
                    t = rc.right();
                    mPainter->drawLine(lastX,lastY,t,rc.bottom()-offset-linePadding);
                } else {
                    offset = maxOffset - offset;
                    mPainter->drawLine(lastX,lastY,t,rc.bottom()-offset-linePadding);
                }
                lastX = t;
                lastY = rc.bottom()-offset-linePadding;
            }
        }
            break;
        }
    }
}

void QSynEditPainter::paintHighlightToken(RowPaintContext &ctx,
                                          const QString& lineText,
                                          const QList<int> &glyphStartCharList,
                                          const QList<int> &glyphStartPositionsList,
                                          bool fillToEOL)
{
    bool isComplexToken;
    int colStart, colEnd, colSelStart, colSelEnd;
    bool unselBefore, hasSelected, unselAfter;
    // Compute some helper variables.
    colStart = std::max(ctx.tokenClipLeft, ctx.tokenAccu.left);
    colEnd = std::min(ctx.tokenClipRight, ctx.tokenAccu.left + ctx.tokenAccu.width);
    if (ctx.hasSelectionInRow) {
        unselBefore = (colStart < ctx.rowSelStart);
        hasSelected = (colStart < ctx.rowSelEnd) && (colEnd >= ctx.rowSelStart);
        unselAfter = (colEnd >= ctx.rowSelEnd);
        isComplexToken = hasSelected && (unselBefore || unselAfter);
    } else {
        hasSelected = false;
        isComplexToken = false;
        unselBefore = false;
        unselAfter = false;
    }
    // Any token chars accumulated?
    if (ctx.tokenAccu.width > 0) {
        // Initialize the colors and the font style.
        ctx.background = ctx.tokenAccu.background;
        ctx.foreground = ctx.tokenAccu.foreground;

        //        if (bSpecialLine && mEdit->mOptions.testFlag(eoSpecialLineDefaultFg))
//            colFG = TokenAccu.FG;


        // Paint the chars
        if (isComplexToken) {
            // first unselected part of the token
            if (unselBefore) {
                QColor unselectedForeground,unselectedBackground;
                getDrawingColors(ctx, false,unselectedForeground,unselectedBackground);
                ctx.rcToken.setRight(lineXToWidgetX(ctx.segmentStartX, ctx.rowSelStart));
                paintToken(
                            ctx,
                            lineText,
                            glyphStartCharList,
                            glyphStartPositionsList,
                            ctx.tokenAccu.startGlyph,
                            ctx.tokenAccu.endGlyph,
                            ctx.tokenAccu.width,ctx.tokenAccu.left,colStart,ctx.rowSelStart,
                            unselectedForeground,unselectedBackground,
                            ctx.tokenAccu.font, ctx.tokenAccu.showSpecialGlyphs);
            }
            // selected part of the token
            QColor foreground,background;
            getDrawingColors(ctx, true,foreground,background);
            colSelStart = std::max(ctx.rowSelStart, colStart);
            colSelEnd = std::min(ctx.rowSelEnd, colEnd);
            ctx.rcToken.setRight(lineXToWidgetX(ctx.segmentStartX, colSelEnd));
            paintToken(
                        ctx,
                        lineText,
                        glyphStartCharList,
                        glyphStartPositionsList,
                        ctx.tokenAccu.startGlyph,
                        ctx.tokenAccu.endGlyph,
                        ctx.tokenAccu.width, ctx.tokenAccu.left, colSelStart, colSelEnd,
                        foreground,background,
                        ctx.tokenAccu.font, ctx.tokenAccu.showSpecialGlyphs);
            // second unselected part of the token
            if (unselAfter) {
                QColor unselectedForeground,unselectedBackground;
                getDrawingColors(ctx, false,unselectedForeground,unselectedBackground);
                ctx.rcToken.setRight(lineXToWidgetX(ctx.segmentStartX, colEnd));
                paintToken(
                            ctx,
                            lineText,
                            glyphStartCharList,
                            glyphStartPositionsList,
                            ctx.tokenAccu.startGlyph,
                            ctx.tokenAccu.endGlyph,
                            ctx.tokenAccu.width, ctx.tokenAccu.left, ctx.rowSelEnd, colEnd,
                            unselectedForeground,unselectedBackground,
                            ctx.tokenAccu.font, ctx.tokenAccu.showSpecialGlyphs);
            }
        } else {
            QColor foreground,background;
            getDrawingColors(ctx, hasSelected,foreground,background);
            ctx.rcToken.setRight(lineXToWidgetX(ctx.segmentStartX, colEnd));
            paintToken(
                        ctx,
                        lineText,
                        glyphStartCharList,
                        glyphStartPositionsList,
                        ctx.tokenAccu.startGlyph,
                        ctx.tokenAccu.endGlyph,
                        ctx.tokenAccu.width, ctx.tokenAccu.left, colStart, colEnd,
                        foreground,background,
                        ctx.tokenAccu.font, ctx.tokenAccu.showSpecialGlyphs);
        }
    }

    // Fill the background to the end of this line if necessary.
    if (fillToEOL && ctx.rcToken.left() < ctx.rcRow.right()) {
        if (ctx.isSpecialLine && ctx.specialLineBackground.isValid())
            ctx.background = ctx.specialLineBackground;
        else
            ctx.background = lineBackgroundColor(ctx.isCurrentLine);
        QColor foreground,background;
        if (ctx.hasSelectionInRow) {
            getDrawingColors(ctx, ctx.isRowEndSelected,foreground,background);
            ctx.rcToken.setRight(ctx.rcRow.right());
            //mPainter->fillRect(ctx.rcToken,mPainter->brush());
        }  else {
            getDrawingColors(ctx, false,foreground,background);
            ctx.rcToken.setRight(ctx.rcRow.right());
            //mPainter->fillRect(ctx.rcToken,mPainter->brush());
        }
        TokenBackgroundInfo backInfo;
        backInfo.left=ctx.rcToken.left();
        backInfo.width=ctx.rcToken.width();
        backInfo.background=background;
        ctx.tokenBackgrounds.append(backInfo);
    }
    if (fillToEOL) {
        flushRowTokens(ctx);
    }
}

// Store the token chars with the attributes in the TokenAccu
// record. This will paint any chars already stored if there is
// a (visible) change in the attributes.
void QSynEditPainter::addHighlightToken(
        RowPaintContext &ctx,
        const QString& lineText,
        const QString& token, int tokenLeft,
        int line, PTokenAttribute attr, bool showGlyphs,
        const QList<int> &glyphStartCharList,
        int tokenStartChar,
        int tokenEndChar,
        bool recomputeGlyphPositions,
        QList<int> &glyphStartXList,
        int &tokenWidth)
{
    int tokenRight;
    int startGlyph, endGlyph;
    if (!recomputeGlyphPositions) {
        tokenRight = std::max(0,tokenLeft);
        startGlyph = searchForSegmentIdx(glyphStartCharList,lineText.length(),tokenStartChar);
        endGlyph = searchForSegmentIdx(glyphStartCharList,lineText.length(),tokenEndChar);
        for (int i=startGlyph;i<endGlyph;i++) {
            int gWidth = calcSegmentInterval(glyphStartXList, ctx.lineWidth, i);
            tokenRight += gWidth;
        }
        tokenWidth = tokenRight-tokenLeft;
        if (tokenRight<ctx.tokenClipLeft) {
            return;
        }
    }
    QColor foreground, background;
    FontStyles style;

    if (attr) {
        foreground = attr->foreground();
        background = attr->background();
        style = attr->styles();
    } else {
        foreground = ctx.foreground;
        background = ctx.background;
        style = getFontStyles(mEdit->font());
    }

//    if (!Background.isValid() || (edit->mActiveLineColor.isValid() && bCurrentLine)) {
//        Background = colEditorBG(ctx.isCurrentLine);
//    }

    mEdit->onPreparePaintHighlightToken(line,mEdit->mSyntaxer->getTokenPos(),
        token,attr,style,foreground,background);

    if (!background.isValid() ) {
        background = lineBackgroundColor(ctx.isCurrentLine);
    }
    if (!foreground.isValid()) {
        foreground = mEdit->mForegroundColor;
    }

    if (ctx.isSpecialLine) {
        if (ctx.specialLineForeground.isValid())
            foreground = ctx.specialLineForeground;
        if (ctx.specialLineBackground.isValid())
            background = ctx.specialLineBackground;
    }

    // Do we have to paint the old chars first, or can we just append?
    bool canAppend = false;
    bool initFont = (ctx.tokenAccu.width==0);
    if (ctx.tokenAccu.width > 0 ) {
        // font style must be the same or token is only spaces
        if (ctx.tokenAccu.style != style) {
            initFont = true;
        } else {
            if (
                (showGlyphs == ctx.tokenAccu.showSpecialGlyphs) &&
              // background color must be the same and
                (ctx.tokenAccu.background == background) &&
              // foreground color must be the same or token is only spaces
              (ctx.tokenAccu.foreground == foreground)) {
                canAppend = true;
            }
        }
        // If we can't append it, then we have to paint the old token chars first.
        if (!canAppend)
            paintHighlightToken(ctx, lineText, glyphStartCharList, glyphStartXList, false);
    }
    if (initFont) {
        ctx.tokenAccu.style = style;
        ctx.tokenAccu.font = mEdit->font();
        ctx.tokenAccu.font.setBold(style & FontStyle::fsBold);
        ctx.tokenAccu.font.setItalic(style & FontStyle::fsItalic);
        ctx.tokenAccu.font.setStrikeOut(style & FontStyle::fsStrikeOut);
        ctx.tokenAccu.font.setUnderline(style & FontStyle::fsUnderline);
    }
    //calculate width of the token ( and update it's glyph start positions )
    if (recomputeGlyphPositions) {
        tokenWidth = mEdit->mGlyphCalculator.updateGlyphStartPositionList(
                    lineText,
                    glyphStartCharList,
                    tokenStartChar,
                    tokenEndChar,
                    QFontMetrics(ctx.tokenAccu.font),
                    glyphStartXList,
                    tokenLeft,
                    tokenRight,
                    startGlyph,
                    endGlyph);
    }

    // Only accumulate tokens if it's visible.
    if (tokenLeft < ctx.tokenClipRight && tokenRight > ctx.tokenClipLeft) {
        if (canAppend) {
            ctx.tokenAccu.width += tokenWidth;
            Q_ASSERT(startGlyph == ctx.tokenAccu.endGlyph);
            ctx.tokenAccu.endGlyph = endGlyph;
        } else {
            ctx.tokenAccu.width = tokenWidth;
            ctx.tokenAccu.left = tokenLeft;
            ctx.tokenAccu.startGlyph = startGlyph;
            ctx.tokenAccu.endGlyph = endGlyph;
            ctx.tokenAccu.foreground = foreground;
            ctx.tokenAccu.background = background;
            ctx.tokenAccu.showSpecialGlyphs = showGlyphs;
        }
    }
}

void QSynEditPainter::paintFoldAttributes()
{
    // Paint indent guides. Use folds to determine indent value of these
    // Use a separate loop so we can use a custom pen
    // Paint indent guides using custom pen
    if (mEdit->mCodeFolding.indentGuides || mEdit->mCodeFolding.fillIndents) {
        QColor paintColor;
        if (mEdit->mCodeFolding.indentGuidesColor.isValid()) {
            paintColor = mEdit->mCodeFolding.indentGuidesColor;
        } else  {
            PTokenAttribute attr = mEdit->mSyntaxer->symbolAttribute();
            paintColor = attr->foreground();
            paintColor = mEdit->palette().color(QPalette::Text);
        }
        QColor gradientStart = paintColor;
        QColor gradientEnd = paintColor;
        QPen oldPen = mPainter->pen();

        // Now loop through all the lines. The indices are valid for Lines.
        for (int row = mFirstRow; row<=mLastRow;row++) {
            int line, segment;
            mEdit->rowToLineSegment(row, line, segment);
            if (line >= mDocument->count())
                break;
            // indent guides are segment-relative: resolve this row's own
            // segment start (0 without word wrap)
            int segmentStartX = mEdit->wrapLayout(line).segmentStartXs[segment];
            int x;
            // Set vertical coord
            int y = row * mEdit->mTextHeight - mEdit->mTopPos; // limit inside clip rect
            if (mEdit->mTextHeight % 2 == 1 && line % 2 == 0) {
                y++;
            }
            int lineIndent = mEdit->getLineIndent(mDocument->getLine(line));
            int indentLevel = 0 ;
            int tabSteps = 0;
            while (tabSteps < lineIndent) {
                x = tabSteps * mEdit->spaceWidth() + mEdit->textOffset() - segmentStartX - 1;
                tabSteps+=mEdit->tabSize();
                indentLevel++ ;
                if (mEdit->mCodeFolding.rainbowIndentGuides) {
                    PTokenAttribute attr = mEdit->mSyntaxer->symbolAttribute();
                    getRainbowColorAttr(indentLevel,attr);
                    paintColor = attr->foreground();
                }
                if (mEdit->mCodeFolding.rainbowIndents) {
                    PTokenAttribute attr = mEdit->mSyntaxer->symbolAttribute();
                    getRainbowColorAttr(indentLevel,attr);
                    gradientStart=attr->foreground();
                    attr = mEdit->mSyntaxer->symbolAttribute();
                    getBraceColorAttr(indentLevel+1,attr);
                    gradientStart=attr->foreground();
                }
                if (mEdit->mCodeFolding.fillIndents) {
                    int x1;
                    if (tabSteps>lineIndent)
                        x1 = lineIndent * mEdit->spaceWidth() + mEdit->textOffset() - segmentStartX - 1;
                    else
                        x1 = tabSteps * mEdit->spaceWidth() + mEdit->textOffset() - segmentStartX - 1;
                    gradientStart.setAlpha(20);
                    gradientEnd.setAlpha(10);
                    QLinearGradient gradient(x,y,x1,y);
                    gradient.setColorAt(1,gradientStart);
                    gradient.setColorAt(0,gradientEnd);
                    mPainter->fillRect(x,y,(x1-x),mEdit->mTextHeight,gradient);
                }

                // Move to top of vertical line
                if (mEdit->mCodeFolding.indentGuides) {
                    QPen dottedPen(Qt::PenStyle::DashLine);
                    dottedPen.setColor(paintColor);
                    mPainter->setPen(dottedPen);
                    mPainter->drawLine(x,y,x,y+mEdit->mTextHeight);
                }
            }
        }
        mPainter->setPen(oldPen);
    }

    if (!mEdit->useCodeFolding())
        return;

    // Paint collapsed lines using changed pen
    if (mEdit->mCodeFolding.showCollapsedLine) {
        mPainter->setPen(mEdit->mCodeFolding.collapsedLineColor);
        foreach(const PCodeBlock& block, mEdit->mCodeBlocks) {
            if (block->collapsed && !block->parentCollapsed() &&
                    (block->fromLine <= mLastLine) && (block->fromLine >= mFirstLine) ) {
                // Get starting and end points
                int y = (mEdit->lineToRow(block->fromLine) - mEdit->yposToRow(0) + 1) * mEdit->mTextHeight - 1;
                mPainter->drawLine(mClip.left(),y, mClip.right(),y);
            }
        }
    }

}

void QSynEditPainter::getRainbowColorAttr(int level, PTokenAttribute &attr)
{
    if (attr->tokenType() != TokenType::Symbol)
        return;
    PTokenAttribute oldAttr = attr;
    switch(level % 4) {
    case 0:
        attr = mEdit->mRainbowAttr0;
        break;
    case 1:
        attr = mEdit->mRainbowAttr1;
        break;
    case 2:
        attr = mEdit->mRainbowAttr2;
        break;
    case 3:
        attr = mEdit->mRainbowAttr3;
        break;
    }
    if (!attr)
        attr = oldAttr;
}

void QSynEditPainter::getBraceColorAttr(int level, PTokenAttribute &attr)
{
    if (!mEdit->mOptions.testFlag(EditorOption::ShowRainbowColor))
        return;
    return getRainbowColorAttr(level, attr);
}

void QSynEditPainter::paintLines()
{
    QString lineTextWithPreedit; // the current line
    QString token; // token info
    int tokenLeft, tokenWidth;
    PTokenAttribute attr;
    EditingAreaList  areaList;
    PCodeBlock foldRange;
//    PTokenAttribute preeditAttr;

    // Now loop through all the lines. The indices are valid for Lines.
    CharPos selectionBegin = mEdit->selBegin();
    CharPos selectionEnd= mEdit->selEnd();
    for (int row = mFirstRow; row<=mLastRow; row++) {
        RowPaintContext ctx;
        ctx.rcRow = mClip;
        int line, segment;
        mEdit->rowToLineSegment(row, line, segment);
        bool lineTextChanged = false;
        if (line >= mDocument->count() && mDocument->count() != 0)
            break;

        // Get the line.
        lineTextWithPreedit = mEdit->lineText(line);
        // determine whether will be painted with ActiveLineColor
        if (mEdit->mActiveSelectionMode == SelectionMode::Column) {
            ctx.isCurrentLine = (line >= selectionBegin.line && line <= selectionEnd.line);
        } else {
            ctx.isCurrentLine = (mEdit->mCaretLine == line);
        }
        if (ctx.isCurrentLine && !mEdit->mInputPreeditString.isEmpty()) {
            int ch = mDocument->charToGlyphStartChar(mEdit->mCaretLine,mEdit->mCaretCh);
            lineTextWithPreedit = lineTextWithPreedit.left(ch) + mEdit->mInputPreeditString
                    + lineTextWithPreedit.mid(ch);
            lineTextChanged = true;
        }
        // Initialize the text and background colors, maybe the line should
        // use special values for them.
        ctx.foreground = mEdit->mForegroundColor;
        ctx.background = lineBackgroundColor(ctx.isCurrentLine);
        ctx.specialLineForeground = QColor();
        ctx.specialLineBackground = QColor();
        ctx.isSpecialLine = mEdit->onGetSpecialLineColors(line, ctx.specialLineForeground, ctx.specialLineBackground);

        ctx.selectionForeground = mEdit->mSelectedForeground;
        ctx.selectionBackground = mEdit->mSelectedBackground;
        mEdit->onGetEditingAreas(line, areaList);
        // Get the information about the line selection. Three different parts
        // are possible (unselected before, selected, unselected after), only
        // unselected or only selected means bComplexLine will be FALSE. Start
        // with no selection, compute based on the visible columns.
        ctx.hasSelectionInRow = false;
        ctx.rowSelStart = 0;
        ctx.rowSelEnd = 0;
        ctx.isRowEndSelected = false;

        // Does the selection intersect the visible area?
        if (anySelection && (row >= mSelStart.row) && (row <= mSelEnd.row)) {
            // Default to a fully selected line. This is correct for the smLine
            // selection mode and a good start for the smNormal mode.
            ctx.hasSelectionInRow = true;
            ctx.rowSelStart = ctx.tokenClipLeft;
            ctx.rowSelEnd = ctx.tokenClipRight + 1;
            if ((mEdit->mActiveSelectionMode == SelectionMode::Column) ||
                    ((mEdit->mActiveSelectionMode == SelectionMode::Normal) && (row == mSelStart.row)) ) {
                // DisplayCoord.x is segment-relative under word wrap:
                // bring it into the absolute glyph-x frame of the clip
                // bounds (a no-op without word wrap)
                int xpos = mSelStart.x + ctx.segmentStartX;
                if (xpos > ctx.tokenClipRight) {
                    ctx.rowSelStart = 0;
                    ctx.rowSelEnd = 0;
                    ctx.hasSelectionInRow = false;
                } else if (xpos > ctx.tokenClipLeft) {
                    ctx.rowSelStart = xpos;
                }
            }
            if ( (mEdit->mActiveSelectionMode == SelectionMode::Column) ||
                 ((mEdit->mActiveSelectionMode == SelectionMode::Normal) && (row == mSelEnd.row)) ) {
                int xpos = mSelEnd.x + ctx.segmentStartX;
                if (xpos < ctx.tokenClipLeft) {
                    ctx.rowSelStart = 0;
                    ctx.rowSelEnd = 0;
                    ctx.hasSelectionInRow = false;
                } else if (xpos < ctx.tokenClipRight) {
                    ctx.rowSelEnd = xpos;
                }
            }
            if (mEdit->mActiveSelectionMode == SelectionMode::Normal) {
                ctx.isRowEndSelected = (row>=mSelStart.row && row < mSelEnd.row);
            }
        } //endif anySelection

        // Update the rcLine rect to this line.
//        rcLine.setTop(rcLine.bottom());
//        rcLine.setBottom(rcLine.bottom()+edit->mTextHeight);
        ctx.rcRow.setTop(row * mEdit->mTextHeight - mEdit->mTopPos);
        ctx.rcRow.setHeight(mEdit->mTextHeight);

        // if (ctx.isSpecialLine && colSpBG.isValid())
        //     colBG = colSpBG;
        // else
        //     colBG = colEditorBG(ctx.isCurrentLine);
        // setDrawingColors(selToEnd);
        // mPainter->fillRect(rcLine,mPainter->brush());

        ctx.rcToken = ctx.rcRow;

        QList<int> glyphStartCharList;
        if (lineTextChanged) {
            glyphStartCharList = mDocument->getGlyphStartCharList(line,lineTextWithPreedit);
        } else {
            glyphStartCharList = mDocument->getGlyphStartCharList(line);
        }
        // Glyph x positions come from the layout cache (wrapLayout computes
        // cold entries synchronously). They are recomputed only when the
        // painted text differs from the document line (IME preedit); those
        // positions describe the preedit text and must never be written
        // back into the cache.
        QList<int> glyphStartPositionsList;
        bool recomputeGlyphPositions = lineTextChanged;
        // token/selection clipping for this row: the intersection of the
        // visible region (mLeft..mRight, which carries the horizontal
        // scroll offset) and this wrap segment's bounds. The two are
        // orthogonal: without word wrap the single segment spans the whole
        // line and the visible region rules; with word wrap mLeftPos is 0
        // and the segment bounds are the tighter side
        const WrapLayout &wrap = mEdit->wrapLayout(line);
        if (recomputeGlyphPositions) {
            // same length, values recomputed while scanning tokens
            glyphStartPositionsList = glyphStartCharList;
        } else {
            glyphStartPositionsList = wrap.glyphStartXList;
            ctx.lineWidth = wrap.totalWidth;
        }
        ctx.segmentStartX = wrap.segmentStartXs[segment];
        if (segment+1 < wrap.segmentStartChars.count())
            ctx.segmentEndX = wrap.segmentStartXs[segment+1]-1;
        else
            ctx.segmentEndX = INT_MAX;
        ctx.tokenClipLeft = std::max(mLeft, ctx.segmentStartX);
        ctx.tokenClipRight = std::min(mRight, ctx.segmentEndX);
        bool isLastSegment = (segment+1 == wrap.segmentStartChars.count());
        // Initialize highlighter with line text and range info. It is
        // necessary because we probably did not scan to the end of the last
        // line - the internal highlighter range might be wrong.
        mEdit->startParseLine(mEdit->mSyntaxer.get(), line, lineTextWithPreedit);
        // Try to concatenate as many tokens as possible to minimize the count
        // of ExtTextOut calls necessary. This depends on the selection state
        // or the line having special colors. For spaces the foreground color
        // is ignored as well.
        ctx.tokenAccu.width = 0;
        tokenLeft = 0;
        // Test first whether anything of this token is visible.
        while (!mEdit->mSyntaxer->eol()) {
            token = mEdit->mSyntaxer->getToken();
            if (token.isEmpty())  {
                mEdit->mSyntaxer->next();
                continue;
            }
            int tokenStartChar = mEdit->mSyntaxer->getTokenPos();
            int tokenEndChar = tokenStartChar + token.length();

            // It's at least partially visible. Get the token attributes now.
            attr = mEdit->mSyntaxer->getTokenAttribute();

            //rainbow parenthesis
            if (token == "["
                    || token == "("
                    || token == "{"
                    ) {
                PSyntaxState rangeState = mEdit->mSyntaxer->getState();
                getBraceColorAttr(rangeState->bracketLevel
                                  +rangeState->braceLevel
                                  +rangeState->parenthesisLevel
                                  ,attr);
            } else if (token == "]"
                       || token == ")"
                       || token == "}"
                       ){
                PSyntaxState rangeState = mEdit->mSyntaxer->getState();
                getBraceColorAttr(rangeState->bracketLevel
                                  +rangeState->braceLevel
                                  +rangeState->parenthesisLevel+1,
                                  attr);
            }
            //input method
//            if (ctx.isCurrentLine && mEdit->mInputPreeditString.length()>0) {
//                int startPos = mEdit->mSyntaxer->getTokenPos();
//                int endPos = mEdit->mSyntaxer->getTokenPos() + token.length();
//                if (!(endPos < mEdit->mCaretCh
//                        || startPos >= mEdit->mCaretCh+mEdit->mInputPreeditString.length())) {
//                    if (!preeditAttr) {
//                        preeditAttr = attr;
//                    } else {
//                        attr = preeditAttr;
//                    }
//                }
//            }
            bool showGlyph=false;
            if (attr && attr->tokenType() == TokenType::Space) {
                int pos = mEdit->mSyntaxer->getTokenPos();
                if (pos==0) {
                    showGlyph = mEdit->mOptions.testFlag(EditorOption::ShowLeadingSpaces);
                } else if (pos+token.length()==lineTextWithPreedit.length()) {
                    showGlyph = mEdit->mOptions.testFlag(EditorOption::ShowTrailingSpaces);
                } else {
                    showGlyph = mEdit->mOptions.testFlag(EditorOption::ShowInnerSpaces);
                }
            }
            addHighlightToken(
                        ctx,
                        lineTextWithPreedit,
                        token,
                        tokenLeft,
                        line, attr,showGlyph,
                        glyphStartCharList,
                        tokenStartChar,
                        tokenEndChar,
                        recomputeGlyphPositions,
                        glyphStartPositionsList,
                        tokenWidth);
            tokenLeft+=tokenWidth;
            // glyph positions come from the cache for unchanged lines: stop
            // scanning once past the right clip edge. Preedit lines compute
            // positions as they go and must scan to the end (the input
            // method cache needs the whole line).
            if (!recomputeGlyphPositions && (tokenLeft>ctx.tokenClipRight))
                    break;
            // Let the highlighter scan the next token.
            mEdit->mSyntaxer->next();
        }
        // line-end decorations (fold string, line break glyph) belong to the
        // line's LAST wrap segment only
        if (isLastSegment && tokenLeft<ctx.tokenClipRight) {
            QString addOnStr;

            // Paint folding
            foldRange = mEdit->foldStartAtLine(line);
            if ((foldRange) && foldRange->collapsed) {
                addOnStr = mEdit->mSyntaxer->foldString(lineTextWithPreedit);
                attr = mEdit->mSyntaxer->symbolAttribute();
                getBraceColorAttr(mEdit->mSyntaxer->getState()->braceLevel,attr);
            } else {
                // Draw LineBreak glyph.
                if (mEdit->mOptions.testFlag(EditorOption::ShowLineBreaks)) {
                    addOnStr = LineBreakGlyph;
                    attr = mEdit->mSyntaxer->whitespaceAttribute();
                }
            }
            if (!addOnStr.isEmpty()) {
                expandGlyphStartCharList(addOnStr, lineTextWithPreedit.length(), glyphStartCharList);
                int len=glyphStartCharList.length()-glyphStartPositionsList.length();
                for (int i=0;i<len;i++) {
                    glyphStartPositionsList.append(tokenLeft);
                }
                int oldLen = lineTextWithPreedit.length();
                lineTextWithPreedit += addOnStr;
                addHighlightToken(
                            ctx,
                            lineTextWithPreedit,
                            addOnStr,
                            tokenLeft,
                            line, attr, false,
                            glyphStartCharList,
                            oldLen,
                            lineTextWithPreedit.length(),
                            true,
                            glyphStartPositionsList,
                            tokenWidth);
                tokenLeft += tokenWidth;
            }
        }
        // Draw anything that's left in the TokenAccu record. Fill to the end
        // of the invalid area with the correct colors.
        paintHighlightToken(ctx, lineTextWithPreedit, glyphStartCharList, glyphStartPositionsList, true);

        //Paint editingAreaBorders
        foreach (const PEditingArea& area, areaList) {
            if (ctx.isCurrentLine && mEdit->mInputPreeditString.length()>0) {
                if (area->beginX > mEdit->mCaretCh) {
                    area->beginX += mEdit->mInputPreeditString.length();
                }
                if (area->endX > mEdit->mCaretCh) {
                    area->endX += mEdit->mInputPreeditString.length();
                }
            }
            int glyphIdx;
            glyphIdx = searchForSegmentIdx(glyphStartCharList, lineTextWithPreedit.length(), area->beginX);
            area->beginX = segmentIntervalStart(glyphStartPositionsList, tokenLeft, glyphIdx);
            glyphIdx = searchForSegmentIdx(glyphStartCharList, lineTextWithPreedit.length(), area->endX);
            area->endX = segmentIntervalStart(glyphStartPositionsList, tokenLeft, glyphIdx);
        }
        //input method
        if (ctx.isCurrentLine && mEdit->mInputPreeditString.length()>0) {
            PEditingArea area = std::make_shared<EditingArea>();
            int glyphIdx;
            glyphIdx = searchForSegmentIdx(glyphStartCharList, lineTextWithPreedit.length(), mEdit->mCaretCh);
            area->beginX = segmentIntervalStart(glyphStartPositionsList, tokenLeft, glyphIdx);
            glyphIdx = searchForSegmentIdx(glyphStartCharList, lineTextWithPreedit.length(), mEdit->mCaretCh+mEdit->mInputPreeditString.length());
            area->endX = segmentIntervalStart(glyphStartPositionsList, tokenLeft, glyphIdx);
            area->type = EditingAreaType::eatUnderLine;
//            if (preeditAttr) {
//                area->color = preeditAttr->foreground();
//            } else {
                area->color = ctx.foreground;
//            }
            areaList.append(area);

            mEdit->mGlyphPostionCacheForInputMethod.str = lineTextWithPreedit;
            mEdit->mGlyphPostionCacheForInputMethod.glyphCharList = glyphStartCharList;
            mEdit->mGlyphPostionCacheForInputMethod.glyphPositionList = glyphStartPositionsList;
            mEdit->mGlyphPostionCacheForInputMethod.strWidth = tokenLeft;
        }
        paintEditAreas(ctx, areaList);

        // Now paint the right edge if necessary. We do it line by line to reduce
        // the flicker. Should not cost very much anyway, compared to the many
        // calls to ExtTextOut.
        if (doRightEdge) {
            mPainter->setPen(mEdit->mRightEdgeColor);
            mPainter->drawLine(rightEdge, ctx.rcRow.top(),rightEdge,ctx.rcRow.bottom()+1);
        }
        ctx.isCurrentLine = false;
    }
}
}
