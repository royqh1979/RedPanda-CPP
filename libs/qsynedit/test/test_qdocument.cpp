#include <QtTest>
#include <QCoreApplication>
#include "test_qdocument.h"
#include "qsynedit/document.h"

namespace QSynedit {

TestDocument::TestDocument()
{

}

TestDocument::~TestDocument()
{

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
    PDocument doc=std::make_shared<Document>(QFont{});
    QByteArray encoding;

    doc->loadFromFile("resources/test1.cpp",ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(doc->count(),6);
    QCOMPARE(doc->getLine(0),"#include <stdio.h>");
    QCOMPARE(doc->getLine(1),"");
    QCOMPARE(doc->getLine(2),"int main() {");
    QCOMPARE(doc->getLine(3),"\tprintf(\"lala\\n\");");
    QCOMPARE(doc->getLine(4),"\treturn 0;");
    QCOMPARE(doc->getLine(5),"}");
}

void TestDocument::test_load_emoji_file()
{
    PDocument doc=std::make_shared<Document>(QFont{});
    QByteArray encoding;

    doc->loadFromFile("resources/emoji.txt",ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(encoding, ENCODING_UTF8);

    QCOMPARE(doc->glyphCount(0),11);
    QCOMPARE(doc->glyphCount(1),7);
    QCOMPARE(doc->glyphCount(2),7);

    QCOMPARE(doc->glyph(0,0),"🌴");
    QCOMPARE(doc->glyph(0,1),"🌳");
    QCOMPARE(doc->glyph(0,2),"🌵");
    QCOMPARE(doc->glyph(0,3),"🌶");

    QCOMPARE(doc->glyphAt(0,0),"🌴");
    QCOMPARE(doc->glyphAt(0,1),"🌴");
    QCOMPARE(doc->glyphAt(0,2),"🌳");
    QCOMPARE(doc->glyphAt(0,3),"🌳");
    QCOMPARE(doc->glyphAt(0,4),"🌵");
    QCOMPARE(doc->glyphAt(0,5),"🌵");
    QCOMPARE(doc->glyphAt(0,6),"🌶");
    QCOMPARE(doc->glyphAt(0,7),"🌶");

}

void TestDocument::test_set_text()
{
    PDocument doc=std::make_shared<Document>(QFont{});
    doc->setText("int x;\nint y;\n");

    QCOMPARE(doc->count(),2);
    QCOMPARE(doc->getLine(0),"int x;");
    QCOMPARE(doc->getLine(1),"int y;");
}

void TestDocument::test_set_contents()
{
    PDocument doc=std::make_shared<Document>(QFont{});
    doc->setContents({"int x1;","int y1;"});
    QCOMPARE(doc->count(),2);
    QCOMPARE(doc->getLine(0),"int x1;");
    QCOMPARE(doc->getLine(1),"int y1;");
}

void TestDocument::test_add_line()
{
    PDocument doc=std::make_shared<Document>(QFont{});
    doc->setContents({"int x1;","int y1;"});
    doc->addLine("int z;");
    QCOMPARE(doc->count(),3);
    QCOMPARE(doc->getLine(2),"int z;");
}

void TestDocument::test_add_lines()
{
    PDocument doc=std::make_shared<Document>(QFont{});
    doc->setContents({"int x1;","int y1;"});
    doc->addLines({"int z;","int q;"});
    QCOMPARE(doc->count(),4);
    QCOMPARE(doc->getLine(2),"int z;");
    QCOMPARE(doc->getLine(3),"int q;");
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
