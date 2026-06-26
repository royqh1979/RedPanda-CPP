#include <QTest>
#include "test_cpptokenizer.h"


TestCppTokenizer::TestCppTokenizer(QObject *parent)
{
}

void TestCppTokenizer::test_parse_string1()
{
    mTokenizer.clear();
    mTokenizer.tokenize(QStringList{
                            "\"test\";"
                         });
    QCOMPARE(mTokenizer.tokenCount(),2);
    QCOMPARE(mTokenizer[0]->text,"\"\"");
    QCOMPARE(mTokenizer[1]->text,";");
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
    QCOMPARE(mTokenizer.tokenCount(),9);
    QCOMPARE(mTokenizer[0]->text,"const");
    QCOMPARE(mTokenizer[1]->text,"std::vector<std::string>");
    QCOMPARE(mTokenizer[2]->text,"s[5]");
    QCOMPARE(mTokenizer[3]->text,",");
    QCOMPARE(mTokenizer[4]->text,"s2");
    QCOMPARE(mTokenizer[5]->text,"{");
    QCOMPARE(mTokenizer[6]->text,"\"\"");
    QCOMPARE(mTokenizer[7]->text,"}");
    QCOMPARE(mTokenizer[8]->text,";");

}

void TestCppTokenizer::test_parse_string2()
{
    mTokenizer.clear();
    mTokenizer.tokenize(QStringList{
                            "static const std::string s4(\"test\");"
                         });
    QCOMPARE(mTokenizer.tokenCount(),8);
    QCOMPARE(mTokenizer[0]->text,"static");
    QCOMPARE(mTokenizer[1]->text,"const");
    QCOMPARE(mTokenizer[2]->text,"std::string");
    QCOMPARE(mTokenizer[3]->text,"s4");
    QCOMPARE(mTokenizer[4]->text,"(");
    QCOMPARE(mTokenizer[5]->text,"\"\"");
    QCOMPARE(mTokenizer[6]->text,")");
    QCOMPARE(mTokenizer[7]->text,";");
}

void TestCppTokenizer::test_parse_char()
{
    mTokenizer.clear();
    mTokenizer.tokenize(QStringList{
                            "ch='c';"
                         });
    QCOMPARE(mTokenizer.tokenCount(),4);
    QCOMPARE(mTokenizer[0]->text,"ch");
    QCOMPARE(mTokenizer[1]->text,"=");
    QCOMPARE(mTokenizer[2]->text,"\'\'");
    QCOMPARE(mTokenizer[3]->text,";");
}
