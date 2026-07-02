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
    void test_parse_ustring1();
    void test_parse_ustring2();
    void test_parse_wide_string();
    void test_parse_u8_string();
    void test_parse_raw_string1();
    void test_parse_raw_string2();
    void test_parse_raw_string3();
    void test_parse_raw_string4();
    void test_parse_raw_string5();
    void test_parse_char();
    void test_parse_wide_char();
    void test_parse_unicode_char1();
    void test_parse_unicode_char2();
    void test_parse_u8_char();
    void test_parse_scope_resolution_operators1();
    void test_parse_scope_resolution_operators2();
    void test_parse_scope_resolution_operators3();
    void test_parse_unend_char_literal();
    void test_parse_unend_string_literal();
protected:
    CppTokenizer mTokenizer;
};

#endif
