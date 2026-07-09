#ifndef TEST_CPPPARSER_H
#define TEST_CPPPARSER_H
#include <QObject>
#include <memory>

class CppParser;

class TestCppParser: public QObject
{
    Q_OBJECT
public:
    TestCppParser(QObject *parent=nullptr);
private slots:
    void init_parser();
    void test_parse_var();
    void test_parse_vars();
    void test_struct();
    void test_structured_bindings();
protected:
    std::shared_ptr<CppParser> mParser;
};

#endif
