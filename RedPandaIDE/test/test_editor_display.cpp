#include <QTest>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QCursor>
#include <QMimeData>
#include <QTemporaryDir>
#include <algorithm>
#include "test_editor_display.h"
#include "test_parser_utils.h"
#include "src/editor/editor.h"
#include "src/editor/cppexpressionextractor.h"
#include "src/parser/cppparser.h"
#include "src/reformatter/basereformatter.h"
#include <qsynedit/syntaxer/cpp.h>

namespace {

class StaticReformatter : public BaseReformatter
{
public:
    StaticReformatter(const QString &content): mContent(content) { }

    QString refomat(const QString &, QString &, bool &isOk) override
    {
        isOk = true;
        return mContent;
    }

private:
    QString mContent;
};

}

void TestEditorDisplay::test_fold_marks_paint()
{
    mEditor->loadContent(QStringList{
        "int main() {",
        "    int a = 1;",
        "    if (a) {",
        "        a++;",
        "    }",
        "}",
    });
    mEditor->resize(600, 400);
    QImage image = mEditor->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    int h = mEditor->textHeight();
    // the fold button band (right edge of the gutter) at the row of line 0
    // ("int main() {") must contain the fold square/line pixels
    int x0 = mEditor->gutterWidth() - mEditor->gutter().rightOffset();
    QSet<QRgb> colors;
    for (int x = x0; x < x0 + mEditor->gutter().rightOffset() - 2; x++)
        for (int y = 2; y < h - 2; y++)
            colors.insert(image.pixel(x, y));
    QVERIFY(colors.count() >= 2);
}

void TestEditorDisplay::test_reformat_restores_view_state()
{
    QStringList oldLines;
    for (int i = 0; i < 200; i++)
        oldLines << QString("int old_line_%1 = %1;").arg(i);
    mEditor->loadContent(oldLines);
    mEditor->resize(600, 400);
    QSynedit::CharPos savedCaret{5, 55};
    mEditor->setCaretPos(savedCaret);
    mEditor->setTopLineSegment(50, 0);

    QStringList newLines;
    for (int i = 0; i < 160; i++)
        newLines << QString("int reformatted_line_%1 = %1;").arg(i);
    QString newContent = newLines.join('\n');
    mEditor->setGetReformatterFunc([newContent](Editor *) {
        return std::make_unique<StaticReformatter>(newContent);
    });
    mEditor->reformat(true);

    QCOMPARE(mEditor->caretPos(), savedCaret);
    QCOMPARE(mEditor->topLine(), 50);
    QCOMPARE(mEditor->topSegment(), 0);
}

void TestEditorDisplay::test_dnd_undo_restores_caret()
{
    QString sourcePath = QFINDTESTDATA("../../libs/qsynedit/test/resources/editormanager.cpp");
    QVERIFY(!sourcePath.isEmpty());
    QFile sourceFile(sourcePath);
    QVERIFY(sourceFile.open(QIODevice::ReadOnly));
    mEditor->loadContent(QString());
    mEditor->setSelText(QString::fromUtf8(sourceFile.readAll()));
    QStringList originalContent = mEditor->content();
    QSynedit::EditorOptions options = mEditor->getOptions();
    options.setFlag(QSynedit::EditorOption::DragDropEditing, true);
    mEditor->setOptions(options);
    mEditor->resize(600, 400);

    int beginLine = 1145;
    int endLine = 1147;
    QSynedit::CharPos begin{mEditor->lineText(beginLine).indexOf("disconnect"), beginLine};
    QSynedit::CharPos end{mEditor->lineText(endLine).indexOf('}') + 1, endLine};
    auto dragAndUndo = [&](int targetLine, int targetChar, int topLine,
                           QSynedit::CharPos caret, QSynedit::CharPos selectionAnchor) {
        mEditor->setCaretAndSelection(caret, selectionAnchor, caret);
        QVERIFY(mEditor->selText().startsWith("disconnect"));
        QCOMPARE(mEditor->caretPos(), caret);
        QSynedit::CharPos caretBeforeDrag = mEditor->caretPos();
        mEditor->setTopLineSegment(1133, 0);
        QSynedit::CharPos dragClick{begin.ch + 4, begin.line};
        QPoint clickPoint = mEditor->displayCoordToPixels(
                    mEditor->bufferToDisplayPos(dragClick)) + QPoint(2, 2);
        QMouseEvent pressEvent(QEvent::MouseButtonPress, clickPoint, Qt::LeftButton,
                               Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &pressEvent);
        QMimeData mime;
        mime.setText(mEditor->selText());
        QDragEnterEvent enterEvent(QPoint(100, 100), Qt::MoveAction, &mime,
                                   Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &enterEvent);
        QVERIFY(enterEvent.isAccepted());
        mEditor->setTopLineSegment(topLine, 0);
        QPoint pt = mEditor->displayCoordToPixels(
                    mEditor->bufferToDisplayPos(QSynedit::CharPos{targetChar, targetLine}))
                + QPoint(0, 2);
        QCursor::setPos(mEditor->viewport()->mapToGlobal(pt));
        QDragMoveEvent moveEvent(pt, Qt::MoveAction, &mime,
                                 Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &moveEvent);
        QDropEvent dropEvent(pt, Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &dropEvent);
        QVERIFY(dropEvent.isAccepted());
        QVERIFY(mEditor->content() != originalContent);
        mEditor->undo();
        QCoreApplication::processEvents();

        QCOMPARE(mEditor->content(), originalContent);
        QCOMPARE(mEditor->selBegin(), std::min(caret, selectionAnchor));
        QCOMPARE(mEditor->selEnd(), std::max(caret, selectionAnchor));
        QCOMPARE(mEditor->caretPos(), caretBeforeDrag);
        if (caret == begin) {
            QVERIFY(mEditor->lineText(beginLine).left(mEditor->caretCh()).trimmed().isEmpty());
            QVERIFY(mEditor->lineText(beginLine).mid(mEditor->caretCh()).startsWith("disconnect"));
        }
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, clickPoint, Qt::LeftButton,
                                 Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(mEditor->viewport(), &releaseEvent);
    };

    int targetLine = 1139;
    int targetChar = mEditor->lineText(targetLine).indexOf("pMainWindow")
            + QString("pMainWindow").length();
    dragAndUndo(targetLine, targetChar, 1130, begin, end);
    dragAndUndo(1122, mEditor->lineText(1122).indexOf("for"), 1116, begin, end);

    QSynedit::CharPos semicolonEnd{mEditor->lineText(1146).indexOf(';') + 1, 1146};
    targetLine = 1137;
    targetChar = mEditor->lineText(targetLine).indexOf("connect");
    dragAndUndo(targetLine, targetChar, 1133, semicolonEnd, begin);
}


#ifdef REDPANDA_PARSER_STDLIB_TEST
namespace {
class PaintProfileEditor : public Editor
{
public:
    void updateProfileLayout()
    {
        QResizeEvent event(size(), QSize());
        Editor::resizeEvent(&event);
    }
};

struct PaintProfileScenario {
    const char *name;
    QStringList lines;
};

QVector<PaintProfileScenario> paintProfileScenarios()
{
    return {
        {"sequence-algorithms", {
            "#include <bits/stdc++.h>",
            "",
            "int main() {",
            "    std::vector<std::string> names = {\"bob\", \"alice\", \"carol\"};",
            "    std::vector<int> values = {3, 1, 4, 1, 5, 9, 2, 6};",
            "    std::deque<int> deque(values.begin(), values.end());",
            "    std::list<int> linked(values.begin(), values.end());",
            "    std::array<int, 8> array;",
            "    std::copy(values.begin(), values.end(), array.begin());",
            "    std::mt19937 random(42);",
            "    names.push_back(\"dave\");",
            "    names.emplace_back(\"erin\");",
            "    values.insert(values.begin() + 2, 7);",
            "    values.erase(std::remove(values.begin(), values.end(), 1), values.end());",
            "    std::sort(values.begin(), values.end());",
            "    std::stable_sort(names.begin(), names.end());",
            "    auto minimum = std::min_element(values.begin(), values.end());",
            "    auto maximum = std::max_element(values.begin(), values.end());",
            "    auto found = std::find(values.begin(), values.end(), 5);",
            "    auto count = std::count(values.begin(), values.end(), 4);",
            "    auto sum = std::accumulate(values.begin(), values.end(), 0);",
            "    std::reverse(values.begin(), values.end());",
            "    std::rotate(values.begin(), values.begin() + 1, values.end());",
            "    auto lower = std::lower_bound(values.begin(), values.end(), 3);",
            "    bool sorted = std::is_sorted(names.begin(), names.end());",
            "    std::shuffle(values.begin(), values.end(), random);",
            "    std::copy(values.begin(), values.end(), std::ostream_iterator<int>(std::cout, \" \"));",
            "    std::cout << '\\n';",
            "    std::transform(values.begin(), values.end(), values.begin(), [](int value) { return value * 2; });",
            "    for (int value : values) {",
            "        std::cout << value << ' ';",
            "    }",
            "    for (auto it = names.begin(); it != names.end(); ++it) {",
            "        if (it->empty())",
            "            continue;",
            "        std::cout << it->size() << ' ';",
            "    }",
            "    names.front().append(\" suffix\");",
            "    names.back().insert(0, \"prefix \");",
            "    linked.push_front(8);",
            "    linked.pop_back();",
            "    deque.push_front(9);",
            "    deque.erase(deque.begin());",
            "    auto dequeFound = std::find(deque.begin(), deque.end(), 4);",
            "    auto linkedIt = linked.begin();",
            "    std::advance(linkedIt, 1);",
            "    if (linkedIt != linked.end()) {",
            "        std::cout << *linkedIt << ' ';",
            "    }",
            "    std::vector<std::vector<int>> matrix(3, std::vector<int>(3));",
            "    matrix[0][0] = 1;",
            "    matrix.at(1).at(1) = 2;",
            "    matrix.front().push_back(4);",
            "    matrix.back().erase(matrix.back().begin());",
            "    auto flattened = std::accumulate(matrix.begin(), matrix.end(), 0, [](int total, const std::vector<int> &row) {",
            "        return total + std::accumulate(row.begin(), row.end(), 0);",
            "    });",
            "    std::sort(matrix.begin(), matrix.end(), [](const auto &left, const auto &right) {",
            "        return left.size() < right.size();",
            "    });",
            "    auto matrixFound = std::find_if(matrix.begin(), matrix.end(), [](const std::vector<int> &row) {",
            "        return !row.empty();",
            "    });",
            "    std::vector<int> destination;",
            "    auto output = std::back_inserter(destination);",
            "    std::copy_if(values.begin(), values.end(), output, [](int value) {",
            "        return value % 2 == 0;",
            "    });",
            "    destination.erase(destination.begin());",
            "    std::inplace_merge(values.begin(), values.begin() + 2, values.end());",
            "    auto renamed = std::count_if(names.begin(), names.end(), [](const std::string &name) { return name.size() > 3; });",
            "    bool any = std::any_of(values.begin(), values.end(), [](int value) { return value > 10; });",
            "    bool all = std::all_of(values.begin(), values.end(), [](int value) { return value >= 0; });",
            "    auto pair = std::minmax_element(values.begin(), values.end());",
            "    std::cout << *pair.first << *pair.second << *minimum << *maximum << *found << *lower << *dequeFound;",
            "    std::cout << (matrixFound != matrix.end()) << count << sorted << any << all << flattened;",
            "    std::cout << renamed << '\\n';",
            "    std::cout << destination.size() << '\\n';",
            "    std::cout << sum << '\\n';",
            "}"
        }},
        {"associative-strings", {
            "#include <bits/stdc++.h>",
            "",
            "int main() {",
            "    std::map<std::string, int> ages;",
            "    std::unordered_map<std::string, int> counts;",
            "    std::set<std::string> names;",
            "    std::multiset<int> values;",
            "    std::string text = \"the quick brown fox\";",
            "    std::string key = \"alice\";",
            "    std::stringstream stream;",
            "    ages[\"alice\"] = 20;",
            "    ages.insert({\"bob\", 21});",
            "    ages.emplace(\"carol\", 22);",
            "    ++counts[key];",
            "    names.insert(\"alice\");",
            "    names.emplace(\"bob\");",
            "    values.insert(1);",
            "    values.insert(1);",
            "    auto found = ages.find(key);",
            "    if (found != ages.end()) {",
            "        std::cout << found->second << '\\n';",
            "    }",
            "    auto range = ages.equal_range(key);",
            "    auto distance = std::distance(range.first, range.second);",
            "    auto count = counts.count(key);",
            "    auto name = names.find(key);",
            "    bool contains = name != names.end();",
            "    ages.erase(\"bob\");",
            "    counts.erase(key);",
            "    values.erase(1);",
            "    ages.at(\"alice\") += 1;",
            "    auto inserted = ages.insert({\"alice\", 30});",
            "    std::cout << inserted.second << '\\n';",
            "    for (const auto &entry : ages) {",
            "        std::cout << entry.first << entry.second << '\\n';",
            "    }",
            "    for (auto it = counts.begin(); it != counts.end(); ++it) {",
            "        it->second += 2;",
            "    }",
            "    text.append(\" jumps\");",
            "    text.insert(0, \"prefix \");",
            "    text.replace(0, 6, \"the \");",
            "    auto position = text.find(\"quick\");",
            "    auto last = text.rfind('o');",
            "    std::string word = text.substr(position, 5);",
            "    text.erase(0, 4);",
            "    std::transform(text.begin(), text.end(), text.begin(), ::toupper);",
            "    stream << text;",
            "    std::string copied;",
            "    stream >> copied;",
            "    auto textCount = std::count(text.begin(), text.end(), 'O');",
            "    std::sort(word.begin(), word.end());",
            "    auto uniqueEnd = std::unique(word.begin(), word.end());",
            "    word.erase(uniqueEnd, word.end());",
            "    std::string joined = std::accumulate(names.begin(), names.end(), std::string());",
            "    names.erase(names.begin(), std::next(names.begin()));",
            "    auto lower = values.lower_bound(1);",
            "    auto upper = values.upper_bound(1);",
            "    auto valueDistance = std::distance(lower, upper);",
            "    std::map<int, std::string> labels;",
            "    labels.insert_or_assign(1, \"one\");",
            "    labels.try_emplace(2, \"two\");",
            "    auto label = labels.find(1);",
            "    if (label != labels.end()) {",
            "        label->second.append(\"!\");",
            "    }",
            "    auto node = labels.extract(1);",
            "    labels.insert(std::move(node));",
            "    std::unordered_set<std::string> uniqueWords;",
            "    uniqueWords.insert(text);",
            "    uniqueWords.emplace(word);",
            "    uniqueWords.reserve(32);",
            "    auto buckets = uniqueWords.bucket_count();",
            "    auto wordFound = uniqueWords.find(word);",
            "    std::cout << (wordFound != uniqueWords.end()) << copied << contains << count << textCount << last << '\\n';",
            "    std::cout << joined.size() << '\\n';",
            "    std::cout << buckets << '\\n';",
            "    std::cout << valueDistance << '\\n';",
            "    std::cout << distance << '\\n';",
            "}"
        }},
        {"smart-pointers-chrono", {
            "#include <bits/stdc++.h>",
            "",
            "struct Record {",
            "    int value;",
            "    std::string name;",
            "};",
            "",
            "int main() {",
            "    using Clock = std::chrono::steady_clock;",
            "    auto start = Clock::now();",
            "    auto shared = std::make_shared<Record>(Record{1, \"first\"});",
            "    auto unique = std::make_unique<Record>(Record{2, \"second\"});",
            "    std::optional<Record> optional = Record{3, \"third\"};",
            "    std::weak_ptr<Record> weak = shared;",
            "    shared->value += unique->value;",
            "    shared->name.append(unique->name);",
            "    auto locked = weak.lock();",
            "    if (locked) {",
            "        locked->value++;",
            "    }",
            "    std::shared_ptr<Record> copied = shared;",
            "    copied.reset();",
            "    unique->name = optional->name;",
            "    optional->value = shared->value;",
            "    optional.value().name.append(\"!\");",
            "    auto optionalValue = optional.value_or(Record{0, \"empty\"});",
            "    optional.reset();",
            "    bool hasValue = optional.has_value();",
            "    std::vector<std::shared_ptr<Record>> records;",
            "    records.push_back(shared);",
            "    records.emplace_back(std::make_shared<Record>(Record{4, \"fourth\"}));",
            "    records.back()->value = 4;",
            "    for (const auto &record : records) {",
            "        if (record) {",
            "            std::cout << record->name << record->value << '\\n';",
            "        }",
            "    }",
            "    auto found = std::find_if(records.begin(), records.end(), [](const std::shared_ptr<Record> &record) {",
            "        return record->value == 4;",
            "    });",
            "    auto sum = std::accumulate(records.begin(), records.end(), 0, [](int total, const std::shared_ptr<Record> &record) {",
            "        return total + record->value;",
            "    });",
            "    auto elapsed = Clock::now() - start;",
            "    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);",
            "    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);",
            "    std::chrono::milliseconds delay(15);",
            "    auto later = start + delay;",
            "    bool earlier = start < later;",
            "    auto seconds = std::chrono::duration<double>(elapsed);",
            "    using namespace std::chrono_literals;",
            "    auto timeout = 250ms;",
            "    auto totalDelay = delay + timeout;",
            "    auto deadline = Clock::now() + totalDelay;",
            "    auto remaining = deadline - Clock::now();",
            "    auto remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);",
            "    std::this_thread::sleep_for(std::chrono::milliseconds(0));",
            "    std::function<int(const Record &)> reader = [](const Record &record) {",
            "        return record.value;",
            "    };",
            "    int total = reader(*shared) + reader(*unique);",
            "    std::function<void(Record &)> writer = [](Record &record) {",
            "        record.value *= 2;",
            "    };",
            "    writer(*shared);",
            "    std::reference_wrapper<Record> reference = *shared;",
            "    reference.get().name = \"updated\";",
            "    auto sharedText = std::make_shared<std::string>(\"text\");",
            "    auto textSize = sharedText->size();",
            "    auto uniqueText = std::make_unique<std::string>(*sharedText);",
            "    uniqueText->append(\"!\");",
            "    auto movedText = std::move(uniqueText);",
            "    std::cout << movedText->size() << '\\n';",
            "    std::cout << milliseconds.count() << '\\n';",
            "    std::cout << microseconds.count() << '\\n';",
            "    std::cout << seconds.count() << '\\n';",
            "    std::cout << remainingMs.count() << '\\n';",
            "    std::cout << total + textSize + optionalValue.value + (earlier ? 1 : 0) + hasValue + (found != records.end()) << '\\n';",
            "    std::cout << sum << '\\n';",
            "}"
        }}
    };
}

qint64 medianPaintProfileValue(QVector<qint64> values)
{
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}
}
#endif

void TestEditorDisplay::test_stl_paint_profile()
{
#ifndef REDPANDA_PARSER_STDLIB_TEST
    QSKIP("Build with REDPANDA_PARSER_STDLIB_TEST to profile STL painting");
#else
    if (!qEnvironmentVariableIsSet("REDPANDA_STL_PAINT_PROFILE"))
        QSKIP("Set REDPANDA_STL_PAINT_PROFILE=1 to profile STL painting");

    constexpr int profileLines = 80;
    int repeat = qEnvironmentVariableIntValue("REDPANDA_STL_PAINT_PROFILE_REPEAT");
    if (repeat <= 0)
        repeat = 9;

    auto profileEditor = std::make_shared<PaintProfileEditor>();
    mEditor = profileEditor;
    mEditor->setEditorSettings(mEditorSettings.get());
    mEditor->setColorManager(mColorManager.get());
    mEditor->setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    PCppParser parser = std::make_shared<CppParser>();
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(parser, bitsStdcpp));
    mEditor->setCppParser(parser);
    QSynedit::EditorOptions options = mEditor->getOptions();
    options.setFlag(QSynedit::EditorOption::WordWrap, false);
    mEditor->setOptions(options);
    QSize profileSize(1600, mEditor->textHeight() * (profileLines + 2));
    mEditor->resize(profileSize);
    mEditor->viewport()->setFixedSize(profileSize);
    profileEditor->updateProfileLayout();

    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    qWarning("scenario,lines,identifiers,extract_ns,eval_ns,semantic_ns,paint_ns,other_ns,line_extract_ns,expression_units");
    for (const PaintProfileScenario &scenario : paintProfileScenarios()) {
        QCOMPARE(scenario.lines.size(), profileLines);
        QString fileName = temporaryDir.filePath(QString("%1.cpp").arg(scenario.name));
        QFile file(fileName);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write(scenario.lines.join('\n').toUtf8()), qint64(scenario.lines.join('\n').toUtf8().size()));
        file.close();
        CppParser::parseFileBlocking(parser, fileName, true, "");

        mEditor->setFilename(fileName);
        mEditor->loadContent(scenario.lines);
        mEditor->setTopLineSegment(0, 0);
        QVERIFY2(mEditor->linesInWindow() >= profileLines,
                 qPrintable(QString("lines=%1 height=%2 textHeight=%3 viewport=%4")
                            .arg(mEditor->linesInWindow()).arg(mEditor->height())
                            .arg(mEditor->textHeight()).arg(mEditor->viewport()->height())));

        QVector<QSynedit::CharPos> positions;
        QSynedit::CppSyntaxer scanner;
        for (int line = 0; line < profileLines; ++line) {
            mEditor->startParseLine(&scanner, line);
            while (!scanner.eol()) {
                if (scanner.getTokenType() == QSynedit::TokenType::Identifier)
                    positions.append({scanner.getTokenPos(), line});
                scanner.next();
            }
        }
        QVERIFY(!positions.isEmpty());

        auto measureSemantic = [&]() {
            qint64 extractedTokens = 0;
            QElapsedTimer extractTimer;
            extractTimer.start();
            for (const QSynedit::CharPos &pos : positions)
                extractedTokens += mEditor->getExpressionAtPosition(pos).size();
            qint64 extractNs = extractTimer.nsecsElapsed();
            Q_UNUSED(extractedTokens)

            QVector<QStringList> expressions;
            expressions.reserve(positions.size());
            for (const QSynedit::CharPos &pos : positions)
                expressions.append(mEditor->getExpressionAtPosition(pos));
            qint64 resolved = 0;
            QElapsedTimer evalTimer;
            evalTimer.start();
            for (int i = 0; i < positions.size(); ++i) {
                PStatement statement = parser->findStatementOf(
                            fileName, expressions[i], positions[i].line);
                while (statement && statement->kind == StatementKind::Alias)
                    statement = parser->findAliasedStatement(statement);
                if (statement)
                    ++resolved;
            }
            qint64 evalNs = evalTimer.nsecsElapsed();
            Q_UNUSED(resolved)
            return QPair<qint64, qint64>(extractNs, evalNs);
        };
        qint64 unitCount = 0;
        auto measureLineExtraction = [&]() {
            unitCount = 0;
            CppExpressionExtractor extractor(mEditor.get(), mEditor->syntaxer());
            QSynedit::PSyntaxer syntaxer = mEditor->syntaxer()->createInstance();
            QElapsedTimer timer;
            timer.start();
            for (int line = 0; line < profileLines; ++line) {
                QVector<CppExpressionInputToken> input;
                mEditor->startParseLine(syntaxer.get(), line);
                while (!syntaxer->eol()) {
                    input.append({syntaxer->getToken(), syntaxer->getTokenType()});
                    syntaxer->next();
                }
                QVector<CppExpressionUnit> units = extractor.extractLine(line, input);
                unitCount += units.size();
            }
            return timer.nsecsElapsed();
        };
        QImage paintImage(profileSize, QImage::Format_ARGB32);
        auto measurePaint = [&]() {
            parser->parseFinished(0, 0);
            QElapsedTimer timer;
            timer.start();
            mEditor->viewport()->render(&paintImage);
            return timer.nsecsElapsed();
        };

        measureSemantic();
        measureLineExtraction();
        measurePaint();
        QVector<qint64> extractSamples;
        QVector<qint64> evalSamples;
        QVector<qint64> lineExtractSamples;
        QVector<qint64> paintSamples;
        for (int i = 0; i < repeat; ++i) {
            QPair<qint64, qint64> semantic = measureSemantic();
            extractSamples.append(semantic.first);
            evalSamples.append(semantic.second);
            lineExtractSamples.append(measureLineExtraction());
            paintSamples.append(measurePaint());
        }
        qint64 extractNs = medianPaintProfileValue(extractSamples);
        qint64 evalNs = medianPaintProfileValue(evalSamples);
        qint64 lineExtractNs = medianPaintProfileValue(lineExtractSamples);
        qint64 paintNs = medianPaintProfileValue(paintSamples);
        qWarning().nospace() << scenario.name << ',' << profileLines << ',' << positions.size()
                             << ',' << extractNs << ',' << evalNs << ',' << (extractNs + evalNs)
                             << ',' << paintNs << ',' << (paintNs - extractNs - evalNs)
                             << ',' << lineExtractNs << ',' << unitCount;
    }
#endif
}
