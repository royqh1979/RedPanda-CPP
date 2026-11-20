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

void TestCppExpressionEvaluator::test_declval_preserves_type_and_outer_template_bindings()
{
    DeclarationInfo declvalDeclaration;
    QVERIFY(parseDeclarationText(
                "template<typename T> auto declval() noexcept -> decltype(__declval<T>(0));",
                declvalDeclaration));
    QCOMPARE(declvalDeclaration.declarators.first().derivations.first().trailingReturnType,
             "decltype(__declval<T>(0))");

    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T, typename U = T&&> U __declval(int);\n"
                "  template<typename T> T __declval(long);\n"
                "  template<typename T> auto declval() noexcept -> decltype(__declval<T>(0));\n"
                "}\n"
                "template<typename T> struct Holder {\n"
                "  using type = decltype(std::declval<T>());\n"
                "};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/declval.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    PStatement declvalStatement = resolver.findStatement("std::declval");
    QVERIFY(declvalStatement);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(declvalStatement)->typeInfo.qualifiedName,
             "decltype(__declval<T>(0))");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(declvalStatement)->typeInfo.qualifiedNameText,
             "decltype(__declval<T>(0))");
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    auto evaluate = [&](const QString &text) {
        CppTokenizer expression;
        expression.tokenize({text});
        return evaluator.evaluate(expression).type.toString();
    };

    QCOMPARE(evaluate("std::__declval<std::string>(0)"), "std::string &&");
    QCOMPARE(evaluate("std::declval<std::string>()"), "std::string &&");
    QCOMPARE(evaluate("std::declval<std::string&>()"), "std::string &");
    QCOMPARE(evaluate("std::declval<std::string&&>()"), "std::string &&");

    DeclarationInfo dependent;
    QVERIFY(parseDeclarationText("Holder<std::string> value;", dependent));
    QCOMPARE(resolver.memberType(dependent.specifiers.type, "type").toString(), "std::string &&");
}

void TestCppExpressionEvaluator::test_declval_with_direct_decltype_return()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T, typename U = T&&> U __declval(int);\n"
                "  template<typename T> T __declval(long);\n"
                "  template<typename T> decltype(std::__declval<T>(0)) declval() noexcept;\n"
                "}\n", declarations));
    QVERIFY(!declarations.isEmpty());
    bool parsedDeclval = false;
    for (const DeclarationInfo &declaration : declarations.first().children) {
        if (declaration.declarators.isEmpty()
                || declaration.declarators.first().name != "declval") {
            continue;
        }
        parsedDeclval = true;
        QCOMPARE(declaration.specifiers.type.qualifiedName, "decltype(std::__declval<T>(0))");
        QCOMPARE(declaration.specifiers.type.qualifiedNameText, "decltype(std::__declval<T>(0))");
    }
    QVERIFY(parsedDeclval);
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/direct_decltype_declval.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    PStatement declval = resolver.findStatement("std::declval");
    QVERIFY(declval);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(declval)->typeInfo.qualifiedName,
             "decltype(std::__declval<T>(0))");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(declval)->typeInfo.qualifiedNameText,
             "decltype(std::__declval<T>(0))");
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    CppTokenizer expression;
    expression.tokenize({"std::declval<std::string>()"});
    QCOMPARE(evaluator.evaluate(expression).type.toString(), "std::string &&");
}

void TestCppExpressionEvaluator::test_noexcept_evaluates_selected_calls()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "void throwing();\n"
                "void nonthrowing() noexcept;\n"
                "template<bool B> void conditional() noexcept(B);\n"
                "void overloaded() noexcept;\n"
                "void overloaded(int);\n"
                "struct Functor { void operator()() noexcept; };\n"
                "struct ThrowingFunctor { void operator()(); };\n"
                "Functor functor;\n"
                "ThrowingFunctor throwingFunctor;\n"
                "auto lambda = []() noexcept(true) {};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/noexcept.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    PStatement nonthrowing = resolver.findStatement("nonthrowing");
    QVERIFY(nonthrowing);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(nonthrowing)->noexceptKind,
             NoexceptKind::Always);
    auto evaluate = [&](const QString &text) {
        return resolver.evaluateNoexceptExpression("noexcept(" + text + ")");
    };

    QCOMPARE(evaluate("throwing()"), ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(evaluate("nonthrowing()"), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate("conditional<true>()"), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate("conditional<false>()"), ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(evaluate("overloaded()"), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate("overloaded(0)"), ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(evaluate("functor()"), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate("throwingFunctor()"), ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(evaluate("lambda()"), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate("1"), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate("missing()"), ConstraintSatisfaction::Unknown);
}

void TestCppExpressionEvaluator::test_evaluator_internal_lookup_and_results()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace Visible { int function(); int variable; }\n"
                "namespace Outer { using namespace Visible; }\n"
                "struct Foo { Foo(); int value; };\n"
                "using FooAlias = Foo;\n"
                "namespace V = Visible;\n"
                "int global;\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/evaluator_lookup.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);

    PStatement visible = resolver.findStatement("Visible");
    PStatement outer = resolver.findStatement("Outer");
    PStatement foo = resolver.findStatement("Foo");
    PStatement function = resolver.findStatement("Visible::function");
    PStatement variable = resolver.findStatement("Visible::variable");
    PStatement global = resolver.findStatement("global");
    QVERIFY(visible && outer && foo && function && variable && global);
    QCOMPARE(evaluator.findDirectStatement({}, "global"), global);
    QCOMPARE(evaluator.findDirectStatement(visible, "variable"), variable);
    QCOMPARE(evaluator.findDirectFunction(visible, "function"), function);
    QCOMPARE(evaluator.findVisibleStatement("variable", outer), variable);
    QCOMPARE(evaluator.findVisibleFunction("function", outer), function);
    QVERIFY(!evaluator.findVisibleStatement("missing", outer));

    CppTypeResolver::EvaluationContext evaluation;
    QCOMPARE(evaluator.createResult(global, &evaluation).kind, EvalStatementKind::Variable);
    QCOMPARE(evaluator.createResult(global, &evaluation).type.qualifiedName, "int");
    QCOMPARE(evaluator.createResult(function, &evaluation).kind, EvalStatementKind::Function);
    QCOMPARE(evaluator.createResult(foo, &evaluation).kind, EvalStatementKind::Type);
    QCOMPARE(evaluator.createResult(visible, &evaluation).kind, EvalStatementKind::Namespace);
    QCOMPARE(evaluator.createResult(resolver.findStatement("FooAlias"), &evaluation)
                     .type.qualifiedName, "Foo");
    QCOMPARE(evaluator.createResult(resolver.findStatement("V"), &evaluation).type.qualifiedName,
             "Visible");
    PStatement constructor = findStatement(std::dynamic_pointer_cast<Statement>(foo)->children, "Foo");
    QVERIFY(constructor);
    EvalResult constructed = evaluator.createResult(constructor, &evaluation);
    QCOMPARE(constructed.type.qualifiedName, "Foo");
    QCOMPARE(constructed.baseStatement, constructor);
    QVERIFY(!evaluator.createResult({}, &evaluation).isValid());
}

void TestCppExpressionEvaluator::test_evaluator_internal_token_parsing()
{
    StatementModel model;
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    CppTypeResolver::EvaluationContext evaluation;

    TypeInfo type = evaluator.parseTypeText("const std::pair<int, double>&");
    QCOMPARE(type.qualifiedName, "std::pair");
    QVERIFY(type.qualifiers.testFlag(TypeQualifier::Const));
    QVERIFY(!type.derivations.isEmpty());
    QCOMPARE(type.derivations.first().kind, TypeDerivationKind::LvalueReference);
    QVERIFY(evaluator.parseTypeText("").qualifiedName.isEmpty());

    CppTokenizer templateTokens;
    templateTokens.tokenize({"<int, 2, std::pair<int, double>>"});
    int index = 0;
    QVector<TemplateArgument> templateArguments;
    QVERIFY(evaluator.readTemplateArguments(templateTokens.tokens(), index, templateArguments));
    QCOMPARE(index, templateTokens.tokenCount());
    QCOMPARE(templateArguments.count(), 3);
    QCOMPARE(templateArguments[0].kind, TemplateArgumentKind::Type);
    QCOMPARE(templateArguments[0].type->qualifiedName, "int");
    QCOMPARE(templateArguments[1].kind, TemplateArgumentKind::NonType);
    QCOMPARE(templateArguments[1].constantExpression, "2");
    QCOMPARE(templateArguments[2].type->qualifiedName, "std::pair");
    CppTokenizer emptyTemplate;
    emptyTemplate.tokenize({"<>"});
    index = 0;
    templateArguments.clear();
    QVERIFY(!evaluator.readTemplateArguments(emptyTemplate.tokens(), index, templateArguments));

    CppTokenizer argumentTokens;
    argumentTokens.tokenize({"(1, 2.0, true)"});
    index = 0;
    QVector<TypeInfo> arguments;
    QVERIFY(evaluator.evaluateArguments(argumentTokens.tokens(), index, {}, arguments, &evaluation));
    QCOMPARE(index, argumentTokens.tokenCount());
    QCOMPARE(arguments.count(), 3);
    QCOMPARE(arguments[0].qualifiedName, "int");
    QCOMPARE(arguments[1].qualifiedName, "double");
    QCOMPARE(arguments[2].qualifiedName, "bool");
    CppTokenizer emptyArguments;
    emptyArguments.tokenize({"()"});
    index = 0;
    arguments.clear();
    QVERIFY(evaluator.evaluateArguments(emptyArguments.tokens(), index, {}, arguments, &evaluation));
    QVERIFY(arguments.isEmpty());
    CppTokenizer malformedArguments;
    malformedArguments.tokenize({"(1"});
    index = 0;
    QVERIFY(!evaluator.evaluateArguments(malformedArguments.tokens(), index, {}, arguments, &evaluation));

    CppTokenizer balanced;
    balanced.tokenize({"(1)"});
    QCOMPARE(evaluator.skipBalanced(balanced.tokens(), 0), 3);
    QCOMPARE(evaluator.skipBalanced(balanced.tokens(), 1), 2);
    QCOMPARE(evaluator.skipBalanced(balanced.tokens(), -1), balanced.tokenCount());

    const QList<QPair<QString, int>> precedences{
        {",", 1}, {"=", 2}, {"?", 3}, {"||", 4}, {"&&", 5}, {"|", 6},
        {"^", 7}, {"&", 8}, {"==", 9}, {"<=>", 10}, {"<<", 11},
        {"+", 12}, {"*", 13}, {".*", 14}, {"unknown", 0}
    };
    for (const auto &precedence : precedences)
        QCOMPARE(evaluator.binaryPrecedence(precedence.first), precedence.second);
    QVERIFY(evaluator.isRightAssociative("="));
    QVERIFY(!evaluator.isRightAssociative("+"));
    QVERIFY(!evaluator.isRightAssociative("?"));
}

void TestCppExpressionEvaluator::test_evaluator_internal_evaluation_stages()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "struct Foo { int value; int operator()(int); };\n"
                "int global;\n"
                "int function(int);\n"
                "Foo foo;\n"
                "Foo* pointer;\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/evaluator_stages.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    CppTypeResolver::EvaluationContext evaluation;
    auto tokens = [](const QString &text) {
        CppTokenizer tokenizer;
        tokenizer.tokenize({text});
        return tokenizer.tokens();
    };

    int index = 0;
    auto literalTokens = tokens("1");
    EvalResult result = evaluator.evaluatePrimary(literalTokens, index, {}, &evaluation);
    QCOMPARE(result.type.qualifiedName, "int");
    QCOMPARE(result.kind, EvalStatementKind::Literal);
    QCOMPARE(index, 1);
    index = 0;
    auto typeTokens = tokens("int");
    QCOMPARE(evaluator.evaluatePrimary(typeTokens, index, {}, &evaluation).kind,
             EvalStatementKind::Type);
    index = 0;
    auto variableTokens = tokens("global");
    QCOMPARE(evaluator.evaluatePrimary(variableTokens, index, {}, &evaluation).type.qualifiedName,
             "int");

    auto unary = [&](const QString &text) {
        int tokenIndex = 0;
        auto tokenList = tokens(text);
        return evaluator.evaluateUnary(tokenList, tokenIndex, {}, &evaluation);
    };
    QCOMPARE(unary("*pointer").type.toString(), "Foo &");
    QCOMPARE(unary("&global").type.toString(), "int *");
    QCOMPARE(unary("sizeof(int)").type.qualifiedName, "std::size_t");
    QCOMPARE(unary("new Foo").type.toString(), "Foo *");
    QCOMPARE(unary("static_cast<double>(1)").type.qualifiedName, "double");
    QCOMPARE(unary("noexcept(function(1))").type.qualifiedName, "bool");
    QVERIFY(!unary("noexcept(missing())").isValid());

    auto postfix = [&](const QString &text) {
        int tokenIndex = 0;
        auto tokenList = tokens(text);
        return evaluator.evaluatePostfix(tokenList, tokenIndex, {}, &evaluation);
    };
    QCOMPARE(postfix("foo.value").type.qualifiedName, "int");
    QCOMPARE(postfix("function(1)").type.qualifiedName, "int");
    QCOMPARE(postfix("foo(1)").type.qualifiedName, "int");
    index = 0;
    auto memberTokens = tokens(".value");
    EvalResult foo = evaluator.createResult(resolver.findStatement("Foo"), &evaluation);
    EvalResult variable = evaluator.createResult(resolver.findStatement("foo"), &evaluation);
    QCOMPARE(evaluator.evaluatePostfixTail(variable, memberTokens, index, {}, &evaluation)
                     .type.qualifiedName, "int");
    QCOMPARE(index, memberTokens.count());
    QVERIFY(foo.isValid());

    auto binary = [&](const QString &text) {
        int tokenIndex = 0;
        auto tokenList = tokens(text);
        EvalResult value = evaluator.evaluateBinary(tokenList, tokenIndex, {}, 1, &evaluation);
        if (tokenIndex != tokenList.count())
            return EvalResult();
        return value;
    };
    QCOMPARE(binary("1+2*3").type.qualifiedName, "int");
    QCOMPARE(binary("true?1:2.0").type.qualifiedName, "double");
    QCOMPARE(binary("1=2").type.qualifiedName, "int");

    CppTypeResolver::EvaluationContext lambdaEvaluation;
    lambdaEvaluation.allowModelChanges = true;
    index = 0;
    auto lambdaTokens = tokens("[](int value) -> int { return value; }");
    EvalResult lambda = evaluator.evaluateLambda(lambdaTokens, index, {}, &lambdaEvaluation);
    QVERIFY(lambda.isValid());
    QCOMPARE(lambda.type.declaration.lock()->kind, StatementKind::Lambda);
    QCOMPARE(index, lambdaTokens.count());
    index = 0;
    EvalResult cached = evaluator.evaluateLambda(lambdaTokens, index, {}, &lambdaEvaluation);
    QCOMPARE(cached.type.declaration.lock(), lambda.type.declaration.lock());
    index = 0;
    auto invalidLambdaTokens = tokens("[");
    QVERIFY(!evaluator.evaluateLambda(invalidLambdaTokens, index, {}, &lambdaEvaluation).isValid());
}

void TestCppExpressionEvaluator::test_evaluator_internal_type_operations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "struct Foo {\n"
                "  int value;\n"
                "  Foo* operator->();\n"
                "  int& operator*();\n"
                "  Foo& operator++();\n"
                "  int operator+(int);\n"
                "};\n"
                "Foo foo;\n"
                "Foo* pointer;\n"
                "int global;\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/evaluator_type_operations.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    CppTypeResolver::EvaluationContext evaluation;
    PStatement fooStatement = resolver.findStatement("Foo");
    EvalResult foo = evaluator.createResult(resolver.findStatement("foo"), &evaluation);
    EvalResult pointer = evaluator.createResult(resolver.findStatement("pointer"), &evaluation);
    EvalResult global = evaluator.createResult(resolver.findStatement("global"), &evaluation);
    QVERIFY(fooStatement && foo.isValid() && pointer.isValid() && global.isValid());

    QCOMPARE(evaluator.memberReceiver(foo, false, &evaluation).declaration, fooStatement);
    QCOMPARE(evaluator.memberReceiver(pointer, true, &evaluation).declaration, fooStatement);
    QCOMPARE(evaluator.memberReceiver(foo, true, &evaluation).declaration, fooStatement);
    QCOMPARE(evaluator.dereference(pointer, &evaluation).type.toString(), "Foo &");
    QCOMPARE(evaluator.dereference(foo, &evaluation).type.toString(), "int &");
    TypeInfo globalReference = global.type;
    globalReference.derivations.append(TypeDerivation::lvalueReference());
    QCOMPARE(evaluator.addressOf(globalReference, &evaluation).toString(), "int *");
    QCOMPARE(evaluator.applyIncrement(foo, "operator++", &evaluation).type.toString(), "Foo &");
    QCOMPARE(evaluator.applyIncrement(global, "operator++", &evaluation).type.qualifiedName, "int");
    TypeInfo rvalueReference = global.type;
    rvalueReference.derivations.append(TypeDerivation::rvalueReference());
    QCOMPARE(evaluator.removeReferences(rvalueReference).toString(), "int");

    EvalResult integer = evaluator.createResult(resolver.findStatement("global"), &evaluation);
    EvalResult floating = integer;
    floating.type.qualifiedName = "double";
    floating.instance = {};
    QCOMPARE(evaluator.applyBinary(integer, floating, "+", &evaluation).type.qualifiedName,
             "double");
    QCOMPARE(evaluator.applyBinary(integer, integer, "==", &evaluation).type.qualifiedName,
             "bool");
    QCOMPARE(evaluator.applyBinary(pointer, pointer, "-", &evaluation).type.qualifiedName,
             "std::ptrdiff_t");
    QCOMPARE(evaluator.applyBinary(integer, floating, ",", &evaluation).type.qualifiedName,
             "double");
    QCOMPARE(evaluator.applyBinary(foo, integer, "+", &evaluation).type.qualifiedName, "int");
    EvalResult memberPointer = integer;
    memberPointer.type.derivations.prepend(TypeDerivation::memberPointer("Foo"));
    QCOMPARE(evaluator.applyBinary(foo, memberPointer, ".*", &evaluation).type.qualifiedName,
             "int");
}
