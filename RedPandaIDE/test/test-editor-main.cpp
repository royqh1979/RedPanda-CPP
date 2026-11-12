#include <QTest>
#include <QGuiApplication>
#include "test_editor_symbol_completion.h"
#include "test_editor_display.h"

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

    {
        TestEditorSymbolCompletion tc;
        if (suite == "all" || suite == tc.metaObject()->className())
            status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestEditorDisplay tc;
        if (suite == "all" || suite == tc.metaObject()->className())
            status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
