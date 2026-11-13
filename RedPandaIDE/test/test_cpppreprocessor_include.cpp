#include "test_cpppreprocessor_include.h"
#include "src/parser/cpppreprocessor.h"
#include "qt_utils/utils.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
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
                                                     const QString &mainFile)
{
    p.preprocess(dir + "/" + mainFile);
    return normalizedResult(p, dir);
}

void TestCppPreprocessorInclude::test_nested_include()
{
    QString dir = scenarioPath("nested_include");
    CppPreprocessor p;
    QStringList result = runScenario(p, dir, "main.cpp");
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
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp"), expected);
    QVERIFY(p.getDefine("GUARD_H"));
    PParsedFileInfo mainInfo = p.findFileInfo(dir + "/main.cpp");
    QVERIFY(mainInfo);
    QCOMPARE(mainInfo->directIncludes(),
             QStringList({dir + "/guard.h", dir + "/guard.h"}));
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
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp"), expected);
}

void TestCppPreprocessorInclude::test_no_guard()
{
    // An unguarded header is re-scanned on every #include (correct for
    // headers like <cassert> that are meant to be re-includable).
    QString dir = scenarioPath("no_guard");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "#include <DIR>/noguard.h:-1",
                 "int unguarded;",
                 "#include <DIR>/main.cpp:0",
                 "#include <DIR>/noguard.h:-1",
                 "int unguarded;",
                 "#include <DIR>/main.cpp:1",
                 "int after;"}));
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
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp"), expected);
}

void TestCppPreprocessorInclude::test_guard_after_define()
{
    // The guard detection requires the guard #ifndef to be the first
    // preprocessor directive in the file; a leading #define clears
    // mFileJustOpenned, so the guard is NOT detected and the file is
    // re-scanned on the second #include. On the re-scan the guard macro
    // from the first scan is still defined, so the guard body is
    // skipped after all; only the pre-guard content (#define SOMETHING)
    // is duplicated.
    QString dir = scenarioPath("guard_after_define");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
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
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp"), expected);
}

void TestCppPreprocessorInclude::test_include_via_macro()
{
    // The include operand may be produced by macro expansion.
    QString dir = scenarioPath("include_via_macro");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "#define HEADER \"dep.h\"",
                 "#include <DIR>/dep.h:-1",
                 "int dep;",
                 "#include <DIR>/main.cpp:1",
                 "int after;"}));
}

void TestCppPreprocessorInclude::test_recursive_include()
{
    // An unguarded include cycle is processed like real compilers do:
    // the files are re-entered on the stack, bounded by the nesting
    // depth cap (set low here to keep the test small).
    QString dir = scenarioPath("recursive_include");
    CppPreprocessor p;
    p.setMaxIncludeNestingDepth(5);
    QStringList result = runScenario(p, dir, "main.cpp");
    int inA = 0, inB = 0;
    bool sawAfter = false;
    for (const QString& line : result) {
        if (line == "int in_a;")
            inA++;
        if (line == "int in_b;")
            inB++;
        if (line == "int after;")
            sawAfter = true;
    }
    // main(1) -> a.h(2) -> b.h(3) -> a.h(4) -> b.h(5): each file is
    // scanned twice before the cap stops the cycle.
    QCOMPARE(inA, 2);
    QCOMPARE(inB, 2);
    QVERIFY(sawAfter);
}

void TestCppPreprocessorInclude::test_include_nonexistent()
{
    // A missing include file is silently skipped.
    QString dir = scenarioPath("include_nonexistent");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
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
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
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
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
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
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
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
    QCOMPARE(runScenario(p, dir, "main_quote.cpp"), QStringList({
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
    QCOMPARE(runScenario(p, dir, "main_angle.cpp"), QStringList({
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
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "",
                 "",
                 "",
                 "int after;"}));
    QVERIFY(!p.getDefine("FROM_A"));
    QVERIFY(!p.findFileInfo(dir + "/a.h"));
}

void TestCppPreprocessorInclude::test_recursive_include_env_sensitive()
{
    // x.h is environment-sensitive and cycles with y.h (x -> y -> x).
    // The cycle is processed like compilers do, bounded by the depth
    // cap; the on-stack re-scan must not invalidate the outer scan's
    // recorded file info (that once crashed the parser).
    QString dir = scenarioPath("env_recursive");
    CppPreprocessor p;
    p.setMaxIncludeNestingDepth(5);
    QStringList result = runScenario(p, dir, "main.cpp");
    int inX = 0, inY = 0;
    for (const QString& line : result) {
        if (line == "int in_x;")
            inX++;
        if (line == "int in_y;")
            inY++;
    }
    // main(1) -> x.h(2) -> y.h(3) -> x.h(4) -> y.h(5): each scanned twice
    QCOMPARE(inX, 2);
    QCOMPARE(inY, 2);
    QVERIFY(!p.getDefine("FROM_X"));
    QVERIFY(p.findFileInfo(dir + "/x.h"));
    QVERIFY(p.findFileInfo(dir + "/y.h"));
    QVERIFY(p.findFileInfo(dir + "/main.cpp"));
}

void TestCppPreprocessorInclude::test_env_sensitive_included_twice()
{
    // Regression: x.h is environment-sensitive, first included via a.h
    // (which makes it transitively included by main), then directly by
    // main. At the second include point it is invalidated for the
    // re-scan - but the "already included" shortcut then dropped it
    // without re-inserting its file info (crashing the parser later).
    QString dir = scenarioPath("env_twice");
    CppPreprocessor p;
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "#include <DIR>/a.h:-1",
                 "#include <DIR>/x.h:-1",
                 "",
                 "",
                 "",
                 "int x_content;",
                 "#include <DIR>/a.h:0",
                 "int a_content;",
                 "#include <DIR>/main.cpp:0",
                 "#include <DIR>/x.h:-1",
                 "",
                 "",
                 "",
                 "int x_content;",
                 "#include <DIR>/main.cpp:1",
                 "int after;"}));
    QVERIFY(!p.getDefine("FROM_X"));
    QVERIFY(p.findFileInfo(dir + "/x.h"));
    QVERIFY(p.findFileInfo(dir + "/a.h"));
    QVERIFY(p.findFileInfo(dir + "/main.cpp"));
}

void TestCppPreprocessorInclude::test_depth_bounded_iteration()
{
    // Depth-bounded intentional cycles (Boost-iteration style) terminate
    // by branch evaluation and work like real compilers: in parser mode
    // the unguarded file is re-entered on the stack until the pattern
    // converges by itself.
    QString dir = scenarioPath("iter_cycle");
    CppPreprocessor p;
    QStringList result = runScenario(p, dir, "main.cpp");
    int markerCount = 0;
    for (const QString& line : result) {
        if (line == "int iter_marker;")
            markerCount++;
    }
    QCOMPARE(markerCount, 4);
    QVERIFY(p.getDefine("ITER_DONE"));
    QVERIFY(p.getDefine("ITER_1"));
    QVERIFY(p.getDefine("ITER_2"));
    QVERIFY(p.getDefine("ITER_3"));
}

void TestCppPreprocessorInclude::test_cycle_depth_cap()
{
    // An unconditional include cycle (a user error) is bounded by the
    // nesting depth cap instead of looping forever: real compilers
    // recurse to a limit and error out, we skip the include at the cap.
    QString dir = scenarioPath("true_cycle");
    CppPreprocessor p;
    p.setMaxIncludeNestingDepth(3);
    QStringList result = runScenario(p, dir, "main.cpp");
    int sCount = 0;
    bool sawAfter = false;
    for (const QString& line : result) {
        if (line == "int s;")
            sCount++;
        if (line == "int after;")
            sawAfter = true;
    }
    // main(1) -> self(2) -> self(3): the cap allows two scans of self.h
    QCOMPARE(sCount, 2);
    QVERIFY(sawAfter);
}

void TestCppPreprocessorInclude::test_has_include()
{
    // __has_include checks whether the file can be found (it once
    // reported whether the root file had already included it).
    QString dir = scenarioPath("has_include");
    {
        // the file exists -> 1 -> the #if branch is taken
        CppPreprocessor p;
        QCOMPARE(runScenario(p, dir, "main1.cpp"), QStringList({
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
        QCOMPARE(runScenario(p, dir, "main2.cpp"), QStringList({
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
    QString dir = scenarioPath("basic_include");
    CppPreprocessor p;
    QStringList first = runScenario(p, dir, "main.cpp");
    QVERIFY(!first.isEmpty());
    QStringList second = runScenario(p, dir, "main.cpp");
    QCOMPARE(second, first);
    QVERIFY(p.getDefine("foo") == nullptr); // foo is a function, no defines here
}

void TestCppPreprocessorInclude::test_clear_registry()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/macro_config.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.preprocess(dir + "/macro_one.cpp");
    QVERIFY(p.fileScanned(header));
    QVERIFY(p.findFileInfo(header));
    QVERIFY(p.getDefine("MACRO_RESULT"));

    p.clear();
    QVERIFY(!p.fileScanned(header));
    QVERIFY(!p.findFileInfo(header));
    QVERIFY(!p.getDefine("MACRO_RESULT"));
    QVERIFY(p.result().isEmpty());

    p.preprocess(dir + "/macro_one.cpp");
    QCOMPARE(p.eventCounts(header).scan, 1);
    QCOMPARE(p.eventCounts(header).replay, 0);
}

void TestCppPreprocessorInclude::test_reprocess_after_remove_scanned()
{
    // The production reparse path invalidates the edited file only;
    // files it includes stay scanned and are replayed (their content
    // lines collapse to empty lines - their parser statements persist).
    QString dir = scenarioPath("basic_include");
    CppPreprocessor p;
    p.addIncludePath(dir);
    p.addProjectFile(dir + "/main.cpp");
    QStringList first = runScenario(p, dir, "main.cpp");
    p.removeScannedFile(dir + "/main.cpp");
    QCOMPARE(runScenario(p, dir, "main.cpp"), QStringList({
                 "#include <DIR>/main.cpp:-1",
                 "",
                 "int main() { return foo(); }"}));
    // Invalidating the included header as well forces a full re-scan
    // and reproduces the original result.
    p.removeScannedFile(dir + "/main.cpp");
    p.removeScannedFile(dir + "/a.h");
    QCOMPARE(runScenario(p, dir, "main.cpp"), first);
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

void TestCppPreprocessorInclude::test_session_file_cache_reuse_and_clear()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString header = tmp.filePath("repeat.h");
    QString main = tmp.filePath("main.cpp");
    QFile headerFile(header);
    QVERIFY(headerFile.open(QIODevice::WriteOnly));
    headerFile.close();
    QFile mainFile(main);
    QVERIFY(mainFile.open(QIODevice::WriteOnly));
    mainFile.write("#include \"repeat.h\"\n#include \"repeat.h\"\n");
    mainFile.close();

    int headerReads = 0;
    CppPreprocessor p;
    p.setOnGetFileStream([&](const QString &fileName, QStringList &out) {
        if (!fileName.endsWith("repeat.h"))
            return false;
        ++headerReads;
        out = QStringList{"int cached_value;"};
        return true;
    });
    p.preprocess(main);
    QCOMPARE(headerReads, 1);
    QCOMPARE(p.result().count("int cached_value;"), 2);

    p.preprocess(main);
    QCOMPARE(headerReads, 2);
    QCOMPARE(p.result().count("int cached_value;"), 2);
}

void TestCppPreprocessorInclude::test_replay_env_dependent()
{
    QString dir = scenarioPath("replay");
    CppPreprocessor p;
    p.addIncludePath(dir);
    p.addProjectFile(dir + "/a.cpp");
    p.addProjectFile(dir + "/b.cpp");
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
    // config.h's conditions reference the environment macro USE_FAST, so
    // it is environment-sensitive and re-scanned at b.cpp's include
    // point (it was once replayed from the defines recorded under
    // a.cpp's environment).
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/b.cpp:-1",
                 "#undef USE_FAST",
                 "#include <DIR>/config.h:-1",
                 "",
                 "",
                 "",
                 "#define SPEED 1",
                 "",
                 "#include <DIR>/b.cpp:1",
                 "int speed_b = 1;"}));
    QCOMPARE(p.getDefine("SPEED")->value, "1");
}

void TestCppPreprocessorInclude::test_replay_env_independent()
{
    // config2.h's condition only depends on hard defines (stable for the
    // whole session), so replaying it is always correct.
    QString dir = scenarioPath("replay");
    CppPreprocessor p;
    p.addIncludePath(dir);
    p.addProjectFile(dir + "/a2.cpp");
    p.addProjectFile(dir + "/b2.cpp");
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

void TestCppPreprocessorInclude::test_replay_config_macro_stable()
{
    // The libstdc++ pattern: config3.h's condition references CFG_V,
    // a macro provided by another header (cfg_provider.h). As long as
    // the includers see the same macro state, config3.h is replayed
    // (NOT re-scanned at every include point); only an includer that
    // actually changes the state forces a re-scan.
    QString dir = scenarioPath("replay");
    CppPreprocessor p;
    p.addIncludePath(dir);
    p.addProjectFile(dir + "/a4.cpp");
    p.addProjectFile(dir + "/b4.cpp");
    p.addProjectFile(dir + "/b5.cpp");
    // a4.cpp: config3.h is scanned with CFG_V=1
    p.preprocess(dir + "/a4.cpp");
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/a4.cpp:-1",
                 "#include <DIR>/cfg_provider.h:-1",
                 "#define CFG_V 1",
                 "#include <DIR>/a4.cpp:0",
                 "#include <DIR>/config3.h:-1",
                 "",
                 "#define CFG_OUT 1",
                 "",
                 "",
                 "",
                 "#include <DIR>/a4.cpp:1",
                 "int va = 1;"}));
    // b4.cpp: same macro state -> replayed, no re-scan
    p.preprocess(dir + "/b4.cpp");
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/b4.cpp:-1",
                 "",
                 "",
                 "int vb = 1;"}));
    // b5.cpp: defines CFG_V differently -> state mismatch -> re-scan
    p.preprocess(dir + "/b5.cpp");
    QCOMPARE(normalizedResult(p, dir), QStringList({
                 "#include <DIR>/b5.cpp:-1",
                 "#define CFG_V 2",
                 "#include <DIR>/config3.h:-1",
                 "",
                 "",
                 "",
                 "#define CFG_OUT 0",
                 "",
                 "#include <DIR>/b5.cpp:1",
                 "int vb5 = 0;"}));
}

void TestCppPreprocessorInclude::test_replay_event_counts()
{
    QString dir = scenarioPath("replay");
    QString config = dir + "/config3.h";
    CppPreprocessor p;
    p.addIncludePath(dir);
    p.addProjectFile(dir + "/a4.cpp");
    p.addProjectFile(dir + "/b4.cpp");
    p.addProjectFile(dir + "/b5.cpp");
    p.preprocess(dir + "/a4.cpp");
    CppPreprocessor::EventCounts counts = p.eventCounts(config);
    QCOMPARE(counts.scan, 1);
    QCOMPARE(counts.replay, 0);
    QCOMPARE(counts.invalidate, 0);

    p.preprocess(dir + "/b4.cpp");
    counts = p.eventCounts(config);
    QCOMPARE(counts.scan, 1);
    QCOMPARE(counts.replay, 1);
    QCOMPARE(counts.invalidate, 0);

    p.preprocess(dir + "/b5.cpp");
    counts = p.eventCounts(config);
    QCOMPARE(counts.scan, 2);
    QCOMPARE(counts.replay, 1);
    QCOMPARE(counts.invalidate, 1);
}

void TestCppPreprocessorInclude::test_replay_miss_reasons_are_observable()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    {
        CppPreprocessor p;
        p.addIncludePath(includeDir);
        p.preprocess(dir + "/macro_one.cpp");
        p.preprocess(dir + "/macro_two.cpp");
        QCOMPARE(p.eventCounts(includeDir + "/macro_config.h").scan, 2);
        QCOMPARE(p.eventCounts(includeDir + "/macro_config.h").invalidate, 1);
    }
    {
        CppPreprocessor p;
        p.addIncludePath(includeDir);
        p.preprocess(dir + "/order_ab.cpp");
        p.preprocess(dir + "/order_ba.cpp");
        QCOMPARE(p.eventCounts(includeDir + "/plain_a.h").scan, 2);
        QCOMPARE(p.eventCounts(includeDir + "/plain_b.h").scan, 2);
    }
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QString header = tmp.filePath("h.h");
        QString main = tmp.filePath("main.cpp");
        QFile headerFile(header);
        QVERIFY(headerFile.open(QIODevice::WriteOnly));
        headerFile.write("int v1;\n");
        headerFile.close();
        QFile mainFile(main);
        QVERIFY(mainFile.open(QIODevice::WriteOnly));
        mainFile.write("#include \"h.h\"\n");
        mainFile.close();
        CppPreprocessor p;
        p.addIncludePath(tmp.path());
        p.addProjectFile(main);
        p.preprocess(main);
        QVERIFY(headerFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
        headerFile.write("int version_two;\n");
        headerFile.close();
        p.preprocess(main);
        QCOMPARE(p.eventCounts(header).scan, 2);
        QCOMPARE(p.eventCounts(header).invalidate, 1);
    }
}

void TestCppPreprocessorInclude::test_metadata_only_open_is_not_counted_as_scan()
{
    QString dir = scenarioPath("boundary_replay");
    QString header = dir + "/include/plain_a.h";
    CppPreprocessor p;
    p.addIncludePath(dir + "/include");
    p.addProjectFile(dir + "/order_ab.cpp");
    p.setScanOptions(false, true);
    p.preprocess(dir + "/order_ab.cpp");
    QCOMPARE(p.eventCounts(header).scan, 0);
}

void TestCppPreprocessorInclude::test_replay_same_user_macros()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/macro_config.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.preprocess(dir + "/macro_one.cpp");
    p.preprocess(dir + "/macro_same.cpp");
    CppPreprocessor::EventCounts counts = p.eventCounts(header);
    QCOMPARE(counts.scan, 1);
    QCOMPARE(counts.replay, 1);
    QCOMPARE(p.expandMacros("MACRO_RESULT"), QString("1"));
}

void TestCppPreprocessorInclude::test_user_macro_change_forces_rescan()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/macro_config.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.preprocess(dir + "/macro_one.cpp");
    p.preprocess(dir + "/macro_two.cpp");
    CppPreprocessor::EventCounts counts = p.eventCounts(header);
    QCOMPARE(counts.scan, 2);
    QCOMPARE(counts.replay, 0);
    QCOMPARE(counts.invalidate, 1);
    QVERIFY(normalizedResult(p, dir).contains("int macro_result = 2;"));
}

void TestCppPreprocessorInclude::test_replayed_entries_extend_prefix()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.preprocess(dir + "/order_ab.cpp");
    p.preprocess(dir + "/order_ab.cpp");
    for (const char *name : {"plain_a.h", "plain_b.h"}) {
        CppPreprocessor::EventCounts counts = p.eventCounts(includeDir + "/" + name);
        QCOMPARE(counts.scan, 1);
        QCOMPARE(counts.replay, 1);
    }
}

void TestCppPreprocessorInclude::test_include_order_change_forces_rescan()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.preprocess(dir + "/order_ab.cpp");
    p.preprocess(dir + "/order_ba.cpp");
    for (const char *name : {"plain_a.h", "plain_b.h"}) {
        CppPreprocessor::EventCounts counts = p.eventCounts(includeDir + "/" + name);
        QCOMPARE(counts.scan, 2);
        QCOMPARE(counts.replay, 0);
        QCOMPARE(counts.invalidate, 1);
    }
}

void TestCppPreprocessorInclude::test_scanned_prefix_entry_forces_lazy_suffix_rescan()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString root = dir + "/order_ab.cpp";
    QString first = includeDir + "/plain_a.h";
    QString second = includeDir + "/plain_b.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.preprocess(root);

    p.invalidateFile(first);
    QCOMPARE(p.eventCounts(first).invalidate, 1);
    QCOMPARE(p.eventCounts(second).invalidate, 0);
    QVERIFY(p.fileScanned(second));

    p.preprocess(root);
    QCOMPARE(p.eventCounts(first).scan, 2);
    QCOMPARE(p.eventCounts(second).scan, 2);
    QCOMPARE(p.eventCounts(first).replay, 0);
    QCOMPARE(p.eventCounts(second).replay, 0);
    QCOMPARE(p.eventCounts(second).invalidate, 1);
    QVERIFY(p.result().contains("int plain_a;"));
    QVERIFY(p.result().contains("int plain_b;"));
}

void TestCppPreprocessorInclude::test_guarded_noop_then_first_boundary_entry()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/guarded.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.preprocess(dir + "/guard_first.cpp");
    QCOMPARE(p.eventCounts(header).scan, 1);
    QCOMPARE(p.eventCounts(header).skip, 1);
    p.preprocess(dir + "/guard_second.cpp");
    QCOMPARE(p.eventCounts(header).scan, 2);
    QCOMPARE(p.eventCounts(header).replay, 0);
}

void TestCppPreprocessorInclude::test_user_to_user_include_has_no_boundary_record()
{
    QString dir = scenarioPath("basic_include");
    CppPreprocessor p;
    p.addProjectFile(dir + "/main.cpp");
    p.preprocess(dir + "/main.cpp");
    QString header = dir + "/a.h";
    QVERIFY(p.fileScanned(header));
    QCOMPARE(p.boundaryAncestor(header, 0), QString());
}

void TestCppPreprocessorInclude::test_system_subtree_belongs_to_boundary_ancestor()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString container = includeDir + "/guard_container.h";
    QString guarded = includeDir + "/guarded.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/guard_first.cpp");
    p.preprocess(dir + "/guard_first.cpp");
    QCOMPARE(p.boundaryAncestor(guarded, 0), container);
    QVERIFY(!p.findFileInfo(guarded));
}

void TestCppPreprocessorInclude::test_nonproject_include_path_file_is_inside_boundary()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/plain_a.h";
    CppPreprocessor p;
    p.addProjectIncludePath(includeDir);
    p.addProjectFile(dir + "/order_ab.cpp");
    p.preprocess(dir + "/order_ab.cpp");
    QCOMPARE(p.boundaryAncestor(header, 0), header);
}

void TestCppPreprocessorInclude::test_system_header_root_is_boundary_entry()
{
    QString dir = scenarioPath("boundary_replay");
    QString header = dir + "/include/plain_a.h";
    CppPreprocessor p;
    p.addIncludePath(dir + "/include");
    p.preprocess(header);
    QVERIFY(p.fileScanned(header));
    QCOMPARE(p.boundaryAncestor(header, 0), header);
}

void TestCppPreprocessorInclude::test_guard_must_cover_whole_file()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/partial_guard.cpp");
    p.preprocess(dir + "/partial_guard.cpp");
    QCOMPARE(p.result().count("int guarded_part;"), 1);
    QCOMPARE(p.result().count("int unguarded_part;"), 2);
}

void TestCppPreprocessorInclude::test_pragma_once_different_environments()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/pragma_env.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/pragma_a.cpp");
    p.addProjectFile(dir + "/pragma_b.cpp");
    p.preprocess(dir + "/pragma_a.cpp");
    QVERIFY(p.result().contains("int pragma_int;"));

    p.preprocess(dir + "/pragma_b.cpp");
    QVERIFY(p.result().contains("int pragma_long;"));
    QCOMPARE(p.eventCounts(header).scan, 2);
}

void TestCppPreprocessorInclude::test_user_macro_undefined_state_forces_rescan()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/macro_config.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/macro_one.cpp");
    p.addProjectFile(dir + "/macro_none.cpp");
    p.preprocess(dir + "/macro_one.cpp");
    p.preprocess(dir + "/macro_none.cpp");
    QCOMPARE(p.eventCounts(header).scan, 2);
    QVERIFY(p.result().contains("int macro_result = 0;"));
}

void TestCppPreprocessorInclude::test_user_function_macro_signature_forces_rescan()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/plain_a.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/function_a.cpp");
    p.addProjectFile(dir + "/function_b.cpp");
    p.preprocess(dir + "/function_a.cpp");
    p.preprocess(dir + "/function_b.cpp");
    QCOMPARE(p.eventCounts(header).scan, 2);
}

void TestCppPreprocessorInclude::test_hard_define_change_resets_parser()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/macro_none.cpp");
    p.addHardDefineByLine("#define MODE 1");
    p.preprocess(dir + "/macro_none.cpp");
    QVERIFY(p.result().contains("int macro_result = 1;"));

    p.clear();
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/macro_none.cpp");
    p.addHardDefineByLine("#define MODE 2");
    p.preprocess(dir + "/macro_none.cpp");
    QVERIFY(p.result().contains("int macro_result = 2;"));
}

void TestCppPreprocessorInclude::test_header_internal_define_undef_replays()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/internal_define.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/internal_a.cpp");
    p.addProjectFile(dir + "/internal_b.cpp");
    p.preprocess(dir + "/internal_a.cpp");
    p.preprocess(dir + "/internal_b.cpp");
    QCOMPARE(p.eventCounts(header).scan, 1);
    QCOMPARE(p.eventCounts(header).replay, 1);
    QCOMPARE(p.expandMacros("INTERNAL_OUT"), QString("1"));
}

void TestCppPreprocessorInclude::test_indirect_user_header_macro_change_forces_rescan()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeFile = [](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(lines.join('\n').toUtf8());
        file.close();
        return true;
    };
    QString provider = tmp.filePath("provider.h");
    QString main = tmp.filePath("main.cpp");
    QString config = tmp.filePath("macro_config.h");
    QVERIFY(writeFile(provider, {"#define MODE 1"}));
    QVERIFY(writeFile(config, {"#ifdef MODE", "#define OUT MODE", "#else",
                               "#define OUT 0", "#endif", "int result = OUT;"}));
    QVERIFY(writeFile(main, {"#include \"provider.h\"", "#include \"macro_config.h\""}));

    CppPreprocessor p;
    p.addIncludePath(tmp.path());
    p.addProjectFile(main);
    p.preprocess(main);
    QVERIFY(p.result().contains("int result = 1;"));

    QVERIFY(writeFile(provider, {"#define MODE 2"}));
    p.invalidateFile(provider);
    p.preprocess(main);
    QVERIFY(p.result().contains("int result = 2;"));
    QCOMPARE(p.eventCounts(config).scan, 2);
}

void TestCppPreprocessorInclude::test_system_internal_include_does_not_extend_prefix()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString container = includeDir + "/guard_container.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/guard_first.cpp");
    p.preprocess(dir + "/guard_first.cpp");
    QCOMPARE(p.currentBoundaryPrefix(), QStringList{container});
}

void TestCppPreprocessorInclude::test_system_to_user_include_is_diagnosed()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/reverse.cpp");
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
                "Boundary-inside file includes a boundary-outside file:.*"));
    p.preprocess(dir + "/reverse.cpp");
    QVERIFY(p.result().contains("int reverse_user;"));
}

void TestCppPreprocessorInclude::test_touch_conservatively_invalidates()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString header = tmp.filePath("touch.h");
    QString main = tmp.filePath("main.cpp");
    QFile headerFile(header);
    QVERIFY(headerFile.open(QIODevice::WriteOnly));
    headerFile.write("int touched;\n");
    headerFile.close();
    QFile mainFile(main);
    QVERIFY(mainFile.open(QIODevice::WriteOnly));
    mainFile.write("#include \"touch.h\"\n");
    mainFile.close();

    CppPreprocessor p;
    p.addIncludePath(tmp.path());
    p.addProjectFile(main);
    p.preprocess(main);
    QDateTime newTime = QFileInfo(header).lastModified().addSecs(2);
    QVERIFY(headerFile.open(QIODevice::ReadWrite));
    QVERIFY(headerFile.setFileTime(newTime, QFileDevice::FileModificationTime));
    p.preprocess(main);
    QCOMPARE(p.eventCounts(header).scan, 2);
    QVERIFY(p.result().contains("int touched;"));
}

void TestCppPreprocessorInclude::test_editor_change_invalidates_without_disk_change()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString header = tmp.filePath("callback.h");
    QString main = tmp.filePath("main.cpp");
    QFile file(header);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    file.setFileName(main);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("#include \"callback.h\"\n");
    file.close();

    int version = 1;
    CppPreprocessor p;
    p.addIncludePath(tmp.path());
    p.addProjectFile(main);
    p.setOnGetFileStream([&](const QString &name, QStringList &out) {
        if (name != header)
            return false;
        out = QStringList{QString("int callback_v%1;").arg(version)};
        return true;
    });
    p.preprocess(main);
    QVERIFY(p.result().contains("int callback_v1;"));

    version = 2;
    p.invalidateFile(header);
    p.preprocess(main);
    QVERIFY(p.result().contains("int callback_v2;"));
    QCOMPARE(p.eventCounts(header).scan, 2);
}

void TestCppPreprocessorInclude::test_depth_cap_without_repeated_file()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeFile = [](const QString &path, const QString &text) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(text.toUtf8());
        file.close();
        return true;
    };
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeFile(main, "#include \"f0.h\"\n"));
    for (int i = 0; i < 60; i++) {
        QString text = i == 59 ? "int last;\n"
                               : QString("#include \"f%1.h\"\n").arg(i + 1);
        QVERIFY(writeFile(tmp.filePath(QString("f%1.h").arg(i)), text));
    }

    CppPreprocessor p;
    p.addIncludePath(tmp.path());
    p.addProjectFile(main);
    p.setMaxIncludeNestingDepth(40);
    p.preprocess(main);
    QCOMPARE(p.eventCounts(tmp.filePath("f39.h")).scan, 0);
    QVERIFY(p.eventCounts(tmp.filePath("f38.h")).scan > 0);
}

void TestCppPreprocessorInclude::test_active_include_stack_is_not_invalidated_mid_scan()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString child = tmp.filePath("child.h");
    QString main = tmp.filePath("main.cpp");
    QFile childFile(child);
    QVERIFY(childFile.open(QIODevice::WriteOnly));
    childFile.close();
    QFile mainFile(main);
    QVERIFY(mainFile.open(QIODevice::WriteOnly));
    mainFile.write("int root_value;\n#include \"child.h\"\n");
    mainFile.close();

    CppPreprocessor p;
    p.addIncludePath(tmp.path());
    p.addProjectFile(main);
    bool triggered = false;
    p.setOnGetFileStream([&](const QString &name, QStringList &out) {
        if (name != child)
            return false;
        if (!triggered) {
            triggered = true;
            p.invalidateFile(main);
        }
        out = QStringList{"int child_value;"};
        return true;
    });
    p.preprocess(main);
    QVERIFY(p.result().contains("int root_value;"));
    QVERIFY(p.result().contains("int child_value;"));
    QVERIFY(p.findFileInfo(main));

    p.preprocess(main);
    QVERIFY(p.result().contains("int root_value;"));
    QCOMPARE(p.eventCounts(main).scan, 2);
    QCOMPARE(p.eventCounts(child).scan, 1);
    QCOMPARE(p.eventCounts(child).replay, 1);
    QVERIFY(p.findFileInfo(child));
}

void TestCppPreprocessorInclude::test_stack_reentry_uses_current_environment()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeFile = [](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(lines.join('\n').toUtf8());
        file.close();
        return true;
    };
    QString a = tmp.filePath("a.h");
    QString b = tmp.filePath("b.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeFile(a, {"#ifdef MODE", "int a_mode;", "#else",
                          "int a_default;", "#endif", "#include \"b.h\""}));
    QVERIFY(writeFile(b, {"#undef MODE", "#include \"a.h\""}));
    QVERIFY(writeFile(main, {"#define MODE 1", "#include \"a.h\""}));

    CppPreprocessor p;
    p.addIncludePath(tmp.path());
    p.addProjectFile(main);
    p.setMaxIncludeNestingDepth(10);
    p.preprocess(main);
    QCOMPARE(p.result().count("int a_mode;"), 1);
    QVERIFY(p.result().count("int a_default;") > 0);
}

void TestCppPreprocessorInclude::test_user_to_system_creates_boundary_record()
{
    QString dir = scenarioPath("boundary_replay");
    QString includeDir = dir + "/include";
    QString header = includeDir + "/plain_a.h";
    CppPreprocessor p;
    p.addIncludePath(includeDir);
    p.addProjectFile(dir + "/order_ab.cpp");
    p.preprocess(dir + "/order_ab.cpp");
    QVERIFY(p.fileScanned(header));
    QCOMPARE(p.boundaryAncestor(header, 0), header);
    QCOMPARE(p.currentBoundaryPrefix().first(), header);
}

void TestCppPreprocessorInclude::test_protected_include_cycle_stops()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeFile = [](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(lines.join('\n').toUtf8());
        file.close();
        return true;
    };
    QString a = tmp.filePath("a.h");
    QString b = tmp.filePath("b.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeFile(a, {"#ifndef A_H", "#define A_H", "#include \"b.h\"",
                          "int in_a;", "#endif"}));
    QVERIFY(writeFile(b, {"#pragma once", "#include \"a.h\"", "int in_b;"}));
    QVERIFY(writeFile(main, {"#include \"a.h\""}));

    CppPreprocessor p;
    p.addIncludePath(tmp.path());
    p.addProjectFile(main);
    p.preprocess(main);
    QCOMPARE(p.result().count("int in_a;"), 1);
    QCOMPARE(p.result().count("int in_b;"), 1);
}

void TestCppPreprocessorInclude::test_replayed_subtree_marks_pragma_once_file_processed()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeFile = [](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(lines.join('\n').toUtf8());
        file.close();
        return true;
    };
    QString inner = tmp.filePath("inner.h");
    QString outer = tmp.filePath("outer.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeFile(inner, {"#pragma once", "int inner_value;"}));
    QVERIFY(writeFile(outer, {"#include \"inner.h\"", "int outer_value;"}));
    QVERIFY(writeFile(main, {"#include \"outer.h\"", "#include \"inner.h\""}));

    CppPreprocessor p;
    p.addIncludePath(tmp.path());
    p.addProjectFile(main);
    p.preprocess(main);
    QCOMPARE(p.eventCounts(outer).scan, 1);
    QCOMPARE(p.eventCounts(inner).scan, 1);
    QCOMPARE(p.eventCounts(inner).skip, 1);

    p.preprocess(main);
    QCOMPARE(p.eventCounts(outer).scan, 1);
    QCOMPARE(p.eventCounts(outer).replay, 1);
    QCOMPARE(p.eventCounts(inner).scan, 1);
    QCOMPARE(p.eventCounts(inner).skip, 2);
    QCOMPARE(p.currentBoundaryPrefix(), QStringList{outer});
    QCOMPARE(p.result().count("int inner_value;"), 0);
}

void TestCppPreprocessorInclude::test_replay_unguarded_cross_reparse()
{
    QString dir = scenarioPath("replay");
    CppPreprocessor p;
    p.addIncludePath(dir);
    p.addProjectFile(dir + "/a3.cpp");
    p.addProjectFile(dir + "/b3.cpp");
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

void TestCppPreprocessorInclude::test_subtree_mtime_invalidates_matching_prefix()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeFile = [](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(lines.join('\n').toUtf8());
        file.close();
        return true;
    };
    QString inner = tmp.filePath("inner.h");
    QString outer = tmp.filePath("outer.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeFile(inner, {"int inner_v1;"}));
    QVERIFY(writeFile(outer, {"#include \"inner.h\"", "int outer;"}));
    QVERIFY(writeFile(main, {"#include \"outer.h\""}));

    CppPreprocessor p;
    p.addIncludePath(tmp.path());
    p.addProjectFile(main);
    p.preprocess(main);
    QVERIFY(p.result().contains("int inner_v1;"));

    QVERIFY(writeFile(inner, {"int inner_v2;"}));
    p.preprocess(main);
    QVERIFY(p.result().contains("int inner_v2;"));
    QCOMPARE(p.eventCounts(outer).invalidate, 1);
}

void TestCppPreprocessorInclude::test_replay_external_modification()
{
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
    p.addIncludePath(tmp.path());
    p.addProjectFile(tmp.filePath("a.cpp"));
    p.addProjectFile(tmp.filePath("b.cpp"));
    p.preprocess(tmp.filePath("a.cpp"));
    QCOMPARE(p.getDefine("V")->value, "1");
    QVERIFY(writeFile(tmp.filePath("h.h"),
                    {"#define V 2", "int v2;", "int v2b;"}));
    p.preprocess(tmp.filePath("b.cpp"));
    // The mtime change invalidates the recorded scan of h.h and forces a re-scan.
    QCOMPARE(p.getDefine("V")->value, "2");
    QStringList bResult = p.result();
    for (QString& line : bResult)
        line.replace(tmp.path(), "<DIR>");
    QCOMPARE(bResult, QStringList({
                 "#include <DIR>/b.cpp:-1",
                 "#include <DIR>/h.h:-1",
                 "#define V 2",
                 "int v2;",
                 "int v2b;",
                 "#include <DIR>/b.cpp:0",
                 "int b;"}));
}
