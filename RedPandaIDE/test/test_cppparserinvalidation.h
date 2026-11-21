#ifndef TEST_CPPPARSERINVALIDATION_H
#define TEST_CPPPARSERINVALIDATION_H

#include "test_cppparsertestcase.h"

class TestCppParserInvalidation : public CppParserTestCase
{
    Q_OBJECT
private slots:
    void init() { init_parser(); }
    void test_shared_boundary_statement_invalidation();
    void test_shared_boundary_statement_invalidation_only();
    void test_invalidation_preserves_earlier_boundary_record();
    void test_removed_boundary_include_remains_inactive_cached_record();
    void test_repeated_template_include_preserves_each_context();
    void test_shared_template_statement_ownership();
    void test_statement_ownership_declaration_definition_invalidation();
    void test_extern_declaration_invalidation_recomputes_variable_properties();
    void test_static_function_definition_invalidation_does_not_create_extern();
    void test_last_variable_occurrence_invalidation_removes_statement();
    void test_boundary_invalidation_removes_defines_and_statements();
    void test_replay_reuses_existing_statements();
    void test_system_subtree_inheritance_uses_boundary_record();
    void test_file_info_for_boundary_children();
    void test_cross_boundary_declaration_definition_file_info();
    void test_user_header_change_invalidates_dependent_roots();
    void test_shared_internal_header_change_invalidates_boundary_ancestors();
    void test_root_switch_with_macro_change_removes_stale_statements();
    void test_root_switch_with_include_order_change_removes_stale_statements();
    void test_internal_header_disk_change_removes_stale_statements();
    void test_bits_stdcpp_not_rescanned_when_switching_roots();
    void test_reset_parser_clears_registry_and_statements();
    void test_auto_invalidation_preserves_definition_after_declaration_change();
};


#endif // TEST_CPPPARSERINVALIDATION_H
