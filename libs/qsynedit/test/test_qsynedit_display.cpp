#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QInputMethodEvent>
#include <QElapsedTimer>
#include <QFile>
#include <climits>

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

class PaintPreparationEdit : public QSynEdit
{
public:
    PaintPreparationEdit()
    {
        setPaintHighlightTokensPreparer(
                    [this](int line, PaintHighlightTokenList &tokens) {
            preparedBatchLines.append(line);
            for (PaintHighlightToken &token : tokens) {
                preparedTexts.append(token.text);
                if (token.tokenType == TokenType::Identifier)
                    token.foreground = identifierColor;
                if (token.text == "...}" || token.text == "..."
                        || token.text == "...*/") {
                    foldPlaceholderSeen = true;
                    foldPlaceholderType = token.tokenType;
                    foldPlaceholderColor = token.foreground;
                }
            }
        });
    }

    QVector<int> preparedBatchLines;
    QStringList preparedTexts;
    QColor identifierColor{17, 83, 161};
    bool foldPlaceholderSeen = false;
    TokenType foldPlaceholderType = TokenType::Default;
    QColor foldPlaceholderColor;
};

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

void TestQSyneditDisplay::drainLayoutJobs(QSynEdit *editor)
{
    if (!editor)
        editor = mEdit.get();
    QTRY_VERIFY(editor->mWrapAllComputed);
    QTRY_VERIFY(editor->mWrapJobState->jobsInFlight == 0);
}

void TestQSyneditDisplay::expectRowMapping(QSynEdit *editor)
{
    if (!editor)
        editor = mEdit.get();
    int expectedRows = 0;
    for (int line = 0; line < editor->lineCount(); ++line) {
        int segments = editor->wrapSegmentCount(line);
        QCOMPARE(editor->lineToRow(line), expectedRows);
        QCOMPARE(editor->rowToLine(expectedRows), line);
        for (int segment = 0; segment < segments; ++segment) {
            int row = editor->lineSegmentToRow(line, segment);
            int mappedLine, mappedSegment;
            editor->rowToLineSegment(row, mappedLine, mappedSegment);
            QCOMPARE(mappedLine, line);
            QCOMPARE(mappedSegment, segment);
        }
        expectedRows += segments;
    }
    QCOMPARE(editor->displayLineCount(), expectedRows);
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

void TestQSyneditDisplay::test_mouse_positioning_shows_caret_immediately()
{
    mEdit->loadContent(makeLines(10));
    QPoint point(mEdit->gutterWidth() + 2 + mEdit->charWidth(), mEdit->textHeight() + 2);
    mEdit->mBlinkStatus = 0;
    mEdit->computeCaret(point);
    QCOMPARE(mEdit->mBlinkStatus, 1);
    int firstTimer = mEdit->mBlinkTimerId;
    QVERIFY(firstTimer != 0);

    mEdit->mBlinkStatus = 0;
    mEdit->computeCaret(point);
    QCOMPARE(mEdit->mBlinkStatus, 1);
    QVERIFY(mEdit->mBlinkTimerId != 0);
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

void TestQSyneditDisplay::test_mouse_double_click_at_token_boundary_data()
{
    QTest::addColumn<QString>("lineText");
    QTest::addColumn<int>("clickCh");
    QTest::addColumn<QString>("selectedText");
    QTest::addColumn<int>("selectionStart");
    QTest::addColumn<int>("selectionEnd");

    QTest::addRow("identifier before symbol") << "value+next" << 5 << "value" << 0 << 5;
    QTest::addRow("number before symbol") << "123+next" << 3 << "123" << 0 << 3;
    QTest::addRow("keyword before symbol") << "return;" << 6 << "return" << 0 << 6;
    QTest::addRow("macro before symbol") << "#define MAX+1" << 11 << "MAX" << 8 << 11;
    QTest::addRow("number in macro") << "#define X 123+next" << 13 << "123" << 10 << 13;
    QTest::addRow("macro parameter") << "#define SUM(x,y)" << 12 << "x" << 12 << 13;
    QTest::addRow("string before symbol") << "\"hello+world\"" << 6 << "hello" << 1 << 6;
    QTest::addRow("escape in string") << "\"a\\b\"" << 2 << "\\b" << 2 << 4;
    QTest::addRow("escaped backslash in string") << "\"a\\\\b\"" << 2 << "\\\\" << 2 << 4;
    QTest::addRow("second char of escaped backslash") << "\"a\\\\b\"" << 3 << "\\\\" << 2 << 4;
    QTest::addRow("char of letter escape") << "\"\\a\"" << 2 << "\\a" << 1 << 3;
    QTest::addRow("invalid escape uses normal word") << "\"\\q\"" << 2 << "q" << 2 << 3;
    QTest::addRow("char of quote escape") << "\"\\\"\"" << 2 << "\\\"" << 1 << 3;
    QTest::addRow("char of escape in define") << "#define X \"\\a\"" << 12 << "\\a" << 11 << 13;
    QTest::addRow("escape before closing quote") << "\"a\\\\\"" << 4 << "\\\\" << 2 << 4;
    QTest::addRow("letter escape before closing quote") << "\"\\a\"" << 3 << "\\a" << 1 << 3;
    QTest::addRow("quote escape before closing quote") << "\"\\\"\"" << 3 << "\\\"" << 1 << 3;
    QTest::addRow("escape in define") << "#define X \"a\\b\"" << 12 << "\\b" << 12 << 14;
    QTest::addRow("escape before quote in define") << "#define X \"a\\\\\"" << 14 << "\\\\" << 12 << 14;
    QTest::addRow("comment before symbol") << "//value+next" << 7 << "value" << 2 << 7;
    QTest::addRow("identifier after symbol") << "value+next" << 6 << "next" << 6 << 10;
    QTest::addRow("space before symbol") << "value +next" << 6 << "+" << 6 << 7;
    QTest::addRow("end of line") << "value" << 5 << "value" << 0 << 5;
    QTest::addRow("end of line after quote") << "\"abc\"" << 5 << "\"" << 4 << 5;
    QTest::addRow("end of line after spaces") << "value  " << 7 << "  " << 5 << 7;
    QTest::addRow("end of line after escape") << "\"\\a\"" << 4 << "\"" << 3 << 4;
    QTest::addRow("end of line in define") << "#define EOL \"TEST\"" << 18 << "\"" << 17 << 18;
}

void TestQSyneditDisplay::test_mouse_double_click_at_token_boundary()
{
    QFETCH(QString, lineText);
    QFETCH(int, clickCh);
    QFETCH(QString, selectedText);
    QFETCH(int, selectionStart);
    QFETCH(int, selectionEnd);
    mEdit->loadContent(QStringList{lineText});
    QPoint pt = mEdit->displayCoordToPixels(
                mEdit->bufferToDisplayPos(CharPos{clickCh, 0})) + QPoint(0, 2);
    QTest::mousePress(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, pt);
    QTest::mouseRelease(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, pt);
    QTest::mouseEvent(QTest::MouseDClick, mEdit->viewport(), Qt::LeftButton,
                      Qt::NoModifier, pt);
    QTest::mouseRelease(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier, pt);

    QCOMPARE(mEdit->selText(), selectedText);
    QCOMPARE(mEdit->selBegin(), CharPos(selectionStart, 0));
    QCOMPARE(mEdit->selEnd(), CharPos(selectionEnd, 0));
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
    QCOMPARE(mEdit->caretPos(), CharPos(0, 2));
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

void TestQSyneditDisplay::test_fold_placeholder_painting()
{
    PaintPreparationEdit edit;
    edit.setSyntaxer(std::make_shared<CppSyntaxer>());
    edit.resize(800, 600);
    edit.loadContent(makeFoldableLines());
    QColor symbolColor(17, 83, 161);
    QColor backgroundColor(250, 250, 250);
    edit.syntaxer()->symbolAttribute()->setForeground(symbolColor);
    edit.setBackgroundColor(backgroundColor);
    QVERIFY(edit.collapse(0, 6));
    edit.setCaretPos(CharPos{0, 7});
    QImage image = edit.grab().toImage();

    QVERIFY(edit.foldPlaceholderSeen);
    QCOMPARE(edit.foldPlaceholderType, TokenType::Symbol);
    QCOMPARE(edit.foldPlaceholderColor, symbolColor);
    QCOMPARE(image.pixelColor(edit.width() - 20, edit.textHeight() - 1),
             backgroundColor);
}

void TestQSyneditDisplay::test_comment_fold_placeholder_painting()
{
    PaintPreparationEdit edit;
    edit.setSyntaxer(std::make_shared<CppSyntaxer>());
    edit.resize(800, 600);
    edit.loadContent({"/* comment", " * text", " */", "int value;"});
    QColor commentColor(17, 83, 161);
    edit.syntaxer()->commentAttribute()->setForeground(commentColor);
    QVERIFY(edit.collapse(0, 2));
    edit.setCaretPos(CharPos{0, 3});
    edit.grab();

    QVERIFY(edit.foldPlaceholderSeen);
    QVERIFY(edit.preparedTexts.contains("...*/"));
    QCOMPARE(edit.foldPlaceholderType, TokenType::Symbol);
    QCOMPARE(edit.foldPlaceholderColor, commentColor);
}

void TestQSyneditDisplay::test_comment_fold_ranges()
{
    mEdit->loadContent({
        "/* first",
        " * { ignored }",
        " */",
        "/** doc",
        " * text",
        " */",
        "/* single */",
        "void f() {",
        "    /* inner",
        "     * }",
        "     */",
        "}"
    });
    PCodeBlock first = mEdit->foldStartAtLine(0);
    QVERIFY(first);
    QCOMPARE(first->kind, CodeBlockKind::Comment);
    QCOMPARE(first->fromLine, 0);
    QCOMPARE(first->toLine, 2);
    QCOMPARE(first->fromCh, 0);
    QCOMPARE(first->toCh, 1);

    PCodeBlock docstring = mEdit->foldStartAtLine(3);
    QVERIFY(docstring);
    QCOMPARE(docstring->kind, CodeBlockKind::Comment);
    QCOMPARE(docstring->toLine, 5);
    QVERIFY(!mEdit->foldStartAtLine(6));

    PCodeBlock function = mEdit->foldStartAtLine(7);
    QVERIFY(function);
    QCOMPARE(function->kind, CodeBlockKind::Code);
    QCOMPARE(function->toLine, 11);
    QCOMPARE(mEdit->subBlockCounts(7, 11), 1);
    PCodeBlock inner = mEdit->foldStartAtLine(8);
    QVERIFY(inner);
    QCOMPARE(inner->kind, CodeBlockKind::Comment);
    QCOMPARE(inner->toLine, 10);
    QVERIFY(inner->parent.lock() == function);

    mEdit->loadContent({"/* open", "text"});
    PCodeBlock unfinished = mEdit->foldStartAtLine(0);
    QVERIFY(unfinished);
    QCOMPARE(unfinished->kind, CodeBlockKind::Comment);
    QVERIFY(unfinished->unfinished);
    QCOMPARE(unfinished->toLine, 1);
    QVERIFY(mEdit->collapse(0, 1));
    QCOMPARE(mEdit->getDisplayStringAtLine(0), QString("/* open...*/"));

    mEdit->properSetLine(1, "text */", true);
    unfinished = mEdit->foldStartAtLine(0);
    QVERIFY(unfinished);
    QVERIFY(!unfinished->unfinished);
    QCOMPARE(unfinished->toLine, 1);
    QCOMPARE(unfinished->toCh, 5);
}

void TestQSyneditDisplay::test_comment_fold_close_open_same_line()
{
    mEdit->loadContent({
        "/* first",
        "*/ int value; /* second",
        "continued",
        "*/"
    });
    PCodeBlock first = mEdit->foldStartAtLine(0);
    PCodeBlock second = mEdit->foldStartAtLine(1);
    QVERIFY(first);
    QVERIFY(second);
    QCOMPARE(first->kind, CodeBlockKind::Comment);
    QCOMPARE(second->kind, CodeBlockKind::Comment);
    QCOMPARE(first->toLine, 1);
    QCOMPARE(first->toCh, 0);
    QCOMPARE(second->fromCh, mEdit->lineText(1).indexOf("/*"));
    QCOMPARE(second->toLine, 3);
    QCOMPARE(second->toCh, 0);
    QVERIFY(first->parent.lock() == nullptr);
    QVERIFY(second->parent.lock() == nullptr);

    QVERIFY(mEdit->collapse(0, 1));
    QVERIFY(mEdit->collapse(1, 3));
    QVERIFY(mEdit->isCollapsed(0, 1));
    QVERIFY(mEdit->isCollapsed(1, 3));
}

void TestQSyneditDisplay::test_comment_fold_end_line_suffix_visibility()
{
    const QStringList suffixes{"", " ", "\t", ";", " int value;"};
    for (const QString &suffix : suffixes) {
        mEdit->loadContent({"/* comment", " * text", " */" + suffix});
        QVERIFY(mEdit->collapse(0, 2));
        PCodeBlock block = mEdit->foldStartAtLine(0);
        QVERIFY(block);
        QCOMPARE(block->kind, CodeBlockKind::Comment);
        QCOMPARE(block->toCh, 1);
        QCOMPARE(block->endDelimiterLength(), 2);
        bool endLineVisible = !suffix.isEmpty();
        QCOMPARE(mEdit->foldEndLineVisible(block), endLineVisible);
        QVERIFY(mEdit->mRowIndex.isHidden(1));
        QCOMPARE(mEdit->mRowIndex.isHidden(2), !endLineVisible);
        QCOMPARE(mEdit->getDisplayStringAtLine(0),
                 QString("/* comment") + (endLineVisible ? "..." : "...*/"));
        if (endLineVisible)
            QCOMPARE(mEdit->lineToRow(2), mEdit->lineToRow(0) + 1);
    }

    mEdit->loadContent({"/* comment", " */ tail"});
    QVERIFY(mEdit->collapse(0, 1));
    QVERIFY(!mEdit->foldHasHiddenLines(mEdit->foldStartAtLine(0)));
    QCOMPARE(mEdit->getDisplayStringAtLine(0), QString("/* comment"));
}

void TestQSyneditDisplay::test_comment_fold_navigation()
{
    mEdit->loadContent({"/* comment", " * text", " */", "int value;"});
    QVERIFY(mEdit->collapse(0, 2));
    int lineLength = mEdit->lineText(0).length();
    CharPos virtualEnd{lineLength + QString("...*/").length(), 0};
    mEdit->setCaretPos(CharPos{lineLength, 0});
    QTest::keyClick(mEdit.get(), Qt::Key_Right);
    QCOMPARE(mEdit->caretPos(), virtualEnd);
    QVERIFY(mEdit->validCaretPos(mEdit->caretPos()));
    QVERIFY(!mEdit->validInDoc(mEdit->caretPos()));
    QCOMPARE(mEdit->displayToBufferPos(mEdit->bufferToDisplayPos(virtualEnd)),
             CharPos(lineLength, 0));
    QCOMPARE(mEdit->documentPositionForFoldCaret(virtualEnd), CharPos(3, 2));

    mEdit->setCaretPos(CharPos{0, 3});
    QTest::keyClick(mEdit.get(), Qt::Key_Left);
    QCOMPARE(mEdit->caretPos(), virtualEnd);
    QTest::keyClick(mEdit.get(), Qt::Key_Right);
    QCOMPARE(mEdit->caretPos(), CharPos(0, 3));

    mEdit->setCaretPos(CharPos{lineLength, 0});
    QTest::keyClick(mEdit.get(), Qt::Key_Right);
    QTest::keyClick(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->lineText(2), QString(" *"));
    QCOMPARE(mEdit->caretPos(), CharPos(2, 2));
    QVERIFY(!mEdit->isCollapsed(0, 2));
    mEdit->undo();
    QCOMPARE(mEdit->lineText(2), QString(" */"));
    QVERIFY(!mEdit->isCollapsed(0, 2));
    mEdit->redo();
    QCOMPARE(mEdit->lineText(2), QString(" *"));
    QVERIFY(!mEdit->isCollapsed(0, 2));
}

void TestQSyneditDisplay::test_comment_fold_edit_rules()
{
    mEdit->loadContent({
        "int before = 0; /* comment", " * text", " */ tail", "int after;"
    });
    QVERIFY(mEdit->collapse(0, 2));
    mEdit->setCaretPos(CharPos{0, 0});
    QTest::keyClick(mEdit.get(), Qt::Key_X);
    QCOMPARE(mEdit->lineText(0), QString("xint before = 0; /* comment"));
    QVERIFY(mEdit->isCollapsed(0, 2));
    QTest::keyClick(mEdit.get(), Qt::Key_Backspace);
    QVERIFY(mEdit->isCollapsed(0, 2));

    mEdit->setCaretPos(CharPos{mEdit->lineText(2).length(), 2});
    QTest::keyClick(mEdit.get(), Qt::Key_X);
    QCOMPARE(mEdit->lineText(2), QString(" */ tailx"));
    QVERIFY(mEdit->isCollapsed(0, 2));
    QTest::keyClick(mEdit.get(), Qt::Key_Backspace);
    QVERIFY(mEdit->isCollapsed(0, 2));

    mEdit->setCaretAndSelection(CharPos{3, 2}, CharPos{3, 2},
                                CharPos{mEdit->lineText(2).length(), 2});
    QTest::keyClick(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->lineText(2), QString(" */"));
    QVERIFY(!mEdit->isCollapsed(0, 2));
    mEdit->undo();
    QCOMPARE(mEdit->lineText(2), QString(" */ tail"));
    QVERIFY(!mEdit->isCollapsed(0, 2));
    mEdit->redo();
    QCOMPARE(mEdit->lineText(2), QString(" */"));
    QVERIFY(!mEdit->isCollapsed(0, 2));

    mEdit->loadContent({"/* comment", " * text", " */", "int after;"});
    QVERIFY(mEdit->collapse(0, 2));
    mEdit->setCaretPos(CharPos{mEdit->lineText(1).length(), 1});
    QTest::keyClick(mEdit.get(), Qt::Key_X);
    QCOMPARE(mEdit->lineText(1), QString(" * textx"));
    QVERIFY(!mEdit->isCollapsed(0, 2));
    mEdit->undo();
    QCOMPARE(mEdit->lineText(1), QString(" * text"));
    QVERIFY(!mEdit->isCollapsed(0, 2));
    mEdit->redo();
    QCOMPARE(mEdit->lineText(1), QString(" * textx"));
    QVERIFY(!mEdit->isCollapsed(0, 2));

    mEdit->loadContent({"/* comment", " * text", " */", "int after;"});
    QVERIFY(mEdit->collapse(0, 2));
    mEdit->setCaretPos(CharPos{mEdit->lineText(0).length(), 0});
    QTest::keyClick(mEdit.get(), Qt::Key_Right);
    QTest::keyClick(mEdit.get(), Qt::Key_X);
    QCOMPARE(mEdit->lineText(2), QString(" */x"));
    QVERIFY(!mEdit->isCollapsed(0, 2));
}

void TestQSyneditDisplay::test_comment_fold_incremental_rescan()
{
    mEdit->loadContent({"void f() {", "    work();", "}"});
    QVERIFY(mEdit->collapse(0, 2));
    mEdit->beginEditing();
    mEdit->properSetLine(0, "/* comment", true);
    mEdit->properSetLine(2, " */", true);
    mEdit->endEditing();

    PCodeBlock block = mEdit->foldStartAtLine(0);
    QVERIFY(block);
    QCOMPARE(block->kind, CodeBlockKind::Comment);
    QCOMPARE(block->toLine, 2);
    QCOMPARE(block->toCh, 1);
    QVERIFY(!mEdit->isCollapsed(0, 2));
    auto verifyFullRescan = [&] {
        QList<std::tuple<int, int, bool>> partial = mEdit->foldRangesForTest();
        mEdit->fullRescanCodeBlocksForTest();
        QCOMPARE(mEdit->foldRangesForTest(), partial);
    };
    verifyFullRescan();

    mEdit->properSetLine(2, " */ trailing", true);
    block = mEdit->foldStartAtLine(0);
    QVERIFY(block);
    QCOMPARE(block->kind, CodeBlockKind::Comment);
    QVERIFY(mEdit->foldEndLineVisible(block));
    verifyFullRescan();

    mEdit->properSetLine(0, "int value;", true);
    QVERIFY(!mEdit->foldStartAtLine(0));
    verifyFullRescan();
}

void TestQSyneditDisplay::test_comment_fold_shared_document_views()
{
    mEdit->loadContent({"/* comment", " * text", " */", "int value;"});
    QSynEdit other;
    other.setSyntaxer(std::make_shared<CppSyntaxer>());
    other.setDocument(mEdit->document());
    QVERIFY(mEdit->collapse(0, 2));
    QVERIFY(!other.isCollapsed(0, 2));
    QVERIFY(mEdit->foldHidesLine(1));
    QVERIFY(!other.foldHidesLine(1));

    mEdit->properSetLine(1, " * changed", true);
    QVERIFY(!mEdit->isCollapsed(0, 2));
    QVERIFY(!other.isCollapsed(0, 2));
    QCOMPARE(other.lineText(1), QString(" * changed"));
}

void TestQSyneditDisplay::test_fold_placeholder_navigation()
{
    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(0, 6));
    int lineLength = mEdit->lineText(0).length();
    int displayLength = mEdit->getDisplayStringAtLine(0).length();
    mEdit->setCaretPos(CharPos{lineLength, 0});
    QTest::keyClick(mEdit.get(), Qt::Key_Right);
    QCOMPARE(mEdit->caretPos(), CharPos(lineLength + QString("...}").length(), 0));
    QCOMPARE(mEdit->caretCh(), displayLength);
    QVERIFY(mEdit->validCaretPos(mEdit->caretPos()));
    QVERIFY(!mEdit->validInDoc(mEdit->caretPos()));
    QVERIFY(mEdit->displayX() > 0);

    QTest::keyClick(mEdit.get(), Qt::Key_Left);
    QCOMPARE(mEdit->caretPos(), CharPos(lineLength, 0));
    mEdit->setCaretPos(CharPos{0, 7});
    QTest::keyClick(mEdit.get(), Qt::Key_Left);
    QCOMPARE(mEdit->caretPos(), CharPos(displayLength, 0));
    QTest::keyClick(mEdit.get(), Qt::Key_Right);
    QCOMPARE(mEdit->caretPos(), CharPos(0, 7));
}

void TestQSyneditDisplay::test_fold_end_line_suffix_visibility()
{
    const QStringList suffixes{"", " ", "\t", ";", " // comment"};
    for (const QString &suffix : suffixes) {
        mEdit->loadContent(QStringList{"void f() {", "    work();", "}" + suffix});
        QVERIFY(mEdit->collapse(0, 2));
        PCodeBlock block = mEdit->foldStartAtLine(0);
        QVERIFY(block);
        QCOMPARE(block->fromCh, mEdit->lineText(0).indexOf("{"));
        QCOMPARE(block->toCh, 0);
        bool endLineVisible = !suffix.isEmpty();
        QCOMPARE(mEdit->foldEndLineVisible(block), endLineVisible);
        QCOMPARE(mEdit->mRowIndex.isHidden(1), true);
        QCOMPARE(mEdit->mRowIndex.isHidden(2), !endLineVisible);
        QCOMPARE(mEdit->getDisplayStringAtLine(0),
                 QString("void f() {") + (endLineVisible ? "..." : "...}"));
        if (endLineVisible) {
            int lineLength = mEdit->lineText(0).length();
            mEdit->setCaretPos(CharPos{lineLength, 0});
            QTest::keyClick(mEdit.get(), Qt::Key_Right);
            QCOMPARE(mEdit->caretPos(), CharPos(0, 2));
            QTest::keyClick(mEdit.get(), Qt::Key_Left);
            QCOMPARE(mEdit->caretPos(), CharPos(lineLength, 0));
        }
    }

    mEdit->loadContent(QStringList{"void f() {", "} trailing;"});
    QVERIFY(mEdit->collapse(0, 1));
    QVERIFY(!mEdit->foldHasHiddenLines(mEdit->foldStartAtLine(0)));
    QCOMPARE(mEdit->getDisplayStringAtLine(0), QString("void f() {"));
}

void TestQSyneditDisplay::test_fold_else_same_line()
{
    mEdit->loadContent(QStringList{
        "void f() {",
        "    if (ok) {",
        "        work();",
        "    } else {",
        "        other();",
        "    }",
        "}"
    });
    QVERIFY(mEdit->collapse(1, 3));
    PCodeBlock block = mEdit->foldStartAtLine(1);
    QVERIFY(block);
    QCOMPARE(block->fromCh, mEdit->lineText(1).indexOf("{"));
    QCOMPARE(block->toCh, mEdit->lineText(3).indexOf("}"));
    QVERIFY(mEdit->foldEndLineVisible(block));
    QVERIFY(mEdit->mRowIndex.isHidden(2));
    QVERIFY(!mEdit->mRowIndex.isHidden(3));
    QCOMPARE(mEdit->getDisplayStringAtLine(1), QString("    if (ok) {..."));
    QCOMPARE(mEdit->lineToRow(3), mEdit->lineToRow(1) + 1);
}

void TestQSyneditDisplay::test_fold_edit_inside_expands_and_undo()
{
    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(0, 6));
    int lineLength = mEdit->lineText(0).length();
    mEdit->setCaretPos(CharPos{lineLength, 0});
    QTest::keyClick(mEdit.get(), Qt::Key_X);
    QVERIFY(!mEdit->isCollapsed(0, 6));
    QCOMPARE(mEdit->lineText(0), QString("int main() {x"));
    QCOMPARE(mEdit->caretPos(), CharPos(lineLength + 1, 0));
    mEdit->undo();
    QCOMPARE(mEdit->lineText(0), QString("int main() {"));
    QVERIFY(!mEdit->isCollapsed(0, 6));
}

void TestQSyneditDisplay::test_fold_virtual_close_backspace()
{
    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(0, 6));
    mEdit->setCaretPos(CharPos{mEdit->lineText(0).length(), 0});
    QTest::keyClick(mEdit.get(), Qt::Key_Right);
    QTest::keyClick(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->lineText(6), QString());
    QCOMPARE(mEdit->caretPos(), CharPos(0, 6));
    QVERIFY(!mEdit->isCollapsed(0, 6));
    mEdit->undo();
    QCOMPARE(mEdit->lineText(6), QString("}"));
    QVERIFY(!mEdit->isCollapsed(0, 6));
}

void TestQSyneditDisplay::test_fold_edit_outside_and_suffix_transitions()
{
    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(0, 6));
    int openCh = mEdit->lineText(0).indexOf("{");
    mEdit->setCaretPos(CharPos{openCh, 0});
    QTest::keyClick(mEdit.get(), Qt::Key_X);
    QCOMPARE(mEdit->lineText(0), QString("int main() x{"));
    QVERIFY(mEdit->isCollapsed(0, 6));
    QTest::keyClick(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->lineText(0), QString("int main() {"));
    QVERIFY(mEdit->isCollapsed(0, 6));

    mEdit->loadContent(QStringList{
        "void f() {", "    work();", "} trailing;", "void g() {}"
    });
    QVERIFY(mEdit->collapse(0, 2));
    mEdit->setCaretPos(CharPos{mEdit->lineText(2).length(), 2});
    QTest::keyClick(mEdit.get(), Qt::Key_X);
    QCOMPARE(mEdit->lineText(2), QString("} trailing;x"));
    QVERIFY(mEdit->isCollapsed(0, 2));
    QTest::keyClick(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->lineText(2), QString("} trailing;"));
    QVERIFY(mEdit->isCollapsed(0, 2));
    mEdit->setCaretPos(CharPos{1, 2});
    QTest::keyClick(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->lineText(2), QString("}trailing;"));
    QVERIFY(mEdit->isCollapsed(0, 2));

    mEdit->setCaretAndSelection(CharPos{1, 2}, CharPos{1, 2},
                                CharPos{mEdit->lineText(2).length(), 2});
    QTest::keyClick(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->lineText(2), QString("}"));
    QVERIFY(!mEdit->isCollapsed(0, 2));

    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(0, 6));
    mEdit->setCaretPos(CharPos{mEdit->lineText(0).length(), 0});
    QTest::keyClick(mEdit.get(), Qt::Key_Right);
    QTest::keyClick(mEdit.get(), Qt::Key_X);
    QCOMPARE(mEdit->lineText(6), QString("}x"));
    QVERIFY(!mEdit->isCollapsed(0, 6));

    mEdit->loadContent(makeFoldableLines());
    QVERIFY(mEdit->collapse(0, 6));
    mEdit->setCaretPos(CharPos{mEdit->lineText(0).length(), 0});
    QTest::keyClick(mEdit.get(), Qt::Key_Right);
    QTest::keyClick(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->caretPos(), CharPos(0, 7));
    QVERIFY(mEdit->isCollapsed(0, 6));
}

void TestQSyneditDisplay::test_fold_edit_shared_document_views()
{
    mEdit->loadContent(QStringList{
        "void first() {", "    work();", "}",
        "void second() {", "    other();", "}"
    });
    QSynEdit other;
    other.setSyntaxer(std::make_shared<CppSyntaxer>());
    other.setDocument(mEdit->document());
    QVERIFY(mEdit->collapse(0, 2));
    QVERIFY(other.collapse(3, 5));
    mEdit->setCaretPos(CharPos{mEdit->lineText(0).length(), 0});
    QTest::keyClick(mEdit.get(), Qt::Key_X);
    QVERIFY(!mEdit->isCollapsed(0, 2));
    QVERIFY(other.isCollapsed(3, 5));
    QCOMPARE(other.lineText(0), mEdit->lineText(0));
}

void TestQSyneditDisplay::test_fold_merge_before_open_does_not_expand()
{
    mEdit->loadContent(QStringList{
        "int before = 0;", "void f() {", "    work();", "}"
    });
    QVERIFY(mEdit->collapse(1, 3));
    mEdit->setCaretPos(CharPos{mEdit->lineText(0).length(), 0});
    QTest::keyClick(mEdit.get(), Qt::Key_Delete);
    QCOMPARE(mEdit->lineText(0), QString("int before = 0;void f() {"));
    QVERIFY(mEdit->isCollapsed(0, 2));
}

void TestQSyneditDisplay::test_fold_whole_block_delete_exception()
{
    mEdit->loadContent(QStringList{
        "void first() {", "    work();", "}",
        "void second() {", "    other();", "}"
    });
    QVERIFY(mEdit->collapse(0, 2));
    QVERIFY(mEdit->collapse(3, 5));
    mEdit->setCaretAndSelection(CharPos{0, 0}, CharPos{0, 0}, CharPos{0, 3});
    QTest::keyClick(mEdit.get(), Qt::Key_Backspace);
    QCOMPARE(mEdit->content(), QStringList({
        "void second() {", "    other();", "}"
    }));
    QVERIFY(mEdit->isCollapsed(0, 2));
}

void TestQSyneditDisplay::test_fold_bulk_delete_matches_full_rescan()
{
    QStringList lines;
    for (int block = 0; block < 40; ++block) {
        lines.append(QStringLiteral("void f%1() {").arg(block));
        lines.append("    work();");
        lines.append("}");
    }
    mEdit->loadContent(lines);
    for (int block = 0; block < 40; block += 3)
        QVERIFY(mEdit->collapse(block * 3, block * 3 + 2));

    mEdit->properDeleteLines(2, 60, true);
    QList<std::tuple<int, int, bool>> partial = mEdit->foldRangesForTest();
    QVERIFY(mEdit->isCollapsed(30, 32));
    mEdit->fullRescanCodeBlocksForTest();
    QCOMPARE(mEdit->foldRangesForTest(), partial);
}

void TestQSyneditDisplay::test_gutter_click_selects_line()
{
    mEdit->loadContent(makeLines(5));
    int h = mEdit->textHeight();
    int x = mEdit->gutter().leftOffset() + 1;
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(x, h + 2));
    QCOMPARE(mEdit->selBegin(), CharPos(0, 1));
    QCOMPARE(mEdit->selEnd(), CharPos(0, 2));
    QCOMPARE(mEdit->caretPos(), CharPos(0, 0));
    QCOMPARE(mEdit->activeSelectionMode(), SelectionMode::Normal);
}

void TestQSyneditDisplay::test_gutter_mark_click_emits_signal()
{
    mEdit->loadContent(makeLines(5));
    int clickCount = 0;
    Qt::MouseButton clickedButton = Qt::NoButton;
    int clickedLine = -1;
    connect(mEdit.get(), &QSynEdit::gutterClicked, mEdit.get(),
            [&](Qt::MouseButton button, int, int, int line) {
        ++clickCount;
        clickedButton = button;
        clickedLine = line;
    });
    int h = mEdit->textHeight();
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(0, h + 2));
    QCOMPARE(clickCount, 1);
    QCOMPARE(clickedButton, Qt::LeftButton);
    QCOMPARE(clickedLine, 1);
    QCOMPARE(mEdit->selBegin(), CharPos(0, 0));
    QCOMPARE(mEdit->selEnd(), CharPos(0, 0));
}

void TestQSyneditDisplay::test_gutter_shift_click_selects_line_range()
{
    mEdit->loadContent(makeLines(5));
    int h = mEdit->textHeight();
    int x = mEdit->gutter().leftOffset() + 1;
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(x, h + 2));
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::ShiftModifier,
                      QPoint(x, 3 * h + 2));
    QCOMPARE(mEdit->selBegin(), CharPos(0, 1));
    QCOMPARE(mEdit->selEnd(), CharPos(0, 4));
    QCOMPARE(mEdit->caretPos(), CharPos(0, 0));

    clearContent();
    mEdit->loadContent(makeLines(5));
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::NoModifier,
                      QPoint(x, 3 * h + 2));
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::ShiftModifier,
                      QPoint(x, h + 2));
    QCOMPARE(mEdit->selBegin(), CharPos(0, 1));
    QCOMPARE(mEdit->selEnd(), CharPos(0, 4));
    QCOMPARE(mEdit->caretPos(), CharPos(0, 0));
}

void TestQSyneditDisplay::test_gutter_shift_click_requires_full_line_selection()
{
    mEdit->loadContent(makeLines(5));
    mEdit->setCaretAndSelection(CharPos{3, 1}, CharPos{1, 1}, CharPos{3, 1});
    int h = mEdit->textHeight();
    int x = mEdit->gutter().leftOffset() + 1;
    QTest::mouseClick(mEdit->viewport(), Qt::LeftButton, Qt::ShiftModifier,
                      QPoint(x, 3 * h + 2));
    QCOMPARE(mEdit->selBegin(), CharPos(0, 3));
    QCOMPARE(mEdit->selEnd(), CharPos(0, 4));
    QCOMPARE(mEdit->caretPos(), CharPos(3, 1));
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

void TestQSyneditDisplay::test_drag_start_requires_distance_and_time()
{
    int oldTime = QApplication::startDragTime();
    int oldDistance = QApplication::startDragDistance();
    auto restore = finally([oldTime, oldDistance] {
        QApplication::setStartDragTime(oldTime);
        QApplication::setStartDragDistance(oldDistance);
    });
    QApplication::setStartDragTime(100);
    QApplication::setStartDragDistance(10);
    QPoint origin(100, 100);
    mEdit->mMouseDownPos = origin;
    mEdit->mMouseDownTime.start();

    QVERIFY(!mEdit->shouldStartDrag(origin + QPoint(11, 0)));
    QTest::qWait(120);
    QVERIFY(mEdit->shouldStartDrag(origin + QPoint(11, 0)));
    QVERIFY(!mEdit->shouldStartDrag(origin + QPoint(9, 0)));
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

void TestQSyneditDisplay::test_dnd_undo_restores_content_and_folds()
{
    QFile sourceFile("resources/editormanager.cpp");
    QVERIFY(sourceFile.open(QIODevice::ReadOnly));
    mEdit->setSelText(QString::fromUtf8(sourceFile.readAll()));
    QStringList lines = mEdit->content();
    auto originalFolds = mEdit->foldRangesForTest();
    auto foldTree = [](const QSynEdit *edit) {
        QStringList result;
        for (const PCodeBlock &block : edit->mCodeBlocks) {
            PCodeBlock parent = block->parent.lock();
            QStringList children;
            for (const PCodeBlock &child : block->subBlocks)
                children << QString("%1-%2").arg(child->fromLine).arg(child->toLine);
            result << QString("%1-%2@%3[%4]")
                    .arg(block->fromLine).arg(block->toLine)
                    .arg(parent ? parent->fromLine : -1)
                    .arg(children.join(','));
        }
        return result;
    };
    QStringList originalTree = foldTree(mEdit.get());
    auto selectTail = [&] {
        int endLine = 1148;
        mEdit->setCaretAndSelection(CharPos{mEdit->lineText(endLine).length(), endLine},
                                    CharPos{4, 1146},
                                    CharPos{mEdit->lineText(endLine).length(), endLine});
    };
    selectTail();
    QString selected = mEdit->selText();
    QMimeData mime;
    mime.setText(selected);
    auto dragTo = [&](int line, int ch) {
        QDragEnterEvent enterEvent(QPoint(100, 100), Qt::MoveAction, &mime,
                                   Qt::LeftButton, Qt::NoModifier);
        mEdit->dragEnterEvent(&enterEvent);
        mEdit->setTopLineSegment(line, 0);
        QPoint pt = mEdit->displayCoordToPixels(
                    mEdit->bufferToDisplayPos(CharPos{ch, line})) + QPoint(0, 2);
        QDropEvent dropEvent(pt, Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
        mEdit->dropEvent(&dropEvent);
    };

    dragTo(1136, 8);
    QCOMPARE(mEdit->lineCount(), lines.count());

    mEdit->loadContent(lines);
    selectTail();
    CharPos caretBeforeDrag = mEdit->caretPos();
    dragTo(1138, 4);
    mEdit->undo();
    QCOMPARE(mEdit->caretPos(), caretBeforeDrag);
    QCOMPARE(mEdit->content(), lines);
    QCOMPARE(mEdit->foldRangesForTest(), originalFolds);
    QCOMPARE(foldTree(mEdit.get()), originalTree);

    selectTail();
    dragTo(1105, 0);
    QCOMPARE(mEdit->lineCount(), lines.count());
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
    mEdit->loadContent(makeLines(200));
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
    mEdit->loadContent(makeLines(200));
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
        QCOMPARE(coord.y, mEdit->lineSegmentToRow(1, seg) * mEdit->textHeight());
        int end = mEdit->wrapSegmentEndChar(1, seg);
        mEdit->setCaretPos(CharPos{end, 1});
        VisualPos endCoord = mEdit->displayXY();
        // a segment end that is also the next segment's start belongs to
        // the next segment's row; only the line end stays on the same row
        if (seg+1 < layout.segmentStartChars.count())
            QCOMPARE(endCoord.y, mEdit->lineSegmentToRow(1, seg+1) * mEdit->textHeight());
        else
            QCOMPARE(endCoord.y, coord.y);
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
    mEdit->loadContent(makeLines(200));
    int h = mEdit->textHeight();
    mEdit->setTopPos(30 * h);
    QCOMPARE(mEdit->mTopLine, 30);

    // insertion above the anchor keeps the current top line, making the
    // inserted content immediately visible
    mEdit->beginEditing();
    mEdit->document()->insertLines(5, 3, false);
    mEdit->endEditing();
    QCOMPARE(mEdit->mTopLine, 30);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 30 * h);
    // the scrollbar tracks the derived pixel offset
    QCOMPARE(mEdit->verticalScrollBar()->value(), 30 * h);

    // delete above the anchor: the anchor moves back up
    mEdit->beginEditing();
    mEdit->document()->deleteLines(5, 3, false);
    mEdit->endEditing();
    QCOMPARE(mEdit->mTopLine, 27);
    QCOMPARE(mEdit->topPos(), 27 * h);

    // insertion at the anchor keeps the current top line
    mEdit->beginEditing();
    mEdit->document()->insertLines(30, 1, false);
    mEdit->endEditing();
    QCOMPARE(mEdit->mTopLine, 27);
    QCOMPARE(mEdit->topPos(), 27 * h);

    // delete a band after the anchor: keep the anchor
    mEdit->beginEditing();
    mEdit->document()->deleteLines(28, 5, false);
    mEdit->endEditing();
    QCOMPARE(mEdit->mTopLine, 27);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 27 * h);

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
        QString("    // 一二三四五六七八九十 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789")
                + QString(300, 'x'),
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
    lines << makeLines(200);
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

void TestQSyneditDisplay::test_batch_top_restore()
{
    mEdit->loadContent(makeLines(200));
    int h = mEdit->textHeight();
    mEdit->setTopLineSegment(50, 0);
    QCOMPARE(mEdit->mTopLine, 50);
    QSynedit::CharPos savedCaret{5, 55}; // ch, line; on screen from the top
    mEdit->setCaretPos(savedCaret);

    QStringList newLines;
    for (int i = 0; i < 160; i++)
        newLines << QString("reformatted line %1; int variable%1 = 0;").arg(i);
    QString newContent = newLines.join('\n');

    mEdit->replaceContent(newContent);

    QCOMPARE(mEdit->mPendingTopLine, -1);
    QCOMPARE(mEdit->caretPos().line, savedCaret.line);
    QCOMPARE(mEdit->caretPos().ch, savedCaret.ch);
    QCOMPARE(mEdit->mTopLine, 50);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 50 * h);
}

void TestQSyneditDisplay::test_batch_ensure_caret_visible_flush()
{
    // a batch that records a deferred ensureCaretVisible but makes no
    // document change emits no contentChanged; the outermost internal-change
    // boundary must flush it
    mEdit->loadContent(makeLines(200));
    mEdit->setTopLineSegment(0, 0);
    mEdit->beginEditing();
    mEdit->setCaretPos(QSynedit::CharPos{0, 100}); // far below the window
    mEdit->endEditing();
    QCOMPARE(mEdit->mPendingTopLine, -1);
    QVERIFY(!mEdit->mStateFlags.testFlag(QSynedit::StateFlag::EnsureCaretVisible));
    QVERIFY(mEdit->mTopLine > 0);
    QVERIFY(mEdit->mTopLine <= 100);
    QVERIFY(100 - mEdit->mTopLine < mEdit->mLinesInWindow);
}

void TestQSyneditDisplay::test_batch_ensure_caret_last_request_wins()
{
    mEdit->loadContent(makeLines(200));
    mEdit->setTopLineSegment(0, 0);
    mEdit->beginEditing();
    mEdit->setCaretPos(CharPos{0, 100});
    mEdit->ensureCaretVisible(true);
    mEdit->endEditing();

    QVERIFY(!mEdit->mStateFlags.testFlag(QSynedit::StateFlag::EnsureCaretVisible));
    QVERIFY(!mEdit->mStateFlags.testFlag(QSynedit::StateFlag::EnsureCaretVisibleForceMiddle));
    QCOMPARE(mEdit->mTopLine, 100 - (mEdit->mLinesInWindow - 1) / 2);
    QCOMPARE(mEdit->verticalScrollBar()->value(), mEdit->topPos());

    mEdit->setTopLineSegment(0, 0);
    mEdit->beginEditing();
    mEdit->ensureCaretVisible(true);
    mEdit->ensureCaretVisible(false);
    mEdit->endEditing();
    QVERIFY(!mEdit->mStateFlags.testFlag(QSynedit::StateFlag::EnsureCaretVisible));
    QVERIFY(!mEdit->mStateFlags.testFlag(QSynedit::StateFlag::EnsureCaretVisibleForceMiddle));
    QCOMPARE(mEdit->mTopLine, 100 - (mEdit->mLinesInWindow - 1));
    QCOMPARE(mEdit->verticalScrollBar()->value(), mEdit->topPos());
}

void TestQSyneditDisplay::test_nested_batch_top_restore()
{
    mEdit->loadContent(makeLines(200));
    int h = mEdit->textHeight();
    mEdit->setTopLineSegment(50, 0);
    QSynedit::CharPos savedCaret{5, 55};
    mEdit->setCaretPos(savedCaret);

    QStringList newLines;
    for (int i = 0; i < 160; i++)
        newLines << QString("reformatted line %1; int variable%1 = 0;").arg(i);

    mEdit->beginEditing();
    mEdit->replaceContent(newLines.join('\n'));
    int pendingAfterInner = mEdit->mPendingTopLine;
    mEdit->endEditing();

    QCOMPARE(pendingAfterInner, 50);
    QCOMPARE(mEdit->mPendingTopLine, -1);
    QCOMPARE(mEdit->caretPos(), savedCaret);
    QCOMPARE(mEdit->mTopLine, 50);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 50 * h);
    QCOMPARE(mEdit->verticalScrollBar()->value(), mEdit->topPos());
}

void TestQSyneditDisplay::test_no_change_batch_top_scrollbar_sync()
{
    mEdit->loadContent(makeLines(200));
    int h = mEdit->textHeight();
    mEdit->setTopLineSegment(0, 0);

    mEdit->beginEditing();
    mEdit->beginEditing();
    mEdit->setTopLineSegment(80, 0);
    mEdit->endEditing();
    int pendingAfterInner = mEdit->mPendingTopLine;
    mEdit->endEditing();

    QCOMPARE(pendingAfterInner, 80);
    QCOMPARE(mEdit->mPendingTopLine, -1);
    QCOMPARE(mEdit->mTopLine, 80);
    QCOMPARE(mEdit->mTopSegment, 0);
    QCOMPARE(mEdit->topPos(), 80 * h);
    QCOMPARE(mEdit->verticalScrollBar()->value(), 80 * h);

    // A direct Document batch has the same changing/changed boundary.
    mEdit->setTopLineSegment(0, 0);
    mEdit->document()->beginUpdate();
    mEdit->setTopLineSegment(60, 0);
    mEdit->document()->endUpdate();
    QCOMPARE(mEdit->mPendingTopLine, -1);
    QCOMPARE(mEdit->mTopLine, 60);
    QCOMPARE(mEdit->topPos(), 60 * h);
    QCOMPARE(mEdit->verticalScrollBar()->value(), 60 * h);
}

void TestQSyneditDisplay::test_batch_top_intent_preserves_command_order()
{
    mEdit->loadContent(makeLines(200));
    mEdit->setTopLineSegment(0, 0);

    mEdit->beginEditing();
    mEdit->document()->insertLines(5, 2, false);
    mEdit->setTopLineSegment(50, 0);
    // This edit is adjacent to the pre-intent insertion and would coalesce
    // with it without a command-order checkpoint.
    mEdit->document()->insertLines(7, 3, false);
    mEdit->endEditing();
    QCOMPARE(mEdit->mTopLine, 50);
    QCOMPARE(mEdit->topPos(), 50 * mEdit->textHeight());

    mEdit->beginEditing();
    mEdit->document()->insertLines(5, 2, false);
    mEdit->setTopLineSegment(40, 0);
    mEdit->endEditing();
    QCOMPARE(mEdit->mTopLine, 40);
    QCOMPARE(mEdit->topPos(), 40 * mEdit->textHeight());

    mEdit->document()->beginUpdate();
    mEdit->document()->insertLines(5, 2, false);
    mEdit->setTopLineSegment(45, 0);
    mEdit->document()->insertLines(7, 3, false);
    mEdit->document()->endUpdate();
    QCOMPARE(mEdit->mTopLine, 45);
    QCOMPARE(mEdit->topPos(), 45 * mEdit->textHeight());
}

void TestQSyneditDisplay::test_shared_document_keeps_top_anchor()
{
    mEdit->loadContent(makeLines(200));
    auto edit2 = std::make_shared<QSynEdit>();
    edit2->setSyntaxer(std::make_shared<CppSyntaxer>());
    edit2->setDocument(mEdit->document());
    edit2->setFont(mEdit->font());
    edit2->resize(600, 400);
    edit2->setTopLineSegment(50, 0);
    edit2->setCaretPos(CharPos{0, 60});
    int h = edit2->textHeight();

    mEdit->beginEditing();
    mEdit->document()->insertLines(5, 3, false);
    mEdit->endEditing();
    QCOMPARE(edit2->topLine(), 50);
    QCOMPARE(edit2->topSegment(), 0);
    QCOMPARE(edit2->caretPos(), CharPos(0, 63));
    QCOMPARE(edit2->topPos(), 50 * h);
    QCOMPARE(edit2->verticalScrollBar()->value(), edit2->topPos());

    mEdit->beginEditing();
    mEdit->document()->deleteLines(5, 3, false);
    mEdit->endEditing();
    QCOMPARE(edit2->topLine(), 50);
    QCOMPARE(edit2->topSegment(), 0);
    QCOMPARE(edit2->caretPos(), CharPos(0, 60));
    QCOMPARE(edit2->topPos(), 50 * h);
    QCOMPARE(edit2->verticalScrollBar()->value(), edit2->topPos());

    mEdit->beginEditing();
    mEdit->document()->moveLine(0, 10, false);
    mEdit->endEditing();
    QCOMPARE(edit2->topLine(), 50);
    QCOMPARE(edit2->topSegment(), 0);
    QCOMPARE(edit2->topPos(), 50 * h);
    QCOMPARE(edit2->verticalScrollBar()->value(), edit2->topPos());
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

void TestQSyneditDisplay::test_shared_document_full_replace_preserves_view_state()
{
    mEdit->loadContent(makeLines(200));
    auto edit2 = std::make_shared<QSynEdit>();
    edit2->setSyntaxer(std::make_shared<CppSyntaxer>());
    edit2->setDocument(mEdit->document());
    edit2->setFont(mEdit->font());
    edit2->resize(600, 400);
    CharPos savedCaret{5, 55};
    CharPos savedSelBegin{3, 50};
    CharPos savedSelEnd{8, 60};
    edit2->setCaretAndSelection(savedCaret, savedSelBegin, savedSelEnd);
    edit2->setTopLineSegment(50, 0);
    mEdit->setCaretPos(savedCaret);
    mEdit->setTopLineSegment(50, 0);

    QStringList newLines;
    for (int i = 0; i < 160; i++)
        newLines << QString("reformatted line %1;").arg(i);
    QSignalSpy statusSpy(mEdit.get(), &QSynEdit::statusChanged);
    mEdit->replaceContent(newLines.join('\n'));

    auto checkSharedViewState = [&] {
        QCOMPARE(edit2->caretPos(), savedCaret);
        QCOMPARE(edit2->selBegin(), savedSelBegin);
        QCOMPARE(edit2->selEnd(), savedSelEnd);
        QCOMPARE(edit2->topLine(), 50);
        QCOMPARE(edit2->topSegment(), 0);
    };
    checkSharedViewState();
    QCOMPARE(mEdit->caretPos(), savedCaret);
    QVERIFY(mEdit->modified());
    bool modifiedReported = false;
    for (const auto &args : statusSpy) {
        modifiedReported = modifiedReported
                || args[0].value<StatusChanges>().testFlag(StatusChange::Modified);
    }
    QVERIFY(modifiedReported);

    mEdit->undo();
    checkSharedViewState();
    QCOMPARE(mEdit->caretPos(), savedCaret);
    mEdit->redo();
    checkSharedViewState();
    QCOMPARE(mEdit->caretPos(), savedCaret);
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
            [&](const DocumentChangeSet &, bool dirty, int L, bool) {
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
        int kind = rng() % 10;
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
        case 6:
            mEdit->document()->moveLine(line, rng() % n, true);
            break;
        case 7:
            if (n > 1)
                mEdit->document()->deleteLines(line, std::min(2, n - line), true);
            break;
        case 8:
            mEdit->document()->insertLines(line, 2, true);
            break;
        case 9: {
            // Reparse first, then move both the document and its freeze line.
            mEdit->document()->beginUpdate();
            mEdit->document()->putLine(line, (rng() % 2) ? "{" : "}", true);
            int insertAt = rng() % mEdit->document()->count();
            mEdit->document()->insertLines(insertAt, 2, true);
            mEdit->document()->moveLine(insertAt, rng() % mEdit->document()->count(), true);
            mEdit->document()->endUpdate();
            break;
        }
        }
        opLog.append(QString("  => %1").arg(dumpFoldRanges(mEdit->foldRangesForTest())));
        verifyRescan();
    }
}

void TestQSyneditDisplay::test_public_edit_fold_rescan()
{
    EditorOptions options = mEdit->getOptions();
    options.setFlag(EditorOption::TabsToSpaces, false);
    mEdit->setOptions(options);
    auto verifyRescan = [&](const QString &name) {
        QList<std::tuple<int,int,bool>> partial = mEdit->foldRangesForTest();
        mEdit->fullRescanCodeBlocksForTest();
        QList<std::tuple<int,int,bool>> full = mEdit->foldRangesForTest();
        QVERIFY2(partial == full,
                 qPrintable(QString("%1\npartial: %2\nfull:    %3")
                            .arg(name, dumpFoldRanges(partial), dumpFoldRanges(full))));
    };
    auto runOnContent = [&](const char *name, const QStringList &initialContent,
                            const std::function<void()> &op) {
        mEdit->loadContent(initialContent);
        mEdit->setInsertMode(true);
        mEdit->setCaretPos(CharPos{5, 1});
        QStringList originalContent = mEdit->content();
        op();
        QStringList editedContent = mEdit->content();
        verifyRescan(name);
        if (mEdit->canUndo()) {
            mEdit->undo();
            QVERIFY2(mEdit->content() == originalContent,
                     qPrintable(QString("%1 / undo content\nactual:   %2\nexpected: %3")
                                .arg(name, mEdit->content().join('\n'), originalContent.join('\n'))));
            verifyRescan(QString("%1 / undo").arg(name));
            QVERIFY(mEdit->canRedo());
            mEdit->redo();
            QVERIFY2(mEdit->content() == editedContent,
                     qPrintable(QString("%1 / redo content\nactual:   %2\nexpected: %3")
                                .arg(name, mEdit->content().join('\n'), editedContent.join('\n'))));
            verifyRescan(QString("%1 / redo").arg(name));
        }
    };
    auto run = [&](const char *name, const std::function<void()> &op) {
        runOnContent(name, makeFoldableLines(), op);
    };
    auto selectLines = [&] {
        mEdit->setCaretAndSelection(CharPos{0, 5}, CharPos{0, 1}, CharPos{0, 5});
    };

    run("input", [&] {
        mEdit->setCaretPos(CharPos{mEdit->lineText(2).length() - 1, 2});
        mEdit->processCommand(EditCommand::Input, "{");
    });
    run("IME input", [&] {
        mEdit->processCommand(EditCommand::ImeStr, "x");
    });
    run("overwrite input", [&] {
        mEdit->setInsertMode(false);
        mEdit->processCommand(EditCommand::Input, "{");
    });
    run("delete previous char", [&] {
        mEdit->setCaretPos(CharPos{mEdit->lineText(2).length(), 2});
        mEdit->processCommand(EditCommand::DeleteLastChar);
    });
    run("delete char", [&] {
        mEdit->setCaretPos(CharPos{mEdit->lineText(2).length() - 1, 2});
        mEdit->processCommand(EditCommand::DeleteChar);
    });
    run("delete word", [&] {
        mEdit->processCommand(EditCommand::DeleteWord);
    });
    run("delete word start", [&] {
        mEdit->setCaretPos(CharPos{8, 0});
        mEdit->processCommand(EditCommand::DeleteWordStart);
    });
    run("delete word end", [&] {
        mEdit->setCaretPos(CharPos{5, 0});
        mEdit->processCommand(EditCommand::DeleteWordEnd);
    });
    run("delete to BOL", [&] {
        mEdit->setCaretPos(mEdit->lineEnd(2));
        mEdit->processCommand(EditCommand::DeleteBOL);
    });
    run("delete to EOL", [&] {
        mEdit->setCaretPos(CharPos{10, 0});
        mEdit->processCommand(EditCommand::DeleteEOL);
    });
    run("delete line", [&] {
        mEdit->setCaretPos(CharPos{0, 2});
        mEdit->processCommand(EditCommand::DeleteLine);
    });
    run("delete selection", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::DeleteChar);
    });
    run("clear all", [&] {
        mEdit->processCommand(EditCommand::ClearAll);
    });
    run("line break", [&] {
        mEdit->setCaretPos(CharPos{mEdit->lineText(2).length() - 1, 2});
        mEdit->processCommand(EditCommand::LineBreak);
    });
    run("line break at end", [&] {
        mEdit->setCaretPos(CharPos{0, 2});
        mEdit->processCommand(EditCommand::LineBreakAtEnd);
    });
    run("duplicate line", [&] {
        mEdit->setCaretPos(CharPos{0, 2});
        mEdit->processCommand(EditCommand::Duplicate);
    });
    run("duplicate selection", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::Duplicate);
    });
    run("move selection up", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::MoveSelUp);
    });
    run("move selection down", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::MoveSelDown);
    });
    run("block indent", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::BlockIndent);
    });
    run("block unindent", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::BlockUnindent);
    });
    run("tab", [&] {
        mEdit->setCaretPos(CharPos{0, 1});
        mEdit->processCommand(EditCommand::Tab);
    });
    run("shift tab", [&] {
        mEdit->setCaretPos(CharPos{0, 1});
        mEdit->processCommand(EditCommand::ShiftTab);
    });
    run("replace line", [&] {
        mEdit->replaceLine(2, "    while (a) {");
    });
    run("set selection text", [&] {
        mEdit->setCaretAndSelection(CharPos{0, 3}, CharPos{0, 2}, CharPos{0, 3});
        mEdit->setSelText("    while (a) {\n");
    });
    run("cut", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::Cut);
    });
    run("paste", [&] {
        QApplication::clipboard()->setText("    if (b) {\n    }\n");
        mEdit->processCommand(EditCommand::Paste);
    });
    run("comment", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::Comment);
    });
    QStringList commentedLines = makeFoldableLines();
    for (int i = 1; i <= 4; i++)
        commentedLines[i].prepend("//");
    runOnContent("uncomment", commentedLines, [&] {
        selectLines();
        mEdit->processCommand(EditCommand::Uncomment);
    });
    run("toggle comment", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::ToggleComment);
    });
    run("toggle block comment", [&] {
        selectLines();
        mEdit->processCommand(EditCommand::ToggleBlockComment);
    });
    QStringList trailingSpaceLines = makeFoldableLines();
    trailingSpaceLines[1].append("   ");
    runOnContent("trim trailing spaces", trailingSpaceLines, [&] {
        mEdit->processCommand(EditCommand::TrimTrailingSpaces);
    });
    run("replace content", [&] {
        mEdit->replaceContent("void g() {\n    if (true) {\n    }\n}\n");
    });
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

    // An unrelated move must not delete an unfinished one-line block.
    mEdit->loadContent(QStringList{"int x;", "int y;", "void f() {"});
    mEdit->document()->moveLine(0, 1, true);
    expectRanges({{2, 2, false}}, "move keeps unfinished one-line block");
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

void TestQSyneditDisplay::test_wrap_job_only_computes_pending_lines()
{
    mEdit->loadContent(makeLines(10));
    PWrapJobSharedState state = std::make_shared<WrapJobSharedState>();
    state->latestGeneration = 1;
    state->jobsInFlight = 1;
    QSet<size_t> pending{mEdit->document()->getLineSeq(1),
                         mEdit->document()->getLineSeq(3)};
    WrapLayoutJob job(mEdit->document(), 1, 0,
                      mEdit->syntaxer()->createInstance(),
                      mEdit->font(), mEdit->mGlyphCalculator,
                      mEdit->wrapWidth(), true, state, pending);
    QList<WrapLayoutBatch> batches;
    QObject context;
    connect(&job, &WrapLayoutJob::layoutsReady, &context,
            [&](const WrapLayoutBatch &batch) {
        batches.append(batch);
    });
    job.run();

    QSet<size_t> computed;
    for (const WrapLayoutBatch &batch : batches) {
        QVERIFY(!batch.aborted);
        for (auto it = batch.layouts.constBegin(); it != batch.layouts.constEnd(); ++it)
            computed.insert(it.key());
    }
    QCOMPARE(computed, pending);
    QVERIFY(batches.constLast().finished);
}

void TestQSyneditDisplay::test_wrap_missing_seqs_incremental_tracking()
{
    auto expectCacheMissingInvariant = [&] {
        for (int line = 0; line < mEdit->lineCount(); ++line) {
            size_t seq = mEdit->document()->getLineSeq(line);
            QCOMPARE(mEdit->mWrapMissingSeqs.contains(seq),
                     !mEdit->mWrapLayouts.contains(seq));
        }
    };

    mEdit->loadContent(makeLines(6));
    drainLayoutJobs();
    QVERIFY(mEdit->mWrapMissingSeqs.isEmpty());
    expectCacheMissingInvariant();

    mEdit->properSetLine(1, "changed", true);
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    QVERIFY(mEdit->mWrapAllComputed);
    QVERIFY(mEdit->mWrapMissingSeqs.isEmpty());
    expectCacheMissingInvariant();

    mEdit->properInsertLine(1, "single", true);
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    QVERIFY(mEdit->mWrapMissingSeqs.isEmpty());
    expectCacheMissingInvariant();

    mEdit->properInsertLines(1, 3, true);
    QSet<size_t> insertedSeqs;
    for (int line = 1; line <= 3; ++line)
        insertedSeqs.insert(mEdit->document()->getLineSeq(line));
    QCOMPARE(mEdit->mWrapMissingSeqs, insertedSeqs);
    expectCacheMissingInvariant();
    drainLayoutJobs();
    QVERIFY(mEdit->mWrapMissingSeqs.isEmpty());
    expectCacheMissingInvariant();

    size_t deletedSeq = mEdit->document()->getLineSeq(1);
    size_t remainingSeq = mEdit->document()->getLineSeq(2);
    mEdit->mWrapLayouts.remove(deletedSeq);
    mEdit->mWrapLayouts.remove(remainingSeq);
    mEdit->mWrapMissingSeqs = {deletedSeq, remainingSeq};
    mEdit->mWrapAllComputed = false;
    mEdit->properDeleteLines(1, 1, true);
    QVERIFY(!mEdit->mWrapMissingSeqs.contains(deletedSeq));
    QCOMPARE(mEdit->mWrapMissingSeqs, QSet<size_t>{remainingSeq});
    expectCacheMissingInvariant();
    drainLayoutJobs();
    QVERIFY(mEdit->mWrapLayouts.contains(remainingSeq));
    QVERIFY(mEdit->mWrapMissingSeqs.isEmpty());

    size_t movedSeq = mEdit->document()->getLineSeq(1);
    mEdit->properMoveLine(1, 3, true);
    QVERIFY(mEdit->mWrapLayouts.contains(movedSeq));
    QVERIFY(mEdit->mWrapMissingSeqs.isEmpty());
    expectRowMapping();

    mEdit->invalidateAllLineMetrics();
    QVERIFY(!mEdit->mWrapMissingSeqs.isEmpty());
    expectCacheMissingInvariant();
    drainLayoutJobs();
    QVERIFY(mEdit->mWrapMissingSeqs.isEmpty());
    expectCacheMissingInvariant();
    expectRowMapping();
}

void TestQSyneditDisplay::test_wrap_schedule_without_missing_lines()
{
    mEdit->loadContent(makeLines(3));
    drainLayoutJobs();
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    QVERIFY(mEdit->mWrapMissingSeqs.isEmpty());

    mEdit->mWrapAllComputed = false;
    mEdit->scheduleWrapLayoutJob();
    QVERIFY(mEdit->mWrapAllComputed);
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
}

void TestQSyneditDisplay::test_single_line_edit_indexes_without_job()
{
    QSynedit::EditorOptions options = mEdit->getOptions();
    options.setFlag(EditorOption::WordWrap, true);
    mEdit->setOptions(options);
    mEdit->loadContent({"short", "mid", "last"});
    drainLayoutJobs();
    QString longText = QString(200, 'x');

    mEdit->document()->beginUpdate();
    mEdit->properInsertLine(1, "", false);
    mEdit->properSetLine(0, longText, true);
    mEdit->document()->endUpdate();

    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    QVERIFY(mEdit->mWrapAllComputed);
    size_t insertedSeq = mEdit->document()->getLineSeq(1);
    QVERIFY(mEdit->mWrapLayouts.contains(insertedSeq));
    expectRowMapping();
    QCOMPARE(mEdit->maxLineWidth(), mEdit->lineWidth(0));
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(0));

    mEdit->properDeleteLines(1, 1, true);
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    expectRowMapping();

    mEdit->setCaretPos(mEdit->lineEnd(0));
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->lineCount(), 4);
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    expectRowMapping();
    mEdit->undo();
    QCOMPARE(mEdit->lineCount(), 3);
    expectRowMapping();
    mEdit->redo();
    QCOMPARE(mEdit->lineCount(), 4);
    expectRowMapping();

    options.setFlag(EditorOption::WordWrap, false);
    mEdit->setOptions(options);
}

#ifdef QSYNEDIT_PROFILE_TESTS
void TestQSyneditDisplay::test_miniaudio_single_line_edit_profile()
{
    if (qEnvironmentVariable("QSYNEDIT_INCREMENTAL_LAYOUT_PROFILE") != "1")
        QSKIP("Set QSYNEDIT_INCREMENTAL_LAYOUT_PROFILE=1 to run the miniaudio profile");
    QByteArray encoding;
    mEdit->loadFromFile("resources/miniaudio.h", ENCODING_AUTO_DETECT, encoding);
    drainLayoutJobs();
    QSignalSpy rangeSpy(mEdit->horizontalScrollBar(), &QAbstractSlider::rangeChanged);
    QElapsedTimer timer;
    timer.start();
    for (int repeat = 0; repeat < 10; ++repeat) {
        mEdit->document()->beginUpdate();
        mEdit->properInsertLine(1, "", false);
        mEdit->properSetLine(0, mEdit->lineText(0), true);
        mEdit->document()->endUpdate();
        QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
        mEdit->properDeleteLines(1, 1, true);
        QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    }
    expectRowMapping();
    QCOMPARE(rangeSpy.count(), 0);
    qInfo().noquote() << QString("INCREMENTAL_LAYOUT_PROFILE edits=20 ns=%1 lines=%2")
                         .arg(timer.nsecsElapsed()).arg(mEdit->lineCount());
}

void TestQSyneditDisplay::test_miniaudio_edit_profile_data()
{
    QTest::addColumn<QString>("operation");
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("foldBoundary");
    QTest::addColumn<QString>("position");

    const QStringList operations{"modify", "insert", "delete", "replace", "move"};
    const QList<int> counts{1, 10, 100, 1000, 10000};
    for (const QString &operation : operations) {
        for (int count : counts) {
            for (bool foldBoundary : {false, true}) {
                QString name = QStringLiteral("%1_%2_%3_middle")
                        .arg(operation).arg(count)
                        .arg(foldBoundary ? QStringLiteral("fold") : QStringLiteral("plain"));
                QTest::newRow(name.toUtf8().constData())
                        << operation << count << foldBoundary << QStringLiteral("middle");
            }
        }
    }
    for (int count : {1, 1000}) {
        for (const QString &position : {QStringLiteral("begin"), QStringLiteral("end")}) {
            for (bool foldBoundary : {false, true}) {
                QString name = QStringLiteral("modify_%1_%2_%3")
                        .arg(count).arg(foldBoundary ? QStringLiteral("fold") : QStringLiteral("plain"), position);
                QTest::newRow(name.toUtf8().constData())
                        << QStringLiteral("modify") << count << foldBoundary << position;
            }
        }
    }
}

void TestQSyneditDisplay::test_miniaudio_edit_profile()
{
    if (qEnvironmentVariable("QSYNEDIT_EDIT_PROFILE") != "1")
        QSKIP("Set QSYNEDIT_EDIT_PROFILE=1 to run view edit profiling");
    QFETCH(QString, operation);
    QFETCH(int, count);
    QFETCH(bool, foldBoundary);
    QFETCH(QString, position);

    static const QStringList sourceLines = [] {
        QFile file("resources/miniaudio.h");
        if (!file.open(QIODevice::ReadOnly))
            return QStringList();
        return QString::fromUtf8(file.readAll()).split('\n', Qt::KeepEmptyParts);
    }();
    QVERIFY(sourceLines.count() > 20000);
    bool repeatValid = false;
    int repeats = qEnvironmentVariableIntValue("QSYNEDIT_EDIT_PROFILE_REPEAT", &repeatValid);
    if (!repeatValid || repeats <= 0)
        repeats = 3;

    auto profileLine = [](bool fold, int index) {
        QString suffix = fold ? (index % 2 == 0 ? QStringLiteral("{") : QStringLiteral("}"))
                              : QStringLiteral(" ");
        return QStringLiteral("static int red_panda_profile_value = 1; ") + suffix;
    };
    auto replaceLines = [&](int startLine, int lineCount, bool fold) {
        mEdit->document()->beginUpdate();
        for (int i = 0; i < lineCount; ++i) {
            mEdit->properSetLine(startLine + i,
                                 fold ? profileLine(true, i)
                                      : QStringLiteral("static int red_panda_profile_value = 0;  "),
                                 i == lineCount - 1);
        }
        mEdit->document()->endUpdate();
    };
    auto applyEdit = [&](int startLine) {
        mEdit->document()->beginUpdate();
        if (operation == QStringLiteral("modify")) {
            for (int i = 0; i < count; ++i) {
                mEdit->properSetLine(startLine + i, profileLine(foldBoundary, i),
                                     i == count - 1);
            }
        } else if (operation == QStringLiteral("insert")) {
            mEdit->document()->insertLinesDeferred(startLine, count);
            for (int i = 0; i < count; ++i) {
                mEdit->properSetLine(startLine + i, profileLine(foldBoundary, i),
                                     i == count - 1);
            }
        } else if (operation == QStringLiteral("move")) {
            for (int i = count - 1; i >= 0; --i) {
                mEdit->properMoveLine(startLine + i, startLine + i + 100, i == 0);
            }
        } else if (operation == QStringLiteral("delete")) {
            mEdit->properDeleteLines(startLine, count, true);
        } else if (operation == QStringLiteral("replace")) {
            mEdit->properDeleteLines(startLine, count, false);
            mEdit->document()->insertLinesDeferred(startLine, count);
            for (int i = 0; i < count; ++i) {
                mEdit->properSetLine(startLine + i, profileLine(foldBoundary, i),
                                     i == count - 1);
            }
        } else {
            Q_ASSERT(false);
        }
        mEdit->document()->endUpdate();
    };

    EditorOptions options = mEdit->getOptions();
    const EditorOptions oldOptions = options;
    options.setFlag(EditorOption::WordWrap, false);
    mEdit->setOptions(options);
    const bool oldFoldingEnabled = mEdit->useCodeFolding();
    bool foldingEnabled = qEnvironmentVariable("QSYNEDIT_EDIT_PROFILE_FOLDING") != "0";
    mEdit->setUseCodeFolding(foldingEnabled);
    mEdit->show();
    for (int sample = 0; sample < repeats; ++sample) {
        mEdit->loadContent(sourceLines);
        drainLayoutJobs();
        int lastStart = qMax(0, mEdit->lineCount() - count - 1);
        int startLine = qMin((mEdit->lineCount() - count) / 2, lastStart);
        if (position == QStringLiteral("begin"))
            startLine = qMin(1000, lastStart);
        else if (position == QStringLiteral("end"))
            startLine = qMax(0, mEdit->lineCount() - count - 1000);
        auto startSyntaxer = mEdit->document()->createSyntaxerInstance();
        while (startLine > 0 && startLine < lastStart) {
            PSyntaxState state = mEdit->document()->getSyntaxState(startLine - 1);
            if (!state || (!startSyntaxer->isCommentNotFinished(state)
                           && !startSyntaxer->isStringNotFinished(state)))
                break;
            ++startLine;
        }
        if (operation != QStringLiteral("insert")) {
            bool prepareFold = (operation == QStringLiteral("delete")
                                || operation == QStringLiteral("move")) && foldBoundary;
            replaceLines(startLine, count, prepareFold);
            drainLayoutJobs();
        }

        QObject context;
        int reparsedLines = 0;
        int changeEntries = 0;
        int changedLines = 0;
        int foldRescans = 0;
        bool foldStateDirty = false;
        auto document = mEdit->document();
        connect(document.get(), &Document::linesReparsed, &context,
                [&](int firstLine, int endLine) {
            int boundedEnd = endLine == INT_MAX ? document->count()
                                                : qMin(endLine, document->count());
            reparsedLines += qMax(0, boundedEnd - firstLine);
        });
        connect(document.get(), &Document::contentChanged, &context,
                [&](const DocumentChangeSet &changes, bool dirty, int, bool) {
            changeEntries += changes.count();
            for (const DocumentChange &change : changes)
                changedLines += change.kind == DocumentChangeKind::Move ? 1 : change.count;
            foldStateDirty = foldStateDirty || dirty;
        });
        connect(mEdit.get(), &QSynEdit::foldsRescaned, &context, [&] {
            ++foldRescans;
        });

        int oldLineCount = mEdit->lineCount();
        QElapsedTimer timer;
        timer.start();
        applyEdit(startLine);
        qint64 syncElapsed = timer.nsecsElapsed();
        while ((!mEdit->mWrapAllComputed || mEdit->mWrapJobState->jobsInFlight != 0)
               && timer.nsecsElapsed() < 120000000000LL) {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        qint64 totalElapsed = timer.nsecsElapsed();
        QVERIFY(mEdit->mWrapAllComputed);
        QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);

        int expectedLineCount = oldLineCount;
        if (operation == QStringLiteral("insert"))
            expectedLineCount += count;
        else if (operation == QStringLiteral("delete"))
            expectedLineCount -= count;
        QCOMPARE(mEdit->lineCount(), expectedLineCount);
        QVERIFY(reparsedLines > 0);
        QCOMPARE(foldStateDirty, foldBoundary);

        qInfo().noquote() << QStringLiteral(
                    "VIEW_EDIT_PROFILE,%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13")
                .arg(operation).arg(count)
                .arg(foldBoundary ? QStringLiteral("fold") : QStringLiteral("plain"), position)
                .arg(sample).arg(syncElapsed).arg(totalElapsed).arg(reparsedLines)
                .arg(changeEntries).arg(changedLines).arg(foldStateDirty ? 1 : 0)
                .arg(foldRescans).arg(foldingEnabled ? 1 : 0);
    }
    mEdit->hide();
    mEdit->setOptions(oldOptions);
    mEdit->setUseCodeFolding(oldFoldingEnabled);
}

void TestQSyneditDisplay::test_miniaudio_view_overhead_profile()
{
    if (qEnvironmentVariable("QSYNEDIT_EDIT_PROFILE") != "1")
        QSKIP("Set QSYNEDIT_EDIT_PROFILE=1 to run view overhead profiling");
    QByteArray encoding;
    mEdit->loadFromFile("resources/miniaudio.h", ENCODING_AUTO_DETECT, encoding);
    drainLayoutJobs();
    for (int sample = 0; sample < 5; ++sample) {
        mEdit->mWrapAllComputed = false;
        QElapsedTimer timer;
        timer.start();
        mEdit->scheduleWrapLayoutJob();
        qint64 elapsed = timer.nsecsElapsed();
        QVERIFY(mEdit->mWrapAllComputed);
        QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
        qInfo().noquote() << QStringLiteral("VIEW_OVERHEAD_PROFILE,schedule_pending,%1,%2,%3")
                .arg(mEdit->lineCount()).arg(sample).arg(elapsed);
    }
    for (int sample = 0; sample < 5; ++sample) {
        QElapsedTimer timer;
        timer.start();
        mEdit->syncFoldVisibility();
        qint64 elapsed = timer.nsecsElapsed();
        qInfo().noquote() << QStringLiteral("VIEW_OVERHEAD_PROFILE,sync_fold_visibility,%1,%2,%3")
                .arg(mEdit->lineCount()).arg(sample).arg(elapsed);
    }
}
#endif

void TestQSyneditDisplay::test_multi_line_edit_indexes()
{
    QSynedit::EditorOptions options = mEdit->getOptions();
    options.setFlag(EditorOption::WordWrap, true);
    mEdit->setOptions(options);
    mEdit->loadContent(makeLines(5));
    drainLayoutJobs();

    mEdit->properInsertLines(1, 3, false);
    drainLayoutJobs();
    expectRowMapping();

    size_t movedSeq = mEdit->document()->getLineSeq(0);
    int movedWidth = mEdit->lineWidth(0);
    mEdit->properMoveLine(0, 4, true);
    expectRowMapping();
    QCOMPARE(mEdit->document()->getLineSeq(4), movedSeq);
    QCOMPARE(mEdit->lineWidth(4), movedWidth);

    mEdit->properDeleteLines(1, 2, true);
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    expectRowMapping();

    options.setFlag(EditorOption::WordWrap, false);
    mEdit->setOptions(options);
}

void TestQSyneditDisplay::test_multi_line_setline_background_layout()
{
    EditorOptions options = mEdit->getOptions();
    const EditorOptions oldOptions = options;
    options.setFlag(EditorOption::WordWrap, true);
    mEdit->setOptions(options);
    QStringList lines = makeLines(120);
    lines[5] = QString(400, 'y');
    mEdit->loadContent(lines);
    mEdit->show();
    drainLayoutJobs();
    mEdit->setCaretPos(CharPos{0, 3});

    auto setLines = [&](int first, int count, const QString &text) {
        mEdit->document()->beginUpdate();
        for (int i = first; i < first + count; ++i)
            mEdit->properSetLine(i, text, i == first + count - 1);
        mEdit->document()->endUpdate();
    };
    auto expectPendingEstimates = [&](int first, int count) {
        int missingCount = 0;
        for (int i = first; i < first + count; ++i) {
            size_t seq = mEdit->document()->getLineSeq(i);
            if (mEdit->mWrapMissingSeqs.contains(seq)) {
                ++missingCount;
                QVERIFY(!mEdit->mWrapLayouts.contains(seq));
                QCOMPARE(mEdit->mRowIndex.segCount(i), 1);
            } else {
                QVERIFY(mEdit->mWrapLayouts.contains(seq));
                QCOMPARE(mEdit->mRowIndex.segCount(i),
                         mEdit->mWrapLayouts.value(seq).segmentStartChars.count());
            }
        }
        QVERIFY(missingCount > 0);
    };

    setLines(80, 3, QString(500, 'x'));
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 1);
    QVERIFY(!mEdit->mWrapAllComputed);
    expectPendingEstimates(80, 3);

    int segment = mEdit->wrapSegmentOfChar(81, 0);
    QCOMPARE(segment, 0);
    size_t queriedSeq = mEdit->document()->getLineSeq(81);
    QVERIFY(!mEdit->mWrapMissingSeqs.contains(queriedSeq));
    QCOMPARE(mEdit->mRowIndex.segCount(81),
             mEdit->mWrapLayouts.value(queriedSeq).segmentStartChars.count());

    drainLayoutJobs();
    QVERIFY(mEdit->mWrapMissingSeqs.isEmpty());
    QCOMPARE(mEdit->maxLineWidth(), mEdit->lineWidth(80));
    expectRowMapping();

    setLines(80, 3, "x");
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 1);
    expectPendingEstimates(80, 3);
    drainLayoutJobs();
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(5));
    expectRowMapping();

    mEdit->hide();
    mEdit->setOptions(oldOptions);
}

void TestQSyneditDisplay::test_max_width_notifications()
{
    mEdit->loadContent({"x", QString(1000, 'y'), QString(800, 'z')});
    drainLayoutJobs();
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(1));
    QSignalSpy rangeSpy(mEdit->horizontalScrollBar(), &QAbstractSlider::rangeChanged);

    mEdit->replaceLine(0, QString(2000, 'x'));
    QTRY_COMPARE(rangeSpy.count(), 1);
    QCOMPARE(mEdit->maxLineWidth(), mEdit->lineWidth(0));
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(0));

    rangeSpy.clear();
    mEdit->replaceLine(2, QString(700, 'z'));
    drainLayoutJobs();
    QCoreApplication::processEvents();
    QCOMPARE(rangeSpy.count(), 0);
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(0));

    rangeSpy.clear();
    mEdit->replaceLine(0, "x");
    QTRY_COMPARE(rangeSpy.count(), 1);
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(1));

    rangeSpy.clear();
    mEdit->replaceLine(1, QString(600, 'x'));
    QCOMPARE(rangeSpy.count(), 0);
    QTRY_COMPARE(rangeSpy.count(), 1);
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(2));

    rangeSpy.clear();
    mEdit->properDeleteLines(2, 1, true);
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    QTRY_COMPARE(rangeSpy.count(), 1);
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(1));

    rangeSpy.clear();
    mEdit->document()->beginUpdate();
    mEdit->properSetLine(0, QString(1000, 'a'), false);
    mEdit->properSetLine(1, QString(1200, 'b'), true);
    mEdit->document()->endUpdate();
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 1);
    QCOMPARE(rangeSpy.count(), 0);
    drainLayoutJobs();
    QTRY_COMPARE(rangeSpy.count(), 1);
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(1));

    rangeSpy.clear();
    mEdit->document()->beginUpdate();
    mEdit->properSetLine(0, QString(10, 'a'), false);
    mEdit->properSetLine(1, QString(20, 'b'), true);
    mEdit->document()->endUpdate();
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 1);
    QCOMPARE(rangeSpy.count(), 0);
    drainLayoutJobs();
    QTRY_COMPARE(rangeSpy.count(), 1);
    QCOMPARE(mEdit->mMaxLineWidthSeq, mEdit->document()->getLineSeq(1));
    expectRowMapping();
}

void TestQSyneditDisplay::test_shared_document_edit_indexes()
{
    QSynedit::EditorOptions options = mEdit->getOptions();
    options.setFlag(EditorOption::WordWrap, true);
    mEdit->setOptions(options);
    mEdit->loadContent(makeLines(4));
    auto second = std::make_shared<QSynEdit>();
    second->setSyntaxer(std::make_shared<CppSyntaxer>());
    second->setFont(mEdit->font());
    second->resize(500, 300);
    second->setDocument(mEdit->document());
    drainLayoutJobs();
    drainLayoutJobs(second.get());

    mEdit->document()->beginUpdate();
    mEdit->properInsertLine(1, "", false);
    mEdit->properSetLine(0, QString(100, 'x'), true);
    mEdit->document()->endUpdate();
    QCOMPARE(mEdit->mWrapJobState->jobsInFlight, 0);
    QCOMPARE(second->mWrapJobState->jobsInFlight, 0);
    expectRowMapping();
    expectRowMapping(second.get());

    mEdit->properMoveLine(0, 2, true);
    expectRowMapping();
    expectRowMapping(second.get());
    mEdit->properDeleteLines(1, 1, true);
    expectRowMapping();
    expectRowMapping(second.get());

    mEdit->document()->beginUpdate();
    mEdit->properSetLine(0, QString(100, 'x'), false);
    mEdit->properSetLine(1, QString(120, 'y'), false);
    mEdit->properSetLine(2, QString(140, 'z'), true);
    mEdit->document()->endUpdate();
    QCOMPARE(mEdit->mWrapJobState->latestContentVersion,
             mEdit->document()->contentVersion());
    QCOMPARE(second->mWrapJobState->latestContentVersion,
             second->document()->contentVersion());
    QVERIFY(!mEdit->mWrapMissingSeqs.isEmpty());
    QVERIFY(!second->mWrapMissingSeqs.isEmpty());
    drainLayoutJobs();
    drainLayoutJobs(second.get());
    expectRowMapping();
    expectRowMapping(second.get());

    options.setFlag(EditorOption::WordWrap, false);
    mEdit->setOptions(options);
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

void TestQSyneditDisplay::test_enter_first_line_repaints_old_first_line()
{
    mEdit->loadContent(QStringList{"first", "second"});
    mEdit->setCaretPos(CharPos{0, 0});
    QTest::keyPress(mEdit.get(), Qt::Key_Return);
    QCOMPARE(mEdit->content(), QStringList({"", "first", "second"}));
    QCOMPARE(mEdit->mTopLine, 0);
    QCOMPARE(mEdit->mTopPos, 0);
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
    // in a later segment are not skipped; check pixels hugging both sides
    // of every segment boundary. A line comment is ONE token for the C++
    // syntaxer, so it straddles several boundaries.
    //
    // The bundled test font draws every glyph as a solid rectangle
    // (gen_test_font.py: "tests measure metrics, they never compare
    // rendered pixels"), so glyph-interior probes must not rely on
    // texture: assert "glyphs hug the edge" as "some pixel near the edge
    // differs from the row's fill color". Every row here belongs to the
    // caret line (line 0), so the fill is the active-line color.
    QSynedit::EditorOptions opts = mEdit->getOptions();
    opts.setFlag(QSynedit::EditorOption::WordWrap, true);
    mEdit->setOptions(opts);
    mEdit->loadContent(QStringList() << ("// " + QString(500, 'x')));
    QTRY_VERIFY_WITH_TIMEOUT(mEdit->mWrapAllComputed, 5000);
    int segments = mEdit->wrapSegmentCount(0);
    QVERIFY(segments > 2);

    QImage image = mEdit->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    int h = mEdit->textHeight();
    int left = mEdit->textOffset() + 1;
    int probe = 3 * mEdit->charWidth();
    const QRgb fill = (mEdit->mActiveLineColor.isValid() ? mEdit->mActiveLineColor
                                                         : mEdit->mBackgroundColor).rgb();
    const WrapLayout &layout = mEdit->wrapLayout(0);
    int rowsToCheck = std::min(segments, mEdit->linesInWindow());
    auto hasGlyphPixel = [&](int x0, int x1, int y) {
        for (int x = x0; x < x1; x++)
            if (image.pixel(x, y) != fill)
                return true;
        return false;
    };
    for (int seg = 0; seg < rowsToCheck; seg++) {
        int y = seg * h + h / 2;
        // the first glyph columns of the segment's row must not be blank
        // (a lost boundary token leaves them empty)
        QVERIFY2(hasGlyphPixel(left, left + probe, y),
                 qPrintable(QString("segment %1 starts blank").arg(seg)));
        // a non-last segment is full width: glyphs must hug its right edge
        // (the segment's own end, which is narrower than the viewport)
        if (seg + 1 < segments) {
            int segRight = mEdit->textOffset()
                    + (layout.segmentStartXs[seg+1] - layout.segmentStartXs[seg]);
            QVERIFY2(hasGlyphPixel(segRight - probe, segRight - 1, y),
                     qPrintable(QString("segment %1 ends blank").arg(seg)));
        }
    }

    opts.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEdit->setOptions(opts);
}

void TestQSyneditDisplay::test_batched_highlight_preparation()
{
    PaintPreparationEdit editor;
    editor.setSyntaxer(std::make_shared<CppSyntaxer>());
    QFont font;
    font.setFamily(defaultMonoFont());
    font.setStyleStrategy(QFont::NoAntialias);
    editor.setFont(font);
    editor.resize(320, 100);
    editor.loadContent("value");

    QImage image(editor.viewport()->size(), QImage::Format_ARGB32);
    editor.viewport()->render(&image);
    QCOMPARE(editor.preparedBatchLines.count(0), 1);
    bool colorFound = false;
    for (int y = 0; y < image.height() && !colorFound; ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixel(x, y) == editor.identifierColor.rgb()) {
                colorFound = true;
                break;
            }
        }
    }
    QVERIFY(colorFound);

    EditorOptions options = editor.getOptions();
    options.setFlag(EditorOption::WordWrap, true);
    editor.setOptions(options);
    editor.resize(100, 60);
    editor.loadContent("object.member + anotherObject.anotherMember");
    editor.preparedBatchLines.clear();
    editor.viewport()->render(&image);
    QVERIFY(editor.wrapLayout(0).segmentStartChars.count() > 1);
    QCOMPARE(editor.preparedBatchLines.count(0), 1);

    options.setFlag(EditorOption::WordWrap, false);
    editor.setOptions(options);
    editor.resize(320, 100);
    editor.loadContent("value");
    editor.setCaretPos({5, 0});
    QInputMethodEvent inputMethodEvent("preedit", {});
    QApplication::sendEvent(&editor, &inputMethodEvent);
    editor.preparedTexts.clear();
    editor.viewport()->render(&image);
    QVERIFY(editor.preparedTexts.contains("valuepreedit"));
    QCOMPARE(editor.lineText(0), QString("value"));
}

}
