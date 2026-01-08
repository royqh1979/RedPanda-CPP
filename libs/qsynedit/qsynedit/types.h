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
#ifndef QSYNEDIT_TYPES_H
#define QSYNEDIT_TYPES_H

#include <QIcon>
#include <QList>
#include <QFlags>
#include <memory>
#include <QDebug>

namespace QSynedit {

enum class SelectionMode {Normal, Column};

enum class EditCaretType {
    VerticalLine=0, HorizontalLine=1, HalfBlock=2, Block=3
};

enum class ScrollStyle {
    None, OnlyHorizontal, OnlyVertical, Both
};

enum class ProgrammingLanguage {
    DecideBySuffix,
    Composition,
    Assembly,
    GNU_Assembly,
    NetwideAssembly,
    CPP,
    GLSL,
    Makefile,
    LUA,
    XMAKE,
    Custom,
    Textfile,
    Unknown
};

struct CharPos {
    int ch; // starts from 0
    int line; // starts from 0

    CharPos(): ch{-1}, line{-1} {}
    CharPos(const CharPos &pos): ch{pos.ch}, line{pos.line} {}
    constexpr CharPos(int ch_, int line_) : ch{ch_}, line{line_} {}

    const CharPos& operator=(const CharPos &coord) {
        ch=coord.ch;
        line=coord.line;
        return *this;
    };

    bool operator==(const CharPos& coord) const;
    bool operator>=(const CharPos& coord) const;
    bool operator>(const CharPos& coord) const;
    bool operator<(const CharPos& coord) const;
    bool operator<=(const CharPos& coord) const;
    bool operator!=(const CharPos& coord) const;

    bool isValid() const { return ch>=0 && line >=0; }
};

QDataStream &operator<<(QDataStream &out, const CharPos &data);

QDataStream &operator>>(QDataStream &in, CharPos &data);

QDebug operator<<(QDebug dbg, const CharPos& data);

struct DisplayCoord {
    int x;
    int row;
};

enum FontStyle {
    fsNone = 0,
    fsBold = 0x0001,
    fsItalic = 0x0002,
    fsUnderline = 0x0004,
    fsStrikeOut = 0x0008
};

Q_DECLARE_FLAGS(FontStyles,FontStyle)

Q_DECLARE_OPERATORS_FOR_FLAGS(FontStyles)

enum class EditingAreaType {
  eatRectangleBorder,
  eatWaveUnderLine,
  eatUnderLine
};

struct EditingArea {
    int beginX;
    int endX;
    QColor color;
    EditingAreaType type;
};


using PEditingArea = std::shared_ptr<EditingArea>;
using EditingAreaList = QList<PEditingArea>;
using PEditingAreaList = std::shared_ptr<EditingAreaList>;

bool isAssemblyLanguage(ProgrammingLanguage lang);
}

Q_DECLARE_METATYPE(QSynedit::CharPos);

#endif // TYPES_H
