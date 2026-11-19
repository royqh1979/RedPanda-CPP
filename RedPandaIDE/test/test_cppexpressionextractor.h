#ifndef TEST_CPPEXPRESSIONEXTRACTOR_H
#define TEST_CPPEXPRESSIONEXTRACTOR_H

#include <QObject>

class TestCppExpressionExtractor : public QObject
{
    Q_OBJECT
private slots:
    void test_extract_data();
    void test_extract();
};

#endif // TEST_CPPEXPRESSIONEXTRACTOR_H
