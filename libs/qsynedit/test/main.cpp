#include <QtTest>
#include <QGuiApplication>
#include "test_charpos.h"
#include "test_document.h"
#include "test_qsynedit.h"
#include "qsynedit/types.h"

int main(int argc, char *argv[]) {
    int status = 0;
    QTest::setMainSourcePath(__FILE__, QT_TESTCASE_BUILDDIR); // Optional: for source path resolution

    QApplication app(argc,argv);
    //CharPos Test
    {
        TestCharPos tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        QSynedit::TestDocumentHelpers tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    //QDocument Test
    {
        QSynedit::TestDocument tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        QSynedit::TestQSyneditCpp tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
