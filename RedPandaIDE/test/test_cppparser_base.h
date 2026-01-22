#ifndef TEST_CPPPARSER_BASE_H
#define TEST_CPPPARSER_BASE_H
#include <QObject>
#include <memory>

class CppParser;

class TestCppParserBase: public QObject
{
    Q_OBJECT
public:
    TestCppParserBase(QObject *parent=nullptr);
protected:
    void init_parser();

protected:
    std::shared_ptr<CppParser> mParser;
};

#endif
