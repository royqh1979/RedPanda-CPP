#ifndef TEST_CPPDECLARATIONPARSER_H
#define TEST_CPPDECLARATIONPARSER_H

#include "test_cppparsercomponenttestcase.h"

class TestCppDeclarationParser : public CppParserComponentTestCase
{
    Q_OBJECT
private slots:
    void test_template_parameter_alpha_encoding();
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
    void test_parse_function_type_template_arguments();
    void test_parse_parameter_pack();
    void test_parse_c_complex_declarations();
    void test_parse_operator_declaration();
    void test_parse_member_pointer_declaration();
    void test_parse_function_template();
    void test_parse_template_parameter_normalization();
    void test_parse_template_parameter_kinds();
    void test_parse_template_specializations();
    void test_syntax_class_declarations();
    void test_class_base_list_nested_commas();
    void test_syntax_template_declarations();
    void test_syntax_namespace_declarations();
    void test_syntax_enum_declarations();
    void test_syntax_using_declarations();
    void test_failed_declaration_restores_cursor();
    void test_non_declaration_keywords_are_rejected();
    void test_requires_clauses_are_consumed();
    void test_explicit_bool_declarations();
    void test_requires_before_member_declaration();
    void test_function_try_block_recovery();
    void test_constructor_declarations_with_specifiers();
    void test_noexcept_specifications_are_preserved();
};

#endif // TEST_CPPDECLARATIONPARSER_H
