#ifndef TEST_CPPSTATEMENTBUILDER_H
#define TEST_CPPSTATEMENTBUILDER_H

#include "test_cppparsercomponenttestcase.h"

class TestCppStatementBuilder : public CppParserComponentTestCase
{
    Q_OBJECT
private slots:
    void test_statement_factory_initializes_statements();
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
    void test_function_scopes_auto_loops_catch_and_using();
    void test_lambda_inference_preserves_source_position();
    void test_normalized_template_function_signatures();
    void test_nested_namespace_declaration();
    void test_template_forward_declaration_with_default_arguments();
    void test_scope_recovery_preserves_following_declarations();
    void test_integer_constant_expression_invalid_division();
    void test_deduction_guides_are_consumed_without_statements();
    void test_unscoped_enumerators_have_outer_and_enum_scope();
};

#endif // TEST_CPPSTATEMENTBUILDER_H
