#ifndef TEST_QSYNEDIT_CPP_H
#define TEST_QSYNEDIT_CPP_H
#include "test_qsynedit_base.h"

namespace QSynedit{

class TestQSyneditCpp : public TestQSyneditBase
{
    Q_OBJECT
private:
    EditorOptions mDefaultOptions;
private slots:
    void initTestCase();
    void cleanup();
    void loadDemoFile();

    void test_next_char();
    void test_get_token_data();
    void test_get_token();
    void test_token_begin_data();
    void test_token_begin();
    void test_token_end_data();
    void test_token_end();
    void test_previous_word_begin_data();
    void test_previous_word_begin();
    void test_next_word_begin_data();
    void test_next_word_begin();
    void test_next_word_end_data();
    void test_next_word_end();
    void test_prev_word_end_data();
    void test_prev_word_end();

    void test_move_caret_x_data();
    void test_move_caret_x();
    void test_move_caret_y_data();
    void test_move_caret_y();
    void test_move_caret_to_line_begin_data();
    void test_move_caret_to_line_begin();
    void test_move_caret_to_line_end_data();
    void test_move_caret_to_line_end();

    void test_match_brackets_data();
    void test_match_brackets();

    void test_get_block_start_end_data();
    void test_get_block_start_end();

    void test_select_data();
    void test_select();

    void test_clear();
    void test_load_file();
    void test_clear_all();
    void test_set_content_qstring();
    void test_set_content_qstringlist();

    void test_input_chars_in_empty_file();
    void test_input_chars_in_empty_file_overwrite_mode();
    void test_input_char_when_select_all();
    void test_input_chars_at_file_begin_end();
    void test_input_chars_at_file_begin_end_overwrite_mode();
    void test_input_chars_in_file();
    void test_input_char_at_end_of_first_line_of_collapsed_block();
    void test_input_string_in_empty_file();
    void test_input_string_in_overwrite_mode();
    void test_input_input_chars_undo();

    void test_input_tab_in_empty_file();
    void test_block_indent();
    void test_block_unindent();

    void test_replace_line_in_empty_file();
    void test_replace_line_at_file_begin_end();

    void test_delete_chars_in_empty_file();
    void test_delete_chars_in_file();
    void test_delete_chars_at_file_begin_end();
    void test_delete_chars_undo();
    void test_delete_chars_undo2();
    void test_merge_with_next_line_with_collapsed_block();
    void test_merge_with_next_line_with_collapsed_block2();
    void test_merge_with_next_line_with_collapsed_block3();

    void test_delete_prev_chars_in_empty_file();
    void test_delete_prev_chars_at_file_begin_end();
    void test_delete_prev_chars_undo();
    void test_merge_with_prev_line_with_collapsed_block();
    void test_merge_with_prev_line_with_collapsed_block2();
    void test_merge_with_prev_line_with_collapsed_block3();

    void test_delete_and_backspace_chars_undo();

    void test_break_line_in_empty_file();
    void test_break_lines();
    void test_break_lines_with_collapsed_block();
    void test_break_lines_with_collapsed_block2();
    void test_break_lines_with_collapsed_block3();
    void test_if_else_indent();

    void test_delete_current_line_in_empty_file();
    void test_delete_current_line();

    void test_duplicate_current_line_in_empty_file();
    void test_duplicate_current_line();
    void test_duplicate_selection();

    void test_delete_selection_in_empty_file();
    void test_delete_while_select_all();
    void test_delete_selection_in_line();
    void test_backspace_selection_in_empty_file();
    void test_backspace_while_select_all();
    void test_input_char_while_select_all();

    void test_toggle_comment_in_empty_file();
    void test_toggle_comment_at_file_begin();
    void test_toggle_comment_at_file_end();
    void test_toggle_comment_select_line_end_to_line_begin();
    void test_toggle_comment_select_line_begin_to_line_end();
    void test_toggle_comment_select_line_begin_to_line_begin();
    void test_toggle_comment_select_line_end_to_line_end();
    void test_toggle_comment_select_multiple_lines();

    void test_move_up_selection_in_empty_file();
    void test_move_up_current_line_at_file_begin();
    void test_move_up_current_line_at_file_end();
    void test_move_up_select_line_end_to_line_begin();
    void test_move_up_select_line_begin_to_line_end();
    void test_move_up_select_line_begin_to_line_begin();
    void test_move_up_select_line_end_to_line_end();
    void test_move_up_after_collapsed_block();
    void test_move_down_select_line_end_to_line_begin();
    void test_move_down_select_line_end_to_line_begin2();
    void test_move_down_before_collapsed_block();

    void test_copy_paste_rawstring();
    void test_cut_selection();
    void test_cut_current_line_without_selection();
    void test_paste_replaces_selection();
    void test_grouped_editing_undo_redo();
    void test_nested_grouped_editing();
    void test_grouped_editing_keeps_selection_mode();
    void test_column_select_content();
    void test_column_input_char();
    void test_column_input_at_column_cursor();
    void test_column_backspace();
    void test_column_cut();
    void test_column_mode_alt_selection();
    void test_column_delete_char();
    void test_column_overwrite_input();
    void test_column_overwrite_at_column_cursor();
    void test_column_overwrite_at_eol();
    void test_column_select_content_cjk();
    void test_column_delete_cjk();
    void test_column_delete_tab_snap();
    void test_column_has_content_cjk_layout();
    void test_column_delete_prop_font();
    void test_column_paste_single_line();
    void test_column_paste_multi_line();
    void test_column_paste_multi_line_unequal();
    void test_column_paste_short_line();
    void test_column_paste_at_column_cursor();
    void test_overwrite_input_with_selection();
    void test_no_auto_indent_break_line();
    void test_copy_paste_ending_with_backslash();
    void test_setseltext_and_indent();
    void test_setseltext_and_indent2();
    void test_setseltext_and_indent3_line_comment();

    void test_auto_indent_for_parenthesis();
    void test_auto_indent_for_brace_after_closing_parenthesis();
    void test_auto_indent_for_if_else();
    void test_edit_operations_are_batched();
    void test_auto_indent_for_if_else_2();
    void test_auto_indent_for_if_else_3();

    void test_shared_document_basics();
    void test_shared_document_fold_shift();
    void test_shared_document_syntaxer();
    void test_shared_document_caret_shift();
    void test_shared_document_close_first_view();
    void test_shared_document_modified_sync();
};

}
#endif
