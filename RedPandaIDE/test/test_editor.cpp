#include <QTest>
#include "src/settings/codecompletionsettings.h"
#include "src/settings/dirsettings.h"
#include "src/settings/editorsettings.h"
#include "src/colorscheme.h"
#include "src/editor.h"
#include "test_editor.h"


TestEditor::TestEditor(QObject *parent):
    QObject{parent}
{

}

void TestEditor::test_complete_double_quote()
{
    SettingsPersistor settingsPersistor("test-editor.ini");
    DirSettings dirSettings(&settingsPersistor);
    EditorSettings editorSettings(&settingsPersistor);
    dirSettings.load();
    editorSettings.load();
    ColorManager colorManager(&dirSettings);

    Editor editor;
    editor.setEditorSettings(&editorSettings);
    editor.setColorManager(&colorManager);
    editor.applySettings();


    QStringList text{
        "",
    };
    QStringList text1{
        "\"\"",
    };
    editor.setContent(text);
    QTest::keyPress(&editor,'"');
    QCOMPARE(editor.content(), text1);

    editor.undo();
    QCOMPARE(editor.content(), text);
}
