#ifndef STATEMENTMODEL_H
#define STATEMENTMODEL_H

#include <QObject>
#include "utils.h"

class StatementModel : public QObject
{
    Q_OBJECT
public:
    explicit StatementModel(QObject *parent = nullptr);

    void add(PStatement statement);
//    function DeleteFirst: Integer;
//    function DeleteLast: Integer;
    void BeginBatchDelete();
    void endBatchDelete();
    void deleteStatement(PStatement statement);
    const StatementMap& childrenStatements(PStatement statement = PStatement());
    const StatementMap& childrenStatements(std::weak_ptr<Statement> statement);
    void clear();
    void dumpTo(const QString& logFile);
    void dumpWithScope(const QString& logFile);

signals:
private:
    int mCount;
    bool mClearing;
    StatementMap mGlobalStatements;  //may have overloaded functions, so use PStatementList to store
    int mBatchDeleteCount;
};

#endif // STATEMENTMODEL_H
