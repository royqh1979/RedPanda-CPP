#include "problemcasevalidator.h"
#include "../utils.h"

ProblemCaseValidator::ProblemCaseValidator()
{

}

bool ProblemCaseValidator::validate(POJProblemCase problemCase, bool ignoreSpaces)
{
    if (!problemCase)
        return false;
    QStringList output = textToLines(problemCase->output);
    QStringList expected = textToLines(problemCase->expected);
    if (output.count()!=expected.count())
        return false;
    for (int i=0;i<output.count();i++) {
        if (ignoreSpaces) {
            if (!equalIgnoringSpaces(output[i],expected[i]))
                return false;
        } else {
            if (output[i]!=expected[i])
                return false;
        }
    }
    return true;
}

bool ProblemCaseValidator::equalIgnoringSpaces(const QString &s1, const QString &s2)
{
    QStringList strList1=split(s1);
    QStringList strList2=split(s2);
    return (strList1==strList2);
}

QStringList ProblemCaseValidator::split(const QString &s)
{
    QStringList result;
    const QChar* p = s.data();
    const QChar* start = p;
    while (p->unicode()!=0) {
        if (p->isSpace()) {
            if (!start->isSpace()) {
                result.append(QString(start,p-start));
            }
            start = p;
        } else if (start->isSpace()) {
            start = p;
        }
        p++;
    }
    if (!start->isSpace()) {
        result.append(QString(start,p-start));
    }
    return result;
}
