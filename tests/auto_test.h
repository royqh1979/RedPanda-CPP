#ifndef AUTOTEST_H
#define AUTOTEST_H

#include <QtTest/qtest.h>
#include <QList>
#include <QString>
#include <QSharedPointer>
#include <QApplication>
#include <QElapsedTimer>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QCommandLineParser>
#include <QMap>

namespace AutoTest {

using TestList =  QList<QObject*>;
using TestMap =   QMap<QString, QObject*>;

inline TestList& testList() {
    static TestList list;
    return list;
}

inline TestMap& testMap() {
    static TestMap map;
    return map;
}

inline bool findObject(QObject* object) {
    TestList& list = testList();
    if (list.contains(object)) {
        return true;
    }
    foreach (QObject* test, list) {
        if (test->objectName() == object->objectName()) {
            return true;
        }
    }
    return false;
}

inline void addTest(QObject* object) {
    TestList& list = testList();
    TestMap& map = testMap();

    if (!findObject(object)) {
        list.append(object);
        map.insert(object->objectName(), object);
    }
}

struct TestResult {
    QString testName;
    int executionTime; // ms
    int passedCount;
    int failedCount;
    int skippedCount;
};

inline int run(int argc, char *argv[], const QString& outputFile = "") {
    QCoreApplication app(argc, argv);
    app.setApplicationName("AutoTest Runner");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("AutoTest Runner");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption listOption("list", "List all available tests");
    parser.addOption(listOption);

    QCommandLineOption filterOption(QStringList() << "f" << "filter", "Run tests matching <pattern>", "pattern");
    parser.addOption(filterOption);

    QCommandLineOption fileOption(QStringList() << "file", "Output to file", "filename");
    parser.addOption(fileOption);
    parser.process(app);

    if (parser.isSet(listOption)) {
        qDebug() << "Available tests:";
        foreach (QObject* test, testList()) {
            qDebug() << "  " << test->objectName();
        }
        return 0;
    }

    QString outputFilename = parser.value(fileOption);
    if (!outputFilename.isEmpty()) {
        qDebug() << "Output will be saved to:" << outputFilename;
    }

    QString filterPattern = parser.value(filterOption);
    QList<QObject*> testsToRun;
    if (filterPattern.isEmpty()) {
        testsToRun = testList();
    } else {
        foreach (QObject* test, testList()) {
            if (test->objectName().contains(filterPattern, Qt::CaseInsensitive)) {
                testsToRun.append(test);
            }
        }
    }

    if (testsToRun.isEmpty()) {
        qWarning() << "No tests match the filter pattern:" << filterPattern;
        return 1;
    }

    QList<TestResult> results;
    int totalPassed = 0;
    int totalFailed = 0;
    int totalSkipped = 0;
    QElapsedTimer totalTimer;
    totalTimer.start();

    foreach (QObject* test, testsToRun) {
        TestResult result;
        result.testName = test->objectName();

        QElapsedTimer timer;
        timer.start();

        int ret = QTest::qExec(test, argc, argv);

        result.executionTime = timer.elapsed();
        result.failedCount = ret;
        result.passedCount = 1;
        result.skippedCount = 0;

        results.append(result);

        totalPassed += result.passedCount;
        totalFailed += result.failedCount;
        totalSkipped += result.skippedCount;
    }

    int totalTime = totalTimer.elapsed();

    QTextStream out(stdout);
    QFile outputFileHandle;
    if (!outputFilename.isEmpty()) {
        outputFileHandle.setFileName(outputFilename);
        if (outputFileHandle.open(QIODevice::WriteOnly | QIODevice::Text)) {
            out.setDevice(&outputFileHandle);
        } else {
            qWarning() << "Failed to open output file:" << outputFilename;
        }
    }
    out << "\nTest Results Summary\n";
    out << "====================\n";
    out << "Total Tests: " << testsToRun.size() << "\n";
    out << "Total Passed: " << totalPassed << "\n";
    out << "Total Failed: " << totalFailed << "\n";
    out << "Total Skipped: " << totalSkipped << "\n";
    out << "Total Time: " << totalTime << " ms\n\n";

    out << "Detailed Results:\n";
    foreach (const TestResult &result, results) {
        out << "  " << result.testName << " - "
            << "Time: " << result.executionTime << " ms, "
            << "Passed: " << result.passedCount << ", "
            << "Failed: " << result.failedCount << "\n";
    }

    if (outputFileHandle.isOpen()) {
        outputFileHandle.close();
    }

    return totalFailed;
}

} // namespace AutoTest

template <class T>
class Test {
public:
    QSharedPointer<T> child;

    Test(const QString& name) : child(new T) {
        child->setObjectName(name);
        AutoTest::addTest(child.data());
    }
};

#define DECLARE_TEST(className) static Test<className> t(#className);

#define TEST_MAIN                                   \
int main(int argc, char *argv[]) {                  \
        QCoreApplication app(argc, argv);           \
        app.setAttribute(Qt::AA_Use96Dpi, true);    \
        QTEST_DISABLE_KEYPAD_NAVIGATION             \
        return AutoTest::run(argc, argv);           \
}

// 测试用例分组
#define TEST_GROUP_BEGIN(groupName)                 \
namespace groupName {                               \
    class GroupTest : public QObject {              \
        Q_OBJECT \
        private Q_SLOTS:

#define TEST_GROUP_END(groupName)                   \
};                                                  \
DECLARE_TEST(GroupTest)                             \
}

#endif // AUTOTEST_H

