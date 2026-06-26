#include <QTest>
#include "test_cpptokenizer.h"


TestCppTokenizer::TestCppTokenizer(QObject *parent)
{
}

void TestCppTokenizer::test_parse_scope_resolution_operators1()
{
    mTokenizer.clear();
    mTokenizer.tokenize(QStringList{
                            "using ::std::vector;"
                         });
    QCOMPARE(mTokenizer.tokenCount(),3);
    QCOMPARE(mTokenizer[0]->text,"using");
    QCOMPARE(mTokenizer[1]->text,"::std::vector");
    QCOMPARE(mTokenizer[2]->text,";");

}

void TestCppTokenizer::test_parse_scope_resolution_operators2()
{
    mTokenizer.clear();
    mTokenizer.tokenize(QStringList{
                            "const std::vector<std::string> s[5],s2{\"test\"};"
                         });
    QCOMPARE(mTokenizer.tokenCount(),8);
    QCOMPARE(mTokenizer[0]->text,"const");
    QCOMPARE(mTokenizer[1]->text,"std::vector<std::string>");
    QCOMPARE(mTokenizer[2]->text,"s[5]");
    QCOMPARE(mTokenizer[3]->text,",");
    QCOMPARE(mTokenizer[4]->text,"s2");
    QCOMPARE(mTokenizer[5]->text,"{");
    QCOMPARE(mTokenizer[6]->text,"\"test\"");
    QCOMPARE(mTokenizer[7]->text,"}");
    QCOMPARE(mTokenizer[8]->text,";");

}
