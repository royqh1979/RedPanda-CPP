#include <QTest>
#include <qsynedit/document.h>
#include <qsynedit/qsynedit.h>
#include "src/editor/cppexpressionextractor.h"
#include "test_cppexpressionextractor.h"

void TestCppExpressionExtractor::test_extract_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QSynedit::CharPos>("pos");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("function name without arguments")
            << "f(a())" << QSynedit::CharPos{0, 0} << QStringList{"f"};
    QTest::newRow("argument expression")
            << "f(a())" << QSynedit::CharPos{2, 0} << QStringList{"a"};
    QTest::newRow("call result as member receiver")
            << "f(a()).b()" << QSynedit::CharPos{7, 0}
            << QStringList{"f", "(", "a", "(", ")", ")", ".", "b"};
    QTest::newRow("left operand")
            << "a() + b()" << QSynedit::CharPos{0, 0} << QStringList{"a"};
    QTest::newRow("right operand")
            << "a() + b()" << QSynedit::CharPos{6, 0} << QStringList{"b"};
    QTest::newRow("object member")
            << "object.member()" << QSynedit::CharPos{7, 0}
            << QStringList{"object", ".", "member"};
    QTest::newRow("member on next line")
            << "a.\n b()" << QSynedit::CharPos{1, 1}
            << QStringList{"a", ".", "b"};
    QTest::newRow("binary operand on next line")
            << "a()\n + b()" << QSynedit::CharPos{3, 1} << QStringList{"b"};
    QTest::newRow("comment") << "// value" << QSynedit::CharPos{3, 0} << QStringList{};
    QTest::newRow("string") << "\"value\"" << QSynedit::CharPos{2, 0} << QStringList{};
}

void TestCppExpressionExtractor::test_extract()
{
    QFETCH(QString, source);
    QFETCH(QSynedit::CharPos, pos);
    QFETCH(QStringList, expected);

    QSynedit::QSynEdit editor;
    editor.document()->setText(source);
    QCOMPARE(CppExpressionExtractor::extract(&editor, pos), expected);
}
