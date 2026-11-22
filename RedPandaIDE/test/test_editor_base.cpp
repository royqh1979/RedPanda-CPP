#include <QTest>
#include "src/settings/codecompletionsettings.h"
#include "src/settings/dirsettings.h"
#include "src/settings/editorsettings.h"
#include "src/colorscheme.h"
#include "src/systemconsts.h"
#include "src/editor/editor.h"
#include "test_editor_base.h"
#include <QLocale>
#include <qt_utils/charsetinfo.h>
#include <qsynedit/syntaxer/cpp.h>
#include <qsynedit/formatter/cppformatter.h>

TestEditorBase::TestEditorBase(QObject *parent):
    QObject{parent}
{
    init_editor();
}

TestEditorBase::~TestEditorBase() = default;

void TestEditorBase::init_editor()
{
    mSettingsPersistor = std::make_unique<SettingsPersistor>("test-editor.ini");
    mDirSettings = std::make_unique<DirSettings>(mSettingsPersistor.get());
    mEditorSettings = std::make_unique<EditorSettings>(mSettingsPersistor.get());
    mDirSettings->load();
    mEditorSettings->load();
    mColorManager = std::make_unique<ColorManager>(mDirSettings.get());
    mSystemConsts = std::make_unique<SystemConsts>();
    pSystemConsts = mSystemConsts.get();
    mCharsetInfoManager = std::make_unique<CharsetInfoManager>(QLocale::system().name());
    pCharsetInfoManager = mCharsetInfoManager.get();
    mEditor = std::make_unique<Editor>();
    mEditor->setEditorSettings(mEditorSettings.get());
    mEditor->setColorManager(mColorManager.get());
    mEditor->setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    mEditor->setFormatter(std::make_shared<QSynedit::CppFormatter>());
    mEditor->applySettings();
    mEditor->setGetCppParserFunc([](Editor *editor) {
        if (editor->syntaxer()->language() != QSynedit::ProgrammingLanguage::CPP)
            return PCppParser();
        return editor->parser();
    });
}
