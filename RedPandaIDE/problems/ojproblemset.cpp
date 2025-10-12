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

OJProblemCase::OJProblemCase(QObject *parent):
    QObject{parent},
    mModified{false},
    testState{ProblemCaseTestState::NotTested},
    firstDiffLine{-1}
{
    QUuid uid = QUuid::createUuid();
    mId = uid.toString();
}

const QString &OJProblemCase::name() const
{
    return mName;
}

void OJProblemCase::setName(const QString &newName)
{
    if (mName != newName) {
        mName = newName;
        setModified(true);
    }
}

const QString &OJProblemCase::input() const
{
    return mInput;
}

void OJProblemCase::setInput(const QString &newInput)
{
    if (mInput != newInput) {
        mInput = newInput;
        setModified(true);
    }
}

const QString &OJProblemCase::expected() const
{
    return mExpected;
}

void OJProblemCase::setExpected(const QString &newExpected)
{
    if (mExpected!=newExpected) {
        mExpected = newExpected;
        setModified(true);
    }
}

const QString &OJProblemCase::inputFileName() const
{
    return mInputFileName;
}

void OJProblemCase::setInputFileName(const QString &newInputFileName)
{
    if (mInputFileName!=newInputFileName) {
        mInputFileName = newInputFileName;
        setModified(true);
    }
}

const QString &OJProblemCase::expectedOutputFileName() const
{
    return mExpectedOutputFileName;
}

void OJProblemCase::setExpectedOutputFileName(const QString &newExpectedOutputFileName)
{
    if (mExpectedOutputFileName != newExpectedOutputFileName) {
        mExpectedOutputFileName = newExpectedOutputFileName;
        setModified(true);
    }
}

bool OJProblemCase::isModified()
{
    return mModified;
}


void OJProblemCase::setModified(bool newModified)
{
    if (mModified == newModified)
        return;
    mModified = newModified;
    emit modifiedChanged(getId());
}

const QString &OJProblemCase::getId() const
{
    return mId;
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
