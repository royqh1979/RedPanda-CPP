#ifndef TEST_CPPPARSER_H
#define TEST_CPPPARSER_H

#include "test_cppparsertestcase.h"

class TestCppParser : public CppParserTestCase
{
    Q_OBJECT
private slots:
    void init() { init_parser(); }
    void test_parse_var();
    void test_global_variable_reference_scope();
    void test_include_visibility();
    void test_class_forward_declaration_merges_with_definition();
    void test_include_global_variable_reference();
    void test_include_inline_function_call();
    void test_local_extern_variable_uses_external_statement();
    void test_static_extern_variable_definitions();
    void test_static_extern_function_definitions();
    void test_extern_variable_declaration_definition_order();
    void test_extern_function_declaration_definition_order();
    void test_unresolved_member_function_reference();
    void test_nested_template_member_definition();
    void test_extern_template_class_declaration();
    void test_forward_declaration_prefers_definition_for_eval();
    void test_base_member_return_uses_declaration_bindings();
    void test_derived_member_hides_base_overloads();
    void test_member_overload_resolution();
    void test_conversion_overload_resolution();
    void test_trailing_return_member_template();
    void test_find_statement_of_overload_resolution();
    void test_find_statement_of_template_specialization();
    void test_expression_identifier_trace();
    void test_parse_vars();
    void test_struct();
    void test_resolve_members_of_class_access_scope();
    void test_deleted_function_statements();
    void test_preprocessor_statements();
    void test_scoped_preprocessor_statements();
    void test_preprocessor_condition_with_unknown_special_macros();
    void test_label_statements();
    void test_hard_define_statements();
    void test_predefined_func_variable();
    void test_structured_bindings();
    void test_find_statement_of_inline_namespace();
    void test_nested_namespace_scope_parentage();
    void test_variable_template_specialization_name();
    void test_member_alias_template_specialization();
    void test_unknown_non_type_default_preserves_alias_resolution();
    void test_using_declaration_function_overload_set();
    void test_overload_rejects_unrelated_class_specialization();
    void test_empty_template_argument_list();
    void test_const_template_parameter_substitution();
    void test_auto_reference_collapsing();
    void test_void_t_detection_failure();
    void test_standard_type_traits_require_cpp11();
    void test_member_declaration_definition_in_namespace();
    void test_overloaded_declaration_definition_merge();
    void test_statement_declaration_navigation_selection();
    void test_local_declarations_in_disjoint_blocks_remain_distinct();
    void test_namespace_index_inline_filter_and_invalidation();
    void test_namespace_index_does_not_promote_inline_descendants();
    void test_reopened_namespaces_share_unique_entity();
    void test_namespace_occurrence_invalidation_across_roots();
    void test_anonymous_namespaces_remain_distinct();
    void test_inline_namespace_occurrence_invalidation();
    void test_inline_namespace_overloads();
    void test_stop_parsing_clears_results_before_finish();
    void test_application_exit_stops_parsing_without_cleanup();
    void test_normal_destructor_releases_resources();
    void test_application_exit_destructor_abandons_resources();
    void test_parse_checks_root_and_changes();
    void test_context_file_info_controls_header_coverage();
    void test_queued_parse_skips_completed_root();
    void test_project_parse_checks_all_files();
    void test_parse_uses_content_versions();
    void test_lambda_function_argument_scope();
    void test_nested_lambda_function_argument_scope();
    void test_lambda_evaluation_reuses_parse_statement();
};


#endif // TEST_CPPPARSER_H
