#include <QTest>
#include <qsynedit/document.h>
#include <qsynedit/qsynedit.h>
#include <qsynedit/syntaxer/cpp.h>
#include <qsynedit/syntaxer/textfile.h>
#include "src/editor/cppexpressionextractor.h"
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
#ifndef QT_DEBUG
    QSynedit::PSyntaxer textSyntaxer = std::make_shared<QSynedit::TextSyntaxer>();
    QVERIFY(CppExpressionExtractor::extract(&editor, textSyntaxer, {0, 0}).isEmpty());
    QVERIFY(CppExpressionExtractor::extract(&editor, QSynedit::PConstSyntaxer{}, {0, 0}).isEmpty());
#endif
}

void TestCppExpressionExtractor::test_custom_syntaxer()
{
    auto syntaxer = std::make_shared<QSynedit::CppSyntaxer>();
    syntaxer->setCustomTypeKeywords({"custom"});
    QCOMPARE(extractAtMarker("|custom", syntaxer), QStringList{});
    QCOMPARE(extractAtMarker("|value", syntaxer), QStringList{"value"});
}
