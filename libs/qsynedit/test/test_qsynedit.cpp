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
            | QSynedit::EditorOption::TabsToSpaces
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
    mEdit->processCommand(EditCommand::ClearAll, QChar(0), nullptr);
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
        mEdit->processCommand(EditCommand::Left, QChar(0), nullptr);
        QTestData& td = QTest::newRow("backward in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right, QChar(0), nullptr);
        QTestData& td = QTest::newRow("forward in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }

    loadDemoFile();
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left, QChar(0), nullptr);
        QTestData& td = QTest::newRow("backward at file start")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right, QChar(0), nullptr);
        QTestData& td = QTest::newRow("forward at file end")<<mEdit->caretXY()<<CharPos{1,76};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left, QChar(0), nullptr);
        QTestData& td = QTest::newRow("backward at line start")<<mEdit->caretXY()<<CharPos{7,14};
        td << mStatusChanges << QList<StatusChanges>{(StatusChange::CaretX | StatusChange::CaretY)};
    }
    {
        mEdit->setCaretXY(CharPos{0,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right, QChar(0), nullptr);
        QTestData& td = QTest::newRow("forward at line start")<<mEdit->caretXY()<<CharPos{1,15};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{40,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left, QChar(0), nullptr);
        QTestData& td = QTest::newRow("backward at line end")<<mEdit->caretXY()<<CharPos{39,15};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{40,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right, QChar(0), nullptr);
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
        mEdit->processCommand(EditCommand::Up, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Up in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Down in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Page Up in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageDown, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Page Down in empty doc")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }

    loadDemoFile();
    //up/down
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Up at file start")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{4,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Up at first line")<<mEdit->caretXY()<<CharPos{4,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{4,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Normal Up")<<mEdit->caretXY()<<CharPos{4,14};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{10,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Up but prev line not long enough")<<mEdit->caretXY()<<CharPos{7,14};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretX | StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Down at file end")<<mEdit->caretXY()<<CharPos{1,76};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Down at lastline")<<mEdit->caretXY()<<CharPos{0,76};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{26,61});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Normal Down")<<mEdit->caretXY()<<CharPos{26,62};
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{45,61});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Down but next line not long enough")<<mEdit->caretXY()<<CharPos{37,62};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX | StatusChange::CaretY };
    }
    // page up/down
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Page Up at file start")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{4,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Page Up at first line")<<mEdit->caretXY()<<CharPos{4,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,75});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageUp, QChar(0), nullptr);
        CharPos newPos=CharPos{0,75-mEdit->linesInWindow()};
        if (newPos.line<0)
            newPos.line = 0;
        QTestData& td = QTest::newRow("Normal Page Up")<<mEdit->caretXY()<<newPos;
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{0,1});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageDown, QChar(0), nullptr);
        CharPos newPos=CharPos{0,1+mEdit->linesInWindow()};
        if (newPos.line > 76)
            newPos.line = 76;
        QTestData& td = QTest::newRow("Normal Page Down")<<mEdit->caretXY()<<newPos;
        td << mStatusChanges << QList<StatusChanges>{StatusChange::CaretY};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::PageDown, QChar(0), nullptr);
        QTestData& td = QTest::newRow("Page Down at file end")<<mEdit->caretXY()<<CharPos{1,76};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{0,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down, QChar(0), nullptr);
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

void TestQSyneditCpp::test_move_caret_to_line_start_data()
{
    QTest::addColumn<CharPos>("pos");
    QTest::addColumn<CharPos>("expect");
    QTest::addColumn<QList<StatusChanges>>("statusChanges");
    QTest::addColumn<QList<StatusChanges>>("expect_statusChange");

    loadDemoFile();
    {
        mEdit->setCaretXY(CharPos{0,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart, QChar(0), nullptr);
        QTestData& td = QTest::newRow("doc begin")<<mEdit->caretXY()<<CharPos{0,0};
        td << mStatusChanges << QList<StatusChanges>{};
    }
    {
        mEdit->setCaretXY(CharPos{1,76});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart, QChar(0), nullptr);
        QTestData& td = QTest::newRow("doc end")<<mEdit->caretXY()<<CharPos{0,76};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX };
    }
    {
        mEdit->setCaretXY(CharPos{0,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart, QChar(0), nullptr);
        QTestData& td = QTest::newRow("begin of line")<<mEdit->caretXY()<<CharPos{4,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{2,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart, QChar(0), nullptr);
        QTestData& td = QTest::newRow("in leading spaces")<<mEdit->caretXY()<<CharPos{0,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{4,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart, QChar(0), nullptr);
        QTestData& td = QTest::newRow("end of leading spaces")<<mEdit->caretXY()<<CharPos{0,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{3,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart, QChar(0), nullptr);
        QTestData& td = QTest::newRow("end of leading spaces - 1")<<mEdit->caretXY()<<CharPos{0,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{5,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart, QChar(0), nullptr);
        QTestData& td = QTest::newRow("end of leading spaces + 1")<<mEdit->caretXY()<<CharPos{4,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{10,15});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineStart, QChar(0), nullptr);
        QTestData& td = QTest::newRow("mid of line")<<mEdit->caretXY()<<CharPos{4,15};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
}

void TestQSyneditCpp::test_move_caret_to_line_start()
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
        mEdit->processCommand(EditCommand::LineEnd, QChar(0), nullptr);
        QTestData& td = QTest::newRow("start of trailing spaces")<<mEdit->caretXY()<<CharPos{34,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{26,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd, QChar(0), nullptr);
        QTestData& td = QTest::newRow("start of trailing spaces + 1")<<mEdit->caretXY()<<CharPos{34,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{24,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd, QChar(0), nullptr);
        QTestData& td = QTest::newRow("start of trailing spaces - 1")<<mEdit->caretXY()<<CharPos{25,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{29,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd, QChar(0), nullptr);
        QTestData& td = QTest::newRow("middle of trailing spaces")<<mEdit->caretXY()<<CharPos{34,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{15,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd, QChar(0), nullptr);
        QTestData& td = QTest::newRow("mid of line")<<mEdit->caretXY()<<CharPos{25,70};
        td << mStatusChanges << QList<StatusChanges>{ StatusChange::CaretX};
    }
    {
        mEdit->setCaretXY(CharPos{34,70});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::LineEnd, QChar(0), nullptr);
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
    QCOMPARE(mDeleteLineCounts, QList<int>{2});
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
    QByteArray encoding;
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
    QCOMPARE(mDeleteStartLines, QList<int>{4});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

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
    QCOMPARE(mInsertStartLines, QList<int>{4});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({4,3,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

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
    QCOMPARE(mDeleteStartLines, QList<int>{4});
    QCOMPARE(mDeleteLineCounts, QList<int>{1});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3}));
    QCOMPARE(mReparseCounts, QList<int>({1}));

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
    QCOMPARE(mInsertStartLines, QList<int>{4});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({4,3,4}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

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
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    mEdit->setCaretXY(mEdit->fileEnd());
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
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
    QCOMPARE(mInsertStartLines, QList<int>{3});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

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
    QCOMPARE(mInsertStartLines, QList<int>{3});
    QCOMPARE(mInsertLineCounts, QList<int>{1});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mDeleteLineCounts, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<StatusChanges>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,2,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

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
}



/*
void TestQSyneditCpp::test_delete_text_normal_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<CharPos>("caret_pos");
    QTest::addColumn<CharPos>("expect_caret_pos");

    QTest::addColumn<QList<int>>("insert_starts");
    QTest::addColumn<QList<int>>("expect_insert_starts");
    QTest::addColumn<QList<int>>("insert_counts");
    QTest::addColumn<QList<int>>("expect_insert_counts");
    QTest::addColumn<QList<int>>("delete_starts");
    QTest::addColumn<QList<int>>("expect_delete_starts");
    QTest::addColumn<QList<int>>("delete_counts");
    QTest::addColumn<QList<int>>("expect_delete_counts");
    QTest::addColumn<QList<int>>("move_line_from");
    QTest::addColumn<QList<int>>("expect_move_line_from");
    QTest::addColumn<QList<int>>("move_line_to");
    QTest::addColumn<QList<int>>("expect_move_line_to");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");

    QTest::addColumn<QString>("afterUndo2");
    QTest::addColumn<QString>("expect_afterUndo2");
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->clearSelection();
        QVERIFY(!mEdit->selAvail());
        mEdit->deleteSelection();
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("empty")<<mEdit->lineText(0)<<"#include <iostream>";
        td<<mEdit->caretXY()<<CharPos{0,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        td<<mEdit->lineText(0)<<"#include <iostream>";
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->setSelBegin(CharPos{0,0});
        mEdit->setSelEnd(CharPos{19,0});
        mEdit->deleteSelection();
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("first line")<<mEdit->lineText(0)<<"";
        td<<mEdit->caretXY()<<CharPos{0,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,76});
        mEdit->setSelBegin(CharPos{0,76});
        mEdit->setSelEnd(CharPos{1,76});
        mEdit->deleteSelection();
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("last line")<<mEdit->lineText(76)<<"";
        td<<mEdit->caretXY()<<CharPos{0,76};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(76)<<"";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->setSelBegin(CharPos{0,0});
        mEdit->setSelEnd(CharPos{0,1});
        mEdit->deleteSelection();
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("first 1 line")<<mEdit->lineText(0)<<"#include <mutex>";
        td<<mEdit->caretXY()<<CharPos{0,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{0}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"#include <mutex>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->setSelBegin(CharPos{7,75});
        mEdit->setSelEnd(CharPos{1,76});
        mEdit->deleteSelection();
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("last 1 line")<<mEdit->lineText(mEdit->fileEnd().line)<<"    }  ";
        td<<mEdit->caretXY()<<CharPos{7,75};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{76}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(mEdit->fileEnd().line)<<"}";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(mEdit->fileEnd().line)<<"    }  ";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(mEdit->fileEnd().line)<<"}";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{15,60});
        mEdit->setSelBegin(CharPos{15,60});
        mEdit->setSelEnd(CharPos{11,61});
        mEdit->deleteSelection();
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("multiple lines")<<mEdit->lineText(60)<<"    std::conditeue<std::function<auto()->void>> tasks;";
        td<<mEdit->caretXY()<<CharPos{15,60};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{61}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(60)<<"    std::condition_variable conv;";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(60)<<"    std::conditeue<std::function<auto()->void>> tasks;";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(60)<<"    std::condition_variable conv;";
    }
}

void TestQSyneditCpp::test_delete_text_normal()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(CharPos, caret_pos);
    QFETCH(CharPos, expect_caret_pos);
    QFETCH(QList<int>, insert_starts);
    QFETCH(QList<int>, expect_insert_starts);
    QFETCH(QList<int>, insert_counts);
    QFETCH(QList<int>, expect_insert_counts);
    QFETCH(QList<int>, delete_starts);
    QFETCH(QList<int>, expect_delete_starts);
    QFETCH(QList<int>, delete_counts);
    QFETCH(QList<int>, expect_delete_counts);
    QFETCH(QList<int>, move_line_from);
    QFETCH(QList<int>, expect_move_line_from);
    QFETCH(QList<int>, move_line_to);
    QFETCH(QList<int>, expect_move_line_to);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QFETCH(QString, afterUndo2);
    QFETCH(QString, expect_afterUndo2);

    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(caret_pos, expect_caret_pos);
    QCOMPARE(insert_starts, expect_insert_starts);
    QCOMPARE(insert_counts, expect_insert_counts);
    QCOMPARE(delete_starts, expect_delete_starts);
    QCOMPARE(delete_counts, expect_delete_counts);
    QCOMPARE(move_line_from, expect_move_line_from);
    QCOMPARE(move_line_to, expect_move_line_to);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
    QCOMPARE(afterUndo2, expect_afterUndo2);
}

void TestQSyneditCpp::test_copy_paste_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<CharPos>("caret_pos");
    QTest::addColumn<CharPos>("expect_caret_pos");

    QTest::addColumn<QList<int>>("insert_starts");
    QTest::addColumn<QList<int>>("expect_insert_starts");
    QTest::addColumn<QList<int>>("insert_counts");
    QTest::addColumn<QList<int>>("expect_insert_counts");
    QTest::addColumn<QList<int>>("delete_starts");
    QTest::addColumn<QList<int>>("expect_delete_starts");
    QTest::addColumn<QList<int>>("delete_counts");
    QTest::addColumn<QList<int>>("expect_delete_counts");

    QTest::addColumn<QList<int>>("move_line_from");
    QTest::addColumn<QList<int>>("expect_move_line_from");
    QTest::addColumn<QList<int>>("move_line_to");
    QTest::addColumn<QList<int>>("expect_move_line_to");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");

    QTest::addColumn<QString>("afterUndo2");
    QTest::addColumn<QString>("expect_afterUndo2");
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->setSelBegin(CharPos{0,0});
        mEdit->setSelEnd(CharPos{1,1});
        mEdit->copyToClipboard();
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->pasteFromClipboard();
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file begin")<<mEdit->lineText(1)<<"##include <iostream>";
        td<<mEdit->caretXY()<<CharPos{1,1};
        td<<mInsertStartLines<<QList<int>{0}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(1)<<"##include <iostream>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->setSelBegin(CharPos{0,0});
        mEdit->setSelEnd(CharPos{1,1});
        mEdit->copyToClipboard();
        mEdit->setCaretXY(CharPos{1,76});
        mEdit->pasteFromClipboard();
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file end")<<mEdit->lineText(76)<<"}#include <iostream>";
        td<<mEdit->caretXY()<<CharPos{1,77};
        td<<mInsertStartLines<<QList<int>{77}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(76)<<"}#include <iostream>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
    }
}

void TestQSyneditCpp::test_copy_paste()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(CharPos, caret_pos);
    QFETCH(CharPos, expect_caret_pos);
    QFETCH(QList<int>, insert_starts);
    QFETCH(QList<int>, expect_insert_starts);
    QFETCH(QList<int>, insert_counts);
    QFETCH(QList<int>, expect_insert_counts);
    QFETCH(QList<int>, delete_starts);
    QFETCH(QList<int>, expect_delete_starts);
    QFETCH(QList<int>, delete_counts);
    QFETCH(QList<int>, expect_delete_counts);
    QFETCH(QList<int>, move_line_from);
    QFETCH(QList<int>, expect_move_line_from);
    QFETCH(QList<int>, move_line_to);
    QFETCH(QList<int>, expect_move_line_to);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QFETCH(QString, afterUndo2);
    QFETCH(QString, expect_afterUndo2);

    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(caret_pos, expect_caret_pos);
    QCOMPARE(insert_starts, expect_insert_starts);
    QCOMPARE(insert_counts, expect_insert_counts);
    QCOMPARE(delete_starts, expect_delete_starts);
    QCOMPARE(delete_counts, expect_delete_counts);
    QCOMPARE(move_line_from, expect_move_line_from);
    QCOMPARE(move_line_to, expect_move_line_to);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
    QCOMPARE(afterUndo2, expect_afterUndo2);
}

void TestQSyneditCpp::test_enter_chars_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<CharPos>("caret_pos");
    QTest::addColumn<CharPos>("expect_caret_pos");

    QTest::addColumn<QList<int>>("insert_starts");
    QTest::addColumn<QList<int>>("expect_insert_starts");
    QTest::addColumn<QList<int>>("insert_counts");
    QTest::addColumn<QList<int>>("expect_insert_counts");
    QTest::addColumn<QList<int>>("delete_starts");
    QTest::addColumn<QList<int>>("expect_delete_starts");
    QTest::addColumn<QList<int>>("delete_counts");
    QTest::addColumn<QList<int>>("expect_delete_counts");

    QTest::addColumn<QList<int>>("move_line_from");
    QTest::addColumn<QList<int>>("expect_move_line_from");
    QTest::addColumn<QList<int>>("move_line_to");
    QTest::addColumn<QList<int>>("expect_move_line_to");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");

    QTest::addColumn<QString>("afterUndo2");
    QTest::addColumn<QString>("expect_afterUndo2");
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        QTest::keyPress(mEdit.get(),Qt::Key_A);
        QTest::keyPress(mEdit.get(),Qt::Key_B);
        QTest::keyPress(mEdit.get(),Qt::Key_C);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file begin")<<mEdit->lineText(0)<<"abc#include <iostream>";
        td<<mEdit->caretXY()<<CharPos{3,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        QVERIFY(!mEdit->editing());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        QVERIFY(!mEdit->editing());
        td<<mEdit->lineText(0)<<"abc#include <iostream>";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        QVERIFY(!mEdit->editing());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->clearUndo();
        mEdit->setCaretXY(CharPos{2,1});
        QTest::keyPress(mEdit.get(),Qt::Key_A);
        QTest::keyPress(mEdit.get(),Qt::Key_B);
        QTest::keyPress(mEdit.get(),Qt::Key_C);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("mid of line")<<mEdit->lineText(1)<<"#iabcnclude <mutex>";
        td<<mEdit->caretXY()<<CharPos{5,1};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(1)<<"#iabcnclude <mutex>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
    }
    {
        initEdit();
        mEdit->clearUndo();
        mEdit->setCaretXY(CharPos{1,76});
        QTest::keyPress(mEdit.get(),Qt::Key_A);
        QTest::keyPress(mEdit.get(),Qt::Key_B);
        QTest::keyPress(mEdit.get(),Qt::Key_C);
        QTestData& td = QTest::newRow("end of file")<<mEdit->lineText(76)<<"}abc";
        td<<mEdit->caretXY()<<CharPos{4,76};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(76)<<"}abc";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
    }
    {
        initEdit();
        mEdit->clearUndo();
        mEdit->setCaretXY(CharPos{8,0});
        mEdit->setSelBegin(CharPos{0,0});
        mEdit->setSelEnd(CharPos{8,0});
        QTest::keyPress(mEdit.get(),Qt::Key_A);
        QTest::keyPress(mEdit.get(),Qt::Key_B);
        QTest::keyPress(mEdit.get(),Qt::Key_C);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("selection at file begin")<<mEdit->lineText(0)<<"abc <iostream>";
        td<<mEdit->caretXY()<<CharPos{3,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"abc <iostream>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->clearUndo();
        mEdit->setCaretXY(CharPos{1,1});
        mEdit->setSelBegin(CharPos{8,0});
        mEdit->setSelEnd(CharPos{1,1});
        QTest::keyPress(mEdit.get(),Qt::Key_A);
        QTest::keyPress(mEdit.get(),Qt::Key_B);
        QTest::keyPress(mEdit.get(),Qt::Key_C);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("selection spanning lines")<<mEdit->lineText(0)<<"#includeabcinclude <mutex>";
        td<<mEdit->caretXY()<<CharPos{11,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{1}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"#includeabcinclude <mutex>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->clearUndo();
        mEdit->setCaretXY(CharPos{1,1});
        mEdit->setSelBegin(CharPos{8,0});
        mEdit->setSelEnd(CharPos{1,1});
        QTest::keyPress(mEdit.get(),Qt::Key_A);
        QTest::keyPress(mEdit.get(),Qt::Key_B);
        QTest::keyPress(mEdit.get(),Qt::Key_C);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("selection spanning lines 2")<<mEdit->lineText(1)<<"#include <condition_variable>";
        td<<mEdit->caretXY()<<CharPos{11,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{1}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(1)<<"#include <condition_variable>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
    }
}

void TestQSyneditCpp::test_enter_chars()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(CharPos, caret_pos);
    QFETCH(CharPos, expect_caret_pos);
    QFETCH(QList<int>, insert_starts);
    QFETCH(QList<int>, expect_insert_starts);
    QFETCH(QList<int>, insert_counts);
    QFETCH(QList<int>, expect_insert_counts);
    QFETCH(QList<int>, delete_starts);
    QFETCH(QList<int>, expect_delete_starts);
    QFETCH(QList<int>, delete_counts);
    QFETCH(QList<int>, expect_delete_counts);
    QFETCH(QList<int>, move_line_from);
    QFETCH(QList<int>, expect_move_line_from);
    QFETCH(QList<int>, move_line_to);
    QFETCH(QList<int>, expect_move_line_to);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QFETCH(QString, afterUndo2);
    QFETCH(QString, expect_afterUndo2);

    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(caret_pos, expect_caret_pos);
    QCOMPARE(insert_starts, expect_insert_starts);
    QCOMPARE(insert_counts, expect_insert_counts);
    QCOMPARE(delete_starts, expect_delete_starts);
    QCOMPARE(delete_counts, expect_delete_counts);
    QCOMPARE(move_line_from, expect_move_line_from);
    QCOMPARE(move_line_to, expect_move_line_to);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
    QCOMPARE(afterUndo2, expect_afterUndo2);
}

void TestQSyneditCpp::test_delete_char_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<CharPos>("caret_pos");
    QTest::addColumn<CharPos>("expect_caret_pos");

    QTest::addColumn<QList<int>>("insert_starts");
    QTest::addColumn<QList<int>>("expect_insert_starts");
    QTest::addColumn<QList<int>>("insert_counts");
    QTest::addColumn<QList<int>>("expect_insert_counts");
    QTest::addColumn<QList<int>>("delete_starts");
    QTest::addColumn<QList<int>>("expect_delete_starts");
    QTest::addColumn<QList<int>>("delete_counts");
    QTest::addColumn<QList<int>>("expect_delete_counts");

    QTest::addColumn<QList<int>>("move_line_from");
    QTest::addColumn<QList<int>>("expect_move_line_from");
    QTest::addColumn<QList<int>>("move_line_to");
    QTest::addColumn<QList<int>>("expect_move_line_to");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");

    QTest::addColumn<QString>("afterUndo2");
    QTest::addColumn<QString>("expect_afterUndo2");
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        QTest::keyPress(mEdit.get(),Qt::Key_Delete);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file begin")<<mEdit->lineText(0)<<"include <iostream>";
        td<<mEdit->caretXY()<<CharPos{0,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"include <iostream>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->setCaretXY(mEdit->fileEnd());
        QTest::keyPress(mEdit.get(),Qt::Key_Delete);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file End")<<mEdit->lineText(76)<<"}";
        td<<mEdit->caretXY()<<CharPos{1,76};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
        td<<mEdit->lineText(76)<<"}";
        td<<mEdit->lineText(76)<<"}";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{19,0});
        QTest::keyPress(mEdit.get(),Qt::Key_Delete);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("line end")<<mEdit->lineText(0)<<"#include <iostream>#include <mutex>";
        td<<mEdit->caretXY()<<CharPos{19,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{1}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"#include <iostream>#include <mutex>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{19,0});
        QTest::keyPress(mEdit.get(),Qt::Key_Delete);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("line end 2")<<mEdit->lineText(1)<<"#include <condition_variable>";
        td<<mEdit->caretXY()<<CharPos{19,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{1}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(1)<<"#include <condition_variable>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{4,47});
        QTest::keyPress(mEdit.get(),Qt::Key_Delete);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("end of empty line")<<mEdit->lineText(47)<<"        ~ThreadPool(){";
        td<<mEdit->caretXY()<<CharPos{4,47};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{47}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(47)<<"    ";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(47)<<"        ~ThreadPool(){";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(47)<<"    ";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{4,47});
        QTest::keyPress(mEdit.get(),Qt::Key_Delete);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("end of empty line 2")<<mEdit->lineText(48)<<"        stop.store(true);";
        td<<mEdit->caretXY()<<CharPos{4,47};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{47}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(48)<<"    ~ThreadPool(){";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(48)<<"        stop.store(true);";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(48)<<"    ~ThreadPool(){";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{11,0});
        mEdit->setSelBegin(CharPos{3,0});
        mEdit->setSelEnd(CharPos{11,0});
        QTest::keyPress(mEdit.get(),Qt::Key_Delete);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("selection")<<mEdit->lineText(0)<<"#inostream>";
        td<<mEdit->caretXY()<<CharPos{3,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"#inostream>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
}

void TestQSyneditCpp::test_delete_char()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(CharPos, caret_pos);
    QFETCH(CharPos, expect_caret_pos);
    QFETCH(QList<int>, insert_starts);
    QFETCH(QList<int>, expect_insert_starts);
    QFETCH(QList<int>, insert_counts);
    QFETCH(QList<int>, expect_insert_counts);
    QFETCH(QList<int>, delete_starts);
    QFETCH(QList<int>, expect_delete_starts);
    QFETCH(QList<int>, delete_counts);
    QFETCH(QList<int>, expect_delete_counts);
    QFETCH(QList<int>, move_line_from);
    QFETCH(QList<int>, expect_move_line_from);
    QFETCH(QList<int>, move_line_to);
    QFETCH(QList<int>, expect_move_line_to);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QFETCH(QString, afterUndo2);
    QFETCH(QString, expect_afterUndo2);

    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(caret_pos, expect_caret_pos);
    QCOMPARE(insert_starts, expect_insert_starts);
    QCOMPARE(insert_counts, expect_insert_counts);
    QCOMPARE(delete_starts, expect_delete_starts);
    QCOMPARE(delete_counts, expect_delete_counts);
    QCOMPARE(move_line_from, expect_move_line_from);
    QCOMPARE(move_line_to, expect_move_line_to);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
    QCOMPARE(afterUndo2, expect_afterUndo2);
}

void TestQSyneditCpp::test_backspace_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<CharPos>("caret_pos");
    QTest::addColumn<CharPos>("expect_caret_pos");

    QTest::addColumn<QList<int>>("insert_starts");
    QTest::addColumn<QList<int>>("expect_insert_starts");
    QTest::addColumn<QList<int>>("insert_counts");
    QTest::addColumn<QList<int>>("expect_insert_counts");
    QTest::addColumn<QList<int>>("delete_starts");
    QTest::addColumn<QList<int>>("expect_delete_starts");
    QTest::addColumn<QList<int>>("delete_counts");
    QTest::addColumn<QList<int>>("expect_delete_counts");

    QTest::addColumn<QList<int>>("move_line_from");
    QTest::addColumn<QList<int>>("expect_move_line_from");
    QTest::addColumn<QList<int>>("move_line_to");
    QTest::addColumn<QList<int>>("expect_move_line_to");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");

    QTest::addColumn<QString>("afterUndo2");
    QTest::addColumn<QString>("expect_afterUndo2");
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file begin")<<mEdit->lineText(0)<<"#include <iostream>";
        td<<mEdit->caretXY()<<CharPos{0,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        td<<mEdit->lineText(0)<<"#include <iostream>";
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{1,76});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file end")<<mEdit->lineText(76)<<"";
        td<<mEdit->caretXY()<<CharPos{0,76};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(76)<<"";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(76)<<"}";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,8});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("begin of line (no empty line)-1")<<mEdit->lineText(7)<<"#include <functional>#include <vector>";
        td<<mEdit->caretXY()<<CharPos{21,7};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{8}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(7)<<"#include <functional>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(7)<<"#include <functional>#include <vector>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(7)<<"#include <functional>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,8});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("begin of line (no empty line)-2")<<mEdit->lineText(8)<<"";
        td<<mEdit->caretXY()<<CharPos{21,7};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{8}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(8)<<"#include <vector>";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(8)<<"";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(8)<<"#include <vector>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,11});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("begin of line (current line empty)-1")<<mEdit->lineText(10)<<"#define TEST 123";
        td<<mEdit->caretXY()<<CharPos{16,10};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{11}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(10)<<"#define TEST 123";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(10)<<"#define TEST 123";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(10)<<"#define TEST 123";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,11});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("begin of line (current line empty)-2)")<<mEdit->lineText(11)<<"class ThreadPool";
        td<<mEdit->caretXY()<<CharPos{16,10};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{11}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(11)<<"";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(11)<<"class ThreadPool";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(11)<<"";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,10});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("begin of line (previous line empty)-1")<<mEdit->lineText(9)<<"#define TEST 123";
        td<<mEdit->caretXY()<<CharPos{0,9};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{9}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(9)<<"";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(9)<<"#define TEST 123";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(9)<<"";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,10});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("begin of line (previous line empty)-2")<<mEdit->lineText(10)<<"";
        td<<mEdit->caretXY()<<CharPos{0,9};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{9}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(10)<<"#define TEST 123";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(10)<<"";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(10)<<"#define TEST 123";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{10,12});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("mid of line")<<mEdit->lineText(12)<<"class ThradPool";
        td<<mEdit->caretXY()<<CharPos{9,12};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(12)<<"class ThreadPool";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(12)<<"class ThradPool";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(12)<<"class ThreadPool";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{16,12});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("end of line")<<mEdit->lineText(12)<<"class ThreadPoo";
        td<<mEdit->caretXY()<<CharPos{15,12};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(12)<<"class ThreadPool";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(12)<<"class ThreadPoo";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(12)<<"class ThreadPool";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{12,12});
        mEdit->setSelBegin(CharPos{8,12});
        mEdit->setSelEnd(CharPos{12,12});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("selection")<<mEdit->lineText(12)<<"class ThPool";
        td<<mEdit->caretXY()<<CharPos{8,12};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(12)<<"class ThreadPool";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(12)<<"class ThPool";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(12)<<"class ThreadPool";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{4,0});
        mEdit->setSelBegin(CharPos{4,0});
        mEdit->setSelEnd(CharPos{6,2});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("multi line selection 1")<<mEdit->lineText(0)<<"#incde <condition_variable>";
        td<<mEdit->caretXY()<<CharPos{4,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{1}<<mDeleteLineCounts<<QList<int>{2};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"#incde <condition_variable>";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{4,0});
        mEdit->setSelBegin(CharPos{4,0});
        mEdit->setSelEnd(CharPos{6,2});
        QTest::keyPress(mEdit.get(),Qt::Key_Backspace);
        QTestData& td = QTest::newRow("multi line selection 2")<<mEdit->lineText(2)<<"#include <queue>";
        td<<mEdit->caretXY()<<CharPos{4,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{1}<<mDeleteLineCounts<<QList<int>{2};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(2)<<"#include <condition_variable>";
        mEdit->redo();
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(2)<<"#include <queue>";
        mEdit->undo();
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(2)<<"#include <condition_variable>";
    }
}

void TestQSyneditCpp::test_backspace()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(CharPos, caret_pos);
    QFETCH(CharPos, expect_caret_pos);
    QFETCH(QList<int>, insert_starts);
    QFETCH(QList<int>, expect_insert_starts);
    QFETCH(QList<int>, insert_counts);
    QFETCH(QList<int>, expect_insert_counts);
    QFETCH(QList<int>, delete_starts);
    QFETCH(QList<int>, expect_delete_starts);
    QFETCH(QList<int>, delete_counts);
    QFETCH(QList<int>, expect_delete_counts);
    QFETCH(QList<int>, move_line_from);
    QFETCH(QList<int>, expect_move_line_from);
    QFETCH(QList<int>, move_line_to);
    QFETCH(QList<int>, expect_move_line_to);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QFETCH(QString, afterUndo2);
    QFETCH(QString, expect_afterUndo2);

    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(caret_pos, expect_caret_pos);
    QCOMPARE(insert_starts, expect_insert_starts);
    QCOMPARE(insert_counts, expect_insert_counts);
    QCOMPARE(delete_starts, expect_delete_starts);
    QCOMPARE(delete_counts, expect_delete_counts);
    QCOMPARE(move_line_from, expect_move_line_from);
    QCOMPARE(move_line_to, expect_move_line_to);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
    QCOMPARE(afterUndo2, expect_afterUndo2);
}

void TestQSyneditCpp::test_break_line_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<CharPos>("caret_pos");
    QTest::addColumn<CharPos>("expect_caret_pos");

    QTest::addColumn<QList<int>>("insert_starts");
    QTest::addColumn<QList<int>>("expect_insert_starts");
    QTest::addColumn<QList<int>>("insert_counts");
    QTest::addColumn<QList<int>>("expect_insert_counts");
    QTest::addColumn<QList<int>>("delete_starts");
    QTest::addColumn<QList<int>>("expect_delete_starts");
    QTest::addColumn<QList<int>>("delete_counts");
    QTest::addColumn<QList<int>>("expect_delete_counts");

    QTest::addColumn<QList<int>>("move_line_from");
    QTest::addColumn<QList<int>>("expect_move_line_from");
    QTest::addColumn<QList<int>>("move_line_to");
    QTest::addColumn<QList<int>>("expect_move_line_to");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");

    QTest::addColumn<QString>("afterUndo2");
    QTest::addColumn<QString>("expect_afterUndo2");
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file start")<<mEdit->lineText(1)<<"#include <iostream>";
        td<<mEdit->caretXY()<<CharPos{0,1};
        td<<mInsertStartLines<<QList<int>{0}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(1)<<"#include <iostream>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{1,76});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file end")<<mEdit->lineText(77)<<"";
        td<<mEdit->caretXY()<<CharPos{0,77};
        td<<mInsertStartLines<<QList<int>{77}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        td<<mEdit->lineText(76)<<"}";
        td<<mEdit->lineText(76)<<"}";
        td<<mEdit->lineText(76)<<"}";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        QVERIFY(mEdit->lineCount()==77);
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        QVERIFY(mEdit->lineCount()==78);
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        QVERIFY(mEdit->lineCount()==77);
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{24,27});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("in trailing spaces (1)")<<mEdit->lineText(27)<<"                        ";
        td<<mEdit->caretXY()<<CharPos{0,28};
        td<<mInsertStartLines<<QList<int>{27}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(27)<<"                        return;";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(27)<<"                        ";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(27)<<"                        return;";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{24,27});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("in trailing spaces (2)")<<mEdit->lineText(28)<<"return;";
        td<<mEdit->caretXY()<<CharPos{0,28};
        td<<mInsertStartLines<<QList<int>{27}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(28)<<"                    ";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(28)<<"return;";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(28)<<"                    ";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{17,15});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("mid of line (1)")<<mEdit->lineText(15)<<"    ThreadPool(ui";
        td<<mEdit->caretXY()<<CharPos{0,16};
        td<<mInsertStartLines<<QList<int>{16}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(15)<<"    ThreadPool(uint32_t num):stop{false}";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(15)<<"    ThreadPool(ui";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(15)<<"    ThreadPool(uint32_t num):stop{false}";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{17,15});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("mid of line (2)")<<mEdit->lineText(16)<<"nt32_t num):stop{false}";
        td<<mEdit->caretXY()<<CharPos{0,16};
        td<<mInsertStartLines<<QList<int>{16}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(16)<<"    {";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(16)<<"nt32_t num):stop{false}";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(16)<<"    {";
    }
}

void TestQSyneditCpp::test_break_line()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(CharPos, caret_pos);
    QFETCH(CharPos, expect_caret_pos);
    QFETCH(QList<int>, insert_starts);
    QFETCH(QList<int>, expect_insert_starts);
    QFETCH(QList<int>, insert_counts);
    QFETCH(QList<int>, expect_insert_counts);
    QFETCH(QList<int>, delete_starts);
    QFETCH(QList<int>, expect_delete_starts);
    QFETCH(QList<int>, delete_counts);
    QFETCH(QList<int>, expect_delete_counts);
    QFETCH(QList<int>, move_line_from);
    QFETCH(QList<int>, expect_move_line_from);
    QFETCH(QList<int>, move_line_to);
    QFETCH(QList<int>, expect_move_line_to);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QFETCH(QString, afterUndo2);
    QFETCH(QString, expect_afterUndo2);

    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(caret_pos, expect_caret_pos);
    QCOMPARE(insert_starts, expect_insert_starts);
    QCOMPARE(insert_counts, expect_insert_counts);
    QCOMPARE(delete_starts, expect_delete_starts);
    QCOMPARE(delete_counts, expect_delete_counts);
    QCOMPARE(move_line_from, expect_move_line_from);
    QCOMPARE(move_line_to, expect_move_line_to);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
    QCOMPARE(afterUndo2, expect_afterUndo2);
}

void TestQSyneditCpp::test_break_line_at_end_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<CharPos>("caret_pos");
    QTest::addColumn<CharPos>("expect_caret_pos");

    QTest::addColumn<QList<int>>("insert_starts");
    QTest::addColumn<QList<int>>("expect_insert_starts");
    QTest::addColumn<QList<int>>("insert_counts");
    QTest::addColumn<QList<int>>("expect_insert_counts");
    QTest::addColumn<QList<int>>("delete_starts");
    QTest::addColumn<QList<int>>("expect_delete_starts");
    QTest::addColumn<QList<int>>("delete_counts");
    QTest::addColumn<QList<int>>("expect_delete_counts");

    QTest::addColumn<QList<int>>("move_line_from");
    QTest::addColumn<QList<int>>("expect_move_line_from");
    QTest::addColumn<QList<int>>("move_line_to");
    QTest::addColumn<QList<int>>("expect_move_line_to");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");

    QTest::addColumn<QString>("afterUndo2");
    QTest::addColumn<QString>("expect_afterUndo2");
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter, Qt::ControlModifier);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file start")<<mEdit->lineText(1)<<"";
        td<<mEdit->caretXY()<<CharPos{0,1};
        td<<mInsertStartLines<<QList<int>{1}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(1)<<"";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,76});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter, Qt::ControlModifier);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file end")<<mEdit->lineText(77)<<"";
        td<<mEdit->caretXY()<<CharPos{0,77};
        td<<mInsertStartLines<<QList<int>{77}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        td<<mEdit->lineText(76)<<"}";
        td<<mEdit->lineText(76)<<"}";
        td<<mEdit->lineText(76)<<"}";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        QVERIFY(mEdit->lineCount()==77);
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        QVERIFY(mEdit->lineCount()==78);
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        QVERIFY(mEdit->lineCount()==77);
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{17,15});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter, Qt::ControlModifier);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("mid of line (1)")<<mEdit->lineText(15)<<"    ThreadPool(uint32_t num):stop{false}";
        td<<mEdit->caretXY()<<CharPos{0,16};
        td<<mInsertStartLines<<QList<int>{16}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(15)<<"    ThreadPool(uint32_t num):stop{false}";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(15)<<"    ThreadPool(uint32_t num):stop{false}";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(15)<<"    ThreadPool(uint32_t num):stop{false}";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{17,15});
        QTest::keyPress(mEdit.get(),Qt::Key_Enter, Qt::ControlModifier);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("mid of line (2)")<<mEdit->lineText(16)<<"";
        td<<mEdit->caretXY()<<CharPos{0,16};
        td<<mInsertStartLines<<QList<int>{16}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(16)<<"    {";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(16)<<"";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(16)<<"    {";
    }
}

void TestQSyneditCpp::test_break_line_at_end()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(CharPos, caret_pos);
    QFETCH(CharPos, expect_caret_pos);
    QFETCH(QList<int>, insert_starts);
    QFETCH(QList<int>, expect_insert_starts);
    QFETCH(QList<int>, insert_counts);
    QFETCH(QList<int>, expect_insert_counts);
    QFETCH(QList<int>, delete_starts);
    QFETCH(QList<int>, expect_delete_starts);
    QFETCH(QList<int>, delete_counts);
    QFETCH(QList<int>, expect_delete_counts);
    QFETCH(QList<int>, move_line_from);
    QFETCH(QList<int>, expect_move_line_from);
    QFETCH(QList<int>, move_line_to);
    QFETCH(QList<int>, expect_move_line_to);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QFETCH(QString, afterUndo2);
    QFETCH(QString, expect_afterUndo2);

    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(caret_pos, expect_caret_pos);
    QCOMPARE(insert_starts, expect_insert_starts);
    QCOMPARE(insert_counts, expect_insert_counts);
    QCOMPARE(delete_starts, expect_delete_starts);
    QCOMPARE(delete_counts, expect_delete_counts);
    QCOMPARE(move_line_from, expect_move_line_from);
    QCOMPARE(move_line_to, expect_move_line_to);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
    QCOMPARE(afterUndo2, expect_afterUndo2);
}

void TestQSyneditCpp::test_delete_current_line_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<CharPos>("caret_pos");
    QTest::addColumn<CharPos>("expect_caret_pos");

    QTest::addColumn<QList<int>>("insert_starts");
    QTest::addColumn<QList<int>>("expect_insert_starts");
    QTest::addColumn<QList<int>>("insert_counts");
    QTest::addColumn<QList<int>>("expect_insert_counts");
    QTest::addColumn<QList<int>>("delete_starts");
    QTest::addColumn<QList<int>>("expect_delete_starts");
    QTest::addColumn<QList<int>>("delete_counts");
    QTest::addColumn<QList<int>>("expect_delete_counts");

    QTest::addColumn<QList<int>>("move_line_from");
    QTest::addColumn<QList<int>>("expect_move_line_from");
    QTest::addColumn<QList<int>>("move_line_to");
    QTest::addColumn<QList<int>>("expect_move_line_to");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");

    QTest::addColumn<QString>("afterUndo2");
    QTest::addColumn<QString>("expect_afterUndo2");
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->processCommand(EditCommand::DeleteLine, QChar{0}, nullptr);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file start")<<mEdit->lineText(0)<<"#include <mutex>";
        td<<mEdit->caretXY()<<CharPos{0,0};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{0}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(0)<<"#include <mutex>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(0)<<"#include <iostream>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{1,76});
        mEdit->processCommand(EditCommand::DeleteLine, QChar{0}, nullptr);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file end")<<mEdit->lineText(75)<<"    }  ";
        td<<mEdit->caretXY()<<CharPos{7,75};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{76}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        td<<mEdit->lineText(75)<<"    }  ";
        td<<mEdit->lineText(75)<<"    }  ";
        td<<mEdit->lineText(75)<<"    }  ";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        QCOMPARE(mEdit->lineCount(),77);
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        QCOMPARE(mEdit->lineCount(),76);
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        QCOMPARE(mEdit->lineCount(),77);
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{17,60});
        mEdit->processCommand(EditCommand::DeleteLine, QChar{0}, nullptr);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("mid of line")<<mEdit->lineText(60)<<"    std::queue<std::function<auto()->void>> tasks;";
        td<<mEdit->caretXY()<<CharPos{0,60};
        td<<mInsertStartLines<<QList<int>{}<<mInsertLineCounts<<QList<int>{};
        td<<mDeleteStartLines<<QList<int>{60}<<mDeleteLineCounts<<QList<int>{1};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(60)<<"    std::condition_variable conv;";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(60)<<"    std::queue<std::function<auto()->void>> tasks;";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(60)<<"    std::condition_variable conv;";
    }
}

void TestQSyneditCpp::test_delete_current_line()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(CharPos, caret_pos);
    QFETCH(CharPos, expect_caret_pos);
    QFETCH(QList<int>, insert_starts);
    QFETCH(QList<int>, expect_insert_starts);
    QFETCH(QList<int>, insert_counts);
    QFETCH(QList<int>, expect_insert_counts);
    QFETCH(QList<int>, delete_starts);
    QFETCH(QList<int>, expect_delete_starts);
    QFETCH(QList<int>, delete_counts);
    QFETCH(QList<int>, expect_delete_counts);
    QFETCH(QList<int>, move_line_from);
    QFETCH(QList<int>, expect_move_line_from);
    QFETCH(QList<int>, move_line_to);
    QFETCH(QList<int>, expect_move_line_to);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QFETCH(QString, afterUndo2);
    QFETCH(QString, expect_afterUndo2);

    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(caret_pos, expect_caret_pos);
    QCOMPARE(insert_starts, expect_insert_starts);
    QCOMPARE(insert_counts, expect_insert_counts);
    QCOMPARE(delete_starts, expect_delete_starts);
    QCOMPARE(delete_counts, expect_delete_counts);
    QCOMPARE(move_line_from, expect_move_line_from);
    QCOMPARE(move_line_to, expect_move_line_to);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
    QCOMPARE(afterUndo2, expect_afterUndo2);
}

void TestQSyneditCpp::test_duplicate_current_line_data()
{
    QTest::addColumn<QString>("afterEdit");
    QTest::addColumn<QString>("expect_afterEdit");

    QTest::addColumn<CharPos>("caret_pos");
    QTest::addColumn<CharPos>("expect_caret_pos");

    QTest::addColumn<QList<int>>("insert_starts");
    QTest::addColumn<QList<int>>("expect_insert_starts");
    QTest::addColumn<QList<int>>("insert_counts");
    QTest::addColumn<QList<int>>("expect_insert_counts");
    QTest::addColumn<QList<int>>("delete_starts");
    QTest::addColumn<QList<int>>("expect_delete_starts");
    QTest::addColumn<QList<int>>("delete_counts");
    QTest::addColumn<QList<int>>("expect_delete_counts");

    QTest::addColumn<QList<int>>("move_line_from");
    QTest::addColumn<QList<int>>("expect_move_line_from");
    QTest::addColumn<QList<int>>("move_line_to");
    QTest::addColumn<QList<int>>("expect_move_line_to");

    QTest::addColumn<QString>("afterUndo");
    QTest::addColumn<QString>("expect_afterUndo");

    QTest::addColumn<QString>("afterRedo");
    QTest::addColumn<QString>("expect_afterRedo");

    QTest::addColumn<QString>("afterUndo2");
    QTest::addColumn<QString>("expect_afterUndo2");
    {
        initEdit();
        mEdit->setCaretXY(CharPos{0,0});
        mEdit->processCommand(EditCommand::DuplicateLine, QChar{0}, nullptr);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file start")<<mEdit->lineText(1)<<"#include <iostream>";
        td<<mEdit->caretXY()<<CharPos{0,0};
        td<<mInsertStartLines<<QList<int>{1}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(1)<<"#include <iostream>";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(1)<<"#include <mutex>";
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{1,76});
        mEdit->processCommand(EditCommand::DuplicateLine, QChar{0}, nullptr);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("file start")<<mEdit->lineText(76)<<"}";
        td<<mEdit->caretXY()<<CharPos{0,76};
        td<<mInsertStartLines<<QList<int>{77}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        td<<mEdit->lineText(76)<<"}";
        td<<mEdit->lineText(76)<<"}";
        td<<mEdit->lineText(76)<<"}";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        QCOMPARE(mEdit->lineCount(),77);
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        QCOMPARE(mEdit->lineCount(),78);
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        QCOMPARE(mEdit->lineCount(),77);
    }
    {
        initEdit();
        mEdit->setCaretXY(CharPos{10,61});
        mEdit->processCommand(EditCommand::DuplicateLine, QChar{0}, nullptr);
        QVERIFY(!mEdit->editing());
        QTestData& td = QTest::newRow("mid of line")<<mEdit->lineText(62)<<"    std::queue<std::function<auto()->void>> tasks;";
        td<<mEdit->caretXY()<<CharPos{0,61};
        td<<mInsertStartLines<<QList<int>{62}<<mInsertLineCounts<<QList<int>{1};
        td<<mDeleteStartLines<<QList<int>{}<<mDeleteLineCounts<<QList<int>{};
        td<<mLineMovedFroms<<QList<int>{}<<mLineMovedTos<<QList<int>{};
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(62)<<"    std::vector<std::thread> threads;";
        mEdit->redo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canRedo());
        td<<mEdit->lineText(62)<<"    std::queue<std::function<auto()->void>> tasks;";
        mEdit->undo();
        QVERIFY(!mEdit->editing());
        QVERIFY(!mEdit->canUndo());
        td<<mEdit->lineText(62)<<"    std::vector<std::thread> threads;";
    }
}

void TestQSyneditCpp::test_duplicate_current_line()
{
    QFETCH(QString, afterEdit);
    QFETCH(QString, expect_afterEdit);
    QFETCH(CharPos, caret_pos);
    QFETCH(CharPos, expect_caret_pos);
    QFETCH(QList<int>, insert_starts);
    QFETCH(QList<int>, expect_insert_starts);
    QFETCH(QList<int>, insert_counts);
    QFETCH(QList<int>, expect_insert_counts);
    QFETCH(QList<int>, delete_starts);
    QFETCH(QList<int>, expect_delete_starts);
    QFETCH(QList<int>, delete_counts);
    QFETCH(QList<int>, expect_delete_counts);
    QFETCH(QList<int>, move_line_from);
    QFETCH(QList<int>, expect_move_line_from);
    QFETCH(QList<int>, move_line_to);
    QFETCH(QList<int>, expect_move_line_to);
    QFETCH(QString, afterUndo);
    QFETCH(QString, expect_afterUndo);
    QFETCH(QString, afterRedo);
    QFETCH(QString, expect_afterRedo);
    QFETCH(QString, afterUndo2);
    QFETCH(QString, expect_afterUndo2);

    QCOMPARE(afterEdit, expect_afterEdit);
    QCOMPARE(caret_pos, expect_caret_pos);
    QCOMPARE(insert_starts, expect_insert_starts);
    QCOMPARE(insert_counts, expect_insert_counts);
    QCOMPARE(delete_starts, expect_delete_starts);
    QCOMPARE(delete_counts, expect_delete_counts);
    QCOMPARE(move_line_from, expect_move_line_from);
    QCOMPARE(move_line_to, expect_move_line_to);
    QCOMPARE(afterUndo, expect_afterUndo);
    QCOMPARE(afterRedo, expect_afterRedo);
    QCOMPARE(afterUndo2, expect_afterUndo2);
}
*/
}

