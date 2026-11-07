#include "test_cpppreprocessor_include.h"
#include "src/parser/cpppreprocessor.h"
#include "qt_utils/utils.h"
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
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

void TestCppPreprocessorInclude::test_include_in_false_branch()
{
    // An #include in a false branch is not processed: no content, no
    // defines, and no file info is recorded for it.
    QString dir = scenarioPath("include_in_false_branch");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "",
                 "",
                 "",
                 "int after;"}));
    QVERIFY(!p.getDefine("FROM_A"));
    QVERIFY(!p.findFileInfo(dir + "/a.h"));
}

void TestCppPreprocessorInclude::test_has_include()
{
    // __has_include checks whether the file can be found (it once
    // reported whether the root file had already included it).
    QString dir = scenarioPath("has_include");
    {
        // the file exists -> 1 -> the #if branch is taken
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main1.cpp", true), QStringList({
                     "#include <DIR>/main1.cpp:-1",
                     "",
                     "int yes;",
                     "",
                     "",
                     ""}));
    }
    {
        // a nonexistent file -> 0 -> the #else branch is taken
        // (even though another file was already included)
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main2.cpp", true), QStringList({
                     "#include <DIR>/main2.cpp:-1",
                     "#include <DIR>/a.h:-1",
                     "int ah;",
                     "#include <DIR>/main2.cpp:0",
                     "",
                     "",
                     "",
                     "int no2;",
                     ""}));
    }
}

void TestCppPreprocessorInclude::test_reprocess_same_instance()
{
    // PINNED: re-preprocessing a file that is still in the scanned set
    // returns an EMPTY result (the root file hits the replay shortcut).
    // Production never relies on this: CppParser always invalidates the
    // file first (see test_reprocess_after_remove_scanned).
    QString dir = scenarioPath("basic_include");
    CppPreprocessor p;
    QStringList first = runScenario(p, dir, "main.cpp", true);
    QVERIFY(!first.isEmpty());
    QStringList second = runScenario(p, dir, "main.cpp", true);
    QCOMPARE(second, QStringList());
    // defines survive the empty replay
    QVERIFY(p.getDefine("foo") == nullptr); // foo is a function, no defines here
}

void TestCppPreprocessorInclude::test_reprocess_after_remove_scanned()
{
    // The production reparse path invalidates the edited file only;
    // files it includes stay scanned and are replayed (their content
    // lines collapse to empty lines - their parser statements persist).
    QString dir = scenarioPath("basic_include");
    CppPreprocessor p;
    QStringList first = runScenario(p, dir, "main.cpp", true);
    p.removeScannedFile(dir + "/main.cpp");
    QCOMPARE(runScenario(p, dir, "main.cpp", true), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "",
                 "int main() { return foo(); }"}));
    // Invalidating the included header as well forces a full re-scan
    // and reproduces the original result.
    p.removeScannedFile(dir + "/main.cpp");
    p.removeScannedFile(dir + "/a.h");
    QCOMPARE(runScenario(p, dir, "main.cpp", true), first);
}

void TestCppPreprocessorInclude::test_hard_define_restore()
{
    // A file define shadowing a hard define is removed on invalidation
    // and the hard define is restored.
    QString dir = scenarioPath("replay");
    CppPreprocessor p;
    p.addHardDefineByLine("#define UNG_V 0");
    p.preprocess(dir + "/a3.cpp");
    QCOMPARE(p.getDefine("UNG_V")->value, "1");
    p.removeScannedFile(dir + "/ung.h");
    QCOMPARE(p.getDefine("UNG_V")->value, "0");
}

void TestCppPreprocessorInclude::test_undef_restore()
{
    // An external #undef (of a macro defined in another file) is
    // recorded per file; invalidating that file restores the define.
    QString dir = scenarioPath("undef_external");
    CppPreprocessor p;
    p.preprocess(dir + "/main.cpp");
    QVERIFY(!p.getDefine("X"));
    p.removeScannedFile(dir + "/main.cpp");
    QCOMPARE(p.getDefine("X")->value, "1");
}

void TestCppPreprocessorInclude::test_on_get_file_stream()
{
    // Content provided through the callback wins over the file on disk.
    QString dir = scenarioPath("basic_include");
    CppPreprocessor p;
    p.setOnGetFileStream([](const QString& name, QStringList& out){
        if (name.endsWith("main.cpp")) {
            out = QStringList{"int custom;"};
            return true;
        }
        return false;
    });
    p.preprocess(dir + "/main.cpp");
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "int custom;"}));
    // a.h was never read: the callback content has no #include
    QVERIFY(!p.findFileInfo(dir + "/a.h"));
}

void TestCppPreprocessorInclude::test_replay_env_dependent()
{
    // config.h's branch depends on the environment at the include point
    // (USE_FAST). a.cpp defines it, b.cpp undefines it. The replay cache
    // hands b.cpp the defines recorded under a.cpp's environment.
    QString dir = scenarioPath("replay");
    CppPreprocessor p;
    p.setFileOnlyIncludeOnce(false); // parser mode
    p.preprocess(dir + "/a.cpp");
    QCOMPARE(p.getDefine("SPEED")->value, "100");
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/a.cpp:-1",
                 "#define USE_FAST",
                 "#include <DIR>/config.h:-1",
                 "",
                 "#define SPEED 100",
                 "",
                 "",
                 "",
                 "#include <DIR>/a.cpp:1",
                 "int speed_a = 100;"}));
    p.preprocess(dir + "/b.cpp");
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/b.cpp:-1",
                 "#undef USE_FAST",
                 "",
                 "int speed_b = 100;"}));
    // Correct semantics: with USE_FAST undefined at b.cpp's include
    // point, re-scanning config.h would yield SPEED 1. The replay cache
    // returns the stale 100 (pinned; fixed in phase 0.5).
    QEXPECT_FAIL("", "replay returns stale environment-dependent defines", Continue);
    QCOMPARE(p.getDefine("SPEED")->value, "1");
}

void TestCppPreprocessorInclude::test_replay_env_independent()
{
    // config2.h's condition only depends on hard defines (stable for the
    // whole session), so replaying it is always correct.
    QString dir = scenarioPath("replay");
    CppPreprocessor p;
    p.setFileOnlyIncludeOnce(false);
    p.addHardDefineByLine("#define __cplusplus 202002L");
    p.preprocess(dir + "/a2.cpp");
    QCOMPARE(p.getDefine("CPP17_OK")->value, "1");
    p.preprocess(dir + "/b2.cpp");
    QCOMPARE(p.getDefine("CPP17_OK")->value, "1");
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/b2.cpp:-1",
                 "",
                 "int v2b = 1;"}));
}

void TestCppPreprocessorInclude::test_replay_unguarded_cross_reparse()
{
    // PINNED: even UNGUARDED headers are replayed (not re-scanned) when
    // a later preprocess run includes them, because the could-repeat set
    // is per-run and starts empty.
    QString dir = scenarioPath("replay");
    CppPreprocessor p;
    p.setFileOnlyIncludeOnce(false);
    p.preprocess(dir + "/a3.cpp");
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/a3.cpp:-1",
                 "#include <DIR>/ung.h:-1",
                 "int ung;",
                 "#define UNG_V 1",
                 "#include <DIR>/a3.cpp:0",
                 "int a3;"}));
    p.preprocess(dir + "/b3.cpp");
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/b3.cpp:-1",
                 "",
                 "int b3;"}));
    // the replayed define is available
    QVERIFY(p.getDefine("UNG_V"));
}

void TestCppPreprocessorInclude::test_replay_external_modification()
{
    // A header changed on disk (git pull, external editor) after the
    // first scan is still replayed from the stale recorded defines.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeFile = [](const QString& path, const QStringList& lines) {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly|QIODevice::Truncate))
            return false;
        f.write(lines.join("\n").toUtf8());
        f.close();
        return true;
    };
    QVERIFY(writeFile(tmp.filePath("h.h"), {"#define V 1", "int v1;"}));
    QVERIFY(writeFile(tmp.filePath("a.cpp"), {"#include \"h.h\"", "int a;"}));
    QVERIFY(writeFile(tmp.filePath("b.cpp"), {"#include \"h.h\"", "int b;"}));
    CppPreprocessor p;
    p.setFileOnlyIncludeOnce(false);
    p.preprocess(tmp.filePath("a.cpp"));
    QCOMPARE(p.getDefine("V")->value, "1");
    QVERIFY(writeFile(tmp.filePath("h.h"),
                    {"#define V 2", "int v2;", "int v2b;"}));
    p.preprocess(tmp.filePath("b.cpp"));
    // Correct semantics: the mtime change should invalidate the recorded
    // defines and re-scan the header. The replay cache does not check
    // (pinned; fixed in phase 0.5).
    QEXPECT_FAIL("", "replay ignores on-disk modification of scanned files", Continue);
    QCOMPARE(p.getDefine("V")->value, "2");
}
