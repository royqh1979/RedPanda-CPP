#include <QtTest>
#include <QCoreApplication>
#include "test_qdocument.h"

TestQDocument::TestQDocument()
{

}

TestQDocument::~TestQDocument()
{

}

void TestQDocument::initTestCase()
{

}

void TestQDocument::cleanupTestCase()
{

}

void TestQDocument::test_case1()
{
    QString str = "Hello";
    QVERIFY(str.toUpper() == "HELLO");
}
