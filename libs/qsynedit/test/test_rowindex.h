#ifndef TEST_ROWINDEX_H
#define TEST_ROWINDEX_H
#include <QObject>

namespace QSynedit {
class TestRowIndex : public QObject
{
    Q_OBJECT

private slots:
    void test_empty();
    void test_reset_and_counts();
    void test_insert_lines();
    void test_erase_lines();
    void test_set_seg_count();
    void test_hidden_basic();
    void test_hidden_overlapping_ranges();
    void test_line_seg_at_visible_row();
    void test_next_prev_visible();
    void test_random_against_reference();
};
}
#endif
