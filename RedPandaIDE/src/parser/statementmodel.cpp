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

QString occurrenceOwner(const StatementOccurrence &occurrence)
{
    return occurrence.boundaryAncestor.isEmpty()
            ? occurrence.fileName : occurrence.boundaryAncestor;
}

void updateNamespaceSource(const PStatement &statement)
{
    auto nameSpace = std::dynamic_pointer_cast<NamespaceStatement>(statement);
    if (statement->declarations.isEmpty()) {
        statement->definition = StatementOccurrence{};
        setStatementHasDefinition(statement, false);
        nameSpace->declSpecifierProperties.setFlag(DeclSpecifierProperty::Inline, false);
        return;
    }
    statement->definition = *statement->declarations.constBegin();
    setStatementHasDefinition(statement, true);
    bool isInline = false;
    for (const StatementOccurrence &occurrence : statement->declarations) {
        isInline |= occurrence.declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline);
    }
    nameSpace->declSpecifierProperties.setFlag(DeclSpecifierProperty::Inline, isInline);
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

void StatementModel::addDeclaration(const PStatement &statement, const QString &fileName,
                                    const SourceRange &range, const QString &boundaryAncestor,
                                    const DeclSpecifierProperties &properties)
{
    if (fileName.isEmpty())
        return;
    StatementOccurrence occurrence{fileName, range, boundaryAncestor, properties};
    QString owner = occurrenceOwner(occurrence);
    const QList<StatementOccurrence> existing = statement->declarations.values(owner);
    for (const StatementOccurrence &item : existing) {
        if (item.fileName == fileName && item.range == range)
            return;
    }
    statement->declarations.insert(owner, occurrence);
}

void StatementModel::addNamespaceDeclaration(const PStatement &statement,
                                             const QString &fileName,
                                             const SourceRange &range,
                                             const QString &boundaryAncestor,
                                             bool isInline)
{
    DeclSpecifierProperties properties;
    properties.setFlag(DeclSpecifierProperty::Inline, isInline);
    addDeclaration(statement, fileName, range, boundaryAncestor, properties);
    updateNamespaceSource(statement);
}

bool StatementModel::hasDeclarationInFile(const PStatement &statement, const QString &fileName) const
{
    if (statement->declarations.contains(fileName))
        return true;
    for (const StatementOccurrence &occurrence : statement->declarations) {
        if (occurrence.fileName == fileName)
            return true;
    }
    return false;
}

bool StatementModel::isInFile(const PStatement &statement, const QString &fileName) const
{
    bool definitionMatches = statementHasDefinition(statement)
            && (statement->definition.fileName == fileName
                || statement->definition.boundaryAncestor == fileName);
    return hasDeclarationInFile(statement, fileName) || definitionMatches;
}

void StatementModel::removeDeclarations(const PStatement &statement, const QString &fileName)
{
    statement->declarations.remove(fileName);
    for (auto it = statement->declarations.begin(); it != statement->declarations.end();) {
        if (it.value().fileName == fileName)
            it = statement->declarations.erase(it);
        else
            ++it;
    }
    auto normal = std::dynamic_pointer_cast<Statement>(statement);
    if (!normal)
        return;
    DeclSpecifierProperties properties;
    if (statementHasDefinition(statement))
        properties = statement->definition.declSpecifierProperties;
    for (const StatementOccurrence &occurrence : statement->declarations)
        properties |= occurrence.declSpecifierProperties;
    normal->declSpecifierProperties = properties;
    statement->properties.setFlag(StatementProperty::Static,
                                  properties.testFlag(DeclSpecifierProperty::Static));
    statement->properties.setFlag(StatementProperty::Constexpr,
                                  properties.testFlag(DeclSpecifierProperty::Constexpr));
}

void StatementModel::removeNamespaceDeclarations(const PStatement &statement,
                                                 const QString &fileName)
{
    statement->declarations.remove(fileName);
    for (auto it = statement->declarations.begin(); it != statement->declarations.end();) {
        if (it.value().fileName == fileName)
            it = statement->declarations.erase(it);
        else
            ++it;
    }
    updateNamespaceSource(statement);
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
            StatementOccurrence aDeclaration = statementDeclarationForNavigation(a, QString());
            StatementOccurrence bDeclaration = statementDeclarationForNavigation(b, QString());
            if (aDeclaration.fileName != bDeclaration.fileName)
                return aDeclaration.fileName < bDeclaration.fileName;
            return aDeclaration.range.start < bDeclaration.range.start;
        });
        for (const PStatement &statement : statements) {
            StatementOccurrence declaration = statementDeclarationForNavigation(statement, QString());
            StatementOccurrence definition = statementDefinitionForNavigation(statement);
            out << QString("%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12")
                   .arg(statement->command).arg(int(statement->kind))
                   .arg(statementType(statement)).arg(statement->fullName)
                   .arg((size_t)(statement->parentScope.lock().get()))
                   .arg((int)statement->accessibility)
                   .arg(declaration.fileName)
                   .arg(declaration.range.start.line)
                   .arg(definition.fileName)
                   .arg(definition.range.start.line) << Qt::endl;
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
        StatementOccurrence declaration = statementDeclarationForNavigation(statement, QString());
        StatementOccurrence definition = statementDefinitionForNavigation(statement);
        out << indent << QString("%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12")
               .arg(statement->command).arg(int(statement->kind))
               .arg(statementType(statement), statement->fullName, normalizedArgs, args)
               .arg((size_t)(statement->parentScope.lock().get()))
               .arg((int)statement->accessibility)
               .arg(declaration.fileName)
               .arg(declaration.range.start.line)
               .arg(definition.fileName)
               .arg(definition.range.start.line);
        out << Qt::endl;
        const StatementMap &children = childrenStatements(statement);
        if (children.isEmpty())
            continue;
        out << indent << statement->command << " {" << Qt::endl;
        dumpStatementMap(children, out, level + 1);
        out << indent << "}" << Qt::endl;
    }
}
