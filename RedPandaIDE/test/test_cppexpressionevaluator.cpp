#include <QTest>
#include "test_cppexpressionevaluator.h"

void TestCppExpressionEvaluator::test_function_template_explicit_arguments()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string { };\n"
                "  template<typename T> struct shared_ptr {\n"
                "    T* operator->();\n"
                "    T& operator*();\n"
                "  };\n"
                "  template<typename T> struct default_delete {};\n"
                "  template<typename T, typename D = default_delete<T>> struct unique_ptr {\n"
                "    T* operator->();\n"
                "    T& operator*();\n"
                "  };\n"
                "  template<typename T> struct _MakeUniq { using __single_object = unique_ptr<T>; };\n"
                "  template<typename T> constexpr bool is_array_v = __is_array(T);\n"
                "  template<typename T> constexpr int extent_v = 0;\n"
                "  template<typename T> requires (!is_array_v<T>) using _NonArray = T;\n"
                "  template<typename T> requires (extent_v<T> != 0) using _BoundedArray = T[];\n"
                "  template<typename T, typename... Args>\n"
                "  shared_ptr<_NonArray<T>> make_shared(Args&&... args);\n"
                "  template<typename T> shared_ptr<_BoundedArray<T>> make_shared();\n"
                "  template<typename T> requires is_array_v<T> int select();\n"
                "  template<typename T> requires (!is_array_v<T>) int select();\n"
                "  template<typename T> int trailing() requires (!is_array_v<T>);\n"
                "  template<typename T> requires UnknownTrait<T> int unknown();\n"
                "  template<typename T> requires UnknownTrait<T> int ambiguous();\n"
                "  template<typename T> requires true int ambiguous();\n"
                "  template<typename T> requires (true || false) && (!is_array_v<T>) int logical();\n"
                "  template<typename T> constexpr bool custom_v = false;\n"
                "  template<typename T> constexpr bool custom_v<T[]> = true;\n"
                "  template<typename T> requires custom_v<T> int specialized();\n"
                "  template<typename T> requires (!is_array_v<T>) int declared();\n"
                "  template<typename T> requires ( ! is_array_v<T> ) int declared() { return 0; }\n"
                "  template<typename T, typename... Args>\n"
                "  typename _MakeUniq<T>::__single_object make_unique(Args&&... args) { return {}; }\n"
                "  template<typename T, typename... Args>\n"
                "  void make_unique(Args&&...) = delete;\n"
                "}\n"
                "auto shared = std::make_shared<std::string>();\n"
                "auto unique = std::make_unique<std::string>();\n", declarations, CppVersion::Cpp20));
    StatementModel model;
    const CppEvaluationOptions options{CppCompilerType::GCC, CppVersion::Cpp20};
    CppStatementBuilder builder(model, options);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);

    CppTypeResolver resolver(model, nullptr, options);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    auto evaluate = [&](const QString &text) {
        CppTokenizer expression;
        expression.tokenize({text});
        return evaluator.evaluate(expression);
    };
    DeclarationInfo stringVariable;
    QVERIFY(parseDeclarationText("std::string value;", stringVariable));
    PStatement isArray = resolver.findStatement("std::is_array_v");
    PStatement extent = resolver.findStatement("std::extent_v");
    PStatement nonArray = resolver.findStatement("std::_NonArray");
    PStatement boundedArray = resolver.findStatement("std::_BoundedArray");
    QVERIFY(isArray && extent && nonArray && boundedArray);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(isArray)->value, "__is_array(T)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(extent)->value, "0");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(nonArray)->templateRequiresClause,
             "(!is_array_v<T>)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(boundedArray)->templateRequiresClause,
             "(extent_v<T> != 0)");
    PStatement custom = resolver.findStatement("std::custom_v");
    QVERIFY(custom);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(custom)->value, "false");
    PStatement makeShared = resolver.findStatement("std::make_shared");
    QVERIFY(makeShared);
    QCOMPARE(makeShared->kind, StatementKind::Function);
    QCOMPARE(resolver.invokeFunction(makeShared, {},
             {TemplateArgument::fromType(stringVariable.specifiers.type)}).toString(),
             "std::shared_ptr<std::string>");
    QCOMPARE(evaluate("std::make_shared<std::string>()").type.toString(),
             "std::shared_ptr<std::string>");
    QCOMPARE(evaluate("std::select<std::string>()").type.toString(), "int");
    DeclarationInfo arrayVariable;
    QVERIFY(parseDeclarationText("std::string value[];", arrayVariable));
    TypeInfo arrayType = arrayVariable.specifiers.type;
    arrayType.derivations = arrayVariable.declarators.first().derivations;
    QCOMPARE(resolver.invokeFunction(resolver.findStatement("std::select"), {},
                                     {TemplateArgument::fromType(arrayType)}).toString(), "int");
    QCOMPARE(evaluate("std::select<std::string[]>()").type.toString(), "int");
    QCOMPARE(evaluate("std::trailing<std::string>()").type.toString(), "int");
    QCOMPARE(evaluate("std::unknown<std::string>()").type.toString(), "int");
    QCOMPARE(evaluate("std::logical<std::string>()").type.toString(), "int");
    QCOMPARE(evaluate("std::specialized<std::string[]>()").type.toString(), "int");
    QCOMPARE(evaluate("std::declared<std::string>()").type.toString(), "int");
    QVERIFY(evaluate("std::ambiguous<std::string>()").type.qualifiedName.isEmpty());
    DeclarationInfo boundedVariable;
    QVERIFY(parseDeclarationText("std::_BoundedArray<std::string> value;", boundedVariable));
    QVERIFY(resolver.resolve(boundedVariable.specifiers.type).qualifiedName.isEmpty());
    QCOMPARE(evaluate("std::make_unique<std::string>()").type.toString(),
             "std::unique_ptr<std::string,std::default_delete<std::string>>");
    QCOMPARE(evaluate("std::make_unique<std::string>(\"unique\")").type.toString(),
             "std::unique_ptr<std::string,std::default_delete<std::string>>");
    PStatement shared = findStatement(model.childrenStatements(), "shared");
    PStatement unique = findStatement(model.childrenStatements(), "unique");
    QVERIFY(shared && unique);
    QCOMPARE(statementType(shared), "std::shared_ptr<std::string>");
    QCOMPARE(statementType(unique), "std::unique_ptr<std::string,std::default_delete<std::string>>");

    CppTypeResolver cpp17Resolver(
                model, nullptr, {CppCompilerType::GCC, CppVersion::Cpp17});
    QVERIFY(cpp17Resolver.invokeFunction(makeShared, {},
            {TemplateArgument::fromType(stringVariable.specifiers.type)}).qualifiedName.isEmpty());
}


void TestCppExpressionEvaluator::test_expression_evaluator_follows_container_member_chain()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  class string { public: int value; unsigned long size(); };\n"
                "  template<typename T> class vector {\n"
                "  public:\n"
                "    using value_type = T;\n"
                "    T& front();\n"
                "    T& operator[](unsigned long);\n"
                "  };\n"
                "  template<typename T> class iterator {\n"
                "  public:\n"
                "    iterator<T> operator++(int);\n"
                "    iterator<T> operator+(int);\n"
                "    T* operator*();\n"
                "  };\n"
                "}\n"
                "std::vector<std::string> lst;\n"
                "std::iterator<std::string> it;\n"
                "std::string obj;\n"
                "std::string* ptr;\n"
                "int std::string::* member;\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);

    auto evaluate = [&](const QString &text) {
        CppTokenizer expression;
        expression.tokenize({text});
        return evaluator.evaluate(expression);
    };
    EvalResult front = evaluate("lst.front()");
    QCOMPARE(front.type.toString(), "std::string &");
    QCOMPARE(front.baseStatement->command, "lst");
    QCOMPARE(evaluate("lst.front().size()").type.toString(), "unsigned long");
    QCOMPARE(evaluate("lst[0]").type.toString(), "std::string &");
    QCOMPARE(evaluate("*it").type.toString(), "std::string *");
    QCOMPARE(evaluate("it++").type.toString(), "std::iterator<std::string>");
    QCOMPARE(evaluate("it + 1").type.toString(), "std::iterator<std::string>");
    QCOMPARE(evaluate("(*it++)->size()").type.toString(), "unsigned long");
    QCOMPARE(evaluate("std::string()").type.toString(), "std::string");
    QCOMPARE(evaluate("new std::string()").type.toString(), "std::string *");
    QCOMPARE(evaluate("(std::string*)nullptr").type.toString(), "std::string *");
    QCOMPARE(evaluate("ptr->").type.toString(), "std::string");
    QCOMPARE(evaluate("obj.*member").type.toString(), "int");
    QCOMPARE(evaluate("ptr->*member").type.toString(), "int");
}


void TestCppExpressionEvaluator::test_expression_evaluator_builds_range_adaptor_pipelines()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  class string { public: unsigned long size() const; };\n"
                "  template<typename T> class vector_iterator { public: T& operator*(); };\n"
                "  template<typename T> class vector {\n"
                "  public:\n"
                "    using value_type = T;\n"
                "    vector_iterator<T> begin();\n"
                "    T& front();\n"
                "  };\n"
                "  namespace ranges {\n"
                "    class filter_fn {};\n"
                "    class transform_fn {};\n"
                "    class join_fn {};\n"
                "    class elements_fn {};\n"
                "  }\n"
                "  namespace views {\n"
                "    std::ranges::filter_fn filter;\n"
                "    std::ranges::transform_fn transform;\n"
                "    std::ranges::join_fn join;\n"
                "    std::ranges::elements_fn elements;\n"
                "  }\n"
                "}\n"
                "class Pred { public: bool operator()(std::string); };\n"
                "class ToString { public: std::string operator()(std::string); };\n"
                "std::vector<std::string> lst;\n"
                "std::vector<std::vector<std::string>> nested;\n"
                "std::vector<std::pair<std::string, int>> pairs;\n"
                "std::vector<int> numbers;\n"
                "Pred pred;\n"
                "ToString convert;\n"
                "auto filtered = lst | std::views::filter(pred);\n"
                "auto transformed = filtered | std::views::transform(convert);\n"
                "auto joined = nested | std::views::join;\n"
                "auto keys = pairs | std::views::elements<0>;\n"
                "auto values = pairs | std::views::elements<1>;\n"
                "auto lambdaTransformed = lst | std::views::transform([](const std::string &text) { return text.size(); });\n"
                "auto arithmeticTransformed = numbers | std::views::transform([](int value) { return value * 2.0; });\n"
                "void useTransformed() { for (auto x : transformed) { x.size(); } }\n"
                "void useElements() { for (auto key : keys) { key.size(); } }\n"
                "void useLambda() { for (auto size : lambdaTransformed) { size; } }\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);

    PStatement filtered = findStatement(model.childrenStatements(), "filtered");
    PStatement transformed = findStatement(model.childrenStatements(), "transformed");
    PStatement joined = findStatement(model.childrenStatements(), "joined");
    QVERIFY(filtered && transformed && joined);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(filtered)->typeInfo.qualifiedName, "std::ranges::filter_view");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(transformed)->typeInfo.qualifiedName, "std::ranges::transform_view");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(joined)->typeInfo.qualifiedName, "std::ranges::join_view");
    QCOMPARE(operations.rangeValue(std::dynamic_pointer_cast<Statement>(filtered)->typeInfo).toString(), "std::string");
    QCOMPARE(operations.rangeValue(std::dynamic_pointer_cast<Statement>(transformed)->typeInfo).toString(), "std::string");
    QCOMPARE(operations.rangeValue(std::dynamic_pointer_cast<Statement>(joined)->typeInfo).toString(), "std::string");
    PStatement keys = findStatement(model.childrenStatements(), "keys");
    PStatement values = findStatement(model.childrenStatements(), "values");
    QVERIFY(keys && values);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(keys)->typeInfo.qualifiedName, "std::ranges::elements_view");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(values)->typeInfo.qualifiedName, "std::ranges::elements_view");
    QCOMPARE(operations.rangeValue(std::dynamic_pointer_cast<Statement>(keys)->typeInfo).toString(), "std::string");
    QCOMPARE(operations.rangeValue(std::dynamic_pointer_cast<Statement>(values)->typeInfo).toString(), "int");
    PStatement lambdaTransformed = findStatement(model.childrenStatements(), "lambdaTransformed");
    PStatement arithmeticTransformed = findStatement(model.childrenStatements(), "arithmeticTransformed");
    QVERIFY(lambdaTransformed && arithmeticTransformed);
    QCOMPARE(operations.rangeValue(std::dynamic_pointer_cast<Statement>(lambdaTransformed)->typeInfo).toString(), "unsigned long");
    QCOMPARE(operations.rangeValue(std::dynamic_pointer_cast<Statement>(arithmeticTransformed)->typeInfo).toString(), "double");

    PStatement useTransformed = findStatement(model.childrenStatements(), "useTransformed");
    QVERIFY(useTransformed);
    PStatement loop = findStatement(std::dynamic_pointer_cast<Statement>(useTransformed)->children, "block");
    QVERIFY(loop);
    PStatement x = findStatement(std::dynamic_pointer_cast<Statement>(loop)->children, "x");
    QVERIFY(x);
    QCOMPARE(statementType(x), "std::string");
    CppExpressionEvaluator evaluator(model, resolver, operations);
    CppTokenizer expression;
    expression.tokenize({"x.size()"});
    QCOMPARE(evaluator.evaluate(expression, loop).type.toString(), "unsigned long");

    PStatement useElements = findStatement(model.childrenStatements(), "useElements");
    QVERIFY(useElements);
    PStatement elementLoop = findStatement(std::dynamic_pointer_cast<Statement>(useElements)->children, "block");
    QVERIFY(elementLoop);
    PStatement key = findStatement(std::dynamic_pointer_cast<Statement>(elementLoop)->children, "key");
    QVERIFY(key);
    QCOMPARE(statementType(key), "std::string");
    PStatement useLambda = findStatement(model.childrenStatements(), "useLambda");
    QVERIFY(useLambda);
    PStatement lambdaLoop = findStatement(std::dynamic_pointer_cast<Statement>(useLambda)->children, "block");
    QVERIFY(lambdaLoop);
    PStatement size = findStatement(std::dynamic_pointer_cast<Statement>(lambdaLoop)->children, "size");
    QVERIFY(size);
    QCOMPARE(statementType(size), "unsigned long");
}


void TestCppExpressionEvaluator::test_function_template_argument_deduction_for_algorithms()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  class string { public: unsigned long size(); };\n"
                "  template<typename T> class iterator {\n"
                "  public:\n"
                "    T& operator*();\n"
                "    T* operator->();\n"
                "  };\n"
                "  template<typename T> class vector {\n"
                "  public:\n"
                "    iterator<T> begin();\n"
                "    iterator<T> end();\n"
                "  };\n"
                "  template<typename II, typename Pred>\n"
                "  II find_if(II, II, Pred);\n"
                "  template<typename InputIt, typename UnaryOp>\n"
                "  InputIt find_if(InputIt first, InputIt last, UnaryOp pred) { return first; }\n"
                "  template<typename InputIt, typename OutputIt, typename UnaryOp>\n"
                "  OutputIt transform(InputIt first, InputIt last, OutputIt result, UnaryOp op);\n"
                "  template<typename T> T front_value(std::vector<T> values);\n"
                "}\n"
                "std::vector<std::string> lst;\n"
                "auto found = std::find_if(lst.begin(), lst.end(), [](const std::string &s) { return s.size() > 0; });\n"
                "auto output = std::transform(lst.begin(), lst.end(), lst.begin(), [](const std::string &s) { return s.size(); });\n"
                "auto element = std::front_value(lst);\n"
                "auto dereferenced = *found;\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);

    PStatement found = findStatement(model.childrenStatements(), "found");
    PStatement output = findStatement(model.childrenStatements(), "output");
    PStatement element = findStatement(model.childrenStatements(), "element");
    PStatement dereferenced = findStatement(model.childrenStatements(), "dereferenced");
    QVERIFY(found && output && element && dereferenced);
    QCOMPARE(statementType(found), "std::iterator<std::string>");
    QCOMPARE(statementType(output), "std::iterator<std::string>");
    QCOMPARE(statementType(element), "std::string");
    QCOMPARE(statementType(dereferenced), "std::string");

    CppTypeResolver resolver(model);
    PStatement findIf = resolver.findStatement("std::find_if");
    QVERIFY(findIf);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(findIf)->normalizedArgs, "(@T0,@T0,@T1)");
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    CppTokenizer expression;
    expression.tokenize({"found->size()"});
    QCOMPARE(evaluator.evaluate(expression).type.toString(), "unsigned long");
}


void TestCppExpressionEvaluator::test_expression_evaluator_preserves_constructor_statement()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("class Foo { public: Foo(); };", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/constructor.cpp";
    builder.build(declarations.first(), context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    PStatement classStatement = resolver.findStatement("Foo");
    QVERIFY(classStatement);
    PStatement constructorStatement = findStatement(std::dynamic_pointer_cast<Statement>(classStatement)->children, "Foo");
    QVERIFY(constructorStatement);
    QCOMPARE(constructorStatement->kind, StatementKind::Constructor);

    CppTokenizer tokenizer;
    tokenizer.tokenize({"Foo"});
    EvalResult result = evaluator.evaluate(tokenizer, classStatement);
    QCOMPARE(result.kind, EvalStatementKind::Type);
    QCOMPARE(result.type.declaration.lock(), classStatement);
    QCOMPARE(result.statement, constructorStatement);
    QCOMPARE(result.baseStatement, constructorStatement);
}


void TestCppExpressionEvaluator::test_expression_evaluator_forbids_model_changes_outside_parse()
{
    StatementModel model;
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    CppTokenizer tokenizer;
    tokenizer.tokenize({"[](){ return 1; }()"});
    EvalResult result = evaluator.evaluate(tokenizer);
    QVERIFY(!result.isValid());
    QCOMPARE(model.count(), 0);
}
