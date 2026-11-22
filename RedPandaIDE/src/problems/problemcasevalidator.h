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
#ifndef PROBLEMCASEVALIDATOR_H
#define PROBLEMCASEVALIDATOR_H

#include "ojproblemset.h"
#include "../utils.h"

enum class ProblemCaseValidationStatus {
    Passed,
    Mismatch,
    Error
};

struct ProblemCaseValidationResult {
    ProblemCaseValidationStatus status = ProblemCaseValidationStatus::Mismatch;
    int outputLineCounts = 0;
    int expectedLineCounts = 0;
    int firstDiffLine = -1;
    QString errorMessage;
};

class ProblemCaseValidator
{
public:
    ProblemCaseValidator();
    ProblemCaseValidationResult validate(const POJProblemCase &problemCase,
                                         ProblemCaseValidateType type);
private:
    bool equalIgnoringSpaces(QStringView s1, QStringView s2);
};

#endif // PROBLEMCASEVALIDATOR_H
