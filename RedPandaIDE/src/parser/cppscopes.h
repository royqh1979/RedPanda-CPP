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
#ifndef CPPSCOPES_H
#define CPPSCOPES_H

#include "cpptypes.h"

struct CppScope {
    SourceRange range;
    std::weak_ptr<Statement> statement;
    int parentIndex = -1;
};

class CppScopes
{
public:
    std::shared_ptr<Statement> findScopeAt(const SourcePosition &position) const;
    void addScope(const SourceRange &range, const std::shared_ptr<Statement> &statement,
                  int parentIndex = -1);
    void addScope(const SourceRange &range, const std::shared_ptr<Statement> &statement,
                  const std::shared_ptr<Statement> &parentScope);
    std::shared_ptr<Statement> lastScope() const;
    void removeLastScope();
    void clear() { mScopes.clear(); }
    int count() const { return mScopes.count(); }

private:
    QVector<CppScope> mScopes;
};

#endif // CPPSCOPES_H
