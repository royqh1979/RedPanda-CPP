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

BasicSearcher::BasicSearcher(QObject *parent):BaseSearcher(parent)
{

}

int BasicSearcher::length(int aIndex)
{
    if (aIndex<0 || aIndex >= mResults.length())
        return 0;
    return pattern().length();
}

int BasicSearcher::result(int aIndex)
{
    if (aIndex<0 || aIndex >= mResults.length())
        return -1;
    return mResults[aIndex];
}

int BasicSearcher::resultCount()
{
    return mResults.count();
}

int BasicSearcher::findAll(const QString &text)
{
    mResults.clear();
    if (pattern().isEmpty())
        return 0;
    int start=0;
    int next=-1;
    while (true) {
        if (options().testFlag(ssoMatchCase)) {
            next = text.indexOf(pattern(),start,Qt::CaseSensitive);
        } else {
            next = text.indexOf(pattern(),start,Qt::CaseInsensitive);
        }
        if (next<0) {
            break;
        }
        start = next + pattern().length();
        if (options().testFlag(ssoWholeWord)) {
            if (((next<=0) || isDelimitChar(text[next-1]))
                    &&
                    ( (start>=text.length()) || isDelimitChar(text[start]) )
                 ) {
                mResults.append(next);
            }
        } else {
            mResults.append(next);
        }

    }
    return mResults.size();
}

QString BasicSearcher::replace(const QString &, const QString &aReplacement)
{
    return aReplacement;
}

}
