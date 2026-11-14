#include <QCoreApplication>
#include <QTest>
#include "test_cpptokenizer.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    TestCppTokenizer test;
    return QTest::qExec(&test, argc, argv);
}
