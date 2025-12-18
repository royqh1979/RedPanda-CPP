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

void TestQSyneditCpp::onLinesDeleted(int line, int count)
{
    mDeleteStartLines.append(line);
    mDeleteLineCounts.append(count);
}

void TestQSyneditCpp::onLinesInserted(int line, int count)
{
    mInsertStartLines.append(line);
    mInsertLineCounts.append(count);
}

void TestQSyneditCpp::onLineMoved(int from, int to)
{
    mLineMovedFroms.append(from);
    mLineMovedTos.append(to);
}

void TestQSyneditCpp::onStatusChanged(StatusChanges change)
{
    mStatusChanges.append(change & ~(StatusChange::TopPos) & ~(StatusChange::LeftPos));
}

void TestQSyneditCpp::onReparsed(int start, int count)
{
    mReparseStarts.append(start);
    mReparseCounts.append(count);
}

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
    mEdit->setOptions(options);
    mEdit->setSyntaxer(std::make_shared<CppSyntaxer>());
    mEdit->setFormatter(std::make_shared<CppFormatter>());

    connect(mEdit.get(), &QSynEdit::linesDeleted, this, &TestQSyneditCpp::onLinesDeleted);
    connect(mEdit.get(), &QSynEdit::linesInserted, this, &TestQSyneditCpp::onLinesInserted);
    connect(mEdit.get(), &QSynEdit::lineMoved, this, &TestQSyneditCpp::onLineMoved);
    connect(mEdit.get(), &QSynEdit::statusChanged, this, &TestQSyneditCpp::onStatusChanged);
    connect(mEdit.get(), &QSynEdit::linesReparesd, this, &TestQSyneditCpp::onReparsed);

}

void TestQSyneditCpp::clearReparseDatas()
{
    mReparseStarts.clear();
    mReparseCounts.clear();
}

void TestQSyneditCpp::clearSignalDatas()
{
    mInsertLineCounts.clear();
    mInsertStartLines.clear();
    mDeleteLineCounts.clear();
    mDeleteStartLines.clear();
    mLineMovedFroms.clear();
    mLineMovedTos.clear();
    mStatusChanges.clear();
    mReparseStarts.clear();
    mReparseCounts.clear();
}

void TestQSyneditCpp::loadDemoFile()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/queue1.cpp",ENCODING_AUTO_DETECT,encoding);

    clearSignalDatas();
}

void TestQSyneditCpp::clearContent()
{
    mEdit->processCommand(EditCommand::ClearAll);
    clearSignalDatas();
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
    QTest::addColumn<QList<StatusChanges>>("statusChanges");
    QTest::addColumn<QList<StatusChanges>>("expect_statusChange");

    clearContent();
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }

    loadDemoFile();
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward at file start")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward at file end")<<mEdit->caretXY()<<CharPos{1,76};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward at line start")<<mEdit->caretXY()<<CharPos{7,14};
        td << mStatusChanges << QList<StatusChanges>{(StatusChange::CaretX | StatusChange::CaretY)};
    }
    {
        mEdit->setCaretXY(CharPos{0,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward at line start")<<mEdit->caretXY()<<CharPos{1,15};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{40,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward at line end")<<mEdit->caretXY()<<CharPos{39,15};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{40,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward at line end")<<mEdit->caretXY()<<CharPos{0,16};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretX | StatusChange::CaretY};
    }
}

void TestQSyneditCpp::test_move_caret_x()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<StatusChanges>, statusChanges);
    QFETCH(QList<StatusChanges>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);
}

void TestQSyneditCpp::test_move_caret_y_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    QTest::addColumn<QList<StatusChanges>>("statusChanges");
    QTest::addColumn<QList<StatusChanges>>("expect_statusChange");

    clearContent();
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Up in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp);
        QTestData& td = QTest::newRow("Page Up in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageDown);
        QTestData& td = QTest::newRow("Page Down in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }

    loadDemoFile();
    //up/down
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Up at file start")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{4,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Up at first line")<<mEdit->caretXY()<<CharPos{4,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{4,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Normal Up")<<mEdit->caretXY()<<CharPos{4,14};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{10,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Up but prev line not long enough")<<mEdit->caretXY()<<CharPos{7,14};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretX | StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down at file end")<<mEdit->caretXY()<<CharPos{1,76};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down at lastline")<<mEdit->caretXY()<<CharPos{0,76};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{26,61});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Normal Down")<<mEdit->caretXY()<<CharPos{26,62};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{45,61});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down but next line not long enough")<<mEdit->caretXY()<<CharPos{37,62};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX | StatusChange::CaretY };
    }
    // page up/down
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp);
        QTestData& td = QTest::newRow("Page Up at file start")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{4,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp);
        QTestData& td = QTest::newRow("Page Up at first line")<<mEdit->caretXY()<<CharPos{4,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,75});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp);
        CharPos newPos=CharPos{0,75-mEdit->linesInWindow()};
        if (newPos.line<0)
            newPos.line = 0;
        QTestData& td = QTest::newRow("Normal Page Up")<<mEdit->caretXY()<<newPos;
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{0,1});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageDown);
        CharPos newPos=CharPos{0,1+mEdit->linesInWindow()};
        if (newPos.line > 76)
            newPos.line = 76;
        QTestData& td = QTest::newRow("Normal Page Down")<<mEdit->caretXY()<<newPos;
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageDown);
        QTestData& td = QTest::newRow("Page Down at file end")<<mEdit->caretXY()<<CharPos{1,76};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Page Down at lastline")<<mEdit->caretXY()<<CharPos{0,76};
        td << mStatusChanges << QList<StatusChanges>{};
    }
}

void TestQSyneditCpp::test_move_caret_y()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<StatusChanges>, statusChanges);
    QFETCH(QList<StatusChanges>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);

}

void TestQSyneditCpp::test_move_caret_to_line_begin_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    QTest::addColumn<QList<StatusChanges>>("statusChanges");
    QTest::addColumn<QList<StatusChanges>>("expect_statusChange");

    loadDemoFile();
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("doc begin")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("doc end")<<mEdit->caretXY()<<CharPos{0,76};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX };
    }
    {
        mEdit->setCaretXY(CharPos{0,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("begin of line")<<mEdit->caretXY()<<CharPos{4,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{2,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("in leading spaces")<<mEdit->caretXY()<<CharPos{0,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{4,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("end of leading spaces")<<mEdit->caretXY()<<CharPos{0,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{3,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("end of leading spaces - 1")<<mEdit->caretXY()<<CharPos{0,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{5,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("end of leading spaces + 1")<<mEdit->caretXY()<<CharPos{4,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{10,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart);
        QTestData& td = QTest::newRow("mid of line")<<mEdit->caretXY()<<CharPos{4,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
}

void TestQSyneditCpp::test_move_caret_to_line_begin()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<StatusChanges>, statusChanges);
    QFETCH(QList<StatusChanges>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);
}

void TestQSyneditCpp::test_move_caret_to_line_end_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    QTest::addColumn<QList<StatusChanges>>("statusChanges");
    QTest::addColumn<QList<StatusChanges>>("expect_statusChange");

    loadDemoFile();
    {
        mEdit->setCaretXY(CharPos{25,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("start of trailing spaces")<<mEdit->caretXY()<<CharPos{34,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{26,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("start of trailing spaces + 1")<<mEdit->caretXY()<<CharPos{34,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{24,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("start of trailing spaces - 1")<<mEdit->caretXY()<<CharPos{25,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{29,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("middle of trailing spaces")<<mEdit->caretXY()<<CharPos{34,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{15,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("mid of line")<<mEdit->caretXY()<<CharPos{25,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{34,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd);
        QTestData& td = QTest::newRow("end of line")<<mEdit->caretXY()<<CharPos{25,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
}

void TestQSyneditCpp::test_move_caret_to_line_end()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<StatusChanges>, statusChanges);
    QFETCH(QList<StatusChanges>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);
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
    QTest::addColumn<QList<QSynedit::StatusChanges>>("statusChanges");
    QTest::addColumn<QList<QSynedit::StatusChanges>>("expect_statusChanges");
    {
        clearContent();
        mEdit->clearSelection();
        QTestData& td = QTest::addRow("empty doc 1")<<mEdit->selBegin()<<CharPos{0,0};
        td<<mEdit->selEnd()<<CharPos{0,0};
        td<<mEdit->selText()<<"";
        td<<mEdit->selCount()<<0;
        td<<mStatusChanges<<QList<StatusChanges>{};
    }
    {
        clearContent();
        mEdit->setSelBeginEnd(mEdit->fileBegin(),mEdit->fileBegin());
        QTestData& td = QTest::addRow("empty doc 2")<<mEdit->selBegin()<<CharPos{0,0};
        td<<mEdit->selEnd()<<CharPos{0,0};
        td<<mEdit->selText()<<"";
        td<<mEdit->selCount()<<0;
        td<<mStatusChanges<<QList<StatusChanges>{};
    }
    {
        loadDemoFile();
        mEdit->setCaretAndSelection(CharPos{5,0},CharPos{0,0}, CharPos{5,0});
        QTestData& td = QTest::addRow("select in line")<<mEdit->selBegin()<<CharPos{0,0};
        td<<mEdit->selEnd()<<CharPos{5,0};
        td<<mEdit->selText()<<"#incl";
        td<<mEdit->selCount()<<5;
        td<<mStatusChanges<<QList<StatusChanges>{StatusChange::CaretX | StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setCaretAndSelection(CharPos{16,1},CharPos{0,1}, CharPos{16,1});
        QTestData& td = QTest::addRow("select from line begin to end")<<mEdit->selBegin()<<CharPos{0,1};
        td<<mEdit->selEnd()<<CharPos{16,1};
        td<<mEdit->selText()<<"#include <mutex>";
        td<<mEdit->selCount()<<16;
        td<<mStatusChanges<<QList<StatusChanges>{StatusChange::CaretX | StatusChange::CaretY | StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setCaretAndSelection(CharPos{0,2},CharPos{0,1}, CharPos{0,2});
        QTestData& td = QTest::addRow("select one whole line")<<mEdit->selBegin()<<CharPos{0,1};
        td<<mEdit->selEnd()<<CharPos{0,2};
        td<<mEdit->selText()<<"#include <mutex>" + mEdit->lineBreak();
        td<<mEdit->selCount()<<16+mEdit->lineBreak().length();
        td<<mStatusChanges<<QList<StatusChanges>{ StatusChange::CaretY | StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setCaretAndSelection(CharPos{11,1},CharPos{4,0}, CharPos{11,1});
        QTestData& td = QTest::addRow("select two lines")<<mEdit->selBegin()<<CharPos{4,0};
        td<<mEdit->selEnd()<<CharPos{11,1};
        td<<mEdit->selText()<<"lude <iostream>" + mEdit->lineBreak() + "#include <m";
        td<<mEdit->selCount()<<15+mEdit->lineBreak().length()+11;
        td<<mStatusChanges<<QList<StatusChanges>{StatusChange::CaretX | StatusChange::CaretY | StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->selectAll();
        QTestData& td = QTest::addRow("select whole doc")<<mEdit->selBegin()<<mEdit->fileBegin();
        td<<mEdit->selEnd()<<mEdit->fileEnd();
        td<<mEdit->selText()<<mEdit->text();
        td<<mEdit->selCount()<<mEdit->text().length();
        td<<mStatusChanges<<QList<StatusChanges>{ StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setCaretXY(mEdit->fileBegin());
        mEdit->setSelBeginEnd(CharPos{0,0}, CharPos{0,5});
        mStatusChanges.clear();
        mEdit->clearSelection();
        QTestData& td = QTest::addRow("clear selection")<<mEdit->selBegin()<<mEdit->caretXY();
        td<<mEdit->selEnd()<<mEdit->caretXY();
        td<<mEdit->selText()<<"";
        td<<mEdit->selCount()<<0;
        td<<mStatusChanges<<QList<StatusChanges>{StatusChange::Selection};
    }
    {
        loadDemoFile();
        mEdit->setSelBeginEnd(CharPos{0,0}, CharPos{0,5});
        mStatusChanges.clear();
        mEdit->setCaretXY(CharPos{0,10});
        QTestData& td = QTest::addRow("set caret would clear selection")<<mEdit->selBegin()<<CharPos{0,10};
        td<<mEdit->selEnd()<<CharPos{0,10};
        td<<mEdit->selText()<<"";
        td<<mEdit->selCount()<<0;
        td<<mStatusChanges<<QList<StatusChanges>{StatusChange::Selection | StatusChange::CaretY};
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
    QFETCH(QList<QSynedit::StatusChanges>, statusChanges);
    QFETCH(QList<QSynedit::StatusChanges>, expect_statusChanges);
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
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{0});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mReparseStarts, QList<int>{0});
    QCOMPARE(mReparseCounts, QList<int>{1});
    QCOMPARE(mStatusChanges, QList<StatusChanges>{StatusChange::ModifyChanged | StatusChange::CaretY});
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
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,77}));
    QCOMPARE(mStatusChanges, QList<StatusChanges>{ StatusChange::ModifyChanged | StatusChange::CaretY});
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
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,3}));
    QCOMPARE(mStatusChanges, QList<StatusChanges>{ StatusChange::ModifyChanged | StatusChange::CaretY});
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
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,3}));
    QCOMPARE(mStatusChanges, QList<StatusChanges>{ StatusChange::ModifyChanged | StatusChange::CaretY});
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
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(8,0));
    QCOMPARE(mEdit->selBegin(),CharPos(8,0));
    QCOMPARE(mEdit->selEnd(),CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc"});
    QCOMPARE(mEdit->caretXY(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList{"abc  "});
    QCOMPARE(mEdit->caretXY(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(8,0));
    QCOMPARE(mEdit->selBegin(),CharPos(8,0));
    QCOMPARE(mEdit->selEnd(),CharPos(8,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(5,0));
    QCOMPARE(mEdit->selBegin(),CharPos(5,0));
    QCOMPARE(mEdit->selEnd(),CharPos(5,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(!mEdit->empty());
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList{"abc"});
    QCOMPARE(mEdit->caretXY(),CharPos(3,0));
    QCOMPARE(mEdit->selBegin(),CharPos(3,0));
    QCOMPARE(mEdit->selEnd(),CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0,0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QVERIFY(mEdit->empty());
    QVERIFY(!mEdit->modified());
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
    mEdit->setCaretXY(mEdit->fileEnd());
    QTest::keyPress(mEdit.get(), Qt::Key_C);
    QTest::keyPress(mEdit.get(), Qt::Key_D);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(3,3));
    QCOMPARE(mEdit->selBegin(),CharPos(3,3));
    QCOMPARE(mEdit->selEnd(),CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(2,0));
    QCOMPARE(mEdit->selBegin(),CharPos(2,0));
    QCOMPARE(mEdit->selEnd(),CharPos(2,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX  | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(mEdit->modified());

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(3,3));
    QCOMPARE(mEdit->selBegin(),CharPos(3,3));
    QCOMPARE(mEdit->selEnd(),CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
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

    mEdit->setCaretXY(CharPos{12,0});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),'a');
    QCOMPARE(mEdit->lineText(0),"int main() {a");
    QCOMPARE(mEdit->caretXY(),CharPos(13,0));
    QCOMPARE(mEdit->selBegin(),CharPos(13,0));
    QCOMPARE(mEdit->selEnd(),CharPos(13,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(15,0));
    QCOMPARE(mEdit->selBegin(),CharPos(15,0));
    QCOMPARE(mEdit->selEnd(),CharPos(15,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(16,0));
    QCOMPARE(mEdit->selBegin(),CharPos(16,0));
    QCOMPARE(mEdit->selEnd(),CharPos(16,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(14,0));
    QCOMPARE(mEdit->selBegin(),CharPos(14,0));
    QCOMPARE(mEdit->selEnd(),CharPos(14,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({9,9}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(),CharPos(12,0));
    QCOMPARE(mEdit->selBegin(),CharPos(12,0));
    QCOMPARE(mEdit->selEnd(),CharPos(12,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canUndo());

    //Redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->lineText(0),"int main() {ab");
    QCOMPARE(mEdit->caretXY(),CharPos(14,0));
    QCOMPARE(mEdit->selBegin(),CharPos(14,0));
    QCOMPARE(mEdit->selEnd(),CharPos(14,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->lineText(0),"int main() {ab{}");
    QCOMPARE(mEdit->caretXY(),CharPos(16,0));
    QCOMPARE(mEdit->selBegin(),CharPos(16,0));
    QCOMPARE(mEdit->selEnd(),CharPos(16,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
     QCOMPARE(mEdit->caretXY(),CharPos(14,0));
     QCOMPARE(mEdit->selBegin(),CharPos(14,0));
     QCOMPARE(mEdit->selEnd(),CharPos(14,0));
     QCOMPARE(mInsertStartLines, QList<int>{});
     QCOMPARE(mInsertLineCounts, QList<int>{});
     QCOMPARE(mDeleteStartLines, QList<int>{});
     QCOMPARE(mLineMovedFroms, QList<int>{});
     QCOMPARE(mStatusChanges,
              QList<StatusChanges>({
                                       (StatusChange::CaretX | StatusChange::Modified),
              }));
     QCOMPARE(mReparseStarts, QList<int>({0,0}));
     QCOMPARE(mReparseCounts, QList<int>({9,9}));

     clearSignalDatas();
     mEdit->undo();
     QCOMPARE(mEdit->content(),text1);
     QCOMPARE(mEdit->caretXY(),CharPos(12,0));
     QCOMPARE(mEdit->selBegin(),CharPos(12,0));
     QCOMPARE(mEdit->selEnd(),CharPos(12,0));
     QCOMPARE(mInsertStartLines, QList<int>{});
     QCOMPARE(mInsertLineCounts, QList<int>{});
     QCOMPARE(mDeleteStartLines, QList<int>{});
     QCOMPARE(mLineMovedFroms, QList<int>{});
     QCOMPARE(mStatusChanges,
              QList<StatusChanges>({
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

    mEdit->setCaretXY(CharPos{4,3});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete); //'+'
    QCOMPARE(mEdit->lineText(3),"\t\tx+;");
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete); //';'
    QCOMPARE(mEdit->lineText(3),"\t\tx+");
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete); //linebreak
    QCOMPARE(mEdit->lineText(3),"\t\tx+\t}");
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{3});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));


    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+");
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{3});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+");
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+\t}");
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{3});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->lineText(3),"\t\tx+");
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{3});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(4,3));
    QCOMPARE(mEdit->selBegin(),CharPos(4,3));
    QCOMPARE(mEdit->selEnd(),CharPos(4,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    mEdit->setCaretXY(mEdit->fileEnd());
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
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

    mEdit->setCaretXY(CharPos{7,6});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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

    mEdit->setCaretXY(CharPos{16,6});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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

    mEdit->setCaretXY(CharPos{2,10});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{11});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{11});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{11});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{11});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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

    mEdit->setCaretXY(CharPos{2,8});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{9});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{9});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{9});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{9});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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

    mEdit->setCaretXY(mEdit->fileEnd());
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace); // delete last '}'
    QCOMPARE(mEdit->codeBlockCount(),0);
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(9,2));
    QCOMPARE(mEdit->selBegin(),CharPos(9,2));
    QCOMPARE(mEdit->selEnd(),CharPos(9,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{3});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(10,2));
    QCOMPARE(mEdit->selBegin(),CharPos(10,2));
    QCOMPARE(mEdit->selEnd(),CharPos(10,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(mEdit->caretXY(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{2});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),0);

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(),CharPos(10,2));
    QCOMPARE(mEdit->selBegin(),CharPos(10,2));
    QCOMPARE(mEdit->selEnd(),CharPos(10,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{3});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::CaretY),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(9,2));
    QCOMPARE(mEdit->selBegin(),CharPos(9,2));
    QCOMPARE(mEdit->selEnd(),CharPos(9,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified)
             }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canRedo());

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(),CharPos(10,2));
    QCOMPARE(mEdit->selBegin(),CharPos(10,2));
    QCOMPARE(mEdit->selEnd(),CharPos(10,2));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(mEdit->caretXY(),CharPos(0,3));
    QCOMPARE(mEdit->selBegin(),CharPos(0,3));
    QCOMPARE(mEdit->selEnd(),CharPos(0,3));
    QCOMPARE(mInsertStartLines, QList<int>{2});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(),CharPos(1,3));
    QCOMPARE(mEdit->selBegin(),CharPos(1,3));
    QCOMPARE(mEdit->selEnd(),CharPos(1,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
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

    mEdit->setCaretXY(CharPos{0,7});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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

    mEdit->setCaretXY(CharPos{0,7});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,7));
    QCOMPARE(mEdit->selBegin(),CharPos(0,7));
    QCOMPARE(mEdit->selEnd(),CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,7));
    QCOMPARE(mEdit->selBegin(),CharPos(0,7));
    QCOMPARE(mEdit->selEnd(),CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(7,6));
    QCOMPARE(mEdit->selBegin(),CharPos(7,6));
    QCOMPARE(mEdit->selEnd(),CharPos(7,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(16,6));
    QCOMPARE(mEdit->selBegin(),CharPos(16,6));
    QCOMPARE(mEdit->selEnd(),CharPos(16,6));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{6});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,7));
    QCOMPARE(mEdit->selBegin(),CharPos(0,7));
    QCOMPARE(mEdit->selEnd(),CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,7));
    QCOMPARE(mEdit->selBegin(),CharPos(0,7));
    QCOMPARE(mEdit->selEnd(),CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>{6});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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

    mEdit->setCaretXY(CharPos{0,11});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{11});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,11));
    QCOMPARE(mEdit->selBegin(),CharPos(0,11));
    QCOMPARE(mEdit->selEnd(),CharPos(0,11));
    QCOMPARE(mInsertStartLines, QList<int>{11});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(2,10));
    QCOMPARE(mEdit->selBegin(),CharPos(2,10));
    QCOMPARE(mEdit->selEnd(),CharPos(2,10));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{11});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,11));
    QCOMPARE(mEdit->selBegin(),CharPos(0,11));
    QCOMPARE(mEdit->selEnd(),CharPos(0,11));
    QCOMPARE(mInsertStartLines, QList<int>{11});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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

    mEdit->setCaretXY(CharPos{0,9});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{9});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,9));
    QCOMPARE(mEdit->selBegin(),CharPos(0,9));
    QCOMPARE(mEdit->selEnd(),CharPos(0,9));
    QCOMPARE(mInsertStartLines, QList<int>{9});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(2,8));
    QCOMPARE(mEdit->selBegin(),CharPos(2,8));
    QCOMPARE(mEdit->selEnd(),CharPos(2,8));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{9});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,9));
    QCOMPARE(mEdit->selBegin(),CharPos(0,9));
    QCOMPARE(mEdit->selEnd(),CharPos(0,9));
    QCOMPARE(mInsertStartLines, QList<int>{9});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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

void QSynedit::TestQSyneditCpp::test_break_line_in_empty_file()
{
    clearContent();
    QTest::keyPress(mEdit.get(), Qt::Key_Enter);
    QCOMPARE(mEdit->content(),QStringList({"",""}));
    QCOMPARE(mEdit->caretXY(),CharPos(0,1));
    QCOMPARE(mEdit->selBegin(),CharPos(0,1));
    QCOMPARE(mEdit->selEnd(),CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>{0});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList({""}));
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{0});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,1));
    QCOMPARE(mEdit->selBegin(),CharPos(0,1));
    QCOMPARE(mEdit->selEnd(),CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>{0});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{0});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    mEdit->setCaretXY(CharPos{3,0});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretXY(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({0,1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1,1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1,1}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    mEdit->setCaretXY(CharPos{13,3});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretXY(), CharPos(1,4));
    QCOMPARE(mEdit->selBegin(), CharPos(1,4));
    QCOMPARE(mEdit->selEnd(), CharPos(1,4));
    QCOMPARE(mInsertStartLines, QList<int>({4,4}));
    QCOMPARE(mInsertLineCounts, QList<int>({1,1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,4,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(3,5));

    mEdit->setCaretXY(CharPos{3,6});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text4);
    QCOMPARE(mEdit->caretXY(), CharPos(0,7));
    QCOMPARE(mEdit->selBegin(), CharPos(0,7));
    QCOMPARE(mEdit->selEnd(), CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretXY(), CharPos(3,6));
    QCOMPARE(mEdit->selBegin(), CharPos(3,6));
    QCOMPARE(mEdit->selEnd(), CharPos(3,6));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretXY(), CharPos(13,3));
    QCOMPARE(mEdit->selBegin(), CharPos(13,3));
    QCOMPARE(mEdit->selEnd(), CharPos(13,3));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({4,4}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1,1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),0);

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretXY(), CharPos(3,0));
    QCOMPARE(mEdit->selBegin(), CharPos(3,0));
    QCOMPARE(mEdit->selEnd(), CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1,0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1,1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({0,1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1,1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::ModifyChanged | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,1,0,1}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretXY(), CharPos(1,4));
    QCOMPARE(mEdit->selBegin(), CharPos(1,4));
    QCOMPARE(mEdit->selEnd(), CharPos(1,4));
    QCOMPARE(mInsertStartLines, QList<int>({4,4}));
    QCOMPARE(mInsertLineCounts, QList<int>({1,1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,4,3,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1,1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(3,5));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text4);
    QCOMPARE(mEdit->caretXY(), CharPos(0,7));
    QCOMPARE(mEdit->selBegin(), CharPos(0,7));
    QCOMPARE(mEdit->selEnd(), CharPos(0,7));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6,7}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretXY(), CharPos(3,6));
    QCOMPARE(mEdit->selBegin(), CharPos(3,6));
    QCOMPARE(mEdit->selEnd(), CharPos(3,6));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({6}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretXY(), CharPos(13,3));
    QCOMPARE(mEdit->selBegin(), CharPos(13,3));
    QCOMPARE(mEdit->selEnd(), CharPos(13,3));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({4,4}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1,1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));
    QCOMPARE(mEdit->codeBlockCount(),0);

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretXY(), CharPos(3,0));
    QCOMPARE(mEdit->selBegin(), CharPos(3,0));
    QCOMPARE(mEdit->selEnd(), CharPos(3,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1,0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1,1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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


    mEdit->setCaretXY(CharPos{1,5}); //caret between "\t" and "if (x>0) {"
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretXY(), CharPos(1,6));
    QCOMPARE(mEdit->selBegin(), CharPos(1,6));
    QCOMPARE(mEdit->selEnd(), CharPos(1,6));
    QCOMPARE(mInsertStartLines, QList<int>({5}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretY}));
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

    mEdit->setCaretXY(CharPos{9,6}); //caret between "if (x>0)" and " {"
    clearSignalDatas();
    QTest::keyPress(mEdit.get(),Qt::Key_Enter);
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretXY(), CharPos(1,7));
    QCOMPARE(mEdit->selBegin(), CharPos(1,7));
    QCOMPARE(mEdit->selEnd(), CharPos(1,7));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(9,6));
    QCOMPARE(mEdit->selBegin(), CharPos(9,6));
    QCOMPARE(mEdit->selEnd(), CharPos(9,6));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified |StatusChange::CaretY | StatusChange::CaretX}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(1,5));
    QCOMPARE(mEdit->selBegin(), CharPos(1,5));
    QCOMPARE(mEdit->selEnd(), CharPos(1,5));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({5}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::ModifyChanged |StatusChange::CaretY | StatusChange::CaretX}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(1,6));
    QCOMPARE(mEdit->selBegin(), CharPos(1,6));
    QCOMPARE(mEdit->selEnd(), CharPos(1,6));
    QCOMPARE(mInsertStartLines, QList<int>({5}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretY}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(1,7));
    QCOMPARE(mEdit->selBegin(), CharPos(1,7));
    QCOMPARE(mEdit->selEnd(), CharPos(1,7));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified | StatusChange::CaretY}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(9,6));
    QCOMPARE(mEdit->selBegin(), CharPos(9,6));
    QCOMPARE(mEdit->selEnd(), CharPos(9,6));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified |StatusChange::CaretY | StatusChange::CaretX}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(1,5));
    QCOMPARE(mEdit->selBegin(), CharPos(1,5));
    QCOMPARE(mEdit->selEnd(), CharPos(1,5));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({5}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::ModifyChanged |StatusChange::CaretY | StatusChange::CaretX}));
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

    mEdit->setCaretXY(CharPos{2,7});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretXY(), CharPos(1,8));
    QCOMPARE(mEdit->selBegin(), CharPos(1,8));
    QCOMPARE(mEdit->selEnd(), CharPos(1,8));
    QCOMPARE(mInsertStartLines, QList<int>({8}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(2,7));
    QCOMPARE(mEdit->selBegin(), CharPos(2,7));
    QCOMPARE(mEdit->selEnd(), CharPos(2,7));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({8}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(1,8));
    QCOMPARE(mEdit->selBegin(), CharPos(1,8));
    QCOMPARE(mEdit->selEnd(), CharPos(1,8));
    QCOMPARE(mInsertStartLines, QList<int>({8}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(2,7));
    QCOMPARE(mEdit->selBegin(), CharPos(2,7));
    QCOMPARE(mEdit->selEnd(), CharPos(2,7));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({8}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
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

    mEdit->setCaretXY(CharPos{11,5});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretXY(), CharPos(2,6));
    QCOMPARE(mEdit->selBegin(), CharPos(2,6));
    QCOMPARE(mEdit->selEnd(), CharPos(2,6));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(11,5));
    QCOMPARE(mEdit->selBegin(), CharPos(11,5));
    QCOMPARE(mEdit->selEnd(), CharPos(11,5));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::ModifyChanged}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(2,6));
    QCOMPARE(mEdit->selBegin(), CharPos(2,6));
    QCOMPARE(mEdit->selEnd(), CharPos(2,6));
    QCOMPARE(mInsertStartLines, QList<int>({6}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::Modified | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY}));
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
    QCOMPARE(mEdit->caretXY(), CharPos(11,5));
    QCOMPARE(mEdit->selBegin(), CharPos(11,5));
    QCOMPARE(mEdit->selEnd(), CharPos(11,5));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({6}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({ StatusChange::ModifyChanged}));
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

    mEdit->setCaretXY(CharPos{12,0});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text2);

    mEdit->setCaretXY(CharPos{7,1});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text3);

    mEdit->setCaretXY(CharPos{9,2});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text4);

    mEdit->setCaretXY(CharPos{10,3});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text5);

    mEdit->setCaretXY(CharPos{7,4});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text6);

    mEdit->setCaretXY(CharPos{6,5});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text7);

    mEdit->setCaretXY(CharPos{7,6});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text8);

    mEdit->setCaretXY(CharPos{10,7});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text9);

    mEdit->setCaretXY(CharPos{24,8});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text10);

    mEdit->setCaretXY(CharPos{8,9});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text11);

    mEdit->setCaretXY(CharPos{5,10});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text12);

    mEdit->setCaretXY(CharPos{7,11});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), text13);

    mEdit->setCaretXY(CharPos{10,12});
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
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({}));
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

    mEdit->setCaretXY(CharPos{0,1});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::DeleteLine);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified | StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({2}));
    QCOMPARE(mEdit->codeBlockCount(),0);


    mEdit->setCaretXY(CharPos{0,2});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::DeleteLine);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(), CharPos(10,1));
    QCOMPARE(mEdit->selBegin(), CharPos(10,1));
    QCOMPARE(mEdit->selEnd(), CharPos(10,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({2}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified | StatusChange::CaretY | StatusChange::CaretX
                                  }));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(), CharPos(0,2));
    QCOMPARE(mEdit->selBegin(), CharPos(0,2));
    QCOMPARE(mEdit->selEnd(), CharPos(0,2));
    QCOMPARE(mInsertStartLines, QList<int>({2}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified | StatusChange::CaretY | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2,1,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::ModifyChanged | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({2}));
    QCOMPARE(mEdit->codeBlockCount(),0);

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(), CharPos(10,1));
    QCOMPARE(mEdit->selBegin(), CharPos(10,1));
    QCOMPARE(mEdit->selEnd(), CharPos(10,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({2}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(), CharPos(0,2));
    QCOMPARE(mEdit->selBegin(), CharPos(0,2));
    QCOMPARE(mEdit->selEnd(), CharPos(0,2));
    QCOMPARE(mInsertStartLines, QList<int>({2}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified | StatusChange::CaretY | StatusChange::CaretX
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2,1,2}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(), CharPos(0,1));
    QCOMPARE(mEdit->selBegin(), CharPos(0,1));
    QCOMPARE(mEdit->selEnd(), CharPos(0,1));
    QCOMPARE(mInsertStartLines, QList<int>({1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    mEdit->processCommand(EditCommand::DuplicateLine);
    QCOMPARE(mEdit->content(),QStringList({"",""}));
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>({1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified | StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList({""}));
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({}));
    QCOMPARE(mReparseCounts, QList<int>({}));
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),QStringList({"",""}));
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>({1}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified | StatusChange::ModifyChanged
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({1}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

    //undo again
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),QStringList({""}));
    QCOMPARE(mEdit->caretXY(), CharPos(0,0));
    QCOMPARE(mEdit->selBegin(), CharPos(0,0));
    QCOMPARE(mEdit->selEnd(), CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({1}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::ModifyChanged
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
    mEdit->setCaretXY(CharPos{1,1});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::DuplicateLine);
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(), CharPos(1,1));
    QCOMPARE(mEdit->selBegin(), CharPos(1,1));
    QCOMPARE(mEdit->selEnd(), CharPos(1,1));
    QCOMPARE(mInsertStartLines, QList<int>({2}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::ModifyChanged | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));

    mEdit->setCaretXY(CharPos{3,3});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::DuplicateLine);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(), CharPos(3,3));
    QCOMPARE(mEdit->selBegin(), CharPos(3,3));
    QCOMPARE(mEdit->selEnd(), CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>({4}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,5));
    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text2);
    QCOMPARE(mEdit->caretXY(), CharPos(3,3));
    QCOMPARE(mEdit->selBegin(), CharPos(3,3));
    QCOMPARE(mEdit->selEnd(), CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({4}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(), CharPos(1,1));
    QCOMPARE(mEdit->selBegin(), CharPos(1,1));
    QCOMPARE(mEdit->selEnd(), CharPos(1,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({2}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(), CharPos(1,1));
    QCOMPARE(mEdit->selBegin(), CharPos(1,1));
    QCOMPARE(mEdit->selEnd(), CharPos(1,1));
    QCOMPARE(mInsertStartLines, QList<int>({2}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::ModifyChanged | StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({3}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretXY(), CharPos(3,3));
    QCOMPARE(mEdit->selBegin(), CharPos(3,3));
    QCOMPARE(mEdit->selEnd(), CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>({4}));
    QCOMPARE(mInsertLineCounts, QList<int>({1}));
    QCOMPARE(mDeleteStartLines, QList<int>({}));
    QCOMPARE(mDeleteLineCounts, QList<int>({}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
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
    QCOMPARE(mEdit->caretXY(), CharPos(3,3));
    QCOMPARE(mEdit->selBegin(), CharPos(3,3));
    QCOMPARE(mEdit->selEnd(), CharPos(3,3));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({4}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({4}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(2,4));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(),text1);
    QCOMPARE(mEdit->caretXY(), CharPos(1,1));
    QCOMPARE(mEdit->selBegin(), CharPos(1,1));
    QCOMPARE(mEdit->selEnd(), CharPos(1,1));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({2}));
    QCOMPARE(mDeleteLineCounts, QList<int>({1}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                       StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({2}));
    QCOMPARE(mReparseCounts, QList<int>({2}));
    QVERIFY(!mEdit->canUndo());
    QCOMPARE(mEdit->codeBlockCount(),1);
    QVERIFY(mEdit->hasCodeBlock(1,3));
}

void TestQSyneditCpp::test_delete_selection_in_empty_file()
{
    //delete
    clearContent();
    mEdit->selectAll();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QVERIFY(mEdit->empty());

    //backspace
    clearContent();
    mEdit->selectAll();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->canRedo());
    QVERIFY(!mEdit->modified());
    QVERIFY(mEdit->empty());
}

void TestQSyneditCpp::test_delete_all()
{
    loadDemoFile();
    QStringList contents = mEdit->content();

    mEdit->setCaretXY(CharPos{3,5});

    clearSignalDatas();
    mEdit->selectAll();
    QCOMPARE(mEdit->caretXY(), CharPos({3,5}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({1,76}));

    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->caretXY(), CharPos({0,0}));
    QCOMPARE(mEdit->selBegin(), CharPos({0,0}));
    QCOMPARE(mEdit->selEnd(), CharPos({0,0}));
    QCOMPARE(mInsertStartLines, QList<int>({}));
    QCOMPARE(mInsertLineCounts, QList<int>({}));
    QCOMPARE(mDeleteStartLines, QList<int>({0}));
    QCOMPARE(mDeleteLineCounts, QList<int>({76}));
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      StatusChange::Modified | StatusChange::Selection | StatusChange::ModifyChanged | StatusChange::CaretX | StatusChange::CaretY
                                  }));
    QCOMPARE(mReparseStarts, QList<int>({0}));
    QCOMPARE(mReparseCounts, QList<int>({1}));
}


}

