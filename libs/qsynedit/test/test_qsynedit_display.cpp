#include <QTest>
#include <QCoreApplication>

#include "test_qsynedit_display.h"
#include "test_utils.h"
#include "qsynedit/qsynedit.h"
#include "qsynedit/document.h"
#include "qsynedit/syntaxer/cpp.h"
#include "qsynedit/formatter/cppformatter.h"

namespace QSynedit {

static QStringList makeLines(int count)
{
    QStringList result;
    for (int i = 0; i < count; i++)
        result << QString("line %1: 0123456789").arg(i);
    return result;
}

// brace-containing C++ so that fold scanning creates code blocks
// (blocks: 0..6, 2..4, 7..9)
static QStringList makeFoldableLines()
{
    return {
        "int main() {",
        "    int a = 1;",
        "    if (a) {",
        "        a++;",
        "    }",
        "    return 0;",
        "}",
        "void f() {",
        "    int b = 2;",
        "}",
    };
}

void TestQSyneditDisplay::initTestCase()
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

    QFont defaultFont;
    defaultFont.setFamily(defaultMonoFont());
    defaultFont.setPointSize(14);
    mEdit->setFont(defaultFont);

    mEdit->resize(600, 400);
}

void TestQSyneditDisplay::init()
{
    clearContent();
}

void TestQSyneditDisplay::test_line_row_roundtrip()
{
    mEdit->setContent(makeLines(20));
    for (int line = 0; line < mEdit->lineCount(); line++) {
        QCOMPARE(mEdit->rowToLine(mEdit->lineToRow(line)), line);
        if (line + 1 < mEdit->lineCount())
            QVERIFY(mEdit->lineToRow(line + 1) > mEdit->lineToRow(line));
    }
}

void TestQSyneditDisplay::test_line_row_roundtrip_with_fold()
{
    mEdit->setContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(2, 4));
    // visible lines keep the roundtrip; folding must not change
    // the relative order of visible rows
    for (int line : {0, 1, 2, 5, 6, 7, 8, 9}) {
        QCOMPARE(mEdit->rowToLine(mEdit->lineToRow(line)), line);
    }
    QVERIFY(mEdit->lineToRow(1) < mEdit->lineToRow(2));
    // lines after the collapsed block directly follow the fold start
    QCOMPARE(mEdit->lineToRow(5), mEdit->lineToRow(2) + 1);
    QVERIFY(mEdit->lineToRow(5) < mEdit->lineToRow(6));
}

void TestQSyneditDisplay::test_display_buffer_roundtrip()
{
    QStringList lines = makeLines(10);
    lines[3] = "\tindented with tab";
    lines[5] = "";
    mEdit->setContent(lines);
    for (const CharPos &p : {CharPos{0, 0}, CharPos{5, 1}, CharPos{17, 2},
                             CharPos{0, 3}, CharPos{3, 3}, CharPos{17, 3},
                             CharPos{0, 5}, CharPos{0, 9}, CharPos{18, 9}}) {
        QCOMPARE(mEdit->displayToBufferPos(mEdit->bufferToDisplayPos(p)), p);
    }
}

void TestQSyneditDisplay::test_display_buffer_roundtrip_with_fold()
{
    mEdit->setContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(2, 4));
    for (const CharPos &p : {CharPos{0, 0}, CharPos{5, 1}, CharPos{7, 2},
                             CharPos{0, 5}, CharPos{1, 6}, CharPos{1, 9}}) {
        QCOMPARE(mEdit->displayToBufferPos(mEdit->bufferToDisplayPos(p)), p);
    }
}

void TestQSyneditDisplay::test_page_up_down()
{
    mEdit->setContent(makeLines(50));
    int linesInWindow = mEdit->linesInWindow();
    QVERIFY(linesInWindow > 0 && linesInWindow < 50);

    mEdit->setCaretXY(CharPos{0, 0});
    mEdit->processCommand(EditCommand::PageDown);
    QCOMPARE(mEdit->caretXY().line, linesInWindow);

    mEdit->processCommand(EditCommand::PageDown);
    QCOMPARE(mEdit->caretXY().line, 2 * linesInWindow);

    mEdit->processCommand(EditCommand::PageUp);
    QCOMPARE(mEdit->caretXY().line, linesInWindow);

    mEdit->processCommand(EditCommand::PageUp);
    QCOMPARE(mEdit->caretXY().line, 0);
}

void TestQSyneditDisplay::test_page_top_bottom()
{
    mEdit->setContent(makeLines(50));
    mEdit->setCaretXY(CharPos{0, 10});

    mEdit->processCommand(EditCommand::PageTop);
    QCOMPARE(mEdit->caretXY().line, 0);

    mEdit->processCommand(EditCommand::PageBottom);
    QCOMPARE(mEdit->caretXY().line, mEdit->linesInWindow() - 1);
}

void TestQSyneditDisplay::test_file_start_end()
{
    mEdit->setContent(makeLines(50));
    mEdit->processCommand(EditCommand::FileEnd);
    QCOMPARE(mEdit->caretXY().line, mEdit->lineCount() - 1);
    mEdit->processCommand(EditCommand::FileStart);
    QCOMPARE(mEdit->caretXY(), CharPos(0, 0));
}

void TestQSyneditDisplay::test_scroll_top_pos()
{
    mEdit->setContent(makeLines(50));
    int value = 3 * mEdit->textHeight();
    mEdit->setTopPos(value);
    QCOMPARE(mEdit->topPos(), value);
}

void TestQSyneditDisplay::test_ensure_caret_visible()
{
    mEdit->setContent(makeLines(50));
    mEdit->setTopPos(0);
    mEdit->setCaretXYCentered(CharPos{0, 49});
    int caretPixelY = mEdit->displayCoordToPixels(mEdit->displayXY()).y();
    QVERIFY(caretPixelY >= 0);
    QVERIFY(caretPixelY < mEdit->viewport()->height());
}

void TestQSyneditDisplay::test_mouse_positioning()
{
    mEdit->setContent(makeLines(10));
    int x = mEdit->gutterWidth() + 2 + mEdit->charWidth() * 2;
    int y = mEdit->textHeight() + 2;
    CharPos pos = mEdit->displayToBufferPos(mEdit->pixelsToGlyphPos(x, y));
    QCOMPARE(pos, CharPos(2, 1));
}

void TestQSyneditDisplay::test_mouse_positioning_with_fold()
{
    mEdit->setContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(2, 4));
    // locate the row showing buffer line 5, click into it,
    // and verify it maps back to line 5
    QPoint pt = mEdit->displayCoordToPixels(mEdit->bufferToDisplayPos(CharPos{4, 5}));
    CharPos pos = mEdit->displayToBufferPos(
                mEdit->pixelsToGlyphPos(pt.x(), pt.y() + 2));
    QCOMPARE(pos, CharPos(4, 5));
}

}
