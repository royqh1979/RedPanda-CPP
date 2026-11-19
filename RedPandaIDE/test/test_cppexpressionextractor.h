#ifndef TEST_CPPEXPRESSIONEXTRACTOR_H
#define TEST_CPPEXPRESSIONEXTRACTOR_H

#include "test_editor_base.h"

class TestCppExpressionExtractor : public TestEditorBase
{
    Q_OBJECT
private slots:
    void test_extract_data();
    void test_extract();
    void test_invalid_positions();
    void test_custom_syntaxer();
    void test_session();
    void test_extract_line_data();
    void test_extract_line();
    void test_display_input();
    void test_profile();
};

#endif // TEST_CPPEXPRESSIONEXTRACTOR_H
