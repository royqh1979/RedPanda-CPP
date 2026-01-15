#include <QTest>
#include "src/settings/codecompletionsettings.h"
#include "src/settings/dirsettings.h"
#include "src/settings/editorsettings.h"
#include "src/colorscheme.h"
#include "src/editor.h"
#include "test_editor.h"
#include <qsynedit/syntaxer/cpp.h>

using QSynedit::CharPos;

TestEditor::TestEditor(QObject *parent):
    QObject{parent}
{
    mSettingsPersistor = std::make_unique<SettingsPersistor>("test-editor.ini");
    mDirSettings = std::make_unique<DirSettings>(mSettingsPersistor.get());
    mEditorSettings = std::make_unique<EditorSettings>(mSettingsPersistor.get());
    mDirSettings->load();
    mEditorSettings->load();
    mColorManager = std::make_unique<ColorManager>(mDirSettings.get());
    mEditor = std::make_unique<Editor>();
    mEditor->setEditorSettings(mEditorSettings.get());
    mEditor->setColorManager(mColorManager.get());
    mEditor->setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    mEditor->applySettings();
}

void TestEditor::test_complete_double_quote1()
{

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
}
