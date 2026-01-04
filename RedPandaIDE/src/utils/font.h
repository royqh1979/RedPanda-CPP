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
#ifndef FONT_H
#define FONT_H

#include <QString>
#include <QStringList>
#include <QLocale>

enum class UnicodeSupportLevel {
    BmpOnly = 0,
    FullCodePoint = 1,
    Contextual = 2,
    Grapheme = 3,
    Bidirectional = 4,
};

QString defaultUiFont();
QString defaultMonoFont();
QString defaultEmojiFont();
bool isCjk(const QString &locale = QLocale::system().name());
QStringList defaultCjkEditorFonts(const QString &locale);
QStringList defaultFallbackEditorFonts();
QStringList defaultEditorFonts();

#endif // FONT_H
