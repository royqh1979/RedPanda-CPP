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
#include <QFile>

namespace {

class TextLineCursor
{
public:
    explicit TextLineCursor(const QString &text)
        : mText(text)
    {
    }

    bool next(QStringView &line)
    {
        if (mPosition >= mText.size())
            return false;
        int end = mText.indexOf('\n', mPosition);
        if (end < 0) {
            line = QStringView(mText).mid(mPosition);
            mPosition = mText.size();
        } else {
            line = QStringView(mText).mid(mPosition, end - mPosition);
            mPosition = end + 1;
        }
        if (line.endsWith('\r'))
            line.chop(1);
        return true;
    }

private:
    const QString &mText;
    int mPosition = 0;
};

}

ProblemCaseValidator::ProblemCaseValidator()
{

}

ProblemCaseValidationResult ProblemCaseValidator::validate(
        const POJProblemCase &problemCase, ProblemCaseValidateType type)
{
    ProblemCaseValidationResult result;
    if (!problemCase) {
        result.status = ProblemCaseValidationStatus::Error;
        result.errorMessage = QObject::tr("No problem case to validate.");
        return result;
    }

    QString expected;
    if (fileExists(problemCase->expectedOutputFileName())) {
        QString path = problemCase->expectedOutputFileName();
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            result.status = ProblemCaseValidationStatus::Error;
            result.errorMessage = QObject::tr("Can't open expected output file '%1' for read.")
                    .arg(path);
            return result;
        }
        TextDecoder decoder = TextDecoder::decoderForUtf8();
        auto decoded = decoder.decode(file.readAll());
        if (!decoded.first) {
            result.status = ProblemCaseValidationStatus::Error;
            result.errorMessage = QObject::tr("Expected output file '%1' is not valid UTF-8.")
                    .arg(path);
            return result;
        }
        expected = decoded.second;
    } else {
        expected = problemCase->expected();
    }

    TextLineCursor outputCursor(problemCase->output);
    TextLineCursor expectedCursor(expected);
    QStringView outputLine;
    QStringView expectedLine;
    bool hasOutput = outputCursor.next(outputLine);
    bool hasExpected = expectedCursor.next(expectedLine);
    while (hasOutput || hasExpected) {
        int line = hasOutput ? result.outputLineCounts : result.expectedLineCounts;
        if (result.firstDiffLine < 0) {
            if (hasOutput != hasExpected) {
                result.firstDiffLine = line;
            } else if (hasOutput) {
                bool equal = false;
                switch (type) {
                case ProblemCaseValidateType::Exact:
                    equal = outputLine == expectedLine;
                    break;
                case ProblemCaseValidateType::IgnoreLeadingTrailingSpaces:
                    equal = outputLine.trimmed() == expectedLine.trimmed();
                    break;
                case ProblemCaseValidateType::IgnoreSpaces:
                    equal = equalIgnoringSpaces(outputLine, expectedLine);
                    break;
                }
                if (!equal)
                    result.firstDiffLine = line;
            }
        }
        if (hasOutput)
            ++result.outputLineCounts;
        if (hasExpected)
            ++result.expectedLineCounts;
        hasOutput = outputCursor.next(outputLine);
        hasExpected = expectedCursor.next(expectedLine);
    }
    result.status = result.firstDiffLine < 0
            ? ProblemCaseValidationStatus::Passed : ProblemCaseValidationStatus::Mismatch;
    return result;
}

bool ProblemCaseValidator::equalIgnoringSpaces(QStringView s1, QStringView s2)
{
    const QChar *p1 = s1.data();
    const QChar *p2 = s2.data();
    const QChar *end1 = p1 + s1.size();
    const QChar *end2 = p2 + s2.size();
    while (true) {
        while (p1 < end1 && p1->isSpace())
            ++p1;
        while (p2 < end2 && p2->isSpace())
            ++p2;
        if (p1 == end1 || p2 == end2)
            return p1 == end1 && p2 == end2;
        const QChar *start1 = p1;
        const QChar *start2 = p2;
        while (p1 < end1 && !p1->isSpace())
            ++p1;
        while (p2 < end2 && !p2->isSpace())
            ++p2;
        if (p1 - start1 != p2 - start2)
            return false;
        for (const QChar *a = start1, *b = start2; a < p1; ++a, ++b) {
            if (*a != *b)
                return false;
        }
    }
}
