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
#include "cppscopes.h"

std::shared_ptr<Statement> CppScopes::findScopeAt(const SourcePosition &position) const
{
    if (mScopes.isEmpty() || !position.isValid())
        return nullptr;

    int low = 0;
    int high = mScopes.count();
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (mScopes[mid].range.start <= position)
            low = mid + 1;
        else
            high = mid;
    }

    int index = low - 1;
    while (index >= 0) {
        const CppScope &scope = mScopes[index];
        if (scope.range.contains(position))
            return scope.statement;
        index = scope.parentIndex;
    }
    return nullptr;
}

void CppScopes::addScope(const SourceRange &range,
                         const std::shared_ptr<Statement> &statement, int parentIndex)
{
    int index = 0;
    while (index < mScopes.count() && mScopes[index].range.start <= range.start)
        ++index;

    mScopes.insert(index, CppScope{range, statement, parentIndex});
    for (int i = 0; i < mScopes.count(); ++i) {
        if (i != index && mScopes[i].parentIndex >= index)
            ++mScopes[i].parentIndex;
    }
    if (mScopes[index].parentIndex >= index)
        ++mScopes[index].parentIndex;
}

std::shared_ptr<Statement> CppScopes::lastScope() const
{
    if (mScopes.isEmpty())
        return nullptr;
    return mScopes.back().statement;
}

void CppScopes::removeLastScope()
{
    if (!mScopes.isEmpty())
        mScopes.pop_back();
}
