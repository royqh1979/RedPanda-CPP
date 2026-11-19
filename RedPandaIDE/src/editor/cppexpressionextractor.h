/*
 * Copyright (C) 2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef CPPEXPRESSIONEXTRACTOR_H
#define CPPEXPRESSIONEXTRACTOR_H

#include <QStringList>
#include <QVector>
#include <qsynedit/syntaxer/syntaxer.h>
#include <qsynedit/types.h>

namespace QSynedit {
class QSynEdit;
}

struct CppExpressionUnit {
    QSynedit::CharPos target;
    QStringList expression;
    QVector<QSynedit::CharPos> coveredIdentifiers;
};

class CppExpressionExtractor
{
public:
    CppExpressionExtractor(const QSynedit::QSynEdit *editor,
                           const QSynedit::PConstSyntaxer &syntaxer);
    QStringList extract(const QSynedit::CharPos &pos);
    QVector<CppExpressionUnit> extractLine(int line);
    static QStringList extract(const QSynedit::QSynEdit *editor,
                               const QSynedit::PConstSyntaxer &syntaxer,
                               const QSynedit::CharPos &pos);

private:
    const QSynedit::QSynEdit *mEditor;
    QSynedit::PSyntaxer mSyntaxer;
    bool mValid;
};

#endif // CPPEXPRESSIONEXTRACTOR_H
