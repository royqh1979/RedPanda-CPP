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
#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <QSet>
#include <QChar>

namespace  QSynedit {

extern const QSet<QChar> WordBreakChars;
extern const QString SpaceGlyph;
extern const QString TabGlyph;
extern const QString LineBreakGlyph;
extern const QString SoftBreakGlyph;

// names for token attributes
#define SYNS_AttrAssembler          "Assembler"
#define SYNS_AttrCharacter          "Character"
#define SYNS_AttrClass              "Class"
#define SYNS_AttrComment            "Comment"
#define SYNS_AttrFloat                  "Float"
#define SYNS_AttrFunction               "Function"
#define SYNS_AttrGlobalVariable         "Global variable"
#define SYNS_AttrHexadecimal            "Hexadecimal"
#define SYNS_AttrIdentifier             "Identifier"
#define SYNS_AttrReserveWord_Type             "Reserve Word for Types"
#define SYNS_AttrIllegalChar            "Illegal Char"
#define SYNS_AttrLocalVariable "Local Variable"
#define SYNS_AttrMarker "Marker"
#define SYNS_AttrNumber "Number"
#define SYNS_AttrOctal "Octal"
#define SYNS_AttrPreprocessor "Preprocessor"
#define SYNS_AttrReservedWord "Reserved Word"
#define SYNS_AttrString "String"
#define SYNS_AttrStringEscapeSequences "Escape sequences"
#define SYNS_AttrSymbol "Symbol"
#define SYNS_AttrVariable "Variable"
#define SYNS_AttrSpace "Space"
#define SYNS_AttrText "Text"


// names of exporter output formats
#define SYNS_ExporterFormatHTML "HTML"
#define SYNS_ExporterFormatRTF "RTF"
#define SYNS_ExporterFormatTeX "TeX"

}


#endif // CONSTANTS_H
