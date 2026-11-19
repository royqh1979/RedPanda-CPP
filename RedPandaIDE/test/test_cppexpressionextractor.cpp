#include <QElapsedTimer>
#include <QFile>
#include <QTest>
#include <limits>
#include <qsynedit/document.h>
#include <qsynedit/qsynedit.h>
#include <qsynedit/syntaxer/cpp.h>
#include <qsynedit/syntaxer/textfile.h>
#include "src/editor/cppexpressionextractor.h"
#include "src/editor/editor.h"
#include "test_cppexpressionextractor.h"

namespace {
QStringList extractAtMarker(const QString &markedSource,
                            const QSynedit::PSyntaxer &syntaxer)
{
    QStringList lines = markedSource.split('\n');
    QSynedit::CharPos pos{-1, -1};
    for (int line = 0; line < lines.size(); ++line) {
        int ch = lines[line].indexOf('|');
        if (ch >= 0) {
            pos = QSynedit::CharPos{ch, line};
            lines[line].remove(ch, 1);
            break;
        }
    }
    if (!pos.isValid())
        return {"<missing marker>"};

    QSynedit::QSynEdit editor;
    editor.setSyntaxer(syntaxer);
    editor.document()->setText(lines.join('\n'));
    return CppExpressionExtractor::extract(&editor, editor.syntaxer(), pos);
}

QStringList extractAtMarker(const QString &markedSource)
{
    return extractAtMarker(markedSource, std::make_shared<QSynedit::CppSyntaxer>());
}

QVector<CppExpressionInputToken> readExpressionTokens(QSynedit::QSynEdit &editor, int line)
{
    QSynedit::PSyntaxer syntaxer = editor.syntaxer()->createInstance();
    QVector<CppExpressionInputToken> result;
    editor.startParseLine(syntaxer.get(), line);
    while (!syntaxer->eol()) {
        result.append({syntaxer->getToken(), syntaxer->getTokenType()});
        syntaxer->next();
    }
    return result;
}
}

void TestCppExpressionExtractor::test_extract_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("function name without arguments")
            << "|f(a())" << QStringList{"f"};
    QTest::newRow("argument expression")
            << "f(|a())" << QStringList{"a"};
    QTest::newRow("call result as member receiver")
            << "f(a()).|b()" << QStringList{"f", "(", "a", "(", ")", ")", ".", "b"};
    QTest::newRow("left operand") << "|a() + b()" << QStringList{"a"};
    QTest::newRow("right operand") << "a() + |b()" << QStringList{"b"};
    QTest::newRow("object member")
            << "object.|member()" << QStringList{"object", ".", "member"};
    QTest::newRow("member on next line")
            << "a.\n |b()" << QStringList{"a", ".", "b"};
    QTest::newRow("binary operand on next line")
            << "a()\n + |b()" << QStringList{"b"};
    QTest::newRow("comment") << "// |value" << QStringList{};
    QTest::newRow("string") << "\"|value\"" << QStringList{};
    QTest::newRow("character") << "'|a'" << QStringList{};

    QTest::newRow("namespace member")
            << "std::|cout" << QStringList{"std", "::", "cout"};
    QTest::newRow("global scope member")
            << "::|value" << QStringList{"::", "value"};
    QTest::newRow("namespace itself") << "|std::cout" << QStringList{"std"};
    QTest::newRow("pointer member")
            << "pointer->|member" << QStringList{"pointer", "->", "member"};
    QTest::newRow("pointer to object member")
            << "object.*|member" << QStringList{"object", ".*", "member"};
    QTest::newRow("pointer to pointer member")
            << "pointer->*|member" << QStringList{"pointer", "->*", "member"};
    QTest::newRow("this receiver")
            << "this->|member" << QStringList{"this", "->", "member"};
    QTest::newRow("destructor")
            << "Class::~|Destructor"
            << QStringList{"Class", "::", "~", "Destructor"};
    QTest::newRow("parenthesized receiver")
            << "(*pointer).|member" << QStringList{"(", "*", "pointer", ")", ".", "member"};
    QTest::newRow("new receiver")
            << "new Type().|member"
            << QStringList{"new", "Type", "(", ")", ".", "member"};
    QTest::newRow("constructor receiver")
            << "Type(value).|member"
            << QStringList{"Type", "(", "value", ")", ".", "member"};
    QTest::newRow("brace constructor receiver")
            << "Type{value}.|member"
            << QStringList{"Type", "{", "value", "}", ".", "member"};

    QTest::newRow("template target")
            << "std::|vector<int>"
            << QStringList{"std", "::", "vector", "<", "int", ">"};
    QTest::newRow("template member")
            << "std::vector<int>::|iterator"
            << QStringList{"std", "::", "vector", "<", "int", ">", "::", "iterator"};
    QTest::newRow("nested template member")
            << "std::vector<std::string>::|iterator"
            << QStringList{"std", "::", "vector", "<", "std", "::", "string", ">", "::", "iterator"};
    QTest::newRow("deeply nested template member")
            << "std::vector<std::vector<int>>::|iterator"
            << QStringList{"std", "::", "vector", "<", "std", "::", "vector", "<", "int", ">>", "::", "iterator"};
    QTest::newRow("template call receiver")
            << "foo<int>(a).|b"
            << QStringList{"foo", "<", "int", ">", "(", "a", ")", ".", "b"};
    QTest::newRow("template function without call")
            << "|foo<int>(a)" << QStringList{"foo", "<", "int", ">"};
    QTest::newRow("less than is expression boundary") << "a < |b" << QStringList{"b"};
    QTest::newRow("greater than is expression boundary")
            << "a > foo().|b" << QStringList{"foo", "(", ")", ".", "b"};
    QTest::newRow("static cast receiver")
            << "static_cast<Type>(value).|member"
            << QStringList{"static_cast", "<", "Type", ">", "(", "value", ")", ".", "member"};

    QTest::newRow("nested argument member")
            << "foo(a.|bar())" << QStringList{"a", ".", "bar"};
    QTest::newRow("outer function without arguments") << "|foo(a.bar())" << QStringList{"foo"};
    QTest::newRow("subscript receiver")
            << "a[i].|b" << QStringList{"a", "[", "i", "]", ".", "b"};
    QTest::newRow("subscript argument") << "a[|i].b" << QStringList{"i"};
    QTest::newRow("multiple subscripts")
            << "matrix[i][j].|value"
            << QStringList{"matrix", "[", "i", "]", "[", "j", "]", ".", "value"};
    QTest::newRow("member call arguments excluded")
            << "f(a()).|b(c())" << QStringList{"f", "(", "a", "(", ")", ")", ".", "b"};
    QTest::newRow("member call argument independent") << "f(a()).b(|c())" << QStringList{"c"};

    QTest::newRow("nested binary expression") << "a + b * |c" << QStringList{"c"};
    QTest::newRow("conditional true operand") << "condition ? |x : y" << QStringList{"x"};
    QTest::newRow("conditional false operand") << "condition ? x : |y" << QStringList{"y"};
    QTest::newRow("assignment member")
            << "value = object.|member" << QStringList{"object", ".", "member"};
    QTest::newRow("return member")
            << "return object.|member" << QStringList{"object", ".", "member"};
    QTest::newRow("dereference stays outside")
            << "*pointer.|member" << QStringList{"pointer", ".", "member"};
    QTest::newRow("address stays outside")
            << "&object.|member" << QStringList{"object", ".", "member"};

    QTest::newRow("comment inside receiver")
            << "a./* comment */|b" << QStringList{"a", ".", "b"};
    QTest::newRow("multiline comment inside receiver")
            << "a./* first\n second */\n |b" << QStringList{"a", ".", "b"};
    QTest::newRow("line comment inside receiver")
            << "a.// comment\n |b" << QStringList{"a", ".", "b"};
    QTest::newRow("raw string") << "R\"tag(|value)tag\"" << QStringList{};
    QTest::newRow("string before operand") << "\"literal\" + |value" << QStringList{"value"};
    QTest::newRow("line continuation")
            << "a.\\\n |b" << QStringList{"a", ".", "b"};
    QTest::newRow("multiline receiver")
            << "f(\n a()).|b" << QStringList{"f", "(", "a", "(", ")", ")", ".", "b"};
    QTest::newRow("argument in multiline call") << "f(\n |a())" << QStringList{"a"};
    QTest::newRow("member operator on target line")
            << "object\n .|member" << QStringList{"object", ".", "member"};

    QTest::newRow("lambda receiver with trailing return")
            << "[&](int x) -> Type { return x; }().|member"
            << QStringList{"[", "&", "]", "(", "int", "x", ")", "->", "Type",
                           "{", "return", "x", ";", "}", "(", ")", ".", "member"};
    QTest::newRow("lambda receiver with deduced return")
            << "[&]{ return value; }().|member"
            << QStringList{"[", "&", "]", "{", "return", "value", ";", "}", "(", ")", ".", "member"};
    QTest::newRow("identifier inside lambda body") << "[&]{ return |value; }()" << QStringList{"value"};
    QTest::newRow("multiline lambda receiver")
            << "[&]{\n return value;\n}().|member"
            << QStringList{"[", "&", "]", "{", "return", "value", ";", "}", "(", ")", ".", "member"};
    QTest::newRow("multiline comment target") << "/* first\n |value\n last */" << QStringList{};
    QTest::newRow("multiline raw string target") << "R\"tag(first\n |value\n last)tag\"" << QStringList{};
    QTest::newRow("blank line inside receiver") << "a.\n\n |b" << QStringList{"a", ".", "b"};
    QTest::newRow("comparison inside template member")
            << "foo<(sizeof(int) > 1)>::|type"
            << QStringList{"foo", "<", "(", "sizeof", "(", "int", ")", ">", "1", ")", ">", "::", "type"};
    QTest::newRow("comparison inside template target")
            << "|foo<(sizeof(int) > 1)>(a)"
            << QStringList{"foo", "<", "(", "sizeof", "(", "int", ")", ">", "1", ")", ">"};
    QTest::newRow("repeated call groups")
            << "f()(x).|b" << QStringList{"f", "(", ")", "(", "x", ")", ".", "b"};
    QTest::newRow("call and subscript groups")
            << "factory()[index].|b"
            << QStringList{"factory", "(", ")", "[", "index", "]", ".", "b"};
    QTest::newRow("missing receiver") << ". |member" << QStringList{};
    QTest::newRow("unknown identifier") << "|unknown" << QStringList{"unknown"};
}

void TestCppExpressionExtractor::test_extract()
{
    QFETCH(QString, source);
    QFETCH(QStringList, expected);
    QStringList actual = extractAtMarker(source);
    if (actual != expected)
        qDebug() << "expression mismatch" << source << actual << expected;
    QCOMPARE(actual, expected);
}

void TestCppExpressionExtractor::test_invalid_positions()
{
    QSynedit::QSynEdit editor;
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.document()->setText("value");
    QVERIFY(CppExpressionExtractor::extract(&editor, editor.syntaxer(), {-1, 0}).isEmpty());
    QVERIFY(CppExpressionExtractor::extract(&editor, editor.syntaxer(), {0, -1}).isEmpty());
    QVERIFY(CppExpressionExtractor::extract(&editor, editor.syntaxer(), {0, 1}).isEmpty());
    QVERIFY(CppExpressionExtractor::extract(&editor, editor.syntaxer(), {99, 0}).isEmpty());
    QVERIFY(CppExpressionExtractor::extract(nullptr, editor.syntaxer(), {0, 0}).isEmpty());
    CppExpressionExtractor extractor(&editor, editor.syntaxer());
    QVERIFY(extractor.extractLine(-1, {}).isEmpty());
    QVERIFY(extractor.extractLine(1, {}).isEmpty());
#ifndef QT_DEBUG
    QSynedit::PSyntaxer textSyntaxer = std::make_shared<QSynedit::TextSyntaxer>();
    QVERIFY(CppExpressionExtractor::extract(&editor, textSyntaxer, {0, 0}).isEmpty());
    QVERIFY(CppExpressionExtractor::extract(&editor, QSynedit::PConstSyntaxer{}, {0, 0}).isEmpty());
    CppExpressionExtractor textExtractor(&editor, textSyntaxer);
    QVERIFY(textExtractor.extractLine(0, readExpressionTokens(editor, 0)).isEmpty());
    CppExpressionExtractor emptyExtractor(&editor, QSynedit::PConstSyntaxer{});
    QVERIFY(emptyExtractor.extractLine(0, readExpressionTokens(editor, 0)).isEmpty());
#endif
}

void TestCppExpressionExtractor::test_custom_syntaxer()
{
    auto syntaxer = std::make_shared<QSynedit::CppSyntaxer>();
    syntaxer->setCustomTypeKeywords({"custom"});
    QCOMPARE(extractAtMarker("|custom", syntaxer), QStringList{});
    QCOMPARE(extractAtMarker("|value", syntaxer), QStringList{"value"});
    QSynedit::QSynEdit editor;
    editor.setSyntaxer(syntaxer);
    editor.document()->setText("custom value");
    CppExpressionExtractor extractor(&editor, editor.syntaxer());
    QVector<CppExpressionUnit> units = extractor.extractLine(
                0, readExpressionTokens(editor, 0));
    QCOMPARE(units.size(), 1);
    QCOMPARE(units.first().expression, QStringList{"value"});
}

void TestCppExpressionExtractor::test_session()
{
    QSynedit::QSynEdit editor;
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.document()->setText("f(a()).b()");
    CppExpressionExtractor extractor(&editor, editor.syntaxer());
    QCOMPARE(extractor.extract({0, 0}), QStringList{"f"});
    QCOMPARE(extractor.extract({2, 0}), QStringList{"a"});
    QCOMPARE(extractor.extract({7, 0}),
             QStringList({"f", "(", "a", "(", ")", ")", ".", "b"}));
    QVector<CppExpressionInputToken> input = readExpressionTokens(editor, 0);
    QVector<CppExpressionUnit> first = extractor.extractLine(0, input);
    QVector<CppExpressionUnit> second = extractor.extractLine(0, input);
    QCOMPARE(first.size(), 1);
    QCOMPARE(second.size(), 1);
    QCOMPARE(first.first().expression, second.first().expression);
    QCOMPARE(first.first().inputTokenIndexes.size(), first.first().expression.size());
}

void TestCppExpressionExtractor::test_extract_line_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<int>("line");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("call result member covers complete chain")
            << "f(a()).b()" << 0 << QStringList{"f ( a ( ) ) . b"};
    QTest::newRow("binary operands stay independent")
            << "f(a()) + b()" << 0 << QStringList{"b", "a", "f"};
    QTest::newRow("member chains stay independent")
            << "object.member + other->field"
            << 0 << QStringList{"other -> field", "object . member"};
    QTest::newRow("nested argument member")
            << "foo(a.bar())" << 0 << QStringList{"a . bar", "foo"};
    QTest::newRow("target call argument stays independent")
            << "f(a()).b(c())" << 0 << QStringList{"c", "f ( a ( ) ) . b"};
    QTest::newRow("nested template member")
            << "std::vector<std::string>::iterator it;"
            << 0 << QStringList{"it", "std :: vector < std :: string > :: iterator"};
    QTest::newRow("assignment operands stay independent")
            << "value = object.member;" << 0 << QStringList{"object . member", "value"};
    QTest::newRow("declaration type stays independent")
            << "Type value" << 0 << QStringList{"value", "Type"};
    QTest::newRow("function declaration type stays independent")
            << "Type function()" << 0 << QStringList{"function", "Type"};
    QTest::newRow("subscript receiver covers index")
            << "a[i].b" << 0 << QStringList{"a [ i ] . b"};
    QTest::newRow("multiple subscripts cover indexes")
            << "matrix[i][j].value" << 0 << QStringList{"matrix [ i ] [ j ] . value"};
    QTest::newRow("comment has no expressions") << "// value" << 0 << QStringList{};
    QTest::newRow("string has no expressions")
            << "\"value\" + other" << 0 << QStringList{"other"};
    QTest::newRow("unknown identifier") << "unknown" << 0 << QStringList{"unknown"};
    QTest::newRow("namespace chains")
            << "std::cout << std::endl;"
            << 0 << QStringList{"std :: endl", "std :: cout"};
    QTest::newRow("global scope member")
            << "::value" << 0 << QStringList{":: value"};
    QTest::newRow("new receiver")
            << "new Type().member" << 0 << QStringList{"new Type ( ) . member"};
    QTest::newRow("parenthesized receiver")
            << "(*pointer).member" << 0 << QStringList{"( * pointer ) . member"};
    QTest::newRow("comment inside receiver")
            << "a./* comment */b" << 0 << QStringList{"a . b"};
    QTest::newRow("return member")
            << "return object.member" << 0 << QStringList{"object . member"};
    QTest::newRow("less than operands")
            << "a < b" << 0 << QStringList{"b", "a"};
    QTest::newRow("greater than and call receiver")
            << "a > foo().b" << 0 << QStringList{"foo ( ) . b", "a"};
    QTest::newRow("lambda receiver")
            << "[&]{ return value; }().member"
            << 0 << QStringList{"[ & ] { return value ; } ( ) . member"};
    QTest::newRow("template function and argument")
            << "foo<int>(a)" << 0 << QStringList{"a", "foo < int >"};
    QTest::newRow("template member")
            << "std::vector<int>::iterator"
            << 0 << QStringList{"std :: vector < int > :: iterator"};
    QTest::newRow("call and argument")
            << "obj.method() + argument"
            << 0 << QStringList{"argument", "obj . method"};
    QTest::newRow("multiline receiver and target argument")
            << "f(\n a()).b(c())" << 1 << QStringList{"c", "f ( a ( ) ) . b"};
    QTest::newRow("member on next line") << "a.\n b" << 1 << QStringList{"a . b"};
    QTest::newRow("argument on next line") << "f(\n a())" << 1 << QStringList{"a"};
}

void TestCppExpressionExtractor::test_extract_line()
{
    QFETCH(QString, source);
    QFETCH(int, line);
    QFETCH(QStringList, expected);

    QSynedit::QSynEdit editor;
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.document()->setText(source);
    CppExpressionExtractor extractor(&editor, editor.syntaxer());
    QVector<CppExpressionInputToken> input = readExpressionTokens(editor, line);
    QVector<CppExpressionUnit> units = extractor.extractLine(line, input);
    QCOMPARE(units.size(), expected.size());

    QSet<int> identifiers;
    for (int i = 0; i < input.size(); ++i) {
        if (input[i].type == QSynedit::TokenType::Identifier)
            identifiers.insert(i);
    }
    QSet<int> covered;
    for (int i = 0; i < units.size(); ++i) {
        const CppExpressionUnit &unit = units[i];
        QCOMPARE(unit.expression.join(' '), expected[i]);
        QCOMPARE(unit.inputTokenIndexes.size(), unit.expression.size());
        QCOMPARE(unit.tokenTypes.size(), unit.expression.size());
        QVERIFY(unit.targetTokenIndex >= 0 && unit.targetTokenIndex < input.size());
        QCOMPARE(input[unit.targetTokenIndex].type, QSynedit::TokenType::Identifier);
        int targetCh = -1;
        int tokenIndex = 0;
        QSynedit::PSyntaxer scanner = editor.syntaxer()->createInstance();
        editor.startParseLine(scanner.get(), line);
        while (!scanner->eol()) {
            if (tokenIndex == unit.targetTokenIndex) {
                targetCh = scanner->getTokenPos();
                break;
            }
            ++tokenIndex;
            scanner->next();
        }
        QVERIFY(targetCh >= 0);
        QCOMPARE(unit.expression, extractor.extract({targetCh, line}));
        for (int j = 0; j < unit.inputTokenIndexes.size(); ++j) {
            int inputIndex = unit.inputTokenIndexes[j];
            if (inputIndex < 0)
                continue;
            QVERIFY(inputIndex < input.size());
            QCOMPARE(unit.expression[j], input[inputIndex].text);
            QCOMPARE(unit.tokenTypes[j], input[inputIndex].type);
            if (input[inputIndex].type == QSynedit::TokenType::Identifier) {
                QVERIFY2(!covered.contains(inputIndex),
                         qPrintable(unit.expression.join(' ')));
                covered.insert(inputIndex);
            }
        }
    }
    for (int index : identifiers)
        QVERIFY(covered.contains(index));
}

void TestCppExpressionExtractor::test_display_input()
{
    QSynedit::QSynEdit editor;
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.document()->setText("document.value");
    QVector<CppExpressionInputToken> input = readExpressionTokens(editor, 0);
    input[0].text = "display";
    input[2].text = "preview";

    CppExpressionExtractor extractor(&editor, editor.syntaxer());
    QVector<CppExpressionUnit> units = extractor.extractLine(0, input);
    QCOMPARE(units.size(), 1);
    QCOMPARE(units.first().targetTokenIndex, 2);
    QCOMPARE(units.first().expression, QStringList({"display", ".", "preview"}));
    QCOMPARE(units.first().inputTokenIndexes, QVector<int>({0, 1, 2}));
}

void TestCppExpressionExtractor::test_profile()
{
    QString fileName = qEnvironmentVariable("REDPANDA_EXPRESSION_PROFILE_FILE");
    if (fileName.isEmpty())
        QSKIP("Set REDPANDA_EXPRESSION_PROFILE_FILE to profile expression extraction");
    QFile file(fileName);
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(fileName));

    Editor &editor = *mEditor;
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.document()->setText(QString::fromUtf8(file.readAll()));
    QVector<QSynedit::CharPos> positions;
    QSynedit::CppSyntaxer scanner;
    for (int line = 0; line < editor.lineCount(); ++line) {
        editor.startParseLine(&scanner, line);
        while (!scanner.eol()) {
            if (scanner.getTokenType() == QSynedit::TokenType::Identifier)
                positions.append({scanner.getTokenPos(), line});
            scanner.next();
        }
    }
    QVERIFY(!positions.isEmpty());
    int repeat = qEnvironmentVariableIntValue("REDPANDA_EXPRESSION_PROFILE_REPEAT");
    if (repeat <= 0)
        repeat = 10;

    volatile qint64 newTokenCount = 0;
    volatile qint64 sessionTokenCount = 0;
    volatile qint64 lineTokenCount = 0;
    volatile qint64 lineUnitCount = 0;
    auto runNew = [&]() {
        qint64 count = 0;
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < repeat; ++i) {
            for (const QSynedit::CharPos &pos : positions)
                count += CppExpressionExtractor::extract(&editor, editor.syntaxer(), pos).size();
        }
        newTokenCount = count;
        return timer.nsecsElapsed();
    };
    auto runSession = [&]() {
        qint64 count = 0;
        CppExpressionExtractor extractor(&editor, editor.syntaxer());
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < repeat; ++i) {
            for (const QSynedit::CharPos &pos : positions)
                count += extractor.extract(pos).size();
        }
        sessionTokenCount = count;
        return timer.nsecsElapsed();
    };
    auto runLines = [&]() {
        qint64 tokenCount = 0;
        qint64 unitCount = 0;
        CppExpressionExtractor extractor(&editor, editor.syntaxer());
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < repeat; ++i) {
            for (int line = 0; line < editor.lineCount(); ++line) {
                QVector<CppExpressionUnit> units = extractor.extractLine(
                            line, readExpressionTokens(editor, line));
                unitCount += units.size();
                for (const CppExpressionUnit &unit : units)
                    tokenCount += unit.expression.size();
            }
        }
        lineTokenCount = tokenCount;
        lineUnitCount = unitCount;
        return timer.nsecsElapsed();
    };

    runNew();
    runSession();
    runLines();
    qint64 newNs = std::numeric_limits<qint64>::max();
    qint64 sessionNs = std::numeric_limits<qint64>::max();
    qint64 lineNs = std::numeric_limits<qint64>::max();
    for (int i = 0; i < 3; ++i) {
        newNs = qMin(newNs, runNew());
        sessionNs = qMin(sessionNs, runSession());
        lineNs = qMin(lineNs, runLines());
    }
    qWarning().nospace() << "EXPRESSION_PROFILE file=" << fileName
                         << " identifiers=" << positions.size()
                         << " repeat=" << repeat
                         << " newNs=" << newNs
                         << " sessionNs=" << sessionNs
                         << " lineNs=" << lineNs
                         << " newTokens=" << newTokenCount
                         << " sessionTokens=" << sessionTokenCount
                         << " lineTokens=" << lineTokenCount
                         << " lineUnits=" << lineUnitCount;
    QCOMPARE(newTokenCount, sessionTokenCount);
}
