#ifndef OJPROBLEMCASESRUNNER_H
#define OJPROBLEMCASESRUNNER_H

#include "runner.h"
#include <QVector>
#include "../problems/ojproblemset.h"

class OJProblemCasesRunner : public Runner
{
    Q_OBJECT
public:
    explicit OJProblemCasesRunner(const QString& filename, const QString& arguments, const QString& workDir,
                                  const QVector<POJProblemCase>& problemCases, QObject *parent = nullptr);
    explicit OJProblemCasesRunner(const QString& filename, const QString& arguments, const QString& workDir,
                                  POJProblemCase problemCase, QObject *parent = nullptr);
signals:
    void caseStarted(const QString& id, int current, int total);
    void caseFinished(const QString& id, int current, int total);
private:
    void runCase(int index, POJProblemCase problemCase);
private:
    QVector<POJProblemCase> mProblemCases;

    // QThread interface
protected:
    void run() override;
};

#endif // OJPROBLEMCASESRUNNER_H
