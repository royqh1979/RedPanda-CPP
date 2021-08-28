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

const StatementMap &StatementModel::childrenStatements(const PStatement& statement) const
{
    if (!statement) {
        return mGlobalStatements;
    } else {
        return statement->children;
    }
}

const StatementMap &StatementModel::childrenStatements(std::weak_ptr<Statement> statement) const
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

#ifdef QT_DEBUG
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
             .arg((int)statement->classScope)
             .arg(statement->fileName)
             .arg(statement->line)
             .arg(statement->endLine)
             .arg(statement->definitionFileName)
             .arg(statement->definitionLine)
             .arg(statement->definitionEndLine)<<Qt::endl;
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
    for (PStatement statement:map.values()) {
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
