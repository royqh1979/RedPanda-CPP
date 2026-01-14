#ifndef TESTCPPSYNTAXER_H
#define TESTCPPSYNTAXER_H

#include <QObject>
#include "qsynedit/syntaxer/cpp.h"

class TestCppSyntaxer : public QObject
{
    Q_OBJECT
public:
    explicit TestCppSyntaxer(QObject *parent = nullptr);
private slots:
    void test_integer_literal1();
    void test_integer_literal2();
    void test_integer_literal3();

    void test_float_literal1();

    void test_string_literal1();
    void test_string_literal2();
    void test_string_literal3();

    void test_rawstring_literal1();
    void test_rawstring_literal2();
    void test_rawstring_literal3();
    void test_rawstring_literal4();
private:
    QSynedit::CppSyntaxer mSyntaxer;

};

#endif // TESTCPPSYNTAXER_H
