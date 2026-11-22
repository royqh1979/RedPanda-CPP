#include <QSemaphore>
#include <QTemporaryDir>
#include <QTest>
#include <atomic>
#include <qt_utils/utils.h>
#include "src/editor/editor.h"
#include "src/parser/cppparser.h"
#include "src/settings/codecompletionsettings.h"
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
