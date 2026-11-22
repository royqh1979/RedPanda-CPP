#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include "test_problemcasevalidator.h"
#include "src/problems/problemcasevalidator.h"

static POJProblemCase makeCase(const QString &output, const QString &expected)
{
    auto problemCase = std::make_shared<OJProblemCase>();
    problemCase->output = output;
    problemCase->setExpected(expected);
    return problemCase;
}

static QString makeLargeText(int lineCount)
{
    QString text;
    text.reserve(lineCount * 20);
    for (int i = 0; i < lineCount; ++i)
        text += QStringLiteral("line %1 value\n").arg(i);
    return text;
}

static QString writeExpectedFile(QTemporaryDir &directory, const QString &text)
{
    QString path = directory.filePath("expected.ans");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return QString();
    QByteArray data = text.toUtf8();
    if (file.write(data) != data.size())
        return QString();
    file.close();
    return path;
}

void TestProblemCaseValidator::test_exact_data()
{
    QTest::addColumn<QString>("output");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<int>("firstDiffLine");
    QTest::addColumn<int>("outputLineCounts");
    QTest::addColumn<int>("expectedLineCounts");

    QTest::newRow("same") << "same" << "same" << true << -1 << 1 << 1;
    QTest::newRow("empty") << "" << "" << true << -1 << 0 << 0;
    QTest::newRow("trailing newline on both") << "a\n" << "a\n" << true << -1 << 1 << 1;
    QTest::newRow("trailing newline on output") << "a\n" << "a" << true << -1 << 1 << 1;
    QTest::newRow("CRLF and LF") << "a\r\nb" << "a\nb\n" << true << -1 << 2 << 2;
    QTest::newRow("content mismatch") << "a\nb\nc" << "a\nB\nc" << false << 1 << 3 << 3;
    QTest::newRow("missing last line") << "a" << "a\nb" << false << 1 << 1 << 2;
    QTest::newRow("extra last line") << "a\nb" << "a" << false << 1 << 2 << 1;
    QTest::newRow("blank line versus empty") << "" << "\n" << false << 0 << 0 << 1;
    QTest::newRow("exact whitespace") << "a " << "a" << false << 0 << 1 << 1;
}

void TestProblemCaseValidator::test_exact()
{
    QFETCH(QString, output);
    QFETCH(QString, expected);
    QFETCH(bool, valid);
    QFETCH(int, firstDiffLine);
    QFETCH(int, outputLineCounts);
    QFETCH(int, expectedLineCounts);

    auto problemCase = makeCase(output, expected);
    ProblemCaseValidator validator;
    ProblemCaseValidationResult result = validator.validate(problemCase, ProblemCaseValidateType::Exact);
    QCOMPARE(result.status, valid ? ProblemCaseValidationStatus::Passed
                                  : ProblemCaseValidationStatus::Mismatch);
    QCOMPARE(result.firstDiffLine, firstDiffLine);
    QCOMPARE(result.outputLineCounts, outputLineCounts);
    QCOMPARE(result.expectedLineCounts, expectedLineCounts);
    QVERIFY(result.errorMessage.isEmpty());
}

void TestProblemCaseValidator::test_ignore_leading_trailing_spaces_data()
{
    QTest::addColumn<QString>("output");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("valid");
    QTest::newRow("spaces and tabs") << " \ta b\t " << "a b" << true;
    QTest::newRow("unicode spaces") << QString::fromUtf8("　值　") << QString::fromUtf8("值") << true;
    QTest::newRow("internal spaces differ") << "a  b" << "a b" << false;
}

void TestProblemCaseValidator::test_ignore_leading_trailing_spaces()
{
    QFETCH(QString, output);
    QFETCH(QString, expected);
    QFETCH(bool, valid);
    auto problemCase = makeCase(output, expected);
    ProblemCaseValidator validator;
    QCOMPARE(validator.validate(problemCase, ProblemCaseValidateType::IgnoreLeadingTrailingSpaces).status,
             valid ? ProblemCaseValidationStatus::Passed : ProblemCaseValidationStatus::Mismatch);
}

void TestProblemCaseValidator::test_ignore_spaces_data()
{
    QTest::addColumn<QString>("output");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("valid");
    QTest::newRow("all whitespace") << " a\t b  c " << "a b c" << true;
    QTest::newRow("unicode whitespace") << QString::fromUtf8("甲　乙") << QString::fromUtf8("甲 乙") << true;
    QTest::newRow("token contents differ") << "a b" << "ab" << false;
    QTest::newRow("whitespace-only lines") << " \t \n" << "\n" << true;
    QTest::newRow("empty document and whitespace line") << "" << " \t " << false;
}

void TestProblemCaseValidator::test_ignore_spaces()
{
    QFETCH(QString, output);
    QFETCH(QString, expected);
    QFETCH(bool, valid);
    auto problemCase = makeCase(output, expected);
    ProblemCaseValidator validator;
    QCOMPARE(validator.validate(problemCase, ProblemCaseValidateType::IgnoreSpaces).status,
             valid ? ProblemCaseValidationStatus::Passed : ProblemCaseValidationStatus::Mismatch);
}

void TestProblemCaseValidator::test_external_expected_file_and_fallback()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto problemCase = makeCase(QString::fromUtf8("文件\n2\n"), "wrong inline");
    problemCase->setExpectedOutputFileName(writeExpectedFile(directory, QString::fromUtf8("文件\r\n2\n")));
    ProblemCaseValidator validator;
    ProblemCaseValidationResult result = validator.validate(problemCase, ProblemCaseValidateType::Exact);
    QCOMPARE(result.status, ProblemCaseValidationStatus::Passed);
    QCOMPARE(result.expectedLineCounts, 2);

    problemCase->output = QString::fromUtf8("文件\nwrong\n");
    result = validator.validate(problemCase, ProblemCaseValidateType::Exact);
    QCOMPARE(result.status, ProblemCaseValidationStatus::Mismatch);
    QCOMPARE(result.firstDiffLine, 1);

    problemCase->setExpectedOutputFileName(directory.filePath("missing.ans"));
    problemCase->setExpected(problemCase->output);
    QCOMPARE(validator.validate(problemCase, ProblemCaseValidateType::Exact).status,
             ProblemCaseValidationStatus::Passed);
}

void TestProblemCaseValidator::test_non_ascii_utf8()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    ProblemCaseValidator validator;
    auto utf8Case = makeCase(QString::fromUtf8("变量✓\n"), "wrong");
    utf8Case->setExpectedOutputFileName(writeExpectedFile(directory, QString::fromUtf8("变量✓\n")));
    QCOMPARE(validator.validate(utf8Case, ProblemCaseValidateType::Exact).status,
             ProblemCaseValidationStatus::Passed);
}

void TestProblemCaseValidator::test_invalid_utf8()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString path = directory.filePath("invalid.ans");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QByteArray invalid = QByteArray::fromHex("c328");
    QCOMPARE(file.write(invalid), invalid.size());
    file.close();

    auto problemCase = makeCase("", "inline");
    problemCase->setExpectedOutputFileName(path);
    ProblemCaseValidator validator;
    ProblemCaseValidationResult result = validator.validate(problemCase, ProblemCaseValidateType::Exact);
    QCOMPARE(result.status, ProblemCaseValidationStatus::Error);
    QCOMPARE(result.firstDiffLine, -1);
    QVERIFY(result.errorMessage.contains(path));
    QVERIFY(result.errorMessage.contains("UTF-8"));
}

void TestProblemCaseValidator::test_result_fields_are_reset()
{
    auto problemCase = makeCase("bad", "good");
    ProblemCaseValidator validator;
    ProblemCaseValidationResult failed = validator.validate(problemCase, ProblemCaseValidateType::Exact);
    QCOMPARE(failed.status, ProblemCaseValidationStatus::Mismatch);
    QCOMPARE(failed.firstDiffLine, 0);
    problemCase->output = "good";
    ProblemCaseValidationResult passed = validator.validate(problemCase, ProblemCaseValidateType::Exact);
    QCOMPARE(passed.status, ProblemCaseValidationStatus::Passed);
    QCOMPARE(passed.firstDiffLine, -1);
    QCOMPARE(failed.firstDiffLine, 0);
}

void TestProblemCaseValidator::test_null_case()
{
    ProblemCaseValidator validator;
    ProblemCaseValidationResult result = validator.validate({}, ProblemCaseValidateType::Exact);
    QCOMPARE(result.status, ProblemCaseValidationStatus::Error);
    QVERIFY(!result.errorMessage.isEmpty());
}

void TestProblemCaseValidator::test_large_expected_file_data()
{
    QTest::addColumn<int>("lineCount");
    QTest::addColumn<QString>("scenario");
    QTest::newRow("below old threshold") << 4999 << "equal";
    QTest::newRow("old threshold") << 5000 << "equal";
    QTest::newRow("above old threshold") << 5001 << "equal";
    QTest::newRow("last line differs") << 20000 << "different";
    QTest::newRow("last line missing") << 20000 << "missing";
    QTest::newRow("first line differs and last line missing") << 20000 << "different and missing";
    QTest::newRow("last line extra") << 20000 << "extra";
    QTest::newRow("large equal") << 100000 << "equal";
}

void TestProblemCaseValidator::test_large_expected_file()
{
    QFETCH(int, lineCount);
    QFETCH(QString, scenario);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString expected = makeLargeText(lineCount);
    QString output = expected;
    bool valid = true;
    int firstDiffLine = -1;
    if (scenario == "different") {
        output.replace(output.lastIndexOf("value"), 5, "other");
        valid = false;
        firstDiffLine = lineCount - 1;
    } else if (scenario == "missing") {
        output = expected.left(expected.lastIndexOf('\n', expected.size() - 2) + 1);
        valid = false;
        firstDiffLine = lineCount - 1;
    } else if (scenario == "different and missing") {
        output.replace(0, 6, "wrong0");
        output = output.left(output.lastIndexOf('\n', output.size() - 2) + 1);
        valid = false;
        firstDiffLine = 0;
    } else if (scenario == "extra") {
        output += "extra\n";
        valid = false;
        firstDiffLine = lineCount;
    }
    auto problemCase = makeCase(output, "wrong inline");
    problemCase->setExpectedOutputFileName(writeExpectedFile(directory, expected));
    ProblemCaseValidator validator;
    ProblemCaseValidationResult result = validator.validate(problemCase, ProblemCaseValidateType::Exact);
    QCOMPARE(result.status, valid ? ProblemCaseValidationStatus::Passed
                                  : ProblemCaseValidationStatus::Mismatch);
    QCOMPARE(result.firstDiffLine, firstDiffLine);
    QCOMPARE(result.outputLineCounts,
             scenario == "missing" || scenario == "different and missing"
                     ? lineCount - 1 : scenario == "extra" ? lineCount + 1 : lineCount);
    QCOMPARE(result.expectedLineCounts, lineCount);
}

#ifdef REDPANDA_PROBLEMSET_PROFILE_TESTS
void TestProblemCaseValidator::test_large_expected_file_profile_data()
{
    QTest::addColumn<int>("lineCount");
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("differentAtEnd");
    for (int lineCount : {10000, 100000, 500000}) {
        for (ProblemCaseValidateType type : {ProblemCaseValidateType::Exact,
                                            ProblemCaseValidateType::IgnoreLeadingTrailingSpaces,
                                            ProblemCaseValidateType::IgnoreSpaces}) {
            for (bool different : {false, true}) {
                QString name = QStringLiteral("%1 lines, type %2, %3")
                        .arg(lineCount).arg(static_cast<int>(type))
                        .arg(different ? "last difference" : "equal");
                QTest::newRow(name.toUtf8().constData())
                        << lineCount << static_cast<int>(type) << different;
            }
        }
    }
}

void TestProblemCaseValidator::test_large_expected_file_profile()
{
    QFETCH(int, lineCount);
    QFETCH(int, type);
    QFETCH(bool, differentAtEnd);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString expected = makeLargeText(lineCount);
    QString output = expected;
    if (differentAtEnd)
        output.replace(output.lastIndexOf("value"), 5, "other");
    auto problemCase = makeCase(output, "wrong inline");
    problemCase->setExpectedOutputFileName(writeExpectedFile(directory, expected));
    ProblemCaseValidator validator;
    ProblemCaseValidateType validateType = static_cast<ProblemCaseValidateType>(type);
    QBENCHMARK {
        validator.validate(problemCase, validateType);
    }
    QCOMPARE(validator.validate(problemCase, validateType).status,
             differentAtEnd ? ProblemCaseValidationStatus::Mismatch
                            : ProblemCaseValidationStatus::Passed);
}
#endif
