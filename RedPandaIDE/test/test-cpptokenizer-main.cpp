#include <QCoreApplication>
#include <QTest>
#include "test_cpptokenizer.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QString suite = "all";
    QVector<char *> qtArgs;
    qtArgs.push_back(argv[0]);
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--suite") {
            if (i + 1 >= argc) {
                qWarning("--suite requires a name");
                return 2;
            }
            suite = QString::fromLocal8Bit(argv[++i]);
        } else if (arg.startsWith("--suite=")) {
            suite = arg.mid(QString("--suite=").size());
        } else {
            qtArgs.push_back(argv[i]);
        }
    }

    TestCppTokenizer test;
    if (suite == "all" || suite == test.metaObject()->className())
        return QTest::qExec(&test, qtArgs.length(), qtArgs.data());
    return 0;
}
