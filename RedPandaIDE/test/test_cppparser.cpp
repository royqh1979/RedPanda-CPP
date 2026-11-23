#include <QTest>
#include <QSemaphore>
#include <QTemporaryDir>
#include <qt_utils/utils.h>
#include "test_cppparser.h"
#include "src/parser/cpptyperesolver.h"

void TestCppParser::test_parse_var()
{
    mParser->setOnGetFileStream([](const QString& filename, QStringList& buffer){
        buffer=QStringList({
                               "int xxx=10;"
                           });
           return true;
                                });
    CppParser::parseFileBlocking(mParser,"text.cpp",false,"");
    QCOMPARE(mParser->statementList().count(),1);
    PStatement statement = mParser->findStatement("xxx");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"xxx");
    QCOMPARE(statement->kind,StatementKind::Variable);

}

void TestCppParser::test_global_variable_reference_scope()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "int ttt;",
            "",
            "int main() {",
            "    return ttt;",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    QCOMPARE(mParser->statementList().count(), 3);
    QVERIFY(!mParser->findStatement("main::ttt"));
    PStatement declaration = mParser->findStatement("ttt");
    QVERIFY(declaration);
    QVERIFY(statementHasDefinition(declaration));
    QCOMPARE(declaration->definition.fileName, "text.cpp");
    QVERIFY(declaration->declarations.isEmpty());
    PStatement reference = mParser->findStatementOf("text.cpp", QStringList{"ttt"}, 3);
    QVERIFY(reference);
    QCOMPARE(reference, declaration);
    QCOMPARE(reference->scope, StatementScope::Global);
    QCOMPARE(getKindOfStatement(reference), StatementKind::GlobalVariable);
}

void TestCppParser::test_include_visibility()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString header = dir.filePath("project.h");
    QString main = dir.filePath("main.cpp");
    QVERIFY(writeLines(header, {"class Bar {};"}));
    QVERIFY(writeLines(main, {"#include \"project.h\"", "Bar value;"}));
    CppParser::parseFileBlocking(mParser, main, false, "");

    QVERIFY(mParser->getIncludedFiles(main).contains(header));
    PStatement statement = mParser->findStatementOf(main, QStringList{"Bar"}, 1);
    QVERIFY(statement);
    QCOMPARE(statement->fullName, "Bar");
    QCOMPARE(statementDefinitionForNavigation(statement).fileName, header);
    QCOMPARE(statementDefinitionForNavigation(statement).range.start.line, 0);
    QCOMPARE(statementDefinitionForNavigation(statement).range.start.ch, 0);
}

void TestCppParser::test_class_forward_declaration_merges_with_definition()
{
    for (bool forwardFirst : {true, false}) {
        init_parser();
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString declaration = dir.filePath("declare.h");
        QString otherDeclaration = dir.filePath("declare_other.h");
        QString definition = dir.filePath("define.h");
        QString main = dir.filePath("main.cpp");
        QVERIFY(writeLines(declaration, {"class Foo;"}));
        QVERIFY(writeLines(otherDeclaration, {"class Foo;"}));
        QVERIFY(writeLines(definition, {"class Foo {};"}));
        QStringList includes = forwardFirst
                ? QStringList{"#include \"declare.h\"", "#include \"declare_other.h\"",
                              "#include \"define.h\""}
                : QStringList{"#include \"define.h\"", "#include \"declare.h\"",
                              "#include \"declare_other.h\""};
        QVERIFY(writeLines(main, includes));
        CppParser::parseFileBlocking(mParser, main, false, "");

        QCOMPARE(mParser->statementList().childrenStatements().values("Foo").size(), 1);
        PStatement statement = mParser->findStatement("Foo");
        QVERIFY(statement && statementHasDefinition(statement));
        QCOMPARE(statement->definition.fileName, definition);
        QCOMPARE(statement->declarations.count(), 2);
        QCOMPARE(mParser->findTypeDefinitionOf(main, "Foo", {}), statement);
    }
}

void TestCppParser::test_include_global_variable_reference()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString header = dir.filePath("project.h");
    QString main = dir.filePath("main.cpp");
    QVERIFY(writeLines(header, {"int shared;"}));
    QVERIFY(writeLines(main, {"#include \"project.h\"", "void f(){shared=1;}"}));
    CppParser::parseFileBlocking(mParser, main, false, "");

    PStatement statement = mParser->findStatementOf(main, QStringList{"shared"}, 1);
    QVERIFY(statement);
    QCOMPARE(statement->fullName, "shared");
    QCOMPARE(statementDefinitionForNavigation(statement).fileName, header);
}

void TestCppParser::test_include_inline_function_call()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString header = dir.filePath("project.h");
    QString main = dir.filePath("main.cpp");
    QVERIFY(writeLines(header, {"inline void inlineFunction() {}"}));
    QVERIFY(writeLines(main, {"#include \"project.h\"", "void f(){inlineFunction();}"}));
    CppParser::parseFileBlocking(mParser, main, false, "");

    PStatement declaration = mParser->findStatement("inlineFunction");
    QVERIFY(declaration);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(declaration)->typeInfo.qualifiedName,
             "void");
    QVERIFY(!mParser->findStatement("f::inlineFunction"));
    QVERIFY(mParser->findStatementOf(main, QStringList{"inlineFunction"}, 1));
    CppTypeResolver resolver(mParser->statementList(), nullptr, CppEvaluationOptions{});
    CallResolution invocation;
    CppTypeResolver::EvaluationContext evaluation;
    TypeInfo returnType = resolver.invokeFunction(
                declaration, {}, {}, &evaluation, nullptr, {}, &invocation);
    QVERIFY(invocation.statement);
    QCOMPARE(returnType.qualifiedName, "void");
    QStringList call{"inlineFunction", "(", ")"};
    ExpressionEvalTrace trace;
    EvalResult eval = mParser->evalExpression(
                main, call, mParser->findScopeStatement(main, 1), &trace);
    QCOMPARE(trace.size(), 1);
    QVERIFY(trace.constFirst().statement);
    QVERIFY(eval.statement);
    QCOMPARE(eval.statement->fullName, "inlineFunction");
    PStatement statement = mParser->findStatementOf(
                main, QStringList{"inlineFunction", "(", ")"}, 1);
    QVERIFY(statement);
    QCOMPARE(statement->fullName, "inlineFunction");
    QCOMPARE(statementDefinitionForNavigation(statement).fileName, header);
}

void TestCppParser::test_local_extern_variable_uses_external_statement()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"int external;", "void f(){extern int external; external=1;}"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement external = mParser->findStatement("external");
    QVERIFY(external);
    PStatement localDeclaration = mParser->findStatementOf(
                "text.cpp", QStringList{"external"}, 1);
    QCOMPARE(localDeclaration, external);
    QCOMPARE(localDeclaration->scope, StatementScope::Global);
    QVERIFY(localDeclaration->declarations.contains("text.cpp"));
    QVERIFY(!mParser->findStatement("f::external"));
}

void TestCppParser::test_static_extern_variable_definitions()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "int global=1;",
            "static int internal=2;",
            "extern int declaration;",
            "extern int defined=3;",
            "struct C {static int member; inline static int inlineMember;};",
            "void f(){static int local=4; extern int declaration;}",
            "extern int braceDefined{};",
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement global = mParser->findStatement("global");
    QVERIFY(global && statementHasDefinition(global));
    QCOMPARE(global->scope, StatementScope::Global);
    QVERIFY(!statementIsStatic(global));
    QVERIFY(!std::dynamic_pointer_cast<Statement>(global)
             ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));

    PStatement internal = mParser->findStatement("internal");
    QVERIFY(internal && statementHasDefinition(internal));
    QCOMPARE(internal->scope, StatementScope::Global);
    QVERIFY(statementIsStatic(internal));

    PStatement declaration = mParser->findStatement("declaration");
    QVERIFY(declaration && !statementHasDefinition(declaration));
    QVERIFY(std::dynamic_pointer_cast<Statement>(declaration)
            ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
    QCOMPARE(mParser->findStatementOf("text.cpp", QStringList{"declaration"}, 5),
             declaration);

    PStatement defined = mParser->findStatement("defined");
    QVERIFY(defined && statementHasDefinition(defined));
    QVERIFY(std::dynamic_pointer_cast<Statement>(defined)
            ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
    QVERIFY(!statementIsStatic(defined));
    PStatement braceDefined = mParser->findStatement("braceDefined");
    QVERIFY(braceDefined && statementHasDefinition(braceDefined));
    QVERIFY(std::dynamic_pointer_cast<Statement>(braceDefined)
            ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));

    PStatement member = mParser->findStatement("C::member");
    QVERIFY(member && !statementHasDefinition(member));
    QCOMPARE(member->scope, StatementScope::ClassLocal);
    QVERIFY(statementIsStatic(member));
    PStatement inlineMember = mParser->findStatement("C::inlineMember");
    QVERIFY(inlineMember && statementHasDefinition(inlineMember));
    QCOMPARE(inlineMember->scope, StatementScope::ClassLocal);
    QVERIFY(statementIsStatic(inlineMember));

    PStatement local = mParser->findStatementOf("text.cpp", QStringList{"local"}, 5);
    QVERIFY(local);
    QVERIFY(statementHasDefinition(local));
    QCOMPARE(local->scope, StatementScope::Local);
    QVERIFY(statementIsStatic(local));
}

void TestCppParser::test_static_extern_function_definitions()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "void global() {}",
            "static void internal() {}",
            "extern void declaration();",
            "extern void defined() {}",
            "struct C {static void member();};",
            "void caller(){extern void declaration();}",
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement global = mParser->findStatement("global");
    QVERIFY(global && statementHasDefinition(global));
    QCOMPARE(global->scope, StatementScope::Global);
    QVERIFY(!statementIsStatic(global));

    PStatement internal = mParser->findStatement("internal");
    QVERIFY(internal && statementHasDefinition(internal));
    QVERIFY(statementIsStatic(internal));

    PStatement declaration = mParser->findStatement("declaration");
    QVERIFY(declaration && !statementHasDefinition(declaration));
    QVERIFY(std::dynamic_pointer_cast<Statement>(declaration)
            ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
    QCOMPARE(mParser->findStatementOf("text.cpp", QStringList{"declaration"}, 5),
             declaration);

    PStatement defined = mParser->findStatement("defined");
    QVERIFY(defined && statementHasDefinition(defined));
    QVERIFY(std::dynamic_pointer_cast<Statement>(defined)
            ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
    QVERIFY(!statementIsStatic(defined));

    PStatement member = mParser->findStatement("C::member");
    QVERIFY(member && !statementHasDefinition(member));
    QCOMPARE(member->scope, StatementScope::ClassLocal);
    QVERIFY(statementIsStatic(member));
}

void TestCppParser::test_extern_variable_declaration_definition_order()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "extern int first;",
            "int first=1;",
            "int second=2;",
            "extern int second;",
            "extern int repeated;",
            "extern int repeated;",
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    for (const QString &name : {QString("first"), QString("second")}) {
        PStatement statement = mParser->findStatement(name);
        QVERIFY(statement && statementHasDefinition(statement));
        QCOMPARE(mParser->statementList().childrenStatements().values(name).size(), 1);
        QCOMPARE(statement->declarations.count(), 1);
        QVERIFY(std::dynamic_pointer_cast<Statement>(statement)
                ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
    }
    QCOMPARE(mParser->findStatement("first")->definition.range.start.line, 1);
    QCOMPARE(mParser->findStatement("second")->definition.range.start.line, 2);

    PStatement repeated = mParser->findStatement("repeated");
    QVERIFY(repeated && !statementHasDefinition(repeated));
    QCOMPARE(mParser->statementList().childrenStatements().values("repeated").size(), 1);
    QCOMPARE(repeated->declarations.count(), 2);
}

void TestCppParser::test_extern_function_declaration_definition_order()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "extern void first();",
            "void first() {}",
            "void second() {}",
            "extern void second();",
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    for (const QString &name : {QString("first"), QString("second")}) {
        PStatement statement = mParser->findStatement(name);
        QVERIFY(statement && statementHasDefinition(statement));
        QCOMPARE(mParser->statementList().childrenStatements().values(name).size(), 1);
        QCOMPARE(statement->declarations.count(), 1);
        QVERIFY(std::dynamic_pointer_cast<Statement>(statement)
                ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
    }
    QCOMPARE(mParser->findStatement("first")->definition.range.start.line, 1);
    QCOMPARE(mParser->findStatement("second")->definition.range.start.line, 2);
}

void TestCppParser::test_unresolved_member_function_reference()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<class T> using Result = typename T::type;",
            "struct C {",
            "    template<class T> Result<T> append();",
            "};",
            "C c;",
            "int main() {",
            "    c.append;",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement reference = mParser->findStatementOf(
                "text.cpp", QStringList{"c", ".", "append"}, 6);
    QVERIFY(reference);
    QCOMPARE(reference->command, "append");
    QCOMPARE(reference->kind, StatementKind::Function);
    QCOMPARE(reference->parentScope.lock(), mParser->findStatement("C"));
}

void TestCppParser::test_nested_template_member_definition()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename T> struct C {",
            "    template<typename U> C<T>& f(U);",
            "};",
            "template<typename T>",
            "template<typename U>",
            "C<T>&",
            "C<T>::",
            "f(U) { return *this; }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement function = mParser->findStatement("C::f");
    QVERIFY(function);
    QCOMPARE(function->kind, StatementKind::Function);
    QCOMPARE(function->parentScope.lock(), mParser->findStatement("C"));
    QVERIFY(statementHasDefinition(function));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(function)->templateParameters.count(), 2);
    QVERIFY(!mParser->findStatement("C<T>"));
}

void TestCppParser::test_extern_template_class_declaration()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename T> struct C {};",
            "extern template class C<int>;",
            "int after;"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement specialization = mParser->findStatement("C<int>");
    QVERIFY(specialization);
    QCOMPARE(specialization->kind, StatementKind::Class);
    QVERIFY(mParser->findStatement("after"));
    for (const PStatement &statement : mParser->statementList().statements()) {
        QVERIFY2(!statement->command.isEmpty() || statement->kind != StatementKind::Variable,
                 QString("empty variable at line %1")
                 .arg(statement->definition.range.start.line).toUtf8().constData());
    }
}

void TestCppParser::test_forward_declaration_prefers_definition_for_eval()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "class C;",
            "class C {",
            "public:",
            "    constexpr explicit operator bool() const noexcept { return true; }",
            "    constexpr const int& value() const& { return x; }",
            "    constexpr int& value() & { return x; }",
            "    int x;",
            "};",
            "C c;",
            "template<class T> class Box;",
            "template<class T> class Box { public: int value(); };",
            "Box<int> box;",
            "int main() {",
            "    c.value();",
            "    box.value();",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement reference = mParser->findStatementOf(
                "text.cpp", QStringList{"c", ".", "value", "(", ")"}, 13);
    QVERIFY(reference);
    QCOMPARE(reference->command, "value");
    QCOMPARE(reference->kind, StatementKind::Function);
    QCOMPARE(statementDeclarationForNavigation(reference, QString()).range.start.line, 5);
    PStatement templateReference = mParser->findStatementOf(
                "text.cpp", QStringList{"box", ".", "value", "(", ")"}, 14);
    QVERIFY(templateReference);
    QCOMPARE(templateReference->command, "value");
    QCOMPARE(statementDeclarationForNavigation(templateReference, QString()).range.start.line, 10);
}

void TestCppParser::test_base_member_return_uses_declaration_bindings()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<class K, class V> struct Pair { using type = V; };",
            "template<class K, class V> struct Base {",
            "    using value_type = V;",
            "    using iterator = Pair<K, value_type>;",
            "    iterator find();",
            "};",
            "template<class K, class V> struct Derived : Base<K, V> {};",
            "Derived<int, std::string> d;",
            "int main() {",
            "    d.find();",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement scope = mParser->findScopeStatement("text.cpp", 9);
    QVERIFY(scope);
    QStringList expression{"d", ".", "find", "(", ")"};
    EvalResult result = mParser->evalExpression("text.cpp", expression, scope);
    QVERIFY(result.isValid());
    PStatement declaration = result.type.declaration.lock();
    QVERIFY(declaration);
    QCOMPARE(declaration->fullName, "Pair");
    QCOMPARE(result.statement->command, "find");
}

void TestCppParser::test_derived_member_hides_base_overloads()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Base { int value(int); long value(double); };",
            "struct Derived : Base { short value(int); };",
            "Derived object;",
            "void test() {}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    PStatement scope = mParser->findScopeStatement("text.cpp", 3);
    QVERIFY(scope);
    QStringList expression{"object", ".", "value", "(", "1.0", ")"};
    EvalResult result = mParser->evalExpression("text.cpp", expression, scope);
    QVERIFY(result.isValid());
    QCOMPARE(result.type.qualifiedName, "short");
    QVERIFY(result.invocation.statement);
    QCOMPARE(result.invocation.statement->parentScope.lock()->fullName, "Derived");
}

void TestCppParser::test_member_overload_resolution()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Implicit { Implicit(const char *); };",
            "struct Explicit { explicit Explicit(const char *); };",
            "struct Other { Other(const char *); };",
            "struct Overloads {",
            "    int convert(Implicit);",
            "    template<class T> long convert(T);",
            "    int ambiguous(Implicit);",
            "    int ambiguous(Other);",
            "    int explicitOnly(Explicit);",
            "    int value();",
            "    long value() const;",
            "    int rvalueOnly() &&;",
            "};",
            "Overloads object;",
            "const Overloads constObject;",
            "int main() {",
            "    object.convert(\"x\");",
            "    object.ambiguous(\"x\");",
            "    object.explicitOnly(\"x\");",
            "    object.value();",
            "    constObject.value();",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement scope = mParser->findScopeStatement("text.cpp", 15);
    QVERIFY(scope);
    auto evaluate = [&](QStringList expression) {
        return mParser->evalExpression("text.cpp", expression, scope);
    };
    EvalResult conversion = evaluate({"object", ".", "convert", "(", "\"x\"", ")"});
    QVERIFY(conversion.isValid());
    QCOMPARE(conversion.type.qualifiedName, "long");
    EvalResult value = evaluate({"object", ".", "value", "(", ")"});
    QVERIFY(value.isValid());
    QCOMPARE(value.type.qualifiedName, "int");
    EvalResult constValue = evaluate({"constObject", ".", "value", "(", ")"});
    QVERIFY(constValue.isValid());
    QCOMPARE(constValue.type.qualifiedName, "long");
    QVERIFY(!evaluate({"object", ".", "explicitOnly", "(", "\"x\"", ")"}).isValid());
    QVERIFY(!evaluate({"object", ".", "ambiguous", "(", "\"x\"", ")"}).isValid());
    QVERIFY(!evaluate({"object", ".", "rvalueOnly", "(", ")"}).isValid());
}

void TestCppParser::test_conversion_overload_resolution()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Implicit { Implicit(int); };",
            "struct Explicit { explicit Explicit(int); };",
            "struct Twice { Twice(Implicit); };",
            "struct IntConversion { operator int() const; };",
            "struct ExplicitConversion { explicit operator double() const; };",
            "struct NoConversion {};",
            "short shortValue;",
            "float floatValue;",
            "IntConversion convertible;",
            "ExplicitConversion explicitConvertible;",
            "NoConversion noConversion;",
            "int promoted(int);",
            "long promoted(long);",
            "double floatPromoted(double);",
            "long floatPromoted(int);",
            "int implicitOnly(Implicit);",
            "int explicitOnly(Explicit);",
            "int twiceOnly(Twice);",
            "int conversionOperator(int);",
            "int explicitConversionOperator(double);",
            "int standardOverUser(double);",
            "long standardOverUser(Implicit);",
            "void test() {}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    PStatement scope = mParser->findScopeStatement("text.cpp", 22);
    QVERIFY(scope);
    PStatement shortStatement = mParser->findStatementOf("text.cpp", "shortValue", 22);
    QVERIFY(shortStatement);
    QCOMPARE(shortStatement->command, "shortValue");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(shortStatement)->typeInfo.qualifiedName, "short");
    auto evaluate = [&](const QString &name, const QString &argument) {
        QStringList expression{name, "(", argument, ")"};
        return mParser->evalExpression("text.cpp", expression, scope);
    };

    QCOMPARE(evaluate("promoted", "shortValue").type.qualifiedName, "int");
    QCOMPARE(evaluate("floatPromoted", "floatValue").type.qualifiedName, "double");
    QVERIFY(evaluate("implicitOnly", "1").isValid());
    QVERIFY(!evaluate("explicitOnly", "1").isValid());
    QVERIFY(!evaluate("twiceOnly", "1").isValid());
    QCOMPARE(evaluate("conversionOperator", "convertible").type.qualifiedName, "int");
    QVERIFY(!evaluate("conversionOperator", "noConversion").isValid());
    QVERIFY(!evaluate("explicitConversionOperator", "explicitConvertible").isValid());
    QCOMPARE(evaluate("standardOverUser", "1").type.qualifiedName, "int");
}

void TestCppParser::test_trailing_return_member_template()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Tree { template<typename T> int _M_find_tr(const T&); };",
            "struct Map {",
            "    Tree _M_t;",
            "    template<typename K> auto find(const K& x) -> decltype(_M_t._M_find_tr(x)) { return _M_t._M_find_tr(x); }",
            "};",
            "Map mapping;",
            "void test() {}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    PStatement scope = mParser->findScopeStatement("text.cpp", 6);
    QVERIFY(scope);
    QStringList expression{"mapping", ".", "find", "(", "\"x\"", ")"};
    EvalResult result = mParser->evalExpression("text.cpp", expression, scope);
    QVERIFY(result.isValid());
    QCOMPARE(result.type.qualifiedName, "int");
    QVERIFY(result.invocation.statement);
    QVERIFY(std::dynamic_pointer_cast<Statement>(result.invocation.statement)->trailingReturnType.contains(
        "_M_find_tr"));
}

void TestCppParser::test_find_statement_of_overload_resolution()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "void f(int);",
            "void f(double);",
            "void f(int) {}",
            "void f(double) {}",
            "void call(){f(1);f(1.0);}",
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement integer = mParser->findStatementOf(
                "text.cpp", {"f", "(", "1", ")"}, 4);
    QVERIFY(integer);
    QCOMPARE(statementDefinitionForNavigation(integer).range.start.line, 2);
    QCOMPARE(statementDefinitionForNavigation(integer).range.start.ch, 0);
    QCOMPARE(statementDeclarationForNavigation(integer, "text.cpp").range.start.line, 0);
    QCOMPARE(statementDeclarationForNavigation(integer, "text.cpp").range.start.ch, 0);

    PStatement real = mParser->findStatementOf(
                "text.cpp", {"f", "(", "1.0", ")"}, 4);
    QVERIFY(real);
    QCOMPARE(statementDefinitionForNavigation(real).range.start.line, 3);
    QCOMPARE(statementDefinitionForNavigation(real).range.start.ch, 0);
    QCOMPARE(statementDeclarationForNavigation(real, "text.cpp").range.start.line, 1);
    QCOMPARE(statementDeclarationForNavigation(real, "text.cpp").range.start.ch, 0);
}

void TestCppParser::test_find_statement_of_template_specialization()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<class T> struct Foo {int primary;};",
            "template<class T> struct Foo<T*> {int partial;};",
            "template<> struct Foo<int*> {int full;};",
            "Foo<char> primary;",
            "Foo<char*> partial;",
            "Foo<int*> full;",
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    const QVector<QPair<QStringList, int>> cases{
        {{"Foo", "<", "char", ">"}, 0},
        {{"Foo", "<", "char", "*", ">"}, 1},
        {{"Foo", "<", "int", "*", ">"}, 2},
    };
    for (int i = 0; i < cases.size(); ++i) {
        PStatement statement = mParser->findStatementOf(
                    "text.cpp", cases.at(i).first, i + 3);
        QVERIFY2(statement, qPrintable(QString("template case %1").arg(i)));
        StatementOccurrence definition = statementDefinitionForNavigation(statement);
        QCOMPARE(definition.range.start.line, cases.at(i).second);
        QCOMPARE(definition.range.start.ch, 0);
        StatementOccurrence declaration = statementDeclarationForNavigation(
                    statement, "text.cpp");
        QCOMPARE(declaration.range.start.line, cases.at(i).second);
        QCOMPARE(declaration.range.start.ch, 0);
    }
}

void TestCppParser::test_expression_identifier_trace()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Object { int value; int get(); };",
            "Object object;",
            "Object *pointer;",
            "namespace outer { int value; } int globalValue;",
            "Object make(Object value) { return value; }",
            "template<class T> struct Box { T value; };",
            "Box<int> box;",
            "int main() {",
            "    object.get();",
            "    pointer->value;",
            "    outer::value;",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement scope = mParser->findScopeStatement("text.cpp", 8);
    QVERIFY(scope);
    auto check = [&](QStringList expression, const QVector<int> &expectedIndexes,
                     const QVector<int> &expectedStarts = {}) {
        ExpressionEvalTrace trace;
        mParser->evalExpression("text.cpp", expression, scope, &trace);
        QCOMPARE(trace.size(), expectedIndexes.size());
        for (int i = 0; i < trace.size(); ++i) {
            QCOMPARE(trace[i].tokenIndex, expectedIndexes[i]);
            int start = expectedStarts.isEmpty() ? 0 : expectedStarts[i];
            PStatement expected = mParser->findStatementOf(
                        "text.cpp", expression.mid(start, trace[i].tokenIndex - start + 1), 8);
            if (trace[i].statement != expected) {
                qDebug() << "trace mismatch" << expression << trace[i].tokenIndex
                         << (trace[i].statement ? trace[i].statement->fullName : QString())
                         << (expected ? expected->fullName : QString());
            }
            QCOMPARE(trace[i].statement, expected);
        }
        CppTokenizer tokenizer;
        tokenizer.tokenize({expression.join(' ')});
        ExpressionEvalTrace directTrace;
        mParser->evalExpression("text.cpp", tokenizer.tokens(), scope, &directTrace);
        QCOMPARE(directTrace.size(), trace.size());
        for (int i = 0; i < directTrace.size(); ++i) {
            QCOMPARE(directTrace[i].tokenIndex, trace[i].tokenIndex);
            QCOMPARE(directTrace[i].statement, trace[i].statement);
        }
    };
    check({"object", ".", "get"}, {0, 2});
    check({"pointer", "->", "value"}, {0, 2});
    check({"outer", "::", "value"}, {0, 2});
    check({"::", "globalValue"}, {1});
    check({"make", "(", "object", ")", ".", "get"}, {0, 2, 5}, {0, 2, 0});
    check({"Box", "<", "int", ">", "::", "value"}, {0, 5});
    check({"object", ".", "missing"}, {0, 2});
}

void TestCppParser::test_parse_vars()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/vars.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    PStatement statement;
    statement = mParser->findStatement("a1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"extern const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[4]");

    statement = mParser->findStatement("a6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int &");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int **");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int **");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a9");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a9");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a10");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a10");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a11");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a11");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int &");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"const unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"extern const unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[4]");

    statement = mParser->findStatement("b6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static unsigned int *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static unsigned int *const");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static unsigned int **");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[4]");

    statement = mParser->findStatement("s6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string &");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("f1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"f1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const struct FILE *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("f2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"f2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const struct FILE");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("ff1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statementType(statement),"int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"( )");

    statement = mParser->findStatement("ff2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statementType(statement),"static const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"( )");

    statement = mParser->findStatement("ff3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statementType(statement),"static const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"( )");

    //TODO:: ff4 ff5

    statement = mParser->findStatement("ff6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statementType(statement),"int *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"( )");

    statement = mParser->findStatement("aa1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"aa1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(!(statement->properties & StatementProperty::FunctionPointer));
    QCOMPARE(statementType(statement),"int (*)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[5]");

    statement = mParser->findStatement("aa2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"aa2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(!(statement->properties & StatementProperty::FunctionPointer));
    QCOMPARE(statementType(statement),"static const std::string (*)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[5]");

}

void TestCppParser::test_struct()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/struct.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    PStatement statement;
    statement = mParser->findStatement("TTT");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->command, "TTT");
    QCOMPARE(statement->kind, StatementKind::Class);
    PStatement child;
    child = std::dynamic_pointer_cast<Statement>(statement)->children.value("xxx").lock();
    QCOMPARE(child->command,"xxx");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::xxx");
    QCOMPARE(statementType(child), "int");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);
    child = std::dynamic_pointer_cast<Statement>(statement)->children.value("yyy").lock();
    QCOMPARE(child->command,"yyy");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::yyy");
    QCOMPARE(statementType(child), "short");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);
    child = std::dynamic_pointer_cast<Statement>(statement)->children.value("zzz").lock();
    QCOMPARE(child->command,"zzz");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::zzz");
    QCOMPARE(statementType(child), "double");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);

    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->children.count(),4);
    child = std::dynamic_pointer_cast<Statement>(statement)->children.value("ppp").lock();
    QCOMPARE(child->command,"ppp");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::ppp");
    QCOMPARE(statementType(child), "int");
    QCOMPARE(child->accessibility, StatementAccessibility::Private);

}

void TestCppParser::test_resolve_members_of_class_access_scope()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "class Base {",
            "public: int publicValue;",
            "protected: int protectedValue;",
            "private: int privateValue;",
            "};",
            "class Derived : private Base { public: void action(); };",
            "struct PublicDerived : Base { void action(); };",
            "struct ProtectedDerived : protected Base { void action(); };"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    PStatement base = mParser->findStatement("Base");
    PStatement derived = mParser->findStatement("Derived");
    PStatement publicDerived = mParser->findStatement("PublicDerived");
    PStatement protectedDerived = mParser->findStatement("ProtectedDerived");
    QVERIFY(base && derived && publicDerived && protectedDerived);
    auto names = [](const StatementList &statements) {
        QSet<QString> result;
        for (const PStatement &statement : statements)
            result.insert(statement->command);
        return result;
    };

    QCOMPARE(names(mParser->resolveMembersOfClass(derived, {})),
             QSet<QString>{"action"});
    QCOMPARE(names(mParser->resolveMembersOfClass(derived, derived)),
             QSet<QString>({"action", "publicValue", "protectedValue"}));
    QCOMPARE(names(mParser->resolveMembersOfClass(derived, base)),
             QSet<QString>({"action", "publicValue", "protectedValue", "privateValue"}));
    QCOMPARE(names(mParser->resolveMembersOfClass(publicDerived, {})),
             QSet<QString>({"action", "publicValue"}));
    QCOMPARE(names(mParser->resolveMembersOfClass(protectedDerived, {})),
             QSet<QString>{"action"});
    QCOMPARE(names(mParser->resolveMembersOfClass(protectedDerived, protectedDerived)),
             QSet<QString>({"action", "publicValue", "protectedValue"}));
}

void TestCppParser::test_preprocessor_statements()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "#define OBJECT_MACRO 7",
            "#define FUNCTION_MACRO(x) ((x)+1)",
            "int value;"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "macros.cpp", false, "");

    PStatement object = mParser->findStatement("OBJECT_MACRO");
    QVERIFY(object);
    QCOMPARE(object->kind, StatementKind::Preprocessor);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(object)->args, "");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(object)->value, "7");
    QCOMPARE(object->definition.fileName, "macros.cpp");
    QCOMPARE(object->definition.range.start.line, 0);
    QVERIFY(statementHasDefinition(object));

    PStatement function = mParser->findStatement("FUNCTION_MACRO");
    QVERIFY(function);
    QCOMPARE(function->kind, StatementKind::Preprocessor);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(function)->args, "(x)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(function)->value, "((x)+1)");
    QCOMPARE(function->definition.fileName, "macros.cpp");
    QCOMPARE(function->definition.range.start.line, 1);
    QVERIFY(statementHasDefinition(function));
}

void TestCppParser::test_label_statements()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "void f() { retry: ; }",
            "void g() { bool flag = true; flag ? retry : retry; "
            "switch (flag) { case true: break; default: break; } }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "labels.cpp", false, "");

    PStatement function = mParser->findStatement("f");
    QVERIFY(function);
    PStatement label = std::dynamic_pointer_cast<Statement>(function)->children.value("retry").lock();
    QVERIFY(label);
    QCOMPARE(label->kind, StatementKind::Label);
    QCOMPARE(label->fullName, "f::retry");
    QCOMPARE(label->definition.range.start.line, 0);
    QCOMPARE(label->definition.fileName, "labels.cpp");

    PStatement notLabels = mParser->findStatement("g");
    QVERIFY(notLabels);
    for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(notLabels)->children) {
        PStatement child = weak.lock();
        QVERIFY(!child || child->kind != StatementKind::Label);
    }
}

void TestCppParser::test_hard_define_statements()
{
    mParser->addHardDefineByLine("#define HARD_MACRO(x) ((x)+1)");
    mParser->parseHardDefines();
    PStatement macro = mParser->findStatement("HARD_MACRO");
    QVERIFY(macro);
    QCOMPARE(macro->kind, StatementKind::Preprocessor);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(macro)->args, "(x)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(macro)->value, "((x)+1)");
    QVERIFY(statementDeclarationForNavigation(macro, QString()).fileName.isEmpty());
    QVERIFY(!statementDefinitionForNavigation(macro).range.isValid());
    QVERIFY(statementHasDefinition(macro));
    QCOMPARE(mParser->statementList().count(), 1);
}

void TestCppParser::test_predefined_func_variable()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "void alpha() {}",
            "struct C { void beta() {} };",
            "void declarationOnly();"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement alphaFunc = mParser->findStatementOf("text.cpp", QStringList{"__func__"}, 0);
    QVERIFY(alphaFunc);
    QCOMPARE(alphaFunc->kind, StatementKind::LocalVariable);
    QCOMPARE(alphaFunc->fullName, "alpha::__func__");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(alphaFunc)->value, "\"alpha\"");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(alphaFunc)->typeInfo.qualifiedName, "char");
    QVERIFY(std::dynamic_pointer_cast<Statement>(alphaFunc)->typeInfo.qualifiers.testFlag(TypeQualifier::Const));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(alphaFunc)->typeInfo.derivations.count(), 1);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(alphaFunc)->typeInfo.derivations.first().kind,
             TypeDerivationKind::Array);
    QVERIFY(statementHasDefinition(alphaFunc));
    PStatement alpha = mParser->findStatement("alpha");
    QVERIFY(alpha);
    QCOMPARE(alphaFunc->definition.fileName, alpha->definition.fileName);
    QCOMPARE(alphaFunc->definition.range, alpha->definition.range);

    PStatement betaFunc = mParser->findStatementOf("text.cpp", QStringList{"__func__"}, 1);
    QVERIFY(betaFunc);
    QCOMPARE(betaFunc->fullName, "C::beta::__func__");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(betaFunc)->value, "\"beta\"");
    QVERIFY(!mParser->findStatementOf("text.cpp", QStringList{"__func__"}, 2));
    QVERIFY(!mParser->findStatementOf("text.cpp", QStringList{"__func__"}, 3));
    QVERIFY(!mParser->findStatement("__func__"));
}

void TestCppParser::test_structured_bindings()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/structured-bindings.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    for (const QString &name : QStringList{"a", "b", "c", "d", "e", "g"}) {
        PStatement statement = mParser->findStatement(name);
        QVERIFY(statement!=nullptr);
        QCOMPARE(statement->fullName,name);
        QCOMPARE(statement->kind,StatementKind::Variable);
        QCOMPARE(statementType(statement),"auto");
        QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");
    }
}

void TestCppParser::test_find_statement_of_inline_namespace()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace std {",
            "  inline namespace __cxx11 {",
            "    class basic_string {};",
            "    using string = basic_string;",
            "    int inlineValue;",
            "  }",
            "  basic_string value;",
            "}",
            "std::string text;",
            "namespace std { namespace literals {",
            "  inline namespace string_view_literals { void f(); }",
            "} }",
            "namespace { int anonymousValue; }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement stdNamespace = mParser->findStatementOf("text.cpp", QStringList{"std"}, 0);
    QVERIFY(stdNamespace);
    QCOMPARE(stdNamespace->fullName, "std");
    PStatement inlineNamespace = mParser->findStatement("std::__cxx11");
    QVERIFY(inlineNamespace);
    QCOMPARE(inlineNamespace->fullName, "std::__cxx11");
    PStatement basicString = mParser->findStatement("std::basic_string");
    QVERIFY(basicString);
    QCOMPARE(basicString->fullName, "std::__cxx11::basic_string");
    QVERIFY(!mParser->findStatement("basic_string"));
    QCOMPARE(mParser->findStatementOf("text.cpp", {"std", "::", "basic_string"}, 0),
             basicString);
    QCOMPARE(mParser->findStatementOf("text.cpp", QStringList{"basic_string"}, 5),
             basicString);
    PStatement value = mParser->findStatementOf("text.cpp", QStringList{"value"}, 5);
    QVERIFY(value);
    QCOMPARE(getKindOfStatement(value), StatementKind::GlobalVariable);
    PStatement inlineValue = mParser->findStatement("std::inlineValue");
    QVERIFY(inlineValue);
    QCOMPARE(inlineValue->fullName, "std::__cxx11::inlineValue");
    PStatement stringAlias = mParser->findStatement("std::string");
    QVERIFY(stringAlias);
    QCOMPARE(stringAlias->fullName, "std::__cxx11::string");
    QCOMPARE(mParser->findStatementOf("text.cpp", {"std", "::", "string"}, 0),
             stringAlias);
    QVERIFY(statementHasDefinition(stringAlias));
    PStatement literalFunction = mParser->findStatement("std::literals::f");
    QVERIFY(literalFunction);
    QCOMPARE(literalFunction->fullName, "std::literals::string_view_literals::f");
    PStatement anonymousValue = mParser->findStatement("anonymousValue");
    QVERIFY(anonymousValue);
    QCOMPARE(anonymousValue->fullName, "anonymousValue");
}

void TestCppParser::test_nested_namespace_scope_parentage()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace A {",
            "namespace B {",
            "namespace C {",
            "namespace D {",
            "class X {};",
            "}",
            "int cValue;",
            "}",
            "int bValue;",
            "}",
            "int aValue;",
            "}",
            "int global;"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    const QStringList expectedScopes{
        "A", "A::B", "A::B::C", "A::B::C::D", "A::B::C::D::X",
        "A::B::C::D", "A::B::C", "A::B::C", "A::B", "A::B",
        "A", "A", ""
    };
    for (int line = 0; line < expectedScopes.count(); ++line) {
        PStatement scope = mParser->findScopeStatement("text.cpp", line);
        QCOMPARE(scope ? scope->fullName : QString(), expectedScopes[line]);
    }
}

void TestCppParser::test_variable_template_specialization_name()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace Detail { struct Identity {}; }",
            "template<typename A, typename B> struct is_same;",
            "template<typename T> struct HashTable {",
            "    using key_type = T;",
            "    template<typename Arg, typename DArg = key_type, typename = Detail::Identity>",
            "    static constexpr bool is_key = false;",
            "    template<typename Arg>",
            "    static constexpr bool is_key<Arg, key_type, Detail::Identity> = is_same<Arg, key_type>::value;",
            "};"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    PStatement specialization;
    for (const PStatement &statement : mParser->statementList().statements()) {
        if (statement->fullName == "HashTable::is_key<Arg,key_type,Detail::Identity>")
            specialization = statement;
    }

    PStatement keyType = mParser->findStatement("HashTable::key_type");
    QVERIFY(keyType);
    QCOMPARE(keyType->kind, StatementKind::Alias);
    PStatement primary = mParser->findStatement("HashTable::is_key");
    QVERIFY(primary);
    QCOMPARE(primary->kind, StatementKind::Variable);
    QVERIFY(specialization);
    QCOMPARE(specialization->kind, StatementKind::Variable);
}

void TestCppParser::test_member_alias_template_specialization()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace std {",
            "    template<bool> struct __conditional {",
            "        template<typename T, typename> using type = T;",
            "    };",
            "    template<> struct __conditional<false> {",
            "        template<typename, typename U> using type = U;",
            "    };",
            "    template<bool C, typename I, typename E>",
            "    using __conditional_t = typename __conditional<C>::template type<I, E>;",
            "}",
            "struct Value {};"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement value = mParser->findStatement("Value");
    QVERIFY(value);
    QCOMPARE(mParser->findTypeDefinitionOf(
                 "text.cpp", "std::__conditional_t<false,int,Value>", {}), value);
}

void TestCppParser::test_unknown_non_type_default_preserves_alias_resolution()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename...> struct And {};",
            "template<bool> struct BoolConstant {};",
            "template<typename T> using CacheDefault = BoolConstant<!And<T>::value>;",
            "template<bool> struct Traits {};",
            "template<typename T, typename TTraits = Traits<CacheDefault<T>::value>>",
            "struct HashTable : And<T> { using key_type = T; };",
            "template<typename T> using Unordered = HashTable<T>;",
            "struct Value {};"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement value = mParser->findStatement("Value");
    QVERIFY(value);
    QCOMPARE(mParser->findTypeDefinitionOf("text.cpp", "Unordered<Value>::key_type", {}), value);
}

void TestCppParser::test_using_declaration_function_overload_set()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Result {};",
            "namespace std { template<typename T> Result remove(T, T); }",
            "int remove(const char *);",
            "namespace std { using ::remove; }",
            "auto value = std::remove(1, 2);"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    QStringList expression{"std", "::", "remove", "(", "1", ",", "2", ")"};
    EvalResult result = mParser->evalExpression("text.cpp", expression, {});
    QVERIFY(result.isValid());
    QCOMPARE(result.type.declaration.lock(), mParser->findStatement("Result"));
}

void TestCppParser::test_overload_rejects_unrelated_class_specialization()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace std {",
            "    template<typename T> struct iterator {};",
            "    template<typename T> struct deque_iterator {};",
            "    template<typename II, typename OI> OI copy(II, II, OI);",
            "    template<typename T> deque_iterator<T> copy(deque_iterator<T>, deque_iterator<T>, deque_iterator<T>);",
            "}",
            "std::iterator<int> it;",
            "std::deque_iterator<int> dequeIt;",
            "auto result = std::copy(it, it, it);",
            "auto dequeResult = std::copy(dequeIt, dequeIt, dequeIt);"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    for (const QString &name : {QString("it"), QString("dequeIt")}) {
        QStringList expression{"std", "::", "copy", "(", name, ",", name, ",", name, ")"};
        EvalResult result = mParser->evalExpression("text.cpp", expression, {});
        QVERIFY(result.isValid());
        PStatement expected = mParser->findStatement(
                    name == "dequeIt" ? "std::deque_iterator" : "std::iterator");
        QCOMPARE(result.type.declaration.lock(), expected);
        QCOMPARE(result.type.templateArguments.count(), 1);
        QVERIFY(result.type.templateArguments.first().type);
        QCOMPARE(result.type.templateArguments.first().type->qualifiedName, QString("int"));
    }
}

void TestCppParser::test_empty_template_argument_list()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename...> using __void_t = void;"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    CppTypeResolver resolver(mParser->statementList());
    TypeInfo type;
    type.qualifiedName = "__void_t";
    type.qualifiedNameText = "__void_t<>";
    TypeInfo resolved = resolver.resolve(type, {}, mParser->findStatement("std"));
    QCOMPARE(resolved.qualifiedName, QString("void"));
}

void TestCppParser::test_const_template_parameter_substitution()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename T> struct Pair {};",
            "template<typename T> struct Map { using value_type = Pair<const T>; };"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    CppTypeResolver resolver(mParser->statementList());
    TypeInfo type;
    type.qualifiedName = "Map<int>::value_type";
    type.qualifiedNameText = "Map<int>::value_type";
    TypeInfo resolved = resolver.resolve(type);
    QCOMPARE(resolved.qualifiedName, QString("Pair"));
    QCOMPARE(resolved.templateArguments.count(), 1);
    QVERIFY(resolved.templateArguments.first().type);
    QCOMPARE(resolved.templateArguments.first().type->qualifiedName, QString("int"));
    QCOMPARE(resolved.templateArguments.first().type->toString(), QString("const int"));
}

void TestCppParser::test_auto_reference_collapsing()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Value {};",
            "Value &get();",
            "auto &value = get();",
            "auto &&other = get();"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    for (const QString &name : {QString("value"), QString("other")}) {
        PStatement statement = mParser->findStatementOf("text.cpp", name, 3);
        QVERIFY(statement);
        const TypeInfo &type = std::dynamic_pointer_cast<Statement>(statement)->typeInfo;
        QVERIFY(!type.derivations.isEmpty());
        QCOMPARE(type.derivations.first().kind, TypeDerivationKind::LvalueReference);
    }
}

void TestCppParser::test_void_t_detection_failure()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename...> using __void_t = void;",
            "template<typename T, typename = __void_t<>> struct Traits { };",
            "template<typename T> struct Traits<T, __void_t<typename T::missing>> { using type = int; };"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    CppTypeResolver resolver(mParser->statementList());
    TypeInfo type;
    type.qualifiedName = "Traits<int>";
    type.qualifiedNameText = "Traits<int>";
    TypeInfo resolved = resolver.resolve(type);
    QCOMPARE(resolved.qualifiedName, QString("Traits"));
    QCOMPARE(resolved.declaration.lock(), mParser->findStatement("Traits"));
}

void TestCppParser::test_standard_type_traits_require_cpp11()
{
    struct Result {
        PStatement resolved;
        PStatement value;
    };
    auto resolveTrait = [&](int cppVersion) {
        init_parser();
        const QString fileName = QString("text-%1.cpp").arg(cppVersion);
        mParser->addHardDefineByLine(QString("#define __cplusplus %1L").arg(cppVersion));
        mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
            buffer = QStringList{
                "namespace std { template<typename T> struct add_lvalue_reference {}; }",
                "struct Value {};"
            };
            return true;
        });
        CppParser::parseFileBlocking(mParser, fileName, false, "");
        return Result{
            mParser->findTypeDefinitionOf(
                        fileName, "std::add_lvalue_reference<Value>::type", {}),
            mParser->findStatement("Value")
        };
    };

    Result cpp98 = resolveTrait(199711);
    QVERIFY(cpp98.value);
    QVERIFY(!cpp98.resolved);
    for (int cppVersion : {CppVersion::Cpp11, CppVersion::Cpp17, CppVersion::Cpp20}) {
        Result result = resolveTrait(cppVersion);
        QVERIFY(result.value);
        QCOMPARE(result.resolved, result.value);
    }
}

void TestCppParser::test_member_declaration_definition_in_namespace()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString declaration = tmp.filePath("declare.h");
    QString definition = tmp.filePath("define.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(declaration, {"namespace N { class C { public: int method(); }; }"}));
    QVERIFY(writeLines(definition, {"namespace N { int C::method() { return 1; } }"}));
    QVERIFY(writeLines(main, {"#include \"declare.h\"", "#include \"define.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatement statement = mParser->findStatement("N::C::method");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));
    QCOMPARE(statement->declarations.value(declaration).boundaryAncestor, declaration);
    QCOMPARE(statement->definition.boundaryAncestor, definition);

    mParser->invalidateFileAndDependents(definition);
    statement = mParser->findStatement("N::C::method");
    QVERIFY(statement);
    QVERIFY(!statementHasDefinition(statement));

    CppParser::parseFileBlocking(mParser, main, true, "");
    statement = mParser->findStatement("N::C::method");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));

    mParser->invalidateFileAndDependents(declaration);
    QVERIFY(!mParser->findStatement("N::C::method"));

    CppParser::parseFileBlocking(mParser, main, true, "");
    statement = mParser->findStatement("N::C::method");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));
    QCOMPARE(statement->declarations.value(declaration).fileName, declaration);
    QCOMPARE(statement->declarations.value(declaration).boundaryAncestor, declaration);
    QCOMPARE(statement->definition.fileName, definition);
    QCOMPARE(statement->definition.boundaryAncestor, definition);
    PStatement parent = statement->parentScope.lock();
    QVERIFY(parent);
    QCOMPARE(parent->fullName, QString("N::C"));
    bool memberIndexed = false;
    for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(parent)->children.values("method")) {
        if (weak.lock() == statement) {
            memberIndexed = true;
            break;
        }
    }
    QVERIFY(memberIndexed);
}

void TestCppParser::test_overloaded_declaration_definition_merge()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString declaration = tmp.filePath("declare.h");
    QString otherDeclaration = tmp.filePath("declare_other.h");
    QString definition = tmp.filePath("define.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(declaration, {"namespace N { int f(int); }"}));
    QVERIFY(writeLines(otherDeclaration, {"namespace N { int f(double); }"}));
    QVERIFY(writeLines(definition, {"namespace N { int f(double value) { return int(value); } }"}));
    QVERIFY(writeLines(main, {"#include \"declare.h\"", "#include \"declare_other.h\"",
                              "#include \"define.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatementList namespaces = mParser->findNamespace("N");
    QVERIFY(namespaces && namespaces->count() == 1);
    auto nameSpace = std::dynamic_pointer_cast<NamespaceStatement>(namespaces->first());
    QVERIFY(nameSpace);
    QCOMPARE(nameSpace->declarations.count(), 3);
    auto overloadedFunctions = [&]() {
        QList<PStatement> result;
        for (const PStatement &nameSpace : *namespaces) {
            for (const PStatement &statement : mParser->membersOfStatement(nameSpace)) {
                if (statement->fullName == "N::f")
                    result.append(statement);
            }
        }
        return result;
    };
    PStatement intDeclaration;
    PStatement doubleDefinition;
    for (const PStatement &statement : overloadedFunctions()) {
        if (std::dynamic_pointer_cast<Statement>(statement)->normalizedArgs == "(int)")
            intDeclaration = statement;
        else if (std::dynamic_pointer_cast<Statement>(statement)->normalizedArgs == "(double)")
            doubleDefinition = statement;
    }
    QVERIFY(intDeclaration);
    QVERIFY(doubleDefinition);
    QVERIFY(intDeclaration != doubleDefinition);
    QVERIFY(!statementHasDefinition(intDeclaration));
    QVERIFY(statementHasDefinition(doubleDefinition));
    QCOMPARE(doubleDefinition->declarations.value(otherDeclaration).boundaryAncestor, otherDeclaration);
    QCOMPARE(doubleDefinition->definition.boundaryAncestor, definition);
    QCOMPARE(doubleDefinition->definition.fileName, definition);

    mParser->invalidateFileAndDependents(definition);
    QVERIFY(!statementHasDefinition(doubleDefinition));
    QVERIFY(!statementHasDefinition(intDeclaration));
    QCOMPARE(overloadedFunctions().count(), 2);
}

void TestCppParser::test_statement_declaration_navigation_selection()
{
    StatementModel model;
    PStatement statement = StatementFactory::createVariable({}, "value", "value");
    model.addDeclaration(statement, "other.h", SourceRange{{10, 0}, {10, 1}},
                         "current.tcc", {});
    model.addDeclaration(statement, "current.tcc", SourceRange{{20, 0}, {20, 1}},
                         "context.h", {});
    StatementOccurrence selected = statementDeclarationForNavigation(statement, "current.tcc");
    QCOMPARE(selected.fileName, "current.tcc");
    QCOMPARE(selected.range.start.line, 20);

    PStatement contextStatement = StatementFactory::createVariable({}, "contextValue", "contextValue");
    model.addDeclaration(contextStatement, "inner.h", SourceRange{{30, 0}, {30, 1}},
                         "context.h", {});
    selected = statementDeclarationForNavigation(contextStatement, "context.h");
    QCOMPARE(selected.fileName, "inner.h");
    QCOMPARE(selected.range.start.line, 30);

    PStatement definitionStatement = StatementFactory::createVariable({}, "definedValue", "definedValue");
    definitionStatement->definition = StatementOccurrence{
        "definition.cpp", SourceRange{{40, 0}, {40, 1}}, QString(), {}};
    setStatementHasDefinition(definitionStatement, true);
    selected = statementDeclarationForNavigation(definitionStatement, "current.tcc");
    QCOMPARE(selected.fileName, "definition.cpp");
    QCOMPARE(selected.range.start.line, 40);
}

void TestCppParser::test_local_declarations_in_disjoint_blocks_remain_distinct()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"void f() {", "  { int value; }", "  { int value; }", "}"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "main.cpp", false, "");
    PStatement function = mParser->findStatement("f");
    QVERIFY(function);
    QList<PStatement> variables;
    for (const std::weak_ptr<AbstractStatement> &weakBlock : std::dynamic_pointer_cast<Statement>(function)->children) {
        PStatement block = weakBlock.lock();
        if (!block || block->kind != StatementKind::Block)
            continue;
        for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(block)->children.values("value")) {
            if (PStatement variable = weak.lock())
                variables.append(variable);
        }
    }
    QCOMPARE(variables.count(), 2);
    QVERIFY(variables[0]->parentScope.lock() != variables[1]->parentScope.lock());
}

void TestCppParser::test_namespace_index_inline_filter_and_invalidation()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"namespace std { int value; }",
                             "namespace std { inline namespace __cxx11 { int inlineValue; } }"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "main.cpp", false, "");
    QCOMPARE(mParser->findNamespace("std")->count(), 1);
    QCOMPARE(mParser->findNamespace("std", true)->count(), 1);
    QVERIFY(mParser->findStatement("std::inlineValue"));
    QCOMPARE(mParser->findNamespace("std::__cxx11")->count(), 1);
    mParser->invalidateFileAndDependents("main.cpp");
    QVERIFY(mParser->findNamespace("std")->isEmpty());
    QVERIFY(mParser->findNamespace("std", true)->isEmpty());
    QVERIFY(mParser->findNamespace("std::__cxx11")->isEmpty());
}

void TestCppParser::test_namespace_index_does_not_promote_inline_descendants()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"namespace outer { namespace regular { } }",
                             "namespace outer::regular { inline namespace abi { int value; } }",
                             "namespace outer::regular { int reopened; }"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "main.cpp", false, "");
    QCOMPARE(mParser->findNamespace("outer")->count(), 1);
    QCOMPARE(mParser->findNamespace("outer", true)->count(), 1);
    QCOMPARE(mParser->findNamespace("outer::regular")->count(), 1);
    QCOMPARE(mParser->findNamespace("outer::regular", true)->count(), 1);
    QVERIFY(mParser->findStatement("outer::regular::value"));
    QVERIFY(mParser->findStatement("outer::regular::reopened"));
    QVERIFY(!mParser->findStatement("outer::value"));
    mParser->invalidateFileAndDependents("main.cpp");
    QVERIFY(mParser->findNamespace("outer", true)->isEmpty());
    QVERIFY(mParser->findNamespace("outer::regular", true)->isEmpty());
}

void TestCppParser::test_reopened_namespaces_share_unique_entity()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace A::B { int first; }",
            "namespace A { namespace B { int second; } }",
            "namespace A::B::C::D { int deep; }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement a = mParser->findStatement("A");
    PStatement b = mParser->findStatement("A::B");
    PStatement c = mParser->findStatement("A::B::C");
    PStatement d = mParser->findStatement("A::B::C::D");
    QVERIFY(a && b && c && d);
    QCOMPARE(b->parentScope.lock(), a);
    QCOMPARE(c->parentScope.lock(), b);
    QCOMPARE(d->parentScope.lock(), c);
    QCOMPARE(mParser->findNamespace("A")->count(), 1);
    QCOMPARE(mParser->findNamespace("A::B")->count(), 1);
    auto namespaceA = std::dynamic_pointer_cast<NamespaceStatement>(a);
    auto namespaceB = std::dynamic_pointer_cast<NamespaceStatement>(b);
    QCOMPARE(namespaceA->declarations.count(), 3);
    QCOMPARE(namespaceB->declarations.count(), 3);
    QSet<int> namespaceBLines;
    for (const StatementOccurrence &occurrence : namespaceB->declarations) {
        QCOMPARE(occurrence.fileName, "text.cpp");
        namespaceBLines.insert(occurrence.range.start.line);
    }
    QCOMPARE(namespaceBLines, QSet<int>({0, 1, 2}));
    QVERIFY(mParser->findStatement("A::B::first"));
    QVERIFY(mParser->findStatement("A::B::second"));
    QVERIFY(mParser->findStatement("A::B::C::D::deep"));
}

void TestCppParser::test_namespace_occurrence_invalidation_across_roots()
{
    mParser->setOnGetFileStream([](const QString &fileName, QStringList &buffer) {
        if (fileName == "a.cpp")
            buffer = QStringList{"namespace N::Deep { int fromA; }"};
        else if (fileName == "b.cpp")
            buffer = QStringList{"namespace N { namespace Deep { int fromB; } }"};
        else
            return false;
        return true;
    });
    CppParser::parseFileBlocking(mParser, "a.cpp", false, "");
    CppParser::parseFileBlocking(mParser, "b.cpp", false, "");

    PStatement n = mParser->findStatement("N");
    PStatement deep = mParser->findStatement("N::Deep");
    QVERIFY(n && deep);
    auto namespaceN = std::dynamic_pointer_cast<NamespaceStatement>(n);
    auto namespaceDeep = std::dynamic_pointer_cast<NamespaceStatement>(deep);
    QCOMPARE(namespaceN->declarations.count(), 2);
    QCOMPARE(namespaceDeep->declarations.count(), 2);
    QVERIFY(namespaceN->declarations.contains("a.cpp"));
    QVERIFY(namespaceN->declarations.contains("b.cpp"));
    QVERIFY(mParser->findStatement("N::Deep::fromA"));
    QVERIFY(mParser->findStatement("N::Deep::fromB"));

    mParser->invalidateFileAndDependents("a.cpp");
    QVERIFY(!mParser->findStatement("N::Deep::fromA"));
    QVERIFY(mParser->findStatement("N::Deep::fromB"));
    QCOMPARE(mParser->findStatement("N"), n);
    QCOMPARE(mParser->findStatement("N::Deep"), deep);
    QCOMPARE(namespaceDeep->declarations.count(), 1);
    QVERIFY(namespaceDeep->declarations.contains("b.cpp"));
    QCOMPARE(namespaceDeep->declarations.value("b.cpp").fileName, "b.cpp");
    QCOMPARE(namespaceDeep->declarations.value("b.cpp").range.start.line, 0);

    mParser->invalidateFileAndDependents("b.cpp");
    QVERIFY(!mParser->findStatement("N"));
    QVERIFY(!mParser->findStatement("N::Deep"));
}

void TestCppParser::test_anonymous_namespaces_remain_distinct()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace { int first; }",
            "namespace { int second; }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    const auto anonymous = mParser->statementList().childrenStatements().values("");
    QCOMPARE(anonymous.count(), 2);
    PStatement firstScope = mParser->findScopeStatement("text.cpp", 0);
    PStatement secondScope = mParser->findScopeStatement("text.cpp", 1);
    QVERIFY(firstScope && secondScope);
    QVERIFY(firstScope != secondScope);
    QVERIFY(mParser->findStatement("first"));
    QVERIFY(mParser->findStatement("second"));
}

void TestCppParser::test_inline_namespace_occurrence_invalidation()
{
    mParser->setOnGetFileStream([](const QString &fileName, QStringList &buffer) {
        if (fileName == "plain.cpp")
            buffer = QStringList{"namespace N { namespace V { int plain; } }"};
        else if (fileName == "inline.cpp")
            buffer = QStringList{"namespace N { inline namespace V { int visible; } }"};
        else
            return false;
        return true;
    });
    CppParser::parseFileBlocking(mParser, "plain.cpp", false, "");
    CppParser::parseFileBlocking(mParser, "inline.cpp", false, "");

    PStatement v = mParser->findStatement("N::V");
    QVERIFY(v);
    QCOMPARE(std::dynamic_pointer_cast<NamespaceStatement>(v)->declarations.count(), 2);
    QVERIFY(mParser->findStatement("N::visible"));
    QVERIFY(mParser->findStatement("N::plain"));

    mParser->invalidateFileAndDependents("inline.cpp");
    QVERIFY(!mParser->findStatement("N::visible"));
    QVERIFY(mParser->findStatement("N::V::plain"));
    QCOMPARE(mParser->findStatement("N::V"), v);
    auto namespaceV = std::dynamic_pointer_cast<NamespaceStatement>(v);
    QCOMPARE(namespaceV->declarations.count(), 1);
    QVERIFY(namespaceV->declarations.contains("plain.cpp"));
    QVERIFY(!namespaceV->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline));
    QCOMPARE(mParser->findNamespace("N", true)->count(), 1);

    CppParser::parseFileBlocking(mParser, "inline.cpp", false, "");
    QCOMPARE(mParser->findStatement("N::V"), v);
    QCOMPARE(namespaceV->declarations.count(), 2);
    QVERIFY(namespaceV->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline));
    QVERIFY(mParser->findStatement("N::visible"));
}

void TestCppParser::test_inline_namespace_overloads()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace N { inline namespace V {",
            "  void f(int);",
            "  void f(double);",
            "} }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement n = mParser->findStatement("N");
    QVERIFY(n);
    const auto aliases = mParser->statementList().childrenStatements(n).values("f");
    QCOMPARE(aliases.count(), 2);
    QSet<const AbstractStatement *> targets;
    for (const std::weak_ptr<AbstractStatement> &weak : aliases) {
        PStatement statement = weak.lock();
        QVERIFY(statement && statement->kind == StatementKind::InlineAlias);
        PStatement target = std::dynamic_pointer_cast<InlineAliasStatement>(statement)->target.lock();
        QVERIFY(target);
        targets.insert(target.get());
    }
    QCOMPARE(targets.count(), 2);
    int visibleOverloads = 0;
    for (const PStatement &statement : mParser->membersOfStatement(n)) {
        if (statement->command == "f")
            ++visibleOverloads;
    }
    QCOMPARE(visibleOverloads, 2);

    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    n = mParser->findStatement("N");
    QVERIFY(n);
    QCOMPARE(mParser->statementList().childrenStatements(n).values("f").count(), 2);
}

void TestCppParser::test_stop_parsing_clears_results_before_finish()
{
    mParser->addHardDefineByLine("#define __STOP_TEST__ 1");
    auto entered = std::make_shared<QSemaphore>();
    auto release = std::make_shared<QSemaphore>();
    auto blockFirstRead = std::make_shared<std::atomic_bool>(true);
    mParser->setOnGetFileStream([=](const QString &, QStringList &buffer) {
        if (blockFirstRead->exchange(false)) {
            entered->release();
            release->acquire();
        }
        buffer = QStringList{"int value;"};
        return true;
    });
    QObject context;
    int statementCountAtFinish = -1;
    bool hasFileInfoAtFinish = false;
    connect(mParser.get(), &CppParser::parseFinished, &context, [&] {
        statementCountAtFinish = mParser->statementList().count();
        hasFileInfoAtFinish = static_cast<bool>(mParser->findFileInfo("text.cpp"));
    }, Qt::DirectConnection);

    CppParser::parseFileNonBlocking(mParser, "text.cpp", false, "");
    QVERIFY(entered->tryAcquire(1, 5000));
    mParser->stopParsing();
    release->release();
    QTRY_VERIFY(!mParser->parsing());

    QCOMPARE(statementCountAtFinish, 1);
    QVERIFY(!hasFileInfoAtFinish);
    QVERIFY(!mParser->findStatement("value"));
    QVERIFY(mParser->findStatement("__STOP_TEST__"));
    QVERIFY(!mParser->findFileInfo("text.cpp"));
}

void TestCppParser::test_application_exit_stops_parsing_without_cleanup()
{
    for (bool stopLocally : {false, true}) {
        init_parser();
        auto entered = std::make_shared<QSemaphore>();
        auto release = std::make_shared<QSemaphore>();
        auto blockFirstRead = std::make_shared<std::atomic_bool>(true);
        mParser->setOnGetFileStream([=](const QString &, QStringList &buffer) {
            if (blockFirstRead->exchange(false)) {
                entered->release();
                release->acquire();
            }
            buffer = QStringList{"int value;"};
            return true;
        });
        QObject context;
        int statementCountAtFinish = -1;
        PParsedFileInfo fileInfoAtFinish;
        connect(mParser.get(), &CppParser::parseFinished, &context, [&] {
            statementCountAtFinish = mParser->statementList().count();
            fileInfoAtFinish = mParser->findFileInfo("text.cpp");
        }, Qt::DirectConnection);

        CppParser::parseFileNonBlocking(mParser, "text.cpp", false, "");
        QVERIFY(entered->tryAcquire(1, 5000));
        CppParser::prepareForApplicationExit();
        auto restoreExitState = finally([] {
            CppParser::resetApplicationExitFlagForTest();
        });
        if (stopLocally)
            mParser->stopParsing();
        release->release();
        QTRY_VERIFY(!mParser->parsing());

        QCOMPARE(statementCountAtFinish, 0);
        QVERIFY(fileInfoAtFinish);
        std::weak_ptr<ParsedFileInfo> fileInfo = fileInfoAtFinish;
        mParser.reset();
        QVERIFY(!fileInfo.expired());
    }
}

void TestCppParser::test_normal_destructor_releases_resources()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"int value;"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    std::weak_ptr<AbstractStatement> statement = mParser->findStatement("value");
    std::weak_ptr<ParsedFileInfo> fileInfo = mParser->findFileInfo("text.cpp");
    QVERIFY(!statement.expired());
    QVERIFY(!fileInfo.expired());

    mParser.reset();

    QVERIFY(statement.expired());
    QVERIFY(fileInfo.expired());
}

void TestCppParser::test_application_exit_destructor_abandons_resources()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"int value;"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    std::weak_ptr<AbstractStatement> statement = mParser->findStatement("value");
    std::weak_ptr<ParsedFileInfo> fileInfo = mParser->findFileInfo("text.cpp");

    CppParser::prepareForApplicationExit();
    int statementCount = mParser->statementList().count();
    bool hasFileInfo = static_cast<bool>(mParser->findFileInfo("text.cpp"));
    mParser.reset();
    CppParser::resetApplicationExitFlagForTest();

    QCOMPARE(statementCount, 1);
    QVERIFY(hasFileInfo);
    QVERIFY(!statement.expired());
    QVERIFY(!fileInfo.expired());
}
