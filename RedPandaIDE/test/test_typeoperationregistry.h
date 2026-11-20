#ifndef TEST_TYPEOPERATIONREGISTRY_H
#define TEST_TYPEOPERATIONREGISTRY_H

#include "test_cppparsercomponenttestcase.h"

class TestTypeOperationRegistry : public CppParserComponentTestCase
{
    Q_OBJECT
private slots:
    void test_container_iterator_and_subscript_operations();
    void test_callable_tuple_and_view_operations();
};

#endif // TEST_TYPEOPERATIONREGISTRY_H
