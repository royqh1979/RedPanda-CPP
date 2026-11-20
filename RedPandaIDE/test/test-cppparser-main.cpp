#include <QTest>
#include <QGuiApplication>
#include "test_cppparser.h"
#include "test_cppparserinvalidation.h"
#include "test_cppparsertyntax.h"
#include "test_cppparserstandardlibrary.h"
#include "test_cpppreprocessor.h"
#include "test_cpppreprocessor_include.h"
#include "test_cpptokenizer.h"
#include "test_cppdeclarationparser.h"
#include "test_cppstatementbuilder.h"
#include "test_cpptyperesolver.h"
#include "test_cppexpressionevaluator.h"
#include "test_pptoken.h"
#include "test_ppexpander.h"

namespace {
template<typename T>
void runTestSuite(const QString &suite, int argc, char **argv, int &status)
{
    T test;
    QString className = test.metaObject()->className();
    className = className.split("::").last();
    if (suite == className || suite == "all")
        status |= QTest::qExec(&test, argc, argv);
}
}

int main(int argc, char *argv[])
{
    int status = 0;
    QTest::setMainSourcePath(__FILE__, QT_TESTCASE_BUILDDIR);

    QApplication app(argc, argv);
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

    initParser();
    runTestSuite<TestCppPreprocessor>(suite, argc, argv, status);
    runTestSuite<TestCppPreprocessorInclude>(suite, argc, argv, status);
    runTestSuite<TestCppTokenizer>(suite, argc, argv, status);
    runTestSuite<TestCppDeclarationParser>(suite, argc, argv, status);
    runTestSuite<TestCppStatementBuilder>(suite, argc, argv, status);
    runTestSuite<TestCppTypeResolver>(suite, argc, argv, status);
    runTestSuite<TestCppExpressionEvaluator>(suite, argc, argv, status);
    runTestSuite<TestPpTokenizer>(suite, argc, argv, status);
    runTestSuite<TestPpExpander>(suite, argc, argv, status);
    runTestSuite<TestCppParser>(suite, argc, argv, status);
    runTestSuite<TestCppParserInvalidation>(suite, argc, argv, status);
    runTestSuite<TestCppParserSyntax>(suite, argc, argv, status);
#ifdef REDPANDA_PARSER_STDLIB_TEST
    runTestSuite<TestCppParserStandardLibrary>(suite, argc, argv, status);
#endif

    return status;
}
