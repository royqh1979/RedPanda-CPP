#ifndef TEST_CPPTYPERESOLVER_H
#define TEST_CPPTYPERESOLVER_H

#include "test_cppparsercomponenttestcase.h"

class TestCppTypeResolver : public CppParserComponentTestCase
{
    Q_OBJECT
private slots:
    void test_type_resolver_binds_custom_template_members();
    void test_type_resolver_applies_default_template_arguments();
    void test_type_resolver_namespace_defaults_do_not_capture_caller_bindings();
    void test_member_lookup_cache_ignores_model_changes();
    void test_type_resolution_cache_respects_bindings_and_switches();
    void test_type_resolver_resolves_multisegment_aliases();
    void test_type_resolver_binds_dependent_base_members();
    void test_type_resolver_reference_collapsing_and_cycles();
    void test_type_resolver_selects_specializations_and_rebind();
    void test_type_resolver_preserves_fixed_arguments_in_specialization_scope();
    void test_type_resolver_evaluates_static_const_member_default_argument();
    void test_type_resolver_evaluates_specialized_static_const_member_default_argument();
    void test_type_resolver_matches_nested_class_specialization_default_argument();
    void test_type_resolver_matches_injected_specialization_default_argument();
    void test_type_resolver_selects_specializations_across_reopened_namespaces();
    void test_type_resolver_computes_bounded_traits();
    void test_type_operations_resolve_container_and_iterator_chain();
    void test_type_operations_resolve_standard_range_views();
    void test_inline_namespace_member_lookup();
    void test_reopened_namespace_lookup_checks_all_declarations();
    void test_reopened_nested_namespace_lookup_checks_all_branches();
    void test_namespace_lookup_transparency_and_scope_boundaries();
    void test_namespace_delete_does_not_reinsert_transparent_descendants();
    void test_qualified_lookup_respects_lexical_and_local_scopes();
    void test_inherited_member_enumeration_and_access();
};

#endif // TEST_CPPTYPERESOLVER_H
