#include "statementmodel.h"

StatementModel::StatementModel(QObject *parent) : QObject(parent)
{
    mCount = 0;
    mClearing = false;
    mBatchDeleteCount = 0;
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
