#ifndef PROBLEMCASEVALIDATOR_H
#define PROBLEMCASEVALIDATOR_H

#include "ojproblemset.h"

class ProblemCaseValidator
{
public:
    ProblemCaseValidator();
    bool validate(POJProblemCase problemCase,bool ignoreSpaces);
private:
    bool equalIgnoringSpaces(const QString& s1, const QString& s2);
    QStringList split(const QString& s);
};

#endif // PROBLEMCASEVALIDATOR_H
