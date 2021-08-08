#include "statementmodel.h"

#include <QFile>
#include <QTextStream>

StatementModel::StatementModel(QObject *parent) : QObject(parent)
{
    mCount = 0;
}

void StatementModel::add(PStatement statement)
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
}

void StatementModel::deleteStatement(PStatement statement)
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
}

const StatementMap &StatementModel::childrenStatements(PStatement statement)
{
    if (!statement) {
        return mGlobalStatements;
    } else {
        return statement->children;
    }
}

const StatementMap &StatementModel::childrenStatements(std::weak_ptr<Statement> statement)
{
    PStatement s = statement.lock();
    return childrenStatements(s);
}

void StatementModel::clear() {
    mCount=0;
    mGlobalStatements.clear();
}

void StatementModel::dump(const QString &logFile)
{
    QFile file(logFile);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);
        dumpStatementMap(mGlobalStatements,out,0);
    }
}

void StatementModel::addMember(StatementMap &map, PStatement statement)
{
    if (!statement)
        return ;
    PStatementList lst = map.value(statement->command, PStatementList());
    if (!lst) {
        lst=std::make_shared<StatementList>();
        map.insert(statement->command,lst);
    }
    lst->append(statement);
}

int StatementModel::deleteMember(StatementMap &map, PStatement statement)
{
    if (!statement)
        return 0;
    PStatementList lst = map.value(statement->command, PStatementList());
    if (!lst) {
        return 0;
    }
    return lst->removeAll(statement);
}

void StatementModel::dumpStatementMap(StatementMap &map, QTextStream &out, int level)
{
    QString indent(' ',level);
    for (PStatementList lst:map) {
        for (PStatement statement:(*lst)) {
            out<<indent<<QString("%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12")
             .arg(statement->command).arg(int(statement->kind))
             .arg(statement->type).arg(statement->fullName)
             .arg((size_t)(statement->parentScope.lock().get()))
             .arg((int)statement->classScope)
             .arg(statement->fileName)
             .arg(statement->line)
             .arg(statement->endLine)
             .arg(statement->definitionFileName)
             .arg(statement->definitionLine)
             .arg(statement->definitionEndLine)<<Qt::endl;
            if (statement->children.isEmpty())
                continue;
            out<<indent<<statement->command<<" {"<<Qt::endl;
            dumpStatementMap(statement->children,out,level+1);
            out<<indent<<"}"<<Qt::endl;
        }
    }
}
