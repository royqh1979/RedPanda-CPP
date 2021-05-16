#include "TextPainter.h"
#include "SynEdit.h"
#include "Constants.h"
#include <cmath>

SynEditTextPainter::SynEditTextPainter(SynEdit *edit)
{
    this->edit = edit;
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

void SynEditTextPainter::PaintEditAreas(PSynEditingAreaList areaList)
{
    QRect rc;
    int x1,x2;
    int offset;
    painter->setClipRect(rcLine);
    rc=rcLine;
    rc.setBottom(rc.bottom()-1);
    setDrawingColors(false);
    for (PSynEditingArea p:*areaList) {
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
void SynEditTextPainter::AddHighlightToken(const QString &Token, int ColumnsBefore, int TokenColumns, int cLine, PSynHighlighterAttribute p_Attri)
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
            LineIndent = edit->GetLineIndent(edit->mLines->getString(LastNonBlank));
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
        exit;

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
    int nTokenPos, nTokenLen;
    PSynHighlighterAttribute attr;
    int vFirstChar;
    int vLastChar;
    PSynEditingAreaList  areaList;
    QColor colBorder;
    PSynEditFoldRange foldRange;
    int nC1,nC2,nFold;
    QString sFold;

    // Initialize rcLine for drawing. Note that Top and Bottom are updated
    // inside the loop. Get only the starting point for this.
    rcLine = AClip;
    rcLine.setBottom((aFirstRow - edit->mTopLine) * edit->mTextHeight);
    // Make sure the token accumulator string doesn't get reassigned too often.
    TokenAccu.Columns = 0;
    TokenAccu.ColumnsBefore = 0;
    if (fHighlighter) then begin
      TokenAccu.MaxLen := Max(128, fCharsInWindow);
      SetLength(TokenAccu.s, TokenAccu.MaxLen);
    end;
    // Now loop through all the lines. The indices are valid for Lines.
    for cRow := aFirstRow to aLastRow do begin
      vLine := RowToLine(cRow);
      if (vLine > Lines.Count) and not (Lines.Count = 0) then
        break;

      // Get the expanded line.
      sLine := Lines.ExpandedStrings[vLine - 1];
      // determine whether will be painted with ActiveLineColor
      bCurrentLine := CaretY = vLine;
      // Initialize the text and background colors, maybe the line should
      // use special values for them.
      colFG := Font.Color;
      colBG := colEditorBG;
      bSpecialLine := DoOnSpecialLineColors(vLine, colFG, colBG);
      if bSpecialLine then begin
        // The selection colors are just swapped, like seen in Delphi.
        colSelFG := colBG;
        colSelBG := colFG;
      end else begin
        colSelFG := fSelectedColor.Foreground;
        colSelBG := fSelectedColor.Background;
        DoOnEditAreas(vLine, areaList,colBorder,areaType);
      end;

      // Removed word wrap support
      vFirstChar := FirstCol;
      vLastChar := LastCol;

      // Get the information about the line selection. Three different parts
      // are possible (unselected before, selected, unselected after), only
      // unselected or only selected means bComplexLine will be FALSE. Start
      // with no selection, compute based on the visible columns.
      bComplexLine := FALSE;
      nLineSelStart := 0;
      nLineSelEnd := 0;
      // Does the selection intersect the visible area?
      if bAnySelection and (cRow >= vSelStart.Row) and (cRow <= vSelEnd.Row) then begin
        // Default to a fully selected line. This is correct for the smLine
        // selection mode and a good start for the smNormal mode.
        nLineSelStart := FirstCol;
        nLineSelEnd := LastCol + 1;
        if (fActiveSelectionMode = smColumn) or
          ((fActiveSelectionMode = smNormal) and (cRow = vSelStart.Row)) then
          if (vSelStart.Column > LastCol) then begin
            nLineSelStart := 0;
            nLineSelEnd := 0;
          end else if (vSelStart.Column > FirstCol) then begin
            nLineSelStart := vSelStart.Column;
            bComplexLine := TRUE;
          end;
        if (fActiveSelectionMode = smColumn) or
          ((fActiveSelectionMode = smNormal) and (cRow = vSelEnd.Row)) then
          if (vSelEnd.Column < FirstCol) then begin
            nLineSelStart := 0;
            nLineSelEnd := 0;
          end else if (vSelEnd.Column < LastCol) then begin
            nLineSelEnd := vSelEnd.Column;
            bComplexLine := TRUE;
          end;
  {$IFDEF SYN_MBCSSUPPORT}
        //todo: nLineSelStart & nLineSelEnd must be buffer coordinates
        if (fActiveSelectionMode = smColumn) then
          MBCSGetSelRangeInLineWhenColumnSelectionMode(sLine, nLineSelStart,
            nLineSelEnd);
  {$ENDIF}
      end; //endif bAnySelection

      // Update the rcLine rect to this line.
      rcLine.Top := rcLine.Bottom;
      Inc(rcLine.Bottom, fTextHeight);

      bLineSelected := (not bComplexLine) and (nLineSelStart > 0);
      rcToken := rcLine;

      if not Assigned(fHighlighter) or (not fHighlighter.Enabled) then begin
        // Remove text already displayed (in previous rows)
        if (vFirstChar <> FirstCol) or (vLastChar <> LastCol) then
          sToken := Copy(sLine, vFirstChar, vLastChar - vFirstChar)
        else
          sToken := sLine;
        if fShowSpecChar and (not bLineSelected) and (not bSpecialLine) and (Length(sLine) < vLastChar) then
          sToken := sToken + SynLineBreakGlyph;
        nTokenLen := Length(sToken);
        if bComplexLine then begin
          SetDrawingColors(FALSE);
          rcToken.Left := Max(rcLine.Left, ColumnToXValue(FirstCol));
          rcToken.Right := Min(rcLine.Right, ColumnToXValue(nLineSelStart));
          PaintToken(sToken, nTokenLen, 0, FirstCol, nLineSelStart,False);
          rcToken.Left := Max(rcLine.Left, ColumnToXValue(nLineSelEnd));
          rcToken.Right := Min(rcLine.Right, ColumnToXValue(LastCol));
          PaintToken(sToken, nTokenLen, 0, nLineSelEnd, LastCol,True);
          SetDrawingColors(TRUE);
          rcToken.Left := Max(rcLine.Left, ColumnToXValue(nLineSelStart));
          rcToken.Right := Min(rcLine.Right, ColumnToXValue(nLineSelEnd));
          PaintToken(sToken, nTokenLen, 0, nLineSelStart, nLineSelEnd - 1,False);
        end else begin
          SetDrawingColors(bLineSelected);
          PaintToken(sToken, nTokenLen, 0, FirstCol, LastCol,bLineSelected);
        end;
      end else begin
        // Initialize highlighter with line text and range info. It is
        // necessary because we probably did not scan to the end of the last
        // line - the internal highlighter range might be wrong.
        if vLine = 1 then begin
          fHighlighter.ResetRange;
          fHighlighter.ResetParenthesisLevel;
          fHighlighter.ResetBracketLevel;
          fHighlighter.ResetBraceLevel;
        end else begin
          fHighlighter.SetRange(Lines.Ranges[vLine - 2]);
          fHighlighter.SetParenthesisLevel(Lines.ParenthesisLevels[vLine - 2]);
          fHighlighter.SetBracketLevel(Lines.BracketLevels[vLine - 2]);
          fHighlighter.SetBraceLevel(Lines.BraceLevels[vLine - 2]);
        end;
        fHighlighter.SetLine(sLine, vLine - 1);
        // Try to concatenate as many tokens as possible to minimize the count
        // of ExtTextOut calls necessary. This depends on the selection state
        // or the line having special colors. For spaces the foreground color
        // is ignored as well.
        TokenAccu.Len := 0;
        nTokenPos := 0;
        // Test first whether anything of this token is visible.
        while not fHighlighter.GetEol do begin
          sToken := fHighlighter.GetToken;
          // Work-around buggy highlighters which return empty tokens.
          if sToken = '' then begin
            fHighlighter.Next;
            if fHighlighter.GetEol then
              break;
            sToken := fHighlighter.GetToken;
            // Maybe should also test whether GetTokenPos changed...
            if sToken = '' then
              raise Exception.Create('The highlighter seems to be in an infinite loop');
          end;
          nTokenPos := fHighlighter.GetTokenPos;
          nTokenLen := Length(sToken);
          if nTokenPos + nTokenLen >= vFirstChar then begin
            if nTokenPos + nTokenLen >= vLastChar then begin
              if nTokenPos >= vLastChar then
                break; //*** BREAK ***

              nTokenLen := vLastChar - nTokenPos - 1;
            end;
            // It's at least partially visible. Get the token attributes now.
            attr := fHighlighter.GetTokenAttribute;
            {
            if (nTokenPos = 0) and (attr = fHighlighter.WhitespaceAttribute) then begin
              sToken := StringOfChar('.',nTokenLen);
            end;
            }
            if sToken = '[' then begin
              GetBraceColorAttr(fHighlighter.GetBracketLevel,attr);
            end else if sToken = ']' then begin
              GetBraceColorAttr(fHighlighter.GetBracketLevel+1,attr);
            end else if sToken = '(' then begin
              GetBraceColorAttr(fHighlighter.GetParenthesisLevel,attr);
            end else if sToken = ')' then begin
              GetBraceColorAttr(fHighlighter.GetParenthesisLevel+1,attr);
            end else if sToken = '{' then begin
              GetBraceColorAttr(fHighlighter.GetBraceLevel,attr);
            end else if sToken = '}' then begin
              GetBraceColorAttr(fHighlighter.GetBraceLevel+1,attr);
            end;
            AddHighlightToken(sToken, nTokenPos - (vFirstChar - FirstCol),
              nTokenLen, cRow,attr);
          end;
          // Let the highlighter scan the next token.
          fHighlighter.Next;
        end;
        // Don't assume HL.GetTokenPos is valid after HL.GetEOL = True.
        Inc(nTokenPos, Length(sToken));
        if fHighlighter.GetEol and (nTokenPos < vLastChar) then begin
          // Draw text that couldn't be parsed by the highlighter, if any.
          if nTokenPos < Length(sLine) then begin
            if nTokenPos + 1 < vFirstChar then
              nTokenPos := vFirstChar - 1;
            nTokenLen := Min(Length(sLine), vLastChar) - (nTokenPos + 1);
            if nTokenLen > 0 then begin
              sToken := Copy(sLine, nTokenPos + 1, nTokenLen);
              AddHighlightToken(sToken, nTokenPos - (vFirstChar - FirstCol),
                nTokenLen, cRow, nil);
            end;
          end;
          // Draw LineBreak glyph.
          if (eoShowSpecialChars in fOptions) and (not bLineSelected)
            and (not bSpecialLine) and (Length(sLine) < vLastChar) then begin
            AddHighlightToken(SynLineBreakGlyph,
              Length(sLine) - (vFirstChar - FirstCol),
              Length(SynLineBreakGlyph),cRow, fHighLighter.WhitespaceAttribute);
          end;
        end;

        // Paint folding
        foldRange := FoldStartAtLine(vLine);
        if assigned(foldRange) and foldRange.Collapsed then begin
          sFold := ' ... }';
          nFold := Length(sFold);
          Attr := fHighlighter.SymbolAttribute;
          GetBraceColorAttr(fHighlighter.GetBraceLevel,attr);
          AddHighlightToken(sFold,Length(sLine)+1 - (vFirstChar - FirstCol)
            , nFold, cRow, attr);
          // Compute some helper variables.
          //nC1 := Max(FirstCol, Length(sLine)+1);
          //nC2 := Min(LastCol, Length(sLine) +1 + nFold + 1);
          //SetDrawingColors(FALSE);
          //PaintToken(sFold,nFold, Length(sLine)+1,nC1, nC2);
        end;

        // Draw anything that's left in the TokenAccu record. Fill to the end
        // of the invalid area with the correct colors.
        PaintHighlightToken(TRUE);

        //Paint editingAreaBorders
        PaintEditAreas(areaList,colBorder,areaType);

      end;

      // Now paint the right edge if necessary. We do it line by line to reduce
      // the flicker. Should not cost very much anyway, compared to the many
      // calls to ExtTextOut.
      if bDoRightEdge then begin
        Canvas.MoveTo(nRightEdge, rcLine.Top);
        Canvas.LineTo(nRightEdge, rcLine.Bottom + 1);
      end;
      bCurrentLine := False;
    end; //endfor cRow
}


