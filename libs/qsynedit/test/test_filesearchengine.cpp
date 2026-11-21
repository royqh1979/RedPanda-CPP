#include <QTest>
#include <QTemporaryDir>
#include <algorithm>
#include "test_filesearchengine.h"

namespace QSynedit {

static QVector<FileSearchMatch> collectMatches(
        FileSearchEngine &engine, const QString &fileName, const QStringList &lines,
        const SearchQuery &query, FileSearchCallbacks callbacks = {})
{
    QVector<FileSearchMatch> matches;
    callbacks.onBatch = [&matches](const QVector<FileSearchMatch> &batch) {
        matches.append(batch);
    };
    if (engine.searchText(fileName, lines, query, callbacks) != matches.size())
        return {};
    return matches;
}

void TestFileSearchEngine::test_search_text_plain_whole_word_and_case()
{
    FileSearchEngine engine;
    auto matches = collectMatches(
                engine, "memory.cpp", {"int intx INT", "// int", "\"int\""},
                {"int", false, false, true});
    QCOMPARE(matches.size(), 4);
    QCOMPARE(matches.at(0).line, 0);
    QCOMPARE(matches.at(0).ch, 0);
    QCOMPARE(matches.at(1).line, 0);
    QCOMPARE(matches.at(1).ch, 9);
    QCOMPARE(matches.at(2).line, 1);
    QCOMPARE(matches.at(2).ch, 3);
    QCOMPARE(matches.at(3).line, 2);
    QCOMPARE(matches.at(3).ch, 1);
    QCOMPARE(matches.at(3).lineText, QString("\"int\""));

    matches = collectMatches(engine, "memory.cpp", {"int INT"}, {"int", false, true, false});
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.at(0).ch, 0);
}

void TestFileSearchEngine::test_search_text_regex_unicode_and_zero_length()
{
    FileSearchEngine engine;
    auto matches = collectMatches(
                engine, "memory.txt", {QString::fromUtf8("变量 非变量")},
                {"变量", true, true, false});
    QCOMPARE(matches.size(), 2);
    QCOMPARE(matches.at(0).ch, 0);
    QCOMPARE(matches.at(1).line, 0);
    QCOMPARE(matches.at(1).ch, 4);

    matches = collectMatches(engine, "memory.txt", {"ab"}, {"x*", true, true, false});
    QCOMPARE(matches.size(), 3);
    QCOMPARE(matches.at(0).length, 0);
    QCOMPARE(matches.at(2).ch, 2);
}

void TestFileSearchEngine::test_search_text_batches_and_cancel()
{
    FileSearchEngine engine;
    QStringList lines;
    for (int i = 0; i < 300; ++i)
        lines.append("a");
    QVector<int> batchSizes;
    int progressCalls = 0;
    FileSearchCallbacks callbacks;
    callbacks.onBatch = [&batchSizes](const QVector<FileSearchMatch> &batch) {
        batchSizes.append(batch.size());
    };
    callbacks.onProgress = [&progressCalls](int, int) {
        progressCalls++;
    };
    QCOMPARE(engine.searchText("memory.txt", lines, {"a", false, true, false}, callbacks), 300);
    QCOMPARE(batchSizes, QVector<int>({256, 44}));
    QCOMPARE(progressCalls, 300);

    int collected = 0;
    callbacks.onBatch = [&collected](const QVector<FileSearchMatch> &batch) {
        collected += batch.size();
    };
    callbacks.shouldCancel = [] {
        return true;
    };
    QCOMPARE(engine.searchText("memory.txt", lines, {"a", false, true, false}, callbacks), 0);
    QCOMPARE(collected, 0);
}

void TestFileSearchEngine::test_search_file_encoding_and_errors()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString utf8Path = directory.filePath("utf8.txt");
    QFile file(utf8Path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QByteArray utf8 = QString::fromUtf8("变量\n").toUtf8();
    QVERIFY(file.write(utf8) == utf8.size());
    file.close();

    FileSearchEngine engine;
    QVector<FileSearchMatch> matches;
    FileSearchCallbacks callbacks;
    callbacks.onBatch = [&matches](const QVector<FileSearchMatch> &batch) {
        matches.append(batch);
    };
    QCOMPARE(engine.searchFile(utf8Path, {QString::fromUtf8("变量"), false, true, false}, callbacks), 1);
    QCOMPARE(matches.at(0).lineText, QString::fromUtf8("变量"));
    QCOMPARE(engine.searchFile(directory.filePath("missing.txt"), {"x", false, true, false}, callbacks), 0);

    QString gbkPath = directory.filePath("gbk.txt");
    file.setFileName(gbkPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    TextEncoder encoder("GBK");
    auto gbk = encoder.encode(QString::fromUtf8("变量\n"));
    QVERIFY(gbk.first);
    QVERIFY(file.write(gbk.second) == gbk.second.size());
    file.close();
    matches.clear();
    QCOMPARE(engine.searchFile(gbkPath, {QString::fromUtf8("变量"), false, true, false},
                               callbacks, "GBK"), 1);
    QCOMPARE(matches.at(0).lineText, QString::fromUtf8("变量"));

    QString binaryPath = directory.filePath("binary.txt");
    file.setFileName(binaryPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QByteArray binary("a\0b", 3);
    QVERIFY(file.write(binary) == binary.size());
    file.close();
    QCOMPARE(engine.searchFile(binaryPath, {"a", false, true, false}, callbacks), 0);
}

void TestFileSearchEngine::test_collect_files_filters_recursion_and_cancel()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QDir(directory.path()).mkdir("nested"));
    auto write = [](const QString &path) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write("x") == 1;
    };
    QVERIFY(write(directory.filePath("root.cpp")));
    QVERIFY(write(directory.filePath("root.txt")));
    QVERIFY(write(directory.filePath("nested/child.cpp")));

    FileSearchEngine engine;
    auto files = engine.collectFiles({directory.path(), {"*.cpp"}, false});
    QCOMPARE(files.size(), 1);
    QCOMPARE(files.at(0).fileName(), QString("root.cpp"));
    files = engine.collectFiles({directory.path(), {"*.cpp"}, true});
    QCOMPARE(files.size(), 2);

    int calls = 0;
    files = engine.collectFiles({directory.path(), {"*.cpp"}, true}, {},
                                [&calls] {
        return ++calls >= 1;
    });
    QVERIFY(files.isEmpty());
}

void TestFileSearchEngine::test_profile()
{
    if (qEnvironmentVariable("QSYNEDIT_SEARCH_PROFILE") != "1")
        QSKIP("Set QSYNEDIT_SEARCH_PROFILE=1 to run search profiling");

    FileSearchEngine engine;
    FileSearchCallbacks callbacks;
    callbacks.onBatch = [](const QVector<FileSearchMatch> &) {};
    QStringList lines;
    for (int i = 0; i < 2000; ++i)
        lines.append("int value = value + value;");
    QList<QPair<QString, SearchQuery>> scenarios{
        {"no-match", {"missing", false, true, false}},
        {"few", {"int", false, true, false}},
        {"dense", {"value", false, true, false}},
        {"whole-word", {"value", false, true, true}},
        {"regex", {"\\w+", true, true, false}},
    };
    for (const auto &scenario : scenarios) {
        QList<qint64> samples;
        int matches = 0;
        for (int repeat = 0; repeat < 7; ++repeat) {
            QElapsedTimer timer;
            timer.start();
            matches = engine.searchText("memory.cpp", lines, scenario.second, callbacks);
            samples.append(timer.nsecsElapsed());
        }
        std::sort(samples.begin(), samples.end());
        qInfo().noquote() << QString("SEARCH_PROFILE scenario=%1 ns=%2 matches=%3")
                             .arg(scenario.first).arg(samples.at(3)).arg(matches);
    }

    QStringList longLines{QString("value ").repeated(10000)};
    QElapsedTimer longTimer;
    longTimer.start();
    int longMatches = engine.searchText("long.txt", longLines, {"value", false, true, false}, callbacks);
    qInfo().noquote() << QString("SEARCH_PROFILE scenario=long-line ns=%1 matches=%2")
                         .arg(longTimer.nsecsElapsed()).arg(longMatches);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString contents;
    for (int i = 0; i < 64; ++i)
        contents += "int value = value + value;\n";
    for (int dir = 0; dir < 8; ++dir) {
        QString subdir = QString("dir%1").arg(dir);
        QVERIFY(QDir(directory.path()).mkdir(subdir));
        for (int file = 0; file < 8; ++file) {
            QFile output(directory.filePath(QString("%1/file%2.cpp").arg(subdir).arg(file)));
            QVERIFY(output.open(QIODevice::WriteOnly));
            QByteArray data = contents.toUtf8();
            QVERIFY(output.write(data) == data.size());
        }
    }
    QElapsedTimer enumerateTimer;
    enumerateTimer.start();
    auto files = engine.collectFiles({directory.path(), {"*.cpp"}, true});
    qInfo().noquote() << QString("SEARCH_PROFILE scenario=enumerate ns=%1 files=%2")
                         .arg(enumerateTimer.nsecsElapsed()).arg(files.size());

    QElapsedTimer filesTimer;
    filesTimer.start();
    int fileMatches = 0;
    for (const QFileInfo &file : files)
        fileMatches += engine.searchFile(file.absoluteFilePath(), {"value", false, true, false}, callbacks);
    qInfo().noquote() << QString("SEARCH_PROFILE scenario=read-and-match ns=%1 files=%2 matches=%3")
                         .arg(filesTimer.nsecsElapsed()).arg(files.size()).arg(fileMatches);
}

}
