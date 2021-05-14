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
        QColor result = edit->mColor;
        if (edit->mHighlighter) {
            PSynHighlighterAttribute attr = edit->mHighlighter->whitespaceAttribute();
            if (attr && attr->background().isValid()) {
                return attr->background();
            }
        }
    }
    return edit->mColor;
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

void SynEditTextPainter::PaintToken(const QString &Token, int TokenLen, int CharsBefore, int First, int Last, bool isSelection)
{
    bool startPaint;
    int nX;

    if (Last >= First && rcToken.right() > rcToken.left()) {
        nX = ColumnToXValue(First);
        First -= CharsBefore;
        Last -= CharsBefore;
        if (First > TokenLen) {
        } else {
          painter->setClipRect(rcToken);
          int tokenColLen=0;
          startPaint = false;
          for (int i=0;i<Token.length();i++) {
              int charCols;
              if (Token[i] == SynTabChar) {
                  charCols = edit->mTabWidth - ((CharsBefore+tokenColLen) % edit->mTabWidth);
              } else {
                  charCols = std::ceil(edit->fontMetrics().horizontalAdvance(Token[i]) * edit->fontMetrics().fontDpi() / 96.0 / edit->mCharWidth);
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

