#ifndef TEST_CPPEXPRESSIONEVALUATOR_H
#define TEST_CPPEXPRESSIONEVALUATOR_H

#include "test_cppparsercomponenttestcase.h"

class TestCppExpressionEvaluator : public CppParserComponentTestCase
{
    Q_OBJECT
private slots:
    void test_function_template_explicit_arguments();
    void test_expression_evaluator_follows_container_member_chain();
    void test_expression_evaluator_builds_range_adaptor_pipelines();
    void test_function_template_argument_deduction_for_algorithms();
    void test_expression_evaluator_preserves_constructor_statement();
    void test_expression_evaluator_forbids_model_changes_outside_parse();
    void test_declval_preserves_type_and_outer_template_bindings();
    void test_declval_with_direct_decltype_return();
    void test_noexcept_evaluates_selected_calls();
    void test_evaluator_internal_lookup_and_results();
    void test_evaluator_internal_token_parsing();
    void test_evaluator_internal_evaluation_stages();
    void test_evaluator_internal_type_operations();
};

#endif // TEST_CPPEXPRESSIONEVALUATOR_H
