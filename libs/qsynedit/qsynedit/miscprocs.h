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
#ifndef MISCPROCS_H
#define MISCPROCS_H
#include <QPoint>
#include <vector>
#include <memory>
#include <QString>
#include <QSet>
#include "syntaxer/syntaxer.h"
#include <QPaintDevice>
#include <QTextStream>
#include <QVector>
#include <initializer_list>
#include <functional>
//#include <QRect>
//#include <QColor>

class QPainter;
class QRect;
class QColor;

namespace QSynedit {

int minMax(int x, int mi, int ma);
BufferCoord maxBufferCoord(const BufferCoord& P1, const BufferCoord& P2);
BufferCoord minBufferCoord(const BufferCoord& P1, const BufferCoord& P2);

int getEOL(const QString& Line, int start);

QStringList splitStrings(const QString& text);

int calSpanLines(const BufferCoord& startPos, const BufferCoord& endPos);

using  TokenAttributeProc = std::function<bool(PSyntaxer syntaxer,
    PTokenAttribute attri, const QString& uniqueAttriName,
    QList<void *> params)>;

// Enums all child syntaxers and their attributes of a TSynMultiSyn through a
// callback function.
// This function also handles nested TSynMultiSyns including their MarkerAttri.
bool enumTokenAttributes(PSyntaxer syntaxer,
                           bool skipDuplicates, TokenAttributeProc tokenAttriProc,
                           std::initializer_list<void *> Params);

FontStyles getFontStyles(const QFont& font);

/**
 * Find the first occurency of word char in s, starting from startPos
 * Note: the index of first char in s in 1
 * @return index of the char founded , 0 if not found
 */
int findWordChar(const QString& s, int startPos);

/**
 * Find the first occurency of non word char in s, starting from startPos
 * Note: the index of first char in s in 1
 * @return index of the char founded , 0 if not found
 */
int findNonWordChar(const QString& s, int startPos);

/**
 * Find the first occurency of word char in s right to left, starting from startPos
 * Note: the index of first char in s in 1
 * @return index of the char founded , 0 if not found
 */
int findLastWordChar(const QString& s, int startPos);

/**
 * Find the first occurency of non word char in s  right to left, starting from startPos
 * Note: the index of first char in s in 1
 * @return index of the char founded , 0 if not found
 */
int findLastNonWordChar(const QString& s, int startPos);

void ensureNotAfter(BufferCoord& cord1, BufferCoord& cord2);

bool isWordChar(const QChar& ch);
}
#endif // MISCPROCS_H
