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

void TestCppPreprocessor::test_comment_markers_inside_string()
{
    // Comment markers inside string literals must not start a comment.
    QStringList text{
        "const char* a = \"http://example\";",
        "const char* b = \"x/*y\";",
        "int c; // trailing"};
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({
                "const char* a = \"http://example\";",
                "const char* b = \"x/*y\";",
                "int c; "}));
}

void TestCppPreprocessor::test_escaped_quote_in_string()
{
    // An escaped quote must not terminate the string literal.
    QStringList text{
        "const char* s = \"a\\\"b\"; // c",
        "int x;"};
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({
                "const char* s = \"a\\\"b\"; ",
                "int x;"}));
}

void TestCppPreprocessor::test_char_literals()
{
    QStringList text{
        "char a = '\\'';",
        "char b = '\\\\';",
        "char c = '\\n';",
        "int d = 'ab'; // x"};
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({
                "char a = '\\'';",
                "char b = '\\\\';",
                "char c = '\\n';",
                "int d = 'ab'; "}));
}

void TestCppPreprocessor::test_quote_inside_comment()
{
    // A quote inside a block comment must not start a string literal.
    QStringList text{"/* \" */ int x; // y"};
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({"  int x; "}));
}

void TestCppPreprocessor::test_raw_string_basic()
{
    // PINNED (suspect, phase 0.5 review): raw string support in
    // replaceCommentsBySpaceChar never terminates the raw-string state
    // (the closing check wants ")" + delimiter where delimiter wrongly
    // contains the opening quote), so the trailing comment survives and
    // the state leaks into the following lines.
    QStringList text{
        "const char* s = R\"(x)\"; // trailing",
        "int y; // also kept"};
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({
                "const char* s = R\"(x)\"; // trailing",
                "int y; // also kept"}));
}

void TestCppPreprocessor::test_raw_string_delimiter()
{
    // PINNED (suspect, phase 0.5 review): the custom delimiter is never
    // recorded either, so a delimited raw string also never terminates
    // (see test_raw_string_basic).
    QStringList text{
        "const char* s = R\"delim(a ) still)delim\"; // c",
        "int y;"};
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({
                "const char* s = R\"delim(a ) still)delim\"; // c",
                "int y;"}));
}

void TestCppPreprocessor::test_raw_string_with_comment_markers()
{
    // Comment-looking text inside a raw string is not a comment.
    // (Passes today, though only because the raw-string state never
    // terminates - see test_raw_string_basic.)
    QStringList text{
        "const char* s = R\"(a // not a comment)\";"};
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({
                "const char* s = R\"(a // not a comment)\";"}));
}

void TestCppPreprocessor::test_raw_string_multiline()
{
    // PINNED (suspect, phase 0.5 review): same root cause as
    // test_raw_string_basic; the raw-string state also never resets at
    // line boundaries, so the comment on the last line survives too.
    QStringList text{
        "const char* s = R\"(multi",
        "line)\";",
        "int z; // c"};
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({
                "const char* s = R\"(multi",
                "line)\";",
                "int z; // c"}));
}

void TestCppPreprocessor::test_raw_string_after_identifier()
{
    // PINNED (suspect, phase 0.5 review): an identifier ending with 'R'
    // followed by a string (COLORR"(y)") is consumed as identifier +
    // regular string, but a bare R"..." still triggers the raw-string
    // state (which then never terminates, see test_raw_string_basic).
    QStringList text{
        "const char* a = R\"(x)\";",
        "const char* b = COLORR\"(y)\"; // c"};
    CppPreprocessor::replaceCommentsBySpaceChar(text);
    QCOMPARE(text, QStringList({
                "const char* a = R\"(x)\";",
                "const char* b = COLORR\"(y)\"; // c"}));
}

void TestCppPreprocessor::test_macro_not_expanded_in_string()
{
    CppPreprocessor p;
    p.addHardDefineByLine("#define X 1");
    QCOMPARE(p.expandMacros("const char* s = \"X\";"),
             "const char* s = \"X\";");
    QCOMPARE(p.expandMacros("char c = 'X';"),
             "char c = 'X';");
    QCOMPARE(p.expandMacros("int a = X;"),
             "int a = 1;");
}

void TestCppPreprocessor::test_raw_string_pipeline_canary()
{
    // PINNED (suspect, phase 0.5 review): end-to-end consequence of the
    // raw-string bug - comments after a raw string are not stripped, and
    // expandMacros (which knows strings but not comments) then expands
    // macro names inside the surviving comment.
    CppPreprocessor p;
    p.addHardDefineByLine("#define X 1");
    QCOMPARE(preprocessContent(p, {
                 "const char* s = R\"(raw)\"; // X",
                 "int a = X;"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "const char* s = R\"(raw)\"; // 1",
                 "int a = 1;"}));
}

void TestCppPreprocessor::test_stringize_with_spaces()
{
    // PINNED: the argument is trimmed but internal whitespace runs are
    // not collapsed (the C standard requires each whitespace sequence to
    // become a single space).
    CppPreprocessor p;
    p.addHardDefineByLine("#define S(x) #x");
    QCOMPARE(p.expandMacros("S(a  b)"), "\"a  b\"");
}

void TestCppPreprocessor::test_stringize_escaping()
{
    // PINNED (suspect, phase 0.5 review), two distinct issues:
    // - a quote inside the argument list breaks the naive string tracking
    //   of the argument scan, so the macro is left completely unexpanded;
    // - backslashes inside the argument are not escaped when stringizing,
    //   producing an ill-formed string literal.
    CppPreprocessor p;
    p.addHardDefineByLine("#define S(x) #x");
    QCOMPARE(p.expandMacros("S(a\"b)"), "S(a\"b)");
    QCOMPARE(p.expandMacros("S(a\\b)"), "\"a\\b\"");
}

void TestCppPreprocessor::test_token_paste_edges()
{
    // PINNED: '##' at the start/end of the macro body degenerates to
    // plain argument substitution.
    CppPreprocessor p;
    p.addHardDefineByLine("#define P(a) ##a");
    p.addHardDefineByLine("#define Q(a) a##");
    QCOMPARE(p.expandMacros("P(x)"), "x");
    QCOMPARE(p.expandMacros("Q(x)"), "x");
}

void TestCppPreprocessor::test_token_paste_chain()
{
    // Consecutive '##' operators chain the arguments together.
    CppPreprocessor p;
    p.addHardDefineByLine("#define CAT3(a,b,c) a##b##c");
    QCOMPARE(p.expandMacros("CAT3(x,y,z)"), "xyz");
}

void TestCppPreprocessor::test_named_args_with_varargs()
{
    // Named parameters and __VA_ARGS__ can be mixed.
    CppPreprocessor p;
    p.addHardDefineByLine("#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)");
    QCOMPARE(p.expandMacros("LOG(\"%d\", 1)"), "printf(\"%d\", 1)");
}

void TestCppPreprocessor::test_args_comma_in_parens()
{
    // Commas inside parentheses do not split macro arguments.
    CppPreprocessor p;
    p.addHardDefineByLine("#define F(a,b) a+b");
    QCOMPARE(p.expandMacros("F((1,2),3)"), "(1,2)+3");
}

void TestCppPreprocessor::test_args_comma_in_template_angle()
{
    // PINNED (known deviation from the standard): angle brackets are not
    // tracked when splitting macro arguments, so a comma inside template
    // arguments splits the parameter and the extra argument is dropped.
    CppPreprocessor p;
    p.addHardDefineByLine("#define F(a,b) a+b");
    QCOMPARE(p.expandMacros("F(std::pair<int,int>, 3)"), "std::pair<int+int>");
}

void TestCppPreprocessor::test_empty_args()
{
    CppPreprocessor p;
    p.addHardDefineByLine("#define F(a,b) a+b");
    QCOMPARE(p.expandMacros("F(,)"), "+");
}

void TestCppPreprocessor::test_empty_varargs()
{
    // PINNED (suspect, phase 0.5 review): the space before '#' is lost by
    // tokenizeValue (the pending Space token is discarded when the '#'
    // token overwrites it), so the expansion has no space between WORD
    // and the stringized (empty) argument list.
    CppPreprocessor p;
    p.addHardDefineByLine("#define OUTER(...) WORD #__VA_ARGS__");
    QCOMPARE(p.expandMacros("OUTER()"), "WORD\"\"");
}

void TestCppPreprocessor::test_chain_rescan_object_to_function()
{
    // An object-like macro expanding to a function-like macro name is
    // rescanned (ISO 6.10.3.4) and the function-like macro is expanded.
    CppPreprocessor p;
    p.addHardDefineByLine("#define A B");
    p.addHardDefineByLine("#define B(x) x+1");
    QCOMPARE(p.expandMacros("A(5)"), "5+1");
}

void TestCppPreprocessor::test_mutual_recursion_function_macros()
{
    // Mutual recursion is terminated by the ignore-set (there is no
    // depth limit; MAX_DEFINE_EXPAND_DEPTH is unused).
    CppPreprocessor p;
    p.addHardDefineByLine("#define F(x) G(x)");
    p.addHardDefineByLine("#define G(x) F(x)");
    QCOMPARE(p.expandMacros("F(1)"), "F(1)");
}

void TestCppPreprocessor::test_redefine_overwrites()
{
    CppPreprocessor p;
    p.addHardDefineByLine("#define X 1");
    p.addHardDefineByLine("#define X 2");
    QCOMPARE(p.expandMacros("X"), "2");
}

void TestCppPreprocessor::test_empty_body_macro()
{
    // An empty-body object macro expands to nothing; the surrounding
    // spaces are both kept.
    CppPreprocessor p;
    p.addHardDefineByLine("#define E");
    QCOMPARE(p.expandMacros("a E b"), "a  b");
    QCOMPARE(p.expandMacros("E"), "");
}

void TestCppPreprocessor::test_macro_word_boundaries()
{
    // Macros only expand as whole identifiers.
    CppPreprocessor p;
    p.addHardDefineByLine("#define N 12");
    QCOMPARE(p.expandMacros("x N y"), "x 12 y");
    QCOMPARE(p.expandMacros("NxN"), "NxN");
    QCOMPARE(p.expandMacros("N12"), "N12");
    QCOMPARE(p.expandMacros("N N"), "12 12");
}

void TestCppPreprocessor::test_if_undefined_identifier()
{
    // PINNED: undefined identifiers are kept verbatim in the expression,
    // so evaluation fails and the branch is taken as false. The C
    // standard requires them to be replaced by 0 (so "UNDEFINED == 0"
    // should be true).
    CppPreprocessor p;
    QCOMPARE(p.evaluateIf("UNDEFINED"), false);
    QCOMPARE(p.evaluateIf("UNDEFINED == 0"), false);
    QCOMPARE(p.evaluateIf("!defined(UNDEFINED)"), true);
}

void TestCppPreprocessor::test_nested_branches()
{
    CppPreprocessor p;
    QStringList result = preprocessContent(p, {
        "#define A 1",        // 1
        "#if A",              // 2
        "int l3;",            // 3
        "#ifndef B",          // 4
        "int l5;",            // 5
        "#if A == 2",         // 6
        "int l7;",            // 7
        "#else",              // 8
        "int l9;",            // 9
        "#endif",             // 10
        "int l11;",           // 11
        "#endif",             // 12
        "int l13;",           // 13
        "#endif",             // 14
        "int l15;"});         // 15
    QCOMPARE(result, QStringList({
                 "#include /virtual/main.cpp:-1",
                 "#define A 1",
                 "",
                 "int l3;",
                 "",
                 "int l5;",
                 "",
                 "",
                 "",
                 "int l9;",
                 "",
                 "int l11;",
                 "",
                 "int l13;",
                 "",
                 "int l15;"}));
    PParsedFileInfo fileInfo = p.findFileInfo("/virtual/main.cpp");
    QVERIFY(fileInfo);
    QVERIFY(fileInfo->isLineVisible(3));
    QVERIFY(!fileInfo->isLineVisible(6));
    // PINNED (suspect, phase 0.5 review): removeCurrentBranch records the
    // visibility restore at mIndex while setCurrentBranch uses mIndex+1,
    // so visibility comes back one line early and the last content line
    // of a false branch (line 7 here) is reported visible even though it
    // was preprocessed away. The result content above is correct; only
    // the branch map (used by the editor to gray out inactive code) is
    // off by one.
    QVERIFY(fileInfo->isLineVisible(7));
    QVERIFY(fileInfo->isLineVisible(9));
    QVERIFY(fileInfo->isLineVisible(15));
}

void TestCppPreprocessor::test_define_in_false_branch()
{
    CppPreprocessor p;
    QStringList result = preprocessContent(p, {
        "#if 0",
        "#define X 1",
        "#endif",
        "#ifdef X",
        "int yes;",
        "#else",
        "int no;",
        "#endif"});
    QCOMPARE(result, QStringList({
                 "#include /virtual/main.cpp:-1",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "int no;",
                 ""}));
    QVERIFY(!p.getDefine("X"));
}

void TestCppPreprocessor::test_elif_chain()
{
    // Once a branch has been taken, later #elif conditions are not
    // evaluated at all.
    CppPreprocessor p;
    preprocessContent(p, {
        "#define A 1",
        "#if A == 1",
        "#define WIN 1",
        "#elif A == 2",
        "#define LOSE2 1",
        "#elif A == 1",
        "#define LOSE3 1",
        "#else",
        "#define LOSE4 1",
        "#endif"});
    QVERIFY(p.getDefine("WIN"));
    QVERIFY(!p.getDefine("LOSE2"));
    QVERIFY(!p.getDefine("LOSE3"));
    QVERIFY(!p.getDefine("LOSE4"));
}

void TestCppPreprocessor::test_multiline_macro_call_in_true_branch()
{
    // A function-like macro call spanning lines consumes the following
    // lines; the consumed lines become empty lines in the result to keep
    // the line alignment.
    CppPreprocessor p;
    QCOMPARE(preprocessContent(p, {
                 "#define F(x) x+1",
                 "int a = F(",
                 "2);",
                 "int b;"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "#define F(x) x+1",
                 "int a = 2+1;",
                 "",
                 "int b;"}));
}

void TestCppPreprocessor::test_multiline_macro_call_in_false_branch()
{
    // In a false branch no macro expansion happens, so the multi-line
    // call does not disturb anything either.
    CppPreprocessor p;
    QCOMPARE(preprocessContent(p, {
                 "#if 0",
                 "int a = F(",
                 "2);",
                 "#endif",
                 "int b;"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "",
                 "",
                 "",
                 "",
                 "int b;"}));
}

void TestCppPreprocessor::test_unbalanced_directives()
{
    // PINNED (robustness): unbalanced directives must not crash; an
    // unmatched #endif is a no-op, and a stray #else at top level makes
    // the following lines invisible.
    CppPreprocessor p;
    QCOMPARE(preprocessContent(p, {
                 "int a;",
                 "#endif",
                 "int b;",
                 "#else",
                 "int c;"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "int a;",
                 "",
                 "int b;",
                 "",
                 ""}));
}

void TestCppPreprocessor::test_and_or_keywords()
{
    CppPreprocessor p;
    QCOMPARE(p.evaluateIf("1 and 1"), true);
    QCOMPARE(p.evaluateIf("1 and 0"), false);
    QCOMPARE(p.evaluateIf("0 or 1"), true);
    QCOMPARE(p.evaluateIf("0 or 0"), false);
}

void TestCppPreprocessor::test_char_constant_in_if()
{
    // PINNED (suspect, phase 0.5 review): character constants are not
    // supported in #if expressions (the C standard evaluates 'a' as its
    // code point, so this should be true).
    CppPreprocessor p;
    QCOMPARE(p.evaluateIf("'a' == 97"), false);
    QCOMPARE(p.evaluateIf("'a'"), false);
}

void TestCppPreprocessor::test_elifdef_elifndef_supported()
{
    // NOTE: the C++23 detection in addDefineByParts uses
    // QString::toLong() on the whole __cplusplus value, which fails for
    // real-world values with the 'L' suffix (see
    // test_elifdef_cpp23_detection). The supported path is only reached
    // with a suffix-less value.
    CppPreprocessor p;
    p.addHardDefineByLine("#define __cplusplus 202302");
    QCOMPARE(preprocessContent(p, {
                 "#ifdef CPU",
                 "int a;",
                 "#elifdef GPU",
                 "int b;",
                 "#elifndef RAM",
                 "int c;",
                 "#else",
                 "int d;",
                 "#endif",
                 "int e;"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "int c;",
                 "",
                 "",
                 "",
                 "int e;"}));
}

void TestCppPreprocessor::test_elifdef_cpp23_detection()
{
    // Regression test: real-world __cplusplus values carry an integer
    // suffix (202302L); the C++23 detection must handle it (it once ran
    // QString::toLong on the whole value and never detected C++23).
    CppPreprocessor p;
    p.addHardDefineByLine("#define __cplusplus 202302L");
    QCOMPARE(p.evaluateIf("__cplusplus >= 202302L"), true);
    QCOMPARE(preprocessContent(p, {
                 "#ifdef CPU",
                 "int a;",
                 "#elifdef GPU",
                 "int b;",
                 "#elifndef RAM",
                 "int c;",
                 "#else",
                 "int d;",
                 "#endif",
                 "int e;"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "int c;",
                 "",
                 "",
                 "",
                 "int e;"}));
}

void TestCppPreprocessor::test_elifdef_elifndef_unsupported()
{
    // PINNED (suspect, phase 0.5 review): when the compiler predates
    // C++23, the #elifdef/#elifndef handlers do nothing at all - the
    // branch stack is left untouched, so the enclosing #if branch stays
    // in effect and a following #else matches that #if instead.
    CppPreprocessor p;
    p.addHardDefineByLine("#define __cplusplus 202002L");
    QCOMPARE(preprocessContent(p, {
                 "#ifdef CPU",
                 "int a;",
                 "#elifdef GPU",
                 "int b;",
                 "#elifndef RAM",
                 "int c;",
                 "#else",
                 "int d;",
                 "#endif",
                 "int e;"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "",
                 "int d;",
                 "",
                 "int e;"}));
}
void TestCppPreprocessor::test_undef_local()
{
    CppPreprocessor p;
    QCOMPARE(preprocessContent(p, {
                 "#define X 1",
                 "#undef X",
                 "int after;"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "#define X 1",
                 "#undef X",
                 "int after;"}));
    QVERIFY(!p.getDefine("X"));
}

void TestCppPreprocessor::test_leading_space_directive()
{
    // PINNED (suspect, phase 0.5 review): only lines whose very first
    // character is '#' are treated as directives; the C standard allows
    // whitespace before '#'. Here the line goes through macro expansion
    // as ordinary text and the define never happens.
    CppPreprocessor p;
    QCOMPARE(preprocessContent(p, {
                 "  #define X 1",
                 "int after;"}),
             QStringList({
                 "#include /virtual/main.cpp:-1",
                 "  #define X 1",
                 "int after;"}));
    QVERIFY(!p.getDefine("X"));
}

void TestCppPreprocessor::test_conditional1_resource()
{
    // The resource file documents the expected #elifdef/#elifndef
    // behavior in both modes (see the comments in conditional1.cpp);
    // the two golden files pin the C++23-supported mode and the
    // degraded legacy mode. The legacy mode is what real compilers
    // currently get: the 'L'-suffixed __cplusplus value defeats the
    // C++23 detection (see test_elifdef_cpp23_detection).
    QFileInfo info("resources/conditional1.cpp");
    QString absPath = info.absoluteFilePath();
    auto run = [&](const QString& cplusplusDefine, const QString& goldenName) {
        CppPreprocessor p;
        p.addHardDefineByLine(cplusplusDefine);
        p.preprocess(absPath);
        QStringList result = p.result();
        for (QString& line : result)
            line.replace(absPath, "<PATH>");
        QCOMPARE(result, readFileToLines(goldenName));
    };
    run("#define __cplusplus 202302", "resources/conditional1-result-cpp23.cpp");
    run("#define __cplusplus 202002L", "resources/conditional1-result-legacy.cpp");
}
