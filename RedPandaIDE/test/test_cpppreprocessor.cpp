#include "test_cpppreprocessor.h"
#include "src/parser/cpppreprocessor.h"
#include "qt_utils/utils.h"
#include <QTest>

TestCppPreprocessor::TestCppPreprocessor(QObject *parent):
    QObject{parent}
{

}

void TestCppPreprocessor::test_combine_lines_ending_with_backslash()
{
    QStringList text1 = readFileToLines("resources/preprocessor-backslashes-1.cpp");
    QStringList text2 = readFileToLines("resources/preprocessor-backslashes-1-result.cpp");
    QVERIFY(!text1.isEmpty());
    QVERIFY(!text2.isEmpty());
    CppPreprocessor::combineLinesEndingWithBackslash(text1);
    QCOMPARE(text1,text2);

}

void TestCppPreprocessor::test_replace_comments_with_space_char()
{
    QStringList text1 = readFileToLines("resources/preprocessor-comments-1.cpp");
    QStringList text2 = readFileToLines("resources/preprocessor-comments-1-result.cpp");
    QVERIFY(!text1.isEmpty());
    QVERIFY(!text2.isEmpty());
    CppPreprocessor::replaceCommentsBySpaceChar(text1);
    QCOMPARE(text1,text2);
}

void TestCppPreprocessor::test_macro_replace_1()
{
    CppPreprocessor preprocessor;
    QFileInfo info("resources/preprocessor-macros-1.cpp");
    preprocessor.preprocess(info.absoluteFilePath());
    QStringList text1 = filterIncludes(preprocessor.result());
    QStringList text2 = readFileToLines("resources/preprocessor-macros-1-result.cpp");
    QVERIFY(!text1.isEmpty());
    QVERIFY(!text2.isEmpty());
    QCOMPARE(text1,text2);
}

void TestCppPreprocessor::test_macro_replace_2()
{
    CppPreprocessor preprocessor;
    preprocessor.addHardDefineByLine("#define EMPTY");
    preprocessor.addHardDefineByLine("#define SCAN(x)     x");
    preprocessor.addHardDefineByLine("#define EXAMPLE_()  EXAMPLE");
    preprocessor.addHardDefineByLine("#define EXAMPLE(n)  EXAMPLE_ EMPTY()(n-1) (n)");
    QCOMPARE("EXAMPLE_ ()(5-1) (5)",
             preprocessor.expandMacros("EXAMPLE(5)"));
    QCOMPARE("EXAMPLE_ ()(5-1-1) (5-1) (5)",
             preprocessor.expandMacros("SCAN(EXAMPLE(5))"));
}

void TestCppPreprocessor::test_macro_replace_3()
{
    CppPreprocessor preprocessor;
    preprocessor.addHardDefineByLine("#define TEST 123");
    QCOMPARE("ttt 123 123",
             preprocessor.expandMacros("ttt TEST TEST"));
}

void TestCppPreprocessor::test_macro_replace_4()
{
    CppPreprocessor preprocessor;
    preprocessor.addHardDefineByLine("#define N TEST");
    preprocessor.addHardDefineByLine("#define TEST N");
    QCOMPARE("ttt TEST TEST",
             preprocessor.expandMacros("ttt TEST TEST"));
    QCOMPARE("ttt N N",
             preprocessor.expandMacros("ttt N N"));
}

void TestCppPreprocessor::test_macro_replace_5()
{
    CppPreprocessor preprocessor;
    preprocessor.addHardDefineByLine("#define NNN 123");
    preprocessor.addHardDefineByLine("#define TEST NNN");
    QCOMPARE("ttt 123 123",
             preprocessor.expandMacros("ttt TEST TEST"));
}

void TestCppPreprocessor::test_macro_replace_6()
{
    CppPreprocessor preprocessor;
    QFileInfo info("resources/preprocessor-macros-2.cpp");
    preprocessor.preprocess(info.absoluteFilePath());
    QStringList text1 = filterIncludes(preprocessor.result());
    QStringList text2 = readFileToLines("resources/preprocessor-macros-2-result.cpp");
    QVERIFY(!text1.isEmpty());
    QVERIFY(!text2.isEmpty());
    QCOMPARE(text1,text2);
}

void TestCppPreprocessor::test_macro_replace_7()
{
    CppPreprocessor preprocessor;
    QFileInfo info("resources/preprocessor-macros-3.cpp");
    preprocessor.preprocess(info.absoluteFilePath());
    QStringList text1 = filterIncludes(preprocessor.result());
    QStringList text2 = readFileToLines("resources/preprocessor-macros-3-result.cpp");
    QVERIFY(!text1.isEmpty());
    QVERIFY(!text2.isEmpty());
    QCOMPARE(text1,text2);
}

void TestCppPreprocessor::test_macro_replace_8()
{
    CppPreprocessor preprocessor;
    QFileInfo info("resources/preprocessor-macros-4.cpp");
    preprocessor.preprocess(info.absoluteFilePath());
    QStringList text1 = filterIncludes(preprocessor.result());
    QStringList text2 = readFileToLines("resources/preprocessor-macros-4-result.cpp");
    QVERIFY(!text1.isEmpty());
    QVERIFY(!text2.isEmpty());
    QCOMPARE(text1,text2);
}

void TestCppPreprocessor::test_replace_macros_in_constant_expression_1()
{
    CppPreprocessor preprocessor;
    preprocessor.addHardDefineByLine("#define __cplusplus 201703L");
    preprocessor.addHardDefineByLine("#define __has_builtin(x) 0");
    preprocessor.addHardDefineByLine("#define __GNUC__ 15");
    preprocessor.addHardDefineByLine("#define __GNUC_MINOR__ 3");
    preprocessor.addHardDefineByLine("#define __MINGW_GNUC_PREREQ(major, minor) (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))");
    QCOMPARE(preprocessor.expandMacrosInConditioningExpression("__cplusplus"),
             "201703L");
    QCOMPARE(preprocessor.expandMacrosInConditioningExpression("__cplusplus > 201703L"),
             "201703L > 201703L");
    QCOMPARE(preprocessor.expandMacrosInConditioningExpression("defined(__cplusplus)"),
             "1");
    QCOMPARE(preprocessor.expandMacrosInConditioningExpression("defined __cplusplus"),
             "1");
    QCOMPARE(preprocessor.expandMacrosInConditioningExpression("defined(__cplus)"),
             "0");
    QCOMPARE(preprocessor.expandMacrosInConditioningExpression("defined __cplus"),
             "0");

    QCOMPARE(preprocessor.expandMacrosInConditioningExpression("__has_builtin(__builtin_is_constant_evaluated)"),
             "0");


}

void TestCppPreprocessor::test_replace_macros_in_constant_expression_2()
{
    CppPreprocessor preprocessor;
    preprocessor.addHardDefineByLine("#define EMPTY");
    preprocessor.addHardDefineByLine("#define SCAN(x)     x");
    preprocessor.addHardDefineByLine("#define EXAMPLE_()  EXAMPLE");
    preprocessor.addHardDefineByLine("#define EXAMPLE(n)  EXAMPLE_ EMPTY()(n-1) (n)");
    QCOMPARE(preprocessor.expandMacrosInConditioningExpression("EXAMPLE(5)"),
             "EXAMPLE_ ()(5-1) (5)");
    QCOMPARE(preprocessor.expandMacrosInConditioningExpression("SCAN(EXAMPLE(5))"),
             "EXAMPLE_ ()(5-1-1) (5-1)  (5)");
}

void TestCppPreprocessor::test_evaluate_constant_expression_1()
{
    CppPreprocessor preprocessor;
    QCOMPARE(preprocessor.evaluateIf("0(0)"),false);
    QCOMPARE(preprocessor.evaluateIf("1 && !1 && !0    && !0"),false);
    QCOMPARE(preprocessor.evaluateIf("1&& (8* 4) <= 32"), true);
    QCOMPARE(preprocessor.evaluateIf("1&& (8* 4) == 32"), true);
    QCOMPARE(preprocessor.evaluateIf("0 || 1"), true);
    QCOMPARE(preprocessor.evaluateIf("1 || 0"), true);
    QCOMPARE(preprocessor.evaluateIf("1 || 1"), true);
    QCOMPARE(preprocessor.evaluateIf("0 || 0"), false);
    QCOMPARE(preprocessor.evaluateIf("(0 || 1) && 1"), true);
    QCOMPARE(preprocessor.evaluateIf("(1 || 0) && 1"), true);
    QCOMPARE(preprocessor.evaluateIf("(1 || 1) && 1"), true);
    QCOMPARE(preprocessor.evaluateIf("(0 || 0) && 1"), false);
    QCOMPARE(preprocessor.evaluateIf("1 && (0 || 1)"), true);
    QCOMPARE(preprocessor.evaluateIf("1 && (1 || 0)"), true);
    QCOMPARE(preprocessor.evaluateIf("1 && (1 || 1)"), true);
    QCOMPARE(preprocessor.evaluateIf("1 && (0 || 0)"), false);
    QCOMPARE(preprocessor.evaluateIf("0 && 1"), false);
    QCOMPARE(preprocessor.evaluateIf("1 && 0"), false);
    QCOMPARE(preprocessor.evaluateIf("1 && 1"), true);
    QCOMPARE(preprocessor.evaluateIf("0 && 0"), false);
    QCOMPARE(preprocessor.evaluateIf("(0 && 1) || 0"), false);
    QCOMPARE(preprocessor.evaluateIf("(1 && 0) || 0"), false);
    QCOMPARE(preprocessor.evaluateIf("(1 && 1) || 0"), true);
    QCOMPARE(preprocessor.evaluateIf("(0 && 0) || 0"), false);
    QCOMPARE(preprocessor.evaluateIf("0 || (0 && 1)"), false);
    QCOMPARE(preprocessor.evaluateIf("0 || (1 && 0)"), false);
    QCOMPARE(preprocessor.evaluateIf("0 || (1 && 1)"), true);
    QCOMPARE(preprocessor.evaluateIf("0 || (0 && 0)"), false);
    QCOMPARE(preprocessor.evaluateIf("!0"), true);
    QCOMPARE(preprocessor.evaluateIf("!1"), false);
    QCOMPARE(preprocessor.evaluateIf("8*4 == 32"), true);
    QCOMPARE(preprocessor.evaluateIf("8*4 >= 32"), true);
    QCOMPARE(preprocessor.evaluateIf("8*4 <= 32"), true);
    QCOMPARE(preprocessor.evaluateIf("8*4 < 32"), false);
    QCOMPARE(preprocessor.evaluateIf("8*4 > 32"), false);
    QCOMPARE(preprocessor.evaluateIf("8*4 != 32"), false);
    QCOMPARE(preprocessor.evaluateIf("0x10 > 15"), true);
    QCOMPARE(preprocessor.evaluateIf("0x10 == 16"), true);
    QCOMPARE(preprocessor.evaluateIf("0x10 < 17"), true);
    QCOMPARE(preprocessor.evaluateIf("0x10L == 16"), true);
    QCOMPARE(preprocessor.evaluateIf("0x10UL == 16"), true);
    QCOMPARE(preprocessor.evaluateIf("0x10LL == 16"), true);
    QCOMPARE(preprocessor.evaluateIf("0x10ULL == 16"), true);
    QCOMPARE(preprocessor.evaluateIf("010 > 7"), true);
    QCOMPARE(preprocessor.evaluateIf("010 == 8"), true);
    QCOMPARE(preprocessor.evaluateIf("010 < 9"), true);
    QCOMPARE(preprocessor.evaluateIf("010L == 8"), true);
    QCOMPARE(preprocessor.evaluateIf("010UL == 8"), true);
    QCOMPARE(preprocessor.evaluateIf("010LL == 8"), true);
    QCOMPARE(preprocessor.evaluateIf("010ULL == 8"), true);
    QCOMPARE(preprocessor.evaluateIf("(15> (14) || (15== (14) && 3>= (15)))"), true);
    QCOMPARE(preprocessor.evaluateIf("(0 > 3 || (0 == 3 && 0 >= 1)) &&     !1  "), false);
    QCOMPARE(preprocessor.evaluateIf("1?0:1"), false);
    QCOMPARE(preprocessor.evaluateIf("0?0:1"), true);
    QCOMPARE(preprocessor.evaluateIf("1?1:0"), true);
    QCOMPARE(preprocessor.evaluateIf("0?1:0"), false);

}

void TestCppPreprocessor::test_evaluate_constant_expression_2()
{
    CppPreprocessor preprocessor;
    preprocessor.addHardDefineByLine("#define __cplusplus 202002L");
    preprocessor.addHardDefineByLine("#define __has_builtin(x) 0");
    preprocessor.addHardDefineByLine("#define __GNUC__ 15");
    preprocessor.addHardDefineByLine("#define __GNUC_MINOR__ 3");
    preprocessor.addHardDefineByLine("#define __MINGW_GNUC_PREREQ(major, minor) (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))");
    QCOMPARE(preprocessor.evaluateIf("__MINGW_GNUC_PREREQ(14,15)"), true);
    QCOMPARE(preprocessor.evaluateIf("__MINGW_GNUC_PREREQ(15,2)"), true);
    QCOMPARE(preprocessor.evaluateIf("__MINGW_GNUC_PREREQ(15,3)"), true);
    QCOMPARE(preprocessor.evaluateIf("__MINGW_GNUC_PREREQ(15,4)"), false);
    QCOMPARE(preprocessor.evaluateIf("__MINGW_GNUC_PREREQ(16,0)"), false);
}

void TestCppPreprocessor::test_compare_cpp_version()
{
    CppPreprocessor preprocessor;
    preprocessor.clear();
    preprocessor.addHardDefineByLine("#define __cplusplus 199711L");
    QCOMPARE(preprocessor.evaluateIf("defined(__cplusplus)"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201103L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201402L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201703L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 202002L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202002L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202302L"), false);

    preprocessor.clear();
    preprocessor.addHardDefineByLine("#define __cplusplus 201103L");
    QCOMPARE(preprocessor.evaluateIf("defined(__cplusplus)"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201103L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201402L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201703L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 202002L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202002L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202302L"), false);

    preprocessor.clear();
    preprocessor.addHardDefineByLine("#define __cplusplus 201402L");
    QCOMPARE(preprocessor.evaluateIf("defined(__cplusplus)"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201103L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201402L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201703L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 202002L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202002L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202302L"), false);

    preprocessor.clear();
    preprocessor.addHardDefineByLine("#define __cplusplus 201703L");
    QCOMPARE(preprocessor.evaluateIf("defined(__cplusplus)"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201103L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201402L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201703L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 202002L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202002L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202302L"), false);


    preprocessor.clear();
    preprocessor.addHardDefineByLine("#define __cplusplus 202002L");
    QCOMPARE(preprocessor.evaluateIf("defined(__cplusplus)"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201103L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201402L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201703L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 202002L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202002L"), false);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202302L"), false);

    preprocessor.clear();
    preprocessor.addHardDefineByLine("#define __cplusplus 202302L");
    QCOMPARE(preprocessor.evaluateIf("defined(__cplusplus)"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201103L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201402L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201703L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 202002L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202002L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202302L"), false);

    preprocessor.clear();
    preprocessor.addHardDefineByLine("#define __cplusplus 202400L");
    QCOMPARE(preprocessor.evaluateIf("defined(__cplusplus)"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201103L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201402L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 201703L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus >= 202002L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202002L"), true);
    QCOMPARE(preprocessor.evaluateIf("__cplusplus > 202302L"), true);

}

QStringList TestCppPreprocessor::filterIncludes(const QStringList &text)
{
    QStringList result;
    foreach(const QString& s, text) {
        if (!s.startsWith("#include"))
            result.append(s);
    }
    return result;
}

QStringList TestCppPreprocessor::preprocessContent(CppPreprocessor &p, const QStringList &lines)
{
    p.setOnGetFileStream([&lines](const QString&, QStringList& out){
        out = lines;
        return true;
    });
    p.preprocess("/virtual/main.cpp");
    return p.result();
}

void TestCppPreprocessor::test_combine_backslash_trailing_space()
{
    // Current behavior is intentionally lenient: a backslash followed by
    // trailing spaces still splices the lines (the C standard requires the
    // backslash to be immediately followed by newline).
    QStringList text{"abc\\  ", "def"};
    CppPreprocessor::combineLinesEndingWithBackslash(text);
    QCOMPARE(text, QStringList({"abcdef", ""}));
}

void TestCppPreprocessor::test_combine_backslash_multiple()
{
    QStringList text{"a\\", "b\\", "c"};
    CppPreprocessor::combineLinesEndingWithBackslash(text);
    QCOMPARE(text, QStringList({"abc", "", ""}));
}

void TestCppPreprocessor::test_combine_backslash_last_line()
{
    // Backslash on the last line: nothing to merge with, line kept as-is.
    QStringList text{"abc\\"};
    CppPreprocessor::combineLinesEndingWithBackslash(text);
    QCOMPARE(text, QStringList({"abc\\"}));
}

void TestCppPreprocessor::test_combine_backslash_in_block_comment()
{
    // Line splicing happens before comment removal, so a block comment
    // split by a line continuation stays intact.
    QStringList text{"/* com\\", "ment */ int x;"};
    CppPreprocessor::combineLinesEndingWithBackslash(text);
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({"  int x;", ""}));
}

void TestCppPreprocessor::test_backslash_in_directive_name()
{
    // Line splicing happens before directive parsing, so even the directive
    // name may be split across lines.
    CppPreprocessor p;
    QCOMPARE(preprocessContent(p, {"#def\\", "ine X 1", "X"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "#define X 1",
                 "",
                 "1"}));
    QCOMPARE(p.getDefine("X")->value, "1");
}

void TestCppPreprocessor::test_backslash_in_string()
{
    CppPreprocessor p;
    QCOMPARE(preprocessContent(p, {"const char* s = \"ab\\", "c\";"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "const char* s = \"abc\";",
                 ""}));
}

void TestCppPreprocessor::test_backslash_result_line_alignment()
{
    // Spliced-away lines become empty lines so that result lines stay
    // aligned with the physical source lines.
    CppPreprocessor p;
    QStringList src{"int a;\\", "int b;", "int c;"};
    QStringList result = preprocessContent(p, src);
    QCOMPARE(result, QStringList({
                 "#include /virtual/main.cpp:-1",
                 "int a;int b;",
                 "",
                 "int c;"}));
    QCOMPARE(result.count(), 1 + src.count());
}
