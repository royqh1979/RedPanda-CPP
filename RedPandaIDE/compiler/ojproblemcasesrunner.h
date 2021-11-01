#ifndef OJPROBLEMCASESRUNNER_H
#define OJPROBLEMCASESRUNNER_H

#include "runner.h"
#include <QVector>

class OJProblemCase;
using POJProblemCase = std::shared_ptr<OJProblemCase>;

class OJProblemCasesRunner : public Runner
{
    Q_OBJECT
public:
    explicit OJProblemCasesRunner(const QString& filename, const QString& arguments, const QString& workDir,
                                  const QVector<POJProblemCase>& problemCases, QObject *parent = nullptr);
    explicit OJProblemCasesRunner(const QString& filename, const QString& arguments, const QString& workDir,
                                  POJProblemCase problemCase, QObject *parent = nullptr);
signals:
    void caseStarted(int total, int current);
    void caseFinished(int total, int current);
private:
    void runCase(int index, POJProblemCase problemCase);
private:
    QVector<POJProblemCase> mProblemCases;

    // QThread interface
protected:
    void run() override;
};

#endif // OJPROBLEMCASESRUNNER_H
