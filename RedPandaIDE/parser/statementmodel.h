#ifndef STATEMENTMODEL_H
#define STATEMENTMODEL_H

#include <QObject>

class StatementModel : public QObject
{
    Q_OBJECT
public:
    explicit StatementModel(QObject *parent = nullptr);

signals:

};

#endif // STATEMENTMODEL_H
