#ifndef TEST_CPPTOKENIZER_H
#define TEST_CPPTOKENIZER_H

#include <QObject>
#include <QStringList>
#include "src/parser/cpptokenizer.h"
#include "src/parser/cpptokencursor.h"
#include "src/parser/cpptypes.h"
#include "src/parser/cppdeclarationparser.h"
#include "src/parser/cppsyntaxparser.h"
#include "src/parser/cppstatementbuilder.h"
#include "src/parser/cpptyperesolver.h"
#include "src/parser/typeoperationregistry.h"
#include "src/parser/cppexpressionevaluator.h"
#include "src/parser/cppscopes.h"
#include "src/parser/parserutils.h"

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
    void test_syntax_class_declarations();
    void test_syntax_template_declarations();
    void test_syntax_namespace_declarations();
    void test_syntax_enum_declarations();
    void test_syntax_using_declarations();
    void test_scope_range_queries();
    void test_scope_queries_same_line();
    void test_declaration_source_ranges();
    void test_statement_builder_expands_declarators_and_types();
    void test_statement_builder_preserves_function_and_access_properties();
    void test_statement_builder_preserves_scopes_positions_and_ancestors();
    void test_statement_builder_assigns_out_of_line_special_members_to_class();
    void test_statement_builder_merges_forward_class_with_definition();
    void test_statement_model_owns_statements_and_weak_indexes();
    void test_statement_builder_preserves_array_suffixes();
    void test_statement_builder_registers_nested_body_scopes();
    void test_statement_builder_scope_uses_recovered_file_end();
    void test_statement_builder_distinguishes_class_body_from_variable();
    void test_type_resolver_binds_custom_template_members();
    void test_type_resolver_applies_default_template_arguments();
    void test_type_resolver_namespace_defaults_do_not_capture_caller_bindings();
    void test_member_lookup_cache_ignores_model_changes();
    void test_type_resolver_resolves_multisegment_aliases();
    void test_type_resolver_binds_dependent_base_members();
    void test_type_resolver_reference_collapsing_and_cycles();
    void test_type_resolver_selects_specializations_and_rebind();
    void test_type_resolver_selects_specializations_across_reopened_namespaces();
    void test_type_resolver_computes_bounded_traits();
    void test_function_template_explicit_arguments();
    void test_type_operations_resolve_container_and_iterator_chain();
    void test_expression_evaluator_follows_container_member_chain();
    void test_function_scopes_auto_loops_catch_and_using();
    void test_type_operations_resolve_standard_range_views();
    void test_expression_evaluator_builds_range_adaptor_pipelines();
    void test_lambda_inference_preserves_source_position();
    void test_function_template_argument_deduction_for_algorithms();
    void test_normalized_template_function_signatures();
    void test_inline_namespace_member_lookup();
    void test_nested_namespace_declaration();
    void test_reopened_namespace_lookup_checks_all_declarations();
    void test_reopened_nested_namespace_lookup_checks_all_branches();
    void test_namespace_lookup_transparency_and_scope_boundaries();
    void test_namespace_delete_does_not_reinsert_transparent_descendants();
    void test_qualified_lookup_respects_lexical_and_local_scopes();
    void test_template_forward_declaration_with_default_arguments();
    void test_scope_recovery_preserves_following_declarations();
    void test_failed_declaration_restores_cursor();
    void test_non_declaration_keywords_are_rejected();
    void test_requires_clauses_are_consumed();
    void test_constructor_declarations_with_specifiers();
    void test_integer_constant_expression_invalid_division();
    void test_deduction_guides_are_consumed_without_statements();
    void test_inherited_member_enumeration_and_access();
    void test_expression_evaluator_preserves_constructor_statement();
    void test_expression_evaluator_forbids_model_changes_outside_parse();

private:
    void tokenize(const QStringList &lines);
    bool parseDeclarationText(const QString &text, DeclarationInfo &result);
    bool parseSyntaxText(const QString &text, QVector<DeclarationInfo> &result);
    bool parseSyntaxFile(const QString &fileName, QVector<DeclarationInfo> &result);
    QStringList tokenTexts() const;

    CppTokenizer mTokenizer;
};

#endif // TEST_CPPTOKENIZER_H
