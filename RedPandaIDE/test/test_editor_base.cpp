#include <QTest>
#include "src/settings/codecompletionsettings.h"
#include "src/settings/dirsettings.h"
#include "src/settings/editorsettings.h"
#include "src/colorscheme.h"
#include "src/editor.h"
#include "test_editor_base.h"
#include <qsynedit/syntaxer/cpp.h>

TestEditorBase::TestEditorBase(QObject *parent):
    QObject{parent}
{
    init_editor();
}

void TestEditorBase::init_editor()
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
