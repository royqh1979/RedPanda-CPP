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
#include "baseseacher.h"

namespace QSynedit {
BaseSearcher::BaseSearcher(QObject *parent) : QObject(parent)
{

}

QString BaseSearcher::pattern()
{
    return mPattern;
}

void BaseSearcher::setPattern(const QString &value)
{
    mPattern = value;
}

SearchOptions BaseSearcher::options() const
{
    return mOptions;
}

void BaseSearcher::setOptions(const SearchOptions &options)
{
    mOptions = options;
}

bool BaseSearcher::isDelimitChar(const QChar& ch) const
{
    return !(ch == '_' || ch.isLetterOrNumber());
}
}
