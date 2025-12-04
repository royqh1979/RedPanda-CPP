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

class TestDocument : public QObject
{
    Q_OBJECT

public:
    TestDocument();
    ~TestDocument();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_load_from_file();
    void test_load_emoji_file();
    void test_set_text();
    void test_set_contents();
    void test_add_line();
    void test_add_lines();
};
}
