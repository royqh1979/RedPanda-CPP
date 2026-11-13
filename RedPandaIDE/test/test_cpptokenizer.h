#ifndef TEST_CPPTOKENIZER_H
#define TEST_CPPTOKENIZER_H

#include <QObject>
#include <QStringList>
#include "src/parser/cpptokenizer.h"
#include "src/parser/cpptokencursor.h"
#include "src/parser/cpptypes.h"
#include "src/parser/cppdeclarationparser.h"

class TestCppTokenizer : public QObject
{
    Q_OBJECT
public:
    explicit TestCppTokenizer(QObject *parent = nullptr);

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
    void test_line_numbers();
    void test_cursor_advance_and_checkpoint();
    void test_greater_token_split_and_rollback();
    void test_same_file_match_skip();
    void test_unmatched_match_skip_to_file_end();
    void test_file_end_token();
    void test_include_end_match_skip();
    void test_cross_file_match_skip();
    void test_invalid_match_skip();
    void test_type_template_arguments();
    void test_array_declarator_order();
    void test_function_pointer_declarator();
    void test_function_returning_function_pointer();
    void test_member_function_qualifiers();
    void test_parameter_rendering();
    void test_decl_specifier_flags();
    void test_parse_simple_declaration();
    void test_parse_multiple_declarators();
    void test_parse_function_pointer_declaration();
    void test_parse_complex_parameter_declaration();
    void test_parse_trailing_return_declaration();
    void test_parse_member_function_declarators();
    void test_parse_direct_initialization_not_function();
    void test_parse_template_type_arguments();
    void test_parse_operator_declaration();
    void test_parse_member_pointer_declaration();
    void test_parse_function_template();
    void test_parse_template_parameter_kinds();
    void test_parse_template_specializations();

private:
    void tokenize(const QStringList &lines);
    bool parseDeclarationText(const QString &text, DeclarationInfo &result);
    QStringList tokenTexts() const;

    CppTokenizer mTokenizer;
};

#endif // TEST_CPPTOKENIZER_H
