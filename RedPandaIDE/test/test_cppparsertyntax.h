#ifndef TEST_CPPPARSERTYNTAX_H
#define TEST_CPPPARSERTYNTAX_H

#include "test_cppparsertestcase.h"

class TestCppParserSyntax : public CppParserTestCase
{
    Q_OBJECT
private slots:
    void init() { init_parser(); }
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
};


#endif // TEST_CPPPARSERTYNTAX_H
