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

enum class SynSelectionMode {Normal, Line, Column};

struct BufferCoord {
    int ch;
    int line;
    bool operator==(const BufferCoord& coord);
    bool operator>=(const BufferCoord& coord);
    bool operator>(const BufferCoord& coord);
    bool operator<(const BufferCoord& coord);
    bool operator<=(const BufferCoord& coord);
    bool operator!=(const BufferCoord& coord);
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
