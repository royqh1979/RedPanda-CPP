#include <QtTest>
#include <QCoreApplication>
#include "test_charpos.h"
#include "test_qdocument.h"

int main(int argc, char *argv[]) {
    int status = 0;
    QTest::setMainSourcePath(__FILE__, QT_TESTCASE_BUILDDIR); // Optional: for source path resolution

    //CharPos Test
    {
        TestCharPos tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    //QDocument Test
    {
        QSynedit::TestDocumentHelpers tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
