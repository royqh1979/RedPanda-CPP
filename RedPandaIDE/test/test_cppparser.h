#ifndef TEST_CPPPARSER_H
#define TEST_CPPPARSER_H
#include <QObject>
#include <memory>

class CppParser;

class TestCppParser: public QObject
{
    Q_OBJECT
public:
    TestCppParser(QObject *parent=nullptr);
private slots:
    void init();
    void init_parser();
    void test_parse_var();
    void test_global_variable_reference_scope();
    void test_unresolved_member_function_reference();
    void test_forward_declaration_prefers_definition_for_eval();
    void test_base_member_return_uses_declaration_bindings();
    void test_member_overload_resolution();
    void test_unknown_non_type_default_preserves_alias_resolution();
    void test_variable_template_specialization_name();
    void test_member_alias_template_specialization();
    void test_parse_vars();
    void test_struct();
    void test_preprocessor_statements();
    void test_label_statements();
    void test_hard_define_statements();
    void test_structured_bindings();
    void test_shared_boundary_statement_invalidation();
    void test_shared_boundary_statement_invalidation_only();
    void test_invalidation_preserves_earlier_boundary_record();
    void test_removed_boundary_include_remains_inactive_cached_record();
    void test_repeated_template_include_preserves_each_context();
    void test_shared_template_statement_ownership();
    void test_statement_ownership_declaration_definition_invalidation();
    void test_boundary_invalidation_removes_defines_and_statements();
    void test_replay_reuses_existing_statements();
    void test_system_subtree_inheritance_uses_boundary_record();
    void test_user_header_change_invalidates_dependent_roots();
    void test_shared_internal_header_change_invalidates_boundary_ancestors();
    void test_root_switch_with_macro_change_removes_stale_statements();
    void test_root_switch_with_include_order_change_removes_stale_statements();
    void test_internal_header_disk_change_removes_stale_statements();
    void test_bits_stdcpp_not_rescanned_when_switching_roots();
    void test_reset_parser_clears_registry_and_statements();
    void test_auto_invalidation_preserves_definition_after_declaration_change();
    void test_cppreference_declarator_syntax();
    void test_cppreference_function_syntax();
    void test_cppreference_class_syntax();
    void test_cppreference_template_syntax();
    void test_cppreference_enum_syntax();
    void test_cppreference_namespace_syntax();
    void test_cppreference_lambda_syntax();
    void test_cppreference_structured_binding_syntax();
    void test_cppreference_range_for_syntax();
    void test_cppreference_ranges_syntax();
    void test_member_declaration_definition_in_namespace();
    void test_overloaded_declaration_definition_merge();
    void test_local_declarations_in_disjoint_blocks_remain_distinct();
    void test_namespace_index_inline_filter_and_invalidation();
    void test_namespace_index_does_not_promote_inline_descendants();
    void test_reopened_namespaces_share_unique_entity();
    void test_namespace_occurrence_invalidation_across_roots();
    void test_anonymous_namespaces_remain_distinct();
    void test_inline_namespace_occurrence_invalidation();
    void test_inline_namespace_overloads();
    void test_find_statement_of_inline_namespace();
    void test_nested_namespace_scope_parentage();
#ifdef REDPANDA_PARSER_STDLIB_TEST
    void test_bits_stdcpp_not_rescanned_on_stable_reparse();
    void test_standard_library_string();
    void test_standard_library_front_append();
    void test_standard_library_vector_subscript();
    void test_standard_library_make_shared_default();
    void test_standard_library_expression_chains();
    void test_standard_library_container_eval_profile();
    void test_standard_library_parse_baseline();
    void test_standard_library_public_api_complete();
#endif
protected:
    std::shared_ptr<CppParser> mParser;
};

#endif
