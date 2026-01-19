#ifndef TEST_EDITOR_SYMBOL_COMPLETION_H
#define TEST_EDITOR_SYMBOL_COMPLETION_H
#include "test_editor_base.h"

class TestEditorSymbolCompletion : public TestEditorBase
{
    Q_OBJECT
public:
    TestEditorSymbolCompletion(QObject *parent=nullptr);
private slots:
    void test_input_double_quotes_in_string1();
    void test_input_double_quotes_in_string2();
    void test_input_double_quotes_in_string3();
    void test_input_double_quotes_in_string4();
    void test_input_double_quotes_in_string5();
    void test_input_double_quotes_on_selection();
    void test_input_double_quotes_in_raw_string1();
    void test_input_double_quotes_in_raw_string2();
    void test_input_double_quotes_in_raw_string3();
    void test_input_double_quotes_in_raw_string4();
    void test_input_double_quotes_in_char_literals1();
    void test_input_double_quotes_in_char_literals2();
    void test_input_double_quotes_in_comments();

    void test_input_single_quotes_in_char_literals1();
    void test_input_single_quotes_in_char_literals2();
    void test_input_single_quotes_in_char_literals3();
    void test_input_single_quotes_in_string_literals();
    void test_input_single_quotes_in_rawstring_not_escaping();
    void test_input_single_quotes_in_rawstring_start();
    void test_input_single_quotes_in_rawstring_end();
    void test_input_single_quotes_in_comments1();
    void test_input_single_quotes_in_comments2();
    void test_input_single_quotes_on_selection();
    void test_input_single_quotes_in_number();

    void test_input_parenthesis1();
    void test_input_brackets1();
    void test_input_braces1();
    void test_input_parenthesis_in_string();
    void test_input_parenthesis_in_comment();

    void test_input_asterisk();
    void test_input_asterisk1();
    void test_input_asterisk2();
    void test_input_asterisk3();

};

#endif
