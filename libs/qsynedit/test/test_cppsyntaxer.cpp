#include <QTest>
#include <qt_utils/utils.h>
#include "test_cppsyntaxer.h"
#include "test_utils.h"

using QSynedit::CppSyntaxer;
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
        "unsigned long long l1 = 18446744073709550592ull;       // C++11 ",
        "unsigned long long l2 = 18'446'744'073'709'550'592llu; // C++14 ",
        "unsigned long long l3 = 1844'6744'0737'0955'0592uLL;   // C++14 ",
        "unsigned long long l4 = 184467'440737'0'95505'92LLU;   // C++14 "
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
    QStringList text = readFileToLines("resources/float_literal.cpp");
    QVERIFY(!text.isEmpty());
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,text);
    QCOMPARE(filterTokens(tokenInfos, mSyntaxer.floatAttribute()),
             QStringList({
                             "58.","4e2","123.456e-67",
                             "123.456e-67f",".1E4f",
                             "0x10.1p0",
                             "0x1p5",
                             // "0x1e5", /* integer, not a float */
                             "3.14'15'92","1.18e-4932l",
                             "3.4028234e38f",
                             "3.4028234e38",
                             "3.4028234e38l",
                             "3.4028234e38f",
                             "3.4028234e38f","3.4028235e38f",
                             "3.4028234e38","3.4028235e38",
                             "3.4028234e38f","3.4028234e38"
             }));
    QCOMPARE(filterTokens(tokenInfos, mSyntaxer.hexAttribute()),
             QStringList({
                             "0x1e5"
                         }));
}

void TestCppSyntaxer::test_string_literal1()
{
    TokenInfoList tokenInfos = parseLine(&mSyntaxer,"\"Hello World\";");
    PTokenInfo tokenInfo;
    tokenInfo = tokenInfos[0];
    QCOMPARE(tokenInfo->token, "\"");
    QCOMPARE(tokenInfo->attribute, mSyntaxer.stringAttribute());
    QVERIFY(mSyntaxer.isStringNotFinished(tokenInfo->state));
    tokenInfo = tokenInfos[1];
    QCOMPARE(tokenInfo->token, "Hello");
    QCOMPARE(tokenInfo->attribute, mSyntaxer.stringAttribute());
    QVERIFY(mSyntaxer.isStringNotFinished(tokenInfo->state));
    tokenInfo = tokenInfos[2];
    QCOMPARE(tokenInfo->token, " ");
    QCOMPARE(tokenInfo->attribute, mSyntaxer.whitespaceAttribute());
    QVERIFY(mSyntaxer.isStringNotFinished(tokenInfo->state));
    tokenInfo = tokenInfos[3];
    QCOMPARE(tokenInfo->token, "World");
    QCOMPARE(tokenInfo->attribute, mSyntaxer.stringAttribute());
    QVERIFY(mSyntaxer.isStringNotFinished(tokenInfo->state));
    tokenInfo = tokenInfos[4];
    QCOMPARE(tokenInfo->token, "\"");
    QCOMPARE(tokenInfo->attribute, mSyntaxer.stringAttribute());
    QVERIFY(!mSyntaxer.isStringNotFinished(tokenInfo->state));

    tokenInfo = tokenInfos[5];
    QCOMPARE(tokenInfo->token, ";");
    QCOMPARE(tokenInfo->attribute, mSyntaxer.symbolAttribute());
}

void TestCppSyntaxer::test_string_literal2()
{
    QStringList text{
        "char array1[] = \"Foo\" \"bar\";"
    };
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,text);
    QStringList strings = filterTokens(tokenInfos, mSyntaxer.stringAttribute());
    QCOMPARE(strings,QStringList({
                 "\"",
                 "Foo",
                 "\"",
                 "\"",
                 "bar",
                 "\""
                                 }));
}

void TestCppSyntaxer::test_string_literal3()
{
    //string not correctly enclosed at line end
    QStringList text{
        "char array1[] = \"Foo\" \"bar;",
        "test test\";"
    };
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,text);
    QStringList strings = filterTokens(tokenInfos, mSyntaxer.stringAttribute());
    QCOMPARE(strings,QStringList({
                 "\"",
                 "Foo",
                 "\"",
                 "\"",
                 "bar",
                 ";",
                 "\"",
                 ";"
                                 }));
}

void TestCppSyntaxer::test_rawstring_literal1()
{
    //multiline
    QStringList text{
        "const char* s1 = R\"foo(",
        "Hello",
        "  World",
        ")foo\";"
    };
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,text);
    PTokenInfo tokenInfo;
    tokenInfo = tokenInfos[0][9];
    QCOMPARE(tokenInfo->token,"R\"foo(");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));

    tokenInfo = tokenInfos[1][0];
    QCOMPARE(tokenInfo->token,"Hello");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(!mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));

    tokenInfo = tokenInfos[2][0];
    QCOMPARE(tokenInfo->token,"  ");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.whitespaceAttribute());
    QVERIFY(!mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));

    tokenInfo = tokenInfos[2][1];
    QCOMPARE(tokenInfo->token,"World");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(!mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));

    tokenInfo = tokenInfos[3][0];
    QCOMPARE(tokenInfo->token,")foo\"");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(!mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(mSyntaxer.isRawStringEnd(tokenInfo->state));
}

void TestCppSyntaxer::test_rawstring_literal2()
{
    QStringList text{
        "R\"foo(asdfsdfoo()foo)foo\"",
    };
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,text);
    PTokenInfo tokenInfo;
    tokenInfo = tokenInfos[0][0];
    QCOMPARE(tokenInfo->token,"R\"foo(");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));
    tokenInfo = tokenInfos[0][1];
    QCOMPARE(tokenInfo->token,"asdfsdfoo()foo");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(!mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));
    tokenInfo = tokenInfos[0][2];
    QCOMPARE(tokenInfo->token,")foo\"");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(!mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(mSyntaxer.isRawStringEnd(tokenInfo->state));
}

void TestCppSyntaxer::test_rawstring_literal3()
{
    //not enclosed at line end(no crash)
    QStringList text{
        "R\"foo(asdfsdfoo()foo)foo",
    };
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,text);
    PTokenInfo tokenInfo;
    tokenInfo = tokenInfos[0][0];
    QCOMPARE(tokenInfo->token,"R\"foo(");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));
    tokenInfo = tokenInfos[0][1];
    QCOMPARE(tokenInfo->token,"asdfsdfoo()foo)foo");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(!mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));
}

void TestCppSyntaxer::test_rawstring_literal4()
{
    //github issue #657
    QStringList text{
        "#include<bits/stdc++.h>",
        "int main(){",
        "  std::cout<<R\"(()a\")\";",
        "  std::cout<<\"123\";",
        "  return 0;",
        "}"
    };
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,text);
    PTokenInfo tokenInfo;
    tokenInfo = tokenInfos[2][5];
    QCOMPARE(tokenInfo->token,"R\"(");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));
    tokenInfo = tokenInfos[2][6];
    QCOMPARE(tokenInfo->token,"()a\"");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(!mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringEnd(tokenInfo->state));
    tokenInfo = tokenInfos[2][7];
    QCOMPARE(tokenInfo->token,")\"");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    QVERIFY(!mSyntaxer.isRawStringStart(tokenInfo->state));
    QVERIFY(!mSyntaxer.isRawStringNoEscape(tokenInfo->state));
    QVERIFY(mSyntaxer.isRawStringEnd(tokenInfo->state));
}

void TestCppSyntaxer::test_backslash_at_line_end1()
{
    //backslash at line end, see: https://cppreference.com/w/c/language/translation_phases.html#Phase_2
    QStringList text = readFileToLines("resources/backslash.cpp");
    QVERIFY(!text.isEmpty());
    QList<TokenInfoList> tokenInfos = parseLines(&mSyntaxer,text);
    PTokenInfo tokenInfo;
    tokenInfo = tokenInfos[2][4];
    QCOMPARE(tokenInfo->token,"p");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.preprocessorAttribute());
    tokenInfo = tokenInfos[2][5];
    QCOMPARE(tokenInfo->token,"\\");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.symbolAttribute());
    QVERIFY(mSyntaxer.mergeWithNextLine(tokenInfo->state));
    tokenInfo = tokenInfos[3][0];
    QCOMPARE(tokenInfo->token,"u");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.preprocessorAttribute());
    tokenInfo = tokenInfos[3][1];
    QCOMPARE(tokenInfo->token,"\\");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.symbolAttribute());
    QVERIFY(mSyntaxer.mergeWithNextLine(tokenInfo->state));
    tokenInfo = tokenInfos[4][0];
    QCOMPARE(tokenInfo->token,"t");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.preprocessorAttribute());
    tokenInfo = tokenInfos[4][1];
    QCOMPARE(tokenInfo->token,"\\");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.symbolAttribute());
    QVERIFY(mSyntaxer.mergeWithNextLine(tokenInfo->state));
    tokenInfo = tokenInfos[5][0];
    QCOMPARE(tokenInfo->token,"s");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.preprocessorAttribute());
    QVERIFY(!mSyntaxer.mergeWithNextLine(tokenInfo->state));


    tokenInfo = tokenInfos[13][17];
    QCOMPARE(tokenInfo->token,"PUT");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.identifierAttribute());
    tokenInfo = tokenInfos[13][18];
    QCOMPARE(tokenInfo->token,"\\");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.symbolAttribute());
    QVERIFY(mSyntaxer.mergeWithNextLine(tokenInfo->state));
    tokenInfo = tokenInfos[14][0];
    QCOMPARE(tokenInfo->token,"S");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.identifierAttribute());
    tokenInfo = tokenInfos[14][1];
    QCOMPARE(tokenInfo->token,"\\");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.symbolAttribute());
    QVERIFY(mSyntaxer.mergeWithNextLine(tokenInfo->state));
    tokenInfo = tokenInfos[15][0];
    QCOMPARE(tokenInfo->token,"(");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.symbolAttribute());
    tokenInfo = tokenInfos[15][7];
    QCOMPARE(tokenInfo->token,"\\");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringEscapeSequenceAttribute());
    tokenInfo = tokenInfos[15][8];
    QCOMPARE(tokenInfo->token,"\\");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.symbolAttribute());
    QVERIFY(mSyntaxer.mergeWithNextLine(tokenInfo->state));
    tokenInfo = tokenInfos[16][0];
    QCOMPARE(tokenInfo->token,"0");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringEscapeSequenceAttribute());
    tokenInfo = tokenInfos[16][1];
    QCOMPARE(tokenInfo->token,"Not");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.stringAttribute());
    tokenInfo = tokenInfos[16].last();
    QCOMPARE(tokenInfo->token,"backslash");
    QCOMPARE(tokenInfo->attribute,mSyntaxer.commentAttribute());
    QVERIFY(!mSyntaxer.mergeWithNextLine(tokenInfo->state));
}
