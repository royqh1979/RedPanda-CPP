#ifndef TEST_CPPTOKENIZER_H
#define TEST_CPPTOKENIZER_H
#include <QObject>
#include <memory>
#include "src/parser/cpptokenizer.h"
class TestCppTokenizer: public QObject
{
    Q_OBJECT
public:
    TestCppTokenizer(QObject *parent=nullptr);
private slots:
    void test_parse_string1();
    void test_parse_string2();
    void test_parse_char();
    void test_parse_scope_resolution_operators1();
    void test_parse_scope_resolution_operators2();
protected:
    CppTokenizer mTokenizer;
};

#endif
