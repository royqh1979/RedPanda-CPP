#include <QTest>
#include "test_cppstatementbuilder.h"

void TestCppStatementBuilder::test_statement_factory_initializes_statements()
{
    PStatement parent = StatementFactory::createNamespace(PStatement(), "Parent", "Parent");
    const QList<std::pair<StatementKind, PStatement>> statements = {
        {StatementKind::Unknown, StatementFactory::createUnknown(parent, "value", "Parent::value")},
        {StatementKind::Namespace, StatementFactory::createNamespace(parent, "value", "Parent::value")},
        {StatementKind::NamespaceAlias, StatementFactory::createNamespaceAlias(parent, "value", "Parent::value")},
        {StatementKind::Class, StatementFactory::createClass(parent, "value", "Parent::value")},
        {StatementKind::Preprocessor, StatementFactory::createPreprocessor(parent, "value", "Parent::value")},
        {StatementKind::EnumType, StatementFactory::createEnumType(parent, "value", "Parent::value")},
        {StatementKind::EnumClassType, StatementFactory::createEnumClassType(parent, "value", "Parent::value")},
        {StatementKind::Typedef, StatementFactory::createTypedef(parent, "value", "Parent::value")},
        {StatementKind::Constructor, StatementFactory::createConstructor(parent, "value", "Parent::value")},
        {StatementKind::Destructor, StatementFactory::createDestructor(parent, "value", "Parent::value")},
        {StatementKind::Function, StatementFactory::createFunction(parent, "value", "Parent::value")},
        {StatementKind::OverloadedOperator, StatementFactory::createOverloadedOperator(parent, "value", "Parent::value")},
        {StatementKind::LiteralOperator, StatementFactory::createLiteralOperator(parent, "value", "Parent::value")},
        {StatementKind::Variable, StatementFactory::createVariable(parent, "value", "Parent::value")},
        {StatementKind::GlobalVariable, StatementFactory::createGlobalVariable(parent, "value", "Parent::value")},
        {StatementKind::LocalVariable, StatementFactory::createLocalVariable(parent, "value", "Parent::value")},
        {StatementKind::Enum, StatementFactory::createEnum(parent, "value", "Parent::value")},
        {StatementKind::Operator, StatementFactory::createOperator(parent, "value", "Parent::value")},
        {StatementKind::Parameter, StatementFactory::createParameter(parent, "value", "Parent::value")},
        {StatementKind::Block, StatementFactory::createBlock(parent, "value", "Parent::value")},
        {StatementKind::Lambda, StatementFactory::createLambda(parent, "value", "Parent::value")},
        {StatementKind::Label, StatementFactory::createLabel(parent, "value", "Parent::value")},
        {StatementKind::UserCodeSnippet, StatementFactory::createUserCodeSnippet(parent, "value", "Parent::value")},
        {StatementKind::Keyword, StatementFactory::createKeyword(parent, "value", "Parent::value")},
        {StatementKind::KeywordType, StatementFactory::createKeywordType(parent, "value", "Parent::value")},
        {StatementKind::Alias, StatementFactory::createAlias(parent, "value", "Parent::value")}
    };
    for (const auto &entry : statements) {
        const PStatement &statement = entry.second;
        QVERIFY(statement);
        QCOMPARE(statement->kind, entry.first);
        QCOMPARE(statement->parentScope.lock(), parent);
        QCOMPARE(statement->command, "value");
        QCOMPARE(statement->fullName, "Parent::value");
        QCOMPARE(statement->scope, StatementScope::Global);
        QCOMPARE(statement->accessibility, StatementAccessibility::None);
        QCOMPARE(statement->usageCount, 0);
        QCOMPARE(statement->matchPosTotal, 0);
        QCOMPARE(statement->matchPosSpan, 0);
        QCOMPARE(statement->firstMatchLength, 0);
        QCOMPARE(statement->caseMatched, 0);
        if (entry.first == StatementKind::Namespace)
            QVERIFY(std::dynamic_pointer_cast<NamespaceStatement>(statement));
        else
            QVERIFY(std::dynamic_pointer_cast<Statement>(statement));
    }

    PStatement genericNamespace = StatementFactory::create(
                StatementKind::Namespace, parent, "Child", "Parent::Child");
    QVERIFY(std::dynamic_pointer_cast<NamespaceStatement>(genericNamespace));
    PStatement target = StatementFactory::createClass(parent, "Actual", "Parent::Actual");
    auto alias = StatementFactory::createInlineAlias(
                parent, "Actual", "Parent::Alias", target);
    QCOMPARE(alias->kind, StatementKind::InlineAlias);
    QCOMPARE(alias->target.lock(), target);
    QVERIFY(std::dynamic_pointer_cast<InlineAliasStatement>(alias));
}

void TestCppStatementBuilder::test_scope_range_queries()
{
    CppScopes scopes;
    auto outer = StatementFactory::createUnknown(PStatement(), QString(), QString());
    auto first = StatementFactory::createUnknown(PStatement(), QString(), QString());
    auto second = StatementFactory::createUnknown(PStatement(), QString(), QString());
    scopes.addScope({{0, 0}, {10, 0}}, outer);
    scopes.addScope({{2, 0}, {4, 0}}, first, 0);
    scopes.addScope({{6, 0}, {8, 0}}, second, 0);

    QCOMPARE(scopes.findScopeAt({3, 0}), first);
    QCOMPARE(scopes.findScopeAt({5, 0}), outer);
    QCOMPARE(scopes.findScopeAt({6, 0}), second);
    QVERIFY(!scopes.findScopeAt({11, 0}));
}

void TestCppStatementBuilder::test_scope_queries_same_line()
{
    CppScopes scopes;
    auto outer = StatementFactory::createUnknown(PStatement(), QString(), QString());
    auto first = StatementFactory::createUnknown(PStatement(), QString(), QString());
    auto second = StatementFactory::createUnknown(PStatement(), QString(), QString());
    scopes.addScope({{0, 0}, {0, 100}}, outer);
    scopes.addScope({{0, 10}, {0, 20}}, first, 0);
    scopes.addScope({{0, 30}, {0, 40}}, second, 0);

    QCOMPARE(scopes.findScopeAt({0, 15}), first);
    QCOMPARE(scopes.findScopeAt({0, 25}), outer);
    QCOMPARE(scopes.findScopeAt({0, 35}), second);
    QCOMPARE(scopes.findScopeAt({0, 99}), outer);
    QVERIFY(!scopes.findScopeAt({0, 101}));
}

void TestCppStatementBuilder::test_declaration_source_ranges()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("int f() { int value; }", declarations));
    QCOMPARE(declarations.count(), 1);
    QCOMPARE(declarations[0].declarators[0].position, (SourcePosition{0, 4}));
    QCOMPARE(declarations[0].range.start, (SourcePosition{0, 0}));
    QCOMPARE(declarations[0].range.end, (SourcePosition{0, 21}));
    QCOMPARE(declarations[0].bodyRange.start, (SourcePosition{0, 8}));
    QCOMPARE(declarations[0].bodyRange.end, (SourcePosition{0, 21}));

    declarations.clear();
    QVERIFY(parseSyntaxText("int main(){ int value;", declarations));
    QCOMPARE(declarations[0].bodyRange.start, (SourcePosition{0, 10}));
    QCOMPARE(declarations[0].bodyRange.end, (SourcePosition{0, 21}));
    QCOMPARE(declarations[0].range.end, (SourcePosition{0, 21}));
}


void TestCppStatementBuilder::test_statement_builder_expands_declarators_and_types()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("int a = 1, *p, f(double value), (*pf)(char);", declarations));
    QCOMPARE(declarations.count(), 1);

    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    builder.build(declarations.first(), context);

    QCOMPARE(model.count(), 5);
    const StatementMap &globals = model.childrenStatements();
    PStatement a = findStatement(globals, "a");
    PStatement p = findStatement(globals, "p");
    PStatement f = findStatement(globals, "f");
    PStatement pf = findStatement(globals, "pf");
    QVERIFY(a && p && f && pf);
    QCOMPARE(a->kind, StatementKind::Variable);
    QCOMPARE(statementType(a), "int");
    QCOMPARE(statementType(p), "int *");
    QCOMPARE(f->kind, StatementKind::Function);
    QCOMPARE(statementType(f), "int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(f)->args, "(double value)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(f)->normalizedArgs, "(double)");
    QVERIFY(pf->properties.testFlag(StatementProperty::FunctionPointer));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(pf)->typeInfo.toString(), "int (*)(char)");
    QCOMPARE(statementType(pf), "int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(pf)->args, "(char)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(pf)->normalizedArgs, "(char)");

    PStatement parameter = findStatement(std::dynamic_pointer_cast<Statement>(f)->children, "value");
    QVERIFY(parameter);
    QCOMPARE(parameter->kind, StatementKind::Parameter);
    QCOMPARE(statementType(parameter), "double");
    QCOMPARE(parameter->fullName, "f::value");
}

void TestCppStatementBuilder::test_statement_builder_names_anonymous_typedef_type()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("typedef struct { int xxx; } Test;", declarations));
    QCOMPARE(declarations.count(), 1);
    QCOMPARE(declarations.first().kind, DeclarationKind::Class);
    QCOMPARE(declarations.first().declarators.count(), 2);

    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    builder.build(declarations.first(), context);

    const StatementMap &globals = model.childrenStatements();
    PStatement alias = findStatement(globals, "Test");
    QVERIFY(alias);
    QCOMPARE(alias->kind, StatementKind::Typedef);
    QString typeName = std::dynamic_pointer_cast<Statement>(alias)->typeInfo.qualifiedName;
    QVERIFY(typeName.startsWith("__RED_PANDA_IDE__"));
    PStatement type = findStatement(globals, typeName);
    QVERIFY(type);
    QCOMPARE(type->kind, StatementKind::Class);
    QVERIFY(findStatement(std::dynamic_pointer_cast<Statement>(type)->children, "xxx"));
    CppTypeResolver resolver(model);
    QCOMPARE(resolver.resolve(std::dynamic_pointer_cast<Statement>(alias)->typeInfo).declaration.lock(),
             type);
}

void TestCppStatementBuilder::test_statement_builder_distinguishes_anonymous_typedef_types()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "typedef struct { int first; } First;\n"
                "typedef struct { int second; } Second;", declarations));
    QCOMPARE(declarations.count(), 2);
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);

    const StatementMap &globals = model.childrenStatements();
    PStatement firstAlias = findStatement(globals, "First");
    PStatement secondAlias = findStatement(globals, "Second");
    QVERIFY(firstAlias && secondAlias);
    QString firstTypeName = std::dynamic_pointer_cast<Statement>(firstAlias)->typeInfo.qualifiedName;
    QString secondTypeName = std::dynamic_pointer_cast<Statement>(secondAlias)->typeInfo.qualifiedName;
    QVERIFY(firstTypeName != secondTypeName);
    PStatement firstType = findStatement(globals, firstTypeName);
    PStatement secondType = findStatement(globals, secondTypeName);
    QVERIFY(firstType && secondType);
    QVERIFY(firstType != secondType);
    QVERIFY(findStatement(std::dynamic_pointer_cast<Statement>(firstType)->children, "first"));
    QVERIFY(!findStatement(std::dynamic_pointer_cast<Statement>(firstType)->children, "second"));
    QVERIFY(findStatement(std::dynamic_pointer_cast<Statement>(secondType)->children, "second"));
    QVERIFY(!findStatement(std::dynamic_pointer_cast<Statement>(secondType)->children, "first"));
    CppTypeResolver resolver(model);
    QCOMPARE(resolver.resolve(std::dynamic_pointer_cast<Statement>(firstAlias)->typeInfo).declaration.lock(),
             firstType);
    QCOMPARE(resolver.resolve(std::dynamic_pointer_cast<Statement>(secondAlias)->typeInfo).declaration.lock(),
             secondType);
}

void TestCppStatementBuilder::test_statement_builder_names_anonymous_variable_type()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("struct { int xxx; } test;", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    builder.build(declarations.first(), context);

    const StatementMap &globals = model.childrenStatements();
    PStatement variable = findStatement(globals, "test");
    QVERIFY(variable);
    QCOMPARE(variable->kind, StatementKind::Variable);
    QString typeName = std::dynamic_pointer_cast<Statement>(variable)->typeInfo.qualifiedName;
    QVERIFY(typeName.startsWith("__RED_PANDA_IDE__"));
    QVERIFY(typeName != "test");
    PStatement type = findStatement(globals, typeName);
    QVERIFY(type);
    QCOMPARE(type->kind, StatementKind::Class);
    QVERIFY(findStatement(std::dynamic_pointer_cast<Statement>(type)->children, "xxx"));
    CppTypeResolver resolver(model);
    QCOMPARE(resolver.resolve(std::dynamic_pointer_cast<Statement>(variable)->typeInfo).declaration.lock(),
             type);
}

void TestCppStatementBuilder::test_statement_builder_names_anonymous_enum_types()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("typedef enum { A } E;\nenum { B } e;", declarations));
    QCOMPARE(declarations.count(), 2);
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);

    const StatementMap &globals = model.childrenStatements();
    PStatement alias = findStatement(globals, "E");
    PStatement variable = findStatement(globals, "e");
    QVERIFY(alias && variable);
    QCOMPARE(alias->kind, StatementKind::Typedef);
    QCOMPARE(variable->kind, StatementKind::Variable);
    QString aliasTypeName = std::dynamic_pointer_cast<Statement>(alias)->typeInfo.qualifiedName;
    QString variableTypeName = std::dynamic_pointer_cast<Statement>(variable)->typeInfo.qualifiedName;
    QVERIFY(aliasTypeName.startsWith("__RED_PANDA_IDE__"));
    QVERIFY(variableTypeName.startsWith("__RED_PANDA_IDE__"));
    QVERIFY(aliasTypeName != variableTypeName);
    PStatement aliasType = findStatement(globals, aliasTypeName);
    PStatement variableType = findStatement(globals, variableTypeName);
    QVERIFY(aliasType && variableType);
    QCOMPARE(aliasType->kind, StatementKind::EnumType);
    QCOMPARE(variableType->kind, StatementKind::EnumType);
    QVERIFY(findStatement(std::dynamic_pointer_cast<Statement>(aliasType)->children, "A"));
    QVERIFY(findStatement(std::dynamic_pointer_cast<Statement>(variableType)->children, "B"));
}

void TestCppStatementBuilder::test_statement_builder_preserves_function_and_access_properties()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "class C {\n"
                "public:\n"
                "  int overload();\n"
                "  int overload() const;\n"
                "  int qualifiers() const volatile &;\n"
                "  int rvalue() &&;\n"
                "  static int count;\n"
                "protected:\n"
                "  void protectedMethod();\n"
                "private:\n"
                "  void privateMethod();\n"
                "};\n"
                "extern int external;\n"
                "static int internal;\n", declarations));

    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);

    PStatement c = findStatement(model.childrenStatements(), "C");
    QVERIFY(c);
    const auto overloads = std::dynamic_pointer_cast<Statement>(c)->children.values("overload");
    QCOMPARE(overloads.count(), 2);
    PStatement constOverload;
    PStatement nonConstOverload;
    for (const std::weak_ptr<AbstractStatement> &weak : overloads) {
        PStatement overload = weak.lock();
        QVERIFY(overload);
        if (overload->properties.testFlag(StatementProperty::Const))
            constOverload = overload;
        else
            nonConstOverload = overload;
    }
    QVERIFY(constOverload && nonConstOverload);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(constOverload)->args, "() const");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(constOverload)->normalizedArgs, "() const");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(nonConstOverload)->args, "()");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(nonConstOverload)->normalizedArgs, "()");

    PStatement qualifiers = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "qualifiers");
    QVERIFY(qualifiers);
    QVERIFY(qualifiers->properties.testFlag(StatementProperty::Const));
    QVERIFY(qualifiers->properties.testFlag(StatementProperty::Volatile));
    QVERIFY(qualifiers->properties.testFlag(StatementProperty::LvalueReference));
    QVERIFY(!qualifiers->properties.testFlag(StatementProperty::RvalueReference));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(qualifiers)->normalizedArgs, "() const volatile &");
    PStatement rvalue = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "rvalue");
    QVERIFY(rvalue);
    QVERIFY(rvalue->properties.testFlag(StatementProperty::RvalueReference));
    QVERIFY(!rvalue->properties.testFlag(StatementProperty::LvalueReference));

    PStatement count = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "count");
    QVERIFY(count && statementIsStatic(count));
    QVERIFY(std::dynamic_pointer_cast<Statement>(count)->declSpecifierProperties.testFlag(DeclSpecifierProperty::Static));
    QCOMPARE(findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "protectedMethod")->accessibility,
             StatementAccessibility::Protected);
    QCOMPARE(findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "privateMethod")->accessibility,
             StatementAccessibility::Private);
    PStatement external = findStatement(model.childrenStatements(), "external");
    QVERIFY(external);
    QVERIFY(std::dynamic_pointer_cast<Statement>(external)->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
    PStatement internal = findStatement(model.childrenStatements(), "internal");
    QVERIFY(internal && statementIsStatic(internal));
    QVERIFY(std::dynamic_pointer_cast<Statement>(internal)->declSpecifierProperties.testFlag(DeclSpecifierProperty::Static));
}

void TestCppStatementBuilder::test_statement_builder_preserves_scopes_positions_and_ancestors()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace N {\n"
                "  class C {\n"
                "  public:\n"
                "    C(int value);\n"
                "    int method() const { return 1; }\n"
                "    static int count;\n"
                "  };\n"
                "}\n", declarations));

    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/include.h";
    context.boundaryAncestor = "/tmp/main.cpp";
    context.inSystemHeader = true;
    builder.build(declarations.first(), context);

    PStatement n = findStatement(model.childrenStatements(), "N");
    QVERIFY(n);
    PStatement c = findStatement(model.childrenStatements(n), "C");
    QVERIFY(c);
    QCOMPARE(c->fullName, "N::C");
    QCOMPARE(c->scope, StatementScope::Global);
    QCOMPARE(c->definition.range.start, (SourcePosition{1, 2}));
    QVERIFY(statementHasDefinition(c));

    PStatement constructor = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "C");
    QVERIFY(constructor);
    QCOMPARE(constructor->kind, StatementKind::Constructor);
    QCOMPARE(constructor->accessibility, StatementAccessibility::Public);
    PStatement parameter = findStatement(std::dynamic_pointer_cast<Statement>(constructor)->children, "value");
    QVERIFY(parameter);
    QCOMPARE(parameter->definition.range.start.line, 3);

    PStatement method = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "method");
    QVERIFY(method);
    QCOMPARE(method->kind, StatementKind::Function);
    QCOMPARE(method->fullName, "N::C::method");
    QCOMPARE(method->definition.range.start, (SourcePosition{4, 4}));
    QCOMPARE(method->definition.fileName, "/tmp/include.h");
    QCOMPARE(method->definition.boundaryAncestor, "/tmp/main.cpp");
    QVERIFY(method->declarations.isEmpty());
    QVERIFY(statementInSystemHeader(method));

    PStatement count = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "count");
    QVERIFY(count);
    QCOMPARE(count->kind, StatementKind::Variable);
    QCOMPARE(count->scope, StatementScope::ClassLocal);
    QVERIFY(statementIsStatic(count));

    std::weak_ptr<AbstractStatement> countReference = count;
    model.clear();
    QVERIFY(std::dynamic_pointer_cast<Statement>(c)->children.isEmpty());
    count.reset();
    QVERIFY(countReference.expired());
}

void TestCppStatementBuilder::test_statement_builder_assigns_out_of_line_special_members_to_class()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace N {\n"
                "  template<typename T> class C {\n"
                "  public:\n"
                "    C();\n"
                "    ~C();\n"
                "    int operator()(int value) const;\n"
                "  };\n"
                "  template<typename T> C<T>::C() {}\n"
                "  template<typename T> C<T>::~C() {}\n"
                "  template<typename T> int C<T>::operator()(int value) const { return value; }\n"
                "}\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);

    PStatement n = findStatement(model.childrenStatements(), "N");
    QVERIFY(n);
    PStatement c = findStatement(model.childrenStatements(n), "C");
    QVERIFY(c);
    QCOMPARE(c->kind, StatementKind::Class);
    QCOMPARE(model.childrenStatements(n).values("C").count(), 1);
    PStatement constructor = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "C");
    PStatement destructor = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "~C");
    QVERIFY(constructor && destructor);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(c)->children.values("C").count(), 1);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(c)->children.values("~C").count(), 1);
    QVERIFY(statementHasDefinition(constructor));
    QVERIFY(statementHasDefinition(destructor));
    QCOMPARE(constructor->parentScope.lock(), c);
    QCOMPARE(destructor->parentScope.lock(), c);
    QCOMPARE(constructor->fullName, "N::C::C");
    QCOMPARE(destructor->fullName, "N::C::~C");
    QVERIFY(constructor->fullName != c->fullName);
    QVERIFY(destructor->fullName != c->fullName);
    PStatement callOperator = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "operator()");
    QVERIFY(callOperator);
    QCOMPARE(callOperator->kind, StatementKind::OverloadedOperator);
    QVERIFY(statementHasDefinition(callOperator));
    QCOMPARE(callOperator->parentScope.lock(), c);
    QCOMPARE(callOperator->fullName, "N::C::operator()");
    CppTypeResolver resolver(model);
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("C<int> value;", variable));
    QCOMPARE(resolver.resolve(variable.specifiers.type, {}, c).declaration.lock(), c);

    declarations.clear();
    QVERIFY(parseSyntaxText(
                "namespace N {\n"
                "  inline namespace V {\n"
                "    template<typename T> class InlineC { public: InlineC(); };\n"
                "  }\n"
                "}\n"
                "template<typename T> N::InlineC<T>::InlineC() {}\n",
                declarations));
    StatementModel inlineModel;
    CppStatementBuilder inlineBuilder(inlineModel);
    for (const DeclarationInfo &declaration : declarations)
        inlineBuilder.build(declaration, context);

    n = findStatement(inlineModel.childrenStatements(), "N");
    QVERIFY(n);
    PStatement v = findStatement(inlineModel.childrenStatements(n), "V");
    QVERIFY(v);
    c = findStatement(inlineModel.childrenStatements(v), "InlineC");
    QVERIFY(c);
    constructor = findStatement(inlineModel.childrenStatements(c), "InlineC");
    QVERIFY(constructor);
    QCOMPARE(constructor->parentScope.lock(), c);
    QCOMPARE(constructor->fullName, "N::V::InlineC::InlineC");
    QVERIFY(statementHasDefinition(constructor));
    QCOMPARE(inlineModel.childrenStatements(c).values("InlineC").count(), 1);
}


void TestCppStatementBuilder::test_statement_builder_merges_forward_class_with_definition()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> class Base { public: using value_type = T; };\n"
                "template<typename T, typename U = T> class C;\n"
                "template<typename T, typename U> class C : Base<U> {};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);

    PStatement c = findStatement(model.childrenStatements(), "C");
    QVERIFY(c);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(c)->baseClasses, QStringList{"Base<U>"});
    QCOMPARE(std::dynamic_pointer_cast<Statement>(c)->templateParameters.count(), 2);
    QVERIFY(std::dynamic_pointer_cast<Statement>(c)->templateParameters[1].defaultArgumentText.isEmpty());
    QCOMPARE(std::dynamic_pointer_cast<Statement>(c)->inheritedDefaultArgumentTexts[1], "T");
    CppTypeResolver resolver(model);
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("C<std::string> value;", variable));
    QCOMPARE(resolver.memberType(variable.specifiers.type, "value_type").toString(),
             "std::string");
}


void TestCppStatementBuilder::test_statement_model_owns_statements_and_weak_indexes()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("int value;", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    QList<std::weak_ptr<AbstractStatement>> built = builder.build(declarations.first(), context);
    QCOMPARE(built.count(), 1);
    QVERIFY(!built.first().expired());

    PStatement value = built.first().lock();
    ParsedFileInfo fileInfo(context.fileName);
    fileInfo.addStatement(value);
    CppScopes scopes;
    scopes.addScope({{0, 0}, {0, 9}}, value);
    QCOMPARE(fileInfo.statements().value("value").lock(), value);
    QCOMPARE(scopes.findScopeAt({0, 5}), value);

    fileInfo.removeStatement(value);
    model.deleteStatement(value);
    QVERIFY(!findStatement(model.childrenStatements(), "value"));
    QCOMPARE(model.count(), 0);
    value.reset();
    QVERIFY(built.first().expired());
    QVERIFY(!scopes.findScopeAt({0, 5}));
}


void TestCppStatementBuilder::test_statement_builder_preserves_array_suffixes()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("int values[2];", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    builder.build(declarations.first(), context);

    PStatement values = findStatement(model.childrenStatements(), "values");
    QVERIFY(values);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(values)->args, "[2]");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(values)->typeInfo.derivations.count(), 1);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(values)->typeInfo.derivations.first().kind, TypeDerivationKind::Array);
}


void TestCppStatementBuilder::test_statement_builder_registers_nested_body_scopes()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("namespace N { class C { int method() { return 1; } }; }", declarations));
    QCOMPARE(declarations.count(), 1);
    const DeclarationInfo &namespaceDeclaration = declarations.first();
    QCOMPARE(namespaceDeclaration.children.count(), 1);
    const DeclarationInfo &classDeclaration = namespaceDeclaration.children.first();
    QCOMPARE(classDeclaration.children.count(), 1);
    const DeclarationInfo &methodDeclaration = classDeclaration.children.first();

    StatementModel model;
    CppStatementBuilder builder(model);
    ParsedFileInfo fileInfo("/tmp/main.cpp");
    StatementBuildContext context;
    context.fileName = fileInfo.fileName();
    context.fileInfo = &fileInfo;
    builder.build(namespaceDeclaration, context);

    PStatement n = findStatement(model.childrenStatements(), "N");
    QVERIFY(n);
    PStatement c = findStatement(model.childrenStatements(n), "C");
    QVERIFY(c);
    PStatement method = findStatement(std::dynamic_pointer_cast<Statement>(c)->children, "method");
    QVERIFY(method);
    QCOMPARE(fileInfo.statements().value("N::C::method").lock(), method);
    QCOMPARE(fileInfo.findScopeAt(namespaceDeclaration.bodyRange.start), n);
    QCOMPARE(fileInfo.findScopeAt(classDeclaration.bodyRange.start), c);
    QCOMPARE(fileInfo.findScopeAt(methodDeclaration.range.start), c);
    QCOMPARE(fileInfo.findScopeAt(methodDeclaration.bodyRange.start), method);
    QCOMPARE(fileInfo.findScopeAt(methodDeclaration.bodyRange.end), method);
    QCOMPARE(fileInfo.findScopeAt(classDeclaration.bodyRange.end), c);
    QVERIFY(!fileInfo.findScopeAt({namespaceDeclaration.bodyRange.end.line,
                                   namespaceDeclaration.bodyRange.end.ch + 1}));
}

void TestCppStatementBuilder::test_statement_builder_scope_uses_recovered_file_end()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("int main(){ int value;", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    ParsedFileInfo fileInfo("/tmp/main.cpp");
    StatementBuildContext context;
    context.fileName = fileInfo.fileName();
    context.fileInfo = &fileInfo;
    builder.build(declarations.first(), context);

    PStatement main = findStatement(model.childrenStatements(), "main");
    QVERIFY(main);
    QVERIFY(statementHasDefinition(main));
    QCOMPARE(fileInfo.findScopeAt(declarations.first().bodyRange.start), main);
    QCOMPARE(fileInfo.findScopeAt(declarations.first().bodyRange.end), main);
}

void TestCppStatementBuilder::test_statement_builder_distinguishes_class_body_from_variable()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("struct S {} value;", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    builder.build(declarations.first(), context);

    PStatement s = findStatement(model.childrenStatements(), "S");
    PStatement value = findStatement(model.childrenStatements(), "value");
    QVERIFY(s && value);
    QVERIFY(statementHasDefinition(s));
    QVERIFY(statementHasDefinition(value));
    QVERIFY(value->declarations.isEmpty());
    QVERIFY(value->definition.range.isValid());
    QCOMPARE(statementType(value), "S");

    declarations.clear();
    QVERIFY(parseSyntaxText(
                "void f() { struct Local { ~Local() {} } value(this); }",
                declarations));
    StatementModel localModel;
    CppStatementBuilder localBuilder(localModel);
    localBuilder.build(declarations.first(), context);

    PStatement function = findStatement(localModel.childrenStatements(), "f");
    QVERIFY(function);
    PStatement local = findStatement(localModel.childrenStatements(function), "Local");
    value = findStatement(localModel.childrenStatements(function), "value");
    QVERIFY(local && value);
    QCOMPARE(value->kind, StatementKind::Variable);
    QCOMPARE(value->parentScope.lock(), function);
    PStatement destructor = findStatement(localModel.childrenStatements(local), "~Local");
    QVERIFY(destructor);
    QCOMPARE(destructor->parentScope.lock(), local);
    QCOMPARE(localModel.childrenStatements(local).values("~Local").count(), 1);
}


void TestCppStatementBuilder::test_function_scopes_auto_loops_catch_and_using()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  class string { public: unsigned long size(); };\n"
                "  class exception { public: const char* what() const; };\n"
                "  template<typename T> class iterator {\n"
                "  public:\n"
                "    iterator<T> operator++();\n"
                "    iterator<T> operator++(int);\n"
                "    T* operator*();\n"
                "    T* operator->();\n"
                "  };\n"
                "  template<typename T> class vector {\n"
                "  public:\n"
                "    using value_type = T;\n"
                "    T& front();\n"
                "    iterator<T> begin();\n"
                "    iterator<T> end();\n"
                "  };\n"
                "}\n"
                "std::vector<std::string> lst;\n"
                "void iterate() {\n"
                "  auto it = lst.begin();\n"
                "  while (++it != lst.end()) {\n"
                "    it->size();\n"
                "  }\n"
                "  for (auto x : lst) {\n"
                "    x.size();\n"
                "  }\n"
                "  for (auto i = lst.begin(); i != lst.end(); ++i) {\n"
                "    i->size();\n"
                "  }\n"
                "  auto ratio = 10 / 2.0;\n"
                "  auto flag = (1 < 2) && true;\n"
                "  auto bits = (1 << 2) | 4;\n"
                "  auto conditional = true ? 10 : 2.0; auto conditionalReverse = true ? 2.0 : 10;\n"
                "  auto floating = 1.0f; auto unsignedValue = 1U;\n"
                "  try { ratio = 1; }\n"
                "  catch (const std::exception &error) {\n"
                "    error.what();\n"
                "  }\n"
                "}\n"
                "void localUsing() {\n"
                "  using namespace std;\n"
                "  auto text = string();\n"
                "  using std::string;\n"
                "  auto text2 = string();\n"
                "}\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    ParsedFileInfo fileInfo("/tmp/main.cpp");
    StatementBuildContext context;
    context.fileName = fileInfo.fileName();
    context.fileInfo = &fileInfo;
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    auto evaluate = [&](const QString &text, const PStatement &scope) {
        CppTokenizer expression;
        expression.tokenize({text});
        return evaluator.evaluate(expression, scope);
    };
    auto findBlock = [&](const PStatement &function, int line) {
        const auto blocks = std::dynamic_pointer_cast<Statement>(function)->children.values("block");
        for (const std::weak_ptr<AbstractStatement> &weakBlock : blocks) {
            PStatement block = weakBlock.lock();
            if (block && statementDeclarationForNavigation(block, QString()).range.start.line == line)
                return block;
        }
        return PStatement();
    };

    PStatement iterate = findStatement(model.childrenStatements(), "iterate");
    QVERIFY(iterate);
    PStatement it = findStatement(std::dynamic_pointer_cast<Statement>(iterate)->children, "it");
    QVERIFY(it);
    QCOMPARE(statementType(it), "std::iterator<std::string>");
    QCOMPARE(statementType(findStatement(std::dynamic_pointer_cast<Statement>(iterate)->children, "ratio")), "double");
    QCOMPARE(statementType(findStatement(std::dynamic_pointer_cast<Statement>(iterate)->children, "flag")), "bool");
    QCOMPARE(statementType(findStatement(std::dynamic_pointer_cast<Statement>(iterate)->children, "bits")), "int");
    QCOMPARE(statementType(findStatement(std::dynamic_pointer_cast<Statement>(iterate)->children, "conditional")), "double");
    QCOMPARE(statementType(findStatement(std::dynamic_pointer_cast<Statement>(iterate)->children, "conditionalReverse")), "double");
    QCOMPARE(statementType(findStatement(std::dynamic_pointer_cast<Statement>(iterate)->children, "floating")), "float");
    QCOMPARE(statementType(findStatement(std::dynamic_pointer_cast<Statement>(iterate)->children, "unsignedValue")), "unsigned int");

    PStatement whileBlock = findBlock(iterate, 21);
    QVERIFY(whileBlock);
    QCOMPARE(evaluate("++it != lst.end()", whileBlock).type.toString(), "bool");
    QCOMPARE(evaluate("it->size()", whileBlock).type.toString(), "unsigned long");
    QCOMPARE(fileInfo.findScopeAt({22, 2}), whileBlock);

    PStatement rangeFor = findBlock(iterate, 24);
    QVERIFY(rangeFor);
    PStatement x = findStatement(std::dynamic_pointer_cast<Statement>(rangeFor)->children, "x");
    QVERIFY(x);
    QCOMPARE(statementType(x), "std::string");
    QCOMPARE(x->scope, StatementScope::Local);
    QCOMPARE(evaluate("x.size()", rangeFor).type.toString(), "unsigned long");
    QVERIFY(!evaluate("x.size()", iterate).isValid());
    QCOMPARE(fileInfo.findScopeAt({25, 2}), rangeFor);

    PStatement classicFor = findBlock(iterate, 27);
    QVERIFY(classicFor);
    PStatement i = findStatement(std::dynamic_pointer_cast<Statement>(classicFor)->children, "i");
    QVERIFY(i);
    QCOMPARE(statementType(i), "std::iterator<std::string>");
    QCOMPARE(evaluate("i->size()", classicFor).type.toString(), "unsigned long");
    QVERIFY(!evaluate("i->size()", iterate).isValid());

    PStatement catchBlock = findBlock(iterate, 36);
    QVERIFY(catchBlock);
    PStatement error = findStatement(std::dynamic_pointer_cast<Statement>(catchBlock)->children, "error");
    QVERIFY(error);
    QCOMPARE(statementType(error), "const std::exception &");
    QCOMPARE(evaluate("error.what()", catchBlock).type.toString(), "const char *");
    QVERIFY(!evaluate("error.what()", iterate).isValid());
    QCOMPARE(fileInfo.findScopeAt({37, 2}), catchBlock);

    PStatement localUsing = findStatement(model.childrenStatements(), "localUsing");
    QVERIFY(localUsing);
    QCOMPARE(statementType(findStatement(std::dynamic_pointer_cast<Statement>(localUsing)->children, "text")), "std::string");
    QCOMPARE(statementType(findStatement(std::dynamic_pointer_cast<Statement>(localUsing)->children, "text2")), "std::string");
}


void TestCppStatementBuilder::test_lambda_inference_preserves_source_position()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "auto callback = [](int value) { return value * 2.0; };\n"
                "auto mutableCallback = [](int value) mutable { return value; };",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);

    PStatement callback = findStatement(model.childrenStatements(), "callback");
    QVERIFY(callback);
    PStatement lambda = std::dynamic_pointer_cast<Statement>(callback)->typeInfo.declaration.lock();
    QVERIFY(lambda);
    QCOMPARE(lambda->kind, StatementKind::Lambda);
    QCOMPARE(lambda->definition.fileName, "/tmp/main.cpp");
    QCOMPARE(lambda->definition.range.start, (SourcePosition{0, 16}));
    PStatement call = findStatement(std::dynamic_pointer_cast<Statement>(lambda)->children, "operator()");
    QVERIFY(call);
    QCOMPARE(statementType(call), "double");
    QVERIFY(call->properties.testFlag(StatementProperty::Const));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(call)->normalizedArgs, "(int) const");
    PStatement value = findStatement(std::dynamic_pointer_cast<Statement>(call)->children, "value");
    QVERIFY(value);
    QCOMPARE(statementType(value), "int");
    PStatement mutableCallback = findStatement(model.childrenStatements(), "mutableCallback");
    QVERIFY(mutableCallback);
    PStatement mutableLambda = std::dynamic_pointer_cast<Statement>(mutableCallback)->typeInfo.declaration.lock();
    QVERIFY(mutableLambda);
    PStatement mutableCall = findStatement(std::dynamic_pointer_cast<Statement>(mutableLambda)->children, "operator()");
    QVERIFY(mutableCall);
    QVERIFY(!mutableCall->properties.testFlag(StatementProperty::Const));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mutableCall)->normalizedArgs, "(int)");

    CppTypeResolver resolver(model);
    TypeInfo intType;
    intType.qualifiedName = "int";
    MemberEvalResult directCall = resolver.invokeMember(
                std::dynamic_pointer_cast<Statement>(callback)->typeInfo, "operator()", {intType});
    QVERIFY(directCall.statement);
    QCOMPARE(directCall.type.toString(), "double");
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(model, resolver, operations);
    CppTokenizer expression;
    expression.tokenize({"callback(2)"});
    QCOMPARE(evaluator.evaluate(expression).type.toString(), "double");

    std::weak_ptr<AbstractStatement> lambdaReference = lambda;
    lambda.reset();
    model.clear();
    QVERIFY(lambdaReference.expired());
}


void TestCppStatementBuilder::test_normalized_template_function_signatures()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace N {\n"
                "  template<typename T, int Size, template<typename> class Container>\n"
                "  void f(Container<T> values, T (&items)[Size]);\n"
                "  template<typename U, int Length, template<typename> class Sequence>\n"
                "  void f(Sequence<U> values, U (&items)[Length]);\n"
                "  template<typename T> void f(T value);\n"
                "  template<typename T, int Size, template<typename> class Container>\n"
                "  void f(Container<T> values, T (&items)[Size], int extra);\n"
                "}\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    PStatement nameSpace = resolver.findStatement("N");
    QVERIFY(nameSpace);
    const auto overloads = model.childrenStatements(nameSpace).values("f");
    QCOMPARE(overloads.count(), 3);
    PStatement merged;
    int mergedCount = 0;
    for (const std::weak_ptr<AbstractStatement> &weak : overloads) {
        PStatement statement = weak.lock();
        if (statement && std::dynamic_pointer_cast<Statement>(statement)->parameters.count() == 2) {
            merged = statement;
            ++mergedCount;
        }
    }
    QCOMPARE(mergedCount, 1);
    QVERIFY(merged);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(merged)->normalizedArgs, "(@TT2<@T0>,@T0(&)[@N1])");
}


void TestCppStatementBuilder::test_nested_namespace_declaration()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("namespace A::B::C { int value; }", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    builder.build(declarations.first(), context);
    CppTypeResolver resolver(model);

    PStatement a = resolver.findStatement("A");
    QVERIFY(a);
    PStatement b = resolver.findStatement("A::B");
    QVERIFY(b);
    PStatement c = resolver.findStatement("A::B::C");
    QVERIFY(c);
    QCOMPARE(a->kind, StatementKind::Namespace);
    QCOMPARE(b->parentScope.lock(), a);
    QCOMPARE(c->parentScope.lock(), b);
    QVERIFY(resolver.findStatement("A::B::C::value"));
}


void TestCppStatementBuilder::test_template_forward_declaration_with_default_arguments()
{
    for (bool inlineFirst : {false, true}) {
        StatementModel model;
        CppStatementBuilder builder(model);
        QVector<DeclarationInfo> declarations;
        PStatement inlineNamespace;
        auto build = [&](const QString &text, const QString &fileName,
                         PStatement *built = nullptr) {
            declarations.clear();
            QVERIFY(parseSyntaxText(text, declarations));
            StatementBuildContext context;
            context.fileName = fileName;
            context.boundaryAncestor = fileName;
            QList<std::weak_ptr<AbstractStatement>> result = builder.build(declarations.first(), context);
            if (built && !result.isEmpty())
                *built = result.first().lock();
        };
        auto buildInline = [&] {
            build("namespace std {\n"
                  "inline namespace __cxx11 __attribute__((__abi_tag__(\"cxx11\"))) {}\n"
                  "}\n", "/tmp/c++config.h", &inlineNamespace);
        };
        auto buildMembers = [&] {
            build("namespace std {\n"
                  "namespace __cxx11 {\n"
                  "template<typename _CharT, typename _Traits = char_traits<_CharT>,\n"
                  "         typename _Alloc = allocator<_CharT> >\n"
                  "class basic_string;\n"
                  "template<typename T = int> class SimpleDefault;\n"
                  "template<typename T = std::pair<int, std::string>> class NestedClosing;\n"
                  "template<int N = 1 < 2> class Comparison;\n"
                  "template<int N = foo(1, 2)> class CallDefault;\n"
                  "}\n"
                  "typedef basic_string<char> string;\n"
                  "}\n", "/tmp/stringfwd.h");
        };
        if (inlineFirst) {
            buildInline();
            buildMembers();
        } else {
            buildMembers();
            buildInline();
        }
        CppTypeResolver resolver(model);

        PStatement string = resolver.findStatement("std::basic_string");
        QVERIFY(string);
        QCOMPARE(std::dynamic_pointer_cast<Statement>(string)->templateParameters.count(), 3);
        QCOMPARE(statementDeclarationForNavigation(string, QString()).range.start.line, 2);
        auto inlineParent = std::dynamic_pointer_cast<NamespaceStatement>(string->parentScope.lock());
        QVERIFY(inlineParent->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline));
        QCOMPARE(inlineParent->declarations.count(), 2);
        int inlineOccurrences = 0;
        for (const StatementOccurrence &occurrence : inlineParent->declarations)
            inlineOccurrences += occurrence.declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline) ? 1 : 0;
        QCOMPARE(inlineOccurrences, 1);
        QCOMPARE(std::dynamic_pointer_cast<Statement>(string)->templateParameters[1].defaultArgumentText, "char_traits<_CharT>");
        QCOMPARE(std::dynamic_pointer_cast<Statement>(string)->templateParameters[2].defaultArgumentText, "allocator<_CharT>");
        QVERIFY(resolver.findStatement("std::string"));

        PStatement simpleDefault = resolver.findStatement("std::SimpleDefault");
        QVERIFY(simpleDefault);
        QCOMPARE(std::dynamic_pointer_cast<Statement>(simpleDefault)->templateParameters[0].defaultArgumentText, "int");
        PStatement nestedClosing = resolver.findStatement("std::NestedClosing");
        QVERIFY(nestedClosing);
        QCOMPARE(std::dynamic_pointer_cast<Statement>(nestedClosing)->templateParameters[0].defaultArgumentText,
                 "std::pair<int, std::string>");
        PStatement comparison = resolver.findStatement("std::Comparison");
        QVERIFY(comparison);
        QCOMPARE(std::dynamic_pointer_cast<Statement>(comparison)->templateParameters[0].defaultArgumentText, "1 < 2");
        QVERIFY(std::dynamic_pointer_cast<Statement>(comparison)->templateParameters[0].type);
        QCOMPARE(std::dynamic_pointer_cast<Statement>(comparison)->templateParameters[0].type->qualifiedName, "int");
        PStatement callDefault = resolver.findStatement("std::CallDefault");
        QVERIFY(callDefault);
        QCOMPARE(std::dynamic_pointer_cast<Statement>(callDefault)->templateParameters[0].defaultArgumentText, "foo(1, 2)");

        inlineNamespace.reset();
        for (const PStatement &statement : model.statements()) {
            if (statement->kind == StatementKind::Namespace
                    && statement->fullName == "std::__cxx11"
                    && std::dynamic_pointer_cast<NamespaceStatement>(statement)->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline)) {
                inlineNamespace = statement;
                break;
            }
        }
        QVERIFY(inlineNamespace);
        model.removeNamespaceDeclarations(inlineNamespace, "/tmp/c++config.h");
        QCOMPARE(std::dynamic_pointer_cast<NamespaceStatement>(inlineNamespace)->declarations.count(), 1);
        QVERIFY(resolver.findStatement("std::basic_string"));
        QVERIFY(resolver.findStatement("std::__cxx11::basic_string"));
    }

    CppTokenizer relocated;
    relocated.tokenize({"template<typename T, typename D = default_delete<T>> class unique_ptr;"});
    relocated.relocateTokens("/tmp/relocated.h", {10, 0});
    CppTokenCursor cursor(relocated);
    CppDeclarationParser parser(cursor);
    DeclarationInfo declaration;
    QVERIFY(parser.parseDeclaration(declaration));
    QCOMPARE(declaration.templateParameters[1].defaultArgumentText, "default_delete<T>");
}


void TestCppStatementBuilder::test_scope_recovery_preserves_following_declarations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace N { class C { int value; public: constexpr C() : value() { } }; }\n"
                "int after;", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/recovery.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    QVERIFY(resolver.findStatement("N::C"));
    QVERIFY(resolver.findStatement("after"));
}


void TestCppStatementBuilder::test_integer_constant_expression_invalid_division()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "enum class E { divisionByZero = 1 / 0, moduloByZero = 1 % 0,"
                " overflow = (-9223372036854775807 - 1) / -1,"
                " unknownDivisor = 1 / unknown, unknownModulo = 1 % unknown, valid = 3 };",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/enum.cpp";
    builder.build(declarations.first(), context);
    CppTypeResolver resolver(model);

    QVERIFY(resolver.findStatement("E::divisionByZero"));
    QVERIFY(resolver.findStatement("E::moduloByZero"));
    QVERIFY(resolver.findStatement("E::overflow"));
    QVERIFY(resolver.findStatement("E::unknownDivisor"));
    QVERIFY(resolver.findStatement("E::unknownModulo"));
    PStatement valid = resolver.findStatement("E::valid");
    QVERIFY(valid);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(valid)->value, QString("3"));
}


void TestCppStatementBuilder::test_deduction_guides_are_consumed_without_statements()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std { inline namespace __cxx11 {\n"
                "template<typename T> class basic_string { public: unsigned long size(); };\n"
                "template<typename InputIterator, typename Allocator>\n"
                "basic_string(InputIterator, InputIterator, Allocator) -> basic_string<InputIterator>;\n"
                "template<typename T, typename Allocator>\n"
                "basic_string(basic_string_view<T>, const Allocator&) -> basic_string<T>;\n"
                "} }\n"
                "auto make_value(int) -> int;", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/guides.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    PStatement basicString = resolver.findStatement("std::basic_string");
    QVERIFY(basicString);
    QCOMPARE(basicString->kind, StatementKind::Class);
    QVERIFY(statementHasDefinition(basicString));
    QVERIFY(resolver.findStatement("std::basic_string::size"));
    QVERIFY(resolver.findStatement("make_value"));
    int entries = 0;
    for (const PStatement &statement : model.statements()) {
        if (statement->fullName == "std::__cxx11::basic_string") {
            ++entries;
            QCOMPARE(statement->kind, StatementKind::Class);
        }
    }
    QCOMPARE(entries, 1);
}

void TestCppStatementBuilder::test_unscoped_enumerators_have_outer_and_enum_scope()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "enum Color { red, green = 4, blue };\n"
                "enum class Scoped { value };\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/enum_scopes.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    PStatement color = resolver.findStatement("Color");
    QVERIFY(color);
    TypeInfo colorType;
    colorType.qualifiedName = colorType.qualifiedNameText = color->fullName;
    colorType.declaration = color;
    for (const QString &name : {QString("red"), QString("green"), QString("blue")}) {
        PStatement outer = resolver.findStatement(name);
        PStatement member = resolver.findStatement("Color::" + name);
        QVERIFY(outer && member);
        QVERIFY(outer != member);
        QCOMPARE(outer->kind, StatementKind::Enum);
        QCOMPARE(member->kind, StatementKind::Enum);
        QCOMPARE(member->parentScope.lock(), color);
        QCOMPARE(outer->definition.range, member->definition.range);
        QCOMPARE(std::dynamic_pointer_cast<Statement>(outer)->value,
                 std::dynamic_pointer_cast<Statement>(member)->value);
        QCOMPARE(resolver.memberStatement(colorType, name), member);
    }
    QCOMPARE(std::dynamic_pointer_cast<Statement>(resolver.findStatement("Color::green"))->value,
             "4");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(resolver.findStatement("blue"))->value, "5");
    QVERIFY(resolver.findStatement("Scoped::value"));
    QVERIFY(!resolver.findStatement("value"));
}
