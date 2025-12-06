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

void TestQSyneditCpp::initEdit()
{
    mEdit = std::make_shared<QSynEdit>();
    QSynedit::EditorOptions options = QSynedit::EditorOption::AltSetsColumnMode
            | QSynedit::EditorOption::DragDropEditing | QSynedit::EditorOption::DropFiles
            | QSynedit::EditorOption::EnhanceHomeKey | QSynedit::EditorOption::EnhanceEndKey
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
    initEdit();
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
    initEdit();
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
    initEdit();
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
    initEdit();
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
    initEdit();
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

void TestQSyneditCpp::test_move_caret_to_line_start_data()
{
    initEdit();
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    {
        mEdit->setCaretXY(CharPos{0,15});
        mEdit->moveCaretToLineStart(false);
        QTest::newRow("begin of line")<<mEdit->caretXY()<<CharPos{4,15};
    }
    {
        mEdit->setCaretXY(CharPos{2,15});
        mEdit->moveCaretToLineStart(false);
        QTest::newRow("in leading spaces")<<mEdit->caretXY()<<CharPos{0,15};
    }
    {
        mEdit->setCaretXY(CharPos{4,15});
        mEdit->moveCaretToLineStart(false);
        QTest::newRow("end of leading spaces")<<mEdit->caretXY()<<CharPos{0,15};
    }
    {
        mEdit->setCaretXY(CharPos{3,15});
        mEdit->moveCaretToLineStart(false);
        QTest::newRow("end of leading spaces - 1")<<mEdit->caretXY()<<CharPos{0,15};
    }
    {
        mEdit->setCaretXY(CharPos{5,15});
        mEdit->moveCaretToLineStart(false);
        QTest::newRow("end of leading spaces + 1")<<mEdit->caretXY()<<CharPos{4,15};
    }
    {
        mEdit->setCaretXY(CharPos{10,15});
        mEdit->moveCaretToLineStart(false);
        QTest::newRow("mid of line")<<mEdit->caretXY()<<CharPos{4,15};
    }
}

void TestQSyneditCpp::test_move_caret_to_line_start()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_move_caret_to_line_end_data()
{
    initEdit();
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    {
        mEdit->setCaretXY(CharPos{25,70});
        mEdit->moveCaretToLineEnd(false);
        QTest::newRow("start of trailing spaces")<<mEdit->caretXY()<<CharPos{34,70};
    }
    {
        mEdit->setCaretXY(CharPos{26,70});
        mEdit->moveCaretToLineEnd(false);
        QTest::newRow("start of trailing spaces + 1")<<mEdit->caretXY()<<CharPos{34,70};
    }
    {
        mEdit->setCaretXY(CharPos{24,70});
        mEdit->moveCaretToLineEnd(false);
        QTest::newRow("start of trailing spaces - 1")<<mEdit->caretXY()<<CharPos{25,70};
    }
    {
        mEdit->setCaretXY(CharPos{29,70});
        mEdit->moveCaretToLineEnd(false);
        QTest::newRow("middle of trailing spaces")<<mEdit->caretXY()<<CharPos{34,70};
    }
    {
        mEdit->setCaretXY(CharPos{15,70});
        mEdit->moveCaretToLineEnd(false);
        QTest::newRow("mid of line")<<mEdit->caretXY()<<CharPos{25,70};
    }
    {
        mEdit->setCaretXY(CharPos{34,70});
        mEdit->moveCaretToLineEnd(false);
        QTest::newRow("end of line")<<mEdit->caretXY()<<CharPos{25,70};
    }
}

void TestQSyneditCpp::test_move_caret_to_line_end()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_previous_word_begin_data()
{
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
    QTest::newRow("start of word after symbol")<<mEdit->prevWordBegin(CharPos{25,21})<<CharPos{23,21};
    QTest::newRow("mid of word after symbol")<<mEdit->prevWordBegin(CharPos{26,21})<<CharPos{25,21};
    QTest::newRow("start of symbol after word")<<mEdit->prevWordBegin(CharPos{58,21})<<CharPos{54,21};
    QTest::newRow("mid of symbol after word")<<mEdit->prevWordBegin(CharPos{59,21})<<CharPos{58,21};
}

void TestQSyneditCpp::test_previous_word_begin()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_next_word_begin_data()
{
    initEdit();
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
    QTest::newRow("start of word before symbol")<<mEdit->nextWordBegin(CharPos{37,21})<<CharPos{40,21};
    QTest::newRow("mid of word before symbol")<<mEdit->nextWordBegin(CharPos{38,21})<<CharPos{40,21};
    QTest::newRow("start of symbol before word")<<mEdit->nextWordBegin(CharPos{40,21})<<CharPos{42,21};
    QTest::newRow("mid of symbol before word")<<mEdit->nextWordBegin(CharPos{41,21})<<CharPos{42,21};
}

void TestQSyneditCpp::test_next_word_begin()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_prev_word_end_data()
{
    initEdit();
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

void TestQSyneditCpp::test_enter_chars_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");
    {
        initEdit();
        mEdit->clearUndo();
        mEdit->setCaretXY(CharPos{0,0});
        QTest::keyPress(mEdit.get(),Qt::Key_A);
        QTest::keyPress(mEdit.get(),Qt::Key_B);
        QTest::keyPress(mEdit.get(),Qt::Key_C);
        QTestData& td = QTest::newRow("file begin")<<mEdit->lineText(0)<<"abc#include <iostream>";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"abc#include <iostream>";
        QVERIFY(mEdit->canUndo());
    }
    {
        initEdit();
        mEdit->clearUndo();
        mEdit->setCaretXY(CharPos{2,1});
        QTest::keyPress(mEdit.get(),Qt::Key_A);
        QTest::keyPress(mEdit.get(),Qt::Key_B);
        QTest::keyPress(mEdit.get(),Qt::Key_C);
        QTestData& td = QTest::newRow("mid of line")<<mEdit->lineText(1)<<"#iabcnclude <mutex>";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(1)<<"#iabcnclude <mutex>";
        QVERIFY(mEdit->canUndo());
    }
    {
        initEdit();
        mEdit->clearUndo();
        mEdit->setCaretXY(CharPos{1,76});
        QTest::keyPress(mEdit.get(),Qt::Key_A);
        QTest::keyPress(mEdit.get(),Qt::Key_B);
        QTest::keyPress(mEdit.get(),Qt::Key_C);
        QTestData& td = QTest::newRow("endoffile")<<mEdit->lineText(76)<<"}abc";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(76)<<"}abc";
        QVERIFY(mEdit->canUndo());
    }
}

void TestQSyneditCpp::test_enter_chars()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
}

}

