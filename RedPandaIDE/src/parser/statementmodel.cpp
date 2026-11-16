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
    bool enabled = qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE");
    qint64 addNs = 0;
    int addCount = 0;
    ~StatementModelProfile() {
        if (enabled)
            qWarning() << "STATEMENT_MODEL_PROFILE addMs" << addNs / 1000000.0 << "count" << addCount;
    }
};

StatementModelProfile statementModelProfile;

QString namespaceOwner(const NamespaceOccurrence &occurrence)
{
    return occurrence.boundaryAncestor.isEmpty()
            ? occurrence.fileName : occurrence.boundaryAncestor;
}

void updateNamespaceSource(const PStatement &statement, bool updateInline)
{
    auto nameSpace = std::dynamic_pointer_cast<NamespaceStatement>(statement);
    if (nameSpace->occurrences.isEmpty())
        return;
    const NamespaceOccurrence &current = *nameSpace->occurrences.constBegin();
    statement->fileName = current.fileName;
    statement->declarationRange = current.range;
    statement->declarationBoundaryAncestor = current.boundaryAncestor;
    statement->definitionRange = current.range;
    statement->definitionFileName = current.fileName;
    statement->definitionBoundaryAncestor = current.boundaryAncestor;
    setStatementHasDefinition(statement, true);
    if (updateInline) {
        bool isInline = false;
        for (const NamespaceOccurrence &occurrence : nameSpace->occurrences)
            isInline |= occurrence.isInline;
        nameSpace->declSpecifierProperties.setFlag(DeclSpecifierProperty::Inline, isInline);
    }
}

struct StatementModelProfileScope {
    StatementModelProfileScope() {
        if (statementModelProfile.enabled)
            timer.start();
    }
    ~StatementModelProfileScope() {
        if (statementModelProfile.enabled)
            statementModelProfile.addNs += timer.nsecsElapsed();
    }
    QElapsedTimer timer;
};
}

StatementModel::StatementModel(QObject *parent) : QObject(parent)
{
    mCount = 0;
}

void StatementModel::addNamespaceOccurrence(const PStatement &statement,
                                            const QString &fileName,
                                            const SourceRange &range,
                                            const QString &boundaryAncestor,
                                            bool isInline)
{
    auto nameSpace = std::dynamic_pointer_cast<NamespaceStatement>(statement);
    if (!nameSpace)
        return;
    NamespaceOccurrence occurrence{fileName, range, boundaryAncestor, isInline};
    QString owner = namespaceOwner(occurrence);
    const QList<NamespaceOccurrence> existing = nameSpace->occurrences.values(owner);
    for (const NamespaceOccurrence &item : existing) {
        if (item.fileName == fileName && item.range == range)
            return;
    }
    nameSpace->occurrences.insert(owner, occurrence);
    updateNamespaceSource(statement, true);
}

bool StatementModel::isInFile(const PStatement &statement, const QString &fileName) const
{
    if (statement->kind == StatementKind::Namespace) {
        auto nameSpace = std::dynamic_pointer_cast<NamespaceStatement>(statement);
        return nameSpace->occurrences.contains(fileName);
    }
    bool declarationMatches = statement->declarationBoundaryAncestor == fileName
            || (statement->declarationBoundaryAncestor.isEmpty()
                && statement->fileName == fileName);
    bool definitionMatches = statementHasDefinition(statement)
            && (statement->definitionBoundaryAncestor == fileName
                || (statement->definitionBoundaryAncestor.isEmpty()
                    && statement->definitionFileName == fileName));
    return declarationMatches || definitionMatches;
}

bool StatementModel::visibleFrom(const PStatement &statement, const QString &fileName,
                                 const PParsedFileInfo &fileInfo) const
{
    if (!fileInfo)
        return true;
    if (statement->kind == StatementKind::Namespace) {
        auto nameSpace = std::dynamic_pointer_cast<NamespaceStatement>(statement);
        for (const NamespaceOccurrence &occurrence : nameSpace->occurrences) {
            QString owner = namespaceOwner(occurrence);
            if (owner == fileName || fileInfo->including(owner))
                return true;
        }
        return false;
    }
    QString owner = statement->declarationBoundaryAncestor.isEmpty()
            ? statement->fileName : statement->declarationBoundaryAncestor;
    return owner == fileName || fileInfo->including(owner);
}

bool StatementModel::removeNamespaceOccurrences(const PStatement &statement,
                                                const QString &fileName)
{
    auto nameSpace = std::dynamic_pointer_cast<NamespaceStatement>(statement);
    if (!nameSpace)
        return false;
    nameSpace->occurrences.remove(fileName);
    if (nameSpace->occurrences.isEmpty())
        return false;
    updateNamespaceSource(statement, false);
    return true;
}

void StatementModel::add(const PStatement& statement)
{
    StatementModelProfileScope profileScope;
    if (!statement)
        return;
    ++statementModelProfile.addCount;
    PStatement parent = statement->parentScope.lock();
    if (parent) {
        if (parent->kind == StatementKind::Namespace)
            addMember(std::dynamic_pointer_cast<NamespaceStatement>(parent)->children, statement);
        else
            addMember(std::dynamic_pointer_cast<Statement>(parent)->children, statement);
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
        if (parent->kind == StatementKind::Namespace)
            deleteMember(std::dynamic_pointer_cast<NamespaceStatement>(parent)->children, statement);
        else
            deleteMember(std::dynamic_pointer_cast<Statement>(parent)->children, statement);
    } else {
        deleteMember(mGlobalStatements, statement);
    }
    if (mStatements.removeOne(statement))
        --mCount;
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
                   .arg(statementType(statement)).arg(statement->fullName)
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
    for (const std::weak_ptr<AbstractStatement> &weakStatement : map) {
        PStatement statement = weakStatement.lock();
        if (!statement)
            continue;
        QString normalizedArgs;
        QString args;
        if (statement->kind != StatementKind::Namespace) {
            auto normal = std::dynamic_pointer_cast<Statement>(statement);
            normalizedArgs = normal->normalizedArgs;
            args = normal->args;
        }
        out << indent << QString("%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12")
               .arg(statement->command).arg(int(statement->kind))
               .arg(statementType(statement), statement->fullName, normalizedArgs, args)
               .arg((size_t)(statement->parentScope.lock().get()))
               .arg((int)statement->accessibility)
               .arg(statement->fileName)
               .arg(statement->declarationRange.start.line)
               .arg(statement->definitionFileName)
               .arg(statement->definitionRange.start.line);
        out << Qt::endl;
        const StatementMap &children = childrenStatements(statement);
        if (children.isEmpty())
            continue;
        out << indent << statement->command << " {" << Qt::endl;
        dumpStatementMap(children, out, level + 1);
        out << indent << "}" << Qt::endl;
    }
}
