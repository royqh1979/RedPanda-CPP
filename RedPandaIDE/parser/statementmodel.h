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

    void add(const PStatement& statement);
//    function DeleteFirst: Integer;
//    function DeleteLast: Integer;
    void deleteStatement(const PStatement& statement);
    const StatementMap& childrenStatements(const PStatement& statement = PStatement()) const;
    const StatementMap& childrenStatements(std::weak_ptr<Statement> statement) const;
    void clear();
    void dump(const QString& logFile);
#ifdef QT_DEBUG
    void dumpAll(const QString& logFile);
#endif
private:
    void addMember(StatementMap& map, const PStatement& statement);
    int deleteMember(StatementMap& map, const PStatement& statement);
    void dumpStatementMap(StatementMap& map, QTextStream& out, int level);
private:
    int mCount;
    StatementMap mGlobalStatements;  //may have overloaded functions, so use PStatementList to store
#ifdef QT_DEBUG
    StatementList mAllStatements;
#endif
};

#endif // STATEMENTMODEL_H
