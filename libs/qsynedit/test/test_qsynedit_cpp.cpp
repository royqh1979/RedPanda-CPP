#include <QTest>
#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>

#include "test_qsynedit_cpp.h"
#include "test_utils.h"
#include "qsynedit/qsynedit.h"
#include "qsynedit/document.h"
#include "qsynedit/syntaxer/cpp.h"
#include "qsynedit/formatter/cppformatter.h"

namespace QSynedit {


void TestQSyneditCpp::initTestCase()
{
    mEdit = std::make_shared<QSynEdit>();
    QSynedit::EditorOptions options = QSynedit::EditorOption::AltSetsColumnMode
            | QSynedit::EditorOption::DragDropEditing | QSynedit::EditorOption::DropFiles
            | QSynedit::EditorOption::EnhanceHomeKey | QSynedit::EditorOption::EnhanceEndKey
            | QSynedit::EditorOption::RightMouseMovesCursor
            | QSynedit::EditorOption::TabIndent
            | QSynedit::EditorOption::GroupUndo
            | QSynedit::EditorOption::AutoIndent
            | QSynedit::EditorOption::SelectWordByDblClick;
    mDefaultOptions = options;
    mEdit->setOptions(options);
    mEdit->setSyntaxer(std::make_shared<CppSyntaxer>());
    mEdit->setFormatter(std::make_shared<CppFormatter>());

    QFont defaultFont;
    defaultFont.setFamily(defaultMonoFont());
    defaultFont.setPointSize(14);
    mEdit->setFont(defaultFont);

    connectEditSignals();
}

void TestQSyneditCpp::cleanup()
{
    mEdit->setOptions(mDefaultOptions);
    mEdit->setInsertMode(true);
    mEdit->setReadOnly(false);
    mEdit->setActiveSelectionMode(SelectionMode::Normal);
}

void TestQSyneditCpp::loadDemoFile()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    clearSignalDatas();
}

void TestQSyneditCpp::test_next_char()
{

}

void TestQSyneditCpp::test_get_token_data()
{
    QTest::addColumn<QString>("word");
    QTest::addColumn<QString>("expect");

    clearContent();
    QTest::newRow("empty document 1")<<mEdit->tokenAt(CharPos{0,0})<<"";

    loadDemoFile();
    QTest::newRow("document begin")<<mEdit->tokenAt(CharPos{0,0})<<"#include";
    QTest::newRow("after document begin")<<mEdit->tokenAt(CharPos{1,0})<<"#include";
    QTest::newRow("before document end")<<mEdit->tokenAt(CharPos{0,76})<<"}";
    QTest::newRow("document end")<<mEdit->tokenAt(CharPos{1,76})<<"";
    QTest::newRow("line begin")<<mEdit->tokenAt(CharPos{0,10})<<"#define";
    QTest::newRow("before word")<<mEdit->tokenAt(CharPos{5,12})<<" ";
    QTest::newRow("word begin")<<mEdit->tokenAt(CharPos{6,12})<<"ThreadPool";
    QTest::newRow("word end")<<mEdit->tokenAt(CharPos{15,12})<<"ThreadPool";
    QTest::newRow("line end")<<mEdit->tokenAt(CharPos{16,12})<<"";
}

void TestQSyneditCpp::test_get_token()
{
    QFETCH(QString, word);
    QFETCH(QString, expect);

    QCOMPARE(word, expect);
}

void TestQSyneditCpp::test_token_begin_data()
{
    QTest::addColumn<CharPos>("begin");
    QTest::addColumn<CharPos>("expect");

    clearContent();
    QTest::newRow("empty doc")<<mEdit->getTokenBegin(CharPos{0,0})<<CharPos{0,0};

    loadDemoFile();
    QTest::newRow("doc begin")<<mEdit->getTokenBegin(CharPos{0,0})<<CharPos{0,0};
    QTest::newRow("after doc begin")<<mEdit->getTokenBegin(CharPos{1,0})<<CharPos{0,0};
    QTest::newRow("before doc end")<<mEdit->getTokenBegin(CharPos{0,76})<<CharPos{0,76};
    QTest::newRow("doc end")<<mEdit->getTokenBegin(CharPos{1,76})<<CharPos{1,76};

    QTest::newRow("line begin")<<mEdit->getTokenBegin(CharPos{0,10})<<CharPos{0,10};
    QTest::newRow("after line begin")<<mEdit->getTokenBegin(CharPos{1,10})<<CharPos{0,10};
    QTest::newRow("before line end")<<mEdit->getTokenBegin(CharPos{15,10})<<CharPos{13,10};
    QTest::newRow("line end")<<mEdit->getTokenBegin(CharPos{16,10})<<CharPos{16,10};
    QTest::newRow("before word begin")<<mEdit->getTokenBegin(CharPos{3,15})<<CharPos{0,15};
    QTest::newRow("at word begin")<<mEdit->getTokenBegin(CharPos{4,15})<<CharPos{4,15};
    QTest::newRow("in word")<<mEdit->getTokenBegin(CharPos{10,15})<<CharPos{4,15};
    QTest::newRow("before word end")<<mEdit->getTokenBegin(CharPos{13,15})<<CharPos{4,15};
    QTest::newRow("at word end")<<mEdit->getTokenBegin(CharPos{14,15})<<CharPos{14,15};
}

void TestQSyneditCpp::test_token_begin()
{
    QFETCH(CharPos, begin);
    QFETCH(CharPos, expect);
    QCOMPARE(begin, expect);
}

void TestQSyneditCpp::test_token_end_data()
{
    QTest::addColumn<CharPos>("end");
    QTest::addColumn<CharPos>("expect");

    clearContent();
    QTest::newRow("empty doc")<<mEdit->getTokenEnd(CharPos{0,0})<<CharPos{0,0};

    loadDemoFile();
    QTest::newRow("doc begin")<<mEdit->getTokenEnd(CharPos{0,0})<<CharPos{8,0};
    QTest::newRow("after doc begin")<<mEdit->getTokenEnd(CharPos{1,0})<<CharPos{8,0};
    QTest::newRow("before doc end")<<mEdit->getTokenEnd(CharPos{0,76})<<CharPos{1,76};
    QTest::newRow("doc end")<<mEdit->getTokenEnd(CharPos{1,76})<<CharPos{1,76};
    QTest::newRow("line begin")<<mEdit->getTokenEnd(CharPos{0,10})<<CharPos{7,10};
    QTest::newRow("line end")<<mEdit->getTokenEnd(CharPos{16,10})<<CharPos{16,10};
    QTest::newRow("before word begin")<<mEdit->getTokenEnd(CharPos{3,15})<<CharPos{4,15};
    QTest::newRow("at word begin")<<mEdit->getTokenEnd(CharPos{4,15})<<CharPos{14,15};
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

void TestQSyneditCpp::test_previous_word_begin_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");

    clearContent();
    QTest::newRow("empty doc")<<mEdit->prevWordBegin(CharPos{0,0})<<CharPos{0,0};

    loadDemoFile();
    QTest::newRow("doc begin")<<mEdit->prevWordBegin(CharPos{0,0})<<CharPos{0,0};
    QTest::newRow("after doc begin")<<mEdit->prevWordBegin(CharPos{1,0})<<CharPos{0,0};
    QTest::newRow("before doc end")<<mEdit->prevWordBegin(CharPos{0,76})<<CharPos{4,75};
    QTest::newRow("doc end")<<mEdit->prevWordBegin(CharPos{1,76})<<CharPos{0,76};
    QTest::newRow("line begin")<<mEdit->prevWordBegin(CharPos{0,10})<<CharPos{16,8};
    QTest::newRow("word begin")<<mEdit->prevWordBegin(CharPos{13,10})<<CharPos{8,10};
    QTest::newRow("word end")<<mEdit->prevWordBegin(CharPos{12,10})<<CharPos{8,10};
    QTest::newRow("word mid")<<mEdit->prevWordBegin(CharPos{11,10})<<CharPos{8,10};
    QTest::newRow("end of first word in line")<<mEdit->prevWordBegin(CharPos{8,10})<<CharPos{0,10};
    QTest::newRow("line end")<<mEdit->prevWordBegin(CharPos{16,10})<<CharPos{13,10};
    QTest::newRow("in space")<<mEdit->prevWordBegin(CharPos{4,19})<<CharPos{39,18};
    QTest::newRow("begin of word after symbol")<<mEdit->prevWordBegin(CharPos{25,21})<<CharPos{23,21};
    QTest::newRow("mid of word after symbol")<<mEdit->prevWordBegin(CharPos{26,21})<<CharPos{25,21};
    QTest::newRow("begin of symbol after word")<<mEdit->prevWordBegin(CharPos{58,21})<<CharPos{54,21};
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
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");

    clearContent();
    QTest::newRow("empty doc")<<mEdit->nextWordBegin(CharPos{0,0})<<CharPos{0,0};

    loadDemoFile();
    QTest::newRow("doc begin")<<mEdit->nextWordBegin(CharPos{0,0})<<CharPos{9,0};
    QTest::newRow("doc end")<<mEdit->nextWordBegin(CharPos{1,76})<<CharPos{1,76};
    QTest::newRow("line begin")<<mEdit->nextWordBegin(CharPos{0,10})<<CharPos{8,10};
    QTest::newRow("line end")<<mEdit->nextWordBegin(CharPos{17,8})<<CharPos{0,10};
    QTest::newRow("word begin")<<mEdit->nextWordBegin(CharPos{8,10})<<CharPos{13,10};
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

void TestQSyneditCpp::test_next_word_end_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");

    clearContent();
    QTest::newRow("empty doc")<<mEdit->nextWordEnd(CharPos{0,0})<<CharPos{0,0};

    loadDemoFile();
    QTest::newRow("doc begin")<<mEdit->nextWordEnd(CharPos{0,0})<<CharPos{8,0};
    QTest::newRow("doc end")<<mEdit->nextWordEnd(CharPos{1,76})<<CharPos{1,76};
    QTest::newRow("line begin")<<mEdit->nextWordEnd(CharPos{0,10})<<CharPos{7,10};
    QTest::newRow("line end")<<mEdit->nextWordEnd(CharPos{17,8})<<CharPos{7,10};
    QTest::newRow("word start")<<mEdit->nextWordEnd(CharPos{8,10})<<CharPos{12,10};
    QTest::newRow("word mid")<<mEdit->nextWordEnd(CharPos{10,10})<<CharPos{12,10};
    QTest::newRow("word end")<<mEdit->nextWordEnd(CharPos{12,10})<<CharPos{16,10};
    QTest::newRow("in space")<<mEdit->nextWordEnd(CharPos{4,19})<<CharPos{21,19};
    QTest::newRow("start of word before symbol")<<mEdit->nextWordEnd(CharPos{37,21})<<CharPos{40,21};
    QTest::newRow("mid of word before symbol")<<mEdit->nextWordEnd(CharPos{38,21})<<CharPos{40,21};
    QTest::newRow("start of symbol before word")<<mEdit->nextWordEnd(CharPos{40,21})<<CharPos{42,21};
    QTest::newRow("mid of symbol before word")<<mEdit->nextWordEnd(CharPos{41,21})<<CharPos{42,21};
}

void TestQSyneditCpp::test_next_word_end()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_prev_word_end_data()
{
    loadDemoFile();
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");

    clearContent();
    QTest::newRow("empty doc")<<mEdit->prevWordEnd(CharPos{0,0})<<CharPos{0,0};

    loadDemoFile();
    QTest::newRow("doc begin")<<mEdit->prevWordEnd(CharPos{0,0})<<CharPos{0,0};
    QTest::newRow("doc end")<<mEdit->prevWordEnd(CharPos{1,76})<<CharPos{5,75};
    QTest::newRow("line begin")<<mEdit->prevWordEnd(CharPos{0,10})<<CharPos{17,8};
    QTest::newRow("line end")<<mEdit->prevWordEnd(CharPos{16,10})<<CharPos{12,10};
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

void TestQSyneditCpp::test_move_caret_x_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    QTest::addColumn<QList<int>>("statusChanges");
    QTest::addColumn<QList<int>>("expect_statusChange");

    clearContent();
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward in empty doc")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward in empty doc")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }

    loadDemoFile();
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward at file start")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward at file end")<<mEdit->caretPos()<<CharPos{1,76};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward at line start")<<mEdit->caretPos()<<CharPos{7,14};
        td << mStatusChanges << QList<int>{(StatusChange::CaretX | StatusChange::CaretY)};
    }
    {
        mEdit->setCaretPos(CharPos{0,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward at line start")<<mEdit->caretPos()<<CharPos{1,15};
        td << mStatusChanges << QList<int>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{40,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward at line end")<<mEdit->caretPos()<<CharPos{39,15};
        td << mStatusChanges << QList<int>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{40,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward at line end")<<mEdit->caretPos()<<CharPos{0,16};
        td << mStatusChanges << QList<int>{StatusChange::CaretX | StatusChange::CaretY};
    }
}

void TestQSyneditCpp::test_move_caret_x()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<int>, statusChanges);
    QFETCH(QList<int>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);
}

void TestQSyneditCpp::test_move_caret_y_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    QTest::addColumn<QList<int>>("statusChanges");
    QTest::addColumn<QList<int>>("expect_statusChange");

    clearContent();
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Up in empty doc")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down in empty doc")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp);
        QTestData& td = QTest::newRow("Page Up in empty doc")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageDown);
        QTestData& td = QTest::newRow("Page Down in empty doc")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }

    loadDemoFile();
    //up/down
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Up at file start")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{4,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Up at first line")<<mEdit->caretPos()<<CharPos{4,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{4,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::SelUp);
        QTestData& td = QTest::newRow("Up at first line with selection")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{StatusChange::CaretX | StatusChange::Selection};
    }
    {
        mEdit->setCaretPos(CharPos{4,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Normal Up")<<mEdit->caretPos()<<CharPos{4,14};
        td << mStatusChanges << QList<int>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretPos(CharPos{10,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Up but prev line not long enough")<<mEdit->caretPos()<<CharPos{7,14};
        td << mStatusChanges << QList<int>{StatusChange::CaretX | StatusChange::CaretY};
    }
    {
        mEdit->setCaretPos(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down at file end")<<mEdit->caretPos()<<CharPos{1,76};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down at lastline")<<mEdit->caretPos()<<CharPos{0,76};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::SelDown);
        QTestData& td = QTest::newRow("Down at lastline with selection")<<mEdit->caretPos()<<CharPos{1,76};
        td << mStatusChanges << QList<int>{StatusChange::CaretX | StatusChange::Selection};
    }
    {
        mEdit->setCaretPos(CharPos{26,61});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Normal Down")<<mEdit->caretPos()<<CharPos{26,62};
        td << mStatusChanges << QList<int>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretPos(CharPos{45,61});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down but next line not long enough")<<mEdit->caretPos()<<CharPos{37,62};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX | StatusChange::CaretY };
    }
    // page up/down
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp);
        QTestData& td = QTest::newRow("Page Up at file start")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{4,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp);
        QTestData& td = QTest::newRow("Page Up at first line")<<mEdit->caretPos()<<CharPos{4,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{4,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::SelPageUp);
        QTestData& td = QTest::newRow("Page Up at first line with selection")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{StatusChange::CaretX | StatusChange::Selection};
    }
    {
        mEdit->setCaretPos(CharPos{0,75});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp);
        CharPos newPos=CharPos{0,75-mEdit->linesInWindow()};
        if (newPos.line<0)
            newPos.line = 0;
        QTestData& td = QTest::newRow("Normal Page Up")<<mEdit->caretPos()<<newPos;
        td << mStatusChanges << QList<int>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretPos(CharPos{0,1});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageDown);
        CharPos newPos=CharPos{0,1+mEdit->linesInWindow()};
        if (newPos.line > 76)
            newPos.line = 76;
        QTestData& td = QTest::newRow("Normal Page Down")<<mEdit->caretPos()<<newPos;
        td << mStatusChanges << QList<int>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretPos(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageDown);
        QTestData& td = QTest::newRow("Page Down at file end")<<mEdit->caretPos()<<CharPos{1,76};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Page Down at lastline")<<mEdit->caretPos()<<CharPos{0,76};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::SelDown);
        QTestData& td = QTest::newRow("Page Down at lastline with selection")<<mEdit->caretPos()<<CharPos{1,76};
        td << mStatusChanges << QList<int>{StatusChange::CaretX | StatusChange::Selection};
    }
}

void TestQSyneditCpp::test_move_caret_y()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<int>, statusChanges);
    QFETCH(QList<int>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);

}

void TestQSyneditCpp::test_move_caret_to_line_begin_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    QTest::addColumn<QList<int>>("statusChanges");
    QTest::addColumn<QList<int>>("expect_statusChange");

    loadDemoFile();
    {
        mEdit->setCaretPos(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("doc begin")<<mEdit->caretPos()<<CharPos{0,0};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("doc end")<<mEdit->caretPos()<<CharPos{0,76};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX };
    }
    {
        mEdit->setCaretPos(CharPos{0,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("begin of line")<<mEdit->caretPos()<<CharPos{4,15};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{2,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("in leading spaces")<<mEdit->caretPos()<<CharPos{0,15};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{4,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("end of leading spaces")<<mEdit->caretPos()<<CharPos{0,15};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{3,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("end of leading spaces - 1")<<mEdit->caretPos()<<CharPos{0,15};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{5,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("end of leading spaces + 1")<<mEdit->caretPos()<<CharPos{4,15};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{10,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("mid of line")<<mEdit->caretPos()<<CharPos{4,15};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
}

void TestQSyneditCpp::test_move_caret_to_line_begin()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<int>, statusChanges);
    QFETCH(QList<int>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);
}

void TestQSyneditCpp::test_move_caret_to_line_end_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    QTest::addColumn<QList<int>>("statusChanges");
    QTest::addColumn<QList<int>>("expect_statusChange");

    loadDemoFile();
    {
        mEdit->setCaretPos(CharPos{25,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("start of trailing spaces")<<mEdit->caretPos()<<CharPos{34,70};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{26,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("start of trailing spaces + 1")<<mEdit->caretPos()<<CharPos{34,70};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{24,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("start of trailing spaces - 1")<<mEdit->caretPos()<<CharPos{25,70};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{29,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("middle of trailing spaces")<<mEdit->caretPos()<<CharPos{34,70};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{15,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("mid of line")<<mEdit->caretPos()<<CharPos{25,70};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{34,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("end of line")<<mEdit->caretPos()<<CharPos{25,70};
        td << mStatusChanges << QList<int>{ StatusChange::CaretX};
    }
}

void TestQSyneditCpp::test_move_caret_to_line_end()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<int>, statusChanges);
    QFETCH(QList<int>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);
}

void TestQSyneditCpp::test_match_brackets_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    loadDemoFile();
    {
        QTest::addRow("at bracket open 1")<<mEdit->getMatchingBracket({0,13})<<CharPos{0,63};
    }
    {
        QTest::addRow("at bracket close 1")<<mEdit->getMatchingBracket({0,63})<<CharPos{0,13};
    }
    {
        QTest::addRow("at parenthesis open 1")<<mEdit->getMatchingBracket({29,22})<<CharPos{21,24};
    }
    {
        QTest::addRow("at parenthesis close 1")<<mEdit->getMatchingBracket({21,24})<<CharPos{29,22};
    }
    {
        QTest::addRow("at parenthesis open 2")<<mEdit->getMatchingBracket({39,72})<<CharPos{63,72};
    }
    {
        QTest::addRow("at parenthesis close 2")<<mEdit->getMatchingBracket({63,72})<<CharPos{39,72};
    }
}

void TestQSyneditCpp::test_match_brackets()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_get_block_start_end_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    loadDemoFile();
    {
        CharPos blockStart,blockEnd;
        QVERIFY(mEdit->getContainingBlockStart({12,18},blockStart));
        QVERIFY(mEdit->getContainingBlockEnd({12,18},blockEnd));

        QTest::addRow("Block Start 1")<<blockStart<<CharPos{42,17};
        QTest::addRow("Block End 1")<<blockEnd<<CharPos{8,35};
    }
    {
        CharPos blockStart,blockEnd;
        QVERIFY(mEdit->getContainingBlockStart({14,34},blockStart));
        QVERIFY(mEdit->getContainingBlockEnd({14,34},blockEnd));

        QTest::addRow("Block Start 2")<<blockStart<<CharPos{42,17};
        QTest::addRow("Block End 2")<<blockEnd<<CharPos{8,35};
    }
}

void TestQSyneditCpp::test_get_block_start_end()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QCOMPARE(pos, expect);
}

void TestQSyneditCpp::test_select_data()
{
    QTest::addColumn<CharPos>("selBegin");
    QTest::addColumn<CharPos>("expect_selBegin");
    QTest::addColumn<CharPos>("selEnd");
    QTest::addColumn<CharPos>("expect_selEnd");
    QTest::addColumn<QString>("selText");
    QTest::addColumn<QString>("expect_selText");
    QTest::addColumn<int>("selCount");
    QTest::addColumn<int>("expect_selCount");
    QTest::addColumn<QList<int>>("statusChanges");
    QTest::addColumn<QList<int>>("expect_statusChanges");
    {
        clearContent();
        mEdit->clearSelection();
        QTestData& td = QTest::addRow("empty doc 1")<<mEdit->selBegin()<<CharPos{0,0};
        td<<mEdit->selEnd()<<CharPos{0,0};
        td<<mEdit->selText()<<"";
        td<<mEdit->selCount()<<0;
        td<<mStatusChanges<<QList<int>{};
    }
    {
        clearContent();
        mEdit->setSelBeginEnd(mEdit->fileBegin(),mEdit->fileBegin());
        QTestData& td = QTest::addRow("empty doc 2")<<mEdit->selBegin()<<CharPos{0,0};
        td<<mEdit->selEnd()<<CharPos{0,0};
        td<<mEdit->selText()<<"";
        td<<mEdit->selCount()<<0;
        td<<mStatusChanges<<QList<int>{};
    }
    {
        loadDemoFile();
        mEdit->setCaretAndSelection(CharPos{5,0},CharPos{0,0}, CharPos{5,0});
        QTestData& td = QTest::addRow("select in line")<<mEdit->selBegin()<<CharPos{0,0};
        td<<mEdit->selEnd()<<CharPos{5,0};
        td<<mEdit->selText()<<"#incl";
        td<<mEdit->selCount()<<5;
        td<<mStatusChanges<<QList<int>{StatusChange::CaretX | StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setCaretAndSelection(CharPos{16,1},CharPos{0,1}, CharPos{16,1});
        QTestData& td = QTest::addRow("select from line begin to end")<<mEdit->selBegin()<<CharPos{0,1};
        td<<mEdit->selEnd()<<CharPos{16,1};
        td<<mEdit->selText()<<"#include <mutex>";
        td<<mEdit->selCount()<<16;
        td<<mStatusChanges<<QList<int>{StatusChange::CaretX | StatusChange::CaretY | StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setCaretAndSelection(CharPos{0,2},CharPos{0,1}, CharPos{0,2});
        QTestData& td = QTest::addRow("select one whole line")<<mEdit->selBegin()<<CharPos{0,1};
        td<<mEdit->selEnd()<<CharPos{0,2};
        td<<mEdit->selText()<<"#include <mutex>" + mEdit->lineBreak();
        td<<mEdit->selCount()<<16+mEdit->lineBreak().length();
        td<<mStatusChanges<<QList<int>{ StatusChange::CaretY | StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setCaretAndSelection(CharPos{11,1},CharPos{4,0}, CharPos{11,1});
        QTestData& td = QTest::addRow("select two lines")<<mEdit->selBegin()<<CharPos{4,0};
        td<<mEdit->selEnd()<<CharPos{11,1};
        td<<mEdit->selText()<<"lude <iostream>" + mEdit->lineBreak() + "#include <m";
        td<<mEdit->selCount()<<15+mEdit->lineBreak().length()+11;
        td<<mStatusChanges<<QList<int>{StatusChange::CaretX | StatusChange::CaretY | StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->selectAll();
        QTestData& td = QTest::addRow("select whole doc")<<mEdit->selBegin()<<mEdit->fileBegin();
        td<<mEdit->selEnd()<<mEdit->fileEnd();
        td<<mEdit->selText()<<mEdit->text();
        td<<mEdit->selCount()<<mEdit->text().length();
        td<<mStatusChanges<<QList<int>{ StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setCaretPos(mEdit->fileBegin());
        mEdit->setSelBeginEnd(CharPos{0,0}, CharPos{0,5});
        mStatusChanges.clear();
        mEdit->clearSelection();
        QTestData& td = QTest::addRow("clear selection")<<mEdit->selBegin()<<mEdit->caretPos();
        td<<mEdit->selEnd()<<mEdit->caretPos();
        td<<mEdit->selText()<<"";
        td<<mEdit->selCount()<<0;
        td<<mStatusChanges<<QList<int>{StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setSelBeginEnd(CharPos{0,0}, CharPos{0,5});
        mStatusChanges.clear();
        mEdit->setCaretPos(CharPos{0,10});
        QTestData& td = QTest::addRow("set caret would clear selection")<<mEdit->selBegin()<<CharPos{0,10};
        td<<mEdit->selEnd()<<CharPos{0,10};
        td<<mEdit->selText()<<"";
        td<<mEdit->selCount()<<0;
        td<<mStatusChanges<<QList<int>{StatusChange::Selection | StatusChange::CaretY};
    }
}

void TestQSyneditCpp::test_select()
{
    QFETCH(CharPos, selBegin);
    QFETCH(CharPos, expect_selBegin);
    QFETCH(CharPos, selEnd);
    QFETCH(CharPos, expect_selEnd);
    QFETCH(QString, selText);
    QFETCH(QString, expect_selText);
    QFETCH(int, selCount);
    QFETCH(int, expect_selCount);
    QFETCH(QList<int>, statusChanges);
    QFETCH(QList<int>, expect_statusChanges);
    QCOMPARE(selBegin, expect_selBegin);
    QCOMPARE(selEnd, expect_selEnd);
    QCOMPARE(selText, expect_selText);
    QCOMPARE(selCount, expect_selCount);
    QCOMPARE(statusChanges, expect_statusChanges);
}

void TestQSyneditCpp::test_clear()
{
    clearContent();
    QTest::keyPress(mEdit.get(), Qt::Key_A);
    QTest::keyPress(mEdit.get(), Qt::Key_Enter);
    QTest::keyPress(mEdit.get(), Qt::Key_B);
    QCOMPARE(mEdit->content(),QStringList({"a","b"}));
    mEdit->undo();
    QVERIFY(mEdit->canUndo());
    QVERIFY(mEdit->canRedo());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->processCommand(EditCommand::ClearAll);
    // clear deletes all lines but the last, which becomes the empty line
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{0});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mReparseStarts, QList<int>{0});
    QCOMPARE(mReparseCounts, QList<int>{1});
    QCOMPARE(mStatusChanges, QList<int>{StatusChange::ModifyChanged | StatusChange::CaretY});
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QVERIFY(mEdit->empty());
}

void TestQSyneditCpp::test_load_file()
{
    clearContent();
    QTest::keyPress(mEdit.get(), Qt::Key_A);
    QTest::keyPress(mEdit.get(), Qt::Key_Enter);
    QTest::keyPress(mEdit.get(), Qt::Key_B);
    QCOMPARE(mEdit->content(),QStringList({"a","b"}));
    mEdit->undo();
    QVERIFY(mEdit->canUndo());
    QVERIFY(mEdit->canRedo());
    QVERIFY(mEdit->modified());

    QCOMPARE(mEdit->codeBlockCount(), 0);

    clearSignalDatas();
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);
    QCOMPARE(mInsertStartLines, QList<int>({0}));
    QCOMPARE(mInsertLineCounts, QList<int>({77}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({2}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({77}));
    QCOMPARE(mStatusChanges, QList<int>{ StatusChange::ModifyChanged | StatusChange::CaretY});
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QVERIFY(!mEdit->empty());

    QCOMPARE(mEdit->codeBlockCount(), 11);
    QVERIFY(mEdit->hasCodeBlock(13, 63));
    QCOMPARE(mEdit->subBlockCounts(13,63),3);
    QVERIFY(mEdit->hasCodeBlock(16, 36));
    QCOMPARE(mEdit->subBlockCounts(16,36),1);
    QVERIFY(mEdit->hasCodeBlock(17, 35));
    QCOMPARE(mEdit->subBlockCounts(17,35),1);
    QVERIFY(mEdit->hasCodeBlock(18, 34));
    QCOMPARE(mEdit->subBlockCounts(18,34),1);
    QVERIFY(mEdit->hasCodeBlock(20, 33));
    QCOMPARE(mEdit->subBlockCounts(20,33),1);
    QVERIFY(mEdit->hasCodeBlock(22, 24));
    QCOMPARE(mEdit->subBlockCounts(22,24),0);
    QVERIFY(mEdit->hasCodeBlock(39, 46));
    QCOMPARE(mEdit->subBlockCounts(39,46),0);
    QVERIFY(mEdit->hasCodeBlock(48, 55));
    QCOMPARE(mEdit->subBlockCounts(48,55),0);
    QVERIFY(mEdit->hasCodeBlock(66, 76));
    QCOMPARE(mEdit->subBlockCounts(66,76),1);
    QVERIFY(mEdit->hasCodeBlock(69, 75));
    QCOMPARE(mEdit->subBlockCounts(69,75),1);
    QVERIFY(mEdit->hasCodeBlock(70, 74));
    QCOMPARE(mEdit->subBlockCounts(70,74),0);
}

void TestQSyneditCpp::test_clear_all()
{
    clearContent();
    QTest::keyPress(mEdit.get(), Qt::Key_A);
    QTest::keyPress(mEdit.get(), Qt::Key_Enter);
    QTest::keyPress(mEdit.get(), Qt::Key_B);
    QCOMPARE(mEdit->content(),QStringList({"a","b"}));
    mEdit->undo();
    QVERIFY(mEdit->canUndo());
    QVERIFY(mEdit->canRedo());
    QVERIFY(mEdit->modified());

    QCOMPARE(mEdit->codeBlockCount(), 0);

    clearSignalDatas();
    mEdit->processCommand(EditCommand::ClearAll);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QCOMPARE(mEdit->codeBlockCount(),0);
    QCOMPARE(mEdit->lineCount(),1);
    QCOMPARE(mEdit->lineText(),"");
    QCOMPARE(mEdit->content(), QStringList{""});
    QVERIFY(mEdit->empty());
    QCOMPARE(mEdit->caretPos(),CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(),CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QVERIFY(!mEdit->selAvail());
    QCOMPARE(mEdit->selCount(),0);
    QCOMPARE(mEdit->selText(),QString{});
}

void TestQSyneditCpp::test_set_content_qstring()
{
    clearContent();
    QTest::keyPress(mEdit.get(), Qt::Key_A);
    QTest::keyPress(mEdit.get(), Qt::Key_Enter);
    QTest::keyPress(mEdit.get(), Qt::Key_B);
    QCOMPARE(mEdit->content(),QStringList({"a","b"}));
    mEdit->undo();
    QVERIFY(mEdit->canUndo());
    QVERIFY(mEdit->canRedo());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    QByteArray encoding;
    mEdit->setContent("123\nabc\n456");
    QCOMPARE(mEdit->content(),QStringList({"123","abc","456"}));
    QCOMPARE(mInsertStartLines, QList<int>({0}));
    QCOMPARE(mInsertLineCounts, QList<int>({3}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({2}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mStatusChanges, QList<int>{ StatusChange::ModifyChanged | StatusChange::CaretY});
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QVERIFY(!mEdit->empty());
}

void TestQSyneditCpp::test_set_content_qstringlist()
{
    clearContent();
    QTest::keyPress(mEdit.get(), Qt::Key_A);
    QTest::keyPress(mEdit.get(), Qt::Key_Enter);
    QTest::keyPress(mEdit.get(), Qt::Key_B);
    QCOMPARE(mEdit->content(),QStringList({"a","b"}));
    mEdit->undo();
    QVERIFY(mEdit->canUndo());
    QVERIFY(mEdit->canRedo());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->setContent({"123","456","789"});
    QCOMPARE(mEdit->content(),QStringList({"123","456","789"}));
    QCOMPARE(mInsertStartLines, QList<int>({0}));
    QCOMPARE(mInsertLineCounts, QList<int>({3}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({2}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mStatusChanges, QList<int>{ StatusChange::ModifyChanged | StatusChange::CaretY});
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QVERIFY(!mEdit->empty());
}

void QSynedit::TestQSyneditCpp::test_input_chars_in_empty_file()
{
    clearContent();
    QTest::keyPress(mEdit.get(),'a');
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    clearReparseDatas();
    QTest::keyPress(mEdit.get(),'b');
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QTest::keyPress(mEdit.get(),'c');
    QTest::keyPress(mEdit.get(),' ');
    QTest::keyPress(mEdit.get(),' ');
    QTest::keyPress(mEdit.get(),'a');
    QTest::keyPress(mEdit.get(),'b');
    QTest::keyPress(mEdit.get(),'c');
    QCOMPARE(mEdit->content(),QStringList{"abc  abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(8,0));
    QCOMPARE(mEdit->selBegin(),CharPos(8,0));
    QCOMPARE(mEdit->selEnd(),CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    //undo to end
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc  "});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QVERIFY(!mEdit->canUndo());
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->modified());

    //redo to end
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"abc  "});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QVERIFY(!mEdit->canRedo());
    QCOMPARE(mEdit->content(),QStringList{"abc  abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(8,0));
    QCOMPARE(mEdit->selBegin(),CharPos(8,0));
    QCOMPARE(mEdit->selEnd(),CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    //undo to end again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc  "});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QVERIFY(!mEdit->canUndo());
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_input_chars_in_empty_file_overwrite_mode()
{
    mEdit->setInsertMode(false);
    clearContent();
    QTest::keyPress(mEdit.get(),'a');
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    clearReparseDatas();
    QTest::keyPress(mEdit.get(),'b');
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QTest::keyPress(mEdit.get(),'c');
    QTest::keyPress(mEdit.get(),' ');
    QTest::keyPress(mEdit.get(),' ');
    QTest::keyPress(mEdit.get(),'a');
    QTest::keyPress(mEdit.get(),'b');
    QTest::keyPress(mEdit.get(),'c');
    QCOMPARE(mEdit->content(),QStringList{"abc  abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(8,0));
    QCOMPARE(mEdit->selBegin(),CharPos(8,0));
    QCOMPARE(mEdit->selEnd(),CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    //undo to end
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc  "});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QVERIFY(!mEdit->canUndo());
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->modified());

    //redo to end
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"abc  "});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QVERIFY(!mEdit->canRedo());
    QCOMPARE(mEdit->content(),QStringList{"abc  abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(8,0));
    QCOMPARE(mEdit->selBegin(),CharPos(8,0));
    QCOMPARE(mEdit->selEnd(),CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    //undo to end again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc  "});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc"});
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QVERIFY(!mEdit->canUndo());
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_input_char_when_select_all()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "#",
                      });

    mEdit->setContent(text1);
    mEdit->setCaretPos(mEdit->fileEnd());
    mEdit->selectAll();
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), '#');
    QCOMPARE(mEdit->content(),text2);
    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);

}

void TestQSyneditCpp::test_input_chars_at_file_begin_end()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "ab#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}cd"
                      });
    QStringList text3({
                          "ab#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    mEdit->setContent(text1);
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_A);
    QTest::keyPress(mEdit.get(), Qt::Key_B);
    mEdit->setCaretPos(mEdit->fileEnd());
    QTest::keyPress(mEdit.get(), Qt::Key_C);
    QTest::keyPress(mEdit.get(), Qt::Key_D);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(3,3));
    QCOMPARE(mEdit->selBegin(),CharPos(3,3));
    QCOMPARE(mEdit->selEnd(),CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::CaretY),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(2,0));
    QCOMPARE(mEdit->selBegin(),CharPos(2,0));
    QCOMPARE(mEdit->selEnd(),CharPos(2,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX  | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(3,3));
    QCOMPARE(mEdit->selBegin(),CharPos(3,3));
    QCOMPARE(mEdit->selEnd(),CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX  | StatusChange::Modified | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canRedo());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
}

void TestQSyneditCpp::test_input_chars_at_file_begin_end_overwrite_mode()
{
    mEdit->setInsertMode(false);
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "abnclude <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text3({
                          "abnclude <iostream>",
                          "int main() {",
                          " return 0;",
                          "cd"
                      });
    QStringList text4({
                          "abnclude <iostream>",
                          "int main() {",
                          " return 0   ",
                          "cd"
                      });
    mEdit->setContent(text1);
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_A);
    QTest::keyPress(mEdit.get(), Qt::Key_B);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,0));
    QCOMPARE(mEdit->selBegin(),CharPos(2,0));
    QCOMPARE(mEdit->selEnd(),CharPos(2,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    mEdit->setCaretPos(CharPos{0,3});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_C);
    QTest::keyPress(mEdit.get(), Qt::Key_D);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(2,3));
    QCOMPARE(mEdit->selBegin(),CharPos(2,3));
    QCOMPARE(mEdit->selEnd(),CharPos(2,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    mEdit->setCaretPos(CharPos{9,2});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Space);
    QTest::keyPress(mEdit.get(), Qt::Key_Space);
    QTest::keyPress(mEdit.get(), Qt::Key_Space);
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(mEdit->caretPos(),CharPos(12,2));
    QCOMPARE(mEdit->selBegin(),CharPos(12,2));
    QCOMPARE(mEdit->selEnd(),CharPos(12,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2,2,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(9,2));
    QCOMPARE(mEdit->selBegin(),CharPos(9,2));
    QCOMPARE(mEdit->selEnd(),CharPos(9,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2,2,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,0));
    QCOMPARE(mEdit->selBegin(),CharPos(2,0));
    QCOMPARE(mEdit->selEnd(),CharPos(2,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(2,3));
    QCOMPARE(mEdit->selBegin(),CharPos(2,3));
    QCOMPARE(mEdit->selEnd(),CharPos(2,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(mEdit->caretPos(),CharPos(12,2));
    QCOMPARE(mEdit->selBegin(),CharPos(12,2));
    QCOMPARE(mEdit->selEnd(),CharPos(12,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2,2,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(9,2));
    QCOMPARE(mEdit->selBegin(),CharPos(9,2));
    QCOMPARE(mEdit->selEnd(),CharPos(9,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2,2,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void QSynedit::TestQSyneditCpp::test_input_chars_in_file()
{
    QStringList text1{
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int main() {ab{}",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,5));
    QCOMPARE(mEdit->subBlockCounts(0,5),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QCOMPARE(mEdit->subBlockCounts(2,4),0);
    QVERIFY(mEdit->hasCodeBlock(6,8));

    mEdit->setCaretPos(CharPos{12,0});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),'a');
    QCOMPARE(mEdit->lineText(0),"int main() {a");
    QCOMPARE(mEdit->caretPos(),CharPos(13,0));
    QCOMPARE(mEdit->selBegin(),CharPos(13,0));
    QCOMPARE(mEdit->selEnd(),CharPos(13,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QTest::keyPress(mEdit.get(),'b');
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,5));
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QVERIFY(mEdit->hasCodeBlock(6,8));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(),'{');
    QCOMPARE(mEdit->lineText(0),"int main() {ab{");
    QCOMPARE(mEdit->caretPos(),CharPos(15,0));
    QCOMPARE(mEdit->selBegin(),CharPos(15,0));
    QCOMPARE(mEdit->selEnd(),CharPos(15,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({9}));
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,5));
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QVERIFY(mEdit->hasCodeBlock(6,8));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(),'}');
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(16,0));
    QCOMPARE(mEdit->selBegin(),CharPos(16,0));
    QCOMPARE(mEdit->selEnd(),CharPos(16,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({9}));
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,5));
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QVERIFY(mEdit->hasCodeBlock(6,8));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(0),"int main() {ab");
    QCOMPARE(mEdit->caretPos(),CharPos(14,0));
    QCOMPARE(mEdit->selBegin(),CharPos(14,0));
    QCOMPARE(mEdit->selEnd(),CharPos(14,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({9,9}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(12,0));
    QCOMPARE(mEdit->selBegin(),CharPos(12,0));
    QCOMPARE(mEdit->selEnd(),CharPos(12,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canUndo());

    //Redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->lineText(0),"int main() {ab");
    QCOMPARE(mEdit->caretPos(),CharPos(14,0));
    QCOMPARE(mEdit->selBegin(),CharPos(14,0));
    QCOMPARE(mEdit->selEnd(),CharPos(14,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->lineText(0),"int main() {ab{}");
    QCOMPARE(mEdit->caretPos(),CharPos(16,0));
    QCOMPARE(mEdit->selBegin(),CharPos(16,0));
    QCOMPARE(mEdit->selEnd(),CharPos(16,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
     QCOMPARE(mReparseStarts, QList<int>({0,0}));
     QCOMPARE(mReparseCounts, QList<int>({9,9}));
     QCOMPARE(mEdit->codeBlockCount(),3);
     QVERIFY(mEdit->hasCodeBlock(0,5));
     QVERIFY(mEdit->hasCodeBlock(2,4));
     QVERIFY(mEdit->hasCodeBlock(6,8));

     //undo agin
     clearSignalDatas();
     mEdit->undo();
     QCOMPARE(mEdit->lineText(0),"int main() {ab");
     QCOMPARE(mEdit->caretPos(),CharPos(14,0));
     QCOMPARE(mEdit->selBegin(),CharPos(14,0));
     QCOMPARE(mEdit->selEnd(),CharPos(14,0));
     QCOMPARE(mInsertStartLines, QList<int>{});
     QCOMPARE(mInsertLineCounts, QList<int>{});
     QCOMPARE(mDeleteStartLines, QList<int>{});
     QCOMPARE(mLineMovedFroms, QList<int>{});
     QCOMPARE(mStatusChanges,
              QList<int>({
                                       (StatusChange::CaretX | StatusChange::Modified),
              }));
     QCOMPARE(mReparseStarts, QList<int>({0,0}));
     QCOMPARE(mReparseCounts, QList<int>({9,9}));

     clearSignalDatas();
     mEdit->undo();
     QCOMPARE(mEdit->content(),text1);
     QCOMPARE(mEdit->caretPos(),CharPos(12,0));
     QCOMPARE(mEdit->selBegin(),CharPos(12,0));
     QCOMPARE(mEdit->selEnd(),CharPos(12,0));
     QCOMPARE(mInsertStartLines, QList<int>{});
     QCOMPARE(mInsertLineCounts, QList<int>{});
     QCOMPARE(mDeleteStartLines, QList<int>{});
     QCOMPARE(mLineMovedFroms, QList<int>{});
     QCOMPARE(mStatusChanges,
              QList<int>({
                                       (StatusChange::CaretX | StatusChange::ModifyChanged),
              }));
     QCOMPARE(mReparseStarts, QList<int>({0,0}));
     QCOMPARE(mReparseCounts, QList<int>({1,1}));
     QVERIFY(!mEdit->canUndo());
     QCOMPARE(mEdit->codeBlockCount(),3);
     QVERIFY(mEdit->hasCodeBlock(0,5));
     QVERIFY(mEdit->hasCodeBlock(2,4));
     QVERIFY(mEdit->hasCodeBlock(6,8));
}

void TestQSyneditCpp::test_input_char_at_end_of_first_line_of_collapsed_block()
{
    QStringList text1{
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,5));
    QCOMPARE(mEdit->subBlockCounts(0,5),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QCOMPARE(mEdit->subBlockCounts(2,4),0);
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QCOMPARE(mEdit->subBlockCounts(6,8),0);

    mEdit->collapse(0,5);
    mEdit->collapse(2,4);
    QVERIFY(mEdit->isCollapsed(0,5));
    QVERIFY(mEdit->isCollapsed(2,4));
    QVERIFY(!mEdit->isCollapsed(6,8));

    QTest::keyPress(mEdit.get(),Qt::Key_End);
    QTest::keyPress(mEdit.get(),'}');
    QCOMPARE(mEdit->codeBlockCount(),2);
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QCOMPARE(mEdit->subBlockCounts(2,4),0);
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QCOMPARE(mEdit->subBlockCounts(6,8),0);
    QVERIFY(!mEdit->isCollapsed(0,5));
    QVERIFY(mEdit->isCollapsed(2,4));
    QVERIFY(!mEdit->isCollapsed(6,8));
}

void TestQSyneditCpp::test_input_string_in_empty_file()
{
    clearContent();
    mEdit->processCommand(EditCommand::ImeStr, "你好呀");
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    clearReparseDatas();
    mEdit->processCommand(EditCommand::ImeStr,"小N");
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QTest::keyPress(mEdit.get(),' ');
    QTest::keyPress(mEdit.get(),' ');
    mEdit->processCommand(EditCommand::ImeStr,"天天");
    mEdit->processCommand(EditCommand::ImeStr,"天晴");
    QTest::keyPress(mEdit.get(),'a');
    QTest::keyPress(mEdit.get(),'b');
    mEdit->processCommand(EditCommand::ImeStr,"小米");
    mEdit->processCommand(EditCommand::ImeStr,"爱吃");
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  天天天晴ab小米爱吃"});
    QCOMPARE(mEdit->caretPos(),CharPos(17,0));
    QCOMPARE(mEdit->selBegin(),CharPos(17,0));
    QCOMPARE(mEdit->selEnd(),CharPos(17,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  天天天晴ab"});
    QCOMPARE(mEdit->caretPos(),CharPos(13,0));
    QCOMPARE(mEdit->selBegin(),CharPos(13,0));
    QCOMPARE(mEdit->selEnd(),CharPos(13,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  天天天晴"});
    QCOMPARE(mEdit->caretPos(),CharPos(11,0));
    QCOMPARE(mEdit->selBegin(),CharPos(11,0));
    QCOMPARE(mEdit->selEnd(),CharPos(11,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  "});
    QCOMPARE(mEdit->caretPos(),CharPos(7,0));
    QCOMPARE(mEdit->selBegin(),CharPos(7,0));
    QCOMPARE(mEdit->selEnd(),CharPos(7,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N"});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
             }));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N"});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  "});
    QCOMPARE(mEdit->caretPos(),CharPos(7,0));
    QCOMPARE(mEdit->selBegin(),CharPos(7,0));
    QCOMPARE(mEdit->selEnd(),CharPos(7,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  天天天晴"});
    QCOMPARE(mEdit->caretPos(),CharPos(11,0));
    QCOMPARE(mEdit->selBegin(),CharPos(11,0));
    QCOMPARE(mEdit->selEnd(),CharPos(11,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  天天天晴ab"});
    QCOMPARE(mEdit->caretPos(),CharPos(13,0));
    QCOMPARE(mEdit->selBegin(),CharPos(13,0));
    QCOMPARE(mEdit->selEnd(),CharPos(13,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  天天天晴ab小米爱吃"});
    QCOMPARE(mEdit->caretPos(),CharPos(17,0));
    QCOMPARE(mEdit->selBegin(),CharPos(17,0));
    QCOMPARE(mEdit->selEnd(),CharPos(17,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  天天天晴ab"});
    QCOMPARE(mEdit->caretPos(),CharPos(13,0));
    QCOMPARE(mEdit->selBegin(),CharPos(13,0));
    QCOMPARE(mEdit->selEnd(),CharPos(13,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  天天天晴"});
    QCOMPARE(mEdit->caretPos(),CharPos(11,0));
    QCOMPARE(mEdit->selBegin(),CharPos(11,0));
    QCOMPARE(mEdit->selEnd(),CharPos(11,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N  "});
    QCOMPARE(mEdit->caretPos(),CharPos(7,0));
    QCOMPARE(mEdit->selBegin(),CharPos(7,0));
    QCOMPARE(mEdit->selEnd(),CharPos(7,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"你好呀小N"});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
                                  }));
}

void TestQSyneditCpp::test_input_string_in_overwrite_mode()
{
    mEdit->setInsertMode(false);
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "张三李四lude <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text3({
                          "张三李四lude <iostream>",
                          "int main() {",
                          " return 0;",
                          "王五孙六"
                      });

    mEdit->setContent(text1);
    clearSignalDatas();
    mEdit->processCommand(EditCommand::ImeStr, "张三");
    mEdit->processCommand(EditCommand::ImeStr, "李四");
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(),CharPos(4,0));
    QCOMPARE(mEdit->selBegin(),CharPos(4,0));
    QCOMPARE(mEdit->selEnd(),CharPos(4,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    mEdit->setCaretPos(CharPos{0,3});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::ImeStr, "王五");
    mEdit->processCommand(EditCommand::ImeStr, "孙六");
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->caretPos(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(),CharPos(4,0));
    QCOMPARE(mEdit->selBegin(),CharPos(4,0));
    QCOMPARE(mEdit->selEnd(),CharPos(4,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->caretPos(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_input_input_chars_undo()
{
    QStringList text {
        "01234",
    };
    QStringList text1 {
        "0abc1234",
    };
    QStringList text2 {
        "0abc1efg234",
    };
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretPos(CharPos{1,0});
    QTest::keyPress(mEdit.get(),'a');
    QTest::keyPress(mEdit.get(),'b');
    QTest::keyPress(mEdit.get(),'c');
    QCOMPARE(mEdit->content(),text1);
    QTest::keyPress(mEdit.get(),Qt::Key_Right);
    QTest::keyPress(mEdit.get(),'e');
    QTest::keyPress(mEdit.get(),'f');
    QTest::keyPress(mEdit.get(),'g');
    QCOMPARE(mEdit->content(),text2);

    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    mEdit->undo();
    QCOMPARE(mEdit->content(),text);
}

void TestQSyneditCpp::test_input_tab_in_empty_file()
{
    clearContent();
    mEdit->processCommand(EditCommand::Tab);
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    clearReparseDatas();
    QTest::keyPress(mEdit.get(),'b');
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QTest::keyPress(mEdit.get(),'c');
    mEdit->processCommand(EditCommand::Tab);
    mEdit->processCommand(EditCommand::Tab);
    QTest::keyPress(mEdit.get(),'a');
    QTest::keyPress(mEdit.get(),'b');
    QTest::keyPress(mEdit.get(),'c');
    QCOMPARE(mEdit->content(),QStringList{"\tbc\t\tabc"});
    QCOMPARE(mEdit->caretPos(),CharPos(8,0));
    QCOMPARE(mEdit->selBegin(),CharPos(8,0));
    QCOMPARE(mEdit->selEnd(),CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    //undo to end
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"\tbc\t\t"});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"\tbc"});
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"\t"});
    QCOMPARE(mEdit->caretPos(),CharPos(1,0));
    QCOMPARE(mEdit->selBegin(),CharPos(1,0));
    QCOMPARE(mEdit->selEnd(),CharPos(1,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QVERIFY(!mEdit->canUndo());
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->modified());

    //redo to end
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"\t"});
    QCOMPARE(mEdit->caretPos(),CharPos(1,0));
    QCOMPARE(mEdit->selBegin(),CharPos(1,0));
    QCOMPARE(mEdit->selEnd(),CharPos(1,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"\tbc"});
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"\tbc\t\t"});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QVERIFY(!mEdit->canRedo());
    QCOMPARE(mEdit->content(),QStringList{"\tbc\t\tabc"});
    QCOMPARE(mEdit->caretPos(),CharPos(8,0));
    QCOMPARE(mEdit->selBegin(),CharPos(8,0));
    QCOMPARE(mEdit->selEnd(),CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    //undo to end again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"\tbc\t\t"});
    QCOMPARE(mEdit->caretPos(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"\tbc"});
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"\t"});
    QCOMPARE(mEdit->caretPos(),CharPos(1,0));
    QCOMPARE(mEdit->selBegin(),CharPos(1,0));
    QCOMPARE(mEdit->selEnd(),CharPos(1,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QVERIFY(!mEdit->canUndo());
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_block_indent()
{

    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "\t#include <iostream>",
                          "\tint main() {",
                          "\t return 0;",
                          "\t}"
                      });
    mEdit->setContent(text1);
    mEdit->setCaretPos(mEdit->fileEnd());
    mEdit->selectAll();
    clearSignalDatas();
    mEdit->processCommand(EditCommand::BlockIndent);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(2,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                            (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,1,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(2,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                            (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));
    QVERIFY(!mEdit->canRedo());
    QVERIFY(mEdit->modified());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,1,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_block_unindent()
{
    QStringList text1({
                          "\t#include <iostream>",
                          "\tint main() {",
                          "\t return 0;",
                          "\t}"
                      });
    QStringList text2({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    mEdit->setContent(text1);
    mEdit->setCaretPos(mEdit->fileEnd());
    mEdit->selectAll();
    clearSignalDatas();
    mEdit->processCommand(EditCommand::BlockUnindent);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                            (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(2,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(2,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,1,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                            (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));
    QVERIFY(!mEdit->canRedo());
    QVERIFY(mEdit->modified());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(2,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(2,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,1,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

}

void TestQSyneditCpp::test_replace_line_in_empty_file()
{
    clearContent();

    clearSignalDatas();
    mEdit->replaceLine(0,"1123");
    QCOMPARE(mEdit->content(),QStringList{"1123"});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"1123"});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_replace_line_at_file_begin_end()
{
    QStringList text1{
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "lalalala"
    };
    QStringList text3{
        "123",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "lalalala"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(), 3);

    mEdit->setCaretAndSelection(CharPos{1,8},CharPos{0,8},CharPos{1,8});
    clearSignalDatas();
    mEdit->replaceLine(8,"lalalala");
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(1,8));
    QCOMPARE(mEdit->selBegin(),CharPos(1,8));
    QCOMPARE(mEdit->selEnd(),CharPos(1,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({8}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(), 2);

    mEdit->setCaretAndSelection(CharPos{12,0},CharPos{0,0},CharPos{12,0});
    clearSignalDatas();
    mEdit->replaceLine(0,"123");
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({6}));
    QCOMPARE(mEdit->codeBlockCount(), 1);

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(12,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(12,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({6}));
    QCOMPARE(mEdit->codeBlockCount(), 2);

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(1,8));
    QCOMPARE(mEdit->selBegin(),CharPos(0,8));
    QCOMPARE(mEdit->selEnd(),CharPos(1,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({8}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(), 3);

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(1,8));
    QCOMPARE(mEdit->selBegin(),CharPos(1,8));
    QCOMPARE(mEdit->selEnd(),CharPos(1,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({8}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(), 2);

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::CaretY | StatusChange::CaretX),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({6}));
    QCOMPARE(mEdit->codeBlockCount(), 1);

    QVERIFY(!mEdit->canRedo());

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(12,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(12,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({6}));
    QCOMPARE(mEdit->codeBlockCount(), 2);

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(1,8));
    QCOMPARE(mEdit->selBegin(),CharPos(0,8));
    QCOMPARE(mEdit->selEnd(),CharPos(1,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY | StatusChange::Selection),
             }));
    QCOMPARE(mReparseStarts, QList<int>({8}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(), 3);

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_delete_chars_in_empty_file()
{
    clearContent();

    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QVERIFY(mEdit->empty());
}

void QSynedit::TestQSyneditCpp::test_delete_chars_in_file()
{
    QStringList text1{
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx+",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,5));
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QVERIFY(mEdit->hasCodeBlock(6,8));

    mEdit->setCaretPos(CharPos{4,3});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete); //'+'
    QCOMPARE(mEdit->lineText(3),"\t\tx+;");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete); //';'
    QCOMPARE(mEdit->lineText(3),"\t\tx+");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete); //linebreak
    QCOMPARE(mEdit->lineText(3),"\t\tx+\t}");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{3});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,4));
    QVERIFY(mEdit->hasCodeBlock(2,3));
    QVERIFY(mEdit->hasCodeBlock(5,7));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete); //'\t'
    QCOMPARE(mEdit->lineText(3),"\t\tx+}");
    QTest::keyPress(mEdit.get(), Qt::Key_Delete); //'}'
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,5}));
    QCOMPARE(mEdit->codeBlockCount(),2);
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QVERIFY(mEdit->hasCodeBlock(5,7));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+}");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({5}));
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,4));
    QVERIFY(mEdit->hasCodeBlock(2,3));
    QVERIFY(mEdit->hasCodeBlock(5,7));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+\t}");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));


    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{3});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,5));
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QVERIFY(mEdit->hasCodeBlock(6,8));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx++;");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+\t}");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{3});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,4));
    QVERIFY(mEdit->hasCodeBlock(2,3));
    QVERIFY(mEdit->hasCodeBlock(5,7));

    clearSignalDatas();
    mEdit->redo(); //'\t'
    QCOMPARE(mEdit->lineText(3),"\t\tx+}");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({5}));

    QVERIFY(!mEdit->canRedo());
    QCOMPARE(mEdit->codeBlockCount(),2);
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QVERIFY(mEdit->hasCodeBlock(5,7));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+}");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({5}));
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,4));
    QVERIFY(mEdit->hasCodeBlock(2,3));
    QVERIFY(mEdit->hasCodeBlock(5,7));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+\t}");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{3});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,5));
    QVERIFY(mEdit->hasCodeBlock(2,4));
    QVERIFY(mEdit->hasCodeBlock(6,8));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx++;");
    QCOMPARE(mEdit->caretPos(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
}

void QSynedit::TestQSyneditCpp::test_delete_chars_at_file_begin_end()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text3({
                          "clude <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    mEdit->setCaretPos(mEdit->fileEnd());
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::Modified),
                                      (StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_delete_chars_undo()
{
    QStringList text {
        "0abc1efg234",
    };
    QStringList text1 {
        "01efg234",
    };
    QStringList text2 {
        "01234",
    };
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretPos(CharPos{1,0});
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text1);
    QTest::keyPress(mEdit.get(),Qt::Key_Right);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text2);

    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    mEdit->undo();
    QCOMPARE(mEdit->content(),text);
}

void TestQSyneditCpp::test_delete_chars_undo2()
{
    QStringList text {
        "0abc efg234",
    };
    QStringList text1 {
        "0 efg234",
    };
    QStringList text2 {
        "0efg234",
    };
    QStringList text3 {
        "0234",
    };
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretPos(CharPos{1,0});
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text1);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text2);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text3);

    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    mEdit->undo();
    QCOMPARE(mEdit->content(),text);
}

void TestQSyneditCpp::test_merge_with_next_line_with_collapsed_block()
{
    //merge one line before '{'
    //merge before '{'
    //the changing block should keep collapsed
    QStringList text1{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text3{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;\tif (x>0)\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };

    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    mEdit->collapse(1,3);
    mEdit->collapse(8,10);
    mEdit->collapse(12,14);
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->setCaretPos(CharPos{7,6});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    mEdit->setCaretPos(CharPos{16,6});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,9));
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,9));
    QVERIFY(mEdit->isCollapsed(6,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,3}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::CaretX),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::CaretX),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,9));
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,9));
    QVERIFY(mEdit->isCollapsed(6,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,3}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::CaretX),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_merge_with_next_line_with_collapsed_block2()
{
    //merge after '}'
    //the changing block should keep collapsed
    QStringList text1{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    mEdit->collapse(1,3);
    mEdit->collapse(8,10);
    mEdit->collapse(12,14);
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->setCaretPos(CharPos{2,10});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{11});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({10}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(11,13));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{11});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({10,11}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{11});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({10}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(11,13));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{11});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({10,11}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

}

void TestQSyneditCpp::test_merge_with_next_line_with_collapsed_block3()
{
    //merge after '{'
    //the changing block should uncollapse
    QStringList text1{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    mEdit->collapse(1,3);
    mEdit->collapse(8,10);
    mEdit->collapse(12,14);
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->setCaretPos(CharPos{2,8});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{9});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({8}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(8,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(!mEdit->isCollapsed(8,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    mEdit->collapse(8,9);
    QVERIFY(mEdit->isCollapsed(8,9));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{9});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({8,9}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(!mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->collapse(8,10);
    QVERIFY(mEdit->isCollapsed(8,10));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{9});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({8}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(8,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(!mEdit->isCollapsed(8,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    mEdit->collapse(8,9);
    QVERIFY(mEdit->isCollapsed(8,9));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{9});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({8,9}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(!mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->collapse(8,10);
    QVERIFY(mEdit->isCollapsed(8,10));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_delete_prev_chars_in_empty_file()
{
    clearContent();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(mEdit->empty());
}

void QSynedit::TestQSyneditCpp::test_delete_prev_chars_at_file_begin_end()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "#include <iostream>",
                          "int main() {",
                          " return 0",
                      });
    QStringList text3({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                      });
    QStringList text4({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          ""
                      });
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace); //done nothing
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace); //done nothing
    QCOMPARE(mEdit->content(),text1);

    mEdit->setCaretPos(mEdit->fileEnd());
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace); // delete last '}'
    QCOMPARE(mEdit->codeBlockCount(),0);
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(9,2));
    QCOMPARE(mEdit->selBegin(),CharPos(9,2));
    QCOMPARE(mEdit->selEnd(),CharPos(9,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{3});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY),
                                      (StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(10,2));
    QCOMPARE(mEdit->selBegin(),CharPos(10,2));
    QCOMPARE(mEdit->selEnd(),CharPos(10,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(mEdit->caretPos(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{2});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(mEdit->caretPos(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),0);

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(10,2));
    QCOMPARE(mEdit->selBegin(),CharPos(10,2));
    QCOMPARE(mEdit->selEnd(),CharPos(10,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{3});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(9,2));
    QCOMPARE(mEdit->selBegin(),CharPos(9,2));
    QCOMPARE(mEdit->selEnd(),CharPos(9,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified)
             }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canRedo());

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(10,2));
    QCOMPARE(mEdit->selBegin(),CharPos(10,2));
    QCOMPARE(mEdit->selEnd(),CharPos(10,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(mEdit->caretPos(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{2});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
}

void TestQSyneditCpp::test_delete_prev_chars_undo()
{
    QStringList text {
        "0abc1efg234",
    };
    QStringList text1 {
        "01efg234",
    };
    QStringList text2 {
        "01234",
    };
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretPos(CharPos{4,0});
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text1);
    QTest::keyPress(mEdit.get(),Qt::Key_Right);
    QTest::keyPress(mEdit.get(),Qt::Key_Right);
    QTest::keyPress(mEdit.get(),Qt::Key_Right);
    QTest::keyPress(mEdit.get(),Qt::Key_Right);

    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text2);

    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    mEdit->undo();
    QCOMPARE(mEdit->content(),text);
}

void TestQSyneditCpp::test_merge_with_prev_line_with_collapsed_block()
{
    //merge one line before '{'
    //merge before '{'
    //the changing block should keep collapsed
    QStringList text1{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text3{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;\tif (x>0)\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };

    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    mEdit->collapse(1,3);
    mEdit->collapse(8,10);
    mEdit->collapse(12,14);
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->setCaretPos(CharPos{0,7});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    mEdit->setCaretPos(CharPos{0,7});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,9));
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,9));
    QVERIFY(mEdit->isCollapsed(6,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(0,7));
    QCOMPARE(mEdit->selBegin(),CharPos(0,7));
    QCOMPARE(mEdit->selEnd(),CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,3}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,7));
    QCOMPARE(mEdit->selBegin(),CharPos(0,7));
    QCOMPARE(mEdit->selEnd(),CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::CaretX),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,9));
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,9));
    QVERIFY(mEdit->isCollapsed(6,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(0,7));
    QCOMPARE(mEdit->selBegin(),CharPos(0,7));
    QCOMPARE(mEdit->selEnd(),CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,3}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,7));
    QCOMPARE(mEdit->selBegin(),CharPos(0,7));
    QCOMPARE(mEdit->selEnd(),CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_merge_with_prev_line_with_collapsed_block2()
{
    //merge after '}'
    //the changing block should keep collapsed
    QStringList text1{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    mEdit->collapse(1,3);
    mEdit->collapse(8,10);
    mEdit->collapse(12,14);
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->setCaretPos(CharPos{0,11});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{11});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({10}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(11,13));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,11));
    QCOMPARE(mEdit->selBegin(),CharPos(0,11));
    QCOMPARE(mEdit->selEnd(),CharPos(0,11));
    QCOMPARE(mInsertStartLines, QList<int>{11});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({10,11}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{11});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified  | StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({10}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(11,13));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,11));
    QCOMPARE(mEdit->selBegin(),CharPos(0,11));
    QCOMPARE(mEdit->selEnd(),CharPos(0,11));
    QCOMPARE(mInsertStartLines, QList<int>{11});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({10,11}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

}

void TestQSyneditCpp::test_merge_with_prev_line_with_collapsed_block3()
{
    //merge after '{'
    //the changing block should uncollapse
    QStringList text1{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t()",
        "{",
        " ttt;",
        "}",
        "int main()",
        "{",
        "\tint x;",
        "\tif (x>0)",
        "\t{\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    mEdit->collapse(1,3);
    mEdit->collapse(8,10);
    mEdit->collapse(12,14);
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->setCaretPos(CharPos{0,9});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{9});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({8}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(8,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(!mEdit->isCollapsed(8,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    mEdit->collapse(8,9);
    QVERIFY(mEdit->isCollapsed(8,9));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,9));
    QCOMPARE(mEdit->selBegin(),CharPos(0,9));
    QCOMPARE(mEdit->selEnd(),CharPos(0,9));
    QCOMPARE(mInsertStartLines, QList<int>{9});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({8,9}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(!mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->collapse(8,10); // caret set to (2,8) here
    QVERIFY(mEdit->isCollapsed(8,10));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{9});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({8}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,10));
    QVERIFY(mEdit->hasCodeBlock(8,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,10));
    QVERIFY(!mEdit->isCollapsed(8,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    mEdit->collapse(8,9);
    QVERIFY(mEdit->isCollapsed(8,9));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(),CharPos(0,9));
    QCOMPARE(mEdit->selBegin(),CharPos(0,9));
    QCOMPARE(mEdit->selEnd(),CharPos(0,9));
    QCOMPARE(mInsertStartLines, QList<int>{9});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({8,9}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,11));
    QVERIFY(mEdit->hasCodeBlock(8,10));
    QVERIFY(mEdit->hasCodeBlock(12,14));
    QVERIFY(mEdit->isCollapsed(1,3));
    QVERIFY(!mEdit->isCollapsed(5,11));
    QVERIFY(!mEdit->isCollapsed(8,10));
    QVERIFY(mEdit->isCollapsed(12,14));

    mEdit->collapse(8,10);
    QVERIFY(mEdit->isCollapsed(8,10));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_delete_and_backspace_chars_undo()
{
    QStringList text {
        "0abcefg234",
    };
    QStringList text1 {
        "0efg234",
    };
    QStringList text2 {
        "0234",
    };
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretPos(CharPos{4,0});
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text1);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text2);

    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    mEdit->undo();
    QCOMPARE(mEdit->content(),text);
}

void QSynedit::TestQSyneditCpp::test_break_line_in_empty_file()
{
    clearContent();
    QTest::keyPress(mEdit.get(), Qt::Key_Enter);
    QCOMPARE(mEdit->content(),QStringList({"",""}));
    QCOMPARE(mEdit->caretPos(),CharPos(0,1));
    QCOMPARE(mEdit->selBegin(),CharPos(0,1));
    QCOMPARE(mEdit->selEnd(),CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>{0});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList({""}));
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{0});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList({"",""}));
    QCOMPARE(mEdit->caretPos(),CharPos(0,1));
    QCOMPARE(mEdit->selBegin(),CharPos(0,1));
    QCOMPARE(mEdit->selEnd(),CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>{0});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canRedo());
    QVERIFY(mEdit->modified());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList({""}));
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{0});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->modified());
}

void QSynedit::TestQSyneditCpp::test_break_lines()
{
    QStringList text1({
                         " /* */",
                         "int main() {  }",
                         "return 0;"
                      });
    QStringList text2({
                         " /*",
                         "",
                         "*/",
                         "int main() {  }",
                         "return 0;"
                      });
    QStringList text3({
                         " /*",
                         "",
                         "*/",
                         "int main() { ",
                         "\t",
                         "}",
                         "return 0;"
                      });
    QStringList text4({
                         " /*",
                         "",
                         "*/",
                         "int main() { ",
                         "\t",
                         "}",
                         "ret",
                         "urn 0;"
                      });
    clearContent();
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),0);
    mEdit->setCaretPos(CharPos{3,0});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({0,1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1,1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1,1}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    mEdit->setCaretPos(CharPos{13,3});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(), CharPos(1,4));
    QCOMPARE(mEdit->selBegin(), CharPos(1,4));
    QCOMPARE(mEdit->selEnd(), CharPos(1,4));
    QCOMPARE(mInsertStartLines, QList<int>({4,4}));
    QCOMPARE(mInsertLineCounts, QList<int>({1,1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,4,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(3,5));

    mEdit->setCaretPos(CharPos{3,6});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text4);
    QCOMPARE(mEdit->caretPos(), CharPos(0,7));
    QCOMPARE(mEdit->selBegin(), CharPos(0,7));
    QCOMPARE(mEdit->selEnd(), CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(), CharPos(3,6));
    QCOMPARE(mEdit->selBegin(), CharPos(3,6));
    QCOMPARE(mEdit->selEnd(), CharPos(3,6));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(13,3));
    QCOMPARE(mEdit->selBegin(), CharPos(13,3));
    QCOMPARE(mEdit->selEnd(), CharPos(13,3));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({4}));
    QCOMPARE(mDeleteLineCounts, QList<int>({2}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),0);

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(3,0));
    QCOMPARE(mEdit->selBegin(), CharPos(3,0));
    QCOMPARE(mEdit->selEnd(), CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1,0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1,1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({0,1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1,1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1,0,1}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(), CharPos(1,4));
    QCOMPARE(mEdit->selBegin(), CharPos(1,4));
    QCOMPARE(mEdit->selEnd(), CharPos(1,4));
    QCOMPARE(mInsertStartLines, QList<int>({4,4}));
    QCOMPARE(mInsertLineCounts, QList<int>({1,1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,4,3,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(3,5));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text4);
    QCOMPARE(mEdit->caretPos(), CharPos(0,7));
    QCOMPARE(mEdit->selBegin(), CharPos(0,7));
    QCOMPARE(mEdit->selEnd(), CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(), CharPos(3,6));
    QCOMPARE(mEdit->selBegin(), CharPos(3,6));
    QCOMPARE(mEdit->selEnd(), CharPos(3,6));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(13,3));
    QCOMPARE(mEdit->selBegin(), CharPos(13,3));
    QCOMPARE(mEdit->selEnd(), CharPos(13,3));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({4}));
    QCOMPARE(mDeleteLineCounts, QList<int>({2}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),0);

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(3,0));
    QCOMPARE(mEdit->selBegin(), CharPos(3,0));
    QCOMPARE(mEdit->selEnd(), CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1,0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1,1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_break_lines_with_collapsed_block()
{
    //break line after leading space
    //break line before '{'
    //collapse state should be keeped
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\t",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text3{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\t",
        "\tif (x>0)",
        "\t{",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
    mEdit->collapse(0,2);
    mEdit->collapse(5,7);
    mEdit->collapse(9,11);
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,8));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(9,11));


    mEdit->setCaretPos(CharPos{1,5}); //caret between "\t" and "if (x>0) {"
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(1,6));
    QCOMPARE(mEdit->selBegin(), CharPos(1,6));
    QCOMPARE(mEdit->selEnd(), CharPos(1,6));
    QCOMPARE(mInsertStartLines, QList<int>({5}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({5,6}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(6,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->setCaretPos(CharPos{9,6}); //caret between "if (x>0)" and " {"
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(), CharPos(1,7));
    QCOMPARE(mEdit->selBegin(), CharPos(1,7));
    QCOMPARE(mEdit->selEnd(), CharPos(1,7));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,3}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(9,6));
    QCOMPARE(mEdit->selBegin(), CharPos(9,6));
    QCOMPARE(mEdit->selEnd(), CharPos(9,6));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified |StatusChange::CaretY | StatusChange::CaretX}));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(6,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(1,5));
    QCOMPARE(mEdit->selBegin(), CharPos(1,5));
    QCOMPARE(mEdit->selEnd(), CharPos(1,5));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({5}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::ModifyChanged |StatusChange::CaretY | StatusChange::CaretX}));
    QCOMPARE(mReparseStarts, QList<int>({5}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,8));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(9,11));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(1,6));
    QCOMPARE(mEdit->selBegin(), CharPos(1,6));
    QCOMPARE(mEdit->selEnd(), CharPos(1,6));
    QCOMPARE(mInsertStartLines, QList<int>({5}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({5,6}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(6,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(), CharPos(1,7));
    QCOMPARE(mEdit->selBegin(), CharPos(1,7));
    QCOMPARE(mEdit->selEnd(), CharPos(1,7));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,3}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,10));
    QVERIFY(mEdit->hasCodeBlock(7,9));
    QVERIFY(mEdit->hasCodeBlock(11,13));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,10));
    QVERIFY(mEdit->isCollapsed(7,9));
    QVERIFY(mEdit->isCollapsed(11,13));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(9,6));
    QCOMPARE(mEdit->selBegin(), CharPos(9,6));
    QCOMPARE(mEdit->selEnd(), CharPos(9,6));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified |StatusChange::CaretY | StatusChange::CaretX}));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(6,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(6,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(1,5));
    QCOMPARE(mEdit->selBegin(), CharPos(1,5));
    QCOMPARE(mEdit->selEnd(), CharPos(1,5));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({5}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::ModifyChanged |StatusChange::CaretY | StatusChange::CaretX}));
    QCOMPARE(mReparseStarts, QList<int>({5}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,8));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(9,11));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_break_lines_with_collapsed_block2()
{
    //break after '}'
    //collapse state should be keeped
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "\t",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };

    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
    mEdit->collapse(0,2);
    mEdit->collapse(5,7);
    mEdit->collapse(9,11);
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,8));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(9,11));

    mEdit->setCaretPos(CharPos{2,7});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(1,8));
    QCOMPARE(mEdit->selBegin(), CharPos(1,8));
    QCOMPARE(mEdit->selEnd(), CharPos(1,8));
    QCOMPARE(mInsertStartLines, QList<int>({8}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({7,8}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(2,7));
    QCOMPARE(mEdit->selBegin(), CharPos(2,7));
    QCOMPARE(mEdit->selEnd(), CharPos(2,7));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({8}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({7}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,8));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(9,11));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(1,8));
    QCOMPARE(mEdit->selBegin(), CharPos(1,8));
    QCOMPARE(mEdit->selEnd(), CharPos(1,8));
    QCOMPARE(mInsertStartLines, QList<int>({8}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({7,8}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));
    QVERIFY(!mEdit->canRedo());

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(2,7));
    QCOMPARE(mEdit->selBegin(), CharPos(2,7));
    QCOMPARE(mEdit->selEnd(), CharPos(2,7));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({8}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({7}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,8));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(9,11));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_break_lines_with_collapsed_block3()
{
    //break after '}'
    //the changing block should be uncollapsed
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\t",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
    mEdit->collapse(0,2);
    mEdit->collapse(5,7);
    mEdit->collapse(9,11);
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,8));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(9,11));

    mEdit->setCaretPos(CharPos{11,5});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(2,6));
    QCOMPARE(mEdit->selBegin(), CharPos(2,6));
    QCOMPARE(mEdit->selEnd(), CharPos(2,6));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({5,6}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(5,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(5,8); //Caret at (5,11) after collapse
    QVERIFY(mEdit->isCollapsed(5,8));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(11,5));
    QCOMPARE(mEdit->selBegin(), CharPos(11,5));
    QCOMPARE(mEdit->selEnd(), CharPos(11,5));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::ModifyChanged, }));
    QCOMPARE(mReparseStarts, QList<int>({5}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,8));
    QVERIFY(!mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(9,11));

    mEdit->collapse(5,7);
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(2,6));
    QCOMPARE(mEdit->selBegin(), CharPos(2,6));
    QCOMPARE(mEdit->selEnd(), CharPos(2,6));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
    QCOMPARE(mReparseStarts, QList<int>({5,6}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(5,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(5,8); //Caret at (5,11) after collapse
    QVERIFY(mEdit->isCollapsed(5,8));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(11,5));
    QCOMPARE(mEdit->selBegin(), CharPos(11,5));
    QCOMPARE(mEdit->selEnd(), CharPos(11,5));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({ StatusChange::ModifyChanged,}));
    QCOMPARE(mReparseStarts, QList<int>({5}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
    QVERIFY(mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,8));
    QVERIFY(!mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(9,11));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_if_else_indent()
{
    QStringList text1({
                          "int main() { int x; if (x>0) if (x<0) x++; else	y--; if (y==0) for (int i=0;i<10;i++) x=10;	else y=10; return 0; }"
                      });
    QStringList text2({
                          "int main() {",
                          "\tint x; if (x>0) if (x<0) x++; else	y--; if (y==0) for (int i=0;i<10;i++) x=10;	else y=10; return 0; }"
                      });
    QStringList text3({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0) if (x<0) x++; else	y--; if (y==0) for (int i=0;i<10;i++) x=10;	else y=10; return 0; }"
                      });
    QStringList text4({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0) x++; else	y--; if (y==0) for (int i=0;i<10;i++) x=10;	else y=10; return 0; }"
                      });
    QStringList text5({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++; else	y--; if (y==0) for (int i=0;i<10;i++) x=10;	else y=10; return 0; }"
                      });
    QStringList text6({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++;",
                          "\t\telse	y--; if (y==0) for (int i=0;i<10;i++) x=10;	else y=10; return 0; }"
                      });
    QStringList text7({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++;",
                          "\t\telse",
                          "\t\t\ty--; if (y==0) for (int i=0;i<10;i++) x=10;	else y=10; return 0; }"
                      });
    QStringList text8({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++;",
                          "\t\telse",
                          "\t\t\ty--;",
                          "\tif (y==0) for (int i=0;i<10;i++) x=10;	else y=10; return 0; }"
                      });
    QStringList text9({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++;",
                          "\t\telse",
                          "\t\t\ty--;",
                          "\tif (y==0)",
                          "\t\tfor (int i=0;i<10;i++) x=10;	else y=10; return 0; }"
                      });
    QStringList text10({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++;",
                          "\t\telse",
                          "\t\t\ty--;",
                          "\tif (y==0)",
                          "\t\tfor (int i=0;i<10;i++)",
                          "\t\t\tx=10;	else y=10; return 0; }"
                      });
    QStringList text11({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++;",
                          "\t\telse",
                          "\t\t\ty--;",
                          "\tif (y==0)",
                          "\t\tfor (int i=0;i<10;i++)",
                          "\t\t\tx=10;",
                          "\telse y=10; return 0; }"
                      });
    QStringList text12({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++;",
                          "\t\telse",
                          "\t\t\ty--;",
                          "\tif (y==0)",
                          "\t\tfor (int i=0;i<10;i++)",
                          "\t\t\tx=10;",
                          "\telse",
                          "\t\ty=10; return 0; }"
                      });
    QStringList text13({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++;",
                          "\t\telse",
                          "\t\t\ty--;",
                          "\tif (y==0)",
                          "\t\tfor (int i=0;i<10;i++)",
                          "\t\t\tx=10;",
                          "\telse",
                          "\t\ty=10;",
                          "\treturn 0; }"
                      });
    QStringList text14({
                          "int main() {",
                          "\tint x;",
                          "\tif (x>0)",
                          "\t\tif (x<0)",
                          "\t\t\tx++;",
                          "\t\telse",
                          "\t\t\ty--;",
                          "\tif (y==0)",
                          "\t\tfor (int i=0;i<10;i++)",
                          "\t\t\tx=10;",
                          "\telse",
                          "\t\ty=10;",
                          "\treturn 0;",
                          "}"
                      });
    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),0);

    mEdit->setCaretPos(CharPos{12,0});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text2);

    mEdit->setCaretPos(CharPos{7,1});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text3);

    mEdit->setCaretPos(CharPos{9,2});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text4);

    mEdit->setCaretPos(CharPos{10,3});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text5);

    mEdit->setCaretPos(CharPos{7,4});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text6);

    mEdit->setCaretPos(CharPos{6,5});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text7);

    mEdit->setCaretPos(CharPos{7,6});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text8);

    mEdit->setCaretPos(CharPos{10,7});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text9);

    mEdit->setCaretPos(CharPos{24,8});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text10);

    mEdit->setCaretPos(CharPos{8,9});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text11);

    mEdit->setCaretPos(CharPos{5,10});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text12);

    mEdit->setCaretPos(CharPos{7,11});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text13);

    mEdit->setCaretPos(CharPos{10,12});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text14);

    //undo
    mEdit->undo();
    QCOMPARE(mEdit->content(), text13);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text12);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text11);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text10);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text9);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text8);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text7);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text6);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text5);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text4);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text3);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text4);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text5);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text6);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text7);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text8);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text9);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text10);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text11);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text12);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text13);
    mEdit->redo();
    QCOMPARE(mEdit->content(),text14);
    QVERIFY(!mEdit->canRedo());

    //undo
    mEdit->undo();
    QCOMPARE(mEdit->content(), text13);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text12);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text11);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text10);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text9);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text8);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text7);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text6);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text5);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text4);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text3);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_delete_current_line_in_empty_file()
{
    clearContent();
    mEdit->processCommand(EditCommand::DeleteLine);
    QCOMPARE(mEdit->content(), QStringList{""});
    QCOMPARE(mEdit->caretPos(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({}));
    QCOMPARE(mReparseStarts, QList<int>({}));
    QCOMPARE(mReparseCounts, QList<int>({}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_delete_current_line()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "#include <iostream>",
                          " return 0;",
                          "}"
                      });
    QStringList text3({
                          "#include <iostream>",
                          " return 0;",
                      });

    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));

    mEdit->setCaretPos(CharPos{0,1});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::DeleteLine);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({2}));
    QCOMPARE(mEdit->codeBlockCount(),0);


    mEdit->setCaretPos(CharPos{0,2});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::DeleteLine);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(), CharPos(10,1));
    QCOMPARE(mEdit->selBegin(), CharPos(10,1));
    QCOMPARE(mEdit->selEnd(), CharPos(10,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({2}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretY | StatusChange::CaretX
                                  }));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos(0,2));
    QCOMPARE(mEdit->selBegin(), CharPos(0,2));
    QCOMPARE(mEdit->selEnd(), CharPos(0,2));
    QCOMPARE(mInsertStartLines, QList<int>({2}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretY | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2,1,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::CaretY | StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1,1,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,2}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({2}));
    QCOMPARE(mEdit->codeBlockCount(),0);

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(), CharPos(10,1));
    QCOMPARE(mEdit->selBegin(), CharPos(10,1));
    QCOMPARE(mEdit->selEnd(), CharPos(10,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({2}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos(0,2));
    QCOMPARE(mEdit->selBegin(), CharPos(0,2));
    QCOMPARE(mEdit->selEnd(), CharPos(0,2));
    QCOMPARE(mInsertStartLines, QList<int>({2}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretY | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2,1,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::CaretY | StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1,1,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,2}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
}

void TestQSyneditCpp::test_duplicate_current_line_in_empty_file()
{
    clearContent();
    mEdit->processCommand(EditCommand::Duplicate);
    QCOMPARE(mEdit->content(),QStringList({"",""}));
    QCOMPARE(mEdit->caretPos(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList({""}));
    QCOMPARE(mEdit->caretPos(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({}));
    QCOMPARE(mReparseCounts, QList<int>({}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList({"",""}));
    QCOMPARE(mEdit->caretPos(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList({""}));
    QCOMPARE(mEdit->caretPos(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({}));
    QCOMPARE(mReparseCounts, QList<int>({}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_duplicate_current_line()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "#include <iostream>",
                          "int main() {",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text3({
                          "#include <iostream>",
                          "int main() {",
                          "int main() {",
                          " return 0;",
                          " return 0;",
                          "}"
                      });

    mEdit->setContent(text1);
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
    mEdit->setCaretPos(CharPos{1,1});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::Duplicate);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos(1,2));
    QCOMPARE(mEdit->selBegin(), CharPos(1,2));
    QCOMPARE(mEdit->selEnd(), CharPos(1,2));
    QCOMPARE(mInsertStartLines, QList<int>({2}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));

    mEdit->setCaretPos(CharPos{3,3});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::Duplicate);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(), CharPos(3,4));
    QCOMPARE(mEdit->selBegin(), CharPos(3,4));
    QCOMPARE(mEdit->selEnd(), CharPos(3,4));
    QCOMPARE(mInsertStartLines, QList<int>({4}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,5));
    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos(3,3));
    QCOMPARE(mEdit->selBegin(), CharPos(3,3));
    QCOMPARE(mEdit->selEnd(), CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({4}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos(1,1));
    QCOMPARE(mEdit->selBegin(), CharPos(1,1));
    QCOMPARE(mEdit->selEnd(), CharPos(1,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({2}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({2}));
    QVERIFY(!mEdit->canUndo());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos(1,2));
    QCOMPARE(mEdit->selBegin(), CharPos(1,2));
    QCOMPARE(mEdit->selEnd(), CharPos(1,2));
    QCOMPARE(mInsertStartLines, QList<int>({2}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(), CharPos(3,4));
    QCOMPARE(mEdit->selBegin(), CharPos(3,4));
    QCOMPARE(mEdit->selEnd(), CharPos(3,4));
    QCOMPARE(mInsertStartLines, QList<int>({4}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canRedo());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,5));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos(3,3));
    QCOMPARE(mEdit->selBegin(), CharPos(3,3));
    QCOMPARE(mEdit->selEnd(), CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({4}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos(1,1));
    QCOMPARE(mEdit->selBegin(), CharPos(1,1));
    QCOMPARE(mEdit->selEnd(), CharPos(1,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({2}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({2}));
    QVERIFY(!mEdit->canUndo());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
}

void TestQSyneditCpp::test_duplicate_selection()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text2({
                          "#includeinclude <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    QStringList text3({
                          "#includeinclude <iostream>",
                          "int main() {",
                          " rain() {",
                          " return 0;",
                          "}"
                      });

    mEdit->setContent(text1);
    mEdit->setCaretAndSelection(CharPos{1,0}, CharPos{1,0},CharPos{8,0});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::Duplicate);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(15,0));
    QCOMPARE(mEdit->selBegin(), CharPos(8,0));
    QCOMPARE(mEdit->selEnd(), CharPos(15,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    mEdit->setCaretAndSelection(CharPos{2,2}, CharPos{5,1},CharPos{2,2});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::Duplicate);
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(), CharPos(2,3));
    QCOMPARE(mEdit->selBegin(), CharPos(2,2));
    QCOMPARE(mEdit->selEnd(), CharPos(2,3));
    QCOMPARE(mInsertStartLines, QList<int>({3}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretY | StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,2}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(2,2));
    QCOMPARE(mEdit->selBegin(), CharPos(5,1));
    QCOMPARE(mEdit->selEnd(), CharPos(2,2));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({3}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretY | StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(1,0));
    QCOMPARE(mEdit->selBegin(), CharPos(1,0));
    QCOMPARE(mEdit->selEnd(), CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretX | StatusChange::CaretY | StatusChange::ModifyChanged | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(15,0));
    QCOMPARE(mEdit->selBegin(), CharPos(8,0));
    QCOMPARE(mEdit->selEnd(), CharPos(15,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(), CharPos(2,3));
    QCOMPARE(mEdit->selBegin(), CharPos(2,2));
    QCOMPARE(mEdit->selEnd(), CharPos(2,3));
    QCOMPARE(mInsertStartLines, QList<int>({3}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,2}));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos(2,2));
    QCOMPARE(mEdit->selBegin(), CharPos(5,1));
    QCOMPARE(mEdit->selEnd(), CharPos(2,2));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({3}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretY | StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos(1,0));
    QCOMPARE(mEdit->selBegin(), CharPos(1,0));
    QCOMPARE(mEdit->selEnd(), CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretX | StatusChange::CaretY | StatusChange::ModifyChanged | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_delete_selection_in_empty_file()
{
    clearContent();
    mEdit->selectAll();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QVERIFY(mEdit->empty());
}

void TestQSyneditCpp::test_delete_while_select_all()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    mEdit->setContent(text1);

    mEdit->setCaretPos(CharPos{3,2});
    clearSignalDatas();
    mEdit->selectAll();
    QCOMPARE(mEdit->caretPos(), CharPos({3,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({3}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                      | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({3,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));
    QCOMPARE(mInsertStartLines, QList<int>({0}));
    QCOMPARE(mInsertLineCounts, QList<int>({3}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({3,1,1,1,1}));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({3}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                      | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({3,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));
    QCOMPARE(mInsertStartLines, QList<int>({0}));
    QCOMPARE(mInsertLineCounts, QList<int>({3}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({3,1,1,1,1}));
}

void TestQSyneditCpp::test_delete_selection_in_line()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}",
                          "int xxx;"
                      });
    QStringList text2({
                          "iostream>",
                          "int main() {",
                          " return 0;",
                          "}",
                          "int xxx;"
                      });
    mEdit->setContent(text1);

    mEdit->setCaretAndSelection(CharPos({10,0}),CharPos({0,0}),CharPos({10,0}));
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX  | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos({10,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({10,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX  | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canRedo());


    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(), CharPos({10,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({10,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

}

void TestQSyneditCpp::test_backspace_selection_in_empty_file()
{
    clearContent();
    mEdit->selectAll();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QVERIFY(mEdit->empty());
}

void TestQSyneditCpp::test_backspace_while_select_all()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    mEdit->setContent(text1);

    mEdit->setCaretPos(CharPos{3,2});
    clearSignalDatas();
    mEdit->selectAll();
    QCOMPARE(mEdit->caretPos(), CharPos({3,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({3}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                      | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({3,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));
    QCOMPARE(mInsertStartLines, QList<int>({0}));
    QCOMPARE(mInsertLineCounts, QList<int>({3}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({3,1,1,1,1}));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{""});
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({3}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                      | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({3,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));
    QCOMPARE(mInsertStartLines, QList<int>({0}));
    QCOMPARE(mInsertLineCounts, QList<int>({3}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({3,1,1,1,1}));
}

void TestQSyneditCpp::test_input_char_while_select_all()
{
    QStringList text1({
                          "#include <iostream>",
                          "int main() {",
                          " return 0;",
                          "}"
                      });
    mEdit->setContent(text1);

    mEdit->setCaretPos(CharPos{3,2});
    clearSignalDatas();
    mEdit->selectAll();
    QCOMPARE(mEdit->caretPos(), CharPos({3,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(), 'a');
    QCOMPARE(mEdit->content(),QStringList{"a"});
    QCOMPARE(mEdit->caretPos(), CharPos({1,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({3}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                      | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({3,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));
    QCOMPARE(mInsertStartLines, QList<int>({0}));
    QCOMPARE(mInsertLineCounts, QList<int>({3}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0,0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,3,1,1,1,1}));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"a"});
    QCOMPARE(mEdit->caretPos(), CharPos({1,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({3}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                      | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({3,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));
    QCOMPARE(mInsertStartLines, QList<int>({0}));
    QCOMPARE(mInsertLineCounts, QList<int>({3}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection
                                      | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0,0,1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,3,1,1,1,1}));
}

void TestQSyneditCpp::test_toggle_comment_in_empty_file()
{
    clearContent();

    mEdit->processCommand(EditCommand::ToggleComment);
    mEdit->processCommand(EditCommand::ToggleComment);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_toggle_comment_at_file_begin()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "//int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };

    mEdit->setContent(text1);

    //comment at doc begin and no selection
    mEdit->setCaretPos(CharPos{0,0});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({2,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),3);

    mEdit->setCaretPos(CharPos{0,0});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),4);

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),3);

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),4);

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({2,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),3);

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),4);

    QVERIFY(!mEdit->canRedo());

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),3);

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),4);

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_toggle_comment_at_file_end()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "//}"
    };

    mEdit->setContent(text1);
    mEdit->setCaretPos(CharPos{1,11});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({3,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({3,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({3,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({11}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),3);

    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretX | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({11}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),4);

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({3,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({3,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({3,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretX | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({11}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),3);

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretX | StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({11}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),4);

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({3,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({3,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({3,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({11}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),3);

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretX | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({11}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),4);

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({3,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({3,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({3,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretX | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({11}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),3);

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                       StatusChange::CaretX | StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({11}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),4);

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_toggle_comment_select_line_end_to_line_begin()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        "// ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    mEdit->setCaretAndSelection(CharPos{0,2},CharPos{9,0},CharPos{0,2});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged,
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_toggle_comment_select_line_begin_to_line_end()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        "// ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    mEdit->setCaretAndSelection(CharPos{5,1},CharPos{0,1},CharPos{5,1});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({7,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({7,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QList<int>({
                             StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Selection
                         });
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({7,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                     StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QList<int>({
                             StatusChange::Modified |StatusChange::CaretX | StatusChange::Selection
                         });
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({7,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QList<int>({
                             StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Selection
                         });
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_toggle_comment_select_line_begin_to_line_begin()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        "// ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    mEdit->setCaretAndSelection(CharPos{0,2},CharPos{0,1},CharPos{0,2});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QList<int>({
                             StatusChange::ModifyChanged | StatusChange::Selection
                         });
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                     StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QList<int>({
                             StatusChange::Modified |StatusChange::Selection
                         });
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QList<int>({
                             StatusChange::ModifyChanged | StatusChange::Selection
                         });
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_toggle_comment_select_line_end_to_line_end()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        "// ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    mEdit->setCaretAndSelection(CharPos{5,1},CharPos{9,0},CharPos{5,1});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({7,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({7,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                             StatusChange::ModifyChanged |StatusChange::CaretX | StatusChange::Selection
                         }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({7,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                     StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QList<int>({
                             StatusChange::Modified | StatusChange::CaretX | StatusChange::Selection
                         });
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({7,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                             StatusChange::ModifyChanged |StatusChange::CaretX | StatusChange::Selection
                         }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_toggle_comment_select_multiple_lines()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        "// ttt;",
        "//}",
        "//int main() {",
        "//\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text3{
        "int t() {",
        "// ttt;",
        "//}",
        "//int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };

    mEdit->setContent(text1);
    mEdit->setCaretAndSelection(CharPos{2,4},CharPos{1,1},CharPos{2,4});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({4,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({3,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({4,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1,2,3,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,5}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    mEdit->setCaretAndSelection(CharPos{6,4},CharPos{1,4},CharPos{7,4});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(), CharPos({4,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,4}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::CaretX | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    mEdit->setCaretAndSelection(CharPos{0,2},CharPos{3,1},CharPos{1,3});
    clearSignalDatas();
    mEdit->toggleComment();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,6}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({3,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,1}));
    QCOMPARE(mReparseCounts, QList<int>({6,7,1}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({6,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,4}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({2,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4,3,2,1}));
    QCOMPARE(mReparseCounts, QList<int>({1,9,10,1}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({4,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({3,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({4,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1,2,3,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,10,9,1}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(), CharPos({4,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,4}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,7,6}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({3,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,1}));
    QCOMPARE(mReparseCounts, QList<int>({6,7,1}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({6,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,4}));
    QCOMPARE(mEdit->selEnd(), CharPos({7,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({2,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Selection | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4,3,2,1}));
    QCOMPARE(mReparseCounts, QList<int>({1,9,10,1}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_move_up_selection_in_empty_file()
{
    clearContent();
    mEdit->processCommand(EditCommand::MoveSelUp);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_move_up_current_line_at_file_begin()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    clearSignalDatas();
    mEdit->processCommand(EditCommand::MoveSelUp);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_move_up_current_line_at_file_end()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}",
    };
    QStringList text2{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "}",
        "\treturn 0;",
    };
    mEdit->setContent(text1);
    mEdit->setCaretPos(mEdit->fileEnd());
    clearSignalDatas();
    mEdit->processCommand(EditCommand::MoveSelUp);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({1,10}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,10}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,10}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{10});
    QCOMPARE(mLineMovedTos, QList<int>{11});
    QList<int>({
                             StatusChange::ModifyChanged |StatusChange::Modified | StatusChange::CaretY
                         });
    QCOMPARE(mReparseStarts, QList<int>({10}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{11});
    QCOMPARE(mLineMovedTos, QList<int>{10});
    QList<int>({
                             StatusChange::ModifyChanged | StatusChange::CaretY
                         });
    QCOMPARE(mReparseStarts, QList<int>({10}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({1,10}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,10}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,10}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{10});
    QCOMPARE(mLineMovedTos, QList<int>{11});
    QList<int>({
                             StatusChange::ModifyChanged | StatusChange::Modified
                         });
    QCOMPARE(mReparseStarts, QList<int>({10}));
    QCOMPARE(mReparseCounts, QList<int>({2}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,11}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{11});
    QCOMPARE(mLineMovedTos, QList<int>{10});
    QList<int>({
                             StatusChange::ModifyChanged | StatusChange::CaretY
                         });
    QCOMPARE(mReparseStarts, QList<int>({10}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_move_up_select_line_end_to_line_begin()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        " ttt;",
        "}",
        "int t() {",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    mEdit->setCaretAndSelection(CharPos{0,3},CharPos{9,0},CharPos{0,3});
    clearSignalDatas();
    mEdit->moveSelUp();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{0});
    QCOMPARE(mLineMovedTos, QList<int>{2});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,3}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{2});
    QCOMPARE(mLineMovedTos, QList<int>{0});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{0});
    QCOMPARE(mLineMovedTos, QList<int>{2});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,3}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{2});
    QCOMPARE(mLineMovedTos, QList<int>{0});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
}

void TestQSyneditCpp::test_move_up_select_line_begin_to_line_end()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        " ttt;",
        "}",
        "int t() {",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    mEdit->setCaretAndSelection(CharPos{1,2},CharPos{0,1},CharPos{1,2});
    clearSignalDatas();
    mEdit->moveSelUp();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({1,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{0});
    QCOMPARE(mLineMovedTos, QList<int>{2});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{2});
    QCOMPARE(mLineMovedTos, QList<int>{0});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({1,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{0});
    QCOMPARE(mLineMovedTos, QList<int>{2});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{2});
    QCOMPARE(mLineMovedTos, QList<int>{0});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_move_up_select_line_begin_to_line_begin()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        " ttt;",
        "}",
        "int t() {",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    mEdit->setCaretAndSelection(CharPos{0,3},CharPos{0,1},CharPos{0,3});
    clearSignalDatas();
    mEdit->moveSelUp();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{0});
    QCOMPARE(mLineMovedTos, QList<int>{2});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,3}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{2});
    QCOMPARE(mLineMovedTos, QList<int>{0});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{0});
    QCOMPARE(mLineMovedTos, QList<int>{2});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,3}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{2});
    QCOMPARE(mLineMovedTos, QList<int>{0});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_move_up_select_line_end_to_line_end()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        " ttt;",
        "}",
        "int t() {",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    mEdit->setContent(text1);
    mEdit->setCaretAndSelection(CharPos{1,2},CharPos{9,0},CharPos{1,2});
    clearSignalDatas();
    mEdit->moveSelUp();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({1,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{0});
    QCOMPARE(mLineMovedTos, QList<int>{2});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{2});
    QCOMPARE(mLineMovedTos, QList<int>{0});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({1,1}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,1}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{0});
    QCOMPARE(mLineMovedTos, QList<int>{2});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({1,2}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,2}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{2});
    QCOMPARE(mLineMovedTos, QList<int>{0});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({12}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_move_up_after_collapsed_block()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "\t();",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t();",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };


    mEdit->setContent(text1);

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    mEdit->collapse(5,7);
    mEdit->collapse(10,12);
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->setCaretPos(CharPos{0,8});
    clearSignalDatas();
    mEdit->moveSelUp();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,7}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,7}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,7}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{7});
    QCOMPARE(mLineMovedTos, QList<int>{8});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({7}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(5,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(5,8); // caret changed here
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,8}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,8}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,8}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{8});
    QCOMPARE(mLineMovedTos, QList<int>{7});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({7}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(5,7);
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,7}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,7}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,7}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{7});
    QCOMPARE(mLineMovedTos, QList<int>{8});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({7}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,8));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(5,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(5,8); // caret changed here
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,8));
    QVERIFY(mEdit->isCollapsed(10,12));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,8}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,8}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,8}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{8});
    QCOMPARE(mLineMovedTos, QList<int>{7});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({7}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(5,7);
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));
}

void TestQSyneditCpp::test_move_down_select_line_end_to_line_begin()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        "int main() {",
        " ttt;",
        "}",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };

    mEdit->setContent(text1);

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    mEdit->setCaretAndSelection(CharPos{0,3},CharPos{9,0},CharPos{0,3});
    clearSignalDatas();
    mEdit->moveSelDown();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({12,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{3});
    QCOMPARE(mLineMovedTos, QList<int>{1});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({8}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,3}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{1});
    QCOMPARE(mLineMovedTos, QList<int>{3});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({8}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({0,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({12,1}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{3});
    QCOMPARE(mLineMovedTos, QList<int>{1});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({8}));

    QVERIFY(!mEdit->canRedo());

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,8));
    QVERIFY(mEdit->hasCodeBlock(1,3));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,3}));
    QCOMPARE(mEdit->selBegin(), CharPos({9,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,3}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{1});
    QCOMPARE(mLineMovedTos, QList<int>{3});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretY | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({8}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));
}

void TestQSyneditCpp::test_move_down_select_line_end_to_line_begin2()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        "int test() {",
        "\treturn 0;",
        " }"
    };
    QStringList text2{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "}",
        " }",
        "int test() {",
        "\treturn 0;",
    };

    mEdit->setContent(text1);

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    mEdit->setCaretAndSelection(CharPos{0,11},CharPos{1,8},CharPos{0,11});
    clearSignalDatas();
    mEdit->moveSelDown();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({10,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,9}));
    QCOMPARE(mEdit->selEnd(), CharPos({10,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{11});
    QCOMPARE(mLineMovedTos, QList<int>{9});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({9}));
    QCOMPARE(mReparseCounts, QList<int>({3}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,8}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{9});
    QCOMPARE(mLineMovedTos, QList<int>{11});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({9}));
    QCOMPARE(mReparseCounts, QList<int>({3}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({10,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({2,9}));
    QCOMPARE(mEdit->selEnd(), CharPos({10,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{11});
    QCOMPARE(mLineMovedTos, QList<int>{9});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretX | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({9}));
    QCOMPARE(mReparseCounts, QList<int>({3}));

    QCOMPARE(mEdit->codeBlockCount(),3);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({0,11}));
    QCOMPARE(mEdit->selBegin(), CharPos({1,8}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,11}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{9});
    QCOMPARE(mLineMovedTos, QList<int>{11});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::Selection
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({9}));
    QCOMPARE(mReparseCounts, QList<int>({3}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,8));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(9,11));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditCpp::test_move_down_before_collapsed_block()
{
    QStringList text1{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tint x;",
        "\tif (x>0) {",
        "\t\tx++;",
        "\t}",
        "\t();",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };
    QStringList text2{
        "int t() {",
        " ttt;",
        "}",
        "int main() {",
        "\tif (x>0) {",
        "\tint x;",
        "\t\tx++;",
        "\t}",
        "\t();",
        "}",
        "int test() {",
        "\treturn 0;",
        "}"
    };


    mEdit->setContent(text1);

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    mEdit->collapse(5,7);
    mEdit->collapse(10,12);
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->setCaretPos(CharPos{5,4});
    clearSignalDatas();
    mEdit->moveSelDown();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({5,5}));
    QCOMPARE(mEdit->selBegin(), CharPos({5,5}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,5}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{5});
    QCOMPARE(mLineMovedTos, QList<int>{4});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(4,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(4,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(4,7); // caret changed here
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(4,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({5,4}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{4});
    QCOMPARE(mLineMovedTos, QList<int>{5});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(5,7);
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretPos(), CharPos({5,5}));
    QCOMPARE(mEdit->selBegin(), CharPos({5,5}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,5}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{5});
    QCOMPARE(mLineMovedTos, QList<int>{4});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::Modified | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(4,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(4,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(4,7); // caret changed here
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(4,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    QVERIFY(!mEdit->canRedo());

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretPos(), CharPos({5,4}));
    QCOMPARE(mEdit->selBegin(), CharPos({5,4}));
    QCOMPARE(mEdit->selEnd(), CharPos({5,4}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{4});
    QCOMPARE(mLineMovedTos, QList<int>{5});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::ModifyChanged | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({2}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    QCOMPARE(mEdit->codeBlockCount(),4);
    QVERIFY(mEdit->hasCodeBlock(0,2));
    QVERIFY(mEdit->hasCodeBlock(3,9));
    QVERIFY(mEdit->hasCodeBlock(5,7));
    QVERIFY(mEdit->hasCodeBlock(10,12));
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(!mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));

    mEdit->collapse(5,7);
    QVERIFY(!mEdit->isCollapsed(0,2));
    QVERIFY(!mEdit->isCollapsed(3,9));
    QVERIFY(mEdit->isCollapsed(5,7));
    QVERIFY(mEdit->isCollapsed(10,12));
}

void TestQSyneditCpp::test_copy_paste_rawstring()
{
    QStringList text({
        "int main() {",
        "const char* s = R\"(",
        "aa",
        "\t\tab",
        "  aa",
        ")\";",
        "int x=0;",
        "}"
    });
    QStringList text1({
        "int main() {",
        "\tconst char* s = R\"(",
        "aa",
        "\t\tab",
        "  aa",
        ")\";",
        "\tint x=0;",
        "}"
    });
    QSynEdit edit;
    edit.setContent(text);
    QCOMPARE(edit.content(), text);
    edit.selectAll();
    edit.copyToClipboard();

    clearContent();
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QVERIFY(mEdit->empty());

    mEdit->redo();
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QCOMPARE(mEdit->content(), QStringList(""));
}

void TestQSyneditCpp::test_copy_paste_ending_with_backslash()
{
    QStringList text({
        "int main() {",
        "const char* s = \"\\",
        "aa\\",
        "\t\tab\\",
        "  aa\\",
        "\";",
        "int x=0;",
        "}"
    });
    QStringList text1({
        "int main() {",
        "\tconst char* s = \"\\",
        "aa\\",
        "\t\tab\\",
        "  aa\\",
        "\";",
        "\tint x=0;",
        "}"
    });
    QSynEdit edit;
    edit.setContent(text);
    QCOMPARE(edit.content(), text);
    edit.selectAll();
    edit.copyToClipboard();

    clearContent();
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QVERIFY(mEdit->empty());

    mEdit->redo();
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QCOMPARE(mEdit->content(), QStringList(""));
}

void TestQSyneditCpp::test_cut_selection()
{
    QStringList text({
        "int main() {",
        "    return 0;",
        "}"
    });
    QStringList textCut({
        "int main() {",
        "     0;",
        "}"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{10,1}, CharPos{4,1}, CharPos{10,1});
    mEdit->cutToClipboard();
    QCOMPARE(mEdit->content(), textCut);
    QCOMPARE(QApplication::clipboard()->text(), QString("return"));
    QCOMPARE(mEdit->caretPos(), CharPos({4,1}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textCut);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textCut);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    mEdit->setCaretAndSelection(CharPos{10,1}, CharPos{4,1}, CharPos{10,1});
    mEdit->cutToClipboard();
    QCOMPARE(mEdit->content(), textCut);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_cut_current_line_without_selection()
{
    QStringList text({
        "int main() {",
        "    return 0;",
        "}"
    });
    QStringList textCut({
        "int main() {",
        "}"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretPos(CharPos{2,1});
    clearSignalDatas();
    mEdit->cutToClipboard();
    QCOMPARE(mEdit->content(), textCut);
    QVERIFY(QApplication::clipboard()->text().contains("    return 0;"));
    QCOMPARE(mDeleteStartLines, QList<int>({1}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textCut);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textCut);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    mEdit->setCaretPos(CharPos{2,1});
    mEdit->cutToClipboard();
    QCOMPARE(mEdit->content(), textCut);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_paste_replaces_selection()
{
    QStringList text({
        "int main() {",
        "    return 0;",
        "}"
    });
    QStringList textPasted({
        "int main() {",
        "    1 0;",
        "}"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setOptions(mEdit->getOptions() & ~EditorOptions(EditorOption::AutoIndent));
    QApplication::clipboard()->setText("1");
    mEdit->setCaretAndSelection(CharPos{10,1}, CharPos{4,1}, CharPos{10,1});
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), textPasted);
    QCOMPARE(mEdit->caretPos(), CharPos({5,1}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    mEdit->setCaretAndSelection(CharPos{10,1}, CharPos{4,1}, CharPos{10,1});
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), textPasted);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_grouped_editing_undo_redo()
{
    QStringList text({
        "int main() {",
        "}"
    });
    QStringList textEdited({
        "int main() {x",
        "y}"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretPos(CharPos{12,0});
    mEdit->beginEditing();
    QTest::keyPress(mEdit.get(),'x');
    mEdit->setCaretPos(CharPos{0,1});
    QTest::keyPress(mEdit.get(),'y');
    mEdit->endEditing();
    QCOMPARE(mEdit->content(), textEdited);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    QCOMPARE(mEdit->caretPos(), CharPos({12,0}));
    mEdit->redo();
    QCOMPARE(mEdit->content(), textEdited);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textEdited);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    mEdit->beginEditing();
    QTest::keyPress(mEdit.get(),'q');
    mEdit->endEditing();
    QCOMPARE(mEdit->content(), textEdited);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_nested_grouped_editing()
{
    QStringList text({
        "int main() {"
    });
    QStringList textEdited({
        "int main() {xyz"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretPos(CharPos{12,0});
    QVERIFY(!mEdit->editing());
    mEdit->beginEditing();
    QTest::keyPress(mEdit.get(),'x');
    mEdit->beginEditing();
    QVERIFY(mEdit->editing());
    QTest::keyPress(mEdit.get(),'y');
    mEdit->endEditing();
    QVERIFY(mEdit->editing());
    QTest::keyPress(mEdit.get(),'z');
    mEdit->endEditing();
    QVERIFY(!mEdit->editing());
    QCOMPARE(mEdit->content(), textEdited);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textEdited);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textEdited);
}

void TestQSyneditCpp::test_grouped_editing_keeps_selection_mode()
{
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    mEdit->beginEditing();
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    mEdit->endEditing();
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
}

void TestQSyneditCpp::test_column_select_content()
{
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QVERIFY(mEdit->hasSelContent());
    QCOMPARE(mEdit->selContent(), QStringList({"bc","gh","lm"}));
    // in normal mode, selection content == selection state
    mEdit->setActiveSelectionMode(SelectionMode::Normal);
    QVERIFY(mEdit->hasSelContent());
    // a zero-width column selection exists, but has no content
    mEdit->setCaretAndSelection(CharPos{1,2}, CharPos{1,0}, CharPos{1,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QVERIFY(mEdit->selAvail());
    QVERIFY(!mEdit->hasSelContent());
    QCOMPARE(mEdit->selText(), QString());
    QCOMPARE(mEdit->selContent(), QStringList());
}

void TestQSyneditCpp::test_column_input_char()
{
    // typing with a column band replaces the band's content on every line
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    QStringList textInput({
        "axde",
        "fxij",
        "kxno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),'x');
    QCOMPARE(mEdit->content(), textInput);
    // same string on every line, so the column cursor stays aligned:
    // keep column mode with the cursor after the inserted text
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({2,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,2}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    QTest::keyPress(mEdit.get(),'y');
    QCOMPARE(mEdit->content(), textInput);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_column_input_at_column_cursor()
{
    // typing at a zero-width column selection (column cursor) inserts
    // the string at the cursor column on every line
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    QStringList textInput({
        "axbcde",
        "fxghij",
        "kxlmno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{1,2}, CharPos{1,0}, CharPos{1,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),'x');
    QCOMPARE(mEdit->content(), textInput);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({2,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,2}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);
}

void TestQSyneditCpp::test_column_backspace()
{
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    QStringList textDeleted({
        "ade",
        "fij",
        "kno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QCOMPARE(mEdit->content(), textDeleted);
    // column deletion leaves a zero-width column selection (column cursor)
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
    QCOMPARE(mEdit->content(), textDeleted);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_column_cut()
{
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    QStringList textCut({
        "ade",
        "fij",
        "kno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    mEdit->cutToClipboard();
    QCOMPARE(mEdit->content(), textCut);
    // column deletion leaves a zero-width column selection (column cursor)
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QString clipboardText = QApplication::clipboard()->text();
    clipboardText.replace("\r\n","\n");
    QCOMPARE(clipboardText, QString("bc\ngh\nlm"));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textCut);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textCut);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    mEdit->cutToClipboard();
    QCOMPARE(mEdit->content(), textCut);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_column_mode_alt_selection()
{
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretPos(CharPos{1,0});
    QTest::keyPress(mEdit.get(),Qt::Key_Down,Qt::AltModifier|Qt::ShiftModifier);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({1,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,1}));
    QTest::keyPress(mEdit.get(),Qt::Key_Down,Qt::AltModifier|Qt::ShiftModifier);
    QCOMPARE(mEdit->selBegin(), CharPos({1,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,2}));
    QTest::keyPress(mEdit.get(),Qt::Key_Right,Qt::AltModifier|Qt::ShiftModifier);
    QVERIFY(mEdit->hasSelContent());
    QCOMPARE(mEdit->selEnd(), CharPos({2,2}));
    QCOMPARE(mEdit->selContent(), QStringList({"b","g","l"}));

    QTest::keyPress(mEdit.get(),Qt::Key_Right,Qt::ShiftModifier);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Normal);
}

void TestQSyneditCpp::test_column_delete_char()
{
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    QStringList textDeleted({
        "ade",
        "fij",
        "kno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(), textDeleted);
    // column deletion leaves a zero-width column selection (column cursor)
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    // undo restores the pre-edit state: column mode with the selection
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({1,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({3,2}));
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted);
    // redo restores the post-edit state: column mode with a zero-width
    // column selection (column cursor)
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(), textDeleted);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_column_overwrite_input()
{
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    QStringList textInput({
        "axde",
        "fxij",
        "kxno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setInsertMode(false);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),'x');
    // overwrite mode with a band: the typed text replaces the band,
    // just like in insert mode
    QCOMPARE(mEdit->content(), textInput);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({2,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,2}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);
}

void TestQSyneditCpp::test_column_overwrite_at_column_cursor()
{
    // overwrite mode at a zero-width column selection: overwrite the
    // glyph under the cursor on every line
    QStringList text({
        "abcde",
        "fghij",
        "klmno"
    });
    QStringList textInput({
        "axcde",
        "fxhij",
        "kxmno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setInsertMode(false);
    mEdit->setCaretAndSelection(CharPos{1,2}, CharPos{1,0}, CharPos{1,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),'x');
    QCOMPARE(mEdit->content(), textInput);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({2,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,2}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);
}

void TestQSyneditCpp::test_column_overwrite_at_eol()
{
    // overwriting with the cursor at a short line's end falls back to
    // appending on that line
    QStringList text({
        "abcde",
        "fg",
        "klmno"
    });
    QStringList textInput({
        "abxde",
        "fgx",
        "klxno"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setInsertMode(false);
    mEdit->setCaretAndSelection(CharPos{2,2}, CharPos{2,0}, CharPos{2,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),'x');
    QCOMPARE(mEdit->content(), textInput);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({3,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({3,2}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textInput);
}

void TestQSyneditCpp::test_column_select_content_cjk()
{
    clearContent();
    mEdit->setContent({"abcde","ab中文de","klmno"});
    mEdit->setOptions(mDefaultOptions | QSynedit::EditorOption::ForceMonospace);
    // select ch1..ch3 on the ASCII lines: the band is x = [1,3) cells
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QVERIFY(mEdit->hasSelContent());
    // line 1 layout: a[0,1) b[1,2) 中[2,4) 文[4,6) d[6,7) e[7,8)
    // x=1 -> 'b' (ch1); x=3 falls inside 中 -> snaps left to its start (ch2)
    QCOMPARE(mEdit->selContent(), QStringList({"bc","b","lm"}));
}

void TestQSyneditCpp::test_column_delete_cjk()
{
    QStringList text({"abcde","ab中文de","klmno"});
    QStringList textDeleted({"ade","a中文de","kno"});
    clearContent();
    mEdit->setContent(text);
    mEdit->setOptions(mDefaultOptions | QSynedit::EditorOption::ForceMonospace);
    // band x=[1,3): on line 1 only 'b' is removed; 中[2,4) is spared by the left snap
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(), textDeleted);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted);

    // band x=[1,4): its right edge is exactly the start of 文, so 中 is fully covered
    QStringList textDeleted2({"ae","a文de","ko"});
    mEdit->undo();
    mEdit->setCaretAndSelection(CharPos{4,2}, CharPos{1,0}, CharPos{4,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(), textDeleted2);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted2);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(), textDeleted2);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_column_delete_tab_snap()
{
    // default tabSize is 4
    QStringList text({"abcde","a\tbcde","klmno"});
    clearContent();
    mEdit->setContent(text);
    mEdit->setOptions(mDefaultOptions | QSynedit::EditorOption::ForceMonospace);
    // line 1 layout: a[0,1) \t[1,4) b[4,5) c[5,6) d[6,7) e[7,8)
    // band x=[1,4): the right edge is exactly the start of 'b', only the tab is removed
    QStringList textDeleted({"ae","abcde","ko"});
    mEdit->setCaretAndSelection(CharPos{4,2}, CharPos{1,0}, CharPos{4,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(), textDeleted);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted);

    // band x=[2,3) lies fully inside the tab: line 1 is left untouched
    QStringList textDeleted2({"abde","a\tbcde","klno"});
    mEdit->undo();
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{2,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(), textDeleted2);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted2);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QCOMPARE(mEdit->content(), textDeleted2);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_column_has_content_cjk_layout()
{
    // the content check only examines the begin/end lines' display x (design doc 8.1)
    clearContent();
    mEdit->setContent({"abcde","中lmno"});
    mEdit->setOptions(mDefaultOptions | QSynedit::EditorOption::ForceMonospace);
    // same ch on both lines, but 中 is 2 cells wide:
    // begin x = 1 cell, end x = 2 cells -> the band has content
    mEdit->setCaretAndSelection(CharPos{1,1}, CharPos{1,0}, CharPos{1,1});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QVERIFY(mEdit->hasSelContent());
    // line 0: x=[1,2) -> "b"; line 1: x=1 falls inside 中[0,2) -> snaps to ch0,
    // x=2 is the start of 'l' -> content is "中"
    QCOMPARE(mEdit->selContent(), QStringList({"b","中"}));
}

void TestQSyneditCpp::test_column_delete_prop_font()
{
    // cover the forceMonospace=false path with the bundled proportional
    // test font (deterministic but non-uniform glyph widths)
    QFont propFont(testProportionalFont());
    propFont.setPointSize(14);
    mEdit->setFont(propFont);
    auto restoreFont = finally([this](){
        QFont monoFont(defaultMonoFont());
        monoFont.setPointSize(14);
        mEdit->setFont(monoFont);
    });
    QStringList text({"abcde","fghij","klmno"});
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QTest::keyPress(mEdit.get(),Qt::Key_Delete);
    QStringList textDeleted = mEdit->content();
    // with non-uniform widths the snapped boundaries differ per line,
    // so don't assert exact contents; column deletion must remove one
    // contiguous span per line, preserving what is left of the band
    QVERIFY(textDeleted != text);
    for (int i=0;i<text.size();i++) {
        const QString &orig = text[i];
        const QString &cur = textDeleted[i];
        int removed = orig.length()-cur.length();
        QVERIFY(removed >= 0);
        int prefix = 0;
        while (prefix < cur.length() && cur[prefix]==orig[prefix])
            prefix++;
        QCOMPARE(orig.mid(prefix+removed), cur.mid(prefix));
    }
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textDeleted);
}

void TestQSyneditCpp::test_column_paste_single_line()
{
    // a single-line clipboard is broadcast to every line of the band
    QStringList text({"abcde","fghij","klmno"});
    QStringList textPasted({"aXde","fXij","kXno"});
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QApplication::clipboard()->setText("X");
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), textPasted);
    // pasted widths are equal: stay in column mode with the column
    // cursor after the pasted text
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({2,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,2}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), textPasted);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_column_paste_multi_line()
{
    // each clipboard line goes to the corresponding selection line
    QStringList text({"abcde","fghij","klmno"});
    QStringList textPasted({"a11de","f22ij","k33no"});
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QApplication::clipboard()->setText("11\n22\n33");
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), textPasted);
    // equal pasted widths: stay in column mode, column cursor after
    // the pasted text
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({3,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({3,2}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);

    // fewer clipboard lines than selection lines: repeat the last one
    QStringList textPasted2({"a11de","f22ij","k22no"});
    mEdit->undo();
    QApplication::clipboard()->setText("11\n22");
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), textPasted2);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted2);
}

void TestQSyneditCpp::test_column_paste_multi_line_unequal()
{
    // pasted lines with different display widths: no aligned column
    // cursor exists, so leave column mode with a normal caret after
    // the pasted text on the caret's line
    QStringList text({"abcde","fghij","klmno"});
    QStringList textPasted({"a1de","f22ij","k333no"});
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{3,2}, CharPos{1,0}, CharPos{3,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QApplication::clipboard()->setText("1\n22\n333");
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), textPasted);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Normal);
    QCOMPARE(mEdit->caretPos(), CharPos({2,0}));
    QVERIFY(!mEdit->selAvail());

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);
}

void TestQSyneditCpp::test_column_paste_short_line()
{
    // lines shorter than the band are padded with spaces up to the column
    QStringList text({"abcde","fg","klmno"});
    QStringList textPasted({"abcX","fg X","klmX"});
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{5,2}, CharPos{3,0}, CharPos{5,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QApplication::clipboard()->setText("X");
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), textPasted);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({4,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({4,2}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);
}

void TestQSyneditCpp::test_column_paste_at_column_cursor()
{
    // pasting at a zero-width column selection (column cursor) inserts
    // on every line without deleting anything
    QStringList text({"abcde","fghij","klmno"});
    QStringList textPasted({"aXbcde","fXghij","kXlmno"});
    clearContent();
    mEdit->setContent(text);
    mEdit->setCaretAndSelection(CharPos{1,2}, CharPos{1,0}, CharPos{1,2});
    mEdit->setActiveSelectionMode(SelectionMode::Column);
    QApplication::clipboard()->setText("X");
    mEdit->pasteFromClipboard();
    QCOMPARE(mEdit->content(), textPasted);
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Column);
    QCOMPARE(mEdit->selBegin(), CharPos({2,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({2,2}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textPasted);
}

void TestQSyneditCpp::test_overwrite_input_with_selection()
{
    QStringList text({
        "int main() {",
        "}"
    });
    QStringList textEdited({
        "int x() {",
        "}"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setInsertMode(false);
    mEdit->setCaretAndSelection(CharPos{8,0}, CharPos{4,0}, CharPos{8,0});
    QTest::keyPress(mEdit.get(),'x');
    QCOMPARE(mEdit->content(), textEdited);
    QCOMPARE(mEdit->caretPos(), CharPos({5,0}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textEdited);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textEdited);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    mEdit->setCaretAndSelection(CharPos{8,0}, CharPos{4,0}, CharPos{8,0});
    QTest::keyPress(mEdit.get(),'y');
    QCOMPARE(mEdit->content(), textEdited);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_no_auto_indent_break_line()
{
    QStringList text({
        "int main() {",
        "}"
    });
    QStringList textBroken({
        "int main() {",
        "",
        "}"
    });
    clearContent();
    mEdit->setContent(text);
    mEdit->setOptions(mEdit->getOptions() & ~EditorOptions(EditorOption::AutoIndent));
    mEdit->setCaretPos(CharPos{12,0});
    QTest::keyPress(mEdit.get(),Qt::Key_Return);
    QCOMPARE(mEdit->content(), textBroken);
    QCOMPARE(mEdit->caretPos(), CharPos({0,1}));

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textBroken);
    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
    mEdit->redo();
    QCOMPARE(mEdit->content(), textBroken);

    mEdit->setModified(false);
    mEdit->setReadOnly(true);
    mEdit->setCaretPos(CharPos{12,0});
    QTest::keyPress(mEdit.get(),Qt::Key_Return);
    QCOMPARE(mEdit->content(), textBroken);
    QVERIFY(!mEdit->modified());
    mEdit->setReadOnly(false);
}

void TestQSyneditCpp::test_setseltext_and_indent()
{
    QStringList text({
        "int main() {",
        "\tif (x>0)",
        "\t\tx=10;",
        "}",
    });
    QStringList text1({
        "int main() {",
        "\tif (x>0)",
        "\t{}x=10;",
        "}",
    });
    mEdit->setContent(text);
    mEdit->setCaretPos({2,2});
    mEdit->setSelText("{}");
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);

    mEdit->redo();
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
}

void TestQSyneditCpp::test_setseltext_and_indent2()
{
    QStringList text({
        "int main() {",
        "\tif (x>0)",
        "\t\tx=10;",
        "}",
    });
    QStringList text1({
        "int main() {",
        "\tif (x>0)",
        "\t{",
        "\t\ttest",
        "\t}x=10;",
        "}",
    });
    mEdit->setContent(text);
    mEdit->setCaretPos({2,2});
    mEdit->setSelText("{\ntest\n}");
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);

    mEdit->redo();
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
}

void TestQSyneditCpp::test_setseltext_and_indent3_line_comment()
{
    QStringList text({
        "int main() {",
        "\tint xxx;",
        "\t",
        "\tint yyy;",
        "}",
    });
    QStringList text1({
        "int main() {",
        "\tint xxx;",
        "\t// test",
        "\tint yyy;",
        "}",
    });
    mEdit->setContent(text);
    mEdit->setCaretPos({1,2});
    mEdit->setSelText("// test");
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);

    mEdit->redo();
    QCOMPARE(mEdit->content(), text1);

    mEdit->undo();
    QCOMPARE(mEdit->content(), text);
}

void TestQSyneditCpp::test_auto_indent_for_parenthesis()
{
    QStringList text({
        "(",
        " (",
        "  (",
        "   (",
        "\t(",
        "\t (",
    });
    QStringList text1({
        "(",
    });
    QStringList text2({
        "(",
        " (",
    });
    QStringList text3({
        "(",
        " (",
        "  (",
    });
    QStringList text4({
        "(",
        " (",
        "  (",
        "   (",
    });
    QStringList text5({
        "(",
        " (",
        "  (",
        "   (",
        "\t(",
    });
    clearContent();
    QTest::keyPress(mEdit.get(),'(');
    QCOMPARE(mEdit->content(),text1);
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QTest::keyPress(mEdit.get(),'(');
    QCOMPARE(mEdit->content(),text2);
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QTest::keyPress(mEdit.get(),'(');
    QCOMPARE(mEdit->content(),text3);
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QTest::keyPress(mEdit.get(),'(');
    QCOMPARE(mEdit->content(),text4);
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QTest::keyPress(mEdit.get(),'(');
    QCOMPARE(mEdit->content(),text5);
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QTest::keyPress(mEdit.get(),'(');
    QCOMPARE(mEdit->content(),text);

    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text5);
    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text4);
    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    mEdit->undo();
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->canUndo());

    mEdit->redo();
    QCOMPARE(mEdit->content(),text1);
    mEdit->redo();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    mEdit->redo();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    mEdit->redo();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text4);
    mEdit->redo();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text5);
    mEdit->redo();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text);
    QVERIFY(!mEdit->canRedo());

    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text5);
    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text4);
    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    mEdit->undo();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    mEdit->undo();
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->canUndo());
}

void TestQSyneditCpp::test_auto_indent_for_brace_after_closing_parenthesis()
{
    QStringList text({
        "void test() {",
        "",
        "}",
    });
    QStringList text1({
        "void test() {",
        "\t{",
        "}",
    });
    mEdit->setContent(text);
    mEdit->setCaretPos({0,1});
    QTest::keyPress(mEdit.get(),'{');
    QCOMPARE(mEdit->content(),text1);
}

void TestQSyneditCpp::test_auto_indent_for_if_else()
{
    QStringList text({
        "int main {",
        "if () {",
        "x=10;",
        "} else {",
        "y=10",
        "}",
        "}"
    });
    QStringList text1({
        "int main {",
        "\tif () {",
        "\t\tx=10;",
        "\t} else {",
        "\t\ty=10",
        "\t}",
        "}"
    });
    mEdit->clear();
    mEdit->setSelText(text.join("\n"));
    QCOMPARE(mEdit->content(),text1);
}

void TestQSyneditCpp::test_auto_indent_for_if_else_2()
{
    QStringList text({
        "int main {",
        "if ()",
        "x=10;",
        "else",
        "y=10;",
        "z=10;",
        "}"
    });
    QStringList text1({
        "int main {",
        "\tif ()",
        "\t\tx=10;",
        "\telse",
        "\t\ty=10;",
        "\tz=10;",
        "}"
    });
    mEdit->clear();
    mEdit->setSelText(text.join("\n"));
    QCOMPARE(mEdit->content(),text1);
}

void TestQSyneditCpp::test_auto_indent_for_if_else_3()
{
    QStringList text({
        "int main {",
        "if ()",
        "if ()",
        "x=10;",
        "else",
        "y=10;",
        "z=10;"
        "}"
    });
    QStringList text1({
        "int main {",
        "\tif ()",
        "\t\tif ()",
        "\t\t\tx=10;",
        "\t\telse",
        "\t\t\ty=10;",
        "\tz=10;"
        "}"
    });
    mEdit->clear();
    mEdit->setSelText(text.join("\n"));
    QCOMPARE(mEdit->content(),text1);
}

}

