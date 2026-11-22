#include <QMimeData>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QTest>
#include <atomic>
#include <qt_utils/utils.h>
#include <qsynedit/syntaxer/textfile.h>
#include "src/editor/editor.h"
#include "src/parser/cppparser.h"
#include "src/settings/codecompletionsettings.h"
#include "src/settings/editorsettings.h"
#include "test_editor_general.h"

void TestEditorGeneral::test_syntax_check_target_editor()
{
    QCOMPARE(mEditor->syntaxCheckTargetEditor(), mEditor.get());

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeLines = [](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(lines.join('\n').toUtf8());
        file.close();
        return true;
    };
    QString contextFile = tmp.filePath("main.cpp");
    QString intermediateFile = tmp.filePath("context.h");
    QString actualFile = tmp.filePath("actual.tcc");
    QVERIFY(writeLines(contextFile, {"#include \"context.h\""}));
    QVERIFY(writeLines(intermediateFile, {"#include \"actual.tcc\""}));
    QVERIFY(writeLines(actualFile, {"int value;"}));

    PCppParser parser = std::make_shared<CppParser>();
    CppParser::parseFileBlocking(parser, actualFile, false, contextFile);
    QVERIFY(parser->isFileParsedInContext(actualFile, contextFile));

    auto contextEditor = std::make_shared<Editor>();
    contextEditor->setFilename(contextFile);
    auto contextProvider = [contextEditor](const QString &fileName) -> Editor * {
        return fileName == contextEditor->filename() ? contextEditor.get() : nullptr;
    };
    mEditor->setFilename(actualFile);
    mEditor->setContextFile(contextFile, false);
    mEditor->setGetOpennedFunc(contextProvider);

    mEditor->setModified(true);
    QCOMPARE(mEditor->syntaxCheckTargetEditor(), mEditor.get());
    mEditor->setModified(false);

    mEditor->setCppParser(std::make_shared<CppParser>());
    QCOMPARE(mEditor->syntaxCheckTargetEditor(), contextEditor.get());

    mEditor->setCppParser(parser);
    QCOMPARE(mEditor->syntaxCheckTargetEditor(), contextEditor.get());

    mEditor->setGetOpennedFunc([](const QString &) -> Editor * { return nullptr; });
    QCOMPARE(mEditor->syntaxCheckTargetEditor(), mEditor.get());

    mEditor->setFilename(tmp.filePath("not-included.tcc"));
    mEditor->setGetOpennedFunc(contextProvider);
    QCOMPARE(mEditor->syntaxCheckTargetEditor(), mEditor.get());

    PCppParser parsingParser = std::make_shared<CppParser>();
    auto entered = std::make_shared<QSemaphore>();
    auto release = std::make_shared<QSemaphore>();
    auto blockFirstRead = std::make_shared<std::atomic_bool>(true);
    parsingParser->setOnGetFileStream([=](const QString &, QStringList &) {
        if (blockFirstRead->exchange(false)) {
            entered->release();
            release->acquire();
        }
        return false;
    });
    CppParser::parseFileNonBlocking(parsingParser, actualFile, false, contextFile);
    entered->acquire();
    {
        auto releaseAction = finally([release] {
            release->release();
        });
        mEditor->setCppParser(parsingParser);
        QCOMPARE(mEditor->syntaxCheckTargetEditor(), contextEditor.get());
    }
    QTRY_VERIFY(!parsingParser->parsing());
}

void TestEditorGeneral::test_hard_define_statement_visibility_and_hint()
{
    PCppParser parser = std::make_shared<CppParser>();
    parser->addHardDefineByLine("#define __cplusplus 201703L");
    parser->parseHardDefines();
    parser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"int value = __cplusplus;"};
        return true;
    });
    CppParser::parseFileBlocking(parser, "main.cpp", false, "");
    PStatement statement = parser->findStatementOf("main.cpp", QStringList{"__cplusplus"}, 0);
    QVERIFY(statement);
    QCOMPARE(statement->kind, StatementKind::Preprocessor);

    mEditor->setFilename("main.cpp");
    mEditor->setCppParser(parser);
    QString hint = mEditor->getParserHint({"__cplusplus"}, {0, 12});
    QVERIFY(!hint.isEmpty());
}

void TestEditorGeneral::test_predefined_func_hint()
{
    PCppParser parser = std::make_shared<CppParser>();
    parser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"void alpha() {", "}", "void declarationOnly();"};
        return true;
    });
    CppParser::parseFileBlocking(parser, "main.cpp", false, "");
    mEditor->setFilename("main.cpp");
    mEditor->setCppParser(parser);
    QString hint = mEditor->getParserHint({"__func__"}, {0, 1});
    QVERIFY(hint.contains("\"alpha\""));
    QVERIFY(hint.contains("const char"));
}

void TestEditorGeneral::test_hide_stops_parsing_and_allows_reparse_after_show()
{
    CodeCompletionSettings completionSettings(mSettingsPersistor.get());
    completionSettings.setEnabled(true);
    mEditor->setCodeCompletionSettings(&completionSettings);
    mEditor->applySettings();
    mEditor->setFilename("text.cpp");
    mEditor->setContextFile("", false);

    PCppParser parser = std::make_shared<CppParser>();
    auto entered = std::make_shared<QSemaphore>();
    auto release = std::make_shared<QSemaphore>();
    auto readCount = std::make_shared<std::atomic_int>(0);
    parser->setOnGetFileStream([=](const QString &, QStringList &buffer) {
        if (readCount->fetch_add(1) == 0) {
            entered->release();
            release->acquire();
        }
        buffer = QStringList{"int value;"};
        return true;
    });
    mEditor->setCppParser(parser);

    mEditor->show();
    mEditor->reparse();
    QVERIFY(entered->tryAcquire(1, 5000));
    mEditor->hide();
    release->release();
    QTRY_VERIFY(!parser->parsing());
    QCOMPARE(parser->statementList().count(), 0);
    QVERIFY(!parser->findFileInfo("text.cpp"));

    mEditor->show();
    mEditor->reparse();
    QTRY_COMPARE(readCount->load(), 2);
    QTRY_VERIFY(!parser->parsing());
    QVERIFY(parser->findStatement("value"));
    mEditor->hide();
}


void TestEditorGeneral::test_auto_backup_lifecycle()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString filename = tmp.filePath("document.cpp");
    mEditor->setFilename(filename);
    mEditor->loadContent("int original;");
    mEditor->setIsNew(false);
    mEditorSettings->setEnableEditTempBackup(true);
    mEditor->setAutoBackupEnabled(true);
    mEditor->initAutoBackup();

    QVERIFY(mEditor->mBackupFile != nullptr);
    QVERIFY(mEditor->mBackupFile->isOpen());
    QVERIFY(mEditor->mAutoBackupTimer.isActive());
    QString backupName = mEditor->mBackupFile->fileName();
    QVERIFY(backupName.startsWith(filename + "."));
    QVERIFY(backupName.endsWith(".editbackup"));
    QFile backup(backupName);
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(backup.readAll()), QString("int original;"));
    backup.close();

    mEditor->setAutoBackupEnabled(true);
    mEditor->selectAll();
    mEditor->setSelText("int changed;");
    mEditor->saveAutoBackup();
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(backup.readAll()), QString("int changed;"));
    backup.close();

    mEditor->save();
    QVERIFY(mEditor->mBackupFile == nullptr);
    QVERIFY(!QFile::exists(backupName));
    QVERIFY(!mEditor->mAutoBackupTimer.isActive());
}

void TestEditorGeneral::test_auto_backup_disabled_and_read_only()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    mEditor->setFilename(tmp.filePath("document.cpp"));
    mEditor->loadContent("int value;");

    mEditor->setAutoBackupEnabled(false);
    mEditorSettings->setEnableEditTempBackup(false);
    mEditor->initAutoBackup();
    QVERIFY(mEditor->mBackupFile == nullptr);
    QVERIFY(!mEditor->mAutoBackupTimer.isActive());

    mEditorSettings->setEnableEditTempBackup(true);
    mEditor->setReadOnly(true);
    mEditor->initAutoBackup();
    QVERIFY(mEditor->mBackupFile == nullptr);
    QVERIFY(!mEditor->mAutoBackupTimer.isActive());

    mEditor->setReadOnly(false);
    mEditor->setAutoBackupEnabled(true);
    mEditor->initAutoBackup();
    QVERIFY(mEditor->mBackupFile != nullptr);
    QVERIFY(mEditor->mAutoBackupTimer.isActive());
    mEditor->cleanAutoBackup();
}

void TestEditorGeneral::test_auto_backup_throttles_recent_changes()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString filename = tmp.filePath("document.cpp");
    mEditor->setFilename(filename);
    mEditor->loadContent("int original;");
    mEditorSettings->setEnableEditTempBackup(true);
    mEditor->setAutoBackupEnabled(true);
    mEditor->initAutoBackup();
    QVERIFY(mEditor->mBackupFile != nullptr);
    QString backupName = mEditor->mBackupFile->fileName();
    mEditor->setAutoBackupEnabled(true);

    mEditor->selectAll();
    mEditor->setSelText("int recentChange;");
    mEditor->onAutoBackupTimer();

    QFile backup(backupName);
    QVERIFY(backup.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(backup.readAll()), QString("int original;"));
    backup.close();
    mEditor->cleanAutoBackup();
    QVERIFY(!QFile::exists(backupName));
}

void TestEditorGeneral::test_copy_mime_data_selection()
{
    mEditor->setFilename("sample.cpp");
    mEditorSettings->setCopyHTMLUseEditorColor(true);
    mEditorSettings->setCopyHTMLWithLineNumber(true);
    mEditorSettings->setCopyHTMLRecalcLineNumber(true);
    mEditorSettings->setGutterLineNumbersStartZero(false);
    mEditor->loadContent("int value;\n// next");
    mEditor->setSelBeginEnd({4, 0}, {7, 1});

    std::unique_ptr<QMimeData> mimeData = mEditor->createCopyMimeData();
    QVERIFY(mimeData);
    QCOMPARE(mimeData->text(), mEditor->selText());
    QVERIFY(mimeData->hasFormat("text/html"));
    QString html = QString::fromUtf8(mimeData->data("text/html"));
    QVERIFY(html.contains("<!--StartFragment-->"));
    QVERIFY(html.contains("<!--EndFragment-->"));
    QVERIFY(html.contains("value"));
    QVERIFY(html.contains("next"));
    QVERIFY(html.contains("<span>1</span>"));
    QVERIFY(!html.contains("<title>"));
}

void TestEditorGeneral::test_copy_mime_data_plain_text()
{
    mEditor->setFileType(FileType::Text, false);
    QVERIFY(!mEditor->parser());
    mEditor->loadContent("plain <&>");
    mEditor->setSelBeginEnd({0, 0}, {9, 0});

    std::unique_ptr<QMimeData> mimeData = mEditor->createCopyMimeData();
    QVERIFY(mimeData);
    QCOMPARE(mimeData->text(), "plain <&>");
    QVERIFY(!mimeData->hasFormat("text/html"));
}

void TestEditorGeneral::test_export_files()
{
    mEditor->setFilename("input.cpp");
    mEditor->loadContent("int first;\nint second;");
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString htmlPath = directory.filePath("output.html");
    QString rtfPath = directory.filePath("output.rtf");
    mEditorSettings->setCopyRTFUseEditorColor(false);
    mEditorSettings->setCopyRTFBackgroundColor(QColor(240, 235, 220));
    mEditor->exportAsHTML(htmlPath);
    mEditor->exportAsRTF(rtfPath);

    QFile htmlFile(htmlPath);
    QVERIFY(htmlFile.open(QIODevice::ReadOnly));
    QByteArray html = htmlFile.readAll();
    QVERIFY(html.contains("int"));
    QVERIFY(html.contains("second"));
    QVERIFY(html.contains("<title>output.html</title>"));

    QFile rtfFile(rtfPath);
    QVERIFY(rtfFile.open(QIODevice::ReadOnly));
    QByteArray rtf = rtfFile.readAll();
    QVERIFY(rtf.startsWith("{\\rtf1\\ansi"));
    QVERIFY(rtf.contains("second"));
    QVERIFY(rtf.contains("{\\title output.rtf}"));
    QVERIFY(rtf.contains("\\red240\\green235\\blue220;"));
}
