#ifndef TEST_CPPPREPROCESSOR_H
#define TEST_CPPPREPROCESSOR_H
#include <QObject>
#include <memory>
#include "src/parser/cpppreprocessor.h"

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
    void test_evaluate_constant_expression_1();
    void test_evaluate_constant_expression_2();
    void test_compare_cpp_version();
    void test_cpp_version_value();
    // group A: line continuation
    void test_combine_backslash_trailing_space();
    void test_combine_backslash_multiple();
    void test_combine_backslash_last_line();
    void test_combine_backslash_in_block_comment();
    void test_backslash_in_directive_name();
    void test_backslash_in_string();
    void test_backslash_result_line_alignment();
    // group B: strings & characters
    void test_comment_markers_inside_string();
    void test_escaped_quote_in_string();
    void test_char_literals();
    void test_quote_inside_comment();
    void test_raw_string_basic();
    void test_raw_string_delimiter();
    void test_raw_string_with_comment_markers();
    void test_raw_string_multiline();
    void test_raw_string_after_identifier();
    void test_macro_not_expanded_in_string();
    void test_raw_string_pipeline_canary();
    void test_raw_string_macro_not_expanded();
    // group C: macro expansion edges
    void test_stringize_with_spaces();
    void test_stringize_escaping();
    void test_token_paste_edges();
    void test_token_paste_chain();
    void test_named_args_with_varargs();
    void test_args_comma_in_parens();
    void test_args_comma_in_template_angle();
    void test_zero_param_macro();
    void test_empty_args();
    void test_empty_varargs();
    void test_chain_rescan_object_to_function();
    void test_mutual_recursion_function_macros();
    void test_redefine_overwrites();
    void test_empty_body_macro();
    void test_macro_word_boundaries();
    // group D: conditional directives
    void test_if_undefined_identifier();
    void test_nested_branches();
    void test_define_in_false_branch();
    void test_elif_chain();
    void test_multiline_macro_call_in_true_branch();
    void test_multiline_macro_call_in_false_branch();
    void test_unbalanced_directives();
    void test_and_or_keywords();
    void test_char_constant_in_if();
    void test_elifdef_elifndef_supported();
    void test_elifdef_elifndef_unsupported();
    void test_elifdef_cpp23_detection();
    void test_conditional1_resource();
    // group E: directives (single-file)
    void test_undef_local();
    void test_leading_space_directive();
private:
    static QStringList filterIncludes(const QStringList& text);
    // Run the full preprocess pipeline over in-memory content
    // (line splicing + comment removal + directive handling + macro expansion).
    static QStringList preprocessContent(CppPreprocessor& p, CppPreprocessor::Session &session,
                                         const QStringList& lines);
};

#endif
