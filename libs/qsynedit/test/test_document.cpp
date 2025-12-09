#include <QtTest>
#include <QCoreApplication>
#include "test_document.h"
#include "qsynedit/document.h"

namespace QSynedit {

TestDocument::TestDocument()
{
}

TestDocument::~TestDocument()
{

}

void TestDocument::init()
{
    mDoc=std::make_shared<Document>(QFont{"monospace"});
}

void TestDocument::cleanup()
{
    disconnect(this);
}

void TestDocument::initTestCase()
{
    {
        QFileInfo file("resources/test1.cpp");
        QVERIFY(file.exists());
    }
    {
        QFileInfo file("resources/emoji.txt");
        QVERIFY(file.exists());
    }
}

void TestDocument::cleanupTestCase()
{

}

void TestDocument::test_load_from_file()
{
    QByteArray encoding;

    mDoc->loadFromFile("resources/test1.cpp",ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(mDoc->count(),6);
    QCOMPARE(mDoc->content(), QStringList({"#include <stdio.h>",
                                           "",
                                           "int main() {",
                                           "\tprintf(\"lala\\n\");",
                                           "\treturn 0;",
                                           "}"}));
}

void TestDocument::test_emoji_glyphs()
{
    QByteArray encoding;
    mDoc->loadFromFile("resources/emoji.txt",ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(encoding, ENCODING_UTF8);

    QCOMPARE(mDoc->glyphCount(0),11);
    QCOMPARE(mDoc->glyphCount(1),7);
    QCOMPARE(mDoc->glyphCount(2),7);

    QCOMPARE(mDoc->glyph(0,0),"🌴");
    QCOMPARE(mDoc->glyph(0,1),"🌳");
    QCOMPARE(mDoc->glyph(0,2),"🌵");
    QCOMPARE(mDoc->glyph(0,3),"🌶");

    QCOMPARE(mDoc->glyphAt(0,0),"🌴");
    QCOMPARE(mDoc->glyphAt(0,1),"🌴");
    QCOMPARE(mDoc->glyphAt(0,2),"🌳");
    QCOMPARE(mDoc->glyphAt(0,3),"🌳");
    QCOMPARE(mDoc->glyphAt(0,4),"🌵");
    QCOMPARE(mDoc->glyphAt(0,5),"🌵");
    QCOMPARE(mDoc->glyphAt(0,6),"🌶");
    QCOMPARE(mDoc->glyphAt(0,7),"🌶");

}

void TestDocument::test_set_text()
{
    initSignalTest();
    mDoc->setText("int x;\nint y;\n");

    QCOMPARE(mDoc->count(),2);
    QCOMPARE(mDoc->getLine(0),"int x;");
    QCOMPARE(mDoc->getLine(1),"int y;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_set_contents()
{
    initSignalTest();
    mDoc->setContents({"int x1;","int y1;"});

    QCOMPARE(mDoc->count(),2);
    QCOMPARE(mDoc->getLine(0),"int x1;");
    QCOMPARE(mDoc->getLine(1),"int y1;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_add_line()
{
    mDoc->setContents({"int x1;","int y1;"});

    initSignalTest();
    mDoc->addLine("int z;");

    QCOMPARE(mDoc->count(),3);
    QCOMPARE(mDoc->content(),
             QStringList({"int x1;",
                          "int y1;",
                          "int z;"}));

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_add_lines()
{
    mDoc->setContents({"int x1;","int y1;"});

    initSignalTest();
    mDoc->addLines({"int z;","int q;"});

    QCOMPARE(mDoc->count(),4);
    QCOMPARE(mDoc->content(),
             QStringList({"int x1;",
                          "int y1;",
                          "int z;",
                          "int q;"}));

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_insert_line()
{
    mDoc->setContents({"int x1;","int y1;"});

    initSignalTest();
    mDoc->insertLine(0,"int p;");

    QCOMPARE(mDoc->count(),3);
    QCOMPARE(mDoc->content(),
             QStringList({"int p;",
                          "int x1;",
                          "int y1;"}));

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_insert_line1()
{
    mDoc->setContents({"int x1;","int y1;"});

    initSignalTest();
    mDoc->insertLine(1,"int p;");

    QCOMPARE(mDoc->count(),3);
    QCOMPARE(mDoc->content(),
             QStringList({"int x1;",
                          "int p;",
                          "int y1;"}));

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_insert_line2()
{
    mDoc->setContents({"int x1;","int y1;"});

    initSignalTest();
    mDoc->insertLine(2,"int p;");

    QCOMPARE(mDoc->count(),3);
    QCOMPARE(mDoc->getLine(0),"int x1;");
    QCOMPARE(mDoc->getLine(1),"int y1;");
    QCOMPARE(mDoc->getLine(2),"int p;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_insert_lines()
{
    mDoc->setContents({"int x1;","int y1;"});

    initSignalTest();
    mDoc->insertLines(0,2);

    QCOMPARE(mDoc->count(),4);
    QCOMPARE(mDoc->getLine(0),"");
    QCOMPARE(mDoc->getLine(1),"");
    QCOMPARE(mDoc->getLine(2),"int x1;");
    QCOMPARE(mDoc->getLine(3),"int y1;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_insert_lines1()
{
    mDoc->setContents({"int x1;","int y1;"});

    initSignalTest();
    mDoc->insertLines(1,2);

    QCOMPARE(mDoc->count(),4);
    QCOMPARE(mDoc->getLine(0),"int x1;");
    QCOMPARE(mDoc->getLine(1),"");
    QCOMPARE(mDoc->getLine(2),"");
    QCOMPARE(mDoc->getLine(3),"int y1;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_insert_lines2()
{
    mDoc->setContents({"int x1;","int y1;"});

    initSignalTest();
    mDoc->insertLines(2,2);

    QCOMPARE(mDoc->count(),4);
    QCOMPARE(mDoc->getLine(0),"int x1;");
    QCOMPARE(mDoc->getLine(1),"int y1;");
    QCOMPARE(mDoc->getLine(2),"");
    QCOMPARE(mDoc->getLine(3),"");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_delete_line()
{
    mDoc->setContents({"int x1;","int y1;","int z1;"});

    initSignalTest();
    mDoc->deleteLine(0);

    QCOMPARE(mDoc->count(),2);
    QCOMPARE(mDoc->getLine(0),"int y1;");
    QCOMPARE(mDoc->getLine(1),"int z1;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_delete_line1()
{
    mDoc->setContents({"int x1;","int y1;","int z1;"});

    initSignalTest();
    mDoc->deleteLine(1);

    QCOMPARE(mDoc->count(),2);
    QCOMPARE(mDoc->getLine(0),"int x1;");
    QCOMPARE(mDoc->getLine(1),"int z1;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_delete_line2()
{
    mDoc->setContents({"int x1;","int y1;","int z1;"});

    initSignalTest();
    mDoc->deleteLine(2);

    QCOMPARE(mDoc->count(),2);
    QCOMPARE(mDoc->getLine(0),"int x1;");
    QCOMPARE(mDoc->getLine(1),"int y1;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_delete_lines()
{
    mDoc->setContents({"int x1;","int y1;","int z1;"});

    initSignalTest();
    mDoc->deleteLines(0,2);

    QCOMPARE(mDoc->count(),1);
    QCOMPARE(mDoc->getLine(0),"int z1;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_delete_lines1()
{
    mDoc->setContents({"int x1;","int y1;","int z1;"});

    initSignalTest();
    mDoc->deleteLines(1,2);

    QCOMPARE(mDoc->count(),1);
    QCOMPARE(mDoc->getLine(0),"int x1;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_put_line()
{
    mDoc->setContents({"int x1;","int y1;","int z1;"});

    initSignalTest();
    mDoc->putLine(0,"test");

    QCOMPARE(mDoc->count(),3);
    QCOMPARE(mDoc->getLine(0),"test");
    QCOMPARE(mDoc->getLine(1),"int y1;");
    QCOMPARE(mDoc->getLine(2),"int z1;");

    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_put_line1()
{
    mDoc->setContents({"int x1;","int y1;","int z1;"});

    initSignalTest();
    mDoc->putLine(2,"test");

    QCOMPARE(mDoc->count(),3);
    QCOMPARE(mDoc->getLine(0),"int x1;");
    QCOMPARE(mDoc->getLine(1),"int y1;");
    QCOMPARE(mDoc->getLine(2),"test");
    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_move_line_to1()
{
    mDoc->setContents({"int a;","int b;","int c;","int d;","int e;"});

    initSignalTest();
    mDoc->moveLineTo(2,4);

    QCOMPARE(mDoc->count(),5);
    QCOMPARE(mDoc->content(),
             QStringList({"int a;",
                          "int b;",
                          "int d;",
                          "int e;",
                          "int c;"}));
    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_move_line_to2()
{
    mDoc->setContents({"int a;","int b;","int c;","int d;","int e;"});

    initSignalTest();
    mDoc->moveLineTo(4,2);

    QCOMPARE(mDoc->count(),5);
    QCOMPARE(mDoc->content(),
             QStringList({"int a;",
                          "int b;",
                          "int e;",
                          "int c;",
                          "int d;",
                          }));
    //signals
    QCOMPARE(mChangedCount,1);
}

void TestDocument::test_clear()
{
    QByteArray encoding;

    mDoc->loadFromFile("resources/test1.cpp",ENCODING_AUTO_DETECT,encoding);

    initSignalTest();
    mDoc->clear();

    QVERIFY(mDoc->count()==0);

    QCOMPARE(mChangedCount, 1);
}

void TestDocument::test_find_last_line_by_seq()
{
    mDoc=std::make_shared<Document>(QFont{});
    QByteArray encoding;

    mDoc->loadFromFile("resources/test1.cpp",ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(mDoc->count(),6);
    int seq = mDoc->getLineSeq(1);
    QCOMPARE(mDoc->findPrevLineBySeq(5,seq),1);
}

void TestDocument::initSignalTest()
{
    mChangedCount=0;
    connect(mDoc.get(), &Document::changed, this, &TestDocument::onChanged);
}

void TestDocument::onChanged()
{
    mChangedCount++;
}

void TestDocumentHelpers::test_calcGlyphStartCharList()
{
    QString s{"int 测试();"};
    QList<int> startChars = calcGlyphStartCharList(s);
    QList<int> expects{0,1,2,3,4,5,6,7,8};
    QCOMPARE(startChars, expects);
}

void TestDocumentHelpers::test_calcSegmentInterval()
{
    QList<int> segments{0,3,11,20};
    int maxVal=25;
    QList<int> expects{3,8,9,5};
    for(int i=0;i<segments.length();i++) {
        QCOMPARE(calcSegmentInterval(segments,maxVal,i), expects[i]);
    }
}

void TestDocumentHelpers::test_segmentIntervalStart()
{
    QList<int> segments{0,3,11,20};
    int maxVal=25;
    QList<int> expects{0,3,11,20,25};
    for(int i=0;i<expects.length();i++) {
        QCOMPARE(segmentIntervalStart(segments,maxVal,i), expects[i]);
    }
}

void TestDocumentHelpers::test_searchForSegmentIdx()
{
    QList<int> segments{0,3,11};
    int maxVal=15;
    QCOMPARE(searchForSegmentIdx(segments,maxVal,0), 0);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,1), 0);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,2), 0);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,3), 1);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,4), 1);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,5), 1);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,6), 1);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,7), 1);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,8), 1);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,9), 1);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,10), 1);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,11), 2);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,12), 2);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,13), 2);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,14), 2);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,15), 3);
    QCOMPARE(searchForSegmentIdx(segments,maxVal,16), 3);
}

}
