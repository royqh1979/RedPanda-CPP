#include <QTest>
#include "src/settings/codecompletionsettings.h"
#include "src/settings/dirsettings.h"
#include "src/settings/editorsettings.h"
#include "src/colorscheme.h"
#include "src/editor/editor.h"
#include "src/parser/cppparser.h"
#include "test_editor_symbol_completion.h"
#include <qsynedit/syntaxer/cpp.h>

using QSynedit::CharPos;

TestEditorSymbolCompletion::TestEditorSymbolCompletion(QObject *parent):
    TestEditorBase{parent}
{
}

void TestEditorSymbolCompletion::addIndentationCases()
{
    QTest::addColumn<bool>("autoIndent");
    QTest::addColumn<bool>("tabsToSpaces");
    QTest::addColumn<QString>("indent");
    QTest::newRow("manual_tabs") << false << false << "    ";
    QTest::newRow("manual_spaces") << false << true << "    ";
    QTest::newRow("auto_tabs") << true << false << "\t";
    QTest::newRow("auto_spaces") << true << true << "    ";
}

void TestEditorSymbolCompletion::configureIndentation(bool autoIndent, bool tabsToSpaces)
{
    mEditor->setTabSize(4);
    mEditor->setAutoIndent(autoIndent);
    QSynedit::EditorOptions options = mEditor->getOptions();
    options.setFlag(QSynedit::EditorOption::TabsToSpaces, tabsToSpaces);
    mEditor->setOptions(options);
}

void TestEditorSymbolCompletion::assertEditingArea(int line, int begin, int end)
{
    QSynedit::EditingAreaList areas;
    mEditor->onGetEditingAreas(line, areas);
    QCOMPARE(areas.count(), 1);
    QCOMPARE(areas.constFirst()->type, QSynedit::EditingAreaType::eatRectangleBorder);
    QCOMPARE(areas.constFirst()->beginX, begin);
    QCOMPARE(areas.constFirst()->endX, end);
    QCOMPARE(mEditor->caretPos(), CharPos(begin, line));
    QCOMPARE(mEditor->selBegin(), CharPos(begin, line));
    QCOMPARE(mEditor->selEnd(), CharPos(end, line));
    QCOMPARE(mEditor->mTabStopY, line);
    QCOMPARE(mEditor->mTabStopBegin, begin);
    QCOMPARE(mEditor->mTabStopEnd, end);
}

void TestEditorSymbolCompletion::test_code_snippet_editing_areas_data()
{
    addIndentationCases();
}

void TestEditorSymbolCompletion::test_code_snippet_editing_areas()
{
    QFETCH(bool, autoIndent);
    QFETCH(bool, tabsToSpaces);
    QFETCH(QString, indent);
    configureIndentation(autoIndent, tabsToSpaces);
    mEditor->setCppParser(nullptr);
    mEditor->loadContent(QStringList{""});
    mEditor->setCaretPos(CharPos{0, 0});

    mEditor->insertCodeSnippet(
                "if (%REPL_BEGIN%condition%REPL_END%) {\n"
                "    %REPL_BEGIN%first%REPL_END% + %REPL_BEGIN%second%REPL_END%;\n"
                "    %INSERT%\n"
                "}");

    QCOMPARE(mEditor->content(), QStringList({
        "if (condition) {",
        indent + "first + second;",
        indent,
        "}"
    }));
    assertEditingArea(0, 4, 13);

    const int offset = indent.length();
    mEditor->tab();
    assertEditingArea(1, offset, offset + 5);
    mEditor->tab();
    assertEditingArea(1, offset + 8, offset + 14);
    mEditor->tab();
    assertEditingArea(2, offset, offset);

    mEditor->tab();
    QSynedit::EditingAreaList areas;
    mEditor->onGetEditingAreas(2, areas);
    QVERIFY(areas.isEmpty());
    QCOMPARE(mEditor->caretPos(), CharPos(offset, 2));
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_string1()
{
    // auto add another '"' when input '"' and put the caret in the middle
    // auto skip the inputted '"' when caret before then enclosing '"' of string
    QStringList text{
        "const char *s = ",
    };
    QStringList text1{
        "const char *s = \"\"",
    };
    QStringList text2{
        "const char *s = \"abc\"",
    };
    QStringList text3{
        "const char *s = \"abc\";",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QTest::keyPress(mEditor.get(),'c');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

}

void TestEditorSymbolCompletion::test_input_double_quotes_in_string2()
{
    //dont skip the inputted '"' when it's in a escaping sequence
    QStringList text{
        "const char *s = \"abcdef\"",
    };
    QStringList text1{
        "const char *s = \"ab\\\"cdef\"",
    };
    QStringList text2{
        "const char *s = \"ab\\\"-\"cdef\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    mEditor->setCaretPos(CharPos{19,0});
    QTest::keyPress(mEditor.get(),'\\');
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'"');
    QTest::keyPress(mEditor.get(),'-');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

}

void TestEditorSymbolCompletion::test_input_double_quotes_in_string3()
{
    // correct input '"' in a un-enclosed string
    // correct input '"' after string
    QStringList text{
        "const char *s = \"abcdef",
    };
    QStringList text1{
        "const char *s = \"abcdef\"",
    };
    QStringList text2{
        "const char *s = \"abcdef\"\"\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'\"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'\"');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_string4()
{
    // correct input '"' before '"'
    QStringList text{
        "const char *s = \"\"",
    };
    QStringList text1{
        "const char *s = \"\"\"\"",
    };
    QStringList text2{
        "const char *s = \"ab\"\"\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'\"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_string5()
{
    // input successive '"'
    QStringList text{
        "const char *s = ",
    };
    QStringList text1{
        "const char *s = \"\"",
    };
    QStringList text2{
        "const char *s = \"\"\"\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1); //auto skip '"'
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text2); //auto skip '"'

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_on_selection()
{
    QStringList text{
        "const char *s = abc",
    };
    QStringList text1{
        "const char *s = \"abc\"",
    };
    QStringList text2{
        "const char *s = \"abc\";",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left,Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(),Qt::Key_Left,Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(),Qt::Key_Left,Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_raw_string1()
{
    // input '"' at start of raw string and
    // input '"' at end of raw string
    QStringList text{
        "const char *s = R",
    };
    QStringList text1{
        "const char *s = R\"\"",
    };
    QStringList text2{
        "const char *s = R\"()\"",
    };
    QStringList text3{
        "const char *s = R\"(ab)\"",
    };
    QStringList text4{
        "const char *s = R\"(ab)\"\"\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'(');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),')');
    QCOMPARE(mEditor->content(), text3); // auto skip ')'
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text3); // auto skip '"'
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_raw_string2()
{
    // input '"' in a raw string
    QStringList text{
        "const char *s = R\"\""
    };
    QStringList text1{
        "const char *s = R\"\"\""
    };
    QStringList text2{
        "const char *s = R\"\"a\""
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'a');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_raw_string3()
{
    // input '"' in a raw string (not escaping)
    QStringList text{
        "const char *s = R\"(\")\""
    };
    QStringList text1{
        "const char *s = R\"(\"\")\""
    };
    QStringList text2{
        "const char *s = R\"(\"a\")\""
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'a');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_raw_string4()
{
    // input '"' before raw string's starting '"'
    QStringList text{
        "const char *s = R\"\""
    };
    QStringList text1{
        "const char *s = R\"\"\"\""
    };
    QStringList text2{
        "const char *s = R\"a\"\"\""
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'a');
    QCOMPARE(mEditor->content(), text2);


    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_char_literals1()
{
    // input '"' in char literal
    QStringList text{
        "char ch = "
    };
    QStringList text1{
        "char ch = ''"
    };
    QStringList text2{
        "char ch = '\"'"
    };
    QStringList text3{
        "char ch = '\"';"
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'\'');
    QCOMPARE(mEditor->content(), text2);  // auto skip '\''
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_char_literals2()
{
    // input '\' '"' in char literal
    QStringList text{
        "char ch = "
    };
    QStringList text1{
        "char ch = ''"
    };
    QStringList text2{
        "char ch = '\\\"'"
    };
    QStringList text3{
        "char ch = '\\\"';"
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'\\');
    QTest::keyPress(mEditor.get(),'\"');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'\'');
    QCOMPARE(mEditor->content(), text2);  // auto skip '\''
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_double_quotes_in_comments()
{
    QStringList text{
        "// const char *s = ",
    };
    QStringList text1{
        "// const char *s = \"",
    };
    QStringList text2{
        "// const char *s = \"\"",
    };
    QStringList text3{
        "// const char *s = \"abc\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), '"');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '"');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(), 'a');
    QTest::keyPress(mEditor.get(), 'b');
    QTest::keyPress(mEditor.get(), 'c');
    QCOMPARE(mEditor->content(), text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_char_literals1()
{
    //input '\'' before ''
    QStringList text{
        "char ch = ''"
    };
    QStringList text1{
        "char ch = ''''"
    };
    QStringList text2{
        "char ch = 'ab'''"
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(), 'a');
    QTest::keyPress(mEditor.get(), 'b');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_char_literals2()
{
    //input '\'' in ''
    QStringList text{
        "char ch = "
    };
    QStringList text1{
        "char ch = ''"
    };
    QStringList text2{
        "char ch = ''''"
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1); //auto skip '\''
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_char_literals3()
{
    //input '\\' '\'' in ''
    QStringList text{
        "char ch = "
    };
    QStringList text1{
        "char ch = ''"
    };
    QStringList text2{
        "char ch = '\\\''"
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(), '\\');
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_string_literals()
{
    QStringList text = {
        "\"\"",
    };
    QStringList text1 = {
        "\"\'\"",
    };
    QStringList text2 = {
        "\"\'\'\"",
    };
    QStringList text3 = {
        "\"\'ab\'\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(), 'a');
    QTest::keyPress(mEditor.get(), 'b');
    QCOMPARE(mEditor->content(), text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_rawstring_not_escaping()
{
    QStringList text = {
        "const char* s = R\"()\";",
    };
    QStringList text1 = {
        "const char* s = R\"(')\";",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_rawstring_start()
{
    QStringList text = {
        "const char* s = R\"()\";",
    };
    QStringList text1 = {
        "const char* s = R\"'()\";",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_rawstring_end()
{
    QStringList text = {
        "const char* s = R\"()\";",
    };
    QStringList text1 = {
        "const char* s = R\"()'\";",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_comments1()
{
    QStringList text = {
        "// test",
    };
    QStringList text1 = {
        "// test\'",
    };
    QStringList text2 = {
        "// test\'\'",
    };
    QStringList text3 = {
        "// test\'ab\'",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(), 'a');
    QTest::keyPress(mEditor.get(), 'b');
    QCOMPARE(mEditor->content(), text3);
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_comments2()
{
    QStringList text = {
        "/* test",
    };
    QStringList text1 = {
        "/* test\'",
    };
    QStringList text2 = {
        "/* test\'\'",
    };
    QStringList text3 = {
        "/* test\'ab\'",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(), 'a');
    QTest::keyPress(mEditor.get(), 'b');
    QCOMPARE(mEditor->content(), text3);

}

void TestEditorSymbolCompletion::test_input_single_quotes_on_selection()
{
    QStringList text{
        "char ch = \\n",
    };
    QStringList text1{
        "char ch = '\\n'",
    };
    QStringList text2{
        "char ch = '\\n';",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left,Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(),Qt::Key_Left,Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(),'\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_single_quotes_in_number()
{
    QStringList text{
        "double x = 2.13",
    };
    QStringList text1{
        "double x = 2.13'",
    };
    QStringList text2{
        "double x = 2.13'45",
    };
    QStringList text3{
        "double x = 2.13'45'",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'4');
    QTest::keyPress(mEditor.get(),'5');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'\'');
    QCOMPARE(mEditor->content(), text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_parenthesis1()
{
    QStringList text{
        "int x=",
    };
    QStringList text1{
        "int x=()",
    };
    QStringList text2{
        "int x=(())",
    };
    QStringList text3{
        "int x=(())()",
    };
    QStringList text4{
        "int x=(())();",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'(');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'(');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),')'); //auto skip ')'
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),')');
    QCOMPARE(mEditor->content(), text2); //auto skip ')'
    QTest::keyPress(mEditor.get(),'(');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),')'); //auto skip ')'
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());


    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_parenthesis_in_string()
{
    QStringList text{
        "\"int x=\"",
    };
    QStringList text1{
        "\"int x=(\"",
    };
    QStringList text2{
        "\"int x=((\"",
    };
    QStringList text3{
        "\"int x=(()\"",
    };
    QStringList text4{
        "\"int x=(())\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'(');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'(');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),')');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),')');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());


    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_parenthesis_in_comment()
{
    QStringList text{
        "/* int x=",
    };
    QStringList text1{
        "/* int x=(",
    };
    QStringList text2{
        "/* int x=((",
    };
    QStringList text3{
        "/* int x=(()",
    };
    QStringList text4{
        "/* int x=(())",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'(');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'(');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),')');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),')');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_parenthesis_in_escaping_sequence()
{
    QStringList text{
        "\"int x=\\)",
    };
    QStringList text1{
        "\"int x=\\()",
    };
    QStringList text2{
        "\"int x=\\())",
    };
    QStringList text3{
        "\"int x=\\()ab)",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'(');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),')');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_parenthesis_on_selection()
{
    QStringList text{
        "\"int x=abc;",
    };
    QStringList text1{
        "\"int x=(abc);",
    };
    QStringList text2{
        "\"int x=(abc)++;",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(), '(');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'+');
    QTest::keyPress(mEditor.get(),'+');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_brackets1()
{
    QStringList text{
        "int x=",
    };
    QStringList text1{
        "int x=[]",
    };
    QStringList text2{
        "int x=[[]]",
    };
    QStringList text3{
        "int x=[[]][]",
    };
    QStringList text4{
        "int x=[[]][];",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'[');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'[');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),']'); //auto skip ']'
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),']');
    QCOMPARE(mEditor->content(), text2); //auto skip ']'
    QTest::keyPress(mEditor.get(),'[');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),']'); //auto skip ']'
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());


    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}


void TestEditorSymbolCompletion::test_input_brackets_in_string()
{
    QStringList text{
        "\"int x=\"",
    };
    QStringList text1{
        "\"int x=[\"",
    };
    QStringList text2{
        "\"int x=[[\"",
    };
    QStringList text3{
        "\"int x=[[]\"",
    };
    QStringList text4{
        "\"int x=[[]]\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'[');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'[');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),']');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),']');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());


    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_brackets_in_comment()
{
    QStringList text{
        "/* int x=",
    };
    QStringList text1{
        "/* int x=[",
    };
    QStringList text2{
        "/* int x=[[",
    };
    QStringList text3{
        "/* int x=[[]",
    };
    QStringList text4{
        "/* int x=[[]]",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'[');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'[');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),']');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),']');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_brackets_in_escaping_sequence()
{
    QStringList text{
        "\"int x=\\]",
    };
    QStringList text1{
        "\"int x=\\[]",
    };
    QStringList text2{
        "\"int x=\\[]]",
    };
    QStringList text3{
        "\"int x=\\[]ab]",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'[');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),']');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_brackets_on_selection()
{
    QStringList text{
        "\"int x=abc;",
    };
    QStringList text1{
        "\"int x=[abc];",
    };
    QStringList text2{
        "\"int x=[abc]++;",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(), '[');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'+');
    QTest::keyPress(mEditor.get(),'+');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_braces1()
{
    QStringList text{
        "int x=",
    };
    QStringList text1{
        "int x={};",
    };
    QStringList text2{
        "int x={{}};",
    };
    QStringList text3{
        "int x={{}}{};",
    };
    QStringList text4{
        "int x={{}}{};ab",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'{');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'{');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'}'); //auto skip '}'
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'}');
    QCOMPARE(mEditor->content(), text2); //auto skip '}'
    QTest::keyPress(mEditor.get(),'{');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),'}'); //auto skip '}'
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text3); //auto skip ';'
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());


    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text3);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}


void TestEditorSymbolCompletion::test_input_braces_in_string()
{
    QStringList text{
        "\"int x=\"",
    };
    QStringList text1{
        "\"int x={\"",
    };
    QStringList text2{
        "\"int x={{\"",
    };
    QStringList text3{
        "\"int x={{}\"",
    };
    QStringList text4{
        "\"int x={{}}\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'{');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'{');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'}');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),'}');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());


    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_braces_in_comment()
{
    QStringList text{
        "/* int x=",
    };
    QStringList text1{
        "/* int x={",
    };
    QStringList text2{
        "/* int x={{",
    };
    QStringList text3{
        "/* int x={{}",
    };
    QStringList text4{
        "/* int x={{}}",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'{');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'{');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'}');
    QCOMPARE(mEditor->content(), text3);
    QTest::keyPress(mEditor.get(),'}');
    QCOMPARE(mEditor->content(), text4);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text4);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_braces_in_escaping_sequence()
{
    QStringList text{
        "\"int x=\\}",
    };
    QStringList text1{
        "\"int x=\\{}",
    };
    QStringList text2{
        "\"int x=\\{}}",
    };
    QStringList text3{
        "\"int x=\\{}ab}",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'{');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'}');
    QCOMPARE(mEditor->content(), text2);
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text3);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_braces_on_selection()
{
    QStringList text{
        "\"int x=abc;",
    };
    QStringList text1{
        "\"int x={abc};",
    };
    QStringList text2{
        "\"int x={abc}++;",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(), Qt::Key_Left, Qt::ShiftModifier);
    QTest::keyPress(mEditor.get(), '{');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'+');
    QTest::keyPress(mEditor.get(),'+');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_braces_with_indent()
{
    QStringList text{
        "int main() {",
        "\tif (x>0) ",
        "\t\tx=100;",
        "}"
    };
    QStringList text1{
        "int main() {",
        "\tif (x>0) ",
        "\t{}x=100;",
        "}"
    };
    QStringList text2{
        "int main() {",
        "\tif (x>0) ",
        "\t{ab}x=100;",
        "}"
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos({2,2});
    QTest::keyPress(mEditor.get(), '{');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_asterisk_for_ansi_c_comments()
{
    QStringList text{
        "  /",
    };
    QStringList text1{
        "  /**/",
    };
    QStringList text2{
        "  /*++*/",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'*');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'+');
    QTest::keyPress(mEditor.get(),'+');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_asterisk_in_comments()
{
    QStringList text{
        "\"  /",
    };
    QStringList text1{
        "\"  /*",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'*');
    QCOMPARE(mEditor->content(), text1);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_asterisk_in_comments2()
{
    QStringList text{
        "/*  /",
    };
    QStringList text1{
        "/*  /*",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'*');
    QCOMPARE(mEditor->content(), text1);
}

void TestEditorSymbolCompletion::test_input_asterisk_in_char_literal()
{
    QStringList text{
        "'/",
    };
    QStringList text1{
        "'/*",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'*');
    QCOMPARE(mEditor->content(), text1);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_asterisk_in_string_literal()
{
    QStringList text{
        "\"/",
    };
    QStringList text1{
        "\"/*",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'*');
    QCOMPARE(mEditor->content(), text1);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_asterisk_in_rawstring_begin()
{
    QStringList text{
        "R\"/",
    };
    QStringList text1{
        "R\"/*",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'*');
    QCOMPARE(mEditor->content(), text1);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_asterisk_in_rawstring()
{
    QStringList text{
        "R\"(/)\"",
    };
    QStringList text1{
        "R\"(/*)\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'*');
    QCOMPARE(mEditor->content(), text1);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

}

void TestEditorSymbolCompletion::test_input_asterisk_in_rawstring_end()
{
    QStringList text{
        "R\"/()/\"",
    };
    QStringList text1{
        "R\"/()/*\"",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),'*');
    QCOMPARE(mEditor->content(), text1);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_periods1()
{
    QStringList text{
        "int x=10;",
    };
    QStringList text1{
        "int x=10;ab",
    };
    QStringList text2{
        "int x=10;ab;",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text); // auto skip ;
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_periods_in_comments()
{
    QStringList text{
        "// int x=10;",
    };
    QStringList text1{
        "// int x=10;;",
    };
    QStringList text2{
        "// int x=10;ab;",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_periods_in_string()
{
    QStringList text{
        "\" int x=10;",
    };
    QStringList text1{
        "\" int x=10;;",
    };
    QStringList text2{
        "\" int x=10;ab;",
    };
    mEditor->loadContent(text);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),Qt::Key_Left);
    QTest::keyPress(mEditor.get(),';');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(),'a');
    QTest::keyPress(mEditor.get(),'b');
    QCOMPARE(mEditor->content(), text2);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());

    //redo
    mEditor->redo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(), text2);
    QVERIFY(!mEditor->canRedo());
    QVERIFY(mEditor->modified());

    //undo again
    mEditor->undo();
    QCOMPARE(mEditor->content(), text1);
    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
    QVERIFY(!mEditor->canUndo());
    QVERIFY(!mEditor->modified());
}

void TestEditorSymbolCompletion::test_input_docstring()
{
    QStringList text1{
        "/***/",
    };
    QStringList text2{
        "/**",
        " * ",
        " */",
    };
    QStringList text3{
        "/**",
        " * ",
        " * ",
        " */",
    };
    mEditor->clear();
    QTest::keyPress(mEditor.get(),'/');
    QTest::keyPress(mEditor.get(),'*');
    QTest::keyPress(mEditor.get(),'*');
    QCOMPARE(mEditor->content(),text1);
    QTest::keyPress(mEditor.get(),Qt::Key_Enter);
    QCOMPARE(mEditor->content(),text2);
    QTest::keyPress(mEditor.get(),Qt::Key_Escape);
    QTest::keyPress(mEditor.get(),Qt::Key_Enter);
    QCOMPARE(mEditor->content(),text3);

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(),text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(),text1);
    mEditor->undo();
    mEditor->undo();
    mEditor->undo();
    QVERIFY(mEditor->empty());
    QVERIFY(!mEditor->canUndo());

    //redo
    mEditor->redo();
    mEditor->redo();
    mEditor->redo();
    QCOMPARE(mEditor->content(),text1);
    mEditor->redo();
    QCOMPARE(mEditor->content(),text2);
    mEditor->redo();
    QCOMPARE(mEditor->content(),text3);
    QVERIFY(!mEditor->canRedo());

    //undo
    mEditor->undo();
    QCOMPARE(mEditor->content(),text2);
    mEditor->undo();
    QCOMPARE(mEditor->content(),text1);
    mEditor->undo();
    mEditor->undo();
    mEditor->undo();
    QVERIFY(mEditor->empty());
    QVERIFY(!mEditor->canUndo());

}

void TestEditorSymbolCompletion::test_completion_disabled_by_settings_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("expected");

    QTest::newRow("global") << "global" << "" << "(" << "(";
    QTest::newRow("parenthesis") << "parenthesis" << "" << "(" << "(";
    QTest::newRow("bracket") << "bracket" << "" << "[" << "[";
    QTest::newRow("brace") << "brace" << "" << "{" << "{";
    QTest::newRow("single quote") << "single" << "" << "'" << "'";
    QTest::newRow("double quote") << "double" << "" << "\"" << "\"";
    QTest::newRow("comment") << "comment" << "/" << "*" << "/*";
    QTest::newRow("overwrite mode") << "overwrite" << "" << "(" << "(";
}

void TestEditorSymbolCompletion::test_completion_disabled_by_settings()
{
    QFETCH(QString, setting);
    QFETCH(QString, initial);
    QFETCH(QString, key);
    QFETCH(QString, expected);

    mEditorSettings->setCompleteSymbols(true);
    mEditorSettings->setCompleteParenthese(true);
    mEditorSettings->setCompleteBracket(true);
    mEditorSettings->setCompleteBrace(true);
    mEditorSettings->setCompleteComment(true);
    mEditorSettings->setCompleteSingleQuote(true);
    mEditorSettings->setCompleteDoubleQuote(true);
    mEditor->setInsertMode(true);
    if (setting == "global")
        mEditorSettings->setCompleteSymbols(false);
    else if (setting == "parenthesis")
        mEditorSettings->setCompleteParenthese(false);
    else if (setting == "bracket")
        mEditorSettings->setCompleteBracket(false);
    else if (setting == "brace")
        mEditorSettings->setCompleteBrace(false);
    else if (setting == "comment")
        mEditorSettings->setCompleteComment(false);
    else if (setting == "single")
        mEditorSettings->setCompleteSingleQuote(false);
    else if (setting == "double")
        mEditorSettings->setCompleteDoubleQuote(false);
    else if (setting == "overwrite")
        mEditor->setInsertMode(false);

    mEditor->loadContent(initial);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), key.at(0).toLatin1());
    QCOMPARE(mEditor->content(), QStringList{expected});
}

void TestEditorSymbolCompletion::test_symbol_skip_settings_data()
{
    QTest::addColumn<QString>("setting");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<int>("ch");
    QTest::addColumn<QString>("key");
    QTest::addColumn<bool>("overwrite");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<int>("expectedCh");

    QTest::newRow("parenthesis overwrite disabled")
            << "overwrite" << "()" << 1 << ")" << false << "())" << 2;
    QTest::newRow("parenthesis completion disabled")
            << "parenthesis" << "()" << 1 << ")" << true << "())" << 2;
    QTest::newRow("bracket overwrite disabled")
            << "overwrite" << "[]" << 1 << "]" << false << "[]]" << 2;
    QTest::newRow("bracket completion disabled")
            << "bracket" << "[]" << 1 << "]" << true << "[]]" << 2;
    QTest::newRow("brace overwrite disabled")
            << "overwrite" << "{}" << 1 << "}" << false << "{}}" << 2;
    QTest::newRow("brace completion disabled")
            << "brace" << "{}" << 1 << "}" << true << "{}}" << 2;
    QTest::newRow("include overwrite disabled")
            << "overwrite" << "#include <>" << 10 << ">" << false << "#include <>>" << 11;
    QTest::newRow("include completion disabled")
            << "include" << "#include <>" << 10 << ">" << true << "#include <>>" << 11;
    QTest::newRow("semicolon overwrite disabled")
            << "overwrite" << ";" << 0 << ";" << false << ";;" << 1;
    QTest::newRow("comma overwrite disabled")
            << "overwrite" << "," << 0 << "," << false << ",," << 1;
}

void TestEditorSymbolCompletion::test_symbol_skip_settings()
{
    QFETCH(QString, setting);
    QFETCH(QString, initial);
    QFETCH(int, ch);
    QFETCH(QString, key);
    QFETCH(bool, overwrite);
    QFETCH(QString, expected);
    QFETCH(int, expectedCh);

    mEditorSettings->setCompleteSymbols(true);
    mEditorSettings->setCompleteParenthese(true);
    mEditorSettings->setCompleteBracket(true);
    mEditorSettings->setCompleteBrace(true);
    mEditorSettings->setCompleteGlobalInclude(true);
    mEditorSettings->setOverwriteSymbols(overwrite);
    if (setting == "parenthesis")
        mEditorSettings->setCompleteParenthese(false);
    else if (setting == "bracket")
        mEditorSettings->setCompleteBracket(false);
    else if (setting == "brace")
        mEditorSettings->setCompleteBrace(false);
    else if (setting == "include")
        mEditorSettings->setCompleteGlobalInclude(false);
    mEditor->setCppParser(std::make_shared<CppParser>());
    mEditor->setInsertMode(true);
    mEditor->loadContent(initial);
    mEditor->setCaretPos(CharPos{ch, 0});
    QTest::keyPress(mEditor.get(), key.at(0).toLatin1());
    QCOMPARE(mEditor->content(), QStringList{expected});
    QCOMPARE(mEditor->caretPos(), CharPos(expectedCh, 0));
}

void TestEditorSymbolCompletion::test_global_include_completion()
{
    mEditorSettings->setCompleteSymbols(true);
    mEditorSettings->setCompleteGlobalInclude(true);
    mEditorSettings->setOverwriteSymbols(true);
    mEditor->setCppParser(std::make_shared<CppParser>());
    mEditor->setInsertMode(true);
    mEditor->loadContent("#include ");
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), '<');
    QCOMPARE(mEditor->content(), QStringList{"#include <>"});
    QCOMPARE(mEditor->caretPos(), CharPos(10, 0));
    QTest::keyPress(mEditor.get(), '>');
    QCOMPARE(mEditor->content(), QStringList{"#include <>"});
    QCOMPARE(mEditor->caretPos(), CharPos(11, 0));

    mEditor->undo();
    QCOMPARE(mEditor->content(), QStringList{"#include "});
    mEditor->redo();
    QCOMPARE(mEditor->content(), QStringList{"#include <>"});

    mEditor->loadContent("int value");
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), '<');
    QCOMPARE(mEditor->content(), QStringList{"int value<"});

    mEditorSettings->setCompleteGlobalInclude(false);
    mEditor->loadContent("#include ");
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), '<');
    QCOMPARE(mEditor->content(), QStringList{"#include <"});
}

void TestEditorSymbolCompletion::test_remove_symbol_pairs_data()
{
    QTest::addColumn<QString>("initial");
    QTest::addColumn<int>("ch");
    QTest::addColumn<QString>("operation");
    QTest::addColumn<QString>("remaining");

    QTest::newRow("parenthesis") << "()" << 1 << "backspace" << ")";
    QTest::newRow("bracket") << "[]" << 1 << "backspace" << "]";
    QTest::newRow("brace") << "{}" << 1 << "backspace" << "}";
    QTest::newRow("single quote") << "''" << 1 << "backspace" << "'";
    QTest::newRow("double quote") << "\"\"" << 1 << "backspace" << "\"";
    QTest::newRow("global include") << "#include <>" << 10 << "backspace" << "#include >";
    QTest::newRow("delete parenthesis") << "()" << 0 << "delete" << ")";
}

void TestEditorSymbolCompletion::test_remove_symbol_pairs()
{
    QFETCH(QString, initial);
    QFETCH(int, ch);
    QFETCH(QString, operation);
    QFETCH(QString, remaining);

    mEditorSettings->setCompleteSymbols(true);
    mEditorSettings->setCompleteParenthese(true);
    mEditorSettings->setCompleteBracket(true);
    mEditorSettings->setCompleteBrace(true);
    mEditorSettings->setCompleteSingleQuote(true);
    mEditorSettings->setCompleteDoubleQuote(true);
    mEditorSettings->setCompleteGlobalInclude(true);
    mEditor->setCppParser(std::make_shared<CppParser>());
    mEditor->setInsertMode(true);
    for (bool removePair : {false, true}) {
        mEditorSettings->setRemoveSymbolPairs(removePair);
        mEditor->loadContent(initial);
        mEditor->setCaretPos(CharPos{ch, 0});
        if (operation == "backspace")
            QTest::keyPress(mEditor.get(), Qt::Key_Backspace);
        else
            QTest::keyPress(mEditor.get(), Qt::Key_Delete);
        QString expected = removePair
                ? (initial.startsWith("#include") ? QString("#include ") : QString())
                : remaining;
        QVERIFY2(mEditor->content() == QStringList{expected},
                 qPrintable(QString("removePair=%1 actual=%2")
                            .arg(removePair).arg(mEditor->lineText())));
        mEditor->undo();
        QCOMPARE(mEditor->content(), QStringList{initial});
        mEditor->redo();
        QCOMPARE(mEditor->content(), QStringList{expected});
    }
}

void TestEditorSymbolCompletion::test_doxygen_header_editing_areas_data()
{
    addIndentationCases();
}

void TestEditorSymbolCompletion::test_doxygen_header_editing_areas()
{
    QFETCH(bool, autoIndent);
    QFETCH(bool, tabsToSpaces);
    QFETCH(QString, indent);
    configureIndentation(autoIndent, tabsToSpaces);

    const QStringList initial{
        "class Foo {",
        "    ",
        "    int add(int first, int second);",
        "};"
    };
    PCppParser parser = std::make_shared<CppParser>();
    parser->setOnGetFileStream([initial](const QString &, QStringList &buffer, int &) {
        buffer = initial;
        return true;
    });
    CppParser::parseFileBlocking(parser, "text.cpp", false, "");
    QVERIFY(parser->findFunctionAt("text.cpp", 2));
    mEditor->setFilename("text.cpp");
    mEditor->setCppParser(parser);
    mEditor->loadContent(initial);
    mEditor->setCaretPos(CharPos{4, 1});
    QTest::keyPress(mEditor.get(), '/');
    QTest::keyPress(mEditor.get(), '*');
    QTest::keyPress(mEditor.get(), '*');
    QCOMPARE(mEditor->lineText(1), "    /***/");

    QTest::keyPress(mEditor.get(), Qt::Key_Enter);

    const QString generatedIndent = autoIndent ? indent : QString();
    const QString openingIndent = "    ";
    const QString closingLine = autoIndent ? indent + " */" : "*/";
    QCOMPARE(mEditor->content(), QStringList({
        "class Foo {",
        openingIndent + "/**",
        generatedIndent + " * @brief ",
        generatedIndent + " * ",
        generatedIndent + " * @param first ",
        generatedIndent + " * @param second ",
        generatedIndent + " * ",
        generatedIndent + " * @return ",
        closingLine,
        "    int add(int first, int second);",
        "};"
    }));

    const int offset = generatedIndent.length();
    assertEditingArea(2, offset + 10, offset + 10);
    mEditor->tab();
    assertEditingArea(4, offset + 16, offset + 16);
    mEditor->tab();
    assertEditingArea(5, offset + 17, offset + 17);
    mEditor->tab();
    assertEditingArea(7, offset + 11, offset + 11);

    mEditor->tab();
    QSynedit::EditingAreaList areas;
    mEditor->onGetEditingAreas(7, areas);
    QVERIFY(areas.isEmpty());
    QCOMPARE(mEditor->caretPos(), CharPos(offset + 11, 7));
    mEditor->setCppParser(nullptr);
}

void TestEditorSymbolCompletion::test_input_after_cpp_comment()
{
    QStringList text1{
        "//abc",
        "",
    };
    QStringList text2{
        "//abc",
        "{}",
    };
    mEditor->clear();
    mEditor->loadContent(text1);
    mEditor->setCaretPos(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(),'{');
    QCOMPARE(mEditor->content(),text2);

}
