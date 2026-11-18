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
#include "glyphcalculator.h"
#include <cmath>

namespace QSynedit {

int searchForSegmentIdx(const QList<int> &segList, int maxVal, int value)
{
    if (value<0)
        return 0;
    if (value>=maxVal)
        return segList.length();
    int start = 0;
    int end = segList.length()-1;
    while ( start<=end ) {
        int mid = (start+end) / 2;
        if (segList[mid]==value) {
            //skip zero length segs
            while(mid+1<segList.length() && segList[mid+1]==value)
                mid++;
            return mid;
        } else if (segList[mid]>value) {
            end = mid-1;
        } else if (mid+1>=segList.length() || segList[mid+1]>value ) {
            return mid;
        } else {
            start = mid+1;
        }
    }
    //Not found, should not happen
    Q_ASSERT(false);
    return -1;
}

int calcSegmentInterval(const QList<int> &segList, int maxVal, int idx)
{
    if (idx<0 || idx>=segList.length())
        return 0;
    if (idx == segList.length()-1)
        return maxVal - segList[idx];
    return segList[idx+1]-segList[idx];
}

int segmentIntervalStart(const QList<int> &segList, int maxVal, int idx)
{
    if (idx<0)
        return 0;
    if (idx>=segList.length())
        return maxVal;
    return segList[idx];
}

QList<int> calcGlyphStartCharList(const QString &text)
{
    QList<int> glyphStartCharList;
    //parse mGlyphs
    int i=0;
    bool consecutive = false;
    while (i<text.length()) {
        QChar ch = text[i];
        if (ch.isHighSurrogate() && i+1<text.length() && QChar::isLowSurrogate(text[i+1].unicode())) {
            //character that larger than 0xffff
            uint ucs4 = QChar::surrogateToUcs4(ch, text[i+1]);
            if (QChar::combiningClass(ucs4)!=0 && !glyphStartCharList.isEmpty()) {
                //a Combining character
            } else if (ucs4>=0xE0100 && ucs4 <= 0xE01EF) {
                //variation selector
            } else if (ucs4>=0x1F3FB && ucs4 <= 0x1F3FF) {
                //Emoji modifiers for skin tone
            } else if (!consecutive) {
                glyphStartCharList.append(i);
                if (ucs4>=0x1F1E6 && ucs4<=0x1F1FF) // National Flags emoji
                    consecutive = true;
                else
                    consecutive = false;
            } else {
                consecutive = false;
            }
            i+=2;
            continue;
        } else if (ch.unicode() == 0x200D || ch.unicode() == 0x200C ) {
            //ZWJ && ZWNJ
            consecutive = true;
        } else if (ch.combiningClass()!=0 && !glyphStartCharList.isEmpty()) {
            //a Combining character
        } else if (ch.unicode()>=0xFE00 && ch.unicode()<=0xFE0F) {
            //variation selector
        } else {
            //qDebug("%x %d", ch.unicode(), ch.combiningClass());
            if (!consecutive)
                glyphStartCharList.append(i);
            consecutive = false;
        }
        i++;
    }
    //qDebug()<<text<<glyphStartCharList;
    if (!text.isEmpty() && glyphStartCharList.isEmpty())
        glyphStartCharList.append(0);
    return glyphStartCharList;
}

void expandGlyphStartCharList(const QString &strAdded, int oldStrLen, QList<int> &glyphStartCharList)
{
    QList<int> addedList = calcGlyphStartCharList(strAdded);
    for (int i=0;i<addedList.length();i++) {
        glyphStartCharList.append(addedList[i]+oldStrLen);
    }
}

namespace {

uint codePointAt(const QString &text, int index)
{
    QChar ch = text[index];
    if (ch.isHighSurrogate() && index + 1 < text.length()
            && text[index + 1].isLowSurrogate()) {
        return QChar::surrogateToUcs4(ch, text[index + 1]);
    }
    return ch.unicode();
}

bool isWideCodePoint(uint ucs4)
{
    if ((ucs4 >= 0x1100 && ucs4 <= 0x115F)
            || (ucs4 >= 0x2329 && ucs4 <= 0x232A)
            || (ucs4 >= 0x2E80 && ucs4 <= 0xA4CF && ucs4 != 0x303F)
            || (ucs4 >= 0xAC00 && ucs4 <= 0xD7A3)
            || (ucs4 >= 0xF900 && ucs4 <= 0xFAFF)
            || (ucs4 >= 0xFE10 && ucs4 <= 0xFE19)
            || (ucs4 >= 0xFE30 && ucs4 <= 0xFE6F)
            || (ucs4 >= 0xFF00 && ucs4 <= 0xFF60)
            || (ucs4 >= 0xFFE0 && ucs4 <= 0xFFE6)
            || (ucs4 >= 0x1F000 && ucs4 <= 0x1FAFF)
            || (ucs4 >= 0x20000 && ucs4 <= 0x3FFFD)) {
        return true;
    }
    return (ucs4 >= 0x231A && ucs4 <= 0x231B)
            || (ucs4 >= 0x23E9 && ucs4 <= 0x23EC)
            || ucs4 == 0x23F0 || ucs4 == 0x23F3
            || (ucs4 >= 0x25FD && ucs4 <= 0x25FE)
            || (ucs4 >= 0x2614 && ucs4 <= 0x2615)
            || (ucs4 >= 0x2648 && ucs4 <= 0x2653)
            || ucs4 == 0x267F || ucs4 == 0x2693 || ucs4 == 0x26A1
            || (ucs4 >= 0x26AA && ucs4 <= 0x26AB)
            || (ucs4 >= 0x26BD && ucs4 <= 0x26BE)
            || (ucs4 >= 0x26C4 && ucs4 <= 0x26C5)
            || ucs4 == 0x26CE || ucs4 == 0x26D4 || ucs4 == 0x26EA
            || (ucs4 >= 0x26F2 && ucs4 <= 0x26F3)
            || ucs4 == 0x26F5 || ucs4 == 0x26FA || ucs4 == 0x26FD
            || ucs4 == 0x2705 || (ucs4 >= 0x270A && ucs4 <= 0x270B)
            || ucs4 == 0x2728 || ucs4 == 0x274C || ucs4 == 0x274E
            || (ucs4 >= 0x2753 && ucs4 <= 0x2755) || ucs4 == 0x2757
            || (ucs4 >= 0x2795 && ucs4 <= 0x2797)
            || ucs4 == 0x27B0 || ucs4 == 0x27BF
            || (ucs4 >= 0x2B1B && ucs4 <= 0x2B1C)
            || ucs4 == 0x2B50 || ucs4 == 0x2B55;
}

}

int glyphLogicalWidth(const QString &glyph)
{
    if (glyph.isEmpty())
        return 0;
    uint ucs4 = codePointAt(glyph, 0);
    if (QChar::combiningClass(ucs4) != 0 || ucs4 == 0x200C || ucs4 == 0x200D
            || (ucs4 >= 0xFE00 && ucs4 <= 0xFE0F)
            || (ucs4 >= 0xE0100 && ucs4 <= 0xE01EF)) {
        return 0;
    }
    for (int i = 0; i < glyph.length(); ++i) {
        uint codePoint = codePointAt(glyph, i);
        if (codePoint == 0xFE0F || codePoint == 0x20E3)
            return 2;
    }
    return isWideCodePoint(ucs4) ? 2 : 1;
}

int calcLogicalColumn(const QString &lineText, int charPos, int tabSize)
{
    Q_ASSERT(tabSize > 0);
    if (tabSize <= 0)
        return 0;
    QList<int> glyphStartCharList = calcGlyphStartCharList(lineText);
    int column = 0;
    for (int i = 0; i < glyphStartCharList.length(); ++i) {
        int start = glyphStartCharList[i];
        if (start >= charPos)
            break;
        int end = i + 1 < glyphStartCharList.length()
                ? glyphStartCharList[i + 1] : lineText.length();
        QString glyph = lineText.mid(start, end - start);
        if (glyph == "\t")
            column += tabSize - column % tabSize;
        else
            column += glyphLogicalWidth(glyph);
    }
    return column;
}

GlyphCalculator::GlyphCalculator(const QFont &font):
    mFontMetrics{font},
    mTabSize{4},
    mForceMonospace{false}
{
    mCharWidth =  mFontMetrics.horizontalAdvance("M");
    mSpaceWidth = mFontMetrics.horizontalAdvance(" ");
}

void GlyphCalculator::setFont(const QFont &newFont)
{
    mFontMetrics = QFontMetrics(newFont);
    mCharWidth =  mFontMetrics.horizontalAdvance("M");
    mSpaceWidth = mFontMetrics.horizontalAdvance(" ");
}

int GlyphCalculator::glyphWidth(const QString &glyph, int left, const QFontMetrics &fontMetrics, bool forceMonospace) const
{
    int glyphWidth;
    if (glyph.length()==0)
        return 0;
    QChar ch = glyph[0];
    if (ch == '\t') {
        glyphWidth = tabWidth() - left % tabWidth();
    } else {
        glyphWidth = fontMetrics.horizontalAdvance(glyph);
        //qDebug()<<glyph<<glyphCols<<width<<mCharWidth;
    }
    if (forceMonospace) {
        int cols = std::ceil(glyphWidth / (double)mCharWidth);
        glyphWidth = cols * mCharWidth;
    }
    return glyphWidth;
}

int GlyphCalculator::stringWidth(const QString &str, int left) const
{
    QList<int> glyphStartCharList = calcGlyphStartCharList(str);
    int right;
    calcGlyphPositionList(str, glyphStartCharList, left, right);
    return right - left;
}

int GlyphCalculator::stringWidth(const QString &str, int left, const QFontMetrics &fontMetrics) const
{
    QList<int> glyphStartCharList = calcGlyphStartCharList(str);
    int right;
    calcGlyphPositionList(str, glyphStartCharList, fontMetrics, left, right);
    return right - left;
}

QList<int> GlyphCalculator::calcGlyphPositionList(const QString &lineText, const QList<int> &glyphStartCharList, const QFontMetrics &fontMetrics, int left, int &right) const
{
    right = std::max(0,left);
    int start,end;
    QList<int> glyphPostionList;
    for (int i=0;i<glyphStartCharList.length();i++) {
        start = glyphStartCharList[i];
        if (i+1<glyphStartCharList.length()) {
            end = glyphStartCharList[i+1];
        } else {
            end = lineText.length();
        }
        QString glyph = lineText.mid(start,end-start);
        int gWidth = glyphWidth(glyph, right, fontMetrics, mForceMonospace);
        glyphPostionList.append(right);
        right += gWidth;
    }
    return glyphPostionList;
}

QList<int> GlyphCalculator::calcGlyphPositionList(const QString &lineText, int &width) const
{
    QList<int> glyphStartCharList = calcGlyphStartCharList(lineText);
    return calcGlyphPositionList(lineText,glyphStartCharList,0,width);
}

int GlyphCalculator::updateGlyphStartPositionList(
        const QString &lineText,
        const QList<int> &glyphStartCharList, int startChar, int endChar,
        const QFontMetrics &fontMetrics,
        QList<int> &glyphStartXList, int left, int &right, int &startGlyph, int &endGlyph) const
{
    right = std::max(0,left);
    startGlyph = searchForSegmentIdx(glyphStartCharList,lineText.length(),startChar);
    endGlyph = searchForSegmentIdx(glyphStartCharList,lineText.length(),endChar);
    for (int i=startGlyph;i<endGlyph;i++) {
        int start = glyphStartCharList[i];
        int end;
        if (i+1<glyphStartCharList.length()) {
            end = glyphStartCharList[i+1];
        } else {
            end = lineText.length();
        }
        QString glyph = lineText.mid(start,end-start);
        int gWidth = glyphWidth(glyph, right, fontMetrics, mForceMonospace);
        glyphStartXList[i] = right;
        right += gWidth;
    }
    if (endGlyph<glyphStartXList.length())
        glyphStartXList[endGlyph] = right;
    return right-left;
}

}
