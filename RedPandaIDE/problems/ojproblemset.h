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
    QString inputFileName;
    QString expectedOutputFileName;
    ProblemCaseTestState testState; // no persistence
    QString output; // no persistence
    int runningTime;
    int firstDiffLine;
    int outputLineCounts;
    int expectedLineCounts;
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
    QString exportFilename;
};

using POJProblemSet  = std::shared_ptr<OJProblemSet>;

#endif // OJPROBLEMSET_H
