#include <QTest>
#include <QTemporaryDir>
#include "test_cppparsertyntax.h"

void TestCppParserSyntax::test_cppreference_declarator_syntax()
{
    parseSyntaxResource(mParser, "declarators.cpp");
    PStatement statement = mParser->findStatement("a");
    QVERIFY(statement);
    QCOMPARE(statement->kind, StatementKind::Variable);
    QCOMPARE(statementType(statement), "int");

    statement = mParser->findStatement("p");
    QVERIFY(statement);
    QCOMPARE(statementType(statement), "int *");

    QVERIFY(mParser->findStatement("f"));
    statement = mParser->findStatement("pf");
    QVERIFY(statement);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);

    for (const char *name : {"S::f", "S::g", "complex_parameters", "complex_defaults",
                             "unnamed_parameters", "fpif", "trailing_noexcept",
                             "printf_like", "variadic_template"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
}

void TestCppParserSyntax::test_cppreference_function_syntax()
{
    parseSyntaxResource(mParser, "functions.cpp");
    PStatement f1 = mParser->findStatement("f1");
    QVERIFY(f1);
    QCOMPARE(statementDeclarationForNavigation(f1, QString()).range.start.line, 9);
    QCOMPARE(f1->definition.range.start.line, 31);
    for (const char *name : {"f0", "f1", "fp03", "fp11", "main", "f2"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
    PStatement deleted = mParser->findStatement("bar");
    QVERIFY(deleted);
    QVERIFY(!statementHasDefinition(deleted));
    QVERIFY(!deleted->declarations.isEmpty());
    QCOMPARE(mParser->findStatement("f0")->kind, StatementKind::Function);
    QCOMPARE(mParser->findStatement("fp03")->kind, StatementKind::Function);
    QCOMPARE(mParser->findStatement("fp11")->kind, StatementKind::Function);
}

void TestCppParserSyntax::test_cppreference_class_syntax()
{
    parseSyntaxResource(mParser, "classes.cpp");
    PStatement d1 = mParser->findStatement("S::d1");
    QVERIFY(d1);
    QCOMPARE(d1->definition.range.start.line, 7);
    for (const char *name : {
             "S::d1", "S::a", "S::d2", "S::f1", "S::d3", "S::d4", "S::f2",
             "S::NORTH", "S::NestedS", "S::NestedS::s", "S::d5", "S::d6",
             "S::value_type", "S::pointer_type", "M::M", "M::operator()",
             "Base::d", "Derived::d", "MemberTemplates::f",
             "MemberTemplates::NestedS", "MemberTemplates::NestedS::s"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
}

void TestCppParserSyntax::test_cppreference_template_syntax()
{
    parseSyntaxResource(mParser, "templates.cpp");
    for (const char *name : {"vec::vector", "A", "A::f", "A<MyClass>", "Ptr",
                             "S::f", "S::NestedS", "S::NestedS::s", "f", "X", "T5"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
}

void TestCppParserSyntax::test_cppreference_enum_syntax()
{
    parseSyntaxResource(mParser, "enums.cpp");
    for (const char *name : {"smallenum", "color", "red", "yellow", "green", "blue",
                             "altitude", "altitude::high", "altitude::low", "d", "e", "f",
                             "E11", "E11::x", "E11::y", "E98::x", "N98::x", "S98::x",
                             "cxx20::long_long_long_name", "cxx20::using_enum_demo"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("green"))->value, "20");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("blue"))->value, "21");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("d"))->value, "0");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("e"))->value, "1");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("f"))->value, "3");
}

void TestCppParserSyntax::test_cppreference_namespace_syntax()
{
    parseSyntaxResource(mParser, "namespaces.cpp");
    for (const char *name : {"Q::V", "Q::V::C", "Q::V::C::m", "Q::V::f", "Q::V::h",
                             "A::B::C", "A::B::C::nested_value", "Lib::Lib_1",
                             "Lib::Lib_1::A", "Lib::Lib_1::g", "Lib::A<MyClass>",
                             "unnamed_value"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
}

void TestCppParserSyntax::test_cppreference_lambda_syntax()
{
    QString file = parseSyntaxResource(mParser, "lambdas.cpp");
    QVERIFY(mParser->findStatement("main"));
    QCOMPARE(mParser->findStatementOf(file, "x", 5)->definition.range.start.line, 5);
    for (const char *name : {"x", "simple", "func1", "func2",
                             "nth_fibonacci", "generic", "y"}) {
        QVERIFY2(mParser->findStatementOf(file, QString::fromLatin1(name), 5), name);
    }
    QCOMPARE(mParser->findStatementOf(file, "func1", 5)->kind, StatementKind::Variable);
    QCOMPARE(mParser->findStatementOf(file, "generic", 5)->kind, StatementKind::Variable);
}

void TestCppParserSyntax::test_cppreference_structured_binding_syntax()
{
    QString file = parseSyntaxResource(mParser, "structured_bindings.cpp");
    QVERIFY(mParser->findStatement("main"));
    QVERIFY(mParser->findStatementOf(file, "myset", 6));
    QCOMPARE(mParser->findStatementOf(file, "myset", 6)->definition.range.start.line, 6);
    QVERIFY(mParser->findStatementOf(file, "iter", 8));
    QVERIFY(mParser->findStatementOf(file, "success", 8));
    QVERIFY(mParser->findStatementOf(file, "BitFields", 12));
    for (const char *name : {"b", "d", "p", "q"}) {
        QVERIFY2(mParser->findStatementOf(file, QString::fromLatin1(name), 18), name);
    }
    QVERIFY(mParser->findStatementOf(file, "s", 26));
}

void TestCppParserSyntax::test_cppreference_range_for_syntax()
{
    QString file = parseSyntaxResource(mParser, "range_for.cpp");
    QVERIFY(mParser->findStatement("main"));
    QVERIFY(mParser->findStatementOf(file, "v", 5));
    QVERIFY(mParser->findStatementOf(file, "cv", 11));
    QVERIFY(mParser->findStatementOf(file, "a", 16));
    QVERIFY(mParser->findStatementOf(file, "i", 7));
    QVERIFY(mParser->findStatementOf(file, "n", 14));
    QVERIFY(mParser->findStatementOf(file, "elem_t", 23));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatementOf(file, "v", 5))->typeInfo.qualifiedNameText,
             "std::vector<int>");
    QCOMPARE(statementType(mParser->findStatementOf(file, "v", 5)), "std::vector<int>");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatementOf(file, "a", 16))->args, "[]");
}

void TestCppParserSyntax::test_cppreference_ranges_syntax()
{
    QString file = parseSyntaxResource(mParser, "ranges.cpp");
    QVERIFY(mParser->findStatement("filter_example"));
    QVERIFY(mParser->findStatement("join_example"));
    QVERIFY(mParser->findStatementOf(file, "even", 9));
    QVERIFY(mParser->findStatementOf(file, "square", 9));
    QVERIFY(mParser->findStatementOf(file, "i", 12));
    QVERIFY(mParser->findStatementOf(file, "bits", 22));
    QVERIFY(mParser->findStatementOf(file, "c", 23));
    QVERIFY(mParser->findStatementOf(file, "vv", 25));
    QVERIFY(mParser->findStatementOf(file, "value", 26));
}
