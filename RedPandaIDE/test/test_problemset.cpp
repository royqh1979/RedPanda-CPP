#include <QSignalSpy>
#include <QTest>
#include "test_problemset.h"
#include "src/problems/ojproblemset.h"

void TestProblemSet::test_case_modification_propagates()
{
    auto problem = std::make_shared<OJProblem>();
    auto problemCase = std::make_shared<OJProblemCase>();
    problem->addCase(problemCase);
    problem->setModified(false);
    problemCase->setModified(false);

    QSignalSpy caseSpy(problem.get(), &OJProblem::problemCaseModified);
    QSignalSpy problemSpy(problem.get(), &OJProblem::modifiedChanged);
    problemCase->setInput("new input");

    QVERIFY(problemCase->isModified());
    QVERIFY(problem->isModified());
    QCOMPARE(caseSpy.count(), 1);
    QCOMPARE(caseSpy.takeFirst().at(0).toString(), problemCase->id());
    QCOMPARE(problemSpy.count(), 1);
}

void TestProblemSet::test_problem_case_operations()
{
    auto problem = std::make_shared<OJProblem>();
    auto first = std::make_shared<OJProblemCase>();
    auto second = std::make_shared<OJProblemCase>();
    auto third = std::make_shared<OJProblemCase>();
    first->setName("first");
    second->setName("second");
    third->setName("third");
    problem->addCase(first);
    problem->addCase(second);
    problem->addCase(third);

    problem->moveCase(0, 2);
    QCOMPARE(problem->cases().at(0), second);
    QCOMPARE(problem->cases().at(1), third);
    QCOMPARE(problem->cases().at(2), first);

    problem->removeCase(-1);
    problem->removeCase(3);
    QCOMPARE(problem->cases().count(), 3);
    problem->removeCase(1);
    QCOMPARE(problem->cases().count(), 2);
    QCOMPARE(problem->cases().at(0), second);
    QCOMPARE(problem->cases().at(1), first);

    problem->clearCases();
    QVERIFY(problem->cases().isEmpty());
}

void TestProblemSet::test_set_problem_operations()
{
    OJProblemSet set;
    auto first = std::make_shared<OJProblem>();
    auto second = std::make_shared<OJProblem>();
    first->setName("first");
    second->setName("second");
    set.addProblem(first);
    set.addProblem(second);
    first->setModified(false);
    second->setModified(false);
    set.setModified(false);

    QSignalSpy problemSpy(&set, &OJProblemSet::problemModified);
    QSignalSpy modifiedSpy(&set, &OJProblemSet::modifiedChanged);
    first->setName("renamed");
    QVERIFY(set.isModified());
    QCOMPARE(problemSpy.count(), 1);
    QCOMPARE(problemSpy.takeFirst().at(0).toString(), first->id());
    QCOMPARE(modifiedSpy.count(), 1);

    set.moveProblem(0, 1);
    QCOMPARE(set.problems().at(0), second);
    QCOMPARE(set.problems().at(1), first);
    set.removeProblem(-1);
    set.removeProblem(2);
    QCOMPARE(set.problems().count(), 2);
    set.removeProblem(0);
    QCOMPARE(set.problems().count(), 1);
    QCOMPARE(set.problems().at(0), first);
    set.clearProblems();
    QVERIFY(set.problems().isEmpty());
}

void TestProblemSet::test_limit_conversions()
{
    OJProblem problem;
    problem.setTimeLimit(3);
    problem.setTimeLimitUnit(ProblemTimeLimitUnit::Seconds);
    QCOMPARE(problem.getTimeLimitInMilliseconds(), 3000);
    problem.setTimeLimitUnit(ProblemTimeLimitUnit::Milliseconds);
    QCOMPARE(problem.getTimeLimitInMilliseconds(), 3);

    problem.setMemoryLimit(2);
    problem.setMemoryLimitUnit(ProblemMemoryLimitUnit::KB);
    QCOMPARE(problem.getMemoryLimitInBytes(), 2048);
    problem.setMemoryLimitUnit(ProblemMemoryLimitUnit::MB);
    QCOMPARE(problem.getMemoryLimitInBytes(), 2 * 1024 * 1024);
    problem.setMemoryLimitUnit(ProblemMemoryLimitUnit::GB);
    QCOMPARE(problem.getMemoryLimitInBytes(), 2ULL * 1024 * 1024 * 1024);
}

void TestProblemSet::test_all_property_changes_mark_modified()
{
    const QList<std::function<void(OJProblem &)>> problemChanges{
        [](OJProblem &problem) { problem.setName("name"); },
        [](OJProblem &problem) { problem.setUrl("https://example.com"); },
        [](OJProblem &problem) { problem.setDescription("description"); },
        [](OJProblem &problem) { problem.setHint("hint"); },
        [](OJProblem &problem) { problem.setAnswerProgram("answer.cpp"); },
        [](OJProblem &problem) { problem.setTimeLimit(1); },
        [](OJProblem &problem) { problem.setMemoryLimit(1); },
        [](OJProblem &problem) { problem.setTimeLimitUnit(ProblemTimeLimitUnit::Milliseconds); },
        [](OJProblem &problem) { problem.setMemoryLimitUnit(ProblemMemoryLimitUnit::KB); },
    };
    for (const auto &change : problemChanges) {
        OJProblem problem;
        QVERIFY(!problem.isModified());
        change(problem);
        QVERIFY(problem.isModified());
    }

    const QList<std::function<void(OJProblemCase &)>> caseChanges{
        [](OJProblemCase &problemCase) { problemCase.setName("name"); },
        [](OJProblemCase &problemCase) { problemCase.setInput("input"); },
        [](OJProblemCase &problemCase) { problemCase.setExpected("expected"); },
        [](OJProblemCase &problemCase) { problemCase.setInputFileName("input.in"); },
        [](OJProblemCase &problemCase) { problemCase.setExpectedOutputFileName("expected.ans"); },
    };
    for (const auto &change : caseChanges) {
        OJProblemCase problemCase;
        QVERIFY(!problemCase.isModified());
        change(problemCase);
        QVERIFY(problemCase.isModified());
    }
}
