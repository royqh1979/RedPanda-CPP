#include <QTest>
#include <qt_utils/utils.h>
#include "test_cppsyntaxer.h"
#include "test_utils.h"

TestCppSyntaxer::TestCppSyntaxer(QObject *parent) : QObject(parent)
{

}

void TestCppSyntaxer::test_integer_literal1()
{
    QStringList example{
        "int d = 42;",
        "int o = 052;",
        "int x = 0x2a;",
        "int X = 0X2A;",
        "int b = 0b101010; // C++14"
    };


    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,example);
    QCOMPARE(tokenInfos[0][0]->token, "int");
    QCOMPARE(tokenInfos[0][1]->token, " ");
    QCOMPARE(tokenInfos[0][2]->token, "d");
    QCOMPARE(tokenInfos[0][3]->token, " ");
    QCOMPARE(tokenInfos[0][4]->token, "=");
    QCOMPARE(tokenInfos[0][5]->token, " ");
    QCOMPARE(tokenInfos[0][6]->token, "42");
    QCOMPARE(tokenInfos[0][6]->attribute, mSyntaxer.numberAttribute());
    QCOMPARE(tokenInfos[0][7]->token, ";");

    QCOMPARE(tokenInfos[1][6]->token, "052");
    QCOMPARE(tokenInfos[1][6]->attribute, mSyntaxer.octAttribute());

    QCOMPARE(tokenInfos[2][6]->token, "0x2a");
    QCOMPARE(tokenInfos[2][6]->attribute, mSyntaxer.hexAttribute());

    QCOMPARE(tokenInfos[3][6]->token, "0X2A");
    QCOMPARE(tokenInfos[3][6]->attribute, mSyntaxer.hexAttribute());

    QCOMPARE(tokenInfos[4][6]->token, "0b101010");
    QCOMPARE(tokenInfos[4][6]->attribute, mSyntaxer.binAttribute());

}

void TestCppSyntaxer::test_integer_literal2()
{
    QStringList example{
        "unsigned long long l1 = 18446744073709550592ull;       // C++11",
        "unsigned long long l2 = 18'446'744'073'709'550'592llu; // C++14",
        "unsigned long long l3 = 1844'6744'0737'0955'0592uLL;   // C++14",
        "unsigned long long l4 = 184467'440737'0'95505'92LLU;   // C++14"
    };
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,example);

    QCOMPARE(tokenInfos[0][10]->token, "18446744073709550592ull");
    QCOMPARE(tokenInfos[0][10]->attribute, mSyntaxer.numberAttribute());

    QCOMPARE(tokenInfos[1][10]->token, "18'446'744'073'709'550'592llu");
    QCOMPARE(tokenInfos[1][10]->attribute, mSyntaxer.numberAttribute());

    QCOMPARE(tokenInfos[2][10]->token, "1844'6744'0737'0955'0592uLL");
    QCOMPARE(tokenInfos[2][10]->attribute, mSyntaxer.numberAttribute());

    QCOMPARE(tokenInfos[3][10]->token, "184467'440737'0'95505'92LLU");
    QCOMPARE(tokenInfos[3][10]->attribute, mSyntaxer.numberAttribute());

}

void TestCppSyntaxer::test_integer_literal3()
{
    QStringList text = readFileToLines("resources/int_literal.cpp");
    QVERIFY(!text.isEmpty());
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,text);
    QCOMPARE(filterTokens(tokenInfos,
                          {
                              mSyntaxer.numberAttribute(),
                              mSyntaxer.hexAttribute(),
                              mSyntaxer.octAttribute(),
                              mSyntaxer.binAttribute()
                          }),
             QStringList({
                 "123","0123","0x123",
                             "0b10","12345678901234567890ull",
                             "12345678901234567890u",
                             "9223372036854775808u",
                             "9223372036854775807",
                             "1","202011L","0UZ","0Z"
             }));
}

void TestCppSyntaxer::test_float_literal1()
{

}
