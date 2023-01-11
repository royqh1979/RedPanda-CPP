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
#ifndef SYNSEARCHREGEX_H
#define SYNSEARCHREGEX_H
#include "baseseacher.h"

#include <QRegularExpression>

namespace QSynedit {
class RegexSearcher : public BaseSearcher
{
    Q_OBJECT
public:
    explicit RegexSearcher(QObject* parent=nullptr);

    // SynSearchBase interface
public:
    int length(int aIndex) override;
    int result(int aIndex) override;
    int resultCount() override;
    int findAll(const QString &text) override;
    QString replace(const QString &aOccurrence, const QString &aReplacement) override;
    void setPattern(const QString &value) override;
    void setOptions(const SearchOptions &options) override;
private:
    void updateRegexOptions();
private:
    QRegularExpression mRegex;
    QList<int> mLengths;
    QList<int> mResults;
};

}

#endif // SYNSEARCHREGEX_H
