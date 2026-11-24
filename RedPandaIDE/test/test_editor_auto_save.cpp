#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include "test_editor_auto_save.h"
#include "src/editormanager.h"
#include "src/settings/editorsettings.h"
#include <qsynedit/syntaxer/cpp.h>
#include <qsynedit/formatter/cppformatter.h>

void TestEditorManagerAutoSave::init()
{
    mEditorSettings->setEnableAutoSave(false);
    mEditorSettings->setAutoSaveInterval(1);
    mEditorSettings->setAutoSaveTarget(astCurrentFile);
    mEditorSettings->setAutoSaveStrategy(assOverwrite);
    mEditorSettings->setEnableEditTempBackup(false);
    mAllowed = true;
    mHasProject = false;
    mNow = QDateTime(QDate(2026, 11, 22), QTime(16, 30, 45));
    mManager = new EditorManager(&mLeftTabs, &mRightTabs, &mSplitter, &mPanel);
    mManager->setAutoSaveEnvironment(
                mEditorSettings.get(),
                [this] { return mAllowed; },
                [this] { return mHasProject; },
                [this] { return mNow; });
}

void TestEditorManagerAutoSave::cleanup()
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
}

Editor *TestEditorManagerAutoSave::addEditor(const QString &filename,
                                             const QString &initial,
                                             QTabWidget *page, bool inProject)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return nullptr;
    file.write(initial.toUtf8());
    file.close();

    Editor *editor = new Editor(page);
    editor->setEditorSettings(mEditorSettings.get());
    editor->setColorManager(mColorManager.get());
    editor->setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor->setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor->applySettings();
    editor->setEditorEncoding(ENCODING_UTF8);
    editor->setFilename(filename);
    editor->setIsNew(false);
    editor->loadContent(initial);
    editor->setModified(false);
    editor->setInProjectQueryFunc([inProject](const Editor *) { return inProject; });
    page->addTab(editor, editor->caption());
    return editor;
}

void TestEditorManagerAutoSave::replaceContent(Editor *editor, const QString &content)
{
    editor->selectAll();
    editor->setSelText(content);
    editor->setCanAutoSave(true);
}

QString TestEditorManagerAutoSave::readFile(const QString &filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    QString content = QString::fromUtf8(file.readAll());
    if (content.endsWith("\r\n"))
        content.chop(2);
    else if (content.endsWith("\n"))
        content.chop(1);
    return content;
}

void TestEditorManagerAutoSave::test_overwrite_current_file()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString filename = tmp.filePath("current.cpp");
    Editor *editor = addEditor(filename, "int original;", &mLeftTabs);
    QVERIFY(editor);
    replaceContent(editor, "int changed;");
    mEditorSettings->setEnableAutoSave(true);
    QSignalSpy finished(mManager, &EditorManager::autoSaveFinished);

    mManager->onAutoSaveTimeout();

    QCOMPARE(readFile(filename), QString("int changed;"));
    QVERIFY(!editor->modified());
    QVERIFY(editor->canAutoSave());
    QCOMPARE(finished.count(), 1);
    QCOMPARE(finished.first().first().toInt(), 1);
}

void TestEditorManagerAutoSave::test_current_and_all_opened_targets()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString firstName = tmp.filePath("first.cpp");
    QString secondName = tmp.filePath("second.cpp");
    Editor *first = addEditor(firstName, "int firstOld;", &mLeftTabs);
    Editor *second = addEditor(secondName, "int secondOld;", &mRightTabs);
    QVERIFY(first);
    QVERIFY(second);
    replaceContent(first, "int firstNew;");
    replaceContent(second, "int secondNew;");
    mEditorSettings->setEnableAutoSave(true);

    mLeftTabs.setCurrentWidget(first);
    mManager->onAutoSaveTimeout();
    QCOMPARE(readFile(firstName), QString("int firstNew;"));
    QCOMPARE(readFile(secondName), QString("int secondOld;"));

    mEditorSettings->setAutoSaveTarget(astAllOpennedFiles);
    mManager->onAutoSaveTimeout();
    QCOMPARE(readFile(firstName), QString("int firstNew;"));
    QCOMPARE(readFile(secondName), QString("int secondNew;"));
}

void TestEditorManagerAutoSave::test_project_target()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString firstName = tmp.filePath("project-first.cpp");
    QString excludedName = tmp.filePath("excluded.cpp");
    QString lastName = tmp.filePath("project-last.cpp");
    Editor *first = addEditor(firstName, "int firstOld;", &mLeftTabs, true);
    Editor *excluded = addEditor(excludedName, "int excludedOld;", &mLeftTabs, false);
    Editor *last = addEditor(lastName, "int lastOld;", &mRightTabs, true);
    QVERIFY(first && excluded && last);
    replaceContent(first, "int firstNew;");
    replaceContent(excluded, "int excludedNew;");
    replaceContent(last, "int lastNew;");
    mEditorSettings->setEnableAutoSave(true);
    mEditorSettings->setAutoSaveTarget(astAllProjectFiles);

    mManager->onAutoSaveTimeout();
    QCOMPARE(readFile(firstName), QString("int firstOld;"));
    QCOMPARE(readFile(excludedName), QString("int excludedOld;"));
    QCOMPARE(readFile(lastName), QString("int lastOld;"));

    mHasProject = true;
    mManager->onAutoSaveTimeout();
    QCOMPARE(readFile(firstName), QString("int firstNew;"));
    QCOMPARE(readFile(excludedName), QString("int excludedOld;"));
    QCOMPARE(readFile(lastName), QString("int lastNew;"));
}

void TestEditorManagerAutoSave::test_timestamp_strategies_data()
{
    QTest::addColumn<int>("strategy");
    QTest::newRow("unix") << static_cast<int>(assAppendUnixTimestamp);
    QTest::newRow("formatted") << static_cast<int>(assAppendFormatedTimeStamp);
}

void TestEditorManagerAutoSave::test_timestamp_strategies()
{
    QFETCH(int, strategy);
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString filename = tmp.filePath("sample.cpp");
    Editor *editor = addEditor(filename, "int original;", &mLeftTabs);
    QVERIFY(editor);
    replaceContent(editor, "int changed;");
    mEditorSettings->setEnableAutoSave(true);
    mEditorSettings->setAutoSaveStrategy(static_cast<AutoSaveStrategy>(strategy));

    mManager->onAutoSaveTimeout();

    QString stamp = strategy == assAppendUnixTimestamp
            ? QString::number(mNow.toSecsSinceEpoch())
            : mNow.toString("yyyy.MM.dd.hh.mm.ss");
    QString copyName = tmp.filePath(QString("sample.%1.cpp").arg(stamp));
    QVERIFY2(QFile::exists(copyName), qPrintable(copyName));
    QCOMPARE(readFile(copyName), QString("int changed;"));
    QCOMPARE(readFile(filename), QString("int original;"));
    QVERIFY(!editor->canAutoSave());
}

void TestEditorManagerAutoSave::test_timer_and_disabled_states()
{
    QVERIFY(!mManager->mAutoSaveTimer.isActive());
    mManager->resetAutoSaveTimer();
    QVERIFY(!mManager->mAutoSaveTimer.isActive());

    mEditorSettings->setEnableAutoSave(true);
    mEditorSettings->setAutoSaveInterval(2);
    QSignalSpy finished(mManager, &EditorManager::autoSaveFinished);
    mManager->resetAutoSaveTimer();
    QVERIFY(mManager->mAutoSaveTimer.isActive());
    QCOMPARE(mManager->mAutoSaveTimer.interval(), 120000);
    QCOMPARE(finished.count(), 1);

    QString filename;
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        filename = tmp.filePath("disabled.cpp");
        Editor *editor = addEditor(filename, "int original;", &mLeftTabs);
        QVERIFY(editor);
        replaceContent(editor, "int changed;");
        mAllowed = false;
        mManager->onAutoSaveTimeout();
        QCOMPARE(readFile(filename), QString("int original;"));
        mAllowed = true;
        mEditorSettings->setEnableAutoSave(false);
        mManager->onAutoSaveTimeout();
        QCOMPARE(readFile(filename), QString("int original;"));
    }
}

void TestEditorManagerAutoSave::test_can_auto_save_false()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString filename = tmp.filePath("unchanged.cpp");
    Editor *editor = addEditor(filename, "int original;", &mLeftTabs);
    QVERIFY(editor);
    replaceContent(editor, "int changed;");
    editor->setCanAutoSave(false);
    mEditorSettings->setEnableAutoSave(true);

    mManager->onAutoSaveTimeout();
    QCOMPARE(readFile(filename), QString("int original;"));
}

void TestEditorManagerAutoSave::test_save_error_signal()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString filename = tmp.filePath("missing/document.cpp");
    Editor *editor = new Editor(&mLeftTabs);
    editor->setEditorSettings(mEditorSettings.get());
    editor->setColorManager(mColorManager.get());
    editor->setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor->setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor->applySettings();
    editor->setEditorEncoding(ENCODING_UTF8);
    editor->setFilename(filename);
    editor->setIsNew(false);
    editor->loadContent("int value;");
    editor->setCanAutoSave(true);
    mLeftTabs.addTab(editor, editor->caption());
    mEditorSettings->setEnableAutoSave(true);
    mEditorSettings->setAutoSaveStrategy(assAppendUnixTimestamp);
    QSignalSpy errors(mManager, &EditorManager::autoSaveError);

    mManager->onAutoSaveTimeout();

    QString expectedFilename = QFileInfo(filename).absoluteDir().filePath(
                QString("document.%1.cpp").arg(mNow.toSecsSinceEpoch()));
    QCOMPARE(errors.count(), 1);
    QCOMPARE(errors.first().at(0).value<Editor *>(), editor);
    QCOMPARE(errors.first().at(1).toString(), expectedFilename);
    QVERIFY(!errors.first().at(2).toString().isEmpty());
}


void TestEditorManagerAutoSave::test_opened_editor_content_version()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString filename = tmp.filePath("version.cpp");
    Editor *editor = addEditor(filename, "int value;", &mLeftTabs);
    QVERIFY(editor);
    int initial = mManager->getVersionOfOpenedEditor(filename);
    QCOMPARE(initial, editor->document()->contentVersion());
    QVERIFY(initial > 0);
    QCOMPARE(mManager->getVersionOfOpenedEditor(tmp.filePath("missing.cpp")), 0);

    replaceContent(editor, "int changed;");
    QCOMPARE(mManager->getVersionOfOpenedEditor(filename),
             editor->document()->contentVersion());
    QVERIFY(mManager->getVersionOfOpenedEditor(filename) > initial);
}
