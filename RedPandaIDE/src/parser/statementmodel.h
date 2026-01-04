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
#ifndef STATEMENTMODEL_H
#define STATEMENTMODEL_H

#include <QObject>
#include <QTextStream>
#include "parserutils.h"

class StatementModel : public QObject
{
    Q_OBJECT
public:
    explicit StatementModel(QObject *parent = nullptr);
    StatementModel(const StatementModel&)=delete;
    StatementModel& operator=(const StatementModel&)=delete;

    void add(const PStatement& statement);
    void deleteStatement(const PStatement& statement);
    const StatementMap& childrenStatements(const PStatement& statement = PStatement()) const {
        if (!statement) {
            return mGlobalStatements;
        } else {
            return statement->children;
        }
    }
    const StatementMap& childrenStatements(std::weak_ptr<Statement> statement) const { return childrenStatements(statement.lock()); }
    void clear() {
        mCount=0;
        mGlobalStatements.clear();
#ifdef QT_DEBUG
        mAllStatements.clear();
#endif
    }
    int count() const { return mCount; }
#ifdef QT_DEBUG
    void dump(const QString& logFile);
    void dumpAll(const QString& logFile);
#endif
private:
    void addMember(StatementMap& map, const PStatement& statement);
    int deleteMember(StatementMap& map, const PStatement& statement);
    void dumpStatementMap(StatementMap& map, QTextStream& out, int level);
private:
    int mCount;
    StatementMap mGlobalStatements;  //may have overloaded functions, so use PStatementList to store
#ifdef QT_DEBUG
    StatementList mAllStatements;
#endif
};

#endif // STATEMENTMODEL_H
