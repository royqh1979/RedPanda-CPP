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

QSynEditPainter::QSynEditPainter(QSynEdit *edit, QPainter *painter, int FirstRow, int LastRow, int FirstCol, int LastCol)
{
    this->edit = edit;
    this->painter = painter;
    this->aFirstRow = FirstRow;
    this->aLastRow = LastRow;
    this->FirstCol = FirstCol;
    this->LastCol = LastCol;
}

void QSynEditPainter::paintTextLines(const QRect& clip)
{
    painter->fillRect(clip, edit->mBackgroundColor);
    AClip = clip;
    vFirstLine = edit->rowToLine(aFirstRow);
    vLastLine = edit->rowToLine(aLastRow);
    bCurrentLine = false;
    // If the right edge is visible and in the invalid area, prepare to paint it.
    // Do this first to realize the pen when getting the dc variable.
    bDoRightEdge = false;
    if (edit->mRightEdge > 0) { // column value
        nRightEdge = edit->textOffset()+ edit->mRightEdge * edit->mCharWidth; // pixel value
        if (nRightEdge >= AClip.left() &&nRightEdge <= AClip.right()) {
            bDoRightEdge = true;
            QPen pen(edit->mRightEdgeColor,1);
            painter->setPen(pen);
        }
    }

    // Paint the visible text lines. To make this easier, compute first the
    // necessary information about the selected area: is there any visible
    // selected area, and what are its lines / columns?
    if (vLastLine >= vFirstLine) {
      computeSelectionInfo();
      paintLines();
    }
    //painter->setClipping(false);

    // If anything of the two pixel space before the text area is visible, then
    // fill it with the component background color.
    if (AClip.left() <edit->mGutterWidth + 2) {
        rcToken = AClip;
        rcToken.setLeft( std::max(AClip.left(), edit->mGutterWidth));
        rcToken.setRight(edit->mGutterWidth + 2);
        // Paint whole left edge of the text with same color.
        // (value of WhiteAttribute can vary in e.g. MultiSyn)
        painter->fillRect(rcToken,colEditorBG());
        // Adjust the invalid area to not include this area.
        AClip.setLeft(rcToken.right());
    }
    // If there is anything visible below the last line, then fill this as well.
    rcToken = AClip;
    rcToken.setTop((aLastRow - edit->mTopLine + 1) * edit->mTextHeight);
    if (rcToken.top() < rcToken.bottom()) {
        painter->fillRect(rcToken,colEditorBG());
        // Draw the right edge if necessary.
        if (bDoRightEdge) {
            QPen pen(edit->mRightEdgeColor,1);
            painter->setPen(pen);
            painter->drawLine(nRightEdge, rcToken.top(),nRightEdge, rcToken.bottom() + 1);
        }
    }

    // This messes with pen colors, so draw after right margin has been drawn
    paintFoldAttributes();
}

void QSynEditPainter::paintGutter(const QRect& clip)
{
    int cRow;
    QRect rcLine, rcFold;
    QString s;
    int vLine;
    int vLineTop;
    int x;

    AClip = clip;

    painter->fillRect(AClip,edit->mGutter.color());

    rcLine=AClip;
    if (edit->mGutter.showLineNumbers()) {
        // prepare the rect initially
        rcLine = AClip;
        rcLine.setRight( std::max(rcLine.right(), edit->mGutterWidth - 2));
        rcLine.setBottom(rcLine.top());

        if (edit->mGutter.useFontStyle()) {
            painter->setFont(edit->mGutter.font());
        } else {
            QFont newFont = painter->font();
            newFont.setBold(false);
            newFont.setItalic(false);
            newFont.setStrikeOut(false);
            newFont.setUnderline(false);
            painter->setFont(newFont);
        }
        QColor textColor;
        if (edit->mGutter.textColor().isValid()) {
            textColor = edit->mGutter.textColor();
        } else {
            textColor = edit->mForegroundColor;
        }
        // draw each line if it is not hidden by a fold
        BufferCoord selectionStart = edit->blockBegin();
        BufferCoord selectionEnd = edit->blockEnd();
        for (int cRow = aFirstRow; cRow <= aLastRow; cRow++) {
            vLine = edit->rowToLine(cRow);
            if ((vLine > edit->mDocument->count()) && (edit->mDocument->count() > 0 ))
                break;
            if (edit->mGutter.activeLineTextColor().isValid()) {
                if (
                        (edit->mCaretY==vLine)     ||
                        (edit->mActiveSelectionMode == SelectionMode::Column && vLine >= selectionStart.line && vLine <= selectionEnd.line)
                        )
                    painter->setPen(edit->mGutter.activeLineTextColor());
                else
                    painter->setPen(textColor);
            } else {
                painter->setPen(textColor);
            }
            vLineTop = (cRow - edit->mTopLine) * edit->mTextHeight;

            // next line rect
            rcLine.setTop(vLineTop);
            rcLine.setBottom(rcLine.top() + edit->mTextHeight);

            s = edit->mGutter.formatLineNumber(vLine);

            edit->onGutterGetText(vLine,s);
            QRectF textRect;
            textRect = painter->boundingRect(textRect, Qt::AlignLeft,s);
            painter->drawText(
                        (edit->mGutterWidth - edit->mGutter.rightOffset() - 2) - textRect.width(),
                        rcLine.bottom() + ((edit->mTextHeight - int(textRect.height())) / 2 - painter->fontMetrics().descent()),
                        s
                        );
        }
    }

    // Draw the folding lines and squares
    if (edit->mUseCodeFolding) {
      for (cRow = aLastRow; cRow>=aFirstRow; cRow--) {
        vLine = edit->rowToLine(cRow);
        if ((vLine > edit->mDocument->count()) && (edit->mDocument->count() != 0))
            continue;

        // Form a rectangle for the square the user can click on
        rcFold.setLeft(edit->mGutterWidth - edit->mGutter.rightOffset());
        rcFold.setTop((cRow - edit->mTopLine) * edit->mTextHeight);
        rcFold.setRight(rcFold.left() + edit->mGutter.rightOffset() - 4);
        rcFold.setBottom(rcFold.top() + edit->mTextHeight);


        painter->setPen(edit->mCodeFolding.folderBarLinesColor);


        // Need to paint a line?
        if (edit->foldAroundLine(vLine)) {
          x = rcFold.left() + (rcFold.width() / 2);
          painter->drawLine(x,rcFold.top(), x, rcFold.bottom());
        }

        // Need to paint a line end?
        if (edit->foldEndAtLine(vLine)) {
            x = rcFold.left() + (rcFold.width() / 2);
            painter->drawLine(x,rcFold.top(), x, rcFold.top() + rcFold.height() / 2);
            painter->drawLine(x,
                              rcFold.top() + rcFold.height() / 2,
                              rcFold.right() - 2 ,
                              rcFold.top() + rcFold.height() / 2);
        }
        // Any fold ranges beginning on this line?
        PCodeFoldingRange foldRange = edit->foldStartAtLine(vLine);
        if (foldRange) {
            // Draw the bottom part of a line
            if (!foldRange->collapsed) {
                x = rcFold.left() + (rcFold.width() / 2);
                painter->drawLine(x, rcFold.top() + rcFold.height() / 2,
                                  x, rcFold.bottom());
            }

            // make a square rect
            inflateRect(rcFold,-2, 0);
            rcFold.setTop(
                        rcFold.top() + ((edit->mTextHeight - rcFold.width()) / 2));
            rcFold.setBottom(rcFold.top() + rcFold.width());

            // Paint the square the user can click on
            painter->setBrush(edit->mGutter.color());
            painter->setPen(edit->mCodeFolding.folderBarLinesColor);
            painter->drawRect(rcFold);

            // Paint minus sign
            painter->drawLine(
                        rcFold.left() + 2, rcFold.top() + (rcFold.height() / 2 ),
                        rcFold.right() - 2, rcFold.top() + (rcFold.height() / 2 ));
            // Paint vertical line of plus sign
            if (foldRange->collapsed) {
                x = rcFold.left() + (rcFold.width() / 2);
                painter->drawLine(x, rcFold.top() + 2,
                                  x, rcFold.bottom() - 2);
            }
        }
      }
    }

    for (cRow = aFirstRow; cRow <=aLastRow; cRow++) {
        vLine = edit->rowToLine(cRow);
        if ((vLine > edit->mDocument->count()) && (edit->mDocument->count() != 0))
            break;
        edit->onGutterPaint(*painter,vLine, 0, (cRow - edit->mTopLine) * edit->mTextHeight);
    }
}

QColor QSynEditPainter::colEditorBG()
{
    if (edit->mActiveLineColor.isValid() && bCurrentLine) {
        return edit->mActiveLineColor;
    } else {
        return edit->mBackgroundColor;
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
    if (edit->mBlockBegin.line < edit->mBlockEnd.line) {
        vStart = edit->mBlockBegin;
        vEnd = edit->mBlockEnd;
    } else if (edit->mBlockBegin.line > edit->mBlockEnd.line) {
        vEnd = edit->mBlockBegin;
        vStart = edit->mBlockEnd;
    } else if (edit->mBlockBegin.ch != edit->mBlockEnd.ch) {
        // it is only on this line.
        vStart.line = edit->mBlockBegin.line;
        vEnd.line = vStart.line;
        if (edit->mBlockBegin.ch < edit->mBlockEnd.ch) {
            vStart.ch = edit->mBlockBegin.ch;
            vEnd.ch = edit->mBlockEnd.ch;
        } else {
            vStart.ch = edit->mBlockEnd.ch;
            vEnd.ch = edit->mBlockBegin.ch;
        }
    } else
        bAnySelection = false;
    if (edit->mInputPreeditString.length()>0) {
        if (vStart.line == edit->mCaretY && vStart.ch >=edit->mCaretX) {
            vStart.ch+=edit->mInputPreeditString.length();
        }
        if (vEnd.line == edit->mCaretY && vEnd.ch >edit->mCaretX) {
            vEnd.ch+=edit->mInputPreeditString.length();
        }
    }
    // If there is any visible selection so far, then test if there is an
    // intersection with the area to be painted.
    if (bAnySelection) {
        // Don't care if the selection is not visible.
        bAnySelection = (vEnd.line >= vFirstLine) && (vStart.line <= vLastLine);
        if (bAnySelection) {
            // Transform the selection from text space into screen space
            vSelStart = edit->bufferToDisplayPos(vStart);
            vSelEnd = edit->bufferToDisplayPos(vEnd);
            if (edit->mInputPreeditString.length()
                    && vStart.line == edit->mCaretY) {
                QString sLine = edit->lineText().left(edit->mCaretX-1)
                        + edit->mInputPreeditString
                        + edit->lineText().mid(edit->mCaretX-1);
                vSelStart.Column = edit->charToColumn(sLine,vStart.ch);
            }
            if (edit->mInputPreeditString.length()
                    && vEnd.line == edit->mCaretY) {
                QString sLine = edit->lineText().left(edit->mCaretX-1)
                        + edit->mInputPreeditString
                        + edit->lineText().mid(edit->mCaretX-1);
                vSelEnd.Column = edit->charToColumn(sLine,vEnd.ch);
            }
            // In the column selection mode sort the begin and end of the selection,
            // this makes the painting code simpler.
            if (edit->mActiveSelectionMode == SelectionMode::Column && vSelStart.Column > vSelEnd.Column)
                std::swap(vSelStart.Column, vSelEnd.Column);
        }
    }
}

void QSynEditPainter::setDrawingColors(bool selected)
{
    if (selected) {
        if (colSelFG.isValid())
            painter->setPen(colSelFG);
        else
            painter->setPen(colFG);
        if (colSelBG.isValid())
            painter->setBrush(colSelBG);
        else
            painter->setBrush(colBG);
        painter->setBackground(edit->mBackgroundColor);
    } else {
        painter->setPen(colFG);
        painter->setBrush(colBG);
        painter->setBackground(edit->mBackgroundColor);
    }
}

int QSynEditPainter::columnToXValue(int col)
{
    return edit->textOffset() + (col - 1) * edit->mCharWidth;
}

void QSynEditPainter::paintToken(const QString &token, int tokenCols, int columnsBefore,
                                    int first, int last, bool /*isSelection*/, const QFont& font,
                                    const QFont& fontForNonAscii, bool showGlyphs)
{
    bool startPaint;
    int nX;

    if (last >= first && rcToken.right() > rcToken.left()) {
//        qDebug()<<"Paint Token"<<Token<<ColumnsBefore<<TokenCols<<First<<Last;
        nX = columnToXValue(first);
        first -= columnsBefore;
        last -= columnsBefore;
        QRect rcTokenBack = rcToken;
        rcTokenBack.setWidth(rcTokenBack.width()-1);
        painter->fillRect(rcTokenBack,painter->brush());
        if (first > tokenCols) {
        } else {
            int tokenColLen=0;
            startPaint = false;
            for (int i=0;i<token.length();i++) {
                int charCols=0;
                QString textToPaint = token[i];
                if (token[i] == '\t') {
                    charCols = edit->tabWidth() - ((columnsBefore+tokenColLen) % edit->tabWidth());
                } else {
                    charCols = edit->charColumns(token[i]);
                }
                if (tokenColLen+charCols>=first) {
                    if (!startPaint && (tokenColLen+1!=first)) {
                        nX-= (first - tokenColLen - 1) * edit->mCharWidth;
                    }
                    startPaint = true;
                }
                if (tokenColLen+charCols > last)
                    break;
                //painter->drawText(nX,rcToken.bottom()-painter->fontMetrics().descent()*edit->dpiFactor() , Token[i]);
                if (startPaint) {
                    bool  drawed = false;
                    if (painter->fontInfo().fixedPitch()
                             && edit->mOptions.testFlag(eoLigatureSupport)
                             && !token[i].isSpace()
                             && (token[i].unicode()<=0xFF)) {
                        while(i+1<token.length()) {
                            if (token[i+1].unicode()>0xFF || token[i+1].isSpace())
                                break;
                            i+=1;
                            charCols +=  edit->charColumns(token[i]);
                            textToPaint+=token[i];
                        }
                        painter->drawText(nX,rcToken.bottom()-painter->fontMetrics().descent() , textToPaint);
                        drawed = true;
                    }
                    if (!drawed) {
                        if (token[i].unicode()<=0xFF) {
                            QChar ch;
                            int padding=0;
                            if (showGlyphs) {
                                switch(token[i].unicode()) {
                                case '\t':
                                    ch=TabGlyph;
                                    padding=(charCols-1)/2*edit->mCharWidth;
                                    break;
                                case ' ':
                                    ch=SpaceGlyph;
                                    break;
                                default:
                                    ch=token[i];
                                }
                            } else {
                                ch=token[i];
                            }
                            painter->drawText(nX+padding,rcToken.bottom()-painter->fontMetrics().descent() , ch);
                        } else {
                            painter->setFont(fontForNonAscii);
                            painter->drawText(nX,rcToken.bottom()-painter->fontMetrics().descent() , token[i]);
                            painter->setFont(font);
                        }
                        drawed = true;
                    }
                    nX += charCols * edit->mCharWidth;
                }

                tokenColLen += charCols;
            }
        }

        rcToken.setLeft(rcToken.right());
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
        if (p->beginX > LastCol)
          continue;
        if (p->endX < FirstCol)
          continue;
        if (p->beginX < FirstCol)
          x1 = FirstCol;
        else
          x1 = p->beginX;
        if (p->endX > LastCol)
          x2 = LastCol;
        else
          x2 = p->endX;
        rc.setLeft(columnToXValue(x1));
        rc.setRight(columnToXValue(x2));
        painter->setPen(p->color);
        painter->setBrush(Qt::NoBrush);
        switch(p->type) {
        case EditingAreaType::eatRectangleBorder:
            painter->drawRect(rc);
            break;
        case EditingAreaType::eatUnderLine:
            painter->drawLine(rc.left(),rc.bottom(),rc.right(),rc.bottom());
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
                painter->drawLine(lastX,lastY,t,rc.bottom()-offset);
                lastX = t;
                lastY = rc.bottom()-offset;
            }

        }
    }
}

void QSynEditPainter::paintHighlightToken(bool bFillToEOL)
{
    bool bComplexToken;
    int nC1, nC2, nC1Sel, nC2Sel;
    bool bU1, bSel, bU2;
    int nX1, nX2;
    // Compute some helper variables.
    nC1 = std::max(FirstCol, mTokenAccu.columnsBefore + 1);
    nC2 = std::min(LastCol, mTokenAccu.columnsBefore + mTokenAccu.columns + 1);
    if (bComplexLine) {
      bU1 = (nC1 < nLineSelStart);
      bSel = (nC1 < nLineSelEnd) && (nC2 >= nLineSelStart);
      bU2 = (nC2 >= nLineSelEnd);
      bComplexToken = bSel && (bU1 || bU2);
    } else {
      bSel = bLineSelected;
      bComplexToken = false;
      bU1 = false; // to shut up compiler warning.
      bU2 = false; // to shut up compiler warning.
    }
    // Any token chars accumulated?
    if (mTokenAccu.columns > 0) {
        // Initialize the colors and the font style.
        colBG = mTokenAccu.background;
        colFG = mTokenAccu.foreground;
        if (bSpecialLine) {
            if (colSpFG.isValid())
                colFG = colSpFG;
            if (colSpBG.isValid())
                colBG = colSpBG;
        }

//        if (bSpecialLine && edit->mOptions.testFlag(eoSpecialLineDefaultFg))
//            colFG = TokenAccu.FG;
        QFont font = edit->font();
        font.setBold(mTokenAccu.style & FontStyle::fsBold);
        font.setItalic(mTokenAccu.style & FontStyle::fsItalic);
        font.setStrikeOut(mTokenAccu.style & FontStyle::fsStrikeOut);
        font.setUnderline(mTokenAccu.style & FontStyle::fsUnderline);
        painter->setFont(font);
        QFont nonAsciiFont = edit->fontForNonAscii();
        nonAsciiFont.setBold(mTokenAccu.style & FontStyle::fsBold);
        nonAsciiFont.setItalic(mTokenAccu.style & FontStyle::fsItalic);
        nonAsciiFont.setStrikeOut(mTokenAccu.style & FontStyle::fsStrikeOut);
        nonAsciiFont.setUnderline(mTokenAccu.style & FontStyle::fsUnderline);

        // Paint the chars
        if (bComplexToken) {
            // first unselected part of the token
            if (bU1) {
                setDrawingColors(false);
                rcToken.setRight(columnToXValue(nLineSelStart));
                paintToken(
                            mTokenAccu.s,mTokenAccu.columns,mTokenAccu.columnsBefore,nC1,nLineSelStart,false,font,nonAsciiFont, mTokenAccu.showSpecialGlyphs);
            }
            // selected part of the token
            setDrawingColors(true);
            nC1Sel = std::max(nLineSelStart, nC1);
            nC2Sel = std::min(nLineSelEnd, nC2);
            rcToken.setRight(columnToXValue(nC2Sel));
            paintToken(mTokenAccu.s, mTokenAccu.columns, mTokenAccu.columnsBefore, nC1Sel, nC2Sel,true,font,nonAsciiFont, mTokenAccu.showSpecialGlyphs);
            // second unselected part of the token
            if (bU2) {
                setDrawingColors(false);
                rcToken.setRight(columnToXValue(nC2));
                paintToken(mTokenAccu.s, mTokenAccu.columns, mTokenAccu.columnsBefore,nLineSelEnd, nC2,false,font,nonAsciiFont, mTokenAccu.showSpecialGlyphs);
            }
        } else {
            setDrawingColors(bSel);
            rcToken.setRight(columnToXValue(nC2));
            paintToken(mTokenAccu.s, mTokenAccu.columns, mTokenAccu.columnsBefore, nC1, nC2,bSel,font,nonAsciiFont, mTokenAccu.showSpecialGlyphs);
        }
    }

    // Fill the background to the end of this line if necessary.
    if (bFillToEOL && rcToken.left() < rcLine.right()) {
        if (bSpecialLine && colSpBG.isValid())
            colBG = colSpBG;
        else
            colBG = colEditorBG();
        if (bComplexLine) {
            nX1 = columnToXValue(nLineSelStart);
            nX2 = columnToXValue(nLineSelEnd);
            if (rcToken.left() < nX1) {
                setDrawingColors(false);
                rcToken.setRight(nX1);
//                if (TokenAccu.Len != 0 && TokenAccu.Style != FontStyle::fsNone)
//                    AdjustEndRect();
                painter->fillRect(rcToken,painter->brush());
                rcToken.setLeft(nX1);
            }
            if (rcToken.left() < nX2) {
                setDrawingColors(true);
                rcToken.setRight(nX2);
                painter->fillRect(rcToken,painter->brush());
                rcToken.setLeft(nX2);
            }
            if (rcToken.left() < rcLine.right()) {
                setDrawingColors(false);
                rcToken.setRight(rcLine.right());
                painter->fillRect(rcToken,painter->brush());
            }
        }  else {
            setDrawingColors(bLineSelected);
            rcToken.setRight(rcLine.right());
//            if (TokenAccu.Len != 0 && TokenAccu.Style != FontStyle::fsNone)
//                AdjustEndRect();
            painter->fillRect(rcToken,painter->brush());
        }
    }
}

// Store the token chars with the attributes in the TokenAccu
// record. This will paint any chars already stored if there is
// a (visible) change in the attributes.
void QSynEditPainter::addHighlightToken(const QString &token, int columnsBefore,
                                           int tokenColumns, int cLine, PTokenAttribute attri, bool showGlyphs)
{
    bool bCanAppend;
    QColor foreground, background;
    FontStyles style;

    if (attri) {
        foreground = attri->foreground();
        background = attri->background();
        style = attri->styles();
    } else {
        foreground = colFG;
        background = colBG;
        style = getFontStyles(edit->font());
    }

//    if (!Background.isValid() || (edit->mActiveLineColor.isValid() && bCurrentLine)) {
//        Background = colEditorBG();
//    }
    if (!background.isValid() ) {
        background = colEditorBG();
    }
    if (!foreground.isValid()) {
        foreground = edit->mForegroundColor;
    }

    edit->onPreparePaintHighlightToken(cLine,edit->mSyntaxer->getTokenPos()+1,
        token,attri,style,foreground,background);

    // Do we have to paint the old chars first, or can we just append?
    bCanAppend = false;
    if (mTokenAccu.columns > 0 ) {
        if (showGlyphs == mTokenAccu.showSpecialGlyphs) {
            // font style must be the same or token is only spaces
            if (mTokenAccu.style == style) {
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
            paintHighlightToken(false);
    }
    // Don't use AppendStr because it's more expensive.
    if (bCanAppend) {
        mTokenAccu.s.append(token);
        mTokenAccu.columns+=tokenColumns;
    } else {
        mTokenAccu.columns = tokenColumns;
        mTokenAccu.s = token;
        mTokenAccu.columnsBefore = columnsBefore;
        mTokenAccu.foreground = foreground;
        mTokenAccu.background = background;
        mTokenAccu.style = style;
        mTokenAccu.showSpecialGlyphs = showGlyphs;
    }
}

void QSynEditPainter::paintFoldAttributes()
{
    int tabSteps, lineIndent, lastNonBlank, X, Y, cRow, vLine;
    // Paint indent guides. Use folds to determine indent value of these
    // Use a separate loop so we can use a custom pen
    // Paint indent guides using custom pen
    if (edit->mCodeFolding.indentGuides || edit->mCodeFolding.fillIndents) {
        QColor paintColor;
        if (edit->mCodeFolding.indentGuidesColor.isValid()) {
            paintColor = edit->mCodeFolding.indentGuidesColor;
        } else  {
            paintColor = edit->palette().color(QPalette::Text);
        }
        QColor gradientStart = paintColor;
        QColor gradientEnd = paintColor;
        QPen oldPen = painter->pen();

        // Now loop through all the lines. The indices are valid for Lines.
        for (cRow = aFirstRow; cRow<=aLastRow;cRow++) {
            vLine = edit->rowToLine(cRow);
            if (vLine > edit->mDocument->count() && edit->mDocument->count() > 0)
                break;
            // Set vertical coord
            Y = (cRow - edit->mTopLine) * edit->mTextHeight; // limit inside clip rect
            if (edit->mTextHeight % 2 == 1 && vLine % 2 == 0) {
                Y++;
            }
            // Get next nonblank line
            lastNonBlank = vLine - 1;
            while (lastNonBlank + 1 < edit->mDocument->count() && edit->mDocument->getLine(lastNonBlank).isEmpty())
                lastNonBlank++;
            if (lastNonBlank>=edit->document()->count())
                continue;
            lineIndent = edit->getLineIndent(edit->mDocument->getLine(lastNonBlank));
            int braceLevel = edit->mDocument->getSyntaxState(lastNonBlank).braceLevel;
            int indentLevel = braceLevel ;
            if (edit->tabWidth()>0)
                indentLevel = lineIndent / edit->tabWidth();
            // Step horizontal coord
            //TabSteps = edit->mTabWidth;
            tabSteps = 0;
            indentLevel = 0;

            while (tabSteps < lineIndent) {
                X = tabSteps * edit->mCharWidth + edit->textOffset() - 2;
                tabSteps+=edit->tabWidth();
                indentLevel++ ;
                if (edit->mSyntaxer) {
                    if (edit->mCodeFolding.indentGuides) {
                        PTokenAttribute attr = edit->mSyntaxer->symbolAttribute();
                        getBraceColorAttr(indentLevel,attr);
                        paintColor = attr->foreground();
                    }
                    if (edit->mCodeFolding.fillIndents) {
                        PTokenAttribute attr = edit->mSyntaxer->symbolAttribute();
                        getBraceColorAttr(indentLevel,attr);
                        gradientStart=attr->foreground();
                        attr = edit->mSyntaxer->symbolAttribute();
                        getBraceColorAttr(indentLevel+1,attr);
                        gradientStart=attr->foreground();
                    }
                }
                if (edit->mCodeFolding.fillIndents) {
                    int X1;
                    if (tabSteps>lineIndent)
                        X1 = lineIndent * edit->mCharWidth + edit->textOffset() - 2;
                    else
                        X1 = tabSteps * edit->mCharWidth + edit->textOffset() - 2;
                    gradientStart.setAlpha(20);
                    gradientEnd.setAlpha(10);
                    QLinearGradient gradient(X,Y,X1,Y);
                    gradient.setColorAt(1,gradientStart);
                    gradient.setColorAt(0,gradientEnd);
                    painter->fillRect(X,Y,(X1-X),edit->mTextHeight,gradient);
                }

                // Move to top of vertical line
                if (edit->mCodeFolding.indentGuides) {
                    QPen dottedPen(Qt::PenStyle::DashLine);
                    dottedPen.setColor(paintColor);
                    painter->setPen(dottedPen);
                    painter->drawLine(X,Y,X,Y+edit->mTextHeight);
                }
            }
        }
        painter->setPen(oldPen);
    }

    if (!edit->mUseCodeFolding)
        return;

    // Paint collapsed lines using changed pen
    if (edit->mCodeFolding.showCollapsedLine) {
        painter->setPen(edit->mCodeFolding.collapsedLineColor);
        for (int i=0; i< edit->mAllFoldRanges->count();i++) {
            PCodeFoldingRange range = (*edit->mAllFoldRanges)[i];
            if (range->collapsed && !range->parentCollapsed() &&
                    (range->fromLine <= vLastLine) && (range->fromLine >= vFirstLine) ) {
                // Get starting and end points
                Y = (edit->lineToRow(range->fromLine) - edit->mTopLine + 1) * edit->mTextHeight - 1;
                painter->drawLine(AClip.left(),Y, AClip.right(),Y);
            }
        }
    }

}

void QSynEditPainter::getBraceColorAttr(int level, PTokenAttribute &attr)
{
    if (!edit->mOptions.testFlag(EditorOption::eoShowRainbowColor))
        return;
    if (attr->tokenType() != TokenType::Operator)
        return;
    PTokenAttribute oldAttr = attr;
    switch(level % 4) {
    case 0:
        attr = edit->mRainbowAttr0;
        break;
    case 1:
        attr = edit->mRainbowAttr1;
        break;
    case 2:
        attr = edit->mRainbowAttr2;
        break;
    case 3:
        attr = edit->mRainbowAttr3;
        break;
    }
    if (!attr)
        attr = oldAttr;
}

void QSynEditPainter::paintLines()
{
    int cRow; // row index for the loop
    int vLine;
    QString sLine; // the current line
    QString sToken; // token info
    int nTokenColumnsBefore, nTokenColumnLen;
    PTokenAttribute attr;
    int vFirstChar;
    int vLastChar;
    EditingAreaList  areaList;
    PCodeFoldingRange foldRange;
    PTokenAttribute preeditAttr;
    int nFold;
    QString sFold;

    // Initialize rcLine for drawing. Note that Top and Bottom are updated
    // inside the loop. Get only the starting point for this.
    rcLine = AClip;
    rcLine.setBottom((aFirstRow - edit->mTopLine) * edit->mTextHeight);
    mTokenAccu.columns = 0;
    mTokenAccu.columnsBefore = 0;
    // Now loop through all the lines. The indices are valid for Lines.
    BufferCoord selectionBegin = edit->blockBegin();
    BufferCoord selectionEnd= edit->blockEnd();
    for (cRow = aFirstRow; cRow<=aLastRow; cRow++) {
        vLine = edit->rowToLine(cRow);
        if (vLine > edit->mDocument->count() && edit->mDocument->count() != 0)
            break;

        // Get the line.
        sLine = edit->mDocument->getLine(vLine - 1);
        // determine whether will be painted with ActiveLineColor
        if (edit->mActiveSelectionMode == SelectionMode::Column) {
            bCurrentLine = (vLine >= selectionBegin.line && vLine <= selectionEnd.line);
        } else {
            bCurrentLine = (edit->mCaretY == vLine);
        }
        if (bCurrentLine && !edit->mInputPreeditString.isEmpty()) {
            int col = edit->charToColumn(edit->mCaretY,edit->mCaretX);
            int ch = edit->columnToChar(vLine,col);
            sLine = sLine.left(ch-1) + edit->mInputPreeditString
                    + sLine.mid(ch-1);
        }
        // Initialize the text and background colors, maybe the line should
        // use special values for them.
        colFG = edit->mForegroundColor;
        colBG = colEditorBG();
        colSpFG = QColor();
        colSpBG = QColor();
        bSpecialLine = edit->onGetSpecialLineColors(vLine, colSpFG, colSpBG);

        colSelFG = edit->mSelectedForeground;
        colSelBG = edit->mSelectedBackground;
        edit->onGetEditingAreas(vLine, areaList);
        // Removed word wrap support
        vFirstChar = FirstCol;
        vLastChar = LastCol;
        // Get the information about the line selection. Three different parts
        // are possible (unselected before, selected, unselected after), only
        // unselected or only selected means bComplexLine will be FALSE. Start
        // with no selection, compute based on the visible columns.
        bComplexLine = false;
        nLineSelStart = 0;
        nLineSelEnd = 0;
        // Does the selection intersect the visible area?
        if (bAnySelection && (cRow >= vSelStart.Row) && (cRow <= vSelEnd.Row)) {
            // Default to a fully selected line. This is correct for the smLine
            // selection mode and a good start for the smNormal mode.
            nLineSelStart = FirstCol;
            nLineSelEnd = LastCol + 1;
            if ((edit->mActiveSelectionMode == SelectionMode::Column) ||
                ((edit->mActiveSelectionMode == SelectionMode::Normal) && (cRow == vSelStart.Row)) ) {
                int ch = edit->columnToChar(vLine,vSelStart.Column);
                ch = edit->charToColumn(vLine,ch);
                if (ch > LastCol) {
                    nLineSelStart = 0;
                    nLineSelEnd = 0;
                } else if (ch > FirstCol) {
                    nLineSelStart = ch;
                    bComplexLine = true;
                }
            }
            if ( (edit->mActiveSelectionMode == SelectionMode::Column) ||
                ((edit->mActiveSelectionMode == SelectionMode::Normal) && (cRow == vSelEnd.Row)) ) {
                int ch = edit->columnToChar(vLine,vSelEnd.Column);
                int col = edit->charToColumn(vLine,ch);
                if (col<vSelEnd.Column)
                    col = edit->charToColumn(vLine,ch+1);
                if (col < FirstCol) {
                    nLineSelStart = 0;
                    nLineSelEnd = 0;
                } else if (col < LastCol) {
                    nLineSelEnd = col;
                    bComplexLine = true;
                }
            }
        } //endif bAnySelection

        // Update the rcLine rect to this line.
//        rcLine.setTop(rcLine.bottom());
//        rcLine.setBottom(rcLine.bottom()+edit->mTextHeight);
        rcLine.setTop((cRow - edit->mTopLine) * edit->mTextHeight);
        rcLine.setHeight(edit->mTextHeight);

        bLineSelected = (!bComplexLine) && (nLineSelStart > 0);
        rcToken = rcLine;
        if (!edit->mSyntaxer || !edit->mSyntaxer->enabled()) {
              sToken = sLine;
              if (bCurrentLine) {
                  nTokenColumnLen = edit->stringColumns(sLine,0);
              } else {
                  nTokenColumnLen = edit->mDocument->lineColumns(vLine-1);
              }
              if (edit->mOptions.testFlag(eoShowLineBreaks) && (!bLineSelected) && (!bSpecialLine) && (nTokenColumnLen < vLastChar)) {
                  sToken = sToken + LineBreakGlyph;
                  nTokenColumnLen += edit->charColumns(LineBreakGlyph);
              }
              if (bComplexLine) {
                  setDrawingColors(true);
                  rcToken.setLeft(std::max(rcLine.left(), columnToXValue(nLineSelStart)));
                  rcToken.setRight(std::min(rcLine.right(), columnToXValue(nLineSelEnd)));
                  paintToken(sToken, nTokenColumnLen, 0, nLineSelStart, nLineSelEnd,false,edit->font(),edit->fontForNonAscii(),false);
                  setDrawingColors(false);
                  rcToken.setLeft(std::max(rcLine.left(), columnToXValue(FirstCol)));
                  rcToken.setRight(std::min(rcLine.right(), columnToXValue(nLineSelStart)));
                  paintToken(sToken, nTokenColumnLen, 0, FirstCol, nLineSelStart,false,edit->font(),edit->fontForNonAscii(),false);
                  rcToken.setLeft(std::max(rcLine.left(), columnToXValue(nLineSelEnd)));
                  rcToken.setRight(std::min(rcLine.right(), columnToXValue(LastCol)));
                  paintToken(sToken, nTokenColumnLen, 0, nLineSelEnd, LastCol,true,edit->font(),edit->fontForNonAscii(),false);
              } else {
                  setDrawingColors(bLineSelected);
                  paintToken(sToken, nTokenColumnLen, 0, FirstCol, LastCol,bLineSelected,edit->font(),edit->fontForNonAscii(),false);
              }
              //Paint editingAreaBorders
              if (bCurrentLine && edit->mInputPreeditString.length()>0) {
                  PEditingArea area = std::make_shared<EditingArea>();
                  int col = edit->charToColumn(edit->mCaretY,edit->mCaretX);
                  int ch = edit->columnToChar(vLine,col);
                  area->beginX = edit->charToColumn(sLine,ch);
                  area->endX = edit->charToColumn(sLine,ch + edit->mInputPreeditString.length());
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
                edit->mSyntaxer->resetState();
            } else {
                edit->mSyntaxer->setState(
                            edit->mDocument->getSyntaxState(vLine-2));
            }
            edit->mSyntaxer->setLine(sLine, vLine - 1);
            // Try to concatenate as many tokens as possible to minimize the count
            // of ExtTextOut calls necessary. This depends on the selection state
            // or the line having special colors. For spaces the foreground color
            // is ignored as well.
            mTokenAccu.columns = 0;
            nTokenColumnsBefore = 0;
            // Test first whether anything of this token is visible.
            while (!edit->mSyntaxer->eol()) {
                sToken = edit->mSyntaxer->getToken();
                // Work-around buggy highlighters which return empty tokens.
                if (sToken.isEmpty())  {
                    edit->mSyntaxer->next();
                    if (edit->mSyntaxer->eol())
                        break;
                    sToken = edit->mSyntaxer->getToken();
                    // Maybe should also test whether GetTokenPos changed...
                    if (sToken.isEmpty()) {
                        //qDebug()<<QSynEdit::tr("The highlighter seems to be in an infinite loop");
                        throw BaseError(QSynEdit::tr("The syntaxer seems to be in an infinite loop"));
                    }
                }
                //nTokenColumnsBefore = edit->charToColumn(sLine,edit->mHighlighter->getTokenPos()+1)-1;
                nTokenColumnLen = edit->stringColumns(sToken, nTokenColumnsBefore);
                if (nTokenColumnsBefore + nTokenColumnLen >= vFirstChar) {
                    if (nTokenColumnsBefore + nTokenColumnLen >= vLastChar) {
                        if (nTokenColumnsBefore >= vLastChar)
                            break; //*** BREAK ***
                        nTokenColumnLen = vLastChar - nTokenColumnsBefore;
                    }
                    // It's at least partially visible. Get the token attributes now.
                    attr = edit->mSyntaxer->getTokenAttribute();
                    if (sToken == "["
                            || sToken == "("
                            || sToken == "{"
                            ) {
                        SyntaxState rangeState = edit->mSyntaxer->getState();
                        getBraceColorAttr(rangeState.bracketLevel
                                          +rangeState.braceLevel
                                          +rangeState.parenthesisLevel
                                          ,attr);
                    } else if (sToken == "]"
                               || sToken == ")"
                               || sToken == "}"
                               ){
                        SyntaxState rangeState = edit->mSyntaxer->getState();
                        getBraceColorAttr(rangeState.bracketLevel
                                          +rangeState.braceLevel
                                          +rangeState.parenthesisLevel+1,
                                          attr);
                    }
                    if (bCurrentLine && edit->mInputPreeditString.length()>0) {
                        int startPos = edit->mSyntaxer->getTokenPos()+1;
                        int endPos = edit->mSyntaxer->getTokenPos() + sToken.length();
                        //qDebug()<<startPos<<":"<<endPos<<" - "+sToken+" - "<<edit->mCaretX<<":"<<edit->mCaretX+edit->mInputPreeditString.length();
                        if (!(endPos < edit->mCaretX
                                || startPos >= edit->mCaretX+edit->mInputPreeditString.length())) {
                            if (!preeditAttr) {
                                preeditAttr = attr;
                            } else {
                                attr = preeditAttr;
                            }
                        }
                    }
                    bool showGlyph=false;
                    if (attr && attr->tokenType() == TokenType::Space) {
                        int pos = edit->mSyntaxer->getTokenPos();
                        if (pos==0) {
                            showGlyph = edit->mOptions.testFlag(eoShowLeadingSpaces);
                        } else if (pos+sToken.length()==sLine.length()) {
                            showGlyph = edit->mOptions.testFlag(eoShowTrailingSpaces);
                        } else {
                            showGlyph = edit->mOptions.testFlag(eoShowInnerSpaces);
                        }
                    }
                    addHighlightToken(sToken, nTokenColumnsBefore - (vFirstChar - FirstCol),
                      nTokenColumnLen, vLine,attr, showGlyph);
                }
                nTokenColumnsBefore+=nTokenColumnLen;
                // Let the highlighter scan the next token.
                edit->mSyntaxer->next();
            }
//            // Don't assume HL.GetTokenPos is valid after HL.GetEOL == True.
//            //nTokenColumnsBefore += edit->stringColumns(sToken,nTokenColumnsBefore);
//            if (edit->mSyntaxer->eol() && (nTokenColumnsBefore < vLastChar)) {
//                int lineColumns = edit->mDocument->lineColumns(vLine-1);
//                // Draw text that couldn't be parsed by the highlighter, if any.
//                if (nTokenColumnsBefore < lineColumns) {
//                    if (nTokenColumnsBefore + 1 < vFirstChar)
//                        nTokenColumnsBefore = vFirstChar - 1;
//                    nTokenColumnLen = std::min(lineColumns, vLastChar) - (nTokenColumnsBefore + 1);
//                    if (nTokenColumnLen > 0) {
//                        sToken = edit->substringByColumns(sLine,nTokenColumnsBefore+1,nTokenColumnLen);
//                        addHighlightToken(sToken, nTokenColumnsBefore - (vFirstChar - FirstCol),
//                            nTokenColumnLen, vLine, PTokenAttribute(),false);
//                    }
//                }
//            }

            // Paint folding
            foldRange = edit->foldStartAtLine(vLine);
            if ((foldRange) && foldRange->collapsed) {
                sFold = edit->syntaxer()->foldString(sLine);
                nFold = edit->stringColumns(sFold,edit->mDocument->lineColumns(vLine-1));
                attr = edit->mSyntaxer->symbolAttribute();
                getBraceColorAttr(edit->mSyntaxer->getState().braceLevel,attr);
                addHighlightToken(sFold,edit->mDocument->lineColumns(vLine-1) - (vFirstChar - FirstCol)
                  , nFold, vLine, attr,false);
            } else  {
                // Draw LineBreak glyph.
                if (edit->mOptions.testFlag(eoShowLineBreaks)
                        && (!bLineSelected)
                        && (!bSpecialLine)
                        && (edit->mDocument->lineColumns(vLine-1) < vLastChar)) {
                    addHighlightToken(LineBreakGlyph,
                      edit->mDocument->lineColumns(vLine-1)  - (vFirstChar - FirstCol),
                      edit->charColumns(LineBreakGlyph),vLine, edit->mSyntaxer->whitespaceAttribute(),false);
                }
            }
            // Draw anything that's left in the TokenAccu record. Fill to the end
            // of the invalid area with the correct colors.
            paintHighlightToken(true);

            //Paint editingAreaBorders
            foreach (const PEditingArea& area, areaList) {
                if (bCurrentLine && edit->mInputPreeditString.length()>0) {
                    if (area->beginX > edit->mCaretX) {
                        area->beginX+=edit->mInputPreeditString.length();
                    }
                    if (area->endX > edit->mCaretX) {
                        area->endX+=edit->mInputPreeditString.length();
                    }
                }
                area->beginX = edit->charToColumn(sLine, area->beginX);
                area->endX = edit->charToColumn(sLine,area->endX);
            }
            if (bCurrentLine && edit->mInputPreeditString.length()>0) {
                PEditingArea area = std::make_shared<EditingArea>();
                int col = edit->charToColumn(edit->mCaretY,edit->mCaretX);
                int ch = edit->columnToChar(vLine,col);
                area->beginX = edit->charToColumn(sLine,ch);
                area->endX = edit->charToColumn(sLine,ch + edit->mInputPreeditString.length());
                area->type = EditingAreaType::eatUnderLine;
                if (preeditAttr) {
                    area->color = preeditAttr->foreground();
                } else {
                    area->color = colFG;
                }
                areaList.append(area);
            }
            paintEditAreas(areaList);
        }

        // Now paint the right edge if necessary. We do it line by line to reduce
        // the flicker. Should not cost very much anyway, compared to the many
        // calls to ExtTextOut.
        if (bDoRightEdge) {
            painter->setPen(edit->mRightEdgeColor);
            painter->drawLine(nRightEdge, rcLine.top(),nRightEdge,rcLine.bottom()+1);
        }
        bCurrentLine = false;
    }
}
}
