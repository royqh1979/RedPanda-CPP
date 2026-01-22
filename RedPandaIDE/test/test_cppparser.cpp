#include <QTest>
#include "src/parser/cppparser.h"
#include "test_cppparser.h"

TestCppParser::TestCppParser(QObject *parent):
    QObject{parent}
{
    init_parser();
}

void TestCppParser::init_parser()
{
    mParser = std::make_shared<CppParser>();
}

void TestCppParser::test_parse_var()
{
    mParser->setOnGetFileStream([](const QString& filename, QStringList& buffer){
        buffer=QStringList({
                               "int xxx=10;"
                           });
           return true;
                                });
    CppParser::parseFileBlocking(mParser,"text.cpp",false,"");
    QCOMPARE(mParser->statementList().count(),1);
    PStatement statement = mParser->findStatement("xxx");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"xxx");
    QCOMPARE(statement->kind,StatementKind::Variable);

}
