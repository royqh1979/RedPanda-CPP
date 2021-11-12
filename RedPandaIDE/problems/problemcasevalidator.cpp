#include "problemcasevalidator.h"
#include "../utils.h"

ProblemCaseValidator::ProblemCaseValidator()
{

}

bool ProblemCaseValidator::validate(POJProblemCase problemCase)
{
    if (!problemCase)
        return false;
    QStringList output = textToLines(problemCase->output);
    QStringList expected = textToLines(problemCase->expected);
    if (output.count()!=expected.count())
        return false;
    for (int i=0;i<output.count();i++) {
        if (output[i]!=expected[i])
            return false;
    }
    return true;
}
