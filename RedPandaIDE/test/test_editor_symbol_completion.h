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
    void test_input_parenthesis_in_string();
    void test_input_parenthesis_in_comment();
    void test_input_parenthesis_in_escaping_sequence();
    void test_input_parenthesis_on_selection();

    void test_input_brackets1();
    void test_input_brackets_in_string();
    void test_input_brackets_in_comment();
    void test_input_brackets_in_escaping_sequence();
    void test_input_brackets_on_selection();

    void test_input_braces1();
    void test_input_braces_in_string();
    void test_input_braces_in_comment();
    void test_input_braces_in_escaping_sequence();
    void test_input_braces_on_selection();
    void test_input_braces_with_indent();

    void test_input_asterisk_for_ansi_c_comments();
    void test_input_asterisk_in_comments();
    void test_input_asterisk_in_comments2();
    void test_input_asterisk_in_char_literal();
    void test_input_asterisk_in_string_literal();
    void test_input_asterisk_in_rawstring_begin();
    void test_input_asterisk_in_rawstring();
    void test_input_asterisk_in_rawstring_end();

    void test_input_periods1();
    void test_input_periods_in_comments();
    void test_input_periods_in_string();

    void test_input_docstring();
    void test_completion_disabled_by_settings_data();
    void test_completion_disabled_by_settings();
    void test_symbol_skip_settings_data();
    void test_symbol_skip_settings();
    void test_global_include_completion();
    void test_remove_symbol_pairs_data();
    void test_remove_symbol_pairs();
    void test_code_snippet_editing_areas_data();
    void test_code_snippet_editing_areas();
    void test_doxygen_header_editing_areas_data();
    void test_doxygen_header_editing_areas();
    void test_input_after_cpp_comment();

private:
    static void addIndentationCases();
    void configureIndentation(bool autoIndent, bool tabsToSpaces);
    void assertEditingArea(int line, int begin, int end);
};

#endif
