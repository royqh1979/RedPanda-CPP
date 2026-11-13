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
#include "statementmodel.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QElapsedTimer>

namespace {
struct StatementModelProfile {
    qint64 addNs = 0;
    int addCount = 0;
    ~StatementModelProfile() { qWarning() << "STATEMENT_MODEL_PROFILE addMs" << addNs / 1000000.0 << "count" << addCount; }
};

StatementModelProfile statementModelProfile;

struct StatementModelProfileScope {
    StatementModelProfileScope() { timer.start(); }
    ~StatementModelProfileScope() { statementModelProfile.addNs += timer.nsecsElapsed(); }
    QElapsedTimer timer;
};
}

StatementModel::StatementModel(QObject *parent) : QObject(parent)
{
    mCount = 0;
}

void StatementModel::add(const PStatement& statement)
{
    StatementModelProfileScope profileScope;
    if (!statement)
        return;
    ++statementModelProfile.addCount;
    PStatement parent = statement->parentScope.lock();
    if (parent) {
        addMember(parent->children, statement);
    } else {
        addMember(mGlobalStatements, statement);
    }
    if (statement->kind == StatementKind::Namespace) {
        addNamespaceStatement(statement->fullName, statement);
        updateNamespaceTransparency(statement);
    }
    mStatements.append(statement);
    ++mCount;
}

void StatementModel::deleteStatement(const PStatement& statement)
{
    if (!statement)
        return;
    PStatement parent = statement->parentScope.lock();
    if (parent) {
        deleteMember(parent->children, statement);
    } else {
        deleteMember(mGlobalStatements, statement);
    }
    if (statement->kind == StatementKind::Namespace) {
        const QList<PStatement> siblings = namespaceStatements(statement->fullName);
        if (parent && parent->kind == StatementKind::Namespace) {
            for (const PStatement &sibling : siblings)
                deleteNamespaceStatement(parent->fullName, sibling);
        }
        for (auto it = mNamespaceStatements.find(statement->fullName);
                it != mNamespaceStatements.end() && it.key() == statement->fullName;) {
            PStatement indexed = it.value().lock();
            if (indexed && indexed->parentScope.lock() == statement)
                it = mNamespaceStatements.erase(it);
            else
                ++it;
        }
        deleteNamespaceStatement(statement->fullName, statement);
        for (const PStatement &sibling : siblings) {
            if (sibling != statement)
                updateNamespaceTransparency(sibling);
        }
    }
    if (mStatements.removeOne(statement))
        --mCount;
}

void StatementModel::updateNamespaceTransparency(const PStatement &statement)
{
    if (!statement || statement->kind != StatementKind::Namespace)
        return;
    const QList<PStatement> siblings = namespaceStatements(statement->fullName);
    bool transparent = statement->command.isEmpty();
    for (const PStatement &sibling : siblings) {
        if (sibling->command.isEmpty()
                || sibling->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline)) {
            transparent = true;
            break;
        }
    }
    if (!transparent)
        return;
    PStatement parent = statement->parentScope.lock();
    if (parent && parent->kind == StatementKind::Namespace) {
        for (const PStatement &sibling : siblings)
            addNamespaceStatement(parent->fullName, sibling);
    }
}

void StatementModel::addNamespaceStatement(const QString &fullName, const PStatement &statement)
{
    const auto statements = mNamespaceStatements.values(fullName);
    for (const std::weak_ptr<Statement> &weak : statements) {
        if (weak.lock() == statement)
            return;
    }
    mNamespaceStatements.insert(fullName, statement);
}

void StatementModel::deleteNamespaceStatement(const QString &fullName, const PStatement &statement)
{
    for (auto it = mNamespaceStatements.find(fullName);
            it != mNamespaceStatements.end() && it.key() == fullName;) {
        PStatement indexed = it.value().lock();
        if (!indexed || indexed == statement)
            it = mNamespaceStatements.erase(it);
        else
            ++it;
    }
}

QList<PStatement> StatementModel::namespaceStatements(const QString &fullName) const
{
    QList<PStatement> result;
    const auto statements = mNamespaceStatements.values(fullName);
    for (const std::weak_ptr<Statement> &weak : statements) {
        if (PStatement statement = weak.lock())
            result.append(statement);
    }
    return result;
}

QList<PStatement> StatementModel::transparentNamespaceStatements(const QString &fullName) const
{
    QList<PStatement> result;
    const QList<PStatement> statements = namespaceStatements(fullName);
    for (const PStatement &statement : statements) {
        if (statement->command.isEmpty() || statement->fullName != fullName)
            result.append(statement);
    }
    return result;
}

#ifdef QT_DEBUG
void StatementModel::dump(const QString &logFile) const
{
    QFile file(logFile);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);
        dumpStatementMap(mGlobalStatements, out, 0);
    }
}

void StatementModel::dumpAll(const QString &logFile) const
{
    QFile file(logFile);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);
        StatementList statements = mStatements;
        std::sort(statements.begin(), statements.end(),
                  [](const PStatement &a, const PStatement &b) {
            if (a->fileName != b->fileName)
                return a->fileName < b->fileName;
            return a->declarationRange.start < b->declarationRange.start;
        });
        for (const PStatement &statement : statements) {
            out << QString("%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12")
                   .arg(statement->command).arg(int(statement->kind))
                   .arg(statement->type()).arg(statement->fullName)
                   .arg((size_t)(statement->parentScope.lock().get()))
                   .arg((int)statement->accessibility)
                   .arg(statement->fileName)
                   .arg(statement->declarationRange.start.line)
                   .arg(statement->definitionFileName)
                   .arg(statement->definitionRange.start.line) << Qt::endl;
        }
    }
}
#endif

void StatementModel::addMember(StatementMap &map, const PStatement& statement)
{
    if (statement)
        map.insert(statement->command, statement);
}

void StatementModel::deleteMember(StatementMap &map, const PStatement& statement)
{
    if (!statement)
        return;
    for (auto it = map.find(statement->command);
            it != map.end() && it.key() == statement->command;) {
        PStatement indexed = it.value().lock();
        if (!indexed || indexed == statement)
            it = map.erase(it);
        else
            ++it;
    }
}

void StatementModel::dumpStatementMap(const StatementMap &map, QTextStream &out, int level) const
{
    QString indent(level, '\t');
    for (const std::weak_ptr<Statement> &weakStatement : map) {
        PStatement statement = weakStatement.lock();
        if (!statement)
            continue;
        out << indent << QString("%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12")
               .arg(statement->command).arg(int(statement->kind))
               .arg(statement->type(), statement->fullName,
                    statement->noNameArgs, statement->args)
               .arg((size_t)(statement->parentScope.lock().get()))
               .arg((int)statement->accessibility)
               .arg(statement->fileName)
               .arg(statement->declarationRange.start.line)
               .arg(statement->definitionFileName)
               .arg(statement->definitionRange.start.line);
        out << Qt::endl;
        if (statement->children.isEmpty())
            continue;
        out << indent << statement->command << " {" << Qt::endl;
        dumpStatementMap(statement->children, out, level + 1);
        out << indent << "}" << Qt::endl;
    }
}
