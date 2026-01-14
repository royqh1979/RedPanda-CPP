#include <QTest>
#include "src/settings/codecompletionsettings.h"
#include "src/settings/dirsettings.h"
#include "src/settings/editorsettings.h"
#include "src/colorscheme.h"
#include "src/editor.h"
#include "test_editor.h"

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
    mEditor->applySettings();
}

void TestEditor::test_complete_double_quote()
{

    QStringList text{
        "",
    };
    QStringList text1{
        "\"\"",
    };
    mEditor->setContent(text);
    QTest::keyPress(mEditor.get(),'"');
    QCOMPARE(mEditor->content(), text1);
    QCOMPARE(mEditor->caretXY(),CharPos({1,0}));

    mEditor->undo();
    QCOMPARE(mEditor->content(), text);
}
