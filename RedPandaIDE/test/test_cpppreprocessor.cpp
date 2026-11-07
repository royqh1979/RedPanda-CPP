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
