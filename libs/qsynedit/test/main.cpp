#include <QtTest>
#include <QGuiApplication>
#include "test_charpos.h"
#include "test_cppsyntaxer.h"
#include "test_document.h"
#include "test_qsynedit_cpp.h"
#include "test_qsynedit_display.h"
#include "test_qsynedit_emoji.h"
#include "test_qsynedit_cpp_search_replace.h"
#include "test_qsynedit_infra.h"
#include "test_rowindex.h"
#include "qsynedit/types.h"

int main(int argc, char *argv[]) {
    int status = 0;
    QTest::setMainSourcePath(__FILE__, QT_TESTCASE_BUILDDIR); // Optional: for source path resolution

    QApplication app(argc,argv);
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

    argc = qtArgs.length();
    argv = qtArgs.data();
    //Test infrastructure
    {
        QSynedit::TestQSyneditInfra tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }
    //CharPos Test
    {
        QSynedit::TestCharPos tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }
    //RowIndex Test
    {
        QSynedit::TestRowIndex tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }
    {
        QSynedit::TestDocumentHelpers tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }

    //QDocument Test
    {
        QSynedit::TestDocument tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }

    //Cpp Syntaxer Test
    {
        TestCppSyntaxer tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }

    //SynEdit Test
    {
        QSynedit::TestQSyneditCpp tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }

    {
        QSynedit::TestQSyneditDisplay tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }

    {
        QSynedit::TestQSyneditEmoji tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }
    {
        QSynedit::TestQSyneditCppSearchReplace tc;
        QString className = tc.metaObject()->className();
        className = className.split("::").last();
        if (suite == className || suite == "all")
            status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
