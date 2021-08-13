#ifndef STATEMENTMODEL_H
#define STATEMENTMODEL_H

#include <QObject>
#include <QTextStream>
#include "parserutils.h"

class StatementModel : public QObject
{
    Q_OBJECT
public:
    explicit StatementModel(QObject *parent = nullptr);

    void add(PStatement statement);
//    function DeleteFirst: Integer;
//    function DeleteLast: Integer;
    void deleteStatement(PStatement statement);
    const StatementMap& childrenStatements(PStatement statement = PStatement());
    const StatementMap& childrenStatements(std::weak_ptr<Statement> statement);
    void clear();
    void dump(const QString& logFile);

signals:

private:
    void addMember(StatementMap& map, PStatement statement);
    int deleteMember(StatementMap& map, PStatement statement);
    void dumpStatementMap(StatementMap& map, QTextStream& out, int level);
private:
    int mCount;
    StatementMap mGlobalStatements;  //may have overloaded functions, so use PStatementList to store
};

#endif // STATEMENTMODEL_H
