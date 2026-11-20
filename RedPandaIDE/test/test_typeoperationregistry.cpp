#include <QTest>
#include "test_typeoperationregistry.h"

void TestTypeOperationRegistry::test_container_iterator_and_subscript_operations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  template<typename T> struct iterator { T& operator*(); };\n"
                "  template<typename T> struct vector {\n"
                "    using value_type = T;\n"
                "    iterator<T> begin();\n"
                "    T& operator[](int);\n"
                "  };\n"
                "}\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/type_operations_container.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppTypeResolver::EvaluationContext evaluation;
    auto type = [&](const QString &text) {
        DeclarationInfo declaration;
        if (!parseDeclarationText(text + " value;", declaration))
            return TypeInfo();
        TypeInfo result = declaration.specifiers.type;
        if (!declaration.declarators.isEmpty())
            result.derivations += declaration.declarators.first().derivations;
        return resolver.resolve(result);
    };
    TypeInfo container = type("std::vector<int>");
    TypeInfo iterator = type("std::iterator<int>");
    QVERIFY(!container.qualifiedName.isEmpty() && !iterator.qualifiedName.isEmpty());

    QCOMPARE(operations.containerValue(container, &evaluation).toString(), "int");
    QCOMPARE(operations.containerReference(container, &evaluation).toString(), "int &");
    QCOMPARE(operations.iteratorValue(iterator, &evaluation).toString(), "int");
    QCOMPARE(operations.iteratorReference(iterator, &evaluation).toString(), "int &");
    QCOMPARE(operations.rangeValue(container, &evaluation).toString(), "int");
    QCOMPARE(operations.rangeReference(container, &evaluation).toString(), "int &");
    QCOMPARE(operations.subscriptValue(container, &evaluation).toString(), "int");
    QCOMPARE(operations.subscriptReference(container, &evaluation).toString(), "int &");
    QCOMPARE(operations.rangeElement(container, false, &evaluation).toString(), "int");
    QCOMPARE(operations.rangeElement(container, true, &evaluation).toString(), "int &");
    TypeInfo reference = type("const int&");
    QCOMPARE(operations.valueFromReference(reference).toString(), "int");
    QVERIFY(operations.containerValue({}, &evaluation).qualifiedName.isEmpty());
    QVERIFY(operations.rangeElement({}, false, &evaluation).qualifiedName.isEmpty());
}

void TestTypeOperationRegistry::test_callable_tuple_and_view_operations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T> struct vector {};\n"
                "  template<typename T, typename U> struct pair {};\n"
                "  template<typename... T> struct tuple {};\n"
                "  template<typename T> struct array {};\n"
                "  struct Functor { string operator()(int); };\n"
                "  namespace ranges {\n"
                "    struct filter_fn {};\n"
                "    struct transform_fn {};\n"
                "    struct join_fn {};\n"
                "    template<typename T> struct filter_closure {};\n"
                "    template<typename T> struct transform_closure {};\n"
                "    template<typename T> struct join_closure {};\n"
                "    template<int> struct elements_fn {};\n"
                "    template<typename... T> struct filter_view {};\n"
                "    template<typename... T> struct transform_view {};\n"
                "    template<typename... T> struct join_view {};\n"
                "    template<typename... T> struct elements_view {};\n"
                "  }\n"
                "  namespace views { struct elements {}; }\n"
                "}\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/type_operations_callable.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppTypeResolver::EvaluationContext evaluation;
    auto type = [&](const QString &text) {
        DeclarationInfo declaration;
        if (!parseDeclarationText(text + " value;", declaration))
            return TypeInfo();
        TypeInfo result = declaration.specifiers.type;
        if (!declaration.declarators.isEmpty())
            result.derivations += declaration.declarators.first().derivations;
        return resolver.resolve(result);
    };
    TypeInfo functor = type("std::Functor");
    TypeInfo integer = type("int");
    CallResolution invocation;
    QCOMPARE(operations.invokeResult(functor, {integer}, &evaluation, &invocation).toString(),
             "std::string");
    QVERIFY(invocation.statement);
    QCOMPARE(operations.callableResult(functor, &evaluation).toString(), "std::string");
    TypeInfo function = integer;
    function.derivations.append(TypeDerivation::function());
    QCOMPARE(operations.callableResult(function, &evaluation).toString(), "int");
    QVERIFY(operations.callableResult({}, &evaluation).qualifiedName.isEmpty());

    TypeInfo tuple = type("std::tuple<int, std::string>");
    QCOMPARE(operations.tupleElement(tuple, 1, &evaluation).toString(), "std::string");
    QCOMPARE(operations.tupleElement(type("std::array<int>"), 5, &evaluation).toString(), "int");
    QCOMPARE(operations.tupleElement(type("std::pair<int, std::string>"), 0, &evaluation)
                     .toString(), "int");
    QVERIFY(operations.tupleElement(tuple, 2, &evaluation).qualifiedName.isEmpty());
    QVERIFY(operations.tupleElement(tuple, -1, &evaluation).qualifiedName.isEmpty());

    TypeInfo view = operations.makeView("std::ranges::filter_view",
                                        {TemplateArgument::fromType(integer)}, &evaluation);
    QCOMPARE(view.qualifiedName, "std::ranges::filter_view");
    QCOMPARE(view.templateArguments.count(), 1);
    QCOMPARE(view.templateArguments.first().type->qualifiedName, "int");
    EvalResult elements;
    elements.baseStatement = resolver.findStatement("std::views::elements");
    QVERIFY(elements.baseStatement);
    TypeInfo elementsFunction = operations.explicitTemplateResult(
                elements, {TemplateArgument::fromConstant("0")}, &evaluation);
    QCOMPARE(elementsFunction.qualifiedName, "std::ranges::elements_fn");
    elements.baseStatement = resolver.findStatement("std::Functor");
    QVERIFY(operations.explicitTemplateResult(
                elements, {TemplateArgument::fromConstant("0")}, &evaluation).qualifiedName.isEmpty());

    QCOMPARE(operations.invokeResult(type("std::ranges::filter_fn"), {integer}, &evaluation)
                     .qualifiedName, "std::ranges::filter_closure");
    QCOMPARE(operations.invokeResult(type("std::ranges::transform_fn"), {integer}, &evaluation)
                     .qualifiedName, "std::ranges::transform_closure");
    QCOMPARE(operations.invokeResult(type("std::ranges::join_fn"), {}, &evaluation)
                     .qualifiedName, "std::ranges::join_closure");

    TypeInfo input = type("std::vector<int>");
    const QList<QPair<QString, QString>> pipes{
        {"std::ranges::filter_closure", "std::ranges::filter_view"},
        {"std::ranges::transform_closure", "std::ranges::transform_view"},
        {"std::ranges::join_closure", "std::ranges::join_view"},
        {"std::ranges::elements_fn", "std::ranges::elements_view"}
    };
    for (const auto &pipe : pipes) {
        QString argument = pipe.first == "std::ranges::elements_fn" ? "<0>" : "<int>";
        TypeInfo adaptor = type(pipe.first + argument);
        QCOMPARE(adaptor.templateArguments.count(), 1);
        if (pipe.first == "std::ranges::elements_fn")
            QCOMPARE(adaptor.templateArguments.first().kind, TemplateArgumentKind::NonType);
        TypeInfo result = operations.pipeResult(input, adaptor, &evaluation);
        QCOMPARE(result.qualifiedName, pipe.second);
        int expectedArguments = pipe.first == "std::ranges::join_closure" ? 1 : 2;
        QCOMPARE(result.templateArguments.count(), expectedArguments);
    }
    QVERIFY(operations.pipeResult(input, {}, &evaluation).qualifiedName.isEmpty());
}
