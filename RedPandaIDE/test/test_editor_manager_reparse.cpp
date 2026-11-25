#include <QSignalSpy>
#include <QTest>
#include "test_editor_manager_reparse.h"
#include "src/editormanager.h"
#include "src/settings/codecompletionsettings.h"
#include <qsynedit/syntaxer/cpp.h>
#include <qsynedit/formatter/cppformatter.h>

void TestEditorManagerReparse::init()
{
    mCompletionSettings = new CodeCompletionSettings(mSettingsPersistor.get());
    mCompletionSettings->setEnabled(true);
    mManager = new EditorManager(&mLeftTabs, &mRightTabs, &mSplitter, &mPanel);
}

void TestEditorManagerReparse::cleanup()
{
    auto clearTabs = [](QTabWidget &tabs) {
        while (tabs.count() > 0) {
            QWidget *widget = tabs.widget(0);
            tabs.removeTab(0);
            delete widget;
        }
    };
    clearTabs(mLeftTabs);
    clearTabs(mRightTabs);
    delete mManager;
    mManager = nullptr;
    delete mCompletionSettings;
    mCompletionSettings = nullptr;
}

Editor *TestEditorManagerReparse::addEditor(const QString &fileName, const QStringList &content,
                                            QTabWidget *page, const PCppParser &parser)
{
    Editor *editor = new Editor(page);
    editor->setEditorSettings(mEditorSettings.get());
    editor->setCodeCompletionSettings(mCompletionSettings);
    editor->setColorManager(mColorManager.get());
    editor->setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor->setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor->applySettings();
    editor->setFilename(fileName);
    editor->setFileType(FileType::CppSource, false);
    editor->setInProjectQueryFunc([](const Editor *) { return false; });
    editor->loadContent(content);
    editor->setCppParser(parser);
    page->addTab(editor, editor->caption());
    return editor;
}

void TestEditorManagerReparse::showAndWait(Editor *editor, const PCppParser &parser,
                                           QSignalSpy &finished, int expectedCount,
                                           const QString &symbol)
{
    editor->show();
    mManager->reparseEditor(editor);
    QTRY_COMPARE(finished.count(), expectedCount);
    QVERIFY(parser->findStatementOf(editor->filename(), symbol, 0));
}

void TestEditorManagerReparse::test_shared_parser_switches_editors()
{
    PCppParser parser = mManager->sharedParser(ParserLanguage::CPlusPlus, &mLeftTabs);
    QSignalSpy finished(parser.get(), &CppParser::parseFinished);
    Editor *a = addEditor("shared-a.cpp", {"int a_value;"}, &mLeftTabs, parser);
    Editor *b = addEditor("shared-b.cpp", {"int b_value;"}, &mLeftTabs, parser);
    QCOMPARE(a->parser(), b->parser());

    showAndWait(a, parser, finished, 1, "a_value");
    a->hide();
    showAndWait(b, parser, finished, 2, "b_value");
    QVERIFY(!parser->findStatementOf(b->filename(), "a_value", 0));
    b->hide();
    showAndWait(a, parser, finished, 3, "a_value");
    QVERIFY(!parser->findStatementOf(a->filename(), "b_value", 0));
    a->hide();
}

void TestEditorManagerReparse::test_split_panels_use_their_own_parsers()
{
    PCppParser leftParser = mManager->sharedParser(ParserLanguage::CPlusPlus, &mLeftTabs);
    PCppParser rightParser = mManager->sharedParser(ParserLanguage::CPlusPlus, &mRightTabs);
    QVERIFY(leftParser != rightParser);
    QSignalSpy leftFinished(leftParser.get(), &CppParser::parseFinished);
    QSignalSpy rightFinished(rightParser.get(), &CppParser::parseFinished);
    Editor *left = addEditor("left.cpp", {"int left_value;"}, &mLeftTabs, leftParser);
    Editor *right = addEditor("right.cpp", {"int right_value;"}, &mRightTabs, rightParser);

    showAndWait(left, leftParser, leftFinished, 1, "left_value");
    QCOMPARE(rightFinished.count(), 0);
    left->hide();
    showAndWait(right, rightParser, rightFinished, 1, "right_value");
    QCOMPARE(leftFinished.count(), 1);
    right->hide();
    showAndWait(left, leftParser, leftFinished, 2, "left_value");
    QCOMPARE(rightFinished.count(), 1);
    left->hide();
}

void TestEditorManagerReparse::test_batch_update_reparses_only_final_editor()
{
    PCppParser parser = mManager->sharedParser(ParserLanguage::CPlusPlus, &mLeftTabs);
    QSignalSpy finished(parser.get(), &CppParser::parseFinished);
    Editor *first = addEditor("batch-first.cpp", {"int first_value;"}, &mLeftTabs, parser);
    Editor *last = addEditor("batch-last.cpp", {"int last_value;"}, &mLeftTabs, parser);

    mManager->beginUpdate();
    first->show();
    first->hide();
    last->show();
    QCoreApplication::processEvents();
    QCOMPARE(finished.count(), 0);
    mLeftTabs.setCurrentWidget(last);
    mManager->endUpdate();
    mManager->reparseEditor(mManager->getEditor());

    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(parser->findStatementOf(last->filename(), "last_value", 0));
    QVERIFY(!parser->findStatementOf(last->filename(), "first_value", 0));
    last->hide();
}
