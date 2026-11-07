#include "test_cpppreprocessor_include.h"
#include "src/parser/cpppreprocessor.h"
#include "qt_utils/utils.h"
#include <QFileInfo>
#include <QTest>

TestCppPreprocessorInclude::TestCppPreprocessorInclude(QObject *parent):
    QObject{parent}
{

}

QString TestCppPreprocessorInclude::scenarioPath(const QString &name)
{
    QFileInfo info(QString("resources/preprocessor/%1").arg(name));
    return info.absoluteFilePath();
}

QStringList TestCppPreprocessorInclude::normalizedResult(CppPreprocessor &p, const QString &scenarioDir)
{
    QStringList result = p.result();
    for (QString& line : result) {
        line.replace(scenarioDir, "<DIR>");
    }
    return result;
}

QStringList TestCppPreprocessorInclude::readGolden(const QString &fileName)
{
    QStringList lines = readFileToLines(fileName);
    return lines;
}

void TestCppPreprocessorInclude::test_basic_include()
{
    QString dir = scenarioPath("basic_include");
    CppPreprocessor preprocessor;
    preprocessor.preprocess(dir + "/main.cpp");
    QCOMPARE(normalizedResult(preprocessor, dir),
             readGolden(dir + "/main-result.cpp"));
}

QStringList TestCppPreprocessorInclude::runScenario(CppPreprocessor &p, const QString &dir,
                                                     const QString &mainFile, bool includeOnce)
{
    p.setFileOnlyIncludeOnce(includeOnce);
    p.preprocess(dir + "/" + mainFile);
    return normalizedResult(p, dir);
}

void TestCppPreprocessorInclude::test_nested_include()
{
    QString dir = scenarioPath("nested_include");
    CppPreprocessor p;
    QStringList result = runScenario(p, dir, "main.cpp", true);
    QCOMPARE(result, QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "int before;",
                 "#include <DIR>/a.h:-1",
                 "int in_a1;",
                 "#include <DIR>/b.h:-1",
                 "int in_b;",
                 "#include <DIR>/a.h:1",
                 "int in_a2;",
                 "#include <DIR>/main.cpp:1",
                 "int after;"}));
    // include relations are recorded transitively
    PParsedFileInfo mainInfo = p.findFileInfo(dir + "/main.cpp");
    QVERIFY(mainInfo);
    QCOMPARE(mainInfo->directIncludes(), QStringList({dir + "/a.h"}));
    QVERIFY(mainInfo->includes().contains(dir + "/a.h"));
    QVERIFY(mainInfo->includes().contains(dir + "/b.h"));
    PParsedFileInfo aInfo = p.findFileInfo(dir + "/a.h");
    QVERIFY(aInfo);
    QCOMPARE(aInfo->directIncludes(), QStringList({dir + "/b.h"}));
    QVERIFY(aInfo->includes().contains(dir + "/b.h"));
}

void TestCppPreprocessorInclude::test_include_guard()
{
    // A header with the classic #ifndef include guard is only processed
    // once; the second #include leaves an empty line in the result.
    // This holds in both include-once modes (in parser mode the guard is
    // detected and the file is removed from the could-repeat set).
    QString dir = scenarioPath("include_guard");
    QStringList expected{
        "#include <DIR>/main.cpp:-1",
        "#include <DIR>/guard.h:-1",
        "",
        "#define GUARD_H",
        "int guarded;",
        "",
        "#include <DIR>/main.cpp:0",
        "",
        "int after;"};
    for (bool includeOnce : {true, false}) {
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main.cpp", includeOnce), expected);
        QVERIFY(p.getDefine("GUARD_H"));
        PParsedFileInfo mainInfo = p.findFileInfo(dir + "/main.cpp");
        QVERIFY(mainInfo);
        QCOMPARE(mainInfo->directIncludes(),
                 QStringList({dir + "/guard.h", dir + "/guard.h"}));
    }
}

void TestCppPreprocessorInclude::test_pragma_once()
{
    QString dir = scenarioPath("pragma_once");
    QStringList expected{
        "#include <DIR>/main.cpp:-1",
        "#include <DIR>/once.h:-1",
        "",
        "int onced;",
        "#include <DIR>/main.cpp:0",
        "",
        "int after;"};
    for (bool includeOnce : {true, false}) {
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main.cpp", includeOnce), expected);
    }
}

void TestCppPreprocessorInclude::test_no_guard()
{
    // THE behavioral difference between the two include-once modes:
    // - include-once mode (default): every scanned file is treated as
    //   include-once, so the second #include just replays defines and
    //   leaves an empty line;
    // - parser mode (fileOnlyIncludeOnce=false, what CppParser uses):
    //   an unguarded header is re-scanned on every #include (correct for
    //   headers like <cassert> that are meant to be re-includable).
    QString dir = scenarioPath("no_guard");
    {
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                     "#include <DIR>/main.cpp:-1",
                     "#include <DIR>/noguard.h:-1",
                     "int unguarded;",
                     "#include <DIR>/main.cpp:0",
                     "",
                     "int after;"}));
    }
    {
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main.cpp", false), QStringList({
                     "#include <DIR>/main.cpp:-1",
                     "#include <DIR>/noguard.h:-1",
                     "int unguarded;",
                     "#include <DIR>/main.cpp:0",
                     "#include <DIR>/noguard.h:-1",
                     "int unguarded;",
                     "#include <DIR>/main.cpp:1",
                     "int after;"}));
    }
}

void TestCppPreprocessorInclude::test_guard_after_comment()
{
    // Comments and blank lines before the guard #ifndef do not disturb
    // the guard detection (only directives clear mFileJustOpenned).
    QString dir = scenarioPath("guard_after_comment");
    QStringList expected{
        "#include <DIR>/main.cpp:-1",
        "#include <DIR>/guard.h:-1",
        " ",
        "",
        "",
        "",
        "#define GUARD_H",
        "int guarded;",
        "",
        "#include <DIR>/main.cpp:0",
        "",
        "int after;"};
    for (bool includeOnce : {true, false}) {
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main.cpp", includeOnce), expected);
    }
}

void TestCppPreprocessorInclude::test_guard_after_define()
{
    // PINNED: the guard detection requires the guard #ifndef to be the
    // first preprocessor directive in the file; a leading #define clears
    // mFileJustOpenned, so in parser mode the guard is NOT detected and
    // the file is re-scanned on the second #include.
    QString dir = scenarioPath("guard_after_define");
    {
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                     "#include <DIR>/main.cpp:-1",
                     "#include <DIR>/guard.h:-1",
                     "#define SOMETHING 1",
                     "",
                     "#define GUARD_H",
                     "int guarded;",
                     "",
                     "#include <DIR>/main.cpp:0",
                     "",
                     "int after;"}));
    }
    {
        CppPreprocessor p;
        // On the re-scan the guard macro from the first scan is still
        // defined, so the guard body is skipped after all; only the
        // pre-guard content (#define SOMETHING) is duplicated.
        QCOMPARE(runScenario(p, dir, "main.cpp", false), QStringList({
                     "#include <DIR>/main.cpp:-1",
                     "#include <DIR>/guard.h:-1",
                     "#define SOMETHING 1",
                     "",
                     "#define GUARD_H",
                     "int guarded;",
                     "",
                     "#include <DIR>/main.cpp:0",
                     "#include <DIR>/guard.h:-1",
                     "#define SOMETHING 1",
                     "",
                     "",
                     "",
                     "",
                     "#include <DIR>/main.cpp:1",
                     "int after;"}));
    }
}

void TestCppPreprocessorInclude::test_guard_after_include()
{
    // An #include before the guard does not disturb the detection
    // (the include directive does not clear mFileJustOpenned).
    QString dir = scenarioPath("guard_after_include");
    QStringList expected{
        "#include <DIR>/main.cpp:-1",
        "#include <DIR>/guard.h:-1",
        "#include <DIR>/base.h:-1",
        "int base;",
        "#include <DIR>/guard.h:0",
        "",
        "#define GUARD_H",
        "int guarded;",
        "",
        "#include <DIR>/main.cpp:0",
        "",
        "int after;"};
    for (bool includeOnce : {true, false}) {
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main.cpp", includeOnce), expected);
    }
}

void TestCppPreprocessorInclude::test_include_via_macro()
{
    // The include operand may be produced by macro expansion.
    QString dir = scenarioPath("include_via_macro");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "#define HEADER \"dep.h\"",
                 "#include <DIR>/dep.h:-1",
                 "int dep;",
                 "#include <DIR>/main.cpp:1",
                 "int after;"}));
}

void TestCppPreprocessorInclude::test_recursive_include()
{
    // Mutually recursive includes are stopped by the include stack
    // (no infinite loop); the recursive include line stays empty.
    QString dir = scenarioPath("recursive_include");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "#include <DIR>/a.h:-1",
                 "#include <DIR>/b.h:-1",
                 "",
                 "int in_b;",
                 "#include <DIR>/a.h:0",
                 "int in_a;",
                 "#include <DIR>/main.cpp:0",
                 "int after;"}));
}

void TestCppPreprocessorInclude::test_include_nonexistent()
{
    // A missing include file is silently skipped.
    QString dir = scenarioPath("include_nonexistent");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "",
                 "int after;"}));
}

void TestCppPreprocessorInclude::test_include_next()
{
    // #include_next continues the search after the directory of the
    // current file in the include path list.
    QString dir = scenarioPath("include_next");
    CppPreprocessor p;
    p.addIncludePath(dir + "/dirA");
    p.addIncludePath(dir + "/dirB");
    QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "#include <DIR>/dirA/x.h:-1",
                 "#include <DIR>/dirB/x.h:-1",
                 "int x_b;",
                 "#include <DIR>/dirA/x.h:0",
                 "int x_a;",
                 "#include <DIR>/main.cpp:0",
                 "int after;"}));
}

void TestCppPreprocessorInclude::test_undef_external()
{
    // #undef of a macro defined in another file removes it from the
    // working define set (and is recorded for invalidation restore).
    QString dir = scenarioPath("undef_external");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "#include <DIR>/def.h:-1",
                 "#define X 1",
                 "#include <DIR>/main.cpp:0",
                 "#undef X",
                 "int after;"}));
    QVERIFY(!p.getDefine("X"));
}

void TestCppPreprocessorInclude::test_ignored_directives()
{
    // Unsupported directives (#error, #warning, #line, other #pragma)
    // are silently ignored.
    QString dir = scenarioPath("ignored_directives");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "int after;"}));
}

void TestCppPreprocessorInclude::test_include_order_quote()
{
    // "..." includes search the includer's directory first, then the
    // project include paths, then the compiler include paths.
    QString dir = scenarioPath("include_order");
    CppPreprocessor p;
    p.addIncludePath(dir + "/sys");
    p.addProjectIncludePath(dir + "/proj");
    QCOMPARE(runScenario(p, dir, "main_quote.cpp", true), QStringList({
                 "#include <DIR>/main_quote.cpp:-1",
                 "#include <DIR>/shared.h:-1",
                 "int from_local;",
                 "#include <DIR>/main_quote.cpp:0",
                 "int after;"}));
}

void TestCppPreprocessorInclude::test_include_order_angle()
{
    // <...> includes search the project include paths first, then the
    // compiler include paths.
    QString dir = scenarioPath("include_order");
    CppPreprocessor p;
    p.addIncludePath(dir + "/sys");
    p.addProjectIncludePath(dir + "/proj");
    QCOMPARE(runScenario(p, dir, "main_angle.cpp", true), QStringList({
                 "#include <DIR>/main_angle.cpp:-1",
                 "#include <DIR>/proj/shared.h:-1",
                 "int from_project;",
                 "#include <DIR>/main_angle.cpp:0",
                 "int after;"}));
}
