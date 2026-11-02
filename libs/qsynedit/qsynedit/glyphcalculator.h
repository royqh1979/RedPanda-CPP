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
#ifndef QSYNEDIT_GLYPHCALCULATOR_H
#define QSYNEDIT_GLYPHCALCULATOR_H

#include <QFont>
#include <QFontMetrics>
#include <QList>
#include <QString>

namespace QSynedit {

// glyph segmentation utilities (font-independent):
// a glyph may be represented by more than one code points (chars);
// the "segment list" below is a list of segment start values.
int searchForSegmentIdx(const QList<int> &segList, int maxVal, int value);

//calc length of each segments
int calcSegmentInterval(const QList<int> &segList, int maxVal, int idx);
int segmentIntervalStart(const QList<int> &segList, int maxVal, int idx);
QList<int> calcGlyphStartCharList(const QString &text);
void expandGlyphStartCharList(const QString& strAdded, int oldStrLen, QList<int> &glyphStartCharList);

/**
 * @brief The GlyphCalculator class
 *
 * Font-dependent glyph geometry: computes pixel positions/widths of the
 * glyphs in a text, using a specific font. This is a per-view concern —
 * views sharing a document may use different fonts, so the document never
 * holds an instance of it.
 */
class GlyphCalculator {
public:
    explicit GlyphCalculator(const QFont& font);

    int tabSize() const {
        return mTabSize;
    }
    void setTabSize(int newTabSize) { mTabSize = newTabSize; }


    int tabWidth() const {
        return mTabSize * spaceWidth();
    }

    int spaceWidth() const {
        if (mForceMonospace)
            return mCharWidth;
        return mSpaceWidth;
    }

    bool forceMonospace() const { return mForceMonospace; }

    void setForceMonospace(bool newForceMonospace) { mForceMonospace = newForceMonospace; }

    const QFontMetrics &fontMetrics() const { return mFontMetrics; }

    void setFont(const QFont &newFont);

    int glyphWidth(const QString& glyph, int left,
                   const QFontMetrics &fontMetrics,
                   bool forceMonospace) const;

    int glyphWidth(const QString &glyph, int left) const{
        return glyphWidth(glyph,left,mFontMetrics,mForceMonospace);
    }

    QList<int> calcGlyphPositionList(const QString& lineText, const QList<int> &glyphStartCharList,
                                     const QFontMetrics &fontMetrics,
                                     int left, int &right) const;

    QList<int> calcGlyphPositionList(const QString& lineText, const QList<int> &glyphStartCharList, int left, int &right) const {
        return calcGlyphPositionList(lineText, glyphStartCharList,
                                     mFontMetrics,
                                     left,right);
    }

    /**
     * @brief calculate display width of a string
     *
     * The string may contains the tab char, whose width depends on the tab size and it's position
     *
     * @param str the string to be displayed
     * @param left start x pos of the string
     * @return width of the string, don't including colsBefore
     */
    int stringWidth(const QString &str, int left) const;

    int stringWidth(const QString &str, int left, const QFontMetrics &fontMetrics) const;

    QList<int> calcLineWidth(const QString& lineText, const QList<int> &glyphStartCharList, int &width) const {
        return calcGlyphPositionList(lineText,glyphStartCharList,0,width);
    }
    QList<int> calcGlyphPositionList(const QString& lineText, int &width) const;

    int updateGlyphStartPositionList(
            const QString& lineText,
            const QList<int> &glyphStartCharList,
            int startChar, int endChar,
            const QFontMetrics &fontMetrics,
            QList<int> &glyphStartXList,
            int left, int &right, int &startGlyph, int &endGlyph) const;
private:
    QFontMetrics mFontMetrics;
    int mTabSize;
    int mCharWidth;
    int mSpaceWidth;
    bool mForceMonospace;
};

}

#endif // QSYNEDIT_GLYPHCALCULATOR_H
