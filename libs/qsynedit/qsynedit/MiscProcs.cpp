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
#include "MiscProcs.h"
#include <QFile>
#include <QPainter>
#include <QTextStream>
#include <algorithm>
#include <QApplication>
#include <QScreen>

namespace QSynedit {
int minMax(int x, int mi, int ma)
{
    x = std::min(x, ma );
    return std::max( x, mi );
}

int getEOL(const QString &Line, int start)
{
    if (start<0 || start>=Line.size()) {
        return start;
    }
    for (int i=start;i<Line.size();i++) {
        if (Line[i] == '\n' || Line[i] == '\r') {
            return i;
        }
    }
    return Line.size();
}

bool internalEnumHighlighterAttris(PHighlighter Highlighter,
                                   bool SkipDuplicates,
                                   HighlighterAttriProc highlighterAttriProc,
                                   std::initializer_list<void *>& Params,
                                   HighlighterList& HighlighterList) {
    bool Result = true;
    if (HighlighterList.indexOf(Highlighter)>0) {
        if (SkipDuplicates)
            return Result;
    } else {
        HighlighterList.append(Highlighter);
    }
    if (Highlighter) {
        for (PHighlighterAttribute pAttr: Highlighter->attributes()){
            QString UniqueAttriName = Highlighter->languageName()
                    +  QString("%1").arg(HighlighterList.indexOf(Highlighter)) + '.'
                    + pAttr->name();
            Result = highlighterAttriProc(Highlighter, pAttr,
                                          UniqueAttriName, Params);
            if (!Result)
                break;
        }
    }
    return Result;
}

bool enumHighlighterAttributes(PHighlighter Highlighter, bool SkipDuplicates,
                           HighlighterAttriProc highlighterAttriProc,
                           std::initializer_list<void *> Params)
{
    if (!Highlighter || !highlighterAttriProc) {
        return false;
    }

    HighlighterList HighlighterList;
    return internalEnumHighlighterAttris(Highlighter, SkipDuplicates,
        highlighterAttriProc, Params, HighlighterList);
}

int mulDiv(int a, int b, int c)
{
    //todo: handle overflow?
    return a*b/c;
}

FontStyles getFontStyles(const QFont &font)
{
    FontStyles styles;
    styles.setFlag(FontStyle::fsBold, font.bold());
    styles.setFlag(FontStyle::fsItalic, font.italic());
    styles.setFlag(FontStyle::fsUnderline, font.underline());
    styles.setFlag(FontStyle::fsStrikeOut, font.strikeOut());
    return styles;
}

bool isWordChar(const QChar& ch) {
    return (ch == '_') || ch.isLetterOrNumber();
}

int findWordChar(const QString &s, int startPos)
{
    for (int i=startPos-1;i<s.length();i++) {
        if (isWordChar(s[i])) {
            return i+1;
        }
    }
    return 0;
}

int findNonWordChar(const QString &s, int startPos)
{
    for (int i=startPos-1;i<s.length();i++) {
        if (!isWordChar(s[i])) {
            return i+1;
        }
    }
    return 0;
}

int findLastWordChar(const QString &s, int startPos)
{
    int i = startPos-1;
    while (i>=0) {
        if (isWordChar(s[i]))
            return i+1;
        i--;
    }
    return 0;
}

int findLastNonWordChar(const QString &s, int startPos)
{
    int i = startPos-1;
    while (i>=0) {
        if (!isWordChar(s[i]))
            return i+1;
        i--;
    }
    return 0;
}

void ensureNotAfter(BufferCoord &cord1, BufferCoord &cord2)
{
    if((cord1.line > cord2.line) || (
                cord1.line == cord2.line &&
                cord1.ch > cord2.ch)) {
        std::swap(cord1,cord2);
    }
}

BufferCoord minBufferCoord(const BufferCoord &P1, const BufferCoord &P2)
{
    if ( (P2.line < P1.line) || ( (P2.line == P1.line) && (P2.ch < P1.ch)) ) {
      return P2;
    } else {
      return P1;
    }
}

BufferCoord maxBufferCoord(const BufferCoord &P1, const BufferCoord &P2)
{
    if ( (P2.line > P1.line) || ( (P2.line == P1.line) && (P2.ch > P1.ch)) ) {
      return P2;
    } else {
      return P1;
    }
}

QStringList splitStrings(const QString &text)
{
    QStringList list;
    int start=0,i=0;
    while(i<text.length()) {
        if (text[i]=='\n' || text[i]=='\r') {
            list.append(text.mid(start,i-start));
            if (text[i]=='\r') {
                i++;
                if (i<text.length() && text[i]=='\n')
                    i++;
            } else {
                i++;
            }
            start=i;
        } else {
            i++;
        }
    }
    if (i>=start)
        list.append(text.mid(start,i));
    return list;
}

int calSpanLines(const BufferCoord &startPos, const BufferCoord &endPos)
{
    return std::abs(endPos.line - startPos.line+1);
}

}
