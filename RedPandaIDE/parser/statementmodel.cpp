#include "statementmodel.h"

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
