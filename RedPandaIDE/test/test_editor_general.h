#ifndef TEST_EDITOR_GENERAL_H
#define TEST_EDITOR_GENERAL_H

#include "test_editor_base.h"

class TestEditorGeneral : public TestEditorBase
{
    Q_OBJECT
private slots:
    void test_syntax_check_target_editor();
};

#endif // TEST_EDITOR_GENERAL_H
