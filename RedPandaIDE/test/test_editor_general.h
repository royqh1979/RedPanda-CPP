#ifndef TEST_EDITOR_GENERAL_H
#define TEST_EDITOR_GENERAL_H

#include "test_editor_base.h"

class TestEditorGeneral : public TestEditorBase
{
    Q_OBJECT
private slots:
    void test_syntax_check_target_editor();
    void test_hard_define_statement_visibility_and_hint();
    void test_predefined_func_hint();
    void test_hide_stops_parsing_and_allows_reparse_after_show();
    void test_auto_backup_lifecycle();
    void test_auto_backup_disabled_and_read_only();
    void test_auto_backup_throttles_recent_changes();
};

#endif // TEST_EDITOR_GENERAL_H
