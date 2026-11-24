#ifndef TEST_CPPPARSERSTANDARDLIBRARY_H
#define TEST_CPPPARSERSTANDARDLIBRARY_H

#include "test_cppparsertestcase.h"

#ifdef REDPANDA_PARSER_STDLIB_TEST
class TestCppParserStandardLibrary : public CppParserTestCase
{
    Q_OBJECT
private slots:
    void init() { init_parser(); }
    void test_bits_stdcpp_not_rescanned_on_stable_reparse();
    void test_standard_library_abort_alias();
    void test_standard_library_string();
    void test_standard_library_header_context();
    void test_standard_library_nested_include_guard_visibility();
    void test_standard_library_direct_c_header_context();
    void test_standard_library_header_guard_context_visibility_data();
    void test_standard_library_header_guard_context_visibility();
    void test_standard_library_front_append();
    void test_standard_library_vector_subscript();
    void test_standard_library_make_shared_default();
    void test_standard_library_expression_chains();
    void test_standard_library_trace_edge_cases();
    void test_standard_library_container_eval_profile();
    void test_standard_library_algorithm_eval_profile();
    void test_standard_library_nothrow_invocable();
    void test_standard_library_libcxx_hash_requirements();
    void test_standard_library_libcxx_copy_constructible();
    void test_standard_library_libcxx_invocable();
    void test_standard_library_parse_baseline();
    void test_standard_library_public_api_complete();
};
#endif

#endif // TEST_CPPPARSERSTANDARDLIBRARY_H
