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
#include "SearchRegex.h"

#include <QRegExp>

SynSearchRegex::SynSearchRegex(QObject* parent):SynSearchBase(parent)
{

}

int SynSearchRegex::length(int aIndex)
{
    if (aIndex<0 || aIndex >= mResults.length())
        return -1;
    return mLengths[aIndex];
}

int SynSearchRegex::result(int aIndex)
{
    if (aIndex<0 || aIndex >= mResults.length())
        return -1;
    return mResults[aIndex];
}

int SynSearchRegex::resultCount()
{
    return mResults.size();
}

int SynSearchRegex::findAll(const QString &text)
{
    if (pattern().isEmpty())
        return 0;
    mResults.clear();
    mLengths.clear();
    QRegularExpressionMatchIterator it = mRegex.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        mLengths.append(match.capturedLength());
        mResults.append(match.capturedStart());
    }
    return mResults.size();
}

QString SynSearchRegex::replace(const QString &aOccurrence, const QString &aReplacement)
{
    QString s=aOccurrence;
    return s.replace(mRegex,aReplacement);
}

void SynSearchRegex::setPattern(const QString &value)
{
    SynSearchBase::setPattern(value);
    mRegex.setPattern(value);
    updateRegexOptions();
}

void SynSearchRegex::setOptions(const SynSearchOptions &options)
{
    SynSearchBase::setOptions(options);
    updateRegexOptions();
}

void SynSearchRegex::updateRegexOptions()
{
    if (options().testFlag(SynSearchOption::ssoMatchCase)) {
        mRegex.setPatternOptions(
                    mRegex.patternOptions() &
                    ~QRegularExpression::CaseInsensitiveOption);
    } else {
        mRegex.setPatternOptions(
                    mRegex.patternOptions() |
                    QRegularExpression::CaseInsensitiveOption);
    }
}
