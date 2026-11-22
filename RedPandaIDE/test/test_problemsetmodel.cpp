#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include "test_problemsetmodel.h"
#include "src/iconsmanager.h"
#include "src/utils.h"
#include "src/widgets/ojproblemsetmodel.h"

static POJProblem makeProblem(const QString &name, const QString &caseName)
{
    auto problem = std::make_shared<OJProblem>();
    problem->setName(name);
    problem->setUrl("https://example.com/" + name);
    problem->setDescription(QString::fromUtf8("描述 ") + name);
    problem->setHint("hint");
    problem->setTimeLimit(1250);
    problem->setTimeLimitUnit(ProblemTimeLimitUnit::Milliseconds);
    problem->setMemoryLimit(64);
    problem->setMemoryLimitUnit(ProblemMemoryLimitUnit::MB);
    auto problemCase = std::make_shared<OJProblemCase>();
    problemCase->setName(caseName);
    problemCase->setInput(QString::fromUtf8("输入 1\n"));
    problemCase->setExpected(QString::fromUtf8("输出 1\n"));
    problem->addCase(problemCase);
    return problem;
}

static void writeTextFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(content), content.size());
}

void TestProblemSetModel::test_problem_set_json_roundtrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString answerPath = directory.filePath("answer.cpp");
    writeTextFile(answerPath, "int main() {}\n");

    OJProblemSetModel model;
    model.create(QString::fromUtf8("试题集"));
    auto first = makeProblem("A", "case A");
    first->setAnswerProgram(answerPath);
    auto second = makeProblem("B", "case B");
    model.addProblem(first);
    model.addProblem(second);
    QString path = directory.filePath("set.json");
    model.saveToFile(path, true, 1);
    QVERIFY(!model.problemSet()->isModified());
    QCOMPARE(model.filePath(), path);

    OJProblemSetModel loaded;
    int currentIndex = -1;
    loaded.loadFromFile(path, true, currentIndex);
    QCOMPARE(currentIndex, 1);
    QCOMPARE(loaded.filePath(), path);
    QCOMPARE(loaded.name(), model.name());
    QCOMPARE(loaded.exportFilename(), path);
    QCOMPARE(loaded.rowCount(QModelIndex()), 2);
    QVERIFY(!loaded.problemSet()->isModified());
    for (int i = 0; i < 2; ++i) {
        POJProblem actual = loaded.problem(i);
        POJProblem expected = model.problem(i);
        QCOMPARE(actual->name(), expected->name());
        QCOMPARE(actual->url(), expected->url());
        QCOMPARE(actual->description(), expected->description());
        QCOMPARE(actual->hint(), expected->hint());
        QCOMPARE(actual->answerProgram(), expected->answerProgram());
        QCOMPARE(actual->timeLimit(), expected->timeLimit());
        QCOMPARE(actual->timeLimitUnit(), expected->timeLimitUnit());
        QCOMPARE(actual->memoryLimit(), expected->memoryLimit());
        QCOMPARE(actual->memoryLimitUnit(), expected->memoryLimitUnit());
        QCOMPARE(actual->cases().count(), 1);
        QCOMPARE(actual->cases().at(0)->name(), expected->cases().at(0)->name());
        QCOMPARE(actual->cases().at(0)->input(), expected->cases().at(0)->input());
        QCOMPARE(actual->cases().at(0)->expected(), expected->cases().at(0)->expected());
        QVERIFY(!actual->isModified());
        QVERIFY(!actual->cases().at(0)->isModified());
    }
}

void TestProblemSetModel::test_problem_set_paths_and_write_failure()
{
    QTemporaryDir directory;
    QTemporaryDir outsideDirectory;
    QVERIFY(directory.isValid());
    QVERIFY(outsideDirectory.isValid());
    QVERIFY(QDir(directory.path()).mkpath("cases"));
    QString inputPath = directory.filePath("cases/input.in");
    QString expectedPath = directory.filePath("cases/expected.ans");
    QString outsidePath = outsideDirectory.filePath("outside.in");
    writeTextFile(inputPath, "1\n");
    writeTextFile(expectedPath, "2\n");
    writeTextFile(outsidePath, "3\n");

    OJProblemSetModel model;
    model.create("paths");
    auto problem = makeProblem("A", "case A");
    problem->cases().at(0)->setInputFileName(inputPath);
    problem->cases().at(0)->setExpectedOutputFileName(expectedPath);
    model.addProblem(problem);
    auto second = makeProblem("B", "case B");
    second->cases().at(0)->setInputFileName(outsidePath);
    model.addProblem(second);
    QString path = directory.filePath("paths.json");
    model.saveToFile(path, false);

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    QJsonArray problems = root.value("problems").toArray();
    QCOMPARE(problems.at(0).toObject().value("cases").toArray().at(0).toObject().value("input_filename").toString(),
             "%ProblemSetPath%/cases/input.in");
    QCOMPARE(problems.at(0).toObject().value("cases").toArray().at(0).toObject().value("expected_output_filename").toString(),
             "%ProblemSetPath%/cases/expected.ans");
    QCOMPARE(problems.at(1).toObject().value("cases").toArray().at(0).toObject().value("input_filename").toString(),
             outsidePath);

    OJProblemSetModel loaded;
    int currentIndex;
    loaded.loadFromFile(path, false, currentIndex);
    QCOMPARE(loaded.problem(0)->cases().at(0)->inputFileName(), inputPath);
    QCOMPARE(loaded.problem(0)->cases().at(0)->expectedOutputFileName(), expectedPath);
    QCOMPARE(loaded.problem(1)->cases().at(0)->inputFileName(), outsidePath);
    QVERIFY(loaded.filePath().isEmpty());

    QVERIFY_EXCEPTION_THROWN(model.saveToFile(directory.path(), false), FileError);
    QString malformedPath = directory.filePath("malformed.json");
    writeTextFile(malformedPath, "{not json");
    QVERIFY_EXCEPTION_THROWN(loaded.loadFromFile(malformedPath, false, currentIndex), FileError);
    QVERIFY_EXCEPTION_THROWN(loaded.loadFromFile(directory.filePath("missing.json"), false, currentIndex), FileError);
}

void TestProblemSetModel::test_problem_set_model_operations()
{
    OJProblemSetModel model;
    model.create("set");
    model.addProblem(makeProblem("A", "a"));
    model.addProblem(makeProblem("B", "b"));
    model.addProblem(makeProblem("C", "c"));
    QVERIFY(model.problemNameUsed("B"));
    QVERIFY(!model.problemNameUsed("D"));

    QModelIndex second = model.index(1);
    QCOMPARE(model.data(second, Qt::DisplayRole).toString(), "B[*]");
    QVERIFY(model.setData(second, "renamed", Qt::EditRole));
    QCOMPARE(model.problem(1)->name(), "renamed");
    QVERIFY(!model.setData(second, "", Qt::EditRole));
    QVERIFY(model.flags(second).testFlag(Qt::ItemIsEditable));
    QVERIFY(model.moveRows(QModelIndex(), 0, 1, QModelIndex(), 2));
    QCOMPARE(model.problem(0)->name(), "renamed");
    QCOMPARE(model.problem(1)->name(), "A");
    QVERIFY(!model.moveRows(QModelIndex(), 0, 1, QModelIndex(), 4));
    model.removeProblem(1);
    QCOMPARE(model.rowCount(QModelIndex()), 2);
    model.removeAllProblems();
    QCOMPARE(model.rowCount(QModelIndex()), 0);
}

void TestProblemSetModel::test_problem_model_operations()
{
    IconsManager iconsManager(nullptr, "test");
    OJProblemModel model(&iconsManager);
    auto problem = std::make_shared<OJProblem>();
    problem->setName("problem");
    model.setProblem(problem);
    auto first = std::make_shared<OJProblemCase>();
    auto second = std::make_shared<OJProblemCase>();
    first->setName("first");
    second->setName("second");
    model.addCase(first);
    model.addCase(second);
    QCOMPARE(model.rowCount(QModelIndex()), 2);
#ifdef Q_OS_WIN
    QCOMPARE(model.columnCount(QModelIndex()), 3);
#else
    QCOMPARE(model.columnCount(QModelIndex()), 2);
#endif
    QCOMPARE(model.getCaseById(first->id()), first);
    QCOMPARE(model.getCaseIndexById(second->id()), 1);
    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), "first[*]");
    QVERIFY(model.setData(model.index(0, 0), "renamed", Qt::EditRole));
    QCOMPARE(first->name(), "renamed");

    first->testState = ProblemCaseTestState::Passed;
    second->testState = ProblemCaseTestState::Failed;
    QCOMPARE(model.getTitle(), "problem[*] (1/2)");
    std::unique_ptr<QMimeData> mimeData(model.mimeData({model.index(0, 0)}));
    QVERIFY(mimeData);
    QVERIFY(model.dropMimeData(mimeData.get(), Qt::MoveAction, 2, 0, QModelIndex()));
    QVERIFY(model.removeRows(0, 1, QModelIndex()));
    QCOMPARE(model.getCase(0)->name(), second->name());
    QCOMPARE(model.getCase(1)->name(), first->name());
    model.removeCase(0);
    QCOMPARE(model.count(), 1);
    model.clear();
    QCOMPARE(model.count(), 0);
}
