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
        } else if (statement->kind == StatementKind::Namespace) {
            return std::dynamic_pointer_cast<NamespaceStatement>(statement)->children;
        } else {
            return std::dynamic_pointer_cast<Statement>(statement)->children;
        }
    }
    const StatementMap& childrenStatements(std::weak_ptr<AbstractStatement> statement) const {
        return childrenStatements(statement.lock());
    }
    void clear() {
        mCount=0;
        for (const PStatement &statement : mStatements) {
            statement->declarations.clear();
            if (statement->kind == StatementKind::Namespace)
                std::dynamic_pointer_cast<NamespaceStatement>(statement)->children.clear();
            else if (statement->kind != StatementKind::InlineAlias)
                std::dynamic_pointer_cast<Statement>(statement)->children.clear();
        }
        mStatements.clear();
        mGlobalStatements.clear();
    }
    int count() const { return mCount; }
    int cppVersion() const { return mCppVersion; }
    void setCppVersion(int cppVersion) { mCppVersion = cppVersion; }
    const StatementList &statements() const { return mStatements; }
    void addDeclaration(const PStatement &statement, const QString &fileName,
                        const SourceRange &range, const QString &boundaryAncestor,
                        const DeclSpecifierProperties &properties);
    void addNamespaceDeclaration(const PStatement &statement, const QString &fileName,
                                 const SourceRange &range, const QString &boundaryAncestor,
                                 bool isInline);
    bool hasDeclarationInFile(const PStatement &statement, const QString &fileName) const;
    bool isInFile(const PStatement &statement, const QString &fileName) const;
    void removeDeclarations(const PStatement &statement, const QString &fileName);
    void removeNamespaceDeclarations(const PStatement &statement, const QString &fileName);
#ifdef QT_DEBUG
    void dump(const QString& logFile) const;
    void dumpAll(const QString& logFile) const;
#endif
private:
    void addMember(StatementMap& map, const PStatement& statement);
    void deleteMember(StatementMap& map, const PStatement& statement);
    void dumpStatementMap(const StatementMap& map, QTextStream& out, int level) const;
private:
    int mCount;
    int mCppVersion = CppVersion::Cpp17;
    StatementList mStatements;
    StatementMap mGlobalStatements;  //may have overloaded functions, so use QMultiMap
};

#endif // STATEMENTMODEL_H
