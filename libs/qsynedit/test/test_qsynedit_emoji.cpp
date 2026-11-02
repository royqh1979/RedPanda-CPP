#include <QTest>
#include <QCoreApplication>

#include "test_qsynedit_emoji.h"
#include "test_utils.h"
#include "qsynedit/qsynedit.h"
#include "qsynedit/document.h"
#include "qsynedit/syntaxer/textfile.h"
#include "qsynedit/formatter/cppformatter.h"

namespace QSynedit {

void TestQSyneditEmoji::initTestCase()
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
    mEdit->setSyntaxer(std::make_shared<TextSyntaxer>());

    QFont defaultFont;
    defaultFont.setFamily(defaultMonoFont());
    defaultFont.setPointSize(14);
    mEdit->setFont(defaultFont);

    connectEditSignals();
}

void TestQSyneditEmoji::cleanup()
{
    mEdit->setInsertMode(true);
}

void TestQSyneditEmoji::loadDemoFile()
{
    QByteArray encoding;
    mEdit->loadFromFile("resources/emoji.txt",ENCODING_AUTO_DETECT,encoding);

    clearSignalDatas();
}

void TestQSyneditEmoji::test_move_caret_x_data()
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
        mEdit->setCaretPos(CharPos{31,9});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward at file end")<<mEdit->caretPos()<<CharPos{31,9};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,8});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward at line start")<<mEdit->caretPos()<<CharPos{12,7};
        td << mStatusChanges << QList<int>{(StatusChange::CaretX | StatusChange::CaretY)};
    }
    {
        mEdit->setCaretPos(CharPos{0,8});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward at line start")<<mEdit->caretPos()<<CharPos{2,8};
        td << mStatusChanges << QList<int>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{16,8});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward at line end")<<mEdit->caretPos()<<CharPos{14,8};
        td << mStatusChanges << QList<int>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{16,8});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("forward at line end")<<mEdit->caretPos()<<CharPos{0,9};
        td << mStatusChanges << QList<int>{StatusChange::CaretX | StatusChange::CaretY};
    }
    {
        mEdit->setCaretPos(CharPos{31,9});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward on emoji zwj sequence")<<mEdit->caretPos()<<CharPos{26,9};
        td << mStatusChanges << QList<int>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{26,9});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Left);
        QTestData& td = QTest::newRow("backward on emoji sequence")<<mEdit->caretPos()<<CharPos{21,9};
        td << mStatusChanges << QList<int>{StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{11,9});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Right);
        QTestData& td = QTest::newRow("backward on emoji zwj sequence 2")<<mEdit->caretPos()<<CharPos{16,9};
        td << mStatusChanges << QList<int>{StatusChange::CaretX};
    }
}

void TestQSyneditEmoji::test_move_caret_x()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<int>, statusChanges);
    QFETCH(QList<int>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);
}

void TestQSyneditEmoji::test_move_caret_y_data()
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
        mEdit->setCaretPos(CharPos{3,1});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Up);
        QTestData& td = QTest::newRow("Up in the middle of emoji")<<mEdit->caretPos()<<CharPos{2,0};
        td << mStatusChanges << QList<int>{StatusChange::CaretY | StatusChange::CaretX};
    }
    {
        mEdit->setCaretPos(CharPos{4,0});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down in the middle of emoji")<<mEdit->caretPos()<<CharPos{3,1};
        td << mStatusChanges << QList<int>{StatusChange::CaretX | StatusChange::CaretY};
    }
    {
        mEdit->setCaretPos(CharPos{31,9});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down at file end")<<mEdit->caretPos()<<CharPos{31,9};
        td << mStatusChanges << QList<int>{};
    }
    {
        mEdit->setCaretPos(CharPos{0,9});
        mStatusChanges.clear();
        mEdit->processCommand(EditCommand::Down);
        QTestData& td = QTest::newRow("Down at lastline")<<mEdit->caretPos()<<CharPos{0,9};
        td << mStatusChanges << QList<int>{};
    }
}

void TestQSyneditEmoji::test_move_caret_y()
{
    QFETCH(CharPos, pos);
    QFETCH(CharPos, expect);
    QFETCH(QList<int>, statusChanges);
    QFETCH(QList<int>, expect_statusChange);
    QCOMPARE(pos, expect);
    QCOMPARE(statusChanges, expect_statusChange);
}

void TestQSyneditEmoji::test_input_chars_at_file_begin_end_overwrite_mode()
{
    mEdit->setInsertMode(false);
    QStringList text1({
                          "🌴🌳🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌶",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬🧑‍✈️👩‍🎨"
                      });
    QStringList text2({
                          "ab🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌶",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬🧑‍✈️👩‍🎨"
                      });
    QStringList text3({
                          "ab🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌶",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬cde"
                      });
    QStringList text4({
                          "ab🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌸🌹👩‍🎨",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬cde"
                      });
    mEdit->loadContent(text1);
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

    mEdit->setCaretPos(CharPos{21,3});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_C);
    QTest::keyPress(mEdit.get(), Qt::Key_D);
    QTest::keyPress(mEdit.get(), Qt::Key_E);
    QCOMPARE(mEdit->content(),text3);
    QCOMPARE(mEdit->caretPos(),CharPos(24,3));
    QCOMPARE(mEdit->selBegin(),CharPos(24,3));
    QCOMPARE(mEdit->selEnd(),CharPos(24,3));
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
    QCOMPARE(mReparseStarts, QList<int>({3,3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    mEdit->setCaretPos(CharPos{10,2});
    clearSignalDatas();
    mEdit->processCommand(EditCommand::Input, "🌸");
    mEdit->processCommand(EditCommand::Input, "🌹");
    mEdit->processCommand(EditCommand::Input, "👩‍🎨");
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(mEdit->caretPos(),CharPos(19,2));
    QCOMPARE(mEdit->selBegin(),CharPos(19,2));
    QCOMPARE(mEdit->selEnd(),CharPos(19,2));
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
    QCOMPARE(mEdit->caretPos(),CharPos(10,2));
    QCOMPARE(mEdit->selBegin(),CharPos(10,2));
    QCOMPARE(mEdit->selEnd(),CharPos(10,2));
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
    QCOMPARE(mEdit->caretPos(),CharPos(21,3));
    QCOMPARE(mEdit->selBegin(),CharPos(21,3));
    QCOMPARE(mEdit->selEnd(),CharPos(21,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

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
    QCOMPARE(mEdit->caretPos(),CharPos(24,3));
    QCOMPARE(mEdit->selBegin(),CharPos(24,3));
    QCOMPARE(mEdit->selEnd(),CharPos(24,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      ( StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(),text4);
    QCOMPARE(mEdit->caretPos(),CharPos(19,2));
    QCOMPARE(mEdit->selBegin(),CharPos(19,2));
    QCOMPARE(mEdit->selEnd(),CharPos(19,2));
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
    QCOMPARE(mEdit->caretPos(),CharPos(10,2));
    QCOMPARE(mEdit->selBegin(),CharPos(10,2));
    QCOMPARE(mEdit->selEnd(),CharPos(10,2));
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
    QCOMPARE(mEdit->caretPos(),CharPos(21,3));
    QCOMPARE(mEdit->selBegin(),CharPos(21,3));
    QCOMPARE(mEdit->selEnd(),CharPos(21,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1,1}));

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
                                      (StatusChange::CaretY | StatusChange::CaretX | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditEmoji::test_delete_chars_in_file()
{
    QStringList text1({
                          "🌴🌳🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌶",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬🧑‍✈️🌳"
                      });
    QStringList text2({
                          "🌴🌳🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌶",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬"
                      });
    QStringList text3({
                          "🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌶",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬"
                      });

    mEdit->loadContent(text1);

    mEdit->setCaretPos(CharPos{21,3});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(),CharPos(21,3));
    QCOMPARE(mEdit->selBegin(),CharPos(21,3));
    QCOMPARE(mEdit->selEnd(),CharPos(21,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    mEdit->setCaretPos(CharPos{0,0});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QTest::keyPress(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->content(), text3);
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
                                      (StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
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
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(),CharPos(21,3));
    QCOMPARE(mEdit->selBegin(),CharPos(21,3));
    QCOMPARE(mEdit->selEnd(),CharPos(21,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(),CharPos(21,3));
    QCOMPARE(mEdit->selBegin(),CharPos(21,3));
    QCOMPARE(mEdit->selEnd(),CharPos(21,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::Modified | StatusChange::CaretX | StatusChange::CaretY)
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canRedo());

    //undo
    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      StatusChange::Modified ,
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(),CharPos(21,3));
    QCOMPARE(mEdit->selBegin(),CharPos(21,3));
    QCOMPARE(mEdit->selEnd(),CharPos(21,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

void TestQSyneditEmoji::test_delete_prev_chars_in_file()
{
    QStringList text1({
                          "🌴🌳🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌶",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬🧑‍✈️🌴"
                      });
    QStringList text2({
                          "🌴🌳🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌶",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬"
                      });
    QStringList text3({
                          "🌵🌶🌷🌸🌹🌺🌻🌼🌽",
                          "🌴1🌳2🌵3🌶",
                          "0🌴1🌳2🌵3🌶",
                          "🙈🙉🙊🧑‍🔬👨‍🔬👩‍🔬"
                      });

    mEdit->loadContent(text1);

    mEdit->setCaretPos(CharPos{28,3});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(),CharPos(21,3));
    QCOMPARE(mEdit->selBegin(),CharPos(21,3));
    QCOMPARE(mEdit->selEnd(),CharPos(21,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    mEdit->setCaretPos(CharPos{4,0});
    clearSignalDatas();
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QTest::keyPress(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified),
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    //undo
    clearSignalDatas();
    mEdit->undo();
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
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(),CharPos(28,3));
    QCOMPARE(mEdit->selBegin(),CharPos(28,3));
    QCOMPARE(mEdit->selEnd(),CharPos(28,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());

    //redo
    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text2);
    QCOMPARE(mEdit->caretPos(),CharPos(21,3));
    QCOMPARE(mEdit->selBegin(),CharPos(21,3));
    QCOMPARE(mEdit->selEnd(),CharPos(21,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::Modified | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->redo();
    QCOMPARE(mEdit->content(), text3);
    QCOMPARE(mEdit->caretPos(),CharPos(0,0));
    QCOMPARE(mEdit->selBegin(),CharPos(0,0));
    QCOMPARE(mEdit->selEnd(),CharPos(0,0));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX |  StatusChange::CaretY | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canRedo());

    //undo again
    clearSignalDatas();
    mEdit->undo();
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
                                      (StatusChange::CaretX | StatusChange::Modified),
             }));
    QCOMPARE(mReparseStarts, QList<int>({0,0}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    clearSignalDatas();
    mEdit->undo();
    QCOMPARE(mEdit->content(), text1);
    QCOMPARE(mEdit->caretPos(),CharPos(28,3));
    QCOMPARE(mEdit->selBegin(),CharPos(28,3));
    QCOMPARE(mEdit->selEnd(),CharPos(28,3));
    QCOMPARE(mInsertStartLines, QList<int>{});
    QCOMPARE(mInsertLineCounts, QList<int>{});
    QCOMPARE(mDeleteStartLines, QList<int>{});
    QCOMPARE(mLineMovedFroms, QList<int>{});
    QCOMPARE(mStatusChanges,
             QList<int>({
                                      (StatusChange::CaretX | StatusChange::CaretY | StatusChange::ModifyChanged),
             }));
    QCOMPARE(mReparseStarts, QList<int>({3,3}));
    QCOMPARE(mReparseCounts, QList<int>({1,1}));

    QVERIFY(!mEdit->canUndo());
    QVERIFY(!mEdit->modified());
}

}
