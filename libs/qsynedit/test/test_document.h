#ifndef TEST_DOCUMENT_H
#define TEST_DOCUMENT_H
#include <QTemporaryDir>
namespace QSynedit {
class TestDocumentHelpers : public QObject
{
    Q_OBJECT
private slots:
    void test_calcSegmentInterval();
    void test_segmentIntervalStart();
    void test_searchForSegmentIdx();
    void test_calcGlyphStartCharList();
};

class TestDocumentLine : public QObject
{
    Q_OBJECT
private slots:

};

class Document;
class TestDocument : public QObject
{
    Q_OBJECT

public:
    TestDocument();
    ~TestDocument();
private:
    QString writeFixture(const QString& name, const QByteArray& data);
    QString fixturePath(const QString& name) const;
private:
    int mChangedCount;
    std::shared_ptr<Document> mDoc;
    QTemporaryDir mTempDir;
private slots:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();
    void test_load_from_file();
    void test_load_from_file2();
    void test_load_from_empty_file();
    void test_load_utf8_bom();
    void test_load_utf16_bom();
    void test_load_utf32_bom();
    void test_load_newline_unix();
    void test_load_no_trailing_newline();
    void test_load_mixed_newlines();
    void test_load_nonexistent_file();
    void test_load_binary_file();
    void test_load_with_utf8_encoding();
    void test_load_with_gbk_encoding();
    void test_save_utf8_roundtrip();
    void test_save_utf8_bom();
    void test_save_utf16_bom();
    void test_save_utf16_no_bom();
    void test_save_utf32_bom();
    void test_save_utf32_no_bom();
    void test_save_gbk();
    void test_save_all_ascii_reports_ascii();
    void test_save_newline_windows();
    void test_save_newline_unix();
    void test_emoji_glyphs();
    void test_set_text();
    void test_set_empty_text();
    void test_set_contents();
    void test_set_empty_contents();
    void test_add_line();
    void test_add_lines();

    void test_search_segment_index();
    void test_insert_line();
    void test_insert_line1();
    void test_insert_line2();

    void test_insert_lines();
    void test_insert_lines1();
    void test_insert_lines2();
    void test_delete_line();
    void test_delete_line1();
    void test_delete_line2();
    void test_delete_lines();
    void test_delete_lines1();
    void test_delete_all();
    void test_put_line();
    void test_put_line1();
    void test_move_line_to1();
    void test_move_line_to2();
    void test_clear();
    void test_content_changed();
    void test_code_block_delta_lexical_context();
    void test_reparse();
    void test_find_last_line_by_seq();

    void test_crash_on_debian_amd_64();

    void test_fold_state_dirty();
    void test_edit_operations_are_batched();

    void initSignalTest();
    void onChanged();
};
}
#endif
