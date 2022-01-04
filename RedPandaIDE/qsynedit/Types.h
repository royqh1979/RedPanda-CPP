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
#ifndef TYPES_H
#define TYPES_H

#include <QIcon>
#include <QList>
#include <QFlags>
#include <memory>

enum class SynSelectionMode {smNormal, smLine, smColumn};

struct BufferCoord {
    int Char;
    int Line;
    bool operator==(const BufferCoord& coord);
    bool operator>=(const BufferCoord& coord);
    bool operator>(const BufferCoord& coord);
    bool operator<(const BufferCoord& coord);
    bool operator<=(const BufferCoord& coord);
    bool operator!=(const BufferCoord& coord);
};

class SynEdit;
/**
 * Nomalized buffer posistion:
 * (0,0) means at the start of the file ('\0')
 * (1,count of lines+1) means at the end of the file ('\0')
 * (length of the line+1, line) means at the line break of the line ('\n')
 */

class ContentsCoord {
public:
    ContentsCoord();
    ContentsCoord(const ContentsCoord& coord);
    int ch() const;
    void setCh(int newChar);

    int line() const;
    void setLine(int newLine);
    bool atStart();
    bool atEnd();
    const SynEdit *edit() const;
    const ContentsCoord& operator=(const ContentsCoord& coord);
    const ContentsCoord& operator=(const ContentsCoord&& coord);
    bool operator==(const ContentsCoord& coord) const;
    bool operator<(const ContentsCoord& coord) const;
    bool operator<=(const ContentsCoord& coord) const;
    bool operator>(const ContentsCoord& coord) const;
    bool operator>=(const ContentsCoord& coord) const;
    size_t operator-(const ContentsCoord& coord) const;
    const ContentsCoord& operator+=(int delta);
    const ContentsCoord& operator-=(int delta);
    ContentsCoord operator+(int delta) const;
    ContentsCoord operator-(int delta) const;
    BufferCoord toBufferCoord() const;
    QChar operator*() const;
private:
    ContentsCoord(const SynEdit* edit, int ch, int line);
    void normalize();
private:
    int mChar;
    int mLine;
    const SynEdit* mEdit;
    friend class SynEdit;
};

struct DisplayCoord {
    int Column;
    int Row;
};

enum SynFontStyle {
    fsNone = 0,
    fsBold = 0x0001,
    fsItalic = 0x0002,
    fsUnderline = 0x0004,
    fsStrikeOut = 0x0008
};

Q_DECLARE_FLAGS(SynFontStyles,SynFontStyle)

Q_DECLARE_OPERATORS_FOR_FLAGS(SynFontStyles)

using PSynIcon = std::shared_ptr<QIcon>;
using SynIconList = QList<PSynIcon>;
using PSynIconList = std::shared_ptr<SynIconList>;

enum class SynEditingAreaType {
  eatRectangleBorder,
  eatWaveUnderLine,
  eatUnderLine
};

struct SynEditingArea {
    int beginX;
    int endX;
    QColor color;
    SynEditingAreaType type;
};


using PSynEditingArea = std::shared_ptr<SynEditingArea>;
using SynEditingAreaList = QList<PSynEditingArea>;
using PSynEditingAreaList = std::shared_ptr<SynEditingAreaList>;


#endif // TYPES_H
