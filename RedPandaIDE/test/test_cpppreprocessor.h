#ifndef TEST_CPPPREPROCESSOR_H
#define TEST_CPPPREPROCESSOR_H
#include <QObject>
#include <memory>

class CppPreprocessor;

class TestCppPreprocessor: public QObject
{
    Q_OBJECT
public:
    TestCppPreprocessor(QObject *parent=nullptr);
private slots:
    void test_combine_lines_ending_with_backslash();
    void test_replace_comments_with_space_char();
    void test_macro_replace_1();
    void test_macro_replace_2();
    void test_macro_replace_3();
    void test_macro_replace_4();
    void test_macro_replace_5();
    void test_macro_replace_6();
    void test_macro_replace_7();
    void test_macro_replace_8();
    void test_replace_macros_in_constant_expression_1();
    void test_replace_macros_in_constant_expression_2();
    void test_evaluate_constant_expression_1();
    void test_evaluate_constant_expression_2();
    void test_compare_cpp_version();
    // group A: line continuation
    void test_combine_backslash_trailing_space();
    void test_combine_backslash_multiple();
    void test_combine_backslash_last_line();
    void test_combine_backslash_in_block_comment();
    void test_backslash_in_directive_name();
    void test_backslash_in_string();
    void test_backslash_result_line_alignment();
private:
    static QStringList filterIncludes(const QStringList& text);
    // Run the full preprocess pipeline over in-memory content
    // (line splicing + comment removal + directive handling + macro expansion).
    static QStringList preprocessContent(CppPreprocessor& p, const QStringList& lines);
};

#endif
