#include <QFile>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTest>
#include "test_exporters.h"
#include "qsynedit/document.h"
#include "qsynedit/exporter/htmlexporter.h"
#include "qsynedit/exporter/rtfexporter.h"
#include "qsynedit/syntaxer/cpp.h"

using namespace QSynedit;

class FailingDevice : public QIODevice
{
protected:
    qint64 readData(char *, qint64) override
    {
        return -1;
    }
    qint64 writeData(const char *, qint64) override
    {
        return -1;
    }
};

static PDocument makeDocument(const QString &text)
{
    auto document = std::make_shared<Document>();
    document->setSyntaxer(std::make_shared<CppSyntaxer>());
    document->setText(text);
    return document;
}

static std::unique_ptr<HTMLExporter> makeHTMLExporter()
{
    auto exporter = std::make_unique<HTMLExporter>(4, ENCODING_UTF8);
    exporter->setSyntaxer(std::make_shared<CppSyntaxer>());
    return exporter;
}

static QString htmlText(const std::unique_ptr<HTMLExporter> &exporter)
{
    QTextDocument document;
    document.setHtml(exporter->text());
    QString text = document.toPlainText();
    text.replace(QChar::Nbsp, ' ');
    // QTextDocument adds one block-separator space after the closing div.
    if (text.endsWith(' '))
        text.chop(1);
    return text;
}

static QString makeLargeSource(int lineCount)
{
    QString text;
    text.reserve(lineCount * 24);
    for (int i = 0; i < lineCount; ++i) {
        if (i > 0)
            text += '\n';
        text += QStringLiteral("int value%1 = %2; // note").arg(i).arg(i);
    }
    return text;
}

void TestHTMLExporter::test_full_document_and_reserved_chars()
{
    QString source = QString::fromUtf8("int main() {\n\tconst char *s = \"<&>\"; // 注释 ✓\n}");
    auto exporter = makeHTMLExporter();
    auto syntaxer = std::make_shared<CppSyntaxer>();
    syntaxer->keywordAttribute()->setStyles(FontStyle::fsBold);
    exporter->setSyntaxer(syntaxer);
    exporter->setTitle("A&B <test>");
    exporter->exportAll(makeDocument(source));

    QString renderedSource = source;
    renderedSource.replace('\t', QString(4, ' '));
    QCOMPARE(htmlText(exporter), renderedSource);
    QVERIFY(exporter->text().startsWith("<!DOCTYPE html>"));
    QVERIFY(!exporter->text().contains("&nbsp;"));
    QVERIFY(!exporter->text().contains("font: -"));
    QVERIFY(exporter->text().contains("&lt;&amp;&gt;"));
    QVERIFY(exporter->text().contains("<title>A&amp;B &lt;test&gt;</title>"));
    QVERIFY(exporter->text().contains("font-weight: bold"));
}

void TestHTMLExporter::test_ranges_data()
{
    QTest::addColumn<int>("startCh");
    QTest::addColumn<int>("startLine");
    QTest::addColumn<int>("endCh");
    QTest::addColumn<int>("endLine");
    QTest::addColumn<QString>("expected");
    QTest::newRow("single line") << 4 << 0 << 9 << 0 << "first";
    QTest::newRow("token boundaries") << 2 << 0 << 8 << 2 << "t first = 1;\nint second = 2;\n// third";
    QTest::newRow("whole lines") << 0 << 1 << 0 << 2 << "int second = 2;\n";
    QTest::newRow("last token prefix") << 0 << 1 << 3 << 1 << "int";
}

void TestHTMLExporter::test_ranges()
{
    QFETCH(int, startCh);
    QFETCH(int, startLine);
    QFETCH(int, endCh);
    QFETCH(int, endLine);
    QFETCH(QString, expected);
    auto exporter = makeHTMLExporter();
    exporter->exportRange(makeDocument("int first = 1;\nint second = 2;\n// third"),
                         {startCh, startLine}, {endCh, endLine});
    QCOMPARE(htmlText(exporter), expected);
}

void TestHTMLExporter::test_empty_and_reused_exports()
{
    auto exporter = makeHTMLExporter();
    auto document = makeDocument("int value;");
    exporter->exportAll(document);
    QVERIFY(!exporter->text().isEmpty());
    exporter->exportRange(document, {2, 0}, {2, 0});
    QVERIFY(exporter->text().isEmpty());
    exporter->exportRange(document, {5, 0}, {2, 0});
    QVERIFY(exporter->text().isEmpty());
    exporter->exportAll(makeDocument(""));
    QVERIFY(exporter->text().isEmpty());
    exporter->exportAll(nullptr);
    QVERIFY(exporter->text().isEmpty());
}

void TestHTMLExporter::test_fragment_and_file_output()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto exporter = makeHTMLExporter();
    exporter->setCreateHTMLFragment(true);
    exporter->exportAll(makeDocument("int value;"));
    QVERIFY(exporter->text().contains("<!--StartFragment-->"));
    QVERIFY(exporter->text().contains("<!--EndFragment-->"));
    QVERIFY(!exporter->text().contains("<title>"));
    QCOMPARE(exporter->clipboardFormat(), "text/html");

    QString path = directory.filePath("output.html");
    exporter->saveToFile(path);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), exporter->buffer());
    QVERIFY_EXCEPTION_THROWN(exporter->saveToFile(directory.path()), FileError);
}

void TestHTMLExporter::test_write_errors()
{
    auto exporter = makeHTMLExporter();
    exporter->exportAll(makeDocument("int value;"));
    FailingDevice device;
    QVERIFY(device.open(QIODevice::WriteOnly));
    QVERIFY_EXCEPTION_THROWN(exporter->writeToStream(device), FileError);
}

void TestHTMLExporter::test_line_numbers_data()
{
    QTest::addColumn<bool>("recalculate");
    QTest::addColumn<bool>("startFromZero");
    QTest::addColumn<QStringList>("numbers");
    QTest::newRow("original one based") << false << false << QStringList{"2", "3"};
    QTest::newRow("original zero based") << false << true << QStringList{"1", "2"};
    QTest::newRow("recalculated one based") << true << false << QStringList{"1", "2"};
    QTest::newRow("recalculated zero based") << true << true << QStringList{"0", "1"};
}

void TestHTMLExporter::test_line_numbers()
{
    QFETCH(bool, recalculate);
    QFETCH(bool, startFromZero);
    QFETCH(QStringList, numbers);
    auto exporter = makeHTMLExporter();
    exporter->setExportLineNumber(true);
    exporter->setRecalcLineNumber(recalculate);
    exporter->setLineNumberStartFromZero(startFromZero);
    exporter->exportRange(makeDocument("int a;\nint b;\nint c;\nint d;"), {0, 1}, {7, 2});
    for (int i = 0; i < numbers.size(); ++i)
        QVERIFY(exporter->text().contains(QString("<span>%1</span>").arg(numbers.at(i))));
}

void TestHTMLExporter::test_combined_text_decorations()
{
    auto exporter = makeHTMLExporter();
    auto syntaxer = std::make_shared<CppSyntaxer>();
    PTokenAttribute keyword = syntaxer->keywordAttribute();
    keyword->setStyles(FontStyles(FontStyle::fsUnderline | FontStyle::fsStrikeOut));
    exporter->setSyntaxer(syntaxer);
    exporter->exportAll(makeDocument("int value;"));
    QVERIFY(exporter->text().contains("text-decoration: underline line-through"));
}

#ifdef QSYNEDIT_PROFILE_TESTS
void TestHTMLExporter::test_large_document_profile_data()
{
    QTest::addColumn<int>("lineCount");
    QTest::newRow("10000") << 10000;
    QTest::newRow("100000") << 100000;
}

void TestHTMLExporter::test_large_document_profile()
{
    QFETCH(int, lineCount);
    PDocument document = makeDocument(makeLargeSource(lineCount));
    auto exporter = makeHTMLExporter();
    QBENCHMARK {
        exporter->exportAll(document);
    }
    qInfo() << "HTML_EXPORT_PROFILE" << lineCount << exporter->text().size() << exporter->buffer().size();
}
#endif

static std::unique_ptr<RTFExporter> makeRTFExporter()
{
    auto exporter = std::make_unique<RTFExporter>(4, ENCODING_UTF8);
    exporter->setSyntaxer(std::make_shared<CppSyntaxer>());
    return exporter;
}

void TestRTFExporter::test_full_document_and_reserved_chars()
{
    auto exporter = makeRTFExporter();
    exporter->setTitle(R"(A\{B})");
    exporter->exportAll(makeDocument(R"(int main() { char c = '\\'; })"));
    QString rtf = exporter->text();
    QVERIFY(rtf.startsWith("{\\rtf1\\ansi"));
    QVERIFY(rtf.contains(R"({\title A\\\{B\}})"));
    QVERIFY(rtf.contains(R"(\{)"));
    QVERIFY(rtf.contains(R"(\})"));
    QVERIFY(rtf.contains(R"(\\)"));
    QVERIFY(rtf.endsWith("}"));
}

void TestRTFExporter::test_ranges()
{
    auto exporter = makeRTFExporter();
    exporter->exportRange(makeDocument("int first;\nint second;\nint third;"),
                         {4, 0}, {0, 2});
    QString rtf = exporter->text();
    QVERIFY(rtf.contains("first"));
    QVERIFY(rtf.contains("second"));
    QVERIFY(rtf.contains("int"));
    QVERIFY(!rtf.contains("third;"));
    QVERIFY(rtf.contains("\\par "));
}

void TestRTFExporter::test_empty_and_reused_exports()
{
    auto exporter = makeRTFExporter();
    auto document = makeDocument("int value;");
    exporter->exportAll(document);
    QString first = exporter->text();
    exporter->exportAll(document);
    QCOMPARE(exporter->text(), first);
    exporter->exportRange(document, {2, 0}, {2, 0});
    QVERIFY(exporter->text().isEmpty());
    exporter->exportAll(makeDocument(""));
    QVERIFY(exporter->text().isEmpty());
    exporter->exportAll(nullptr);
    QVERIFY(exporter->text().isEmpty());
}

void TestRTFExporter::test_file_output_and_errors()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto exporter = makeRTFExporter();
    exporter->exportAll(makeDocument("int value;"));
    QString path = directory.filePath("output.rtf");
    exporter->saveToFile(path);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), exporter->buffer());
    QVERIFY_EXCEPTION_THROWN(exporter->saveToFile(directory.path()), FileError);
}

void TestRTFExporter::test_styles_and_colors()
{
    auto exporter = makeRTFExporter();
    auto syntaxer = std::make_shared<CppSyntaxer>();
    PTokenAttribute keyword = syntaxer->keywordAttribute();
    keyword->setStyles(FontStyles(FontStyle::fsUnderline | FontStyle::fsStrikeOut));
    keyword->setForeground(QColor(12, 34, 56));
    exporter->setSyntaxer(syntaxer);
    exporter->exportAll(makeDocument("int value;"));
    QVERIFY(exporter->text().contains("\\ul"));
    QVERIFY(exporter->text().contains("\\strike"));
    QVERIFY(exporter->text().contains("\\red12\\green34\\blue56;"));
    QVERIFY(exporter->text().contains("\\cf"));
}

void TestRTFExporter::test_background_not_overridden_by_whitespace()
{
    auto exporter = makeRTFExporter();
    auto syntaxer = std::make_shared<CppSyntaxer>();
    syntaxer->whitespaceAttribute()->setBackground(Qt::red);
    exporter->setBackgroundColor(Qt::white);
    exporter->setUseBackground(true);
    exporter->setSyntaxer(syntaxer);
    exporter->exportAll(makeDocument("int value;"));

    QCOMPARE(exporter->backgroundColor(), QColor(Qt::white));
    QVERIFY(exporter->text().contains("\\cfpat1\\cbpat1"));
}

void TestRTFExporter::test_unicode()
{
    auto exporter = makeRTFExporter();
    QString source = QString::fromUtf8("// 变量✓😀");
    exporter->exportAll(makeDocument(source));
    QVERIFY(exporter->text().contains("\\u21464?\\u-28209?\\u10003?\\u-10179?\\u-8704?"));
    QVERIFY(exporter->text().contains("\\uc1"));
    QVERIFY(!exporter->text().contains("\\deflang"));

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QString path = directory.filePath("unicode.rtf");
    exporter->saveToFile(path);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), exporter->buffer());
}

#ifdef QSYNEDIT_PROFILE_TESTS
void TestRTFExporter::test_large_document_profile_data()
{
    QTest::addColumn<int>("lineCount");
    QTest::newRow("10000") << 10000;
    QTest::newRow("100000") << 100000;
}

void TestRTFExporter::test_large_document_profile()
{
    QFETCH(int, lineCount);
    PDocument document = makeDocument(makeLargeSource(lineCount));
    auto exporter = makeRTFExporter();
    QBENCHMARK {
        exporter->exportAll(document);
    }
    qInfo() << "RTF_EXPORT_PROFILE" << lineCount << exporter->text().size() << exporter->buffer().size();
}
#endif
