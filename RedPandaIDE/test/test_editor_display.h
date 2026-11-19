#ifndef TEST_EDITOR_DISPLAY_H
#define TEST_EDITOR_DISPLAY_H
#include "test_editor_base.h"

// Editor-level display tests (gutter fold marks, etc.)
class TestEditorDisplay : public TestEditorBase
{
    Q_OBJECT
private slots:
    void test_fold_marks_paint();
    void test_reformat_restores_view_state();
    void test_dnd_undo_restores_caret();
    void test_stl_paint_profile();
};

#endif // TEST_EDITOR_DISPLAY_H
