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
    void test_macros_1();
private:
    static QStringList filterIncludes(const QStringList& text);
};

#endif
