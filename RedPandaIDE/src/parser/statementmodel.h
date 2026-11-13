/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
    const StatementMap& childrenStatements(std::weak_ptr<Statement> statement) const {
        return childrenStatements(statement.lock());
    }
    void clear() {
        mCount=0;
        for (const PStatement &statement : mStatements) {
            statement->children.clear();
            statement->publicProperties.clear();
        }
        mStatements.clear();
        mGlobalStatements.clear();
        mFullNameStatements.clear();
    }
    int count() const { return mCount; }
    const StatementList &statements() const { return mStatements; }
    PStatement findByFullName(const QString &fullName) const;
#ifdef QT_DEBUG
    void dump(const QString& logFile);
    void dumpAll(const QString& logFile);
#endif
private:
    void addMember(StatementMap& map, const PStatement& statement);
    void deleteMember(StatementMap& map, const PStatement& statement);
    void dumpStatementMap(const StatementMap& map, QTextStream& out, int level);
private:
    int mCount;
    StatementList mStatements;
    StatementMap mGlobalStatements;  //may have overloaded functions, so use QMultiMap
    StatementMap mFullNameStatements;
};

#endif // STATEMENTMODEL_H
