#ifndef TEST_QSYNEDIT_EMOJI_H
#define TEST_QSYNEDIT_EMOJI_H
#include "test_qsynedit_base.h"

namespace QSynedit{

class TestQSyneditEmoji : public TestQSyneditBase
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup();
    void loadDemoFile();

    void test_move_caret_x_data();
    void test_move_caret_x();
    void test_move_caret_y_data();
    void test_move_caret_y();

    void test_input_chars_at_file_begin_end_overwrite_mode();
    void test_delete_chars_in_file();
    void test_delete_prev_chars_in_file();


};

}
#endif
