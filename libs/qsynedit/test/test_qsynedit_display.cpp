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
    mEdit->setCaretPos(CharPos{0, 10});

    mEdit->processCommand(EditCommand::PageTop);
    QCOMPARE(mEdit->caretPos().line, 0);

    mEdit->processCommand(EditCommand::PageBottom);
    QCOMPARE(mEdit->caretPos().line, mEdit->linesInWindow() - 1);
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
        DisplayCoord coord = mEdit->displayXY();
        QCOMPARE(coord.x, 0);
        QCOMPARE(coord.row, mEdit->lineSegmentToRow(1, seg));
        int end = mEdit->wrapSegmentEndChar(1, seg);
        mEdit->setCaretPos(CharPos{end, 1});
        DisplayCoord endCoord = mEdit->displayXY();
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
    // EnhanceEndKey first goes after the last non-blank of the segment
    // (char 43 = 'z'), then to the segment end (char 45)
    mEdit->setCaretPos(CharPos{layout.segmentStartChars[1]+2, 1});
    mEdit->processCommand(QSynedit::EditCommand::LineStart);
    QCOMPARE(mEdit->caretCh(), layout.segmentStartChars[1]);
    mEdit->processCommand(QSynedit::EditCommand::LineEnd);
    QCOMPARE(mEdit->caretCh(), 44);
    mEdit->processCommand(QSynedit::EditCommand::LineEnd);
    QCOMPARE(mEdit->caretCh(), mEdit->wrapSegmentEndChar(1, 1));
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
    // line, while other lines are still accepted
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
    mEdit->onWrapLayoutBatch(early);
    QVERIFY(!mEdit->mWrapLayouts.contains(editedSeq));
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

void TestQSyneditDisplay::test_advance_anchor_row()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    // displayLineCount() reads the layout cache only, so force the long
    // line's layout first (background jobs don't run in tests)
    QVERIFY(mEdit->wrapSegmentCount(1) > 1);
    int totalRows = mEdit->displayLineCount();
    QVERIFY(totalRows > 3);

    // multi-step advance matches the global row mapping
    int line = 0, seg = 0;
    QCOMPARE(mEdit->advanceAnchorRow(line, seg, 2), 2);
    QCOMPARE(mEdit->lineSegmentToRow(line, seg), 2);
    QCOMPARE(mEdit->advanceAnchorRow(line, seg, -1), -1);
    QCOMPARE(mEdit->lineSegmentToRow(line, seg), 1);
    QCOMPARE(mEdit->advanceAnchorRow(line, seg, 0), 0);
    QCOMPARE(mEdit->lineSegmentToRow(line, seg), 1);

    // deltas past the boundaries stop there and report the actual movement
    QCOMPARE(mEdit->advanceAnchorRow(line, seg, totalRows+100), totalRows-2);
    QCOMPARE(mEdit->lineSegmentToRow(line, seg), totalRows-1);
    QCOMPARE(mEdit->advanceAnchorRow(line, seg, -1), -1);
    QCOMPARE(mEdit->advanceAnchorRow(line, seg, -totalRows-100), -(totalRows-2));
    QCOMPARE(line, 0);
    QCOMPARE(seg, 0);

    // out-of-range inputs are clamped before stepping
    line = -5; seg = 3;
    QCOMPARE(mEdit->advanceAnchorRow(line, seg, 1), 1);
    QCOMPARE(line, 1);
    QCOMPARE(seg, 0);
    line = 100; seg = 0;
    QVERIFY(!mEdit->advanceLineSegment(line, seg));
    QCOMPARE(line, 2);
    QCOMPARE(seg, 0);
    line = 0; seg = 99;
    QCOMPARE(mEdit->advanceAnchorRow(line, seg, 1), 1);
    QCOMPARE(line, 1);
    QCOMPARE(seg, 0);

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

void TestQSyneditDisplay::test_scroll_to_row()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "    return 0;",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    // the scrollbar maximum is only refreshed when background wrap layout
    // batches arrive (onWrapLayoutBatch -> updateVScrollbar), so wait for
    // the job to finish; otherwise it stays at the 1-segment-per-line
    // lower bound and setTopPos is clamped back down by the scrollbar
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    QVERIFY(mEdit->wrapSegmentCount(1) > 1);
    int totalRows = mEdit->displayLineCount();
    int h = mEdit->textHeight();

    mEdit->scrollToRow(2);
    QCOMPARE(mEdit->topPos(), 2 * h);
    int line, seg;
    mEdit->rowToLineSegment(2, line, seg);
    QCOMPARE(mEdit->mTopLine, line);
    QCOMPARE(mEdit->mTopSegment, seg);

    mEdit->scrollToRow(-3);
    QCOMPARE(mEdit->topPos(), 0);
    QCOMPARE(mEdit->mTopLine, 0);
    QCOMPARE(mEdit->mTopSegment, 0);

    mEdit->scrollToRow(totalRows + 100);
    QCOMPARE(mEdit->topPos(), (totalRows - 1) * h);
    QCOMPARE(mEdit->mTopLine, 3);
    QCOMPARE(mEdit->mTopSegment, 0);

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

void TestQSyneditDisplay::test_anchor_row_for_line_segment()
{
    mEdit->loadContent(QStringList{
        "int main() {",
        "    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789",
        "    return 0;",
        "}"});
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::ScrollPastEof, true);
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    QVERIFY(mEdit->wrapSegmentCount(1) > 1);
    int totalRows = mEdit->displayLineCount();
    int h = mEdit->textHeight();

    // anchor at row 1 (the first segment of the wrapped line)
    mEdit->setTopPos(h);
    QCOMPARE(mEdit->anchorRowForLineSegment(1, 0), 0);
    // every visible position maps to its global row minus the top row
    for (int row = 0; row < totalRows; row++) {
        int line, seg;
        mEdit->rowToLineSegment(row, line, seg);
        QCOMPARE(mEdit->anchorRowForLineSegment(line, seg), row - 1);
    }
    // moving the anchor moves all relative rows
    mEdit->setTopPos(0);
    for (int row = 0; row < totalRows; row++) {
        int line, seg;
        mEdit->rowToLineSegment(row, line, seg);
        QCOMPARE(mEdit->anchorRowForLineSegment(line, seg), row);
    }

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
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
