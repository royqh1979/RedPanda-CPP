#include <QTest>
#include <QGuiApplication>
#include "test_cppparser.h"

int main(int argc, char *argv[]) {
    int status = 0;
    QTest::setMainSourcePath(__FILE__, QT_TESTCASE_BUILDDIR); // Optional: for source path resolution

    QApplication app(argc,argv);
    {
        TestCppParser tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
