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

StatementModel::StatementModel(QObject *parent) : QObject(parent)
{
    mCount = 0;
}

void StatementModel::add(const PStatement& statement)
{
    if (!statement)
        return;
    PStatement parent = statement->parentScope.lock();
    if (parent) {
        addMember(parent->children, statement);
        if (statement->kind == StatementKind::Variable
                && statement->accessibility == StatementAccessibility::Public)
            parent->publicProperties.append(statement);
    } else {
        addMember(mGlobalStatements, statement);
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
        for (auto it = parent->publicProperties.begin();
                it != parent->publicProperties.end();) {
            PStatement property = it->lock();
            if (!property || property == statement)
                it = parent->publicProperties.erase(it);
            else
                ++it;
        }
    } else {
        deleteMember(mGlobalStatements, statement);
    }
    if (mStatements.removeOne(statement))
        --mCount;
}

#ifdef QT_DEBUG
void StatementModel::dump(const QString &logFile)
{
    QFile file(logFile);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);
        dumpStatementMap(mGlobalStatements, out, 0);
    }
}

void StatementModel::dumpAll(const QString &logFile)
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

void StatementModel::dumpStatementMap(const StatementMap &map, QTextStream &out, int level)
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
