#ifndef TEST_QSYNEDIT_DISPLAY_H
#define TEST_QSYNEDIT_DISPLAY_H
#include "test_qsynedit_base.h"

namespace QSynedit{

class TestQSyneditDisplay : public TestQSyneditBase
{
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void test_line_row_roundtrip();
    void test_line_row_roundtrip_with_fold();
    void test_display_buffer_roundtrip();
    void test_display_buffer_roundtrip_with_fold();
    void test_page_up_down();
    void test_page_top_bottom();
    void test_file_start_end();
    void test_scroll_top_pos();
    void test_scrollbar_drag_is_pixel_smooth();
    void test_ensure_caret_visible();
    void test_mouse_positioning();
    void test_mouse_positioning_with_fold();
    void test_mouse_press_moves_caret();
    void test_mouse_press_moves_caret_wrap();
    void test_mouse_press_moves_caret_fold();
    void test_mouse_double_click_selects_word();
    void test_mouse_alt_press_column_mode();
    void test_gutter_click_toggles_fold();
    void test_gutter_fold_button_paints();
    void test_wrap_selection_paints_on_start_segment();
    void test_dnd_drop_path();
    void test_wrap_mapping_reset_keeps_anchor();
    void test_paging_wrap_before_ready();
    void test_wordwrap_basics();
    void test_wordwrap_caret_display();
    void test_wordwrap_vertical_movement();
    void test_wordwrap_mouse();
    void test_wordwrap_fold();
    void test_wordwrap_batch_stale_rejection();
    void test_wordwrap_segment_coord_conversions();
    void test_advance_retreat_line_segment();
    void test_advance_retreat_with_fold();
    void test_toppos_anchor_sync();
    void test_wrap_toggle_preserves_anchor();
    void test_edit_shifts_anchor_marker_style();
    void test_setline_clamps_anchor_segment();
    void test_collapse_adsorbs_anchor();
    void test_wrap_batch_recomputes_toppos();
    void test_set_top_line_segment();
    void test_partial_fold_rescan();
    void test_fold_shift_maintenance();
    void test_wrap_job_early_abort();
    void test_horizontal_scroll_painting();
    void test_wordwrap_paints_all_segments();
};

}
#endif // TEST_QSYNEDIT_DISPLAY_H
