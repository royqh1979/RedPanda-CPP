/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
    QStringList expected;
    if (fileExists(problemCase->expectedOutputFileName))
        expected = readFileToLines(problemCase->expectedOutputFileName);
    else
        expected = textToLines(problemCase->expected);
    problemCase->outputLineCounts = output.count();
    problemCase->expectedLineCounts = expected.count();
    if (problemCase->expectedLineCounts>5000) {
        if (output.count()<expected.count()) {
            problemCase->firstDiffLine=output.count();
            return false;
        }  else if (output.count()>expected.count()) {
            problemCase->firstDiffLine=expected.count();
            return false;
        }
    }
    int count=std::min(output.count(), expected.count());
    for (int i=0;i<count;i++) {
        if (ignoreSpaces) {
            if (!equalIgnoringSpaces(output[i],expected[i])) {
                problemCase->firstDiffLine = i;
                return false;
            }
        } else {
            if (output[i]!=expected[i]) {
                problemCase->firstDiffLine = i;
                return false;
            }
        }
    }
    if (output.count()<expected.count()) {
        problemCase->firstDiffLine=output.count();
        return false;
    }  else if (output.count()>expected.count()) {
        problemCase->firstDiffLine=expected.count();
        return false;
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
