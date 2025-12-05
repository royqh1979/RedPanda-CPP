#include <QtTest>
#include <QCoreApplication>

#include "test_qsynedit.h"
#include "qsynedit/qsynedit.h"
#include "qsynedit/document.h"
#include "qsynedit/syntaxer/cpp.h"
#include "qsynedit/formatter/cppformatter.h"

template <>inline char *QTest::toString(const QSynedit::CharPos &pos) {
    return toString(QString("CharPos(ch=%1,line=%2)").arg(pos.ch).arg(pos.line));
}

namespace QSynedit {

void TestQSyneditCpp::init()
{
    mEdit = std::make_shared<QSynEdit>();
    QSynedit::EditorOptions options = QSynedit::EditorOption::AltSetsColumnMode
            | QSynedit::EditorOption::DragDropEditing | QSynedit::EditorOption::DropFiles
            | QSynedit::EditorOption::RightMouseMovesCursor
            | QSynedit::EditorOption::TabIndent
            | QSynedit::EditorOption::GroupUndo
            | QSynedit::EditorOption::SelectWordByDblClick;
    mEdit->setOptions(options);
    mEdit->setSyntaxer(std::make_shared<CppSyntaxer>());
    mEdit->setFormatter(std::make_shared<CppFormatter>());

    QByteArray encoding;
    mEdit->document()->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);
}

void TestQSyneditCpp::test_get_token_data()
{
    init();
    QTest::addColumn<QString>("word");
    QTest::addColumn<QString>("expect");

    QTest::newRow("document start")<<mEdit->tokenAt(CharPos{0,0})<<"#include";
    QTest::newRow("document end")<<mEdit->tokenAt(CharPos{0,76})<<"}";
    QTest::newRow("line start")<<mEdit->tokenAt(CharPos{0,10})<<"#define";
    QTest::newRow("before word")<<mEdit->tokenAt(CharPos{5,12})<<" ";
    QTest::newRow("word start")<<mEdit->tokenAt(CharPos{6,12})<<"ThreadPool";
    QTest::newRow("word end")<<mEdit->tokenAt(CharPos{15,12})<<"ThreadPool";
    QTest::newRow("line end")<<mEdit->tokenAt(CharPos{16,12})<<"";
}

void TestQSyneditCpp::test_get_token()
{
    QFETCH(QString, word);
    QFETCH(QString, expect);

    QCOMPARE(word, expect);
}

void TestQSyneditCpp::test_token_start_data()
{
    init();
    QTest::addColumn<CharPos>("start");
    QTest::addColumn<CharPos>("expect");

    QTest::newRow("before word start")<<mEdit->getTokenStart(CharPos{3,15})<<CharPos{0,15};
    QTest::newRow("at word start")<<mEdit->getTokenStart(CharPos{4,15})<<CharPos{4,15};
    QTest::newRow("in word")<<mEdit->getTokenStart(CharPos{10,15})<<CharPos{4,15};
    QTest::newRow("before word end")<<mEdit->getTokenStart(CharPos{13,15})<<CharPos{4,15};
    QTest::newRow("at word end")<<mEdit->getTokenStart(CharPos{14,15})<<CharPos{14,15};
}

void TestQSyneditCpp::test_token_start()
{
    QFETCH(CharPos, start);
    QFETCH(CharPos, expect);
    QCOMPARE(start, expect);
}

void TestQSyneditCpp::test_token_end_data()
{
    init();
    QTest::addColumn<CharPos>("end");
    QTest::addColumn<CharPos>("expect");

    QTest::newRow("before word start")<<mEdit->getTokenEnd(CharPos{3,15})<<CharPos{4,15};
    QTest::newRow("at word start")<<mEdit->getTokenEnd(CharPos{4,15})<<CharPos{14,15};
    QTest::newRow("in word")<<mEdit->getTokenEnd(CharPos{10,15})<<CharPos{14,15};
    QTest::newRow("before word end")<<mEdit->getTokenEnd(CharPos{13,15})<<CharPos{14,15};
    QTest::newRow("at word end")<<mEdit->getTokenEnd(CharPos{14,15})<<CharPos{15,15};
}

void TestQSyneditCpp::test_token_end()
{
    QFETCH(CharPos, end);
    QFETCH(CharPos, expect);
    QCOMPARE(end,expect);
}

void TestQSyneditCpp::test_move_caret_x_data()
{
    init();
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    {
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->moveCaretHorz(-1,false);
        QTest::newRow("backward at file start")<<mEdit->caretXY()<<CharPos{0,0};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mEdit->moveCaretHorz(1,false);
        QTest::newRow("forward at file end")<<mEdit->caretXY()<<CharPos{1,76};
    }
    {
        mEdit->setCaretXY(CharPos{0,15});
        mEdit->moveCaretHorz(-1,false);
        QTest::newRow("backward at line start")<<mEdit->caretXY()<<CharPos{7,14};
    }
    {
        mEdit->setCaretXY(CharPos{0,15});
        mEdit->moveCaretHorz(1,false);
        QTest::newRow("forward at line start")<<mEdit->caretXY()<<CharPos{1,15};
    }
    {
        mEdit->setCaretXY(CharPos{40,15});
        mEdit->moveCaretHorz(-1,false);
        QTest::newRow("backward at line end")<<mEdit->caretXY()<<CharPos{39,15};
    }
    {
        mEdit->setCaretXY(CharPos{40,15});
        mEdit->moveCaretHorz(1,false);
        QTest::newRow("forward at line end")<<mEdit->caretXY()<<CharPos{0,16};
    }
}

void TestQSyneditCpp::test_move_caret_x()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_move_caret_y_data()
{
    init();
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    {
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->moveCaretVert(-1,false);
        QTest::newRow("backward at file start")<<mEdit->caretXY()<<CharPos{0,0};
    }
    {
        mEdit->setCaretXY(CharPos{4,0});
        mEdit->moveCaretVert(-1,false);
        QTest::newRow("backward at first line")<<mEdit->caretXY()<<CharPos{4,0};
    }
    {
        mEdit->setCaretXY(CharPos{4,15});
        mEdit->moveCaretVert(-1,false);
        QTest::newRow("normal backward")<<mEdit->caretXY()<<CharPos{4,14};
    }
    {
        mEdit->setCaretXY(CharPos{10,15});
        mEdit->moveCaretVert(-1,false);
        QTest::newRow("backward but prev line not long enough")<<mEdit->caretXY()<<CharPos{7,14};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mEdit->moveCaretVert(1,false);
        QTest::newRow("forward at file end")<<mEdit->caretXY()<<CharPos{1,76};
    }
    {
        mEdit->setCaretXY(CharPos{0,76});
        mEdit->moveCaretVert(1,false);
        QTest::newRow("forward at lastline")<<mEdit->caretXY()<<CharPos{0,76};
    }
    {
        mEdit->setCaretXY(CharPos{26,61});
        mEdit->moveCaretVert(1,false);
        QTest::newRow("normal forward")<<mEdit->caretXY()<<CharPos{26,62};
    }
    {
        mEdit->setCaretXY(CharPos{45,61});
        mEdit->moveCaretVert(1,false);
        QTest::newRow("forward but next line not long enough")<<mEdit->caretXY()<<CharPos{37,62};
    }
}

void TestQSyneditCpp::test_move_caret_y()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_previous_word_begin_data()
{
    init();
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");

    QTest::newRow("at file start")<<mEdit->prevWordBegin(CharPos{0,0})<<CharPos{0,0};
    QTest::newRow("line end")<<mEdit->prevWordBegin(CharPos{16,10})<<CharPos{13,10};
    QTest::newRow("word start")<<mEdit->prevWordBegin(CharPos{13,10})<<CharPos{8,10};
    QTest::newRow("word end")<<mEdit->prevWordBegin(CharPos{12,10})<<CharPos{8,10};
    QTest::newRow("word mid")<<mEdit->prevWordBegin(CharPos{11,10})<<CharPos{8,10};
    QTest::newRow("end of first word in line")<<mEdit->prevWordBegin(CharPos{8,10})<<CharPos{0,10};
    QTest::newRow("line start")<<mEdit->prevWordBegin(CharPos{0,10})<<CharPos{16,8};
    QTest::newRow("in space")<<mEdit->prevWordBegin(CharPos{4,19})<<CharPos{39,18};
}

void TestQSyneditCpp::test_previous_word_begin()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_next_word_begin_data()
{
    init();
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");

    QTest::newRow("file end")<<mEdit->nextWordBegin(CharPos{100,100})<<CharPos{1,76};
    QTest::newRow("file start")<<mEdit->nextWordBegin(CharPos{0,0})<<CharPos{9,0};
    QTest::newRow("line end")<<mEdit->nextWordBegin(CharPos{17,8})<<CharPos{0,10};
    QTest::newRow("line start")<<mEdit->nextWordBegin(CharPos{0,10})<<CharPos{8,10};
    QTest::newRow("word start")<<mEdit->nextWordBegin(CharPos{8,10})<<CharPos{13,10};
    QTest::newRow("word mid")<<mEdit->nextWordBegin(CharPos{10,10})<<CharPos{13,10};
    QTest::newRow("word end")<<mEdit->nextWordBegin(CharPos{12,10})<<CharPos{13,10};
    QTest::newRow("in space")<<mEdit->nextWordBegin(CharPos{4,19})<<CharPos{16,19};
}

void TestQSyneditCpp::test_next_word_begin()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_prev_word_end_data()
{
    init();
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");

    QTest::newRow("at file start")<<mEdit->prevWordEnd(CharPos{0,0})<<CharPos{0,0};
    QTest::newRow("at line end")<<mEdit->prevWordEnd(CharPos{16,10})<<CharPos{12,10};
    QTest::newRow("word mid")<<mEdit->prevWordEnd(CharPos{15,10})<<CharPos{12,10};
    QTest::newRow("word begin")<<mEdit->prevWordEnd(CharPos{13,10})<<CharPos{12,10};
    QTest::newRow("word end")<<mEdit->prevWordEnd(CharPos{12,10})<<CharPos{7,10};
    QTest::newRow("in space")<<mEdit->prevWordEnd(CharPos{4,19})<<CharPos{40,18};
}

void TestQSyneditCpp::test_prev_word_end()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

}

