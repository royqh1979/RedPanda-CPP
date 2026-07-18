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
private:
    static QStringList filterIncludes(const QStringList& text);
};

#endif
