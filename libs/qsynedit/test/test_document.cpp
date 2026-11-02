#include <QTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include "test_document.h"
#include "qsynedit/document.h"
#include "qsynedit/syntaxer/cpp.h"

namespace QSynedit {

namespace {

QByteArray utf16LeBytes(const QString& text)
{
    QByteArray data;
    for (QChar ch : text) {
        data.append(char(ch.unicode() & 0xFF));
        data.append(char(ch.unicode() >> 8));
    }
    return data;
}

QByteArray utf32LeBytes(const QString& text)
{
    QByteArray data;
    for (char32_t ch : text.toUcs4()) {
        data.append(char(ch & 0xFF));
        data.append(char((ch >> 8) & 0xFF));
        data.append(char((ch >> 16) & 0xFF));
        data.append(char((ch >> 24) & 0xFF));
    }
    return data;
}

bool gbkCodecAvailable()
{
    TextEncoder encoder("GBK");
    return encoder.isValid();
}

}

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
    QVERIFY(mTempDir.isValid());
    writeFixture("utf8_bom.txt",
                 QByteArray("\xEF\xBB\xBF") + QString("中文第一行\nsecond line\n").toUtf8());
    writeFixture("utf16_bom.txt",
                 QByteArray("\xFF\xFE") + utf16LeBytes("中文第一行\r\nsecond line\r\n"));
    writeFixture("utf32_bom.txt",
                 QByteArray("\xFF\xFE\x00\x00",4) + utf32LeBytes("abc\ndef\n"));
    writeFixture("newline_unix.txt", "abc\ndef\n");
    writeFixture("no_trailing_newline.txt", "abc\ndef");
    writeFixture("mixed_newlines.txt", "a\r\nb\nc\r\n");
    writeFixture("binary.bin", QByteArray("abc\0def\n", 8));
    writeFixture("utf8_plain.txt", QString("中文测试\n第二行\n").toUtf8());
    if (gbkCodecAvailable()) {
        TextEncoder encoder("GBK");
        writeFixture("gbk.txt", encoder.encodeUnchecked("中文测试\r\n第二行\r\n"));
    }
}

QString TestDocument::writeFixture(const QString& name, const QByteArray& data)
{
    QString path = fixturePath(name);
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        return QString();
    if (file.write(data) != data.size())
        return QString();
    return path;
}

QString TestDocument::fixturePath(const QString& name) const
{
    return mTempDir.filePath(name);
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
    QCOMPARE(mDoc->charToGlyphIndex(0,0),0);
    QCOMPARE(mDoc->charToGlyphIndex(0,1),1);
    QCOMPARE(mDoc->charToGlyphIndex(0,2),2);
    QCOMPARE(mDoc->charToGlyphIndex(0,3),3);
    QCOMPARE(mDoc->charToGlyphIndex(0,4),4);
    QCOMPARE(mDoc->charToGlyphIndex(0,5),5);
    QCOMPARE(mDoc->charToGlyphIndex(0,6),6);
    QCOMPARE(mDoc->charToGlyphIndex(0,7),7);
    QCOMPARE(mDoc->charToGlyphIndex(0,8),8);
    QCOMPARE(mDoc->charToGlyphIndex(0,9),9);
    QCOMPARE(mDoc->charToGlyphIndex(0,10),10);
    QCOMPARE(mDoc->charToGlyphIndex(0,11),11);
    QCOMPARE(mDoc->charToGlyphIndex(0,12),12);
    QCOMPARE(mDoc->charToGlyphIndex(0,13),13);
    QCOMPARE(mDoc->charToGlyphIndex(0,14),14);
    QCOMPARE(mDoc->charToGlyphIndex(0,15),15);
    QCOMPARE(mDoc->charToGlyphIndex(0,16),16);
    QCOMPARE(mDoc->charToGlyphIndex(0,17),17);
    QCOMPARE(mDoc->charToGlyphIndex(0,18),18);
    QCOMPARE(mDoc->charToGlyphIndex(0,19),18);
    QCOMPARE(mDoc->charToGlyphIndex(1,0),0);
    QCOMPARE(mDoc->charToGlyphIndex(1,1),0);

}

void TestDocument::test_load_from_file2()
{
    QByteArray encoding;

    mDoc->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    QCOMPARE(mDoc->getLine(0), "#include <iostream>");
    QCOMPARE(mDoc->getGlyphStartCharListForTest(0),
             QList<int>({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18}));
}

void TestDocument::test_load_from_empty_file()
{
    QByteArray encoding;

    mDoc->loadFromFile("resources/empty.txt",ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(mDoc->count(),1);
    QCOMPARE(mDoc->content(), QStringList({""}));

    QVERIFY(mDoc->empty());
}

void TestDocument::test_load_utf8_bom()
{
    QByteArray encoding;

    mDoc->loadFromFile(fixturePath("utf8_bom.txt"),ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(encoding, ENCODING_UTF8_BOM);
    QCOMPARE(mDoc->content(), QStringList({"中文第一行","second line"}));
}

void TestDocument::test_load_utf16_bom()
{
    QByteArray encoding;

    mDoc->loadFromFile(fixturePath("utf16_bom.txt"),ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(encoding, ENCODING_UTF16_BOM);
    QCOMPARE(mDoc->content(), QStringList({"中文第一行","second line"}));
}

void TestDocument::test_load_utf32_bom()
{
    QByteArray encoding;

    mDoc->loadFromFile(fixturePath("utf32_bom.txt"),ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(encoding, ENCODING_UTF32_BOM);
    QCOMPARE(mDoc->content(), QStringList({"abc","def"}));
}

void TestDocument::test_load_newline_unix()
{
    QByteArray encoding;

    mDoc->loadFromFile(fixturePath("newline_unix.txt"),ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(mDoc->getNewlineType(), NewlineType::Unix);
    QCOMPARE(mDoc->content(), QStringList({"abc","def"}));
}

void TestDocument::test_load_no_trailing_newline()
{
    QByteArray encoding;

    mDoc->loadFromFile(fixturePath("no_trailing_newline.txt"),ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(mDoc->content(), QStringList({"abc","def"}));
}

void TestDocument::test_load_mixed_newlines()
{
    QByteArray encoding;

    mDoc->loadFromFile(fixturePath("mixed_newlines.txt"),ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(mDoc->getNewlineType(), NewlineType::Windows);
    QCOMPARE(mDoc->content(), QStringList({"a","b","c"}));
}

void TestDocument::test_load_nonexistent_file()
{
    QByteArray encoding;

    QVERIFY_EXCEPTION_THROWN(
                mDoc->loadFromFile(fixturePath("no_such_file.txt"),ENCODING_AUTO_DETECT,encoding),
                FileError);
}

void TestDocument::test_load_binary_file()
{
    QByteArray encoding;

    QVERIFY_EXCEPTION_THROWN(
                mDoc->loadFromFile(fixturePath("binary.bin"),ENCODING_AUTO_DETECT,encoding),
                BinaryFileError);
}

void TestDocument::test_load_with_utf8_encoding()
{
    QByteArray encoding;

    mDoc->loadFromFile(fixturePath("utf8_plain.txt"),ENCODING_UTF8,encoding);
    QCOMPARE(encoding, ENCODING_UTF8);
    QCOMPARE(mDoc->content(), QStringList({"中文测试","第二行"}));
}

void TestDocument::test_load_with_gbk_encoding()
{
    if (!gbkCodecAvailable())
        QSKIP("GBK codec not available in this Qt build");
    QByteArray encoding;

    mDoc->loadFromFile(fixturePath("gbk.txt"),"GBK",encoding);
    QCOMPARE(encoding, "GBK");
    QCOMPARE(mDoc->content(), QStringList({"中文测试","第二行"}));
}

void TestDocument::test_save_utf8_roundtrip()
{
    mDoc->setContents({"中文第一行","second line"});
    mDoc->setNewlineType(NewlineType::Unix);
    QString path = fixturePath("save_utf8.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,ENCODING_UTF8,encoding);
    }
    QCOMPARE(encoding, ENCODING_UTF8);
    {
        QFile file(path);
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(), QString("中文第一行\nsecond line\n").toUtf8());
    }
    QByteArray loadEncoding;
    mDoc->loadFromFile(path,ENCODING_AUTO_DETECT,loadEncoding);
    QCOMPARE(loadEncoding, ENCODING_UTF8);
    QCOMPARE(mDoc->content(), QStringList({"中文第一行","second line"}));
}

void TestDocument::test_save_utf8_bom()
{
    mDoc->setContents({"中文第一行","second line"});
    mDoc->setNewlineType(NewlineType::Unix);
    QString path = fixturePath("save_utf8_bom.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,ENCODING_UTF8_BOM,encoding);
    }
    {
        QFile file(path);
        QVERIFY(file.open(QFile::ReadOnly));
        QVERIFY(file.readAll().startsWith("\xEF\xBB\xBF"));
    }
    QByteArray loadEncoding;
    mDoc->loadFromFile(path,ENCODING_AUTO_DETECT,loadEncoding);
    QCOMPARE(loadEncoding, ENCODING_UTF8_BOM);
    QCOMPARE(mDoc->content(), QStringList({"中文第一行","second line"}));
}

void TestDocument::test_save_utf16_bom()
{
    mDoc->setContents({"中文第一行","second line"});
    mDoc->setNewlineType(NewlineType::Unix);
    QString path = fixturePath("save_utf16_bom.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,ENCODING_UTF16_BOM,encoding);
    }
    {
        QFile file(path);
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(),
                 QByteArray("\xFF\xFE",2) + utf16LeBytes("中文第一行\nsecond line\n"));
    }
    QByteArray loadEncoding;
    mDoc->loadFromFile(path,ENCODING_AUTO_DETECT,loadEncoding);
    QCOMPARE(loadEncoding, ENCODING_UTF16_BOM);
    QCOMPARE(mDoc->content(), QStringList({"中文第一行","second line"}));
}

void TestDocument::test_save_utf16_no_bom()
{
    mDoc->setContents({"中文第一行","second line"});
    mDoc->setNewlineType(NewlineType::Unix);
    QString path = fixturePath("save_utf16.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,ENCODING_UTF16,encoding);
    }
    {
        QFile file(path);
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(), utf16LeBytes("中文第一行\nsecond line\n"));
    }
    QByteArray loadEncoding;
    mDoc->loadFromFile(path,ENCODING_UTF16,loadEncoding);
    QCOMPARE(mDoc->content(), QStringList({"中文第一行","second line"}));
}

void TestDocument::test_save_utf32_bom()
{
    mDoc->setContents({"abc","def"});
    mDoc->setNewlineType(NewlineType::Unix);
    QString path = fixturePath("save_utf32_bom.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,ENCODING_UTF32_BOM,encoding);
    }
    {
        QFile file(path);
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(),
                 QByteArray("\xFF\xFE\x00\x00",4) + utf32LeBytes("abc\ndef\n"));
    }
    QByteArray loadEncoding;
    mDoc->loadFromFile(path,ENCODING_AUTO_DETECT,loadEncoding);
    QCOMPARE(loadEncoding, ENCODING_UTF32_BOM);
    QCOMPARE(mDoc->content(), QStringList({"abc","def"}));
}

void TestDocument::test_save_utf32_no_bom()
{
    mDoc->setContents({"abc","def"});
    mDoc->setNewlineType(NewlineType::Unix);
    QString path = fixturePath("save_utf32.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,ENCODING_UTF32,encoding);
    }
    {
        QFile file(path);
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(), utf32LeBytes("abc\ndef\n"));
    }
    QByteArray loadEncoding;
    mDoc->loadFromFile(path,ENCODING_UTF32,loadEncoding);
    QCOMPARE(mDoc->content(), QStringList({"abc","def"}));
}

void TestDocument::test_save_gbk()
{
    if (!gbkCodecAvailable())
        QSKIP("GBK codec not available in this Qt build");
    mDoc->setContents({"中文测试","第二行"});
    mDoc->setNewlineType(NewlineType::Unix);
    QString path = fixturePath("save_gbk.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,"GBK",encoding);
    }
    {
        TextEncoder encoder("GBK");
        QFile file(path);
        QVERIFY(file.open(QFile::ReadOnly));
        QCOMPARE(file.readAll(), encoder.encodeUnchecked("中文测试\n第二行\n"));
    }
    QByteArray loadEncoding;
    mDoc->loadFromFile(path,"GBK",loadEncoding);
    QCOMPARE(mDoc->content(), QStringList({"中文测试","第二行"}));
}

void TestDocument::test_save_all_ascii_reports_ascii()
{
    mDoc->setContents({"abc","def"});
    mDoc->setNewlineType(NewlineType::Unix);
    QString path = fixturePath("save_ascii.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,ENCODING_UTF8,encoding);
    }
    QCOMPARE(encoding, ENCODING_ASCII);
}

void TestDocument::test_save_newline_windows()
{
    mDoc->setContents({"abc","def"});
    mDoc->setNewlineType(NewlineType::Windows);
    QString path = fixturePath("save_crlf.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,ENCODING_UTF8,encoding);
    }
    QFile file(path);
    QVERIFY(file.open(QFile::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("abc\r\ndef\r\n"));
}

void TestDocument::test_save_newline_unix()
{
    mDoc->setContents({"abc","def"});
    mDoc->setNewlineType(NewlineType::Unix);
    QString path = fixturePath("save_lf.txt");
    QByteArray encoding;
    {
        QFile file(path);
        mDoc->saveToFile(file,ENCODING_UTF8,encoding);
    }
    QFile file(path);
    QVERIFY(file.open(QFile::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("abc\ndef\n"));
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

    QCOMPARE(mDoc->charToGlyphIndex(0,0),0);
    QCOMPARE(mDoc->charToGlyphIndex(0,1),0);
    QCOMPARE(mDoc->charToGlyphIndex(0,2),1);
    QCOMPARE(mDoc->charToGlyphIndex(0,3),1);
    QCOMPARE(mDoc->charToGlyphIndex(0,4),2);

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

void TestDocument::test_set_empty_text()
{
    initSignalTest();
    mDoc->setText("");

    QCOMPARE(mDoc->count(),1);
    QCOMPARE(mDoc->getLine(0),"");

    QVERIFY(mDoc->empty());

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

void TestDocument::test_set_empty_contents()
{
    initSignalTest();
    mDoc->setContents({});

    QCOMPARE(mDoc->count(),1);
    QCOMPARE(mDoc->getLine(0),"");

    QVERIFY(mDoc->empty());

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

void TestDocument::test_search_segment_index()
{
    QString lineText = "#include <iostream>";
    QList<int> glyphStartCharList({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18});
    QCOMPARE(searchForSegmentIdx(glyphStartCharList,lineText.length(),0),0);

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

void TestDocument::test_delete_all()
{
    mDoc->setContents({"int x1;","int y1;","int z1;"});

    initSignalTest();
    mDoc->deleteLines(0,3);

    QCOMPARE(mDoc->count(),1);
    QCOMPARE(mDoc->getLine(0),"");

    QVERIFY(mDoc->empty());

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
    mDoc->moveLine(2,4);

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
    mDoc->moveLine(4,2);

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

    QCOMPARE(1, mDoc->count());

    QVERIFY(mDoc->empty());

    QCOMPARE(mChangedCount, 1);
}

void TestDocument::test_content_changed()
{
    QList<DocumentChangeSet> batches;
    connect(mDoc.get(), &Document::contentChanged, this,
            [&batches](const DocumentChangeSet &changes){
        batches.append(changes);
    });
    auto checkEntry = [](const DocumentChange &change, DocumentChangeKind kind,
                         int line, int count) {
        return change.kind == kind && change.line == line && change.count == count;
    };

    // setText on a fresh doc: delete the initial empty line + insert all,
    // one batch, inserts coalesced
    mDoc->setText("a\nb\nc");
    QCOMPARE(batches.count(), 1);
    QCOMPARE(batches[0].count(), 2);
    QVERIFY(checkEntry(batches[0][0], DocumentChangeKind::Delete, 0, 1));
    QVERIFY(checkEntry(batches[0][1], DocumentChangeKind::Insert, 0, 3));

    // setText again: delete all + insert all, still one batch
    batches.clear();
    mDoc->setText("x\ny");
    QCOMPARE(batches.count(), 1);
    QCOMPARE(batches[0].count(), 2);
    QVERIFY(checkEntry(batches[0][0], DocumentChangeKind::Delete, 0, 3));
    QVERIFY(checkEntry(batches[0][1], DocumentChangeKind::Insert, 0, 2));

    // clear: delete all lines but the last, then set the last one empty
    mDoc->setContents({"a","b","c"});
    batches.clear();
    mDoc->clear();
    QCOMPARE(batches.count(), 1);
    QCOMPARE(batches[0].count(), 2);
    QVERIFY(checkEntry(batches[0][0], DocumentChangeKind::Delete, 0, 2));
    QVERIFY(checkEntry(batches[0][1], DocumentChangeKind::SetLine, 0, 1));

    // clear on an already-empty doc: no changes, no signal
    batches.clear();
    mDoc->clear();
    QCOMPARE(batches.count(), 0);

    // putLine with unchanged text: not recorded
    batches.clear();
    mDoc->putLine(0, "");
    QCOMPARE(batches.count(), 0);

    // putLine with changed text: SetLine
    batches.clear();
    mDoc->putLine(0, "hello");
    QCOMPARE(batches.count(), 1);
    QVERIFY(checkEntry(batches[0][0], DocumentChangeKind::SetLine, 0, 1));

    // same-index consecutive deletes coalesce within one batch
    mDoc->setText("a\nb\nc\nd");
    batches.clear();
    mDoc->beginUpdate();
    mDoc->deleteLine(1);
    mDoc->deleteLine(1);
    mDoc->endUpdate();
    QCOMPARE(batches.count(), 1);
    QCOMPARE(batches[0].count(), 1);
    QVERIFY(checkEntry(batches[0][0], DocumentChangeKind::Delete, 1, 2));

    // no cross-kind merging: insert then delete stay separate entries
    batches.clear();
    mDoc->beginUpdate();
    mDoc->insertLine(0, "new");
    mDoc->deleteLine(0);
    mDoc->endUpdate();
    QCOMPARE(batches.count(), 1);
    QCOMPARE(batches[0].count(), 2);
    QVERIFY(checkEntry(batches[0][0], DocumentChangeKind::Insert, 0, 1));
    QVERIFY(checkEntry(batches[0][1], DocumentChangeKind::Delete, 0, 1));

    // Move entry: line=from, count=to
    batches.clear();
    mDoc->moveLine(0, 1);
    QCOMPARE(batches.count(), 1);
    QVERIFY(checkEntry(batches[0][0], DocumentChangeKind::Move, 0, 1));
}

void TestDocument::test_reparse()
{
    QSignalSpy reparseSpy(mDoc.get(), &Document::linesReparsed);

    // setSyntaxer reparses the whole (empty) document itself
    mDoc->setSyntaxer(std::make_shared<CppSyntaxer>());
    QCOMPARE(reparseSpy.count(), 1);

    // content changes alone do not reparse; the document reparses on demand
    // and the states are correct without any view attached
    mDoc->setContents({"int main() {", "    return 0;", "}"});
    reparseSpy.clear();
    int end = mDoc->reparseLines(0, 1, true);
    // parsing continues past endLine until the states converge (the early
    // stop line is an optimization detail, only the states are contractual)
    QVERIFY(end>=2);
    QCOMPARE(mDoc->braceLevel(0), 1);
    QCOMPARE(mDoc->braceLevel(1), 1);
    QCOMPARE(mDoc->braceLevel(2), 0);
    QCOMPARE(reparseSpy.count(), 1);
    QCOMPARE(reparseSpy.takeFirst().at(0).toInt(), 0);

    // without parseToEnd the reparse stops at endLine
    mDoc->putLine(0, "void f() {}");
    reparseSpy.clear();
    end = mDoc->reparseLines(0, 1, false);
    QCOMPARE(end, 1);
    QCOMPARE(mDoc->braceLevel(0), 0);
    QCOMPARE(reparseSpy.count(), 1);
}

void TestDocument::test_find_last_line_by_seq()
{
    mDoc=std::make_shared<Document>(QFont{});
    QByteArray encoding;

    mDoc->loadFromFile("resources/test1.cpp",ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(mDoc->count(),6);
    size_t seq = mDoc->getLineSeq(1);
    PDocumentLine line = mDoc->findLineBySeq(seq);
    QVERIFY(line!=nullptr);
    QCOMPARE(line->lineText(), mDoc->getLine(1));
    QCOMPARE(line->lineSeq(), seq);
}

void TestDocument::test_crash_on_debian_amd_64()
{
    mDoc=std::make_shared<Document>(QFont{});
    QByteArray encoding;

    mDoc->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    QCOMPARE(mDoc->getLine(0), "#include <iostream>");
    QCOMPARE(mDoc->charToGlyphStartPosition(0, mDoc->getLine(0), 0),0);
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
