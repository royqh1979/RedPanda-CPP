#ifndef TEST_DOCUMENT_H
#define TEST_DOCUMENT_H
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
    int mChangedCount;
    std::shared_ptr<Document> mDoc;
private slots:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();
    void test_load_from_file();
    void test_load_from_file2();
    void test_load_from_empty_file();
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
    void test_find_last_line_by_seq();

    void test_crash_on_debian_amd_64();

    void initSignalTest();
    void onChanged();
};
}
#endif
