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
#include "types.h"
#include "qsynedit.h"
#include <QDebug>

namespace QSynedit {

bool CharPos::operator==(const CharPos &coord) const
{
    return coord.ch == ch && coord.line == line;
}

bool CharPos::operator>=(const CharPos &coord) const
{
    return (line > coord.line)
            || (line == coord.line && ch >= coord.ch);
}

bool CharPos::operator>(const CharPos &coord) const
{
    return (line > coord.line)
            || (line == coord.line && ch > coord.ch);
}

bool CharPos::operator<(const CharPos &coord) const
{
    return (line < coord.line)
            || (line == coord.line && ch < coord.ch);
}

bool CharPos::operator<=(const CharPos &coord) const
{
    return (line < coord.line)
            || (line == coord.line && ch <= coord.ch);
}

bool CharPos::operator!=(const CharPos &coord) const
{
    return coord.ch != ch || coord.line != line;
}

bool isAssemblyLanguage(ProgrammingLanguage lang)
{
    return lang == ProgrammingLanguage::Assembly
            || lang == ProgrammingLanguage::GNU_Assembly
            || lang == ProgrammingLanguage::NetwideAssembly;
}

QDataStream &operator<<(QDataStream &out, const CharPos &data)
{
    out<<data.ch;
    out<<data.line;
    return out;
}

QDataStream &operator>>(QDataStream &in, CharPos &data)
{
    in>>data.ch;
    in>>data.line;
    return in;
}

QDebug operator<<(QDebug dbg, const CharPos& data){
    dbg.space() << "CharPos(ch=" << data.ch << ", line=" << data.line << ")";
    return dbg;
}

}
