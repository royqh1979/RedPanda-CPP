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
#include "basicsearcher.h"

namespace QSynedit {

BasicSearcher::BasicSearcher(QObject *parent):Searcher(parent)
{

}

QVector<TextMatch> BasicSearcher::findAll(const QString &text)
{
    QVector<TextMatch> results;
    if (pattern().isEmpty())
        return results;
    int start = 0;
    Qt::CaseSensitivity sensitivity = options().testFlag(ssoMatchCase)
            ? Qt::CaseSensitive : Qt::CaseInsensitive;
    while (true) {
        int next = text.indexOf(pattern(), start, sensitivity);
        if (next < 0)
            break;
        results.append({next, pattern().length()});
        start = next + pattern().length();
    }
    return results;
}

QString BasicSearcher::replace(const QString &, const QString &aReplacement)
{
    return aReplacement;
}

}
