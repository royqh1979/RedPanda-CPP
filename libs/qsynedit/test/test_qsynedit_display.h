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
    void test_ensure_caret_visible();
    void test_mouse_positioning();
    void test_mouse_positioning_with_fold();
    void test_wordwrap_basics();
    void test_wordwrap_caret_display();
    void test_wordwrap_vertical_movement();
    void test_wordwrap_mouse();
    void test_wordwrap_fold();
    void test_wordwrap_batch_stale_rejection();
    void test_wordwrap_segment_coord_conversions();
    void test_advance_retreat_line_segment();
    void test_advance_retreat_with_fold();
    void test_advance_anchor_row();
};

}
#endif // TEST_QSYNEDIT_DISPLAY_H
