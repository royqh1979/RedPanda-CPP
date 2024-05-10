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
#include "regexsearcher.h"

#include <QRegularExpression>

namespace QSynedit {

RegexSearcher::RegexSearcher(QObject* parent):BaseSearcher(parent)
{

}

int RegexSearcher::length(int aIndex)
{
    if (aIndex<0 || aIndex >= mResults.length())
        return -1;
    return mLengths[aIndex];
}

int RegexSearcher::result(int aIndex)
{
    if (aIndex<0 || aIndex >= mResults.length())
        return -1;
    return mResults[aIndex];
}

int RegexSearcher::resultCount()
{
    return mResults.size();
}

int RegexSearcher::findAll(const QString &text)
{
    if (pattern().isEmpty())
        return 0;
    mResults.clear();
    mLengths.clear();
    QRegularExpressionMatchIterator it = mRegex.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        if (options().testFlag(ssoWholeWord)) {
            int start = match.capturedStart();
            int end = match.capturedStart()+match.capturedLength();
            if (((start<=0) || isDelimitChar(text[start-1]))
                    &&
                    ( (end>=text.length()) || isDelimitChar(text[end]) )
                 ) {
                mLengths.append(match.capturedLength());
                mResults.append(match.capturedStart());
            }
        } else {
            mLengths.append(match.capturedLength());
            mResults.append(match.capturedStart());
        }
    }
    return mResults.size();
}

QString RegexSearcher::replace(const QString &aOccurrence, const QString &aReplacement)
{
    QString s=aOccurrence;
    return s.replace(mRegex,aReplacement);
}

void RegexSearcher::setPattern(const QString &value)
{
    BaseSearcher::setPattern(value);
    mRegex.setPattern(value);
    updateRegexOptions();
}

void RegexSearcher::setOptions(const SearchOptions &options)
{
    BaseSearcher::setOptions(options);
    updateRegexOptions();
}

void RegexSearcher::updateRegexOptions()
{
    if (options().testFlag(SearchOption::ssoMatchCase)) {
        mRegex.setPatternOptions(
                    mRegex.patternOptions() &
                    ~QRegularExpression::CaseInsensitiveOption);
    } else {
        mRegex.setPatternOptions(
                    mRegex.patternOptions() |
                    QRegularExpression::CaseInsensitiveOption);
    }
}

}
