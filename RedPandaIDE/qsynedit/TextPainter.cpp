#include "TextPainter.h"
#include "SynEdit.h"
#include "Constants.h"
#include <cmath>

SynEditTextPainter::SynEditTextPainter(SynEdit *edit, int FirstRow, int LastRow, int FirstCol, int LastCol)
{
    this->edit = edit;
    this->aFirstRow = FirstRow;
    this->aLastRow = LastRow;
    this->FirstCol = FirstCol;
    this->LastCol = LastCol;
}

void SynEditTextPainter::paintTextLines(const QRect& clip)
{
    AClip = clip;
    vFirstLine = edit->rowToLine(aFirstRow);
    vLastLine = edit->rowToLine(aLastRow);
    bCurrentLine = false;
    // If the right edge is visible and in the invalid area, prepare to paint it.
    // Do this first to realize the pen when getting the dc variable.
    QString SynTabGlyphString = SynTabGlyph;
    bDoRightEdge = false;
    if (edit->mRightEdge > 0) { // column value
        nRightEdge = edit->mTextOffset + edit->mRightEdge * edit->mCharWidth; // pixel value
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
      ComputeSelectionInfo();
      PaintLines();
    }

    // If anything of the two pixel space before the text area is visible, then
    // fill it with the component background color.
    if (AClip.left() < edit->clientLeft()+ edit->mGutterWidth + 2) {
        rcToken = AClip;
        rcToken.setLeft( std::max(AClip.left(), edit->mGutterWidth));
        rcToken.setRight(edit->mGutterWidth + 2);
        // Paint whole left edge of the text with same color.
        // (value of WhiteAttribute can vary in e.g. MultiSyn)
        painter->setPen(Qt::NoPen);
        painter->setBrush(colEditorBG());
        painter->drawRect(rcToken);
        // Adjust the invalid area to not include this area.
        AClip.setLeft(rcToken.right());
    }
    // If there is anything visible below the last line, then fill this as well.
    rcToken = AClip;
    rcToken.setTop((aLastRow - edit->mTopLine + 1) * edit->mTextHeight);
    if (rcToken.top() < rcToken.bottom()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(colEditorBG());
        painter->drawRect(rcToken);
        // Draw the right edge if necessary.
        if (bDoRightEdge) {
            QPen pen(edit->mRightEdgeColor,1);
            painter->setPen(pen);
            painter->drawLine(nRightEdge, rcToken.top(),nRightEdge, rcToken.bottom() + 1);
        }
    }

    // This messes with pen colors, so draw after right margin has been drawn
    PaintFoldAttributes();
}

void SynEditTextPainter::paintGutter(const QRect& clip)
{
    int cRow;
    int cMark;
    QRect rcLine, rcFold;
    QList<int> aGutterOffs;
    bool bHasOtherMarks;
    QString s;
    int vFirstLine;
    int vLastLine;
    int vLine;
    int vMarkRow;
    int vGutterRow;
    int vLineTop;
    QSize TextSize;
    int x;
    PSynEditFoldRange FoldRange;

    AClip = clip;

    //todo: Does the following comment still apply?
    // Changed to use fTextDrawer.BeginDrawing and fTextDrawer.EndDrawing only
    // when absolutely necessary.  Note: Never change brush / pen / font of the
    // canvas inside of this block (only through methods of fTextDrawer)!
    // If we have to draw the line numbers then we don't want to erase
    // the background first. Do it line by line with TextRect instead
    // and fill only the area after the last visible line.
    painter->setBrush(edit->mGutter.color());
    painter->setPen(Qt::NoPen);
    rcLine=AClip;

    painter->drawRect(AClip);

    if (edit->mGutter.showLineNumbers()) {
        if (edit->mGutter.useFontStyle()) {
            painter->setFont(edit->mGutter.font());
            painter->setPen(edit->mGutter.textColor());
        } else {
            painter->setPen(edit->palette().color(QPalette::Text));
        }
        painter->setBrush(edit->mGutter.color());

        // prepare the rect initially
        rcLine = AClip;
        rcLine.setRight( std::max(rcLine.right(), edit->mGutterWidth - 2));
        rcLine.setBottom(rcLine.top());
        painter->drawRect(AClip);

        // draw each line if it is not hidden by a fold
        for (int cRow = aFirstRow; cRow <= aLastRow; cRow++) {
            vLine = edit->rowToLine(cRow);
            if ((vLine > edit->mLines->count()) && (edit->mLines->count() > 0 ))
                break;
            vLineTop = (cRow - edit->mTopLine) * edit->mTextHeight;

            // next line rect
            rcLine.setTop(vLineTop);
            rcLine.setBottom(rcLine.top() + edit->mTextHeight);

            s = edit->mGutter.formatLineNumber(vLine);

            if (edit->mOnGutterGetText) {
                edit->mOnGutterGetText(vLine,s);
            }

            QRectF textRect;
            textRect = painter->boundingRect(textRect, Qt::AlignLeft,s);
            painter->drawText(
                 (edit->mGutterWidth - edit->mGutter.rightOffset() - 2) - textRect.width(),
                        rcLine.top() + ((edit->mTextHeight - int(textRect.height())) % 2),
                        s
                        );
        }
    }

    // Draw the folding lines and squares
    if (edit->mUseCodeFolding) {
      for (cRow = aFirstRow; cRow<=aLastRow; cRow++) {
        vLine = edit->rowToLine(cRow);
        if ((vLine > edit->mLines->count()) && (edit->mLines->count() != 0))
            break;

        // Form a rectangle for the square the user can click on
        //rcFold.Left := Gutter.RealGutterWidth(CharWidth) - Gutter.RightOffset;
        rcFold.setLeft(edit->mGutterWidth - edit->mGutter.rightOffset());
        rcFold.setRight(rcFold.left() + edit->mGutter.rightOffset() - 4);
        rcFold.setTop((cRow - edit->mTopLine) * edit->mTextHeight);
        rcFold.setBottom(rcFold.top() + edit->mTextHeight);

        painter->setPen(edit->mCodeFolding.folderBarLinesColor);


        // Need to paint a line?
        if (edit->foldAroundLine(vLine)) {
          x = rcFold.left() + (rcFold.width() % 2);
          painter->drawLine(x,rcFold.top(), x, rcFold.bottom());
        }

        // Need to paint a line end?
        if (edit->foldEndAtLine(vLine)) {
            x = rcFold.left() + (rcFold.width() % 2);
            painter->drawLine(x,rcFold.top(), x, rcFold.top() + rcFold.height() % 2);
            painter->drawLine(x,
                              rcFold.top() + rcFold.height() % 2,
                              rcFold.right() - 2 ,
                              rcFold.top() + rcFold.height() % 2);
        }
        // Any fold ranges beginning on this line?
        FoldRange = edit->foldStartAtLine(vLine);
        if (FoldRange) {
            // Draw the bottom part of a line
            if (!FoldRange->collapsed) {
                x = rcFold.left() + (rcFold.width() % 2);
                painter->drawLine(x, rcFold.top() + rcFold.height() % 2,
                                  x, rcFold.bottom());
            }

            // make a square rect
            inflateRect(rcFold,-2, 0);
            rcFold.setTop(
                        rcFold.top() + ((edit->mTextHeight - rcFold.width()) % 2));
            rcFold.setBottom(rcFold.top() + rcFold.width());

            // Paint the square the user can click on
            painter->setBrush(edit->mGutter.color());
            painter->setPen(edit->mCodeFolding.folderBarLinesColor);
            painter->drawRect(rcFold);

            // Paint minus sign
            painter->drawLine(
                        rcFold.left() + 2, rcFold.top() + (rcFold.height() % 2 ),
                        rcFold.right() - 2, rcFold.top() + (rcFold.height() % 2 ));
            // Paint vertical line of plus sign
            if (FoldRange->collapsed) {
                x = rcFold.left() + (rcFold.width() % 2);
                painter->drawLine(x, rcFold.top() + 2,
                                  x, rcFold.bottom() + 2);
            }
        }
      }
    }

//    // the gutter separator if visible
//    if (edit->mGutter.borderStyle <> gbsNone) and (AClip.Right >= fGutterWidth - 2) then
//      with Canvas do begin
//        Pen.Color := fGutter.BorderColor;
//        Pen.Width := 1;
//        with AClip do begin
//          if fGutter.BorderStyle = gbsMiddle then begin
//            MoveTo(fGutterWidth - 2, Top);
//            LineTo(fGutterWidth - 2, Bottom);
//            Pen.Color := fGutter.Color;
//          end;
//          MoveTo(fGutterWidth - 1, Top);
//          LineTo(fGutterWidth - 1, Bottom);
//        end;
//      end;

//    // now the gutter marks
//    if BookMarkOptions.GlyphsVisible and (Marks.Count > 0) and (aLastRow >= aFirstRow) then begin
//      aGutterOffs := AllocMem((aLastRow - aFirstRow + 1) * SizeOf(integer));
//      vFirstLine := RowToLine(aFirstRow);
//      vLastLine := RowToLine(aLastRow);
//      try
//        // Instead of making a two pass loop we look while drawing the bookmarks
//        // whether there is any other mark to be drawn
//        bHasOtherMarks := FALSE;
//        for cMark := 0 to Marks.Count - 1 do
//          with Marks[cMark] do
//            if Visible and (Line >= vFirstLine) and (Line <= vLastLine) then begin
//              if IsBookmark <> BookMarkOptions.DrawBookmarksFirst then
//                bHasOtherMarks := TRUE
//              else begin
//                vMarkRow := LineToRow(Line);
//                if vMarkRow >= aFirstRow then
//                  DrawMark(Marks[cMark], aGutterOffs[vMarkRow - aFirstRow], vMarkRow);
//              end
//            end;
//        if bHasOtherMarks then
//          for cMark := 0 to Marks.Count - 1 do
//            with Marks[cMark] do begin
//              if Visible and (IsBookmark <> BookMarkOptions.DrawBookmarksFirst)
//                and (Line >= vFirstLine) and (Line <= vLastLine) then begin
//                vMarkRow := LineToRow(Line);
//                if vMarkRow >= aFirstRow then
//                  DrawMark(Marks[cMark], aGutterOffs[vMarkRow - aFirstRow], vMarkRow);
//              end;
//            end;
//        if Assigned(OnGutterPaint) then
//          for cRow := aFirstRow to aLastRow do begin
//            OnGutterPaint(Self, cRow, aGutterOffs[cRow - aFirstRow],
//              (vGutterRow - TopLine) * LineHeight);
//          end;
//      finally
//        FreeMem(aGutterOffs);
//      end;
//    end;
    for (cRow = aFirstRow; cRow <=aLastRow; cRow++) {
        vLine = edit->rowToLine(cRow);
        edit->onGutterPaint(vLine, 0, (cRow - edit->mTopLine) * edit->mTextHeight);
    }
}

QColor SynEditTextPainter::colEditorBG()
{
    if (edit->mActiveLineColor.isValid() && bCurrentLine) {
        return edit->mActiveLineColor;
    } else {
        if (edit->mHighlighter) {
            PSynHighlighterAttribute attr = edit->mHighlighter->whitespaceAttribute();
            if (attr && attr->background().isValid()) {
                return attr->background();
            }
        }
    }
    return edit->palette().color(QPalette::Base);
}

void SynEditTextPainter::ComputeSelectionInfo()
{
    BufferCoord vStart;
    BufferCoord vEnd;
    bAnySelection = false;
    // Only if selection is visible anyway.
    if (!edit->mHideSelection || edit->hasFocus()) {
        bAnySelection = true;
        // Get the *real* start of the selected area.
        if (edit->mBlockBegin.Line < edit->mBlockEnd.Line) {
            vStart = edit->mBlockBegin;
            vEnd = edit->mBlockEnd;
        } else if (edit->mBlockBegin.Line > edit->mBlockEnd.Line) {
            vEnd = edit->mBlockBegin;
            vStart = edit->mBlockEnd;
        } else if (edit->mBlockBegin.Char != edit->mBlockEnd.Char) {
            // No selection at all, or it is only on this line.
            vStart.Line = edit->mBlockBegin.Line;
            vEnd.Line = vStart.Line;
            if (edit->mBlockBegin.Char < edit->mBlockEnd.Char) {
                vStart.Char = edit->mBlockBegin.Char;
                vEnd.Char = edit->mBlockEnd.Char;
            } else {
                vStart.Char = edit->mBlockEnd.Char;
                vEnd.Char = edit->mBlockBegin.Char;
            }
        } else
            bAnySelection = false;
        // If there is any visible selection so far, then test if there is an
        // intersection with the area to be painted.
        if (bAnySelection) {
            // Don't care if the selection is not visible.
            bAnySelection = (vEnd.Line >= vFirstLine) and (vStart.Line <= vLastLine);
            if (bAnySelection) {
                // Transform the selection from text space into screen space
                vSelStart = edit->bufferToDisplayPos(vStart);
                vSelEnd = edit->bufferToDisplayPos(vEnd);
                // In the column selection mode sort the begin and end of the selection,
                // this makes the painting code simpler.
                if (edit->mActiveSelectionMode == SynSelectionMode::smColumn && vSelStart.Column > vSelEnd.Column)
                    std::swap(vSelStart.Column, vSelEnd.Column);
            }
        }
    }
}

void SynEditTextPainter::setDrawingColors(bool Selected)
{
    if (Selected) {
        painter->setPen(colSelFG);
        painter->setBrush(colSelBG);
        painter->setBackground(colSelBG);
    } else {
        painter->setPen(colFG);
        painter->setBrush(colBG);
        painter->setBackground(colBG);
    }
}

int SynEditTextPainter::ColumnToXValue(int Col)
{
    return edit->mTextOffset + (Col - 1) * edit->mCharWidth;
}

void SynEditTextPainter::PaintToken(const QString &Token, int TokenCols, int ColumnsBefore, int First, int Last, bool)
{
    bool startPaint;
    int nX;

    if (Last >= First && rcToken.right() > rcToken.left()) {
        nX = ColumnToXValue(First);
        First -= ColumnsBefore;
        Last -= ColumnsBefore;
        if (First > TokenCols) {
        } else {
          painter->setClipRect(rcToken);
          int tokenColLen=0;
          startPaint = false;
          for (int i=0;i<Token.length();i++) {
              int charCols;
              if (Token[i] == SynTabChar) {
                  charCols = edit->mTabWidth - ((ColumnsBefore+tokenColLen) % edit->mTabWidth);
              } else {
                  charCols = edit->charColumns(Token[i]);
              }
              if (tokenColLen+charCols>=First) {
                  startPaint = true;
                  if (tokenColLen+1!=First) {
                      nX-= (First - tokenColLen - 1) * edit->mCharWidth;
                  }
              }
              painter->drawText(nX,rcToken.bottom(), Token[i]);

              tokenColLen += charCols;
              nX += charCols * edit->mCharWidth;
              if (tokenColLen > Last)
                  break;
          }
        }

        rcToken.setLeft(rcToken.right());
    }
}

void SynEditTextPainter::PaintEditAreas(const SynEditingAreaList &areaList)
{
    QRect rc;
    int x1,x2;
    int offset;
    painter->setClipRect(rcLine);
    rc=rcLine;
    rc.setBottom(rc.bottom()-1);
    setDrawingColors(false);
    for (PSynEditingArea p:areaList) {
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
        rc.setLeft(ColumnToXValue(x1));
        rc.setRight(ColumnToXValue(x2));
        painter->setPen(p->color);
        painter->setBrush(Qt::NoBrush);
        switch(p->type) {
        case SynEditingAreaType::eatRectangleBorder:
            painter->drawRect(rc);
            break;
        case SynEditingAreaType::eatUnderLine:
            painter->drawLine(rc.left(),rc.bottom(),rc.right(),rc.bottom());
            break;
        case SynEditingAreaType::eatWaveUnderLine:
            offset=3;
            int lastX=rc.left();
            int lastY=rc.bottom()-offset;
            int t = rc.left();
            while (t<=rc.right()) {
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

void SynEditTextPainter::PaintHighlightToken(bool bFillToEOL)
{
    bool bComplexToken;
    int nC1, nC2, nC1Sel, nC2Sel;
    bool bU1, bSel, bU2;
    int nX1, nX2;
    // Compute some helper variables.
    nC1 = std::max(FirstCol, TokenAccu.ColumnsBefore + 1);
    nC2 = std::min(LastCol, TokenAccu.ColumnsBefore + TokenAccu.Columns + 1);
    if (bComplexLine) {
      bU1 = (nC1 < nLineSelStart);
      bSel = (nC1 < nLineSelEnd) && (nC2 >= nLineSelStart);
      bU2 = (nC2 >= nLineSelEnd);
      bComplexToken = bSel && (bU1 or bU2);
    } else {
      bSel = bLineSelected;
      bComplexToken = false;
//      bU1 = false; // to shut up compiler warning.
//      bU2 = false; // to shut up compiler warning.
    }
    // Any token chars accumulated?
    if (TokenAccu.Columns > 0) {
        // Initialize the colors and the font style.
        if (!bSpecialLine) {
          colBG = TokenAccu.BG;
          colFG = TokenAccu.FG;
        }

        if (bSpecialLine && edit->mOptions.testFlag(eoSpecialLineDefaultFg))
            colFG = TokenAccu.FG;
        QFont font = edit->font();
        font.setBold(TokenAccu.Style | SynFontStyle::fsBold);
        font.setItalic(TokenAccu.Style | SynFontStyle::fsItalic);
        font.setStrikeOut(TokenAccu.Style | SynFontStyle::fsStrikeOut);
        font.setUnderline(TokenAccu.Style | SynFontStyle::fsUnderline);
        painter->setFont(font);
        // Paint the chars
        if (bComplexToken) {
            // first unselected part of the token
            if (bU1) {
                setDrawingColors(false);
                rcToken.setRight(ColumnToXValue(nLineSelStart));
                PaintToken(TokenAccu.s,TokenAccu.Columns,TokenAccu.ColumnsBefore,nC1,nLineSelStart,false);
            }
            // selected part of the token
            setDrawingColors(true);
            nC1Sel = std::max(nLineSelStart, nC1);
            nC2Sel = std::min(nLineSelEnd, nC2);
            rcToken.setRight(ColumnToXValue(nC2Sel));
            PaintToken(TokenAccu.s, TokenAccu.Columns, TokenAccu.ColumnsBefore, nC1Sel, nC2Sel,true);
            // second unselected part of the token
            if (bU2) {
                setDrawingColors(false);
                rcToken.setRight(ColumnToXValue(nC2));
                PaintToken(TokenAccu.s, TokenAccu.Columns, TokenAccu.ColumnsBefore,nLineSelEnd, nC2,false);
            }
        } else {
            setDrawingColors(bSel);
            rcToken.setRight(ColumnToXValue(nC2));
            PaintToken(TokenAccu.s, TokenAccu.Columns, TokenAccu.ColumnsBefore, nC1, nC2,bSel);
        }
    }

    // Fill the background to the end of this line if necessary.
    if (bFillToEOL && rcToken.left() < rcLine.right()) {
        if (!bSpecialLine)
            colBG = colEditorBG();
        if (bComplexLine) {
            nX1 = ColumnToXValue(nLineSelStart);
            nX2 = ColumnToXValue(nLineSelEnd);
            if (rcToken.left() < nX1) {
                setDrawingColors(false);
                rcToken.setRight(nX1);
//                if (TokenAccu.Len != 0 && TokenAccu.Style != SynFontStyle::fsNone)
//                    AdjustEndRect();
                painter->setPen(Qt::NoPen);
                painter->drawRect(rcToken);
                rcToken.setLeft(nX1);
            }
            if (rcToken.left() < nX2) {
                setDrawingColors(true);
                rcToken.setRight(nX2);
                painter->setPen(Qt::NoPen);
                painter->drawRect(rcToken);
                rcToken.setLeft(nX2);
            }
            if (rcToken.left() < rcLine.right()) {
                setDrawingColors(false);
                rcToken.setRight(rcLine.right());
                painter->setPen(Qt::NoPen);
                painter->drawRect(rcToken);
            }
        }  else {
            setDrawingColors(bLineSelected);
            rcToken.setRight(rcLine.right());
//            if (TokenAccu.Len != 0 && TokenAccu.Style != SynFontStyle::fsNone)
//                AdjustEndRect();
            painter->setPen(Qt::NoPen);
            painter->drawRect(rcToken);
        }
    }
}

bool SynEditTextPainter::TokenIsSpaces(bool &bSpacesTest, const QString& Token, bool& bIsSpaces)
{
    QString pTok;
    if (!bSpacesTest) {
        bSpacesTest = true;
        for (QChar ch:Token) {
            //todo: should include tabs?
            if (ch!= ' ') {
                bIsSpaces = false;
                return bIsSpaces;
            }
        }
        bIsSpaces = true;
    }
    return bIsSpaces;
}

// Store the token chars with the attributes in the TokenAccu
// record. This will paint any chars already stored if there is
// a (visible) change in the attributes.
void SynEditTextPainter::AddHighlightToken(const QString &Token, int ColumnsBefore,
                                           int TokenColumns, int cLine, PSynHighlighterAttribute p_Attri)
{
    bool bCanAppend;
    QColor Foreground, Background;
    SynFontStyles Style;
    bool bSpacesTest,bIsSpaces;

    if (p_Attri) {
        Foreground = p_Attri->foreground();
        Background = p_Attri->background();
        Style = p_Attri->styles();
    } else {
        Foreground = colFG;
        Background = colBG;
        Style = getFontStyles(edit->font());
    }

    if (Background.isValid() || (edit->mActiveLineColor.isValid() && bCurrentLine)) {
        Background = colEditorBG();
    }
    if (Background.isValid()) {
        Foreground = edit->palette().color(QPalette::Text);
    }

    //todo : change char(getTokenPos) to column?
    if (edit->mOnPaintHighlightToken)
        edit->mOnPaintHighlightToken(cLine,edit->mHighlighter->getTokenPos(),
        Token,p_Attri,Style,Foreground,Background);

    // Do we have to paint the old chars first, or can we just append?
    bCanAppend = false;
    bSpacesTest = false;
    if (TokenAccu.Columns > 0) {
        // font style must be the same or token is only spaces
        if (TokenAccu.Style == Style ||  ( (Style & SynFontStyle::fsUnderline) == (TokenAccu.Style & fsUnderline)
                                           && TokenIsSpaces(bSpacesTest,Token,bIsSpaces)) ) {
            // either special colors or same colors
            if ((bSpecialLine && !(edit->mOptions.testFlag(SynEditorOption::eoSpecialLineDefaultFg))) || bLineSelected ||
              // background color must be the same and
            ((TokenAccu.BG == Background) &&
              // foreground color must be the same or token is only spaces
              ((TokenAccu.FG == Foreground) || (TokenIsSpaces(bSpacesTest,Token,bIsSpaces) && !edit->mShowSpecChar)))) {
              bCanAppend = true;
            }
        }
        // If we can't append it, then we have to paint the old token chars first.
        if (!bCanAppend)
            PaintHighlightToken(false);
    }
    // Don't use AppendStr because it's more expensive.
    if (bCanAppend) {
        TokenAccu.s.append(Token);
        TokenAccu.Columns+=TokenColumns;
    } else {
      TokenAccu.Columns = TokenColumns;
      TokenAccu.s = Token;
      TokenAccu.ColumnsBefore = ColumnsBefore;
      TokenAccu.FG = Foreground;
      TokenAccu.BG = Background;
      TokenAccu.Style = Style;
    }
}

void SynEditTextPainter::PaintFoldAttributes()
{
    int i, TabSteps, LineIndent, LastNonBlank, X, Y, cRow, vLine;
    QBrush DottedPenDesc;
    // Paint indent guides. Use folds to determine indent value of these
    // Use a separate loop so we can use a custom pen
    // Paint indent guides using custom pen
    if (edit->mCodeFolding.indentGuides) {
        QPen dottedPen(Qt::PenStyle::DashLine);
        dottedPen.setColor(edit->mCodeFolding.indentGuidesColor);

        QPen oldPen = painter->pen();
        painter->setPen(dottedPen);
        // Now loop through all the lines. The indices are valid for Lines.
        for (cRow = aFirstRow; cRow<=aLastRow;cRow++) {
            vLine = edit->rowToLine(cRow);
            if (vLine > edit->mLines->count() && edit->mLines->count() > 0)
                break;
            // Set vertical coord
            Y = (vLine - edit->mTopLine) * edit->mTextHeight; // limit inside clip rect
            if (edit->mTextHeight % 2 == 1 && vLine % 2 == 0) {
                Y++;
            }
            // Get next nonblank line
            LastNonBlank = vLine - 1;
            while (LastNonBlank + 1 < edit->mLines->count() && edit->mLines->getString(LastNonBlank).trimmed().isEmpty())
                LastNonBlank++;
            LineIndent = edit->getLineIndent(edit->mLines->getString(LastNonBlank));
            // Step horizontal coord
            TabSteps = edit->mTabWidth;
            while (TabSteps < LineIndent) {
                X = TabSteps * edit->mCharWidth + edit->mTextOffset - 2;
                TabSteps+=edit->mTabWidth;

                // Move to top of vertical line
                painter->drawLine(X,Y,X,Y+edit->mTextHeight);
            }
        }
        painter->setPen(oldPen);
    }

    if (!edit->mUseCodeFolding)
        return;

    // Paint collapsed lines using changed pen
    if (edit->mCodeFolding.showCollapsedLine) {
        painter->setPen(edit->mCodeFolding.collapsedLineColor);
        for (int i=0; i< edit->mAllFoldRanges.count();i++) {
            PSynEditFoldRange range = edit->mAllFoldRanges[i];
            if (range->collapsed && !range->parentCollapsed() &&
                    (range->fromLine <= vLastLine) && (range->fromLine >= vFirstLine) ) {
                // Get starting and end points
                Y = (edit->lineToRow(range->fromLine) - edit->mTopLine + 1) * edit->mTextHeight - 1;
                painter->drawLine(AClip.left(),Y, AClip.right(),Y);
            }
        }
    }

}

void SynEditTextPainter::GetBraceColorAttr(int level, PSynHighlighterAttribute &attr)
{
    if (!edit->mOptions.testFlag(SynEditorOption::eoShowRainbowColor))
        return;
    if (attr != edit->mHighlighter->symbolAttribute())
        return;
    switch(level % 4) {
    case 0:
        attr = edit->mHighlighter->keywordAttribute();
    case 1:
        attr = edit->mHighlighter->symbolAttribute();
    case 2:
        attr = edit->mHighlighter->stringAttribute();
    case 3:
        attr = edit->mHighlighter->identifierAttribute();
    }
}

void SynEditTextPainter::PaintLines()
{
    int cRow; // row index for the loop
    int vLine;
    QString sLine; // the current line
    QString sToken; // highlighter token info
    int nTokenStartColumn, nTokenColumnLen;
    PSynHighlighterAttribute attr;
    int vFirstChar;
    int vLastChar;
    SynEditingAreaList  areaList;
    QColor colBorder;
    PSynEditFoldRange foldRange;
    int nC1,nC2,nFold;
    QString sFold;

    // Initialize rcLine for drawing. Note that Top and Bottom are updated
    // inside the loop. Get only the starting point for this.
    rcLine = AClip;
    rcLine.setBottom((aFirstRow - edit->mTopLine) * edit->mTextHeight);
    TokenAccu.Columns = 0;
    TokenAccu.ColumnsBefore = 0;
    // Now loop through all the lines. The indices are valid for Lines.
    for (cRow = aFirstRow; cRow<=aLastRow; cRow++) {
        vLine = edit->rowToLine(cRow);
        if (vLine > edit->mLines->count() && edit->mLines->count() != 0)
            break;

        // Get the line.
        sLine = edit->mLines->getString(vLine - 1);
        // determine whether will be painted with ActiveLineColor
        bCurrentLine = (edit->mCaretY == vLine);
        // Initialize the text and background colors, maybe the line should
        // use special values for them.
        colFG = edit->palette().color(QPalette::Text);
        colBG = colEditorBG();
        bSpecialLine = edit->onSpecialLineColors(vLine, colFG, colBG);
        if (bSpecialLine) {
            // The selection colors are just swapped, like seen in Delphi.
            colSelFG = colBG;
            colSelBG = colFG;
        } else {
            colSelFG = edit->mSelectedForeground;
            colSelBG = edit->mSelectedBackground;
        }
        edit->onEditingAreas(vLine, areaList);
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
            if ((edit->mActiveSelectionMode == SynSelectionMode::smColumn) ||
                ((edit->mActiveSelectionMode == SynSelectionMode::smNormal) && (cRow == vSelStart.Row)) ) {
                if (vSelStart.Column > LastCol) {
                    nLineSelStart = 0;
                    nLineSelEnd = 0;
                } else if (vSelStart.Column > FirstCol) {
                    nLineSelStart = vSelStart.Column;
                    bComplexLine = true;
                }
            }
            if ( (edit->mActiveSelectionMode == SynSelectionMode::smColumn) ||
                ((edit->mActiveSelectionMode == SynSelectionMode::smNormal) && (cRow == vSelEnd.Row)) ) {
                if (vSelEnd.Column < FirstCol) {
                    nLineSelStart = 0;
                    nLineSelEnd = 0;
                } else if (vSelEnd.Column < LastCol) {
                    nLineSelEnd = vSelEnd.Column;
                    bComplexLine = true;
                }
            }
        } //endif bAnySelection

        // Update the rcLine rect to this line.
        rcLine.setTop(rcLine.bottom());
        rcLine.setBottom(rcLine.bottom()+edit->mTextHeight);

        bLineSelected = (!bComplexLine) && (nLineSelStart > 0);
        rcToken = rcLine;
        if (!edit->mHighlighter || !edit->mHighlighter->enabled()) {
              sToken = sLine;
              nTokenColumnLen = edit->mLines->lineColumns(vLine-1);
              if (edit->mShowSpecChar && (!bLineSelected) && (!bSpecialLine) && (nTokenColumnLen < vLastChar)) {
                  sToken = sToken + SynLineBreakGlyph;
                  nTokenColumnLen += edit->charColumns(SynLineBreakGlyph);
              }
              if (bComplexLine) {
                  setDrawingColors(false);
                  rcToken.setLeft(std::max(rcLine.left(), ColumnToXValue(FirstCol)));
                  rcToken.setRight(std::min(rcLine.right(), ColumnToXValue(nLineSelStart)));
                  PaintToken(sToken, nTokenColumnLen, 0, FirstCol, nLineSelStart,false);
                  rcToken.setLeft(std::max(rcLine.left(), ColumnToXValue(nLineSelEnd)));
                  rcToken.setRight(std::min(rcLine.right(), ColumnToXValue(LastCol)));
                  PaintToken(sToken, nTokenColumnLen, 0, nLineSelEnd, LastCol,true);
                  setDrawingColors(false);
                  rcToken.setLeft(std::max(rcLine.left(), ColumnToXValue(nLineSelStart)));
                  rcToken.setRight(std::min(rcLine.right(), ColumnToXValue(nLineSelEnd)));
                  PaintToken(sToken, nTokenColumnLen, 0, nLineSelStart, nLineSelEnd - 1,false);
              } else {
                  setDrawingColors(bLineSelected);
                  PaintToken(sToken, nTokenColumnLen, 0, FirstCol, LastCol,bLineSelected);
              }
        } else {
            // Initialize highlighter with line text and range info. It is
            // necessary because we probably did not scan to the end of the last
            // line - the internal highlighter range might be wrong.
            if (vLine == 1) {
                edit->mHighlighter->resetState();
            } else {
                edit->mHighlighter->setState(
                            edit->mLines->ranges(vLine-2),
                            edit->mLines->braceLevels(vLine-2),
                            edit->mLines->bracketLevels(vLine-2),
                            edit->mLines->parenthesisLevels(vLine-2)
                            );
                edit->mHighlighter->setLine(sLine, vLine - 1);
            }
            // Try to concatenate as many tokens as possible to minimize the count
            // of ExtTextOut calls necessary. This depends on the selection state
            // or the line having special colors. For spaces the foreground color
            // is ignored as well.
            TokenAccu.Columns = 0;
            nTokenStartColumn = 0;
            // Test first whether anything of this token is visible.
            while (edit->mHighlighter->eol()) {
                sToken = edit->mHighlighter->getToken();
                // Work-around buggy highlighters which return empty tokens.
                if (sToken.isEmpty())  {
                    edit->mHighlighter->next();
                    if (edit->mHighlighter->eol())
                        break;
                    sToken = edit->mHighlighter->getToken();
                    // Maybe should also test whether GetTokenPos changed...
                    if (sToken.isEmpty())
                        throw BaseError(SynEdit::tr("The highlighter seems to be in an infinite loop"));
                }
                nTokenStartColumn = edit->charToColumn(vLine,edit->mHighlighter->getTokenPos());
                nTokenColumnLen = edit->stringColumns(sToken, nTokenStartColumn-1);
                if (nTokenStartColumn + nTokenColumnLen >= vFirstChar) {
                    if (nTokenStartColumn + nTokenColumnLen >= vLastChar) {
                        if (nTokenStartColumn >= vLastChar)
                            break; //*** BREAK ***
                        nTokenColumnLen = vLastChar - nTokenStartColumn - 1;
                    }
                    // It's at least partially visible. Get the token attributes now.
                    attr = edit->mHighlighter->getTokenAttribute();
                    if (sToken == "[") {
                      GetBraceColorAttr(edit->mHighlighter->getBracketLevel(),attr);
                    } else if (sToken == "]") {
                      GetBraceColorAttr(edit->mHighlighter->getBracketLevel()+1,attr);
                    } else if (sToken == "(") {
                      GetBraceColorAttr(edit->mHighlighter->getParenthesisLevel(),attr);
                    } else if (sToken == ")") {
                      GetBraceColorAttr(edit->mHighlighter->getParenthesisLevel()+1,attr);
                    } else if (sToken == "{") {
                      GetBraceColorAttr(edit->mHighlighter->getBraceLevel(),attr);
                    } else if (sToken == "}") {
                      GetBraceColorAttr(edit->mHighlighter->getBraceLevel()+1,attr);
                    }
                    AddHighlightToken(sToken, nTokenStartColumn - (vFirstChar - FirstCol),
                      nTokenColumnLen, vLine,attr);
                }
                // Let the highlighter scan the next token.
                edit->mHighlighter->next();
            }
            // Don't assume HL.GetTokenPos is valid after HL.GetEOL == True.
            nTokenStartColumn += edit->stringColumns(sToken,nTokenStartColumn-1);
            if (edit->mHighlighter->eol() && (nTokenStartColumn < vLastChar)) {
                int lineColumns = edit->mLines->lineColumns(vLine-1);
                // Draw text that couldn't be parsed by the highlighter, if any.
                if (nTokenStartColumn < lineColumns) {
                    if (nTokenStartColumn + 1 < vFirstChar)
                        nTokenStartColumn = vFirstChar - 1;
                    nTokenColumnLen = std::min(lineColumns, vLastChar) - (nTokenStartColumn + 1);
                    if (nTokenColumnLen > 0) {
                        sToken = edit->substringByColumns(sLine,nTokenStartColumn+1,nTokenColumnLen);
                        AddHighlightToken(sToken, nTokenStartColumn - (vFirstChar - FirstCol),
                            nTokenColumnLen, vLine, PSynHighlighterAttribute());
                    }
                }
            // Draw LineBreak glyph.
            if (edit->mShowSpecChar && (!bLineSelected) &&
                    (!bSpecialLine) && (edit->mLines->lineColumns(vLine-1) < vLastChar)) {
                AddHighlightToken(SynLineBreakGlyph,
                  edit->mLines->lineColumns(vLine-1)  - (vFirstChar - FirstCol),
                  edit->charColumns(SynLineBreakGlyph),vLine, edit->mHighlighter->whitespaceAttribute());
            }
            }

            // Paint folding
            foldRange = edit->foldStartAtLine(vLine);
            if ((foldRange) && foldRange->collapsed) {
                sFold = " ... } ";
                nFold = edit->stringColumns(sFold,edit->mLines->lineColumns(vLine-1));
                attr = edit->mHighlighter->symbolAttribute();
                GetBraceColorAttr(edit->mHighlighter->getBraceLevel(),attr);
                AddHighlightToken(sFold,edit->mLines->lineColumns(vLine-1)+1 - (vFirstChar - FirstCol)
                  , nFold, vLine, attr);
            }

            // Draw anything that's left in the TokenAccu record. Fill to the end
            // of the invalid area with the correct colors.
            PaintHighlightToken(true);

            //Paint editingAreaBorders
            PaintEditAreas(areaList);
        }

        // Now paint the right edge if necessary. We do it line by line to reduce
        // the flicker. Should not cost very much anyway, compared to the many
        // calls to ExtTextOut.
        if (bDoRightEdge) {
            painter->drawLine(nRightEdge, rcLine.top(),nRightEdge,rcLine.bottom()+1);
        }
        bCurrentLine = false;
    }
}

void SynEditTextPainter::drawMark(PSynEditMark aMark, int &aGutterOff, int aMarkRow)
{
    //todo
}


