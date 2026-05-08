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
private:
    static QStringList filterIncludes(const QStringList& text);
};

#endif
