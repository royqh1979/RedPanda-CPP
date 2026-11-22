#include <QApplication>
#include <QLocale>
#include <QTest>
#include "qt_utils/charsetinfo.h"
#include "test_problemset.h"
#include "test_problemsetmodel.h"
#include "test_freeproblemsetformat.h"
#include "test_problemcasevalidator.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    CharsetInfoManager charsetInfoManager(QLocale::system().name());
    pCharsetInfoManager = &charsetInfoManager;

    QString suite = "all";
    QVector<char *> qtArgs{argv[0]};
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--suite" && i + 1 < argc)
            suite = QString::fromLocal8Bit(argv[++i]);
        else if (arg.startsWith("--suite="))
            suite = arg.mid(QString("--suite=").size());
        else
            qtArgs.push_back(argv[i]);
    }
    argc = qtArgs.size();
    argv = qtArgs.data();

    int status = 0;
    auto runSuite = [&](QObject &test) {
        QString className = QString::fromLatin1(test.metaObject()->className());
        if (suite == "all" || suite == className)
            status |= QTest::qExec(&test, argc, argv);
    };
    TestProblemSet problemSetTest;
    runSuite(problemSetTest);
    TestProblemSetModel problemSetModelTest;
    runSuite(problemSetModelTest);
    TestFreeProblemSetFormat freeProblemSetFormatTest;
    runSuite(freeProblemSetFormatTest);
    TestProblemCaseValidator problemCaseValidatorTest;
    runSuite(problemCaseValidatorTest);
    pCharsetInfoManager = nullptr;
    return status;
}
