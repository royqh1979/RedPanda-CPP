#include <QTest>
#include "src/settings/codecompletionsettings.h"
#include "src/settings/dirsettings.h"
#include "src/settings/editorsettings.h"
#include "src/colorscheme.h"
#include "src/editor.h"
#include "test_editor_symbol_completion.h"
#include <qsynedit/syntaxer/cpp.h>

using QSynedit::CharPos;

TestEditorSymbolCompletion::TestEditorSymbolCompletion(QObject *parent):
    TestEditorBase{parent}
{
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
    mEditor->setCaretXY(CharPos{19,0});
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
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
    mEditor->setContent(text);
    mEditor->setCaretXY(mEditor->fileEnd());
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), Qt::Key_Left);
    QTest::keyPress(mEditor.get(), '\'');
    QCOMPARE(mEditor->content(), text1);
    QTest::keyPress(mEditor.get(), 'a');
    QTest::keyPress(mEditor.get(), 'b');
    QCOMPARE(mEditor->content(), text2);
}
