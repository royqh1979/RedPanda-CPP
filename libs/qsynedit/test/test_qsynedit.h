#include <QtTest>
#include <QCoreApplication>
#include "qsynedit/qsynedit.h"

namespace QSynedit{

class TestQSyneditCpp : public QObject
{
    Q_OBJECT
private:
    std::shared_ptr<QSynEdit> mEdit;
    QList<int> mDeleteStartLines;
    QList<int> mDeleteLineCounts;
    QList<int> mInsertStartLines;
    QList<int> mInsertLineCounts;
    QList<int> mLineMovedFroms;
    QList<int> mLineMovedTos;
    QList<StatusChanges> mStatusChanges;
    QList<int> mReparseStarts;
    QList<int> mReparseCounts;
private slots:
    void onLinesDeleted(int line, int count);
    void onLinesInserted(int line, int count);
    void onLineMoved(int from, int to);
    void onStatusChanged(StatusChanges change);
    void onReparsed(int start, int count);

    void initTestCase();
    void clearReparseDatas();
    void clearSignalDatas();
    void loadDemoFile();
    void clearContent();

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

    void test_select_data();
    void test_select();

    void test_clear();
    void test_load_file();
    void test_clear_all();
    void test_set_content_qstring();
    void test_set_content_qstringlist();

    void test_input_chars_in_empty_file();
    void test_input_chars_at_file_begin_end();
    void test_input_chars_in_file();
    void test_input_char_at_end_of_first_line_of_collapsed_block();

    void test_delete_chars_in_empty_file();
    void test_delete_chars_in_file();
    void test_delete_chars_at_file_begin_end();
    void test_merge_with_next_line_with_collapsed_block();
    void test_merge_with_next_line_with_collapsed_block2();
    void test_merge_with_next_line_with_collapsed_block3();

    void test_delete_prev_chars_in_empty_file();
    void test_delete_prev_chars_at_file_begin_end();
    void test_merge_with_prev_line_with_collapsed_block();
    void test_merge_with_prev_line_with_collapsed_block2();
    void test_merge_with_prev_line_with_collapsed_block3();

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

    void test_delete_selection_in_empty_file();
    void test_delete_while_select_all();
    void test_delete_selection_in_line();
    void test_backspace_selection_in_empty_file();
    void test_backspace_while_select_all();
    void test_input_char_while_select_all();

    void test_toggle_comment_in_empty_file();
    void test_toggle_comment_at_file_begin();
    void test_toggle_comment_at_file_end();
/*
    void test_delete_text_normal_data();
    void test_delete_text_normal();
    void test_copy_paste_data();
    void test_copy_paste();
    void test_enter_chars_data();
    void test_enter_chars();
    void test_delete_char_data();
    void test_delete_char();
    void test_backspace_data();
    void test_backspace();
    void test_break_line_data();
    void test_break_line();
    void test_break_line_at_end_data();
    void test_break_line_at_end();
    void test_delete_current_line_data();
    void test_delete_current_line();
    void test_duplicate_current_line_data();
    void test_duplicate_current_line();
    */

};

}
