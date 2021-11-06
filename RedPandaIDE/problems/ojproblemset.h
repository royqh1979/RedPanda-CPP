#ifndef OJPROBLEMSET_H
#define OJPROBLEMSET_H
#include <QString>
#include <memory>
#include <QVector>

enum class ProblemCaseTestState {
    NotTested,
    Testing,
    Passed,
    Failed
};

struct OJProblemCase {
    QString name;
    QString input;
    QString expected;
    ProblemCaseTestState testState; // no persistence
    QString output; // no persistence
    OJProblemCase();

public:
    const QString &getId() const;

private:
    QString id;
};

using POJProblemCase = std::shared_ptr<OJProblemCase>;

struct OJProblem {
    QString name;
    QString url;
    QString description;
    QString answerProgram;
    QVector<POJProblemCase> cases;
};

using POJProblem = std::shared_ptr<OJProblem>;

struct OJProblemSet {
    QString name;
    QVector<POJProblem> problems;
};

using POJProblemSet  = std::shared_ptr<OJProblemSet>;

#endif // OJPROBLEMSET_H
