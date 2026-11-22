#ifndef TEST_QSYNEDIT_DISPLAY_H
#define TEST_QSYNEDIT_DISPLAY_H
#include "test_qsynedit_base.h"

namespace QSynedit{

class TestQSyneditDisplay : public TestQSyneditBase
{
    Q_OBJECT
private:
    void drainLayoutJobs(QSynEdit *editor = nullptr);
    void expectRowMapping(QSynEdit *editor = nullptr);

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
    void test_mouse_positioning_shows_caret_immediately();
    void test_mouse_press_moves_caret_wrap();
    void test_mouse_press_moves_caret_fold();
    void test_mouse_double_click_selects_word();
    void test_mouse_double_click_at_token_boundary_data();
    void test_mouse_double_click_at_token_boundary();
    void test_mouse_alt_press_column_mode();
    void test_gutter_click_toggles_fold();
    void test_gutter_fold_button_paints();
    void test_fold_placeholder_painting();
    void test_fold_placeholder_navigation();
    void test_fold_end_line_suffix_visibility();
    void test_fold_else_same_line();
    void test_fold_edit_inside_expands_and_undo();
    void test_fold_virtual_close_backspace();
    void test_fold_edit_outside_and_suffix_transitions();
    void test_fold_edit_shared_document_views();
    void test_fold_merge_before_open_does_not_expand();
    void test_fold_whole_block_delete_exception();
    void test_fold_bulk_delete_matches_full_rescan();
    void test_gutter_click_selects_line();
    void test_gutter_mark_click_emits_signal();
    void test_gutter_shift_click_selects_line_range();
    void test_gutter_shift_click_requires_full_line_selection();
    void test_wrap_selection_paints_on_start_segment();
    void test_drag_start_requires_distance_and_time();
    void test_dnd_drop_path();
    void test_dnd_undo_restores_content_and_folds();
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
    void test_batch_top_restore();
    void test_batch_ensure_caret_visible_flush();
    void test_batch_ensure_caret_last_request_wins();
    void test_nested_batch_top_restore();
    void test_no_change_batch_top_scrollbar_sync();
    void test_batch_top_intent_preserves_command_order();
    void test_shared_document_keeps_top_anchor();
    void test_shared_document_full_replace_preserves_view_state();
    void test_partial_fold_rescan();
    void test_public_edit_fold_rescan();
    void test_fold_shift_maintenance();
    void test_wrap_job_early_abort();
    void test_wrap_job_only_computes_pending_lines();
    void test_wrap_missing_seqs_incremental_tracking();
    void test_wrap_schedule_without_missing_lines();
    void test_single_line_edit_indexes_without_job();
#ifdef QSYNEDIT_PROFILE_TESTS
    void test_miniaudio_single_line_edit_profile();
    void test_miniaudio_edit_profile_data();
    void test_miniaudio_edit_profile();
    void test_miniaudio_view_overhead_profile();
#endif
    void test_multi_line_edit_indexes();
    void test_range_move_view_and_undo();
    void test_multi_line_setline_background_layout();
    void test_max_width_notifications();
    void test_shared_document_edit_indexes();
    void test_layout_with_zero_wrap_width();
    void test_enter_first_line_keeps_top();
    void test_enter_first_line_repaints_old_first_line();
    void test_horizontal_scroll_painting();
    void test_wordwrap_paints_all_segments();
    void test_wordwrap_boundary_token_painting();
    void test_batched_highlight_preparation();
};

}
#endif // TEST_QSYNEDIT_DISPLAY_H
