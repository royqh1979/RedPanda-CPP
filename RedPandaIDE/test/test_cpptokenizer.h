#ifndef TEST_CPPTOKENIZER_H
#define TEST_CPPTOKENIZER_H

#include "test_cppparsercomponenttestcase.h"

class TestCppTokenizer : public CppParserComponentTestCase
{
    Q_OBJECT
private slots:
    void test_identifiers_and_operators();
    void test_qualified_names_and_templates_are_split();
    void test_array_and_lambda_brackets_are_split();
    void test_integer_and_floating_literals();
    void test_string_and_character_literals();
    void test_escaped_literals();
    void test_raw_string_with_custom_delimiter();
    void test_unterminated_raw_string();
    void test_balanced_match_indices();
    void test_unmatched_pairs_use_file_end();
    void test_file_boundaries_and_file_names();
    void test_cross_file_match_indices();
    void test_unmatched_include_pair_uses_include_end();
    void test_literal_prefixes();
    void test_attributes_are_skipped();
    void test_preprocessor_tokens();
    void test_preprocessor_cursor_stays_valid_after_continued_lines();
    void test_line_numbers();
    void test_character_positions();
    void test_cursor_advance_and_checkpoint();
    void test_greater_token_split_and_rollback();
    void test_same_file_match_skip();
    void test_unmatched_match_skip_to_file_end();
    void test_file_end_token();
    void test_include_end_match_skip();
    void test_cross_file_match_skip();
    void test_invalid_match_skip();
};

#endif // TEST_CPPTOKENIZER_H
