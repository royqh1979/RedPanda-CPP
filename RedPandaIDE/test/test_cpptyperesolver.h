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
    void test_type_resolver_expands_base_aliases_and_specializations();
    void test_type_resolver_reference_collapsing_and_cycles();
    void test_type_resolver_selects_specializations_and_rebind();
    void test_type_resolver_ignores_recursive_nonmatching_specialization();
    void test_type_resolver_preserves_fixed_arguments_in_specialization_scope();
    void test_type_resolver_evaluates_static_const_member_default_argument();
    void test_type_resolver_evaluates_specialized_static_const_member_default_argument();
    void test_type_resolver_matches_nested_class_specialization_default_argument();
    void test_type_resolver_matches_injected_specialization_default_argument();
    void test_type_resolver_matches_function_specialization_packs();
    void test_type_resolver_selects_specializations_across_reopened_namespaces();
    void test_type_resolver_computes_bounded_traits();
    void test_type_operations_resolve_container_and_iterator_chain();
    void test_type_operations_resolve_standard_range_views();
    void test_inline_namespace_member_lookup();
    void test_reopened_namespace_lookup_checks_all_declarations();
    void test_namespace_using_lookup_prefers_declared_function();
    void test_reopened_nested_namespace_lookup_checks_all_branches();
    void test_namespace_lookup_transparency_and_scope_boundaries();
    void test_namespace_delete_does_not_reinsert_transparent_descendants();
    void test_qualified_lookup_respects_lexical_and_local_scopes();
    void test_inherited_member_enumeration_and_access();
    void test_const_member_with_nonmember_candidates();
    void test_type_resolver_evaluates_nothrow_invocable_for_functors();
    void test_type_resolver_matches_explicit_specialization_through_alias();
    void test_type_resolver_evaluates_constructible_traits();
    void test_type_resolver_evaluates_constructible_from_types();
    void test_type_resolver_evaluates_copy_constructible_through_reference_alias();
    void test_type_resolver_evaluates_copy_constructible_in_inline_namespace();
    void test_type_resolver_evaluates_move_and_default_constructible();
    void test_type_resolver_evaluates_invocable_r();
    void test_type_resolver_evaluates_invocable_r_with_inherited_call();
    void test_type_resolver_expands_non_type_pack_arguments();
    void test_type_resolver_evaluates_logical_trait_single_values();
    void test_type_resolver_resolves_alias_to_clang_type_builtin();
    void test_type_resolver_internal_member_interfaces();
    void test_type_resolver_internal_binding_interfaces();
    void test_type_resolver_ranks_partial_function_candidates();
    void test_type_resolver_internal_type_and_lookup_interfaces();
    void test_type_resolver_internal_constant_interfaces();
    void test_type_resolver_matches_alias_patterns_structurally();
};

#endif // TEST_CPPTYPERESOLVER_H
