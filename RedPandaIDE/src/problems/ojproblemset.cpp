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
    emit modifiedChanged(id());
}

const QString &OJProblemCase::id() const
{
    return mId;
}

size_t OJProblem::getTimeLimitInMilliseconds()
{
    switch(mTimeLimitUnit) {
    case ProblemTimeLimitUnit::Seconds:
        return mTimeLimit*1000;
    default:
        return mTimeLimit;
    }
}

size_t OJProblem::getMemoryLimitInBytes()
{
    switch(mMemoryLimitUnit) {
    case ProblemMemoryLimitUnit::KB:
        return mMemoryLimit*1024;
    case ProblemMemoryLimitUnit::MB:
        return mMemoryLimit*1024*1024;
    default:
        return mMemoryLimit*1024*1024*1024;
    }
}

const QString &OJProblem::name() const
{
    return mName;
}

void OJProblem::setName(const QString &newName)
{
    if (mName != newName) {
        mName = newName;
        setModified(true);
    }
}

const QString &OJProblem::url() const
{
    return mUrl;
}

void OJProblem::setUrl(const QString &newUrl)
{
    if (mUrl != newUrl) {
        mUrl = newUrl;
        setModified(true);
    }
}

const QString &OJProblem::description() const
{
    return mDescription;
}

void OJProblem::setDescription(const QString &newDescription)
{
    if (mDescription != newDescription) {
        mDescription = newDescription;
        setModified(true);
    }
}

const QString &OJProblem::hint() const
{
    return mHint;
}

void OJProblem::setHint(const QString &newHint)
{
    if (mHint != newHint) {
        mHint = newHint;
        setModified(true);
    }
}

const QString &OJProblem::answerProgram() const
{
    return mAnswerProgram;
}

void OJProblem::setAnswerProgram(const QString &newAnswerProgram)
{
    if (mAnswerProgram != newAnswerProgram) {
        mAnswerProgram = newAnswerProgram;
        setModified(true);
    }
}

void OJProblem::setTimeLimit(size_t newTimeLimit)
{
    if (mTimeLimit != newTimeLimit) {
        mTimeLimit = newTimeLimit;
        setModified(true);
    }
}

void OJProblem::setMemoryLimit(size_t newMemoryLimit)
{
    if (mMemoryLimit != newMemoryLimit) {
        mMemoryLimit = newMemoryLimit;
        setModified(true);
    }
}

ProblemTimeLimitUnit OJProblem::timeLimitUnit() const
{
    return mTimeLimitUnit;
}

void OJProblem::setTimeLimitUnit(ProblemTimeLimitUnit newTimeLimitUnit)
{
    mTimeLimitUnit = newTimeLimitUnit;
}

ProblemMemoryLimitUnit OJProblem::memoryLimitUnit() const
{
    return mMemoryLimitUnit;
}

void OJProblem::setMemoryLimitUnit(ProblemMemoryLimitUnit newMemoryLimitUnit)
{
    if (mMemoryLimitUnit != newMemoryLimitUnit) {
        mMemoryLimitUnit = newMemoryLimitUnit;
        setModified(true);
    }
}

const QVector<POJProblemCase> &OJProblem::cases() const
{
    return mCases;
}

void OJProblem::addCase(POJProblemCase &problemCase)
{
    connect(problemCase.get(), &OJProblemCase::modifiedChanged, this, &OJProblem::onProblemCaseModified);
    mCases.append(problemCase);
    setModified(true);
}

void OJProblem::removeCase(int idx)
{
    if(idx<0 || idx>=mCases.count())
        return;
    disconnect(mCases[idx].get());
    mCases.removeAt(idx);
    setModified(true);
}

void OJProblem::moveCase(int fromIdx, int toIdx)
{
    mCases.move(fromIdx, toIdx);
    setModified(true);
}

void OJProblem::clearCases()
{
    foreach(const POJProblemCase &problemCase, mCases) {
        disconnect(problemCase.get());
    }
    mCases.clear();
    setModified(true);
}

void OJProblem::onProblemCaseModified(const QString &caseId)
{
    emit problemCaseModified(caseId);
    foreach(const POJProblemCase &problemCase, mCases) {
        if (problemCase->id() == caseId && problemCase->isModified()) {
            setModified(true);
            break;
        }
    }
}

size_t OJProblem::memoryLimit() const
{
    return mMemoryLimit;
}

size_t OJProblem::timeLimit() const
{
    return mTimeLimit;
}


bool OJProblem::isModified() const
{
    return mModified;
}

void OJProblem::setModified(bool newModified)
{
    if (mModified == newModified)
        return;
    mModified = newModified;
    emit modifiedChanged(id());
}

OJProblem::OJProblem(QObject *parent) :
    QObject(parent),
    mTimeLimit(0),
    mMemoryLimit(0),
    mTimeLimitUnit(ProblemTimeLimitUnit::Seconds),
    mMemoryLimitUnit(ProblemMemoryLimitUnit::MB),
    mModified{false}
{
    QUuid uid = QUuid::createUuid();
    mId = uid.toString();
}

const QString &OJProblem::id() const
{
    return mId;
}

OJProblemSet::OJProblemSet(QObject *parent):QObject{parent},
    mModified{false}
{

}

bool OJProblemSet::isModified() const
{
    return mModified;
}

void OJProblemSet::setModified(bool newModified)
{
    if (mModified == newModified)
        return;
    mModified = newModified;
    emit modifiedChanged();
}

const QString &OJProblemSet::name() const
{
    return mName;
}

void OJProblemSet::setName(const QString &newName)
{
    if (mName != newName) {
        mName = newName;
        setModified(true);
    }
}

const QList<POJProblem> &OJProblemSet::problems() const
{
    return mProblems;
}

const QString &OJProblemSet::exportFilename() const
{
    return mExportFilename;
}

void OJProblemSet::setExportFilename(const QString &newExportFilename)
{
    if (mExportFilename != newExportFilename) {
        mExportFilename = newExportFilename;
        setModified(true);
    }
}

void OJProblemSet::addProblem(const POJProblem &problem)
{
    connect(problem.get(), &OJProblem::modifiedChanged, this, &OJProblemSet::onProblemModified);
    mProblems.append(problem);
    setModified(true);
}

void OJProblemSet::removeProblem(int idx)
{
    if (idx<0 || idx>=mProblems.count())
        return;
    disconnect(mProblems[idx].get());
    mProblems.removeAt(idx);
    setModified(true);
}

void OJProblemSet::clearProblems()
{
    foreach( const POJProblem &problem, mProblems) {
        disconnect(problem.get());
    }
    mProblems.clear();
    setModified(true);
}

void OJProblemSet::moveProblem(int fromIdx, int toIdx)
{
    mProblems.move(fromIdx, toIdx);
    setModified(true);
}

void OJProblemSet::onProblemModified(const QString &id)
{
    emit problemModified(id);
    foreach( const POJProblem &problem, mProblems) {
        if (problem->id() == id && problem->isModified()) {
            setModified(true);
            break;
        }
    }
}
