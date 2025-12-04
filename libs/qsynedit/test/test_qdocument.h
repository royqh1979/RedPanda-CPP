#include <QtTest>
#include <QCoreApplication>

// add necessary includes here

class TestQDocument : public QObject
{
    Q_OBJECT

public:
    TestQDocument();
    ~TestQDocument();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_case1();

};
