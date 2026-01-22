#include <QTest>
#include "src/parser/cppparser.h"
#include "test_cppparser_base.h"

TestCppParserBase::TestCppParserBase(QObject *parent):
    QObject{parent}
{
    init_parser();
}

void TestCppParserBase::init_parser()
{
    mParser = std::make_shared<CppParser>();
}
