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
#include "statementmodel.h"

#include <QFile>
#include <QTextStream>

StatementModel::StatementModel(QObject *parent) : QObject(parent)
{
    mCount = 0;
}

void StatementModel::add(const PStatement& statement)
{
    if (!statement) {
        return ;
    }
    PStatement parent = statement->parentScope.lock();
    if (parent) {
        addMember(parent->children,statement);
    } else {
        addMember(mGlobalStatements,statement);
    }
    mCount++;
#ifdef QT_DEBUG
    mAllStatements.append(statement);
#endif
}

void StatementModel::deleteStatement(const PStatement& statement)
{
    if (!statement) {
        return ;
    }
    PStatement parent = statement->parentScope.lock();
    int count = 0;
    if (parent) {
        count = deleteMember(parent->children,statement);
    } else {
        count = deleteMember(mGlobalStatements,statement);
    }
    mCount -= count;
#ifdef QT_DEBUG
    mAllStatements.removeOne(statement);
#endif

}

#ifdef QT_DEBUG
void StatementModel::dump(const QString &logFile)
{
    QFile file(logFile);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);
        dumpStatementMap(mGlobalStatements,out,0);
    }
}

void StatementModel::dumpAll(const QString &logFile)
{
    QFile file(logFile);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);
        for (PStatement statement:mAllStatements) {
            out<<QString("%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12")
             .arg(statement->command).arg(int(statement->kind))
             .arg(statement->type).arg(statement->fullName)
             .arg((size_t)(statement->parentScope.lock().get()))
             .arg((int)statement->accessibility)
             .arg(statement->fileName)
             .arg(statement->line)
             .arg(statement->definitionFileName)
             .arg(statement->definitionLine)<<Qt::endl;
        }
    }
}
#endif

void StatementModel::addMember(StatementMap &map, const PStatement& statement)
{
    if (!statement)
        return ;
    map.insert(statement->command,statement);
//    QList<PStatement> lst = map.values(statement->command);
//    if (!lst) {
//        lst=std::make_shared<StatementList>();
//        map.insert(,lst);
//    }
//    lst->append(statement);
}

int StatementModel::deleteMember(StatementMap &map, const PStatement& statement)
{
    if (!statement)
        return 0;
    return map.remove(statement->command,statement);
}

void StatementModel::dumpStatementMap(StatementMap &map, QTextStream &out, int level)
{
    QString indent(level,'\t');
    foreach (const PStatement& statement,map) {
        out<<indent<<QString("%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12")
         .arg(statement->command).arg(int(statement->kind))
         .arg(statement->type,statement->fullName)
         .arg(statement->noNameArgs)
         .arg(statement->args)
         .arg((size_t)(statement->parentScope.lock().get()))
         .arg((int)statement->accessibility)
         .arg(statement->fileName)
         .arg(statement->line)
         .arg(statement->definitionFileName)
         .arg(statement->definitionLine);
        out<<Qt::endl;
        if (statement->children.isEmpty())
            continue;
        out<<indent<<statement->command<<" {"<<Qt::endl;
        dumpStatementMap(statement->children,out,level+1);
        out<<indent<<"}"<<Qt::endl;

    }
}
