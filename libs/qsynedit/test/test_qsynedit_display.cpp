#include <QTest>
#include <QCoreApplication>
#include <QScrollBar>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

#include "test_qsynedit_display.h"
#include "test_utils.h"
#include "qsynedit/qsynedit.h"
#include "qsynedit/document.h"
#include "qsynedit/syntaxer/cpp.h"
#include "qsynedit/formatter/cppformatter.h"
#include "qsynedit/wraplayoutjob.h"

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
    mEdit->loadContent(makeLines(20));
    for (int line = 0; line < mEdit->lineCount(); line++) {
        QCOMPARE(mEdit->rowToLine(mEdit->lineToRow(line)), line);
        if (line + 1 < mEdit->lineCount())
            QVERIFY(mEdit->lineToRow(line + 1) > mEdit->lineToRow(line));
    }
}

void TestQSyneditDisplay::test_line_row_roundtrip_with_fold()
{
    mEdit->loadContent(makeFoldableLines());
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
    mEdit->loadContent(lines);
    for (const CharPos &p : {CharPos{0, 0}, CharPos{5, 1}, CharPos{17, 2},
                             CharPos{0, 3}, CharPos{3, 3}, CharPos{17, 3},
                             CharPos{0, 5}, CharPos{0, 9}, CharPos{18, 9}}) {
        QCOMPARE(mEdit->displayToBufferPos(mEdit->bufferToDisplayPos(p)), p);
    }
}

void TestQSyneditDisplay::test_display_buffer_roundtrip_with_fold()
{
    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(2, 4));
    for (const CharPos &p : {CharPos{0, 0}, CharPos{5, 1}, CharPos{7, 2},
                             CharPos{0, 5}, CharPos{1, 6}, CharPos{1, 9}}) {
        QCOMPARE(mEdit->displayToBufferPos(mEdit->bufferToDisplayPos(p)), p);
    }
}

void TestQSyneditDisplay::test_mouse_press_moves_caret()
{
    mEdit->loadContent(makeLines(10));
    int x = mEdit->gutterWidth() + 2 + mEdit->charWidth() * 2;
    int y = mEdit->textHeight() + 2;
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(x, y));
    QCOMPARE(mEdit->caretPos(), CharPos(2, 1));
    QCOMPARE(mEdit->mActiveSelectionMode, SelectionMode::Normal);
}

void TestQSyneditDisplay::test_mouse_press_moves_caret_wrap()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    // click into the second segment of the wrapped line
    const WrapLayout &layout = mEdit->wrapLayout(1);
    QVERIFY(layout.segmentStartChars.count() > 1);
    int start = layout.segmentStartChars[1];
    QPoint pt = mEdit->displayCoordToPixels(mEdit->bufferToDisplayPos(CharPos{start, 1}));
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier,
                      pt + QPoint(0, 2));
    QCOMPARE(mEdit->caretPos(), CharPos(start, 1));

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_mouse_press_moves_caret_fold()
{
    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(2, 4));
    // click into the row showing buffer line 5 (directly below the fold)
    QPoint pt = mEdit->displayCoordToPixels(mEdit->bufferToDisplayPos(CharPos{4, 5}));
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier,
                      pt + QPoint(0, 2));
    QCOMPARE(mEdit->caretPos(), CharPos(4, 5));
    mEdit->unCollpaseAll();
}

void TestQSyneditDisplay::test_mouse_double_click_selects_word()
{
    mEdit->loadContent(QStringList{"int main() {"});
    // double-click on "main" (chars 4..7), driving the double click
    // manually (press/release/dblclick/release) like Qt delivers it
    QPoint pt = mEdit->displayCoordToPixels(mEdit->bufferToDisplayPos(CharPos{5, 0}))
            + QPoint(0, 2);
    QTest::mousePress(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, pt);
    QTest::mouseRelease(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, pt);
    QTest::mouseEvent(QTest::MouseDClick, mEdit->viewport(), Qt::LeftButton,
                      Qt::NoModifier, pt);
    QTest::mouseRelease(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, pt);
    QCOMPARE(mEdit->selBegin(), CharPos(4, 0));
    QCOMPARE(mEdit->selEnd(), CharPos(8, 0));
    QCOMPARE(mEdit->selText(), QString("main"));
}

void TestQSyneditDisplay::test_mouse_alt_press_column_mode()
{
    mEdit->loadContent(makeLines(5));
    QPoint pt(mEdit->gutterWidth() + 4, mEdit->textHeight() + 2);
    QTest::mousePress(mEdit->viewport(), Qt::LeftButton, Qt::AltModifier, pt);
    QCOMPARE(mEdit->mActiveSelectionMode, SelectionMode::Column);
    QTest::mouseRelease(mEdit->viewport(), Qt::LeftButton, Qt::AltModifier, pt);
    // a plain press switches back to normal mode
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, pt);
    QCOMPARE(mEdit->mActiveSelectionMode, SelectionMode::Normal);
}

void TestQSyneditDisplay::test_gutter_click_toggles_fold()
{
    mEdit->loadContent(makeFoldableLines());
    // the fold button of the block starting at line 2
    int h = mEdit->textHeight();
    int x = mEdit->gutterWidth() - mEdit->gutter().rightOffset() + 2;
    int y = 2 * h + 2;
    QVERIFY(mEdit->lineToRow(5) == 5);  // not folded yet
    QTest::mousePress(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(x, y));
    QTest::mouseRelease(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(x, y));
    // lines 3..4 hidden: line 5 now directly follows line 2
    QCOMPARE(mEdit->lineToRow(5), mEdit->lineToRow(2) + 1);
    mEdit->unCollpaseAll();
}

void TestQSyneditDisplay::test_gutter_fold_button_paints()
{
    mEdit->loadContent(makeFoldableLines());
    QImage image = mEdit->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    int h = mEdit->textHeight();
    // the fold button square of the block starting at line 2
    int x0 = mEdit->gutterWidth() - mEdit->gutter().rightOffset();
    int y0 = 2 * h;
    QSet<QRgb> colors;
    for (int x = x0; x < x0 + mEdit->gutter().rightOffset() - 2; x++)
        for (int y = y0 + 2; y < y0 + h - 2; y++)
            colors.insert(image.pixel(x, y));
    QVERIFY(colors.count() >= 2);

    // same with word wrap on
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    image = mEdit->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    colors.clear();
    for (int x = x0; x < x0 + mEdit->gutter().rightOffset() - 2; x++)
        for (int y = y0 + 2; y < y0 + h - 2; y++)
            colors.insert(image.pixel(x, y));
    QVERIFY(colors.count() >= 2);

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_wrap_selection_paints_on_start_segment()
{
    // regression: the selection's VisualPos.x is segment-relative (canvas
    // x); computeSelectionInfo used line-absolute x, which double-shifted
    // the start/end comparisons and dropped the highlight on the
    // selection's start segment when it wasn't the line's first
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    const WrapLayout &layout = mEdit->wrapLayout(1);
    QVERIFY(layout.segmentStartChars.count() > 1);

    // select from mid segment 1 of line 1 into line 2
    int start = layout.segmentStartChars[1] + 2;
    mEdit->setCaretAndSelection(CharPos{5, 2}, CharPos{start, 1}, CharPos{5, 2});

    QImage image = mEdit->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    int h = mEdit->textHeight();
    int row = mEdit->lineSegmentToRow(1, 1);
    int y = row * h + h / 2;
    // the selection runs to the row's end: the right third must contain
    // highlight-background pixels
    QRgb highlight = mEdit->palette().color(QPalette::Highlight).rgb();
    int x0 = mEdit->gutterWidth() + 2 + mEdit->viewport()->width() / 2;
    bool found = false;
    for (int x = x0; x < mEdit->viewport()->width() - 2; x++) {
        if (image.pixel(x, y) == highlight) {
            found = true;
            break;
        }
    }
    QVERIFY(found);

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_dnd_drop_path()
{
    // the drop path's edit dance: dragEnter adds a scratch line at the end,
    // drop inserts the text, deletes the original (Move) and the scratch
    // line, and restores caret/scroll. Wrap mode + a background sweep in
    // flight must not interfere.
    QStringList lines;
    for (int i = 0; i < 200; i++)
        lines << QString("line %1 ").arg(i) + QString(200, 'x');
    mEdit->loadContent(lines);
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    mEdit->setOptions(opts);

    // select a word in the middle of the document
    mEdit->setCaretAndSelection(CharPos{10, 100}, CharPos{5, 100}, CharPos{10, 100});
    QVERIFY(mEdit->selAvail());
    QString selected = mEdit->selText();

    QMimeData mime;
    mime.setText(selected);
    QDragEnterEvent enterEvent(QPoint(100, 100), Qt::MoveAction, &mime,
                               Qt::LeftButton, Qt::NoModifier);
    mEdit->dragEnterEvent(&enterEvent);
    QVERIFY(mEdit->mDragging);
    QCOMPARE(mEdit->lineCount(), 201);  // the scratch line

    // let background batches arrive mid-drag
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 30000);

    // drop at line 50
    QPoint pt = mEdit->displayCoordToPixels(mEdit->bufferToDisplayPos(CharPos{3, 50}))
            + QPoint(0, 2);
    QDropEvent dropEvent(pt, Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
    mEdit->dropEvent(&dropEvent);
    QVERIFY(!mEdit->mDragging);

    // content: the word moved from line 100 to line 50
    QCOMPARE(mEdit->lineCount(), 200);
    QVERIFY(mEdit->lineText(50).contains(selected));
    QVERIFY(!mEdit->lineText(100).contains(selected));

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_wrap_mapping_reset_keeps_anchor()
{
    // regression: a mapping reset (resize/font/wrap toggle) during the
    // estimate window must not lose the anchor or leave stale content
    // painted — the pixel offset is re-derived from the anchor under the
    // estimate mapping, and the scrollbar estimate never pulls the view back
    QStringList lines;
    for (int i = 0; i < 200; i++)
        lines << QString("line %1 ").arg(i) + QString(200, 'x');
    mEdit->loadContent(lines);
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    mEdit->setOptions(opts);
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 30000);
    mEdit->setTopLineSegment(50, 0);
    QCOMPARE(mEdit->mTopLine, 50);

    int h = mEdit->textHeight();
    // grab() forces a resize on the never-shown widget, which resets the
    // mapping mid-estimate; the anchor and the painted content must survive
    QImage image = mEdit->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(mEdit->mTopLine, 50);
    QCOMPARE(mEdit->topPos(), mEdit->lineSegmentToRow(50, 0) * h);
    for (int row = 0; row <= 3; row++) {
        QSet<QRgb> colors;
        int y = row * h + h / 2;
        for (int x = mEdit->gutterWidth() + 2; x < mEdit->viewport()->width() - 2; x++)
            colors.insert(image.pixel(x, y));
        QVERIFY2(colors.count() >= 2,
                 qPrintable(QString("row %1 blank after mapping reset").arg(row)));
    }

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_paging_wrap_before_ready()
{
    // caret vertical movement walks segment by segment from the caret and
    // computes crossed lines' layouts on demand: correct landings even
    // while the background sweep is still running
    QStringList lines;
    for (int i = 0; i < 100; i++)
        lines << QString("line %1 ").arg(i) + QString(200, 'x');
    mEdit->loadContent(lines);
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    mEdit->setOptions(opts);
    // no wait for the sweep: everything below the window is an estimate

    int windowRows = mEdit->linesInWindow();
    int prevLine = 0, prevSegment = 0;
    for (int i = 0; i < 5; i++) {
        mEdit->processCommand(EditCommand::PageDown);
        // a line may wrap into more segments than a window holds rows, so
        // strict advance is checked in (line, segment) space, not by line
        int segment, x;
        mEdit->charToSegmentCoord(mEdit->caretLine(), mEdit->caretCh(), segment, x);
        QVERIFY(mEdit->caretLine() > prevLine
                || (mEdit->caretLine() == prevLine && segment > prevSegment));
        prevLine = mEdit->caretLine();
        prevSegment = segment;
        // the caret must end up fully visible
        int row = mEdit->windowRowOfLineSegment(mEdit->caretLine(), segment);
        QVERIFY(row >= 0 && row < windowRows);
    }
    // and back up
    for (int i = 0; i < 5; i++) {
        mEdit->processCommand(EditCommand::PageUp);
        int segment, x;
        mEdit->charToSegmentCoord(mEdit->caretLine(), mEdit->caretCh(), segment, x);
        QVERIFY(mEdit->caretLine() < prevLine
                || (mEdit->caretLine() == prevLine && segment < prevSegment));
        prevLine = mEdit->caretLine();
        prevSegment = segment;
    }
    // estimates may shift mid-test (the sweep completes in the background),
    // so down/up counts need not cancel exactly; paging up to a standstill
    // must converge on the very start
    for (int i = 0; i < 10 && (prevLine > 0 || prevSegment > 0); i++) {
        mEdit->processCommand(EditCommand::PageUp);
        int segment, x;
        mEdit->charToSegmentCoord(mEdit->caretLine(), mEdit->caretCh(), segment, x);
        prevLine = mEdit->caretLine();
        prevSegment = segment;
    }
    QCOMPARE(mEdit->caretLine(), 0);

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_page_up_down()
{
    mEdit->loadContent(makeLines(50));
    int linesInWindow = mEdit->linesInWindow();
    QVERIFY(linesInWindow > 0 && linesInWindow < 50);

    mEdit->setCaretPos(CharPos{0, 0});
    mEdit->processCommand(EditCommand::PageDown);
    QCOMPARE(mEdit->caretPos().line, linesInWindow);

    mEdit->processCommand(EditCommand::PageDown);
    QCOMPARE(mEdit->caretPos().line, 2 * linesInWindow);

    mEdit->processCommand(EditCommand::PageUp);
    QCOMPARE(mEdit->caretPos().line, linesInWindow);

    mEdit->processCommand(EditCommand::PageUp);
    QCOMPARE(mEdit->caretPos().line, 0);
}

void TestQSyneditDisplay::test_page_top_bottom()
{
    mEdit->loadContent(makeLines(50));
    int windowRows = mEdit->linesInWindow();
    QVERIFY(windowRows > 0 && windowRows < 50);
    // start with the caret inside the window: setting it must not scroll,
    // so the top stays at line 0 regardless of the environment's metrics
    mEdit->setCaretPos(CharPos{0, windowRows / 2});

    mEdit->processCommand(EditCommand::PageTop);
    QCOMPARE(mEdit->caretPos().line, 0);

    mEdit->processCommand(EditCommand::PageBottom);
    QCOMPARE(mEdit->caretPos().line, windowRows - 1);
}

void TestQSyneditDisplay::test_file_start_end()
{
    mEdit->loadContent(makeLines(50));
    mEdit->processCommand(EditCommand::FileEnd);
    QCOMPARE(mEdit->caretPos().line, mEdit->lineCount() - 1);
    mEdit->processCommand(EditCommand::FileStart);
    QCOMPARE(mEdit->caretPos(), CharPos(0, 0));
}

void TestQSyneditDisplay::test_scroll_top_pos()
{
    mEdit->loadContent(makeLines(50));
    int value = 3 * mEdit->textHeight();
    mEdit->setTopPos(value);
    QCOMPARE(mEdit->topPos(), value);
}

void TestQSyneditDisplay::test_scrollbar_drag_is_pixel_smooth()
{
    // thumb drags report pixel positions: the top offset must keep the
    // sub-row remainder (content shifts by single pixels)
    mEdit->loadContent(makeLines(50));
    int h = mEdit->textHeight();
    QScrollBar *vsb = mEdit->verticalScrollBar();
    vsb->setValue(10 * h + h / 2);
    QCOMPARE(mEdit->topPos(), 10 * h + h / 2);
    QCOMPARE(mEdit->mTopLine, 10);
    QCOMPARE(mEdit->mTopSegment, 0);
    // the painted first row is shifted up by exactly the remainder
    QCOMPARE(mEdit->displayCoordToPixels(mEdit->bufferToDisplayPos(CharPos{0, 10})).y(),
             -(h / 2));
}

void TestQSyneditDisplay::test_ensure_caret_visible()
{
    mEdit->loadContent(makeLines(50));
    mEdit->setTopPos(0);
    mEdit->setCaretPosCentered(CharPos{0, 49});
    int caretPixelY = mEdit->displayCoordToPixels(mEdit->displayXY()).y();
    QVERIFY(caretPixelY >= 0);
    QVERIFY(caretPixelY < mEdit->viewport()->height());
}

void TestQSyneditDisplay::test_mouse_positioning()
{
    mEdit->loadContent(makeLines(10));
    int x = mEdit->gutterWidth() + 2 + mEdit->charWidth() * 2;
    int y = mEdit->textHeight() + 2;
    CharPos pos = mEdit->displayToBufferPos(mEdit->pixelsToGlyphPos(x, y));
    QCOMPARE(pos, CharPos(2, 1));
}

void TestQSyneditDisplay::test_mouse_positioning_with_fold()
{
    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(2, 4));
    // locate the row showing buffer line 5, click into it,
    // and verify it maps back to line 5
    QPoint pt = mEdit->displayCoordToPixels(mEdit->bufferToDisplayPos(CharPos{4, 5}));
    CharPos pos = mEdit->displayToBufferPos(
                mEdit->pixelsToGlyphPos(pt.x(), pt.y() + 2));
    QCOMPARE(pos, CharPos(4, 5));
}

void TestQSyneditDisplay::test_wordwrap_basics()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);

    int width = mEdit->wrapWidth();
    QVERIFY(width > 0);
    QVERIFY(mEdit->wrapSegmentCount(1) > 1);
    QCOMPARE(mEdit->wrapSegmentCount(0), 1);
    QCOMPARE(mEdit->wrapSegmentCount(2), 1);

    // every segment fits the wrap width and segments partition the line
    const WrapLayout &layout = mEdit->wrapLayout(1);
    int segments = layout.segmentStartChars.count();
    QCOMPARE(segments, mEdit->wrapSegmentCount(1));
    QCOMPARE(layout.segmentStartChars.first(), 0);
    QCOMPARE(layout.segmentStartXs.first(), 0);
    for (int i=0;i<segments;i++) {
        int start = layout.segmentStartChars[i];
        int end = mEdit->wrapSegmentEndChar(1, i);
        QVERIFY(end > start);
        if (i+1<segments) {
            int w = layout.segmentStartXs[i+1] - layout.segmentStartXs[i];
            QVERIFY2(w <= width+1, qPrintable(QString("seg %1 w %2 width %3").arg(i).arg(w).arg(width)));
        }
    }
    QVERIFY(layout.totalWidth - layout.segmentStartXs.last() <= width+1);
    // row mapping roundtrip
    for (int row=0; row<mEdit->displayLineCount(); row++) {
        int line, seg;
        mEdit->rowToLineSegment(row, line, seg);
        QCOMPARE(mEdit->lineSegmentToRow(line, seg), row);
    }
    // disabling restores single-segment behavior
    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
    QCOMPARE(mEdit->wrapSegmentCount(1), 1);
    QCOMPARE(mEdit->displayLineCount(), 3);
}

void TestQSyneditDisplay::test_wordwrap_caret_display()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    const WrapLayout &layout = mEdit->wrapLayout(1);
    QVERIFY(layout.segmentStartChars.count() > 1);


    for (int seg=0; seg<layout.segmentStartChars.count(); seg++) {
        int start = layout.segmentStartChars[seg];
        mEdit->setCaretPos(CharPos{start, 1});
        VisualPos coord = mEdit->displayXY();
        QCOMPARE(coord.x, 0);
        QCOMPARE(coord.row, mEdit->lineSegmentToRow(1, seg));
        int end = mEdit->wrapSegmentEndChar(1, seg);
        mEdit->setCaretPos(CharPos{end, 1});
        VisualPos endCoord = mEdit->displayXY();
        // a segment end that is also the next segment's start belongs to
        // the next segment's row; only the line end stays on the same row
        if (seg+1 < layout.segmentStartChars.count())
            QCOMPARE(endCoord.row, mEdit->lineSegmentToRow(1, seg+1));
        else
            QCOMPARE(endCoord.row, coord.row);
        QVERIFY(endCoord.x >= 0);
        QCOMPARE(mEdit->displayToBufferPos(endCoord), CharPos(end, 1));
    }

    // Home/End operate on the wrap segment, not the buffer line.
    // EnhanceEndKey first goes after the last non-blank of the segment,
    // then to the segment end; both targets are derived from the layout
    // so the test does not depend on the environment's font metrics
    mEdit->setCaretPos(CharPos{layout.segmentStartChars[1]+2, 1});
    mEdit->processCommand(QSynedit::EditCommand::LineStart);
    QCOMPARE(mEdit->caretCh(), layout.segmentStartChars[1]);
    int segEnd = mEdit->wrapSegmentEndChar(1, 1);
    QString text = mEdit->lineText(1);
    int lastNonBlank = segEnd - 1;
    while (lastNonBlank >= layout.segmentStartChars[1]
           && (text[lastNonBlank] == ' ' || text[lastNonBlank] == '\t'))
        lastNonBlank--;
    mEdit->processCommand(QSynedit::EditCommand::LineEnd);
    QCOMPARE(mEdit->caretCh(), lastNonBlank + 1);
    mEdit->processCommand(QSynedit::EditCommand::LineEnd);
    QCOMPARE(mEdit->caretCh(), segEnd);
}

void TestQSyneditDisplay::test_wordwrap_vertical_movement()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);

    mEdit->setCaretPos(CharPos{0,0});
    mEdit->processCommand(QSynedit::EditCommand::Down);
    QCOMPARE(mEdit->caretLine(), 1);
    QCOMPARE(mEdit->caretCh(), 0);
    mEdit->processCommand(QSynedit::EditCommand::Down);
    QCOMPARE(mEdit->caretLine(), 1);
    QVERIFY(mEdit->caretCh() > 0);
    mEdit->processCommand(QSynedit::EditCommand::Up);
    QCOMPARE(mEdit->caretCh(), 0);
    mEdit->processCommand(QSynedit::EditCommand::Up);
    QCOMPARE(mEdit->caretLine(), 0);
}

void TestQSyneditDisplay::test_wordwrap_mouse()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    const WrapLayout &layout = mEdit->wrapLayout(1);
    QVERIFY(layout.segmentStartChars.count() > 1);

    // click on the second segment of the long line
    int seg = 1;
    int start = layout.segmentStartChars[seg];
    QPoint pt = mEdit->displayCoordToPixels(mEdit->bufferToDisplayPos(CharPos{start, 1}));
    CharPos pos = mEdit->displayToBufferPos(mEdit->pixelsToGlyphPos(pt.x(), pt.y() + 2));
    QCOMPARE(pos, CharPos(start, 1));

    // and on the first segment
    pt = mEdit->displayCoordToPixels(mEdit->bufferToDisplayPos(CharPos{2, 1}));
    pos = mEdit->displayToBufferPos(mEdit->pixelsToGlyphPos(pt.x(), pt.y() + 2));
    QCOMPARE(pos, CharPos(2, 1));
}

void TestQSyneditDisplay::test_wordwrap_fold()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "    int a = 1;",
        "    if (a) {",
        "        a = 2;",
        "    }",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    int longLine = 1;
    int longSegments = mEdit->wrapSegmentCount(longLine);
    QVERIFY(longSegments > 1);
    int rowsBeforeFold = mEdit->displayLineCount();

    QVERIFY(mEdit->collapse(3, 5));
    // the fold hides whole lines with all their segments; only the long
    // line's segments remain beyond the fold
    int rowsAfterFold = mEdit->displayLineCount();
    QVERIFY(rowsAfterFold < rowsBeforeFold);

    // row mapping stays consistent for the long line
    int seg = longSegments - 1;
    int row = mEdit->lineSegmentToRow(longLine, seg);
    int line, segment;
    mEdit->rowToLineSegment(row, line, segment);
    QCOMPARE(line, longLine);
    QCOMPARE(segment, seg);
}

void TestQSyneditDisplay::test_wordwrap_batch_stale_rejection()
{
    mEdit->loadContent(makeLines(5));
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);

    // a batch with a stale generation must be dropped entirely
    WrapLayoutBatch batch;
    batch.generation = mEdit->mWrapGeneration + 100;
    batch.contentVersion = mEdit->mWrapContentVersion;
    batch.document = mEdit->document();
    WrapLayout fake;
    fake.segmentStartChars = {0, 3};
    fake.segmentStartXs = {0, 10};
    batch.layouts.insert(mEdit->document()->getLineSeq(1), fake);
    batch.finished = true;
    mEdit->onWrapLayoutBatch(batch);
    QVERIFY(!mEdit->mWrapLayouts.contains(mEdit->document()->getLineSeq(1)));
    QVERIFY(!mEdit->mWrapAllComputed);

    // a batch from a different document must be dropped too
    batch.generation = mEdit->mWrapGeneration;
    batch.document = std::make_shared<Document>();
    mEdit->onWrapLayoutBatch(batch);
    QVERIFY(!mEdit->mWrapLayouts.contains(mEdit->document()->getLineSeq(1)));

    // a valid batch is accepted
    batch.document = mEdit->document();
    mEdit->onWrapLayoutBatch(batch);
    QVERIFY(mEdit->mWrapLayouts.contains(mEdit->document()->getLineSeq(1)));

    // after an edit, a result computed BEFORE the edit is rejected for that
    // line, while other lines are still accepted. (Since batch B the edit
    // itself relayouts the edited line synchronously, so rejection shows
    // as the stale batch not overwriting that fresh layout.)
    size_t editedSeq = mEdit->document()->getLineSeq(1);
    size_t otherSeq = mEdit->document()->getLineSeq(2);
    mEdit->mWrapLayouts.remove(editedSeq);
    mEdit->mWrapLayouts.remove(otherSeq);
    WrapLayoutBatch early = batch;
    early.layouts.insert(otherSeq, fake);
    early.contentVersion = mEdit->mWrapContentVersion - 1;
    early.finished = false;
    mEdit->setCaretPos(CharPos{0,1});
    QTest::keyPress(mEdit.get(), 'x');
    // the edit synchronously relayout the edited line
    QVERIFY(mEdit->mWrapLayouts.contains(editedSeq));
    WrapLayout freshLayout = mEdit->mWrapLayouts.value(editedSeq);
    mEdit->onWrapLayoutBatch(early);
    // the stale batch must not overwrite it
    QCOMPARE(mEdit->mWrapLayouts.value(editedSeq).segmentStartChars,
             freshLayout.segmentStartChars);
    QCOMPARE(mEdit->mWrapLayouts.value(editedSeq).totalWidth,
             freshLayout.totalWidth);
    QVERIFY(mEdit->mWrapLayouts.contains(otherSeq));

    // a batch computed AFTER the edit is accepted for the edited line
    WrapLayoutBatch later = batch;
    later.contentVersion = mEdit->mWrapContentVersion;
    later.finished = false;
    mEdit->onWrapLayoutBatch(later);
    QVERIFY(mEdit->mWrapLayouts.contains(editedSeq));
}

void TestQSyneditDisplay::test_wordwrap_segment_coord_conversions()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    const WrapLayout &layout = mEdit->wrapLayout(1);
    int segments = layout.segmentStartChars.count();
    QVERIFY(segments > 1);
    int lineLen = mEdit->lineText(1).length();

    // (line, char) -> (segment, x) -> char roundtrips on every line
    // (roundtrip target is the glyph start: a char inside a multi-char
    // glyph maps to the glyph's start)
    for (int line = 0; line < mEdit->lineCount(); line++) {
        int len = mEdit->lineText(line).length();
        for (int ch : {0, 1, len/2, len-1, len}) {
            if (ch < 0 || ch > len)
                continue;
            int seg, x;
            mEdit->charToSegmentCoord(line, ch, seg, x);
            QCOMPARE(seg, mEdit->wrapSegmentOfChar(line, ch));
            QVERIFY(x >= 0);
            QCOMPARE(mEdit->segmentCoordToChar(line, seg, x),
                     mEdit->document()->charToGlyphStartChar(line, ch));
        }
    }
    // segment starts map to x == 0; the last char of a segment stays within
    // the segment's pixel width
    for (int seg = 0; seg < segments; seg++) {
        int foundSeg, x;
        mEdit->charToSegmentCoord(1, layout.segmentStartChars[seg], foundSeg, x);
        QCOMPARE(foundSeg, seg);
        QCOMPARE(x, 0);
        if (seg+1 < segments) {
            int segWidth = layout.segmentStartXs[seg+1] - layout.segmentStartXs[seg];
            int lastChar = layout.segmentStartChars[seg+1] - 1;
            mEdit->charToSegmentCoord(1, lastChar, foundSeg, x);
            QCOMPARE(foundSeg, seg);
            QVERIFY(x < segWidth);
        }
    }
    // out-of-range segment / x are clamped
    QCOMPARE(mEdit->segmentCoordToChar(1, 0, 0), 0);
    QCOMPARE(mEdit->segmentCoordToChar(1, segments-1, layout.totalWidth), lineLen);
    QCOMPARE(mEdit->segmentCoordToChar(1, segments+10, 0), layout.segmentStartChars.last());

    // without wrap everything lives in segment 0 and the roundtrip holds
    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
    int seg, x;
    mEdit->charToSegmentCoord(1, 10, seg, x);
    QCOMPARE(seg, 0);
    QCOMPARE(mEdit->segmentCoordToChar(1, seg, x), 10);
}

void TestQSyneditDisplay::test_advance_retreat_line_segment()
{
    // no wrap: one segment per line
    mEdit->loadContent(makeLines(5));
    int line = 0, seg = 0;
    for (int i = 1; i < 5; i++) {
        QVERIFY(mEdit->advanceLineSegment(line, seg));
        QCOMPARE(line, i);
        QCOMPARE(seg, 0);
    }
    // at the document end: false, position unchanged
    QVERIFY(!mEdit->advanceLineSegment(line, seg));
    QCOMPARE(line, 4);
    QCOMPARE(seg, 0);
    for (int i = 3; i >= 0; i--) {
        QVERIFY(mEdit->retreatLineSegment(line, seg));
        QCOMPARE(line, i);
        QCOMPARE(seg, 0);
    }
    QVERIFY(!mEdit->retreatLineSegment(line, seg));
    QCOMPARE(line, 0);
    QCOMPARE(seg, 0);

    // wrap: the walk visits every segment exactly once, in global-row order
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    int longSegments = mEdit->wrapSegmentCount(1);
    QVERIFY(longSegments > 1);

    line = 0; seg = 0;
    QCOMPARE(mEdit->lineSegmentToRow(line, seg), 0);
    int steps = 0;
    while (mEdit->advanceLineSegment(line, seg)) {
        steps++;
        QCOMPARE(mEdit->lineSegmentToRow(line, seg), steps);
    }
    QCOMPARE(steps+1, mEdit->displayLineCount());
    QCOMPARE(line, 2);
    QCOMPARE(seg, 0);
    // walking back visits the same positions in reverse
    while (mEdit->retreatLineSegment(line, seg)) {
        steps--;
        QCOMPARE(mEdit->lineSegmentToRow(line, seg), steps);
    }
    QCOMPARE(steps, 0);
    QCOMPARE(line, 0);
    QCOMPARE(seg, 0);

    // crossing the wrapped line step by step
    line = 0; seg = 0;
    QVERIFY(mEdit->advanceLineSegment(line, seg));
    QCOMPARE(line, 1);
    QCOMPARE(seg, 0);
    for (int i = 1; i < longSegments; i++) {
        QVERIFY(mEdit->advanceLineSegment(line, seg));
        QCOMPARE(line, 1);
        QCOMPARE(seg, i);
    }
    QVERIFY(mEdit->advanceLineSegment(line, seg));
    QCOMPARE(line, 2);
    QCOMPARE(seg, 0);

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_advance_retreat_with_fold()
{
    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(2, 4));
    // visible lines: 0,1,2,5,6,7,8,9
    int line = 1, seg = 0;
    QVERIFY(mEdit->advanceLineSegment(line, seg));
    QCOMPARE(line, 2);
    // advance skips the hidden lines 3..4
    QVERIFY(mEdit->advanceLineSegment(line, seg));
    QCOMPARE(line, 5);
    QCOMPARE(seg, 0);
    QVERIFY(mEdit->retreatLineSegment(line, seg));
    QCOMPARE(line, 2);
    QCOMPARE(seg, 0);

    // starting from a hidden line: advance lands on the next visible line,
    // retreat on the previous one (its last segment)
    line = 3; seg = 0;
    QVERIFY(mEdit->advanceLineSegment(line, seg));
    QCOMPARE(line, 5);
    QCOMPARE(seg, 0);
    line = 4; seg = 0;
    QVERIFY(mEdit->retreatLineSegment(line, seg));
    QCOMPARE(line, 2);
    QCOMPARE(seg, mEdit->wrapSegmentCount(2)-1);

    // a hidden line with no visible line below snaps back to the last
    // visible segment and reports failure
    QVERIFY(mEdit->collapse(7, 9));
    line = 8; seg = 0;
    QVERIFY(!mEdit->advanceLineSegment(line, seg));
    QCOMPARE(line, 7);
    QCOMPARE(seg, mEdit->wrapSegmentCount(7)-1);

    // fold + wrap: the full walk still matches the global row mapping
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "    int a = 1;",
        "    if (a) {",
        "        a = 2;",
        "    }",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    QVERIFY(mEdit->wrapSegmentCount(1) > 1);
    QVERIFY(mEdit->collapse(3, 5));
    line = 0; seg = 0;
    int steps = 0;
    while (mEdit->advanceLineSegment(line, seg)) {
        steps++;
        QCOMPARE(mEdit->lineSegmentToRow(line, seg), steps);
    }
    QCOMPARE(steps+1, mEdit->displayLineCount());

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_toppos_anchor_sync()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "    return 0;",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    mEdit->setOptions(opts);
    int h = mEdit->textHeight();

    // no wrap: the anchor is (topRow, 0)
    for (int row : {0, 1, 2, 3, 1, 0}) {
        mEdit->setTopPos(row * h);
        QCOMPARE(mEdit->mTopLine, row);
        QCOMPARE(mEdit->mTopSegment, 0);
    }
    // a sub-row pixel offset keeps the same anchor
    mEdit->setTopPos(2 * h + h / 2);
    QCOMPARE(mEdit->mTopLine, 2);
    QCOMPARE(mEdit->mTopSegment, 0);

    // wrap: the anchor must always match the global mapping of the top row
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    QVERIFY(mEdit->wrapSegmentCount(1) > 1);
    int totalRows = mEdit->displayLineCount();
    QVERIFY(totalRows > 4);
    // small steps down, then one big jump up
    for (int row = 0; row < totalRows; row++) {
        mEdit->setTopPos(row * h);
        int line, seg;
        mEdit->rowToLineSegment(row, line, seg);
        QCOMPARE(mEdit->mTopLine, line);
        QCOMPARE(mEdit->mTopSegment, seg);
    }
    mEdit->setTopPos(h);
    QCOMPARE(mEdit->mTopLine, 1);
    QCOMPARE(mEdit->mTopSegment, 0);
    // past the end: clamps to the last visible segment
    mEdit->setTopPos((totalRows + 100) * h);
    QCOMPARE(mEdit->mTopLine, 3);
    QCOMPARE(mEdit->mTopSegment, 0);
    // scrollbar-driven scrolling keeps the same invariant
    mEdit->onVScrolled(2 * h);
    int line, seg;
    mEdit->rowToLineSegment(2, line, seg);
    QCOMPARE(mEdit->mTopLine, line);
    QCOMPARE(mEdit->mTopSegment, seg);

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_wrap_toggle_preserves_anchor()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "    return 0;",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    mEdit->setOptions(opts);
    int h = mEdit->textHeight();
    mEdit->setTopPos(2 * h);
    QCOMPARE(mEdit->mTopLine, 2);

    // wrap on: the top line stays, the pixel offset follows its new row
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    QCOMPARE(mEdit->mTopLine, 2);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), mEdit->lineSegmentToRow(2, 0) * h);

    // an anchor inside the wrapped line clamps its segment on wrap-off
    mEdit->setTopPos(mEdit->lineSegmentToRow(1, 1) * h);
    QCOMPARE(mEdit->mTopLine, 1);
    QCOMPARE(mEdit->mTopSegment, 1);
    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
    QCOMPARE(mEdit->mTopLine, 1);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), h);

    // and back on: the top line still survives the roundtrip
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    QCOMPARE(mEdit->mTopLine, 1);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), h);

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_edit_shifts_anchor_marker_style()
{
    // edits through the document (as another view of it would) shift the
    // anchor marker-style: the content at the viewport top stays put
    mEdit->loadContent(makeLines(50));
    int h = mEdit->textHeight();
    mEdit->setTopPos(30 * h);
    QCOMPARE(mEdit->mTopLine, 30);

    // insert above the anchor: the anchor follows its content
    mEdit->document()->insertLines(5, 3, false);
    QCOMPARE(mEdit->mTopLine, 33);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 33 * h);
    // the scrollbar tracks the derived pixel offset
    QCOMPARE(mEdit->verticalScrollBar()->value(), 33 * h);

    // delete above the anchor: the anchor moves back up
    mEdit->document()->deleteLines(5, 3, false);
    QCOMPARE(mEdit->mTopLine, 30);
    QCOMPARE(mEdit->topPos(), 30 * h);

    // delete a band containing the anchor: clamp to the band start
    mEdit->document()->deleteLines(28, 5, false);
    QCOMPARE(mEdit->mTopLine, 28);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 28 * h);

    // a user scroll drives the same derivation through onVScrolled
    mEdit->verticalScrollBar()->setValue(2 * h);
    QCOMPARE(mEdit->topPos(), 2 * h);
    QCOMPARE(mEdit->mTopLine, 2);
    QCOMPARE(mEdit->mTopSegment, 0);
}

void TestQSyneditDisplay::test_setline_clamps_anchor_segment()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "    return 0;",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    mEdit->setOptions(opts);
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    int segs = mEdit->wrapSegmentCount(1);
    QVERIFY(segs > 2);
    int h = mEdit->textHeight();

    // anchor on a later segment of the wrapped line
    mEdit->setTopPos(mEdit->lineSegmentToRow(1, segs-1) * h);
    QCOMPARE(mEdit->mTopLine, 1);
    QCOMPARE(mEdit->mTopSegment, segs-1);

    // shortening the anchor line clamps the anchor segment
    mEdit->document()->putLine(1, "    // short", false);
    QCOMPARE(mEdit->mTopLine, 1);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), h);

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_collapse_adsorbs_anchor()
{
    mEdit->loadContent(makeFoldableLines());
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    mEdit->setOptions(opts);
    int h = mEdit->textHeight();
    // anchor inside the soon-to-be-collapsed block (2..4); the caret sits
    // below the fold and stays visible, so collapse does not scroll to it
    mEdit->setCaretPos(CharPos{0, 8});
    mEdit->setTopPos(3 * h);
    QCOMPARE(mEdit->mTopLine, 3);
    QVERIFY(mEdit->collapse(2, 4));
    // re-adsorbed to the fold start (the nearest visible line above)
    QCOMPARE(mEdit->mTopLine, 2);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 2 * h);

    // uncollapsing keeps the anchor (nothing was hidden by it)
    mEdit->unCollpaseAll();
    QCOMPARE(mEdit->mTopLine, 2);

    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_wrap_batch_recomputes_toppos()
{
    // background refinement must keep the anchor and re-derive the pixel
    // offset from it (no content drift, scrollbar value converges)
    QStringList lines;
    for (int i = 0; i < 10; i++)
        lines << QString("long line %1: ").arg(i) + QString(300, 'x');
    lines << makeLines(40);
    mEdit->loadContent(lines);
    int h = mEdit->textHeight();

    // anchor placed without word wrap (the mapping is trivially exact)
    mEdit->setTopPos(30 * h);
    QCOMPARE(mEdit->mTopLine, 30);
    QCOMPARE(mEdit->mTopSegment, 0);

    // enabling wrap rebuilds the mapping but preserves the anchor...
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    QCOMPARE(mEdit->mTopLine, 30);

    // ...and the background sweep must not move it either: the batch path
    // re-derives the pixel offset from the anchor (its no-clamp assertion
    // holds in debug builds) and syncs the scrollbar value
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    QCOMPARE(mEdit->mTopLine, 30);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), mEdit->lineSegmentToRow(30, 0) * h);
    QCOMPARE(mEdit->verticalScrollBar()->value(), mEdit->topPos());

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_set_top_line_segment()
{
    mEdit->loadContent(makeFoldableLines());
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    mEdit->setOptions(opts);
    int h = mEdit->textHeight();

    mEdit->setTopLineSegment(5, 0);
    QCOMPARE(mEdit->mTopLine, 5);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 5 * h);

    // out-of-range values clamp
    mEdit->setTopLineSegment(-3, 0);
    QCOMPARE(mEdit->mTopLine, 0);
    QCOMPARE(mEdit->topPos(), 0);
    mEdit->setTopLineSegment(100, 7);
    QCOMPARE(mEdit->mTopLine, mEdit->lineCount()-1);
    QCOMPARE(mEdit->mTopSegment, 0);

    // a folded-away line snaps to the fold start
    mEdit->setTopLineSegment(0, 0);
    QVERIFY(mEdit->collapse(2, 4));
    mEdit->setTopLineSegment(3, 0);
    QCOMPARE(mEdit->mTopLine, 2);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 2 * h);
    mEdit->unCollpaseAll();

    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, false);
    mEdit->setOptions(opts);
}

}

#include <random>
#include <tuple>

namespace QSynedit {

static QString dumpFoldRanges(const QList<std::tuple<int,int,bool>> &ranges)
{
    QStringList out;
    for (const auto &[from, to, collapsed] : ranges)
        out << QString("(%1,%2,%3)").arg(from).arg(to).arg(collapsed);
    return out.join(" ");
}

void TestQSyneditDisplay::test_partial_fold_rescan()
{
    // the edit-driven partial rescan must produce the same fold list
    // (from,to,collapsed) as a full rescan
    QStringList lines;
    lines << "int main() {";
    for (int f = 0; f < 12; f++) {
        lines << QString("    void f%1() {").arg(f);
        for (int i = 0; i < 6; i++)
            lines << QString("        int x%1 = %2;").arg(i).arg(f + i);
        lines << "        if (x0) {";
        lines << "            x0++;";
        lines << "        }";
        lines << "    }";
    }
    lines << "}";
    mEdit->loadContent(lines);

    QStringList opLog;
    // capture the last contentChanged payload for diagnostics
    bool lastDirty = false;
    int lastReparseToLine = -2;
    // a dedicated context object: the connection dies with the test.
    // (mEdit outlives it; a dead lambda would keep writing to these
    // stack variables on every later contentChanged and corrupt memory)
    QObject connContext;
    connect(mEdit->document().get(), &Document::contentChanged, &connContext,
            [&](const DocumentChangeSet &, bool dirty, int L) {
        lastDirty = dirty;
        lastReparseToLine = L;
    });
    auto verifyRescan = [&]() {
        QList<std::tuple<int,int,bool>> partial = mEdit->foldRangesForTest();
        mEdit->fullRescanCodeBlocksForTest();
        QList<std::tuple<int,int,bool>> full = mEdit->foldRangesForTest();
        if (full != partial) {
            qDebug() << "lastDirty:" << lastDirty << "reparseToLine:" << lastReparseToLine;
            for (const QString &entry : opLog.mid(qMax(0, opLog.count() - 16)))
                qDebug() << entry;
            qDebug() << "document lines:";
            for (int i = 0; i < mEdit->document()->count(); i++)
                qDebug() << i << ":" << mEdit->document()->getLine(i);
            QFAIL(qPrintable(QString("partial: %1\nfull:    %2")
                             .arg(dumpFoldRanges(partial), dumpFoldRanges(full))));
        }
    };

    verifyRescan();
    // collapse some blocks; their flags must survive unrelated edits
    QVERIFY(mEdit->collapse(1, 11));   // void f0()
    QVERIFY(mEdit->collapse(12, 22));  // void f1()

    std::mt19937 rng(20261101);
    for (int op = 0; op < 200; op++) {
        int n = mEdit->document()->count();
        int kind = rng() % 6;
        int line = rng() % n;
        opLog.append(QString("op %1: kind=%2 line=%3").arg(op).arg(kind).arg(line));
        switch (kind) {
        case 0: // plain SetLine
            mEdit->document()->putLine(
                        line, QString("        int v%1 = %1;").arg(op), true);
            break;
        case 1: // SetLine with a brace: pairing change
            mEdit->document()->putLine(line, (rng()%2) ? "{" : "}", true);
            break;
        case 2: // insert plain or brace line
            mEdit->document()->insertLine(
                        line, (rng()%3) ? "        x++;" : "        if (x) {", true);
            break;
        case 3: // delete a line
            if (n > 1)
                mEdit->document()->deleteLine(line, true);
            break;
        case 4: // insert a blank/plain line
            mEdit->document()->insertLine(line, "", true);
            break;
        case 5: { // toggle a random fold's collapsed state
            QList<std::tuple<int,int,bool>> ranges = mEdit->foldRangesForTest();
            if (!ranges.isEmpty()) {
                auto [from, to, collapsed] = ranges[rng() % ranges.count()];
                if (collapsed)
                    mEdit->uncollapase(from, to);
                else
                    mEdit->collapse(from, to);
            }
            break;
        }
        }
        opLog.append(QString("  => %1").arg(dumpFoldRanges(mEdit->foldRangesForTest())));
        verifyRescan();
    }
}

}

namespace QSynedit {

static QStringList makeShiftFoldLines()
{
    return {
        "int a() {",     // 0
        "    return 1;", // 1
        "}",             // 2
        "int b() {",     // 3
        "    return 2;", // 4
        "}",             // 5
        "int c() {",     // 6
        "    return 3;", // 7
        "}",             // 8
        "int d() {",     // 9
        "    return 4;", // 10
        "}",             // 11
    };
}

void TestQSyneditDisplay::test_fold_shift_maintenance()
{
    auto expectRanges = [&](const QList<std::tuple<int,int,bool>> &expected, const char *what) {
        QVERIFY2(mEdit->foldRangesForTest() == expected,
                 qPrintable(QString("%1: got %2")
                            .arg(what, dumpFoldRanges(mEdit->foldRangesForTest()))));
    };

    // insert lines before all blocks: everything shifts, flags survive
    mEdit->loadContent(makeShiftFoldLines());
    QVERIFY(mEdit->collapse(3, 5));
    mEdit->document()->insertLines(0, 2, true);
    expectRanges({{2, 4, false}, {5, 7, true}, {8, 10, false}, {11, 13, false}},
                 "insert before");

    // insert into the collapsed block: it uncollapses and extends
    mEdit->loadContent(makeShiftFoldLines());
    QVERIFY(mEdit->collapse(3, 5));
    mEdit->document()->insertLine(4, "    x++;", true);
    expectRanges({{0, 2, false}, {3, 6, false}, {7, 9, false}, {10, 12, false}},
                 "insert into collapsed");

    // delete a body line inside a collapsed block: it shrinks and
    // (existing semantics) uncollapses because its range changed
    mEdit->loadContent(makeShiftFoldLines());
    QVERIFY(mEdit->collapse(3, 5));
    mEdit->document()->deleteLine(4, true);
    expectRanges({{0, 2, false}, {3, 4, false}, {5, 7, false}, {8, 10, false}},
                 "delete inside collapsed");

    // delete lines before a collapsed block: shifts, flag survives
    mEdit->loadContent(makeShiftFoldLines());
    QVERIFY(mEdit->collapse(3, 5));
    mEdit->document()->deleteLines(0, 2, true);
    expectRanges({{1, 3, true}, {4, 6, false}, {7, 9, false}},
                 "delete before");

    // delete a block's opener line: the block is gone for good
    mEdit->loadContent(makeShiftFoldLines());
    mEdit->document()->deleteLine(3, true);
    expectRanges({{0, 2, false}, {5, 7, false}, {8, 10, false}},
                 "delete opener");

    // delete a block's closure line: only the closure is deleted, so the
    // block must stay open in the list (otherwise the partial rescan
    // loses it from its initial stack); it ends unfinished (to EOF)
    // since no close is left below
    mEdit->loadContent(makeShiftFoldLines());
    QVERIFY(mEdit->collapse(3, 5));
    mEdit->document()->deleteLine(5, true);
    // b opens at 3 but never closes: c(5,7) and d(8,10) nest inside
    expectRanges({{0, 2, false}, {3, 10, false}, {5, 7, false}, {8, 10, false}},
                 "delete closure keeps block open");

    // move a collapsed block's boundary line: it uncollapses
    mEdit->loadContent(makeShiftFoldLines());
    QVERIFY(mEdit->collapse(3, 5));
    mEdit->document()->moveLine(3, 0, true);
    QVERIFY(mEdit->codeBlockCount() > 0);
    for (const auto &[from, to, collapsed] : mEdit->foldRangesForTest())
        QVERIFY(!collapsed);
    // and the list matches a full rescan
    QList<std::tuple<int,int,bool>> partial = mEdit->foldRangesForTest();
    mEdit->fullRescanCodeBlocksForTest();
    QCOMPARE(mEdit->foldRangesForTest(), partial);

    // move a line up past a collapsed block: the block shifts down and
    // KEEPS its collapsed state (regression: the from > to branch was
    // dead code duplicating the from < to condition)
    mEdit->loadContent(makeShiftFoldLines());
    QVERIFY(mEdit->collapse(3, 5));
    mEdit->document()->moveLine(9, 0, true);
    expectRanges({{0, 11, false}, {1, 3, false}, {4, 6, true}, {7, 9, false}},
                 "move up keeps collapsed state");
}

}

namespace QSynedit {

void TestQSyneditDisplay::test_wrap_job_early_abort()
{
    // a WrapLayoutJob must abort early (posting only an aborted control
    // signal, never stale layouts) when the document is being or has been
    // modified since the job was scheduled
    mEdit->loadContent(makeLines(20));
    auto runJob = [&](std::function<void()> beforeRun) {
        PWrapJobSharedState state = std::make_shared<WrapJobSharedState>();
        state->latestGeneration = 1;
        state->jobsInFlight = 1;
        WrapLayoutJob job(mEdit->document(), 1, 0,
                          mEdit->syntaxer()->createInstance(),
                          mEdit->font(), mEdit->mGlyphCalculator,
                          mEdit->wrapWidth(), true, state);
        QList<WrapLayoutBatch> batches;
        connect(&job, &WrapLayoutJob::layoutsReady, mEdit.get(),
                [&](const WrapLayoutBatch &batch) {
            batches.append(batch);
        });
        beforeRun();
        job.run();
        return std::make_pair(batches, state->jobsInFlight);
    };

    // aborted while the document is inside an update batch
    auto [batches1, inFlight1] = runJob([&]{
        mEdit->document()->beginUpdate();
    });
    mEdit->document()->endUpdate();
    QCOMPARE(inFlight1, 0);
    QCOMPARE(batches1.count(), 1);
    QVERIFY(batches1[0].aborted);
    QVERIFY(batches1[0].layouts.isEmpty());
    QVERIFY(!batches1[0].finished);

    // aborted when the content version changed after scheduling
    auto [batches2, inFlight2] = runJob([&]{
        mEdit->document()->putLine(0, "changed", true);
    });
    QCOMPARE(inFlight2, 0);
    QCOMPARE(batches2.count(), 1);
    QVERIFY(batches2[0].aborted);
    QVERIFY(batches2[0].layouts.isEmpty());

    // an untouched document is computed completely and finishes normally
    auto [batches3, inFlight3] = runJob([]{});
    QCOMPARE(inFlight3, 0);
    QCOMPARE(batches3.count(), 1);
    QVERIFY(!batches3[0].aborted);
    QVERIFY(batches3[0].finished);
    QCOMPARE(batches3[0].layouts.count(), 20);
}

void TestQSyneditDisplay::test_layout_with_zero_wrap_width()
{
    // regression: layouts computed before the widget has a valid width
    // (load time) must still carry real glyph positions - glyph x is
    // width-independent and the painter always needs them. A width-0 layout
    // with an empty glyph list poisoned the cache and painted blank lines
    // until the next full metrics invalidation (e.g. a syntaxer switch).
    mEdit->loadContent(QStringList() << "hello world");
    WrapLayout layout = computeWrapLayoutForLine(
                mEdit->document(), 0, mEdit->mSyntaxer.get(),
                mEdit->mGlyphCalculator, mEdit->font(), 0, false);
    QCOMPARE(layout.glyphStartXList.count(),
             mEdit->document()->getGlyphStartCharList(0).count());
    QVERIFY(layout.totalWidth > 0);
}

void TestQSyneditDisplay::test_enter_first_line_keeps_top()
{
    clearContent();
    mEdit->loadContent(QStringList{""});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    mEdit->setOptions(opts);
    QTest::keyClicks(mEdit.get(), "abc");
    QCOMPARE(mEdit->mTopLine, 0);
    QCOMPARE(mEdit->mTopPos, 0);
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->caretPos(), CharPos(0,1));
    QCOMPARE(mEdit->mTopLine, 0);
    QCOMPARE(mEdit->mTopPos, 0);
    QTest::keyClicks(mEdit.get(), "def");
    QCOMPARE(mEdit->mTopLine, 0);
    QCOMPARE(mEdit->mTopPos, 0);
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->caretPos(), CharPos(0,2));
    QCOMPARE(mEdit->mTopLine, 0);
    QCOMPARE(mEdit->mTopPos, 0);
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_horizontal_scroll_painting()
{
    // regression: the painter's token clip bounds must stay the visible
    // region (which carries mLeftPos), not the wrap segment bounds;
    // otherwise scrolling right without word wrap painted nothing
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
    // the caret stays on line 0: its row gets the active-line background,
    // so scan the second row instead
    mEdit->loadContent(QStringList() << "" << QString(500, 'x'));

    auto countTextRowColors = [&]() {
        QImage image = mEdit->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        int y = mEdit->textHeight() + mEdit->textHeight() / 2;
        QSet<QRgb> colors;
        for (int x = mEdit->gutterWidth() + 2; x < mEdit->viewport()->width() - 2; x++)
            colors.insert(image.pixel(x, y));
        return colors.count();
    };
    // glyphs visible before scrolling...
    QVERIFY(countTextRowColors() >= 2);
    // ...and after scrolling right past the window width
    mEdit->setLeftPos(200 * mEdit->charWidth());
    QVERIFY(countTextRowColors() >= 2);
}

void TestQSyneditDisplay::test_wordwrap_paints_all_segments()
{
    // regression: the token clip bounds are in the line-absolute frame (the
    // widget-space visible region shifted by the segment start); mixing the
    // frames painted segment 1 and left segments 2+ blank
    mEdit->loadContent(QStringList() << QString(500, 'x'));
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    QVERIFY(mEdit->wrapSegmentCount(0) > 2);

    QImage image = mEdit->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    int h = mEdit->textHeight();
    for (int row = 1; row <= 2; row++) {
        QSet<QRgb> colors;
        int y = row * h + h / 2;
        for (int x = mEdit->gutterWidth() + 2; x < mEdit->viewport()->width() - 2; x++)
            colors.insert(image.pixel(x, y));
        QVERIFY2(colors.count() >= 2,
                 qPrintable(QString("wrap segment at row %1 not painted").arg(row)));
    }

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_wordwrap_boundary_token_painting()
{
    // regression: with the continuation scan, a token straddling a wrap
    // boundary is painted (clipped) in BOTH segments, and tokens starting
    // in a later segment are not skipped; check glyph pixels hugging both
    // sides of every segment boundary. A line comment is ONE token for
    // the C++ syntaxer, so it straddles several boundaries.
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    mEdit->loadContent(QStringList() << ("// " + QString(500, 'x')));
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    int segments = mEdit->wrapSegmentCount(0);
    QVERIFY(segments > 2);

    QImage image = mEdit->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    int h = mEdit->textHeight();
    int left = mEdit->gutterWidth() + 2;
    int probe = 3 * mEdit->charWidth();
    const WrapLayout &layout = mEdit->wrapLayout(0);
    int rowsToCheck = std::min(segments, mEdit->linesInWindow());
    for (int seg = 0; seg < rowsToCheck; seg++) {
        int y = seg * h + h / 2;
        // the first glyph columns of the segment's row must not be blank
        // (a lost boundary token leaves them empty)
        QSet<QRgb> leftColors;
        for (int x = left; x < left + probe; x++)
            leftColors.insert(image.pixel(x, y));
        QVERIFY2(leftColors.count() >= 2,
                 qPrintable(QString("segment %1 starts blank").arg(seg)));
        // a non-last segment is full width: glyphs must hug its right edge
        // (the segment's own end, which is narrower than the viewport)
        if (seg + 1 < segments) {
            int segRight = mEdit->textOffset()
                    + (layout.segmentStartXs[seg+1] - layout.segmentStartXs[seg]);
            QSet<QRgb> rightColors;
            for (int x = segRight - probe; x < segRight - 1; x++)
                rightColors.insert(image.pixel(x, y));
            QVERIFY2(rightColors.count() >= 2,
                     qPrintable(QString("segment %1 ends blank").arg(seg)));
        }
    }

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
}

}
