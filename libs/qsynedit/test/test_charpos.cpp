#include <QtTest>
#include <QCoreApplication>

#include "qsynedit/types.h"
#include "test_charpos.h"

namespace QSynedit {

void TestCharPos::test_default_constructor() {
    CharPos pos;
    QVERIFY(pos.ch == -1);
    QVERIFY(pos.line == -1);
}

void TestCharPos::test_constructor()
{
    int ch = 3;
    int line = 5;
    CharPos pos{ch,line};
    QVERIFY(pos.ch == ch);
    QVERIFY(pos.line == line);
}

void TestCharPos::test_copy_constructor()
{
    CharPos pos{4,9};
    CharPos pos2{pos};
    QVERIFY(pos.ch == pos2.ch);
    QVERIFY(pos.line == pos2.line);
}

void TestCharPos::test_isInvalid()
{
    CharPos pos;
    QVERIFY(!pos.isValid());
    pos = CharPos{0,0};
    QVERIFY(pos.isValid());
    pos = CharPos{3,5};
    QVERIFY(pos.isValid());
}

void TestCharPos::test_compare()
{
    CharPos pos0{5,3};
    CharPos pos1{10,3};
    CharPos pos2{20,3};
    CharPos pos4{5,5};
    CharPos pos5{10,10};
    CharPos pos6{20,20};
    CharPos pos7{5,3};

    QVERIFY(pos0==pos7);
    QVERIFY(pos0==pos0);
    QVERIFY(pos0!=pos1);
    QVERIFY(pos0!=pos4);

    QVERIFY(pos0<pos1);
    QVERIFY(pos0<pos4);
    QVERIFY(pos2<pos5);
    QVERIFY(pos2<pos6);

    QVERIFY(pos1>pos0);
    QVERIFY(pos4>pos0);
    QVERIFY(pos5>pos2);
    QVERIFY(pos6>pos2);


    QVERIFY(pos0<=pos0);
    QVERIFY(pos0<=pos1);
    QVERIFY(pos0<=pos4);
    QVERIFY(pos2<=pos5);
    QVERIFY(pos2<=pos6);

    QVERIFY(pos0>=pos0);
    QVERIFY(pos1>=pos0);
    QVERIFY(pos4>=pos0);
    QVERIFY(pos5>=pos2);
    QVERIFY(pos6>=pos2);

}

}
