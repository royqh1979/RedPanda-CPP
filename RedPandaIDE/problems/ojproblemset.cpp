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
#include "ojproblemset.h"

#include <QUuid>

OJProblemCase::OJProblemCase():
    testState(ProblemCaseTestState::NotTested),
    firstDiffLine(-1),
    outputLineCounts(0),
    expectedLineCounts(0)
{
    QUuid uid = QUuid::createUuid();
    id = uid.toString();
}

const QString &OJProblemCase::getId() const
{
    return id;
}

size_t OJProblem::getTimeLimit()
{
    switch(timeLimitUnit) {
    case ProblemTimeLimitUnit::Seconds:
        return timeLimit*1000;
    default:
        return timeLimit;
    }
}

size_t OJProblem::getMemoryLimit()
{
    switch(memoryLimitUnit) {
    case ProblemMemoryLimitUnit::KB:
        return memoryLimit*1024;
    case ProblemMemoryLimitUnit::MB:
        return memoryLimit*1024*1024;
    default:
        return memoryLimit*1024*1024*1024;
    }
}

OJProblem::OJProblem() :
    timeLimit(0),
    memoryLimit(0),
    timeLimitUnit(ProblemTimeLimitUnit::Seconds),
    memoryLimitUnit(ProblemMemoryLimitUnit::MB)
{

}
