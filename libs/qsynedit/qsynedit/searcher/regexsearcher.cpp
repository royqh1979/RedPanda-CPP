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

namespace QSynedit {

RegexSearcher::RegexSearcher(QObject* parent):Searcher(parent)
{

}

QVector<TextMatch> RegexSearcher::findAll(const QString &text)
{
    QVector<TextMatch> results;
    if (pattern().isEmpty())
        return results;
    QRegularExpressionMatchIterator it = mRegex.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        results.append({match.capturedStart(), match.capturedLength()});
    }
    return results;
}

QString RegexSearcher::replace(const QString &aOccurrence, const QString &aReplacement)
{
    QString s=aOccurrence;
    return s.replace(mRegex,aReplacement);
}

void RegexSearcher::setPattern(const QString &value)
{
    Searcher::setPattern(value);
    mRegex.setPattern(value);
    updateRegexOptions();
}

void RegexSearcher::setOptions(const SearchOptions &options)
{
    Searcher::setOptions(options);
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
