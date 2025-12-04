#include <QtTest>
#include <QCoreApplication>

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
    QList<int> mPuttedLines;
    QList<int> mInsertedLines;
    QList<int> mInsertedLineCounts;
    QList<int> mDeletedLines;
    QList<int> mDeletedLineCounts;
    int mChangedCount;
    std::shared_ptr<Document> mDoc;
private slots:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();
    void test_load_from_file();
    void test_load_emoji_file();
    void test_set_text();
    void test_set_contents();
    void test_add_line();
    void test_add_lines();

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
    void test_put_line();
    void test_put_line1();
    void test_clear();
    void test_find_last_line_by_seq();

    void initSignalTest();
    void onLinesPutted(int line);
    void onLinesInserted(int startLine, int count);
    void onLinesDeleted(int startLine, int count);
    void onChanged();
};
}
