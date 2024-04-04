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
#include "constants.h"
#include <cmath>
#include <QDebug>

namespace QSynedit {

QSynEditPainter::QSynEditPainter(QSynEdit *edit, QPainter *painter, int firstRow, int lastRow, int left, int right):
    mEdit{edit},
    mPainter{painter},
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
    mIsCurrentLine = false;
    // If the right edge is visible and in the invalid area, prepare to paint it.
    // Do this first to realize the pen when getting the dc variable.
    bDoRightEdge = false;
    if (mEdit->mRightEdge > 0) { // column value
        nRightEdge = mEdit->textOffset()+ mEdit->mRightEdge * mEdit->mCharWidth; // pixel value
        if (nRightEdge >= mClip.left() &&nRightEdge <= mClip.right()) {
            bDoRightEdge = true;
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
        mRcToken = mClip;
        mRcToken.setLeft( std::max(mClip.left(), mEdit->mGutterWidth));
        mRcToken.setRight(mEdit->mGutterWidth + 2);
        // Paint whole left edge of the text with same color.
        // (value of WhiteAttribute can vary in e.g. MultiSyn)
        mPainter->fillRect(mRcToken,colEditorBG());
        // Adjust the invalid area to not include this area.
        mClip.setLeft(mRcToken.right());
    }
    // If there is anything visible below the last line, then fill this as well.
    mRcToken = mClip;
    mRcToken.setTop((mLastRow - mEdit->yposToRow(0) + 1) * mEdit->mTextHeight);
    if (mRcToken.top() < mRcToken.bottom()) {
        mPainter->fillRect(mRcToken,colEditorBG());
        // Draw the right edge if necessary.
        if (bDoRightEdge) {
            QPen pen(mEdit->mRightEdgeColor,1);
            mPainter->setPen(pen);
            mPainter->drawLine(nRightEdge, mRcToken.top(),nRightEdge, mRcToken.bottom() + 1);
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
        BufferCoord selectionStart = mEdit->blockBegin();
        BufferCoord selectionEnd = mEdit->blockEnd();
        for (int row = mFirstRow; row <= mLastRow; row++) {
            int line = mEdit->rowToLine(row);
            if ((line > mEdit->mDocument->count()) && (mEdit->mDocument->count() > 0 ))
                break;
            if (mEdit->mGutter.activeLineTextColor().isValid()) {
                if (
                        (mEdit->mCaretY==line)     ||
                        (mEdit->mActiveSelectionMode == SelectionMode::Column && line >= selectionStart.line && line <= selectionEnd.line)
                        )
                    mPainter->setPen(mEdit->mGutter.activeLineTextColor());
                else
                    mPainter->setPen(textColor);
            } else {
                mPainter->setPen(textColor);
            }
            int lineTop = (row - 1) * mEdit->mTextHeight - mEdit->mTopPos;

            // next line rect
            rcLine.setTop(lineTop);
            rcLine.setBottom(rcLine.top() + mEdit->mTextHeight);

            QString s = mEdit->mGutter.formatLineNumber(line);

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
          int line = mEdit->rowToLine(row);
          if ((line > mEdit->mDocument->count()) && (mEdit->mDocument->count() != 0))
            continue;

        // Form a rectangle for the square the user can click on
          rcFold.setLeft(mEdit->mGutterWidth - mEdit->mGutter.rightOffset());
          rcFold.setTop((row - 1) * mEdit->mTextHeight - mEdit->mTopPos);
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
          PCodeFoldingRange foldRange = mEdit->foldStartAtLine(line);
        if (foldRange) {
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
        int line = mEdit->rowToLine(row);
        if ((line > mEdit->mDocument->count()) && (mEdit->mDocument->count() != 0))
            break;
        mEdit->onGutterPaint(*mPainter,line, 0, (row - mEdit->yposToRow(0)) * mEdit->mTextHeight);
    }
}

QColor QSynEditPainter::colEditorBG()
{
    if (mEdit->mActiveLineColor.isValid() && mIsCurrentLine) {
        return mEdit->mActiveLineColor;
    } else {
        return mEdit->mBackgroundColor;
    }
}

void QSynEditPainter::computeSelectionInfo()
{
    BufferCoord vStart{0,0};
    BufferCoord vEnd{0,0};
    bAnySelection = false;
    // Only if selection is visible anyway.
    bAnySelection = true;
    // Get the *real* start of the selected area.
    if (mEdit->mBlockBegin.line < mEdit->mBlockEnd.line) {
        vStart = mEdit->mBlockBegin;
        vEnd = mEdit->mBlockEnd;
    } else if (mEdit->mBlockBegin.line > mEdit->mBlockEnd.line) {
        vEnd = mEdit->mBlockBegin;
        vStart = mEdit->mBlockEnd;
    } else if (mEdit->mBlockBegin.ch != mEdit->mBlockEnd.ch) {
        // it is only on this line.
        vStart.line = mEdit->mBlockBegin.line;
        vEnd.line = vStart.line;
        if (mEdit->mBlockBegin.ch < mEdit->mBlockEnd.ch) {
            vStart.ch = mEdit->mBlockBegin.ch;
            vEnd.ch = mEdit->mBlockEnd.ch;
        } else {
            vStart.ch = mEdit->mBlockEnd.ch;
            vEnd.ch = mEdit->mBlockBegin.ch;
        }
    } else
        bAnySelection = false;
    if (mEdit->mInputPreeditString.length()>0) {
        if (vStart.line == mEdit->mCaretY && vStart.ch >=mEdit->mCaretX) {
            vStart.ch+=mEdit->mInputPreeditString.length();
        }
        if (vEnd.line == mEdit->mCaretY && vEnd.ch >mEdit->mCaretX) {
            vEnd.ch+=mEdit->mInputPreeditString.length();
        }
    }
    // If there is any visible selection so far, then test if there is an
    // intersection with the area to be painted.
    if (bAnySelection) {
        // Don't care if the selection is not visible.
        bAnySelection = (vEnd.line >= mFirstLine) && (vStart.line <= mLastLine);
        if (bAnySelection) {
            // Transform the selection from text space into screen space
            mSelStart = mEdit->bufferToDisplayPos(vStart);
            mSelEnd = mEdit->bufferToDisplayPos(vEnd);
            if (mEdit->mInputPreeditString.length()
                    && vStart.line == mEdit->mCaretY) {
                QString sLine = mEdit->lineText().left(mEdit->mCaretX-1)
                        + mEdit->mInputPreeditString
                        + mEdit->lineText().mid(mEdit->mCaretX-1);
                mSelStart.x = mEdit->charToGlyphLeft(mEdit->mCaretY, sLine,vStart.ch);
            }
            if (mEdit->mInputPreeditString.length()
                    && vEnd.line == mEdit->mCaretY) {
                QString sLine = mEdit->lineText().left(mEdit->mCaretX-1)
                        + mEdit->mInputPreeditString
                        + mEdit->lineText().mid(mEdit->mCaretX-1);
                mSelEnd.x = mEdit->charToGlyphLeft(mEdit->mCaretY, sLine,vEnd.ch);
            }
            // In the column selection mode sort the begin and end of the selection,
            // this makes the painting code simpler.
            if (mEdit->mActiveSelectionMode == SelectionMode::Column && mSelStart.x > mSelEnd.x)
                std::swap(mSelStart.x, mSelEnd.x);
        }
    }
}

void QSynEditPainter::getDrawingColors(bool selected, QColor &foreground, QColor &background)
{
    if (selected) {
        if (colSelFG.isValid())
            foreground = colSelFG;
        else
            foreground = colFG;
        if (colSelBG.isValid())
            background = colSelBG;
        else
            background = colBG;
    } else {
        foreground = colFG;
        background = colBG;
    }
}

int QSynEditPainter::fixXValue(int xpos)
{
    return mEdit->textOffset() + xpos;
}

void QSynEditPainter::paintLine()
{
    QRect rect = mRcLine;
    for (int i=0;i<mLineTokenBackgrounds.length();i++) {
        rect.setX(mLineTokenBackgrounds[i].left);
        rect.setWidth(mLineTokenBackgrounds[i].width);
        mPainter->fillRect(rect, mLineTokenBackgrounds[i].background);
    }

    QFont font;
    QFontMetrics fm{font};
    int lineHeight = mRcLine.height();
    for (int i=0;i<mLineTokens.length();i++) {
        if (font!=mLineTokens[i].font) {
            font = mLineTokens[i].font;
            fm = QFontMetrics{font};
        }
        int fontHeight = fm.descent() + fm.ascent();
        int linePadding = (lineHeight - fontHeight) / 2;
        int nY = mRcLine.bottom() - linePadding - fm.descent();
        if (font!=mPainter->font())
            mPainter->setFont(font);
        QPen pen(mLineTokens[i].foreground);
        if (pen!=mPainter->pen())
            mPainter->setPen(pen);
        mPainter->drawText(mLineTokens[i].left,nY, mLineTokens[i].token);
    }
    mLineTokens.clear();
    mLineTokenBackgrounds.clear();
}

void QSynEditPainter::paintToken(
        const QString& lineText,
        const QList<int> &glyphStartCharList,
        const QList<int> &glyphStartPositionList,
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

    if (last >= first && mRcToken.right() > mRcToken.left()) {
        int nX = fixXValue(first);
        first -= tokenLeft;
        last -= tokenLeft;
        QRect rcTokenBack = mRcToken;
        TokenBackgroundInfo backInfo;
        backInfo.left=rcTokenBack.left();
        backInfo.width=rcTokenBack.width();
        backInfo.background=background;
        mLineTokenBackgrounds.append(backInfo);
        //mPainter->fillRect(rcTokenBack,mPainter->brush());
        if (first > tokenWidth) {
        } else {
            int tokenWidth=0;
            startPaint = false;
            for (int i=startGlyph; i<endGlyph;i++) {
                int glyphStart = glyphStartCharList[i];
                int glyphLen = calcSegmentInterval(glyphStartCharList,lineText.length(),i);
                QString glyph = lineText.mid(glyphStart,glyphLen);
                int glyphWidth = calcSegmentInterval(glyphStartPositionList, tokenRight, i);
                if (tokenWidth+glyphWidth>first) {
                    if (!startPaint ) {
                        nX-= (first - tokenWidth - 1) ;
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
                            while(i+1<endGlyph && tokenWidth + glyphWidth < last) {
                                int glyphStart = glyphStartCharList[i+1];
                                int glyphLen = calcSegmentInterval(glyphStartCharList,lineText.length(),i+1);
                                QString glyph2 = lineText.mid(glyphStart,glyphLen);
                                // if (!OperatorGlyphs.contains(glyph))
                                //     break;
                                if ( glyph2.length()<1
                                     ||
                                     (glyph2.length()==1
                                      && glyph2.front().unicode()<=32))
                                    break;
                                int glyph2Width = calcSegmentInterval(glyphStartPositionList, tokenRight, i+1);
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
                            tokenInfo.left = nX;
                            tokenInfo.token = textToPaint;
                            tokenInfo.foreground=foreground;
                            tokenInfo.font = font;
                            mLineTokens.append(tokenInfo);
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
                                tokenInfo.left = nX+padding;
                                tokenInfo.token = textToPaint;
                                tokenInfo.foreground=foreground;
                                tokenInfo.font = font;
                                mLineTokens.append(tokenInfo);
                            }
                        }
                        drawed = true;
                    }
                    nX += glyphWidth;
                }
                tokenWidth += glyphWidth;
                if (tokenWidth >= last)
                    break;
            }
        }

        mRcToken.setLeft(mRcToken.right()+1);
    }
}

void QSynEditPainter::paintEditAreas(const EditingAreaList &areaList)
{
    QRect rc;
    int x1,x2;
    rc=mRcLine;
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
        rc.setLeft(fixXValue(x1));
        rc.setRight(fixXValue(x2));
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

void QSynEditPainter::paintHighlightToken(const QString& lineText,
                                          const QList<int> &glyphStartCharList,
                                          const QList<int> &glyphStartPositionsList,
                                          bool bFillToEOL)
{
    bool isComplexToken;
    int nC1, nC2, nC1Sel, nC2Sel;
    bool bU1, bSel, bU2;
    // Compute some helper variables.
    nC1 = std::max(mLeft, mTokenAccu.left);
    nC2 = std::min(mRight, mTokenAccu.left + mTokenAccu.width);
    if (mHasSelectionInLine) {
        bU1 = (nC1 < mLineSelStart);
        bSel = (nC1 < mLineSelEnd) && (nC2 >= mLineSelStart);
        bU2 = (nC2 >= mLineSelEnd);
        isComplexToken = bSel && (bU1 || bU2);
    } else {
        bSel = false;
        isComplexToken = false;
        bU1 = false;
        bU2 = false;
    }
    // Any token chars accumulated?
    if (mTokenAccu.width > 0) {
        // Initialize the colors and the font style.
        colBG = mTokenAccu.background;
        colFG = mTokenAccu.foreground;
        if (mIsSpecialLine) {
            if (colSpFG.isValid())
                colFG = colSpFG;
            if (colSpBG.isValid())
                colBG = colSpBG;
        }

        //        if (bSpecialLine && mEdit->mOptions.testFlag(eoSpecialLineDefaultFg))
//            colFG = TokenAccu.FG;


        // Paint the chars
        if (isComplexToken) {
            // first unselected part of the token
            if (bU1) {
                QColor foreground,background;
                getDrawingColors(false,foreground,background);
                mRcToken.setRight(fixXValue(mLineSelStart));
                paintToken(
                            lineText,
                            glyphStartCharList,
                            glyphStartPositionsList,
                            mTokenAccu.startGlyph,
                            mTokenAccu.endGlyph,
                            mTokenAccu.width,mTokenAccu.left,nC1,mLineSelStart,
                            foreground,background,
                            mTokenAccu.font, mTokenAccu.showSpecialGlyphs);
            }
            // selected part of the token
            QColor foreground,background;
            getDrawingColors(true,foreground,background);
            nC1Sel = std::max(mLineSelStart, nC1);
            nC2Sel = std::min(mLineSelEnd, nC2);
            mRcToken.setRight(fixXValue(nC2Sel));
            paintToken(
                        lineText,
                        glyphStartCharList,
                        glyphStartPositionsList,
                        mTokenAccu.startGlyph,
                        mTokenAccu.endGlyph,
                        mTokenAccu.width, mTokenAccu.left, nC1Sel, nC2Sel,
                        foreground,background,
                        mTokenAccu.font, mTokenAccu.showSpecialGlyphs);
            // second unselected part of the token
            if (bU2) {
                QColor foreground,background;
                getDrawingColors(false,foreground,background);
                mRcToken.setRight(fixXValue(nC2));
                paintToken(
                            lineText,
                            glyphStartCharList,
                            glyphStartPositionsList,
                            mTokenAccu.startGlyph,
                            mTokenAccu.endGlyph,
                            mTokenAccu.width, mTokenAccu.left, mLineSelEnd, nC2,
                            foreground,background,
                            mTokenAccu.font, mTokenAccu.showSpecialGlyphs);
            }
        } else {
            QColor foreground,background;
            getDrawingColors(bSel,foreground,background);
            mRcToken.setRight(fixXValue(nC2));
            paintToken(
                        lineText,
                        glyphStartCharList,
                        glyphStartPositionsList,
                        mTokenAccu.startGlyph,
                        mTokenAccu.endGlyph,
                        mTokenAccu.width, mTokenAccu.left, nC1, nC2,
                        foreground,background,
                        mTokenAccu.font, mTokenAccu.showSpecialGlyphs);
        }
    }

    // Fill the background to the end of this line if necessary.
    if (bFillToEOL && mRcToken.left() < mRcLine.right()) {
        if (mIsSpecialLine && colSpBG.isValid())
            colBG = colSpBG;
        else
            colBG = colEditorBG();
        QColor foreground,background;
        if (mHasSelectionInLine) {
            getDrawingColors(mIsLineEndSelected,foreground,background);
            mRcToken.setRight(mRcLine.right());
            //mPainter->fillRect(mRcToken,mPainter->brush());
        }  else {
            getDrawingColors(false,foreground,background);
            mRcToken.setRight(mRcLine.right());
            //mPainter->fillRect(mRcToken,mPainter->brush());
        }
        TokenBackgroundInfo backInfo;
        backInfo.left=mRcToken.left();
        backInfo.width=mRcToken.width();
        backInfo.background=background;
        mLineTokenBackgrounds.append(backInfo);
    }
    if (bFillToEOL) {
        paintLine();
    }
}

// Store the token chars with the attributes in the TokenAccu
// record. This will paint any chars already stored if there is
// a (visible) change in the attributes.
void QSynEditPainter::addHighlightToken(
        const QString& lineText,
        const QString& token, int tokenLeft,
        int line, PTokenAttribute attri, bool showGlyphs,
        const QList<int> glyphStartCharList,
        int tokenStartChar,
        int tokenEndChar,
        bool calcGlyphPosition,
        QList<int> &glyphStartPositionList,
        int &tokenWidth)
{
    int tokenRight;
    int startGlyph, endGlyph;
    if (!calcGlyphPosition) {
        tokenRight = std::max(0,tokenLeft);
        startGlyph = searchForSegmentIdx(glyphStartCharList,0,lineText.length(),tokenStartChar);
        endGlyph = searchForSegmentIdx(glyphStartCharList,0,lineText.length(),tokenEndChar);
        for (int i=startGlyph;i<endGlyph;i++) {
            int gWidth = calcSegmentInterval(glyphStartPositionList, mCurrentLineWidth, i);
            tokenRight += gWidth;
        }
        tokenWidth = tokenRight-tokenLeft;
        if (tokenRight<mLeft) {
            return;
        }
    }
    QColor foreground, background;
    FontStyles style;

    if (attri) {
        foreground = attri->foreground();
        background = attri->background();
        style = attri->styles();
    } else {
        foreground = colFG;
        background = colBG;
        style = getFontStyles(mEdit->font());
    }

//    if (!Background.isValid() || (edit->mActiveLineColor.isValid() && bCurrentLine)) {
//        Background = colEditorBG();
//    }

    mEdit->onPreparePaintHighlightToken(line,mEdit->mSyntaxer->getTokenPos()+1,
        token,attri,style,foreground,background);

    if (!background.isValid() ) {
        background = colEditorBG();
    }
    if (!foreground.isValid()) {
        foreground = mEdit->mForegroundColor;
    }

    // Do we have to paint the old chars first, or can we just append?
    bool bCanAppend = false;
    bool bInitFont = (mTokenAccu.width==0);
    if (mTokenAccu.width > 0 ) {
        // font style must be the same or token is only spaces
        if (mTokenAccu.style != style) {
            bInitFont = true;
        } else {
            if (
                (showGlyphs == mTokenAccu.showSpecialGlyphs) &&
              // background color must be the same and
                (mTokenAccu.background == background) &&
              // foreground color must be the same or token is only spaces
              (mTokenAccu.foreground == foreground)) {
                bCanAppend = true;
            }
        }
        // If we can't append it, then we have to paint the old token chars first.
        if (!bCanAppend)
            paintHighlightToken(lineText, glyphStartCharList, glyphStartPositionList, false);
    }
    if (bInitFont) {
        mTokenAccu.style = style;
        mTokenAccu.font = mEdit->font();
        mTokenAccu.font.setBold(style & FontStyle::fsBold);
        mTokenAccu.font.setItalic(style & FontStyle::fsItalic);
        mTokenAccu.font.setStrikeOut(style & FontStyle::fsStrikeOut);
        mTokenAccu.font.setUnderline(style & FontStyle::fsUnderline);
    }
    //calculate width of the token ( and update it's glyph start positions )
    if (calcGlyphPosition) {
        tokenWidth = mEdit->mDocument->updateGlyphStartPositionList(
                    lineText,
                    glyphStartCharList,
                    tokenStartChar,
                    tokenEndChar,
                    QFontMetrics(mTokenAccu.font),
                    glyphStartPositionList,
                    tokenLeft,
                    tokenRight,
                    startGlyph,
                    endGlyph);
    }

    // Only accumulate tokens if it's visible.
    if (tokenLeft < mRight && tokenRight>mLeft) {
        if (bCanAppend) {
            mTokenAccu.width += tokenWidth;
            Q_ASSERT(startGlyph == mTokenAccu.endGlyph);
            mTokenAccu.endGlyph = endGlyph;
        } else {
            mTokenAccu.width = tokenWidth;
            mTokenAccu.left = tokenLeft;
            mTokenAccu.startGlyph = startGlyph;
            mTokenAccu.endGlyph = endGlyph;
            mTokenAccu.foreground = foreground;
            mTokenAccu.background = background;
            mTokenAccu.showSpecialGlyphs = showGlyphs;
        }
    }
}

void QSynEditPainter::paintFoldAttributes()
{
    int tabSteps, lineIndent, lastNonBlank;
    // Paint indent guides. Use folds to determine indent value of these
    // Use a separate loop so we can use a custom pen
    // Paint indent guides using custom pen
    if (mEdit->mCodeFolding.indentGuides || mEdit->mCodeFolding.fillIndents) {
        QColor paintColor;
        if (mEdit->mCodeFolding.indentGuidesColor.isValid()) {
            paintColor = mEdit->mCodeFolding.indentGuidesColor;
        } else  {
            paintColor = mEdit->palette().color(QPalette::Text);
        }
        QColor gradientStart = paintColor;
        QColor gradientEnd = paintColor;
        QPen oldPen = mPainter->pen();

        // Now loop through all the lines. The indices are valid for Lines.
        for (int row = mFirstRow; row<=mLastRow;row++) {
            int vLine = mEdit->rowToLine(row);
            if (vLine > mEdit->mDocument->count() && mEdit->mDocument->count() > 0)
                break;
            int X;
            // Set vertical coord
            int Y = (row-1) * mEdit->mTextHeight - mEdit->mTopPos; // limit inside clip rect
            if (mEdit->mTextHeight % 2 == 1 && vLine % 2 == 0) {
                Y++;
            }
            // Get next nonblank line
            lastNonBlank = vLine - 1;
            while (lastNonBlank + 1 < mEdit->mDocument->count() && mEdit->mDocument->getLine(lastNonBlank).isEmpty())
                lastNonBlank++;
            if (lastNonBlank>=mEdit->document()->count())
                continue;
            lineIndent = mEdit->getLineIndent(mEdit->mDocument->getLine(lastNonBlank));
            int braceLevel = mEdit->mDocument->getSyntaxState(lastNonBlank).braceLevel;
            int indentLevel = braceLevel ;
            tabSteps = 0;
            indentLevel = 0;
            while (tabSteps < lineIndent) {
                X = tabSteps * mEdit->mDocument->spaceWidth() + mEdit->textOffset() - 1;
                tabSteps+=mEdit->tabSize();
                indentLevel++ ;
                if (mEdit->mCodeFolding.indentGuides) {
                    PTokenAttribute attr = mEdit->mSyntaxer->symbolAttribute();
                    getBraceColorAttr(indentLevel,attr);
                    paintColor = attr->foreground();
                }
                if (mEdit->mCodeFolding.fillIndents) {
                    PTokenAttribute attr = mEdit->mSyntaxer->symbolAttribute();
                    getBraceColorAttr(indentLevel,attr);
                    gradientStart=attr->foreground();
                    attr = mEdit->mSyntaxer->symbolAttribute();
                    getBraceColorAttr(indentLevel+1,attr);
                    gradientStart=attr->foreground();
                }
                if (mEdit->mCodeFolding.fillIndents) {
                    int X1;
                    if (tabSteps>lineIndent)
                        X1 = lineIndent * mEdit->mDocument->spaceWidth() + mEdit->textOffset() - 1;
                    else
                        X1 = tabSteps * mEdit->mDocument->spaceWidth() + mEdit->textOffset() - 1;
                    gradientStart.setAlpha(20);
                    gradientEnd.setAlpha(10);
                    QLinearGradient gradient(X,Y,X1,Y);
                    gradient.setColorAt(1,gradientStart);
                    gradient.setColorAt(0,gradientEnd);
                    mPainter->fillRect(X,Y,(X1-X),mEdit->mTextHeight,gradient);
                }

                // Move to top of vertical line
                if (mEdit->mCodeFolding.indentGuides) {
                    QPen dottedPen(Qt::PenStyle::DashLine);
                    dottedPen.setColor(paintColor);
                    mPainter->setPen(dottedPen);
                    mPainter->drawLine(X,Y,X,Y+mEdit->mTextHeight);
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
        for (int i=0; i< mEdit->mAllFoldRanges->count();i++) {
            PCodeFoldingRange range = (*mEdit->mAllFoldRanges)[i];
            if (range->collapsed && !range->parentCollapsed() &&
                    (range->fromLine <= mLastLine) && (range->fromLine >= mFirstLine) ) {
                // Get starting and end points
                int Y = (mEdit->lineToRow(range->fromLine) - mEdit->yposToRow(0) + 1) * mEdit->mTextHeight - 1;
                mPainter->drawLine(mClip.left(),Y, mClip.right(),Y);
            }
        }
    }

}

void QSynEditPainter::getBraceColorAttr(int level, PTokenAttribute &attr)
{
    if (!mEdit->mOptions.testFlag(EditorOption::ShowRainbowColor))
        return;
    if (attr->tokenType() != TokenType::Operator)
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

void QSynEditPainter::paintLines()
{
    mEdit->mDocument->beginSetLinesWidth();
    auto action = finally([this](){
        mEdit->mDocument->endSetLinesWidth();
    });
    QString sLine; // the current line
    QString sToken; // token info
    int tokenLeft, tokenWidth;
    PTokenAttribute attr;
    EditingAreaList  areaList;
    PCodeFoldingRange foldRange;
    PTokenAttribute preeditAttr;

    // Initialize rcLine for drawing. Note that Top and Bottom are updated
    // inside the loop. Get only the starting point for this.
    mRcLine = mClip;
    mRcLine.setBottom((mFirstRow - 1) * mEdit->mTextHeight - mEdit->mTopPos);
    mTokenAccu.width = 0;
    mTokenAccu.left = 0;
    mTokenAccu.style = FontStyle::fsNone;
    // Now loop through all the lines. The indices are valid for Lines.
    BufferCoord selectionBegin = mEdit->blockBegin();
    BufferCoord selectionEnd= mEdit->blockEnd();
    for (int row = mFirstRow; row<=mLastRow; row++) {
        int vLine = mEdit->rowToLine(row);
        bool lineTextChanged = false;
        if (vLine > mEdit->mDocument->count() && mEdit->mDocument->count() != 0)
            break;

        // Get the line.
        sLine = mEdit->lineText(vLine);
        // determine whether will be painted with ActiveLineColor
        if (mEdit->mActiveSelectionMode == SelectionMode::Column) {
            mIsCurrentLine = (vLine >= selectionBegin.line && vLine <= selectionEnd.line);
        } else {
            mIsCurrentLine = (mEdit->mCaretY == vLine);
        }
        if (mIsCurrentLine && !mEdit->mInputPreeditString.isEmpty()) {
            int ch = mEdit->mDocument->charToGlyphStartChar(mEdit->mCaretY-1,mEdit->mCaretX-1);
            sLine = sLine.left(ch) + mEdit->mInputPreeditString
                    + sLine.mid(ch);
            lineTextChanged = true;
        }
        // Initialize the text and background colors, maybe the line should
        // use special values for them.
        colFG = mEdit->mForegroundColor;
        colBG = colEditorBG();
        colSpFG = QColor();
        colSpBG = QColor();
        mIsSpecialLine = mEdit->onGetSpecialLineColors(vLine, colSpFG, colSpBG);

        colSelFG = mEdit->mSelectedForeground;
        colSelBG = mEdit->mSelectedBackground;
        mEdit->onGetEditingAreas(vLine, areaList);
        // Get the information about the line selection. Three different parts
        // are possible (unselected before, selected, unselected after), only
        // unselected or only selected means bComplexLine will be FALSE. Start
        // with no selection, compute based on the visible columns.
        mHasSelectionInLine = false;
        mLineSelStart = 0;
        mLineSelEnd = 0;
        mIsLineEndSelected = false;

        // Does the selection intersect the visible area?
        if (bAnySelection && (row >= mSelStart.row) && (row <= mSelEnd.row)) {
            // Default to a fully selected line. This is correct for the smLine
            // selection mode and a good start for the smNormal mode.
            mHasSelectionInLine = true;
            mLineSelStart = mLeft;
            mLineSelEnd = mRight + 1;
            if ((mEdit->mActiveSelectionMode == SelectionMode::Column) ||
                    ((mEdit->mActiveSelectionMode == SelectionMode::Normal) && (row == mSelStart.row)) ) {
                int xpos = mSelStart.x;
                if (xpos > mRight) {
                    mLineSelStart = 0;
                    mLineSelEnd = 0;
                    mHasSelectionInLine = false;
                } else if (xpos > mLeft) {
                    mLineSelStart = xpos;
                }
            }
            if ( (mEdit->mActiveSelectionMode == SelectionMode::Column) ||
                 ((mEdit->mActiveSelectionMode == SelectionMode::Normal) && (row == mSelEnd.row)) ) {
                int xpos = mSelEnd.x;
                if (xpos < mLeft) {
                    mLineSelStart = 0;
                    mLineSelEnd = 0;
                    mHasSelectionInLine = false;
                } else if (xpos < mRight) {
                    mLineSelEnd = xpos;
                }
            }
            if (mEdit->mActiveSelectionMode == SelectionMode::Normal) {
                mIsLineEndSelected = (row>=mSelStart.row && row < mSelEnd.row);
            }
        } //endif bAnySelection

        // Update the rcLine rect to this line.
//        rcLine.setTop(rcLine.bottom());
//        rcLine.setBottom(rcLine.bottom()+edit->mTextHeight);
        mRcLine.setTop((row - 1) * mEdit->mTextHeight - mEdit->mTopPos);
        mRcLine.setHeight(mEdit->mTextHeight);

        // if (mIsSpecialLine && colSpBG.isValid())
        //     colBG = colSpBG;
        // else
        //     colBG = colEditorBG();
        // setDrawingColors(selToEnd);
        // mPainter->fillRect(rcLine,mPainter->brush());

        mRcToken = mRcLine;

        QList<int> glyphStartCharList;
        if (lineTextChanged) {
            glyphStartCharList = mEdit->mDocument->getGlyphStartCharList(vLine-1,sLine);
        } else {
            glyphStartCharList = mEdit->mDocument->getGlyphStartCharList(vLine-1);
        }
        // Ensure the list has the right number of elements.
        // Values in it doesn't matter, we'll recalculate them.
        QList<int> glyphStartPositionsList;
        bool lineWidthValid = mEdit->mDocument->lineWidthValid(vLine-1);
        bool calculateGlyphPositions = ( mHasSelectionInLine ||  lineTextChanged || !lineWidthValid);
        if (calculateGlyphPositions) {
            glyphStartPositionsList = glyphStartCharList;
        } else {
            glyphStartPositionsList = mEdit->mDocument->getGlyphStartPositionList(vLine-1);
            mCurrentLineWidth = mEdit->mDocument->getLineWidth(vLine-1);
        }
        // Initialize highlighter with line text and range info. It is
        // necessary because we probably did not scan to the end of the last
        // line - the internal highlighter range might be wrong.
        if (vLine == 1) {
            mEdit->mSyntaxer->resetState();
        } else {
            mEdit->mSyntaxer->setState(
                        mEdit->mDocument->getSyntaxState(vLine-2));
        }
        mEdit->mSyntaxer->setLine(sLine, vLine - 1);
        // Try to concatenate as many tokens as possible to minimize the count
        // of ExtTextOut calls necessary. This depends on the selection state
        // or the line having special colors. For spaces the foreground color
        // is ignored as well.
        mTokenAccu.width = 0;
        tokenLeft = 0;
        // Test first whether anything of this token is visible.
        while (!mEdit->mSyntaxer->eol()) {
            sToken = mEdit->mSyntaxer->getToken();
            if (sToken.isEmpty())  {
                continue;
            }
            int tokenStartChar = mEdit->mSyntaxer->getTokenPos();
            int tokenEndChar = tokenStartChar + sToken.length();

            // It's at least partially visible. Get the token attributes now.
            attr = mEdit->mSyntaxer->getTokenAttribute();

            //rainbow parenthesis
            if (sToken == "["
                    || sToken == "("
                    || sToken == "{"
                    ) {
                SyntaxState rangeState = mEdit->mSyntaxer->getState();
                getBraceColorAttr(rangeState.bracketLevel
                                  +rangeState.braceLevel
                                  +rangeState.parenthesisLevel
                                  ,attr);
            } else if (sToken == "]"
                       || sToken == ")"
                       || sToken == "}"
                       ){
                SyntaxState rangeState = mEdit->mSyntaxer->getState();
                getBraceColorAttr(rangeState.bracketLevel
                                  +rangeState.braceLevel
                                  +rangeState.parenthesisLevel+1,
                                  attr);
            }
            //input method
            if (mIsCurrentLine && mEdit->mInputPreeditString.length()>0) {
                int startPos = mEdit->mSyntaxer->getTokenPos()+1;
                int endPos = mEdit->mSyntaxer->getTokenPos() + sToken.length();
                //qDebug()<<startPos<<":"<<endPos<<" - "+sToken+" - "<<edit->mCaretX<<":"<<edit->mCaretX+edit->mInputPreeditString.length();
                if (!(endPos < mEdit->mCaretX
                        || startPos >= mEdit->mCaretX+mEdit->mInputPreeditString.length())) {
                    if (!preeditAttr) {
                        preeditAttr = attr;
                    } else {
                        attr = preeditAttr;
                    }
                }
            }
            bool showGlyph=false;
            if (attr && attr->tokenType() == TokenType::Space) {
                int pos = mEdit->mSyntaxer->getTokenPos();
                if (pos==0) {
                    showGlyph = mEdit->mOptions.testFlag(EditorOption::ShowLeadingSpaces);
                } else if (pos+sToken.length()==sLine.length()) {
                    showGlyph = mEdit->mOptions.testFlag(EditorOption::ShowTrailingSpaces);
                } else {
                    showGlyph = mEdit->mOptions.testFlag(EditorOption::ShowInnerSpaces);
                }
            }
            addHighlightToken(
                        sLine,
                        sToken,
                        tokenLeft,
                        vLine, attr,showGlyph,
                        glyphStartCharList,
                        tokenStartChar,
                        tokenEndChar,
                        calculateGlyphPositions,
                        glyphStartPositionsList,
                        tokenWidth);
            tokenLeft+=tokenWidth;
            //We don't need to calculate line width,
            //So we just quit if already out of the right edge of the editor
            if (
                    (!calculateGlyphPositions || lineTextChanged)
                    && (tokenLeft>mRight))
                    break;
            // Let the highlighter scan the next token.
            mEdit->mSyntaxer->next();
        }
        if (!lineWidthValid)
            mEdit->mDocument->setLineWidth(vLine-1, tokenLeft, glyphStartPositionsList);
        if (tokenLeft<mRight) {
            QString addOnStr;

            // Paint folding
            foldRange = mEdit->foldStartAtLine(vLine);
            if ((foldRange) && foldRange->collapsed) {
                addOnStr = mEdit->mSyntaxer->foldString(sLine);
                attr = mEdit->mSyntaxer->symbolAttribute();
                getBraceColorAttr(mEdit->mSyntaxer->getState().braceLevel,attr);
            } else {
                // Draw LineBreak glyph.
                if (mEdit->mOptions.testFlag(EditorOption::ShowLineBreaks)
                        && (mEdit->mDocument->lineWidth(vLine-1) < mRight)) {
                    addOnStr = LineBreakGlyph;
                    attr = mEdit->mSyntaxer->whitespaceAttribute();
                }
            }
            if (!addOnStr.isEmpty()) {
                expandGlyphStartCharList(addOnStr, sLine.length(), glyphStartCharList);
                int len=glyphStartCharList.length()-glyphStartPositionsList.length();
                for (int i=0;i<len;i++) {
                    glyphStartPositionsList.append(tokenLeft);
                }
                int oldLen = sLine.length();
                sLine += addOnStr;
                addHighlightToken(
                            sLine,
                            addOnStr,
                            tokenLeft,
                            vLine, attr, false,
                            glyphStartCharList,
                            oldLen,
                            sLine.length(),
                            calculateGlyphPositions,
                            glyphStartPositionsList,
                            tokenWidth);
                tokenLeft += tokenWidth;
            }
        }
        // Draw anything that's left in the TokenAccu record. Fill to the end
        // of the invalid area with the correct colors.
        paintHighlightToken(sLine, glyphStartCharList, glyphStartPositionsList, true);

        //Paint editingAreaBorders
        foreach (const PEditingArea& area, areaList) {
            if (mIsCurrentLine && mEdit->mInputPreeditString.length()>0) {
                if (area->beginX > mEdit->mCaretX) {
                    area->beginX += mEdit->mInputPreeditString.length();
                }
                if (area->endX > mEdit->mCaretX) {
                    area->endX += mEdit->mInputPreeditString.length();
                }
            }
            int glyphIdx;
            glyphIdx = searchForSegmentIdx(glyphStartCharList, 0, sLine.length(), area->beginX-1);
            area->beginX = segmentIntervalStart(glyphStartPositionsList, 0, tokenLeft, glyphIdx);
            glyphIdx = searchForSegmentIdx(glyphStartCharList, 0, sLine.length(), area->endX-1);
            area->endX = segmentIntervalStart(glyphStartPositionsList, 0, tokenLeft, glyphIdx);
        }
        //input method
        if (mIsCurrentLine && mEdit->mInputPreeditString.length()>0) {
            PEditingArea area = std::make_shared<EditingArea>();
            int glyphIdx;
            glyphIdx = searchForSegmentIdx(glyphStartCharList, 0, sLine.length(), mEdit->mCaretX-1);
            area->beginX = segmentIntervalStart(glyphStartPositionsList, 0, tokenLeft, glyphIdx);
            glyphIdx = searchForSegmentIdx(glyphStartCharList, 0, sLine.length(), mEdit->mCaretX+mEdit->mInputPreeditString.length()-1);
            area->endX = segmentIntervalStart(glyphStartPositionsList, 0, tokenLeft, glyphIdx);
            area->type = EditingAreaType::eatUnderLine;
            if (preeditAttr) {
                area->color = preeditAttr->foreground();
            } else {
                area->color = colFG;
            }
            areaList.append(area);

            mEdit->mGlyphPostionCacheForInputMethod.str = sLine;
            mEdit->mGlyphPostionCacheForInputMethod.glyphCharList = glyphStartCharList;
            mEdit->mGlyphPostionCacheForInputMethod.glyphPositionList = glyphStartPositionsList;
            mEdit->mGlyphPostionCacheForInputMethod.strWidth = tokenLeft;
        }
        paintEditAreas(areaList);

        // Now paint the right edge if necessary. We do it line by line to reduce
        // the flicker. Should not cost very much anyway, compared to the many
        // calls to ExtTextOut.
        if (bDoRightEdge) {
            mPainter->setPen(mEdit->mRightEdgeColor);
            mPainter->drawLine(nRightEdge, mRcLine.top(),nRightEdge,mRcLine.bottom()+1);
        }
        mIsCurrentLine = false;
    }
}
}
