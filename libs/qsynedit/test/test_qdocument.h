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
    void test_calcSegmentInterval();
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
};
}
