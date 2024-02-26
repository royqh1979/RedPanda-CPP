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

QSet<QString> QSynEditPainter::OperatorGlyphs {
    "-",
    "+",
    "*",
    "/",
    "\\",
    "~",
    "!",
    "@",
    "#",
    "$",
    "%",
    "^",
    "&",
    "|",
    "=",
    "<",
    ">",
    "?",
    ":",
};


QSynEditPainter::QSynEditPainter(QSynEdit *edit, QPainter *painter, int firstRow, int lastRow, int left, int right):
    mEdit{edit},
    mPainter{painter},
    mFirstRow{firstRow},
    mLastRow{lastRow},
    mLeft{left},
    mRight{right}
{
}

void QSynEditPainter::paintTextLines(const QRect& clip)
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
            QPen pen(mEdit->mRightEdgeColor,1);
            mPainter->setPen(pen);
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
        rcToken = mClip;
        rcToken.setLeft( std::max(mClip.left(), mEdit->mGutterWidth));
        rcToken.setRight(mEdit->mGutterWidth + 2);
        // Paint whole left edge of the text with same color.
        // (value of WhiteAttribute can vary in e.g. MultiSyn)
        mPainter->fillRect(rcToken,colEditorBG());
        // Adjust the invalid area to not include this area.
        mClip.setLeft(rcToken.right());
    }
    // If there is anything visible below the last line, then fill this as well.
    rcToken = mClip;
    rcToken.setTop((mLastRow - mEdit->mTopLine + 1) * mEdit->mTextHeight);
    if (rcToken.top() < rcToken.bottom()) {
        mPainter->fillRect(rcToken,colEditorBG());
        // Draw the right edge if necessary.
        if (bDoRightEdge) {
            QPen pen(mEdit->mRightEdgeColor,1);
            mPainter->setPen(pen);
            mPainter->drawLine(nRightEdge, rcToken.top(),nRightEdge, rcToken.bottom() + 1);
        }
    }

    // This messes with pen colors, so draw after right margin has been drawn
    paintFoldAttributes();
}

void QSynEditPainter::paintGutter(const QRect& clip)
{
    QRect rcLine, rcFold;
    int x;

    mClip = clip;

    mPainter->fillRect(mClip,mEdit->mGutter.color());

    rcLine=mClip;
    if (mEdit->mGutter.showLineNumbers()) {
        // prepare the rect initially
        rcLine = mClip;
        rcLine.setRight( std::max(rcLine.right(), mEdit->mGutterWidth - 2));
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
            int lineTop = (row - mEdit->mTopLine) * mEdit->mTextHeight;

            // next line rect
            rcLine.setTop(lineTop);
            rcLine.setBottom(rcLine.top() + mEdit->mTextHeight);

            QString s = mEdit->mGutter.formatLineNumber(line);

            mEdit->onGutterGetText(line,s);
            QRectF textRect;
            textRect = mPainter->boundingRect(textRect, Qt::AlignLeft,s);
            mPainter->drawText(
                        (mEdit->mGutterWidth - mEdit->mGutter.rightOffset() - 2) - textRect.width(),
                        rcLine.bottom() + ((mEdit->mTextHeight - int(textRect.height())) / 2 - mPainter->fontMetrics().descent()),
                        s
                        );
        }
    }

    // Draw the folding lines and squares
    if (mEdit->mUseCodeFolding) {
      for (int row = mLastRow; row>= mFirstRow; row--) {
          int line = mEdit->rowToLine(row);
          if ((line > mEdit->mDocument->count()) && (mEdit->mDocument->count() != 0))
            continue;

        // Form a rectangle for the square the user can click on
          rcFold.setLeft(mEdit->mGutterWidth - mEdit->mGutter.rightOffset());
          rcFold.setTop((row - mEdit->mTopLine) * mEdit->mTextHeight);
          rcFold.setRight(rcFold.left() + mEdit->mGutter.rightOffset() - 4);
          rcFold.setBottom(rcFold.top() + mEdit->mTextHeight);

          mPainter->setPen(mEdit->mCodeFolding.folderBarLinesColor);

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
            inflateRect(rcFold,-2, 0);
            rcFold.setTop(
                        rcFold.top() + ((mEdit->mTextHeight - rcFold.width()) / 2));
            rcFold.setBottom(rcFold.top() + rcFold.width());

            // Paint the square the user can click on
            mPainter->setBrush(mEdit->mGutter.color());
            mPainter->setPen(mEdit->mCodeFolding.folderBarLinesColor);
            mPainter->drawRect(rcFold);

            // Paint minus sign
            mPainter->drawLine(
                        rcFold.left() + 2, rcFold.top() + (rcFold.height() / 2 ),
                        rcFold.right() - 2, rcFold.top() + (rcFold.height() / 2 ));
            // Paint vertical line of plus sign
            if (foldRange->collapsed) {
                x = rcFold.left() + (rcFold.width() / 2);
                mPainter->drawLine(x, rcFold.top() + 2,
                                  x, rcFold.bottom() - 2);
            }
        }
      }
    }

    for (int row = mFirstRow; row <= mLastRow; row++) {
        int line = mEdit->rowToLine(row);
        if ((line > mEdit->mDocument->count()) && (mEdit->mDocument->count() != 0))
            break;
        mEdit->onGutterPaint(*mPainter,line, 0, (row - mEdit->mTopLine) * mEdit->mTextHeight);
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

void QSynEditPainter::setDrawingColors(bool selected)
{
    if (selected) {
        if (colSelFG.isValid())
            mPainter->setPen(colSelFG);
        else
            mPainter->setPen(colFG);
        if (colSelBG.isValid())
            mPainter->setBrush(colSelBG);
        else
            mPainter->setBrush(colBG);
        mPainter->setBackground(mEdit->mBackgroundColor);
    } else {
        mPainter->setPen(colFG);
        mPainter->setBrush(colBG);
        mPainter->setBackground(mEdit->mBackgroundColor);
    }
}

int QSynEditPainter::fixXValue(int xpos)
{
    return mEdit->textOffset() + xpos;
}

void QSynEditPainter::paintToken(
        const QString& lineText,
        const QList<int> &glyphStartCharList,
        const QList<int> &glyphStartPositionList,
        int startGlyph,
        int endGlyph,
        int tokenWidth, int tokenLeft,
        int first, int last, bool /*isSelection*/, const QFont& font,
        const QFont& fontForNonAscii, bool showGlyphs)
{
    bool startPaint;
    int nX;
    bool lastGlyphAscii = false;
    bool fontInited = false;
    int tokenRight = tokenWidth+tokenLeft;

    // qDebug()<<"Paint token"<<lineText<<tokenWidth<<tokenLeft<<first<<last<<rcToken;
    // qDebug()<<glyphStartCharList;
    // qDebug()<<glyphStartPositionList;
    // qDebug()<<startGlyph<<endGlyph;

    if (last >= first && rcToken.right() > rcToken.left()) {
        nX = fixXValue(first);
        first -= tokenLeft;
        last -= tokenLeft;
        QRect rcTokenBack = rcToken;
        mPainter->fillRect(rcTokenBack,mPainter->brush());
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
                // qDebug()<<"painting:"<<glyph<<glyphWidth<<nX<<tokenWidth+glyphWidth<<first<<last;
                //painter->drawText(nX,rcToken.bottom()-painter->fontMetrics().descent()*edit->dpiFactor() , Token[i]);
                if (startPaint) {
                    bool drawed = false;
                    if (mEdit->mOptions.testFlag(eoLigatureSupport))  {
                        bool tryLigature = false;
                        bool isAscii = false;
                        if (glyph.length()==0) {
                        } else if (glyph.length()==1 && glyph.front().unicode()<=32){
                        } else if (glyph.length()==1
                            && glyph.front().unicode()>32
                            && glyph.front().unicode()<128) {
                            tryLigature = true;
                            isAscii = true;
                        } else {
                            tryLigature = true;
                            isAscii = false;
                        }
                        if (tryLigature) {
                            QString textToPaint = glyph;
                            while(i+1<glyphStartCharList.length()) {
                                int glyphStart = glyphStartCharList[i+1];
                                int glyphLen = calcSegmentInterval(glyphStartCharList,lineText.length(),i+1);
                                QString glyph2 = lineText.mid(glyphStart,glyphLen);
                                // if (!OperatorGlyphs.contains(glyph))
                                //     break;
                                if (isAscii) {
                                    if ( glyph2.length()!=1
                                            || glyph2.front().unicode()<=32
                                            || glyph2.front().unicode()>=128)
                                        break;
                                } else {
                                    if ( glyph2.length()<1
                                         ||
                                         (glyph2.length()==1
                                          && glyph2.front().unicode()>32
                                          && glyph2.front().unicode()<128))
                                        break;
                                }
                                i+=1;
                                glyphWidth += calcSegmentInterval(glyphStartPositionList, tokenRight, i);
                                textToPaint+=glyph2;
                                if (tokenWidth + glyphWidth > last )
                                    break;
                            }
                            if (!fontInited || lastGlyphAscii!=isAscii) {
                                if (isAscii) {
                                    mPainter->setFont(font);
                                } else {
                                    mPainter->setFont(fontForNonAscii);
                                }
                                lastGlyphAscii = isAscii;
                                fontInited = true;
                            }
                            mPainter->drawText(nX,rcToken.bottom()-mPainter->fontMetrics().descent() , textToPaint);
                            drawed = true;
                        }
                    }
                    if (!drawed) {
                        if (glyph.length()==1 && glyph.front().unicode()<128) {
                            QString ch;
                            int padding=0;
                            if (showGlyphs) {
                                switch(glyph.front().unicode()) {
                                case '\t':
                                    ch=TabGlyph;
                                    padding=(glyphWidth-1)/2*mEdit->mCharWidth;
                                    break;
                                case ' ':
                                    ch=SpaceGlyph;
                                    break;
                                default:
                                    ch=glyph;
                                }
                            } else {
                                ch=glyph;
                            }
                            if (ch!=" " && ch!="\t") {
                                if (!fontInited || !lastGlyphAscii) {
                                    mPainter->setFont(font);
                                    fontInited = true;
                                    lastGlyphAscii = true;
                                }
                                mPainter->drawText(nX+padding,rcToken.bottom()-mPainter->fontMetrics().descent() , ch);
                            }
                            //qDebug()<<"Drawing"<<glyph<<nX<<glyphWidth;
                        } else {
                            if (!fontInited || lastGlyphAscii) {
                                mPainter->setFont(fontForNonAscii);
                                fontInited = true;
                                lastGlyphAscii = false;
                            }
                            mPainter->drawText(nX,rcToken.bottom()-mPainter->fontMetrics().descent() , glyph);
                        }
                        drawed = true;
                    }
                    nX += glyphWidth;
                }
                tokenWidth += glyphWidth;
                if (tokenWidth > last)
                    break;
            }
        }

        rcToken.setLeft(rcToken.right()+1);
    }
}

void QSynEditPainter::paintEditAreas(const EditingAreaList &areaList)
{
    QRect rc;
    int x1,x2;
    int offset;
    //painter->setClipRect(rcLine);
    rc=rcLine;
    rc.setBottom(rc.bottom()-1);
    setDrawingColors(false);
    for (const PEditingArea& p:areaList) {
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
        mPainter->setPen(p->color);
        mPainter->setBrush(Qt::NoBrush);
        switch(p->type) {
        case EditingAreaType::eatRectangleBorder:
            mPainter->drawRect(rc);
            break;
        case EditingAreaType::eatUnderLine:
            mPainter->drawLine(rc.left(),rc.bottom(),rc.right(),rc.bottom());
            break;
        case EditingAreaType::eatWaveUnderLine:
            offset=3;
            int lastX=rc.left();
            int lastY=rc.bottom()-offset;
            int t = rc.left();
            while (t<rc.right()) {
                t+=3;
                if (t>rc.right())
                    t = rc.right();
                offset = 3 - offset;
                mPainter->drawLine(lastX,lastY,t,rc.bottom()-offset);
                lastX = t;
                lastY = rc.bottom()-offset;
            }

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
    int nX1, nX2;
    // Compute some helper variables.
    nC1 = std::max(mLeft, mTokenAccu.left);
    nC2 = std::min(mRight, mTokenAccu.left + mTokenAccu.width);
    if (mIsComplexLine) {
        bU1 = (nC1 < mLineSelStart);
        bSel = (nC1 < mLineSelEnd) && (nC2 >= mLineSelStart);
        bU2 = (nC2 >= mLineSelEnd);
        isComplexToken = bSel && (bU1 || bU2);
    } else {
        bSel = mIsLineSelected;
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
                setDrawingColors(false);
                rcToken.setRight(fixXValue(mLineSelStart));
                paintToken(
                            lineText,
                            glyphStartCharList,
                            glyphStartPositionsList,
                            mTokenAccu.startGlyph,
                            mTokenAccu.endGlyph,
                            mTokenAccu.width,mTokenAccu.left,nC1,mLineSelStart,false,mTokenAccu.font,mTokenAccu.nonAsciiFont, mTokenAccu.showSpecialGlyphs);
            }
            // selected part of the token
            setDrawingColors(true);
            nC1Sel = std::max(mLineSelStart, nC1);
            nC2Sel = std::min(mLineSelEnd, nC2);
            rcToken.setRight(fixXValue(nC2Sel));
            paintToken(
                        lineText,
                        glyphStartCharList,
                        glyphStartPositionsList,
                        mTokenAccu.startGlyph,
                        mTokenAccu.endGlyph,
                        mTokenAccu.width, mTokenAccu.left, nC1Sel, nC2Sel,true,mTokenAccu.font,mTokenAccu.nonAsciiFont, mTokenAccu.showSpecialGlyphs);
            // second unselected part of the token
            if (bU2) {
                setDrawingColors(false);
                rcToken.setRight(fixXValue(nC2));
                paintToken(
                            lineText,
                            glyphStartCharList,
                            glyphStartPositionsList,
                            mTokenAccu.startGlyph,
                            mTokenAccu.endGlyph,
                            mTokenAccu.width, mTokenAccu.left, mLineSelEnd, nC2,false,mTokenAccu.font,mTokenAccu.nonAsciiFont, mTokenAccu.showSpecialGlyphs);
            }
        } else {
            setDrawingColors(bSel);
            rcToken.setRight(fixXValue(nC2));
            paintToken(
                        lineText,
                        glyphStartCharList,
                        glyphStartPositionsList,
                        mTokenAccu.startGlyph,
                        mTokenAccu.endGlyph,
                        mTokenAccu.width, mTokenAccu.left, nC1, nC2,bSel,mTokenAccu.font,mTokenAccu.nonAsciiFont, mTokenAccu.showSpecialGlyphs);
        }
    }

    // Fill the background to the end of this line if necessary.
    if (bFillToEOL && rcToken.left() < rcLine.right()) {
        if (mIsSpecialLine && colSpBG.isValid())
            colBG = colSpBG;
        else
            colBG = colEditorBG();
        if (mIsComplexLine) {
            nX1 = fixXValue(mLineSelStart);
            nX2 = fixXValue(mLineSelEnd);
            if (rcToken.left() < nX1) {
                setDrawingColors(false);
                rcToken.setRight(nX1);
//                if (TokenAccu.Len != 0 && TokenAccu.Style != FontStyle::fsNone)
//                    AdjustEndRect();
                mPainter->fillRect(rcToken,mPainter->brush());
                rcToken.setLeft(nX1);
            }
            if (rcToken.left() < nX2) {
                setDrawingColors(true);
                rcToken.setRight(nX2);
                mPainter->fillRect(rcToken,mPainter->brush());
                rcToken.setLeft(nX2);
            }
            if (rcToken.left() < rcLine.right()) {
                setDrawingColors(false);
                rcToken.setRight(rcLine.right());
                mPainter->fillRect(rcToken,mPainter->brush());
            }
        }  else {
            setDrawingColors(mIsLineSelected);
            rcToken.setRight(rcLine.right());
//            if (TokenAccu.Len != 0 && TokenAccu.Style != FontStyle::fsNone)
//                AdjustEndRect();
            mPainter->fillRect(rcToken,mPainter->brush());
        }
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
        QList<int> &glyphStartPositionList,
        int &tokenWidth)
{
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
    if (!background.isValid() ) {
        background = colEditorBG();
    }
    if (!foreground.isValid()) {
        foreground = mEdit->mForegroundColor;
    }

    mEdit->onPreparePaintHighlightToken(line,mEdit->mSyntaxer->getTokenPos()+1,
        token,attri,style,foreground,background);

    // Do we have to paint the old chars first, or can we just append?
    bool bCanAppend = false;
    bool bInitFont = (mTokenAccu.width==0);
    if (mTokenAccu.width > 0 ) {
        if (showGlyphs == mTokenAccu.showSpecialGlyphs) {
            // font style must be the same or token is only spaces
            if (mTokenAccu.style != style) {
                bInitFont = true;
            } else {
                if (
                  // background color must be the same and
                    (mTokenAccu.background == background) &&
                  // foreground color must be the same or token is only spaces
                  (mTokenAccu.foreground == foreground)) {
                    bCanAppend = true;
                }
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
        mTokenAccu.nonAsciiFont = mEdit->fontForNonAscii();
        mTokenAccu.nonAsciiFont.setBold(style & FontStyle::fsBold);
        mTokenAccu.nonAsciiFont.setItalic(style & FontStyle::fsItalic);
        mTokenAccu.nonAsciiFont.setStrikeOut(style & FontStyle::fsStrikeOut);
        mTokenAccu.nonAsciiFont.setUnderline(style & FontStyle::fsUnderline);
    }
    //calculate width of the token ( and update it's glyph start positions )
    int tokenRight;
    int startGlyph, endGlyph;
    tokenWidth = mEdit->mDocument->updateGlyphStartPositionList(
                lineText,
                glyphStartCharList,
                tokenStartChar,
                tokenEndChar,
                QFontMetrics(mTokenAccu.font),
                QFontMetrics(mTokenAccu.nonAsciiFont),
                glyphStartPositionList,
                tokenLeft,
                tokenRight,
                startGlyph,
                endGlyph);

    // Only accumulate tokens if it's visible.
    if (tokenLeft < mRight) {
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
    int tabSteps, lineIndent, lastNonBlank, X, Y;
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
            // Set vertical coord
            Y = (row - mEdit->mTopLine) * mEdit->mTextHeight; // limit inside clip rect
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
            if (mEdit->tabSize()>0)
                indentLevel = lineIndent / mEdit->tabSize();
            // Step horizontal coord
            //TabSteps = edit->mTabWidth;
            tabSteps = 0;
            indentLevel = 0;

            while (tabSteps < lineIndent) {
                X = tabSteps * mEdit->mCharWidth + mEdit->textOffset() - 2;
                tabSteps+=mEdit->tabSize();
                indentLevel++ ;
                if (mEdit->mSyntaxer) {
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
                }
                if (mEdit->mCodeFolding.fillIndents) {
                    int X1;
                    if (tabSteps>lineIndent)
                        X1 = lineIndent * mEdit->mCharWidth + mEdit->textOffset() - 2;
                    else
                        X1 = tabSteps * mEdit->mCharWidth + mEdit->textOffset() - 2;
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

    if (!mEdit->mUseCodeFolding)
        return;

    // Paint collapsed lines using changed pen
    if (mEdit->mCodeFolding.showCollapsedLine) {
        mPainter->setPen(mEdit->mCodeFolding.collapsedLineColor);
        for (int i=0; i< mEdit->mAllFoldRanges->count();i++) {
            PCodeFoldingRange range = (*mEdit->mAllFoldRanges)[i];
            if (range->collapsed && !range->parentCollapsed() &&
                    (range->fromLine <= mLastLine) && (range->fromLine >= mFirstLine) ) {
                // Get starting and end points
                Y = (mEdit->lineToRow(range->fromLine) - mEdit->mTopLine + 1) * mEdit->mTextHeight - 1;
                mPainter->drawLine(mClip.left(),Y, mClip.right(),Y);
            }
        }
    }

}

void QSynEditPainter::getBraceColorAttr(int level, PTokenAttribute &attr)
{
    if (!mEdit->mOptions.testFlag(EditorOption::eoShowRainbowColor))
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
    QString sLine; // the current line
    QString sToken; // token info
    int tokenLeft, tokenWidth;
    PTokenAttribute attr;
    EditingAreaList  areaList;
    PCodeFoldingRange foldRange;
    PTokenAttribute preeditAttr;

    // Initialize rcLine for drawing. Note that Top and Bottom are updated
    // inside the loop. Get only the starting point for this.
    rcLine = mClip;
    rcLine.setBottom((mFirstRow - mEdit->mTopLine) * mEdit->mTextHeight);
    mTokenAccu.width = 0;
    mTokenAccu.left = 0;
    mTokenAccu.style = FontStyle::fsNone;
    // Now loop through all the lines. The indices are valid for Lines.
    BufferCoord selectionBegin = mEdit->blockBegin();
    BufferCoord selectionEnd= mEdit->blockEnd();
    for (int row = mFirstRow; row<=mLastRow; row++) {
        int vLine = mEdit->rowToLine(row);
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
        mIsComplexLine = false;
        mLineSelStart = 0;
        mLineSelEnd = 0;
        // Does the selection intersect the visible area?
        if (bAnySelection && (row >= mSelStart.row) && (row <= mSelEnd.row)) {
            // Default to a fully selected line. This is correct for the smLine
            // selection mode and a good start for the smNormal mode.
            mLineSelStart = mLeft;
            mLineSelEnd = mRight + 1;
            if ((mEdit->mActiveSelectionMode == SelectionMode::Column) ||
                    ((mEdit->mActiveSelectionMode == SelectionMode::Normal) && (row == mSelStart.row)) ) {
                int xpos = mEdit->xposToGlyphLeft(vLine,mSelStart.x);
                if (xpos > mRight) {
                    mLineSelStart = 0;
                    mLineSelEnd = 0;
                } else if (xpos > mLeft) {
                    mLineSelStart = xpos;
                    mIsComplexLine = true;
                }
            }
            if ( (mEdit->mActiveSelectionMode == SelectionMode::Column) ||
                 ((mEdit->mActiveSelectionMode == SelectionMode::Normal) && (row == mSelEnd.row)) ) {
                int xpos = mEdit->xposToGlyphLeft(vLine,mSelEnd.x);
                if (xpos < mLeft) {
                    mLineSelStart = 0;
                    mLineSelEnd = 0;
                } else if (xpos < mRight) {
                    mLineSelEnd = xpos;
                    mIsComplexLine = true;
                }
            }
        } //endif bAnySelection

        // Update the rcLine rect to this line.
//        rcLine.setTop(rcLine.bottom());
//        rcLine.setBottom(rcLine.bottom()+edit->mTextHeight);
        rcLine.setTop((row - mEdit->mTopLine) * mEdit->mTextHeight);
        rcLine.setHeight(mEdit->mTextHeight);

        mIsLineSelected = (!mIsComplexLine) && (mLineSelStart > 0);
        rcToken = rcLine;

        int lineWidth;
        QList<int> glyphStartCharList = mEdit->mDocument->getGlyphStartCharList(vLine-1,sLine);
        QList<int> glyphStartPositionsList = mEdit->mDocument->getGlyphStartPositionList(vLine-1,sLine, lineWidth);

        if (!mEdit->mSyntaxer || !mEdit->mSyntaxer->enabled()) {
              sToken = sLine;
              tokenWidth = lineWidth;
              if (mEdit->mOptions.testFlag(eoShowLineBreaks) && (!mIsLineSelected) && (!mIsSpecialLine) && (tokenWidth <= mLeft)) {
                  expandGlyphStartCharList(LineBreakGlyph, sLine.length(), glyphStartCharList);
                  int width = mEdit->document()->glyphWidth(LineBreakGlyph,0);
                  glyphStartPositionsList.append(tokenWidth);
                  sLine+=LineBreakGlyph;
                  tokenWidth += width;
              }
              if (mIsComplexLine) {
                  setDrawingColors(true);
                  rcToken.setLeft(std::max(rcLine.left(), fixXValue(mLineSelStart)));
                  rcToken.setRight(std::min(rcLine.right(), fixXValue(mLineSelEnd)));
                  paintToken(
                              sLine,
                              glyphStartCharList,
                              glyphStartPositionsList,
                              0,
                              glyphStartCharList.length(),
                              tokenWidth, 0, mLineSelStart, mLineSelEnd,false,mEdit->font(),mEdit->fontForNonAscii(),false);
                  setDrawingColors(false);
                  rcToken.setLeft(std::max(rcLine.left(), fixXValue(mLeft)));
                  rcToken.setRight(std::min(rcLine.right(), fixXValue(mLineSelStart)));
                  paintToken(
                              sLine,
                              glyphStartCharList,
                              glyphStartPositionsList,
                              0,
                              glyphStartCharList.length(),
                              tokenWidth, 0, mLeft, mLineSelStart,false,mEdit->font(),mEdit->fontForNonAscii(),false);
                  rcToken.setLeft(std::max(rcLine.left(), fixXValue(mLineSelEnd)));
                  rcToken.setRight(std::min(rcLine.right(), fixXValue(mRight)));
                  paintToken(
                              sLine,
                              glyphStartCharList,
                              glyphStartPositionsList,
                              0,
                              glyphStartCharList.length(),
                              tokenWidth, 0, mLineSelEnd, mRight,true, mEdit->font(), mEdit->fontForNonAscii(),false);
              } else {
                  setDrawingColors(mIsLineSelected);
                  paintToken(
                              sLine,
                              glyphStartCharList,
                              glyphStartPositionsList,
                              0,
                              glyphStartCharList.length(),
                              tokenWidth, 0, mLeft, mRight, mIsLineSelected,mEdit->font(),mEdit->fontForNonAscii(),false);
              }
              //Paint editingAreaBorders
              if (mIsCurrentLine && mEdit->mInputPreeditString.length()>0) {
                  PEditingArea area = std::make_shared<EditingArea>();
                  area->beginX = mEdit->charToGlyphLeft(vLine, sLine, mEdit->mCaretX);
                  area->endX = mEdit->charToGlyphLeft(vLine, sLine,mEdit->mCaretX + mEdit->mInputPreeditString.length());
                  area->type = EditingAreaType::eatUnderLine;
                  area->color = colFG;
                  areaList.append(area);
                  paintEditAreas(areaList);
              }
        } else {
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
                    // mEdit->mSyntaxer->next();
                    // if (mEdit->mSyntaxer->eol())
                    //     break;
                    // sToken = mEdit->mSyntaxer->getToken();
                    // // Maybe should also test whether GetTokenPos changed...
                    // if (sToken.isEmpty()) {
                    //     //qDebug()<<QSynEdit::tr("The highlighter seems to be in an infinite loop");
                    //     throw BaseError(QSynEdit::tr("The syntaxer seems to be in an infinite loop"));
                    // }
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
                        showGlyph = mEdit->mOptions.testFlag(eoShowLeadingSpaces);
                    } else if (pos+sToken.length()==sLine.length()) {
                        showGlyph = mEdit->mOptions.testFlag(eoShowTrailingSpaces);
                    } else {
                        showGlyph = mEdit->mOptions.testFlag(eoShowInnerSpaces);
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
                            glyphStartPositionsList,
                            tokenWidth);
                tokenLeft+=tokenWidth;
                // Let the highlighter scan the next token.
                mEdit->mSyntaxer->next();
            }
            mEdit->mDocument->setLineWidth(vLine-1, sLine, tokenLeft, glyphStartPositionsList);
            if (tokenLeft<mRight) {
                QString addOnStr;

                // Paint folding
                foldRange = mEdit->foldStartAtLine(vLine);
                if ((foldRange) && foldRange->collapsed) {
                    addOnStr = mEdit->syntaxer()->foldString(sLine);
                } else {
                    // Draw LineBreak glyph.
                    if (mEdit->mOptions.testFlag(eoShowLineBreaks)
                            && (!mIsLineSelected)
                            && (!mIsSpecialLine)
                            && (mEdit->mDocument->lineWidth(vLine-1) < mRight)) {
                        addOnStr = LineBreakGlyph;
                    }
                }
                if (!addOnStr.isEmpty()) {
                    expandGlyphStartCharList(addOnStr, sLine.length(), glyphStartCharList);
                    int len=glyphStartCharList.length()-glyphStartPositionsList.length();
                    for (int i=0;i<len;i++) {
                        glyphStartPositionsList.append(tokenLeft);
                    }
                    attr = mEdit->mSyntaxer->symbolAttribute();
                    getBraceColorAttr(mEdit->mSyntaxer->getState().braceLevel,attr);
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
        }

        // Now paint the right edge if necessary. We do it line by line to reduce
        // the flicker. Should not cost very much anyway, compared to the many
        // calls to ExtTextOut.
        if (bDoRightEdge) {
            mPainter->setPen(mEdit->mRightEdgeColor);
            mPainter->drawLine(nRightEdge, rcLine.top(),nRightEdge,rcLine.bottom()+1);
        }
        mIsCurrentLine = false;
    }
}
}
