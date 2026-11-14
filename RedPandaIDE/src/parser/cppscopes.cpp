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

#include <QDebug>
#include <QElapsedTimer>

namespace {
struct ScopesProfile {
    bool enabled = qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE");
    qint64 insertNs = 0;
    qint64 parentLookupNs = 0;
    int insertCount = 0;
    ~ScopesProfile()
    {
        if (!enabled)
            return;
        qWarning() << "SCOPES_PROFILE insertMs" << insertNs / 1000000.0
                   << "parentLookupMs" << parentLookupNs / 1000000.0
                   << "count" << insertCount;
    }
};

ScopesProfile scopesProfile;

struct ScopesProfileScope {
    explicit ScopesProfileScope(qint64 &target) : target(target) {
        if (scopesProfile.enabled)
            timer.start();
    }
    ~ScopesProfileScope() {
        if (scopesProfile.enabled)
            target += timer.nsecsElapsed();
    }
    qint64 &target;
    QElapsedTimer timer;
};
}

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
            return scope.statement.lock();
        index = scope.parentIndex;
    }
    return nullptr;
}

std::shared_ptr<Statement> CppScopes::findScopeAtLine(int line) const
{
    if (mScopes.isEmpty() || line < 0)
        return nullptr;
    int low = 0;
    int high = mScopes.count();
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (mScopes[mid].range.start.line <= line)
            low = mid + 1;
        else
            high = mid;
    }
    int index = low - 1;
    while (index >= 0) {
        const CppScope &scope = mScopes[index];
        if (scope.range.start.line <= line && line <= scope.range.end.line)
            return scope.statement.lock();
        index = scope.parentIndex;
    }
    return nullptr;
}

void CppScopes::addScope(const SourceRange &range,
                         const std::shared_ptr<Statement> &statement, int parentIndex)
{
    ScopesProfileScope profileScope(scopesProfile.insertNs);
    ++scopesProfile.insertCount;
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

void CppScopes::addScope(const SourceRange &range,
                         const std::shared_ptr<Statement> &statement,
                         const std::shared_ptr<Statement> &parentScope)
{
    int parentIndex = -1;
    if (parentScope) {
        ScopesProfileScope profileScope(scopesProfile.parentLookupNs);
        for (int i = 0; i < mScopes.count(); ++i) {
            if (mScopes[i].statement.lock() == parentScope
                    && mScopes[i].range.contains(range.start))
                parentIndex = i;
        }
    }
    addScope(range, statement, parentIndex);
}

std::shared_ptr<Statement> CppScopes::lastScope() const
{
    if (mScopes.isEmpty())
        return nullptr;
    return mScopes.back().statement.lock();
}

void CppScopes::removeLastScope()
{
    if (!mScopes.isEmpty())
        mScopes.pop_back();
}
