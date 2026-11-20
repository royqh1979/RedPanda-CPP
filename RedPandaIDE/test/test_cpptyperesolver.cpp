#include <QTest>
#include "test_cpptyperesolver.h"

void TestCppTypeResolver::test_type_resolver_binds_custom_template_members()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> using Ptr = T*;\n"
                "template<typename T> class Box {\n"
                "public:\n"
                "  using ValueType = T;\n"
                "  T& front();\n"
                "  ValueType value();\n"
                "};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    DeclarationInfo pointerVariable;
    QVERIFY(parseDeclarationText("Ptr<std::string> pointer;", pointerVariable));
    QCOMPARE(resolver.resolve(pointerVariable.specifiers.type).toString(), "std::string *");
    DeclarationInfo pairVariable;
    QVERIFY(parseDeclarationText("Pair<Ptr<int>, Ptr<int>> pair;", pairVariable));
    TypeInfo pairType = resolver.resolve(pairVariable.specifiers.type);
    QCOMPARE(pairType.templateArguments.count(), 2);
    QCOMPARE(pairType.templateArguments[0].type->toString(), "int *");
    QCOMPARE(pairType.templateArguments[1].type->toString(), "int *");

    DeclarationInfo boxVariable;
    QVERIFY(parseDeclarationText("Box<std::string> lst;", boxVariable));
    const TypeInfo &boxType = boxVariable.specifiers.type;
    PStatement box = findStatement(model.childrenStatements(), "Box");
    QVERIFY(box);
    QCOMPARE(resolver.resolve(boxType).declaration.lock(), box);
    QCOMPARE(resolver.memberType(boxType, "ValueType").toString(), "std::string");
    QCOMPARE(resolver.invokeResult(boxType, "front").toString(), "std::string &");
    QCOMPARE(resolver.invokeResult(boxType, "value").toString(), "std::string");
    QVERIFY(resolver.invokeResult(boxType, "missing").qualifiedName.isEmpty());
}


void TestCppTypeResolver::test_type_resolver_applies_default_template_arguments()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T, typename U = T> class Box {\n"
                "public:\n"
                "  using ValueType = U;\n"
                "  U& front();\n"
                "};\n"
                "struct Value { using value_type = int; };\n"
                "template<typename T, typename = typename T::value_type> struct DependentDefault {\n"
                "  using reference = typename T::value_type&;\n"
                "  reference front();\n"
                "};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    DeclarationInfo boxVariable;
    QVERIFY(parseDeclarationText("Box<std::string> value;", boxVariable));
    PStatement box = findStatement(model.childrenStatements(), "Box");
    QVERIFY(box);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(box)->templateParameters[1].defaultArgumentText, "T");
    int statementCount = model.count();
    QCOMPARE(resolver.memberType(boxVariable.specifiers.type, "ValueType").toString(),
             "std::string");
    QCOMPARE(resolver.invokeResult(boxVariable.specifiers.type, "front").toString(),
             "std::string &");
    QCOMPARE(model.count(), statementCount);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(box)->templateParameters[1].defaultArgumentText, "T");
    DeclarationInfo dependentVariable;
    QVERIFY(parseDeclarationText("DependentDefault<Value> value;", dependentVariable));
    QCOMPARE(resolver.invokeResult(dependentVariable.specifiers.type, "front").toString(), "int &");
}


void TestCppTypeResolver::test_type_resolver_namespace_defaults_do_not_capture_caller_bindings()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "using U = double;\n"
                "namespace N {\n"
                "template<typename T = U> struct Identity { using type = T; };\n"
                "}\n"
                "template<typename U> struct Caller { using type = N::Identity; };\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    DeclarationInfo integerCaller;
    QVERIFY(parseDeclarationText("Caller<int> value;", integerCaller));
    TypeInfo integerIdentity = resolver.memberType(integerCaller.specifiers.type, "type");
    QCOMPARE(integerIdentity.toString(), "N::Identity<double>");
    QCOMPARE(resolver.memberType(integerIdentity, "type").toString(), "double");
    DeclarationInfo stringCaller;
    QVERIFY(parseDeclarationText("Caller<std::string> value;", stringCaller));
    TypeInfo stringIdentity = resolver.memberType(stringCaller.specifiers.type, "type");
    QCOMPARE(stringIdentity.toString(), "N::Identity<double>");
    QCOMPARE(resolver.memberType(stringIdentity, "type").toString(), "double");
}


void TestCppTypeResolver::test_member_lookup_cache_ignores_model_changes()
{
    StatementModel model;
    CppTypeResolver::EvaluationContext evaluation;
    CppTypeResolver resolver(model, &evaluation);
    auto nameSpace = StatementFactory::createNamespace(PStatement(), "N", "N");
    model.add(nameSpace);

    QVERIFY(!resolver.findStatement("N::Missing"));
    QVERIFY(!resolver.findStatement("N::Missing"));

    evaluation.allowModelChanges = true;
    auto missing = StatementFactory::createClass(nameSpace, "Missing", "N::Missing");
    model.add(missing);
    QCOMPARE(resolver.findStatement("N::Missing"), missing);
}


void TestCppTypeResolver::test_type_resolution_cache_respects_bindings_and_switches()
{
    StatementModel model;
    CppTypeResolver resolver(model);
    TypeInfo dependent;
    dependent.qualifiedName = "T";
    dependent.qualifiedNameText = "T";
    DeclarationInfo stringDeclaration;
    DeclarationInfo intDeclaration;
    DeclarationInfo doubleDeclaration;
    QVERIFY(parseDeclarationText("std::string value;", stringDeclaration));
    QVERIFY(parseDeclarationText("int value;", intDeclaration));
    QVERIFY(parseDeclarationText("double value;", doubleDeclaration));
    TemplateBindings stringBindings;
    stringBindings.insert("T", TemplateArgument::fromType(stringDeclaration.specifiers.type));
    TemplateBindings intBindings;
    intBindings.insert("T", TemplateArgument::fromType(intDeclaration.specifiers.type));
    TemplateBindings doubleBindings;
    doubleBindings.insert("T", TemplateArgument::fromType(doubleDeclaration.specifiers.type));

    CppTypeResolver::EvaluationContext evaluation;
    QCOMPARE(resolver.resolveInstance(dependent, stringBindings, {}, &evaluation).type.toString(),
             "std::string");
    QCOMPARE(resolver.resolveInstance(dependent, intBindings, {}, &evaluation).type.toString(), "int");
    QCOMPARE(resolver.resolveInstance(dependent, stringBindings, {}, &evaluation).type.toString(),
             "std::string");
    int cacheCount = evaluation.resolvedTypes->count();

    evaluation.caches.setFlag(CacheLevel::ResolvedType, false);
    QCOMPARE(resolver.resolveInstance(dependent, doubleBindings, {}, &evaluation).type.toString(),
             "double");
    QCOMPARE(evaluation.resolvedTypes->count(), cacheCount);
}


void TestCppTypeResolver::test_type_resolver_resolves_multisegment_aliases()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> struct Outer {\n"
                "  template<typename U> struct Inner { using type = U; };\n"
                "};\n"
                "template<typename T> struct Traits {\n"
                "  using value_type = T;\n"
                "  using reference = T&;\n"
                "};\n"
                "template<typename T> struct Vector {\n"
                "  using traits = Traits<T>;\n"
                "  typename traits::reference front();\n"
                "  typename Outer<T>::template Inner<std::string>::type nested();\n"
                "};\n"
                "struct Value { using value_type = int; };\n"
                "template<typename T> struct Dependent { typename T::value_type get(); };\n"
                "template<typename T> struct BaseValue { using value_type = T; };\n"
                "template<typename T> struct AliasDerived {\n"
                "  using BaseType = BaseValue<T>;\n"
                "  using value_type = typename BaseType::value_type;\n"
                "};\n"
                "namespace Reopened {}\n"
                "namespace Reopened { struct Value { using value_type = int; }; }\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    DeclarationInfo vectorVariable;
    QVERIFY(parseDeclarationText("Vector<int> values;", vectorVariable));
    QCOMPARE(resolver.invokeResult(vectorVariable.specifiers.type, "front").toString(), "int &");
    QCOMPARE(resolver.invokeResult(vectorVariable.specifiers.type, "nested").toString(),
             "std::string");
    DeclarationInfo dependentVariable;
    QVERIFY(parseDeclarationText("Dependent<Value> value;", dependentVariable));
    QCOMPARE(resolver.invokeResult(dependentVariable.specifiers.type, "get").toString(), "int");
    DeclarationInfo aliasDerived;
    QVERIFY(parseDeclarationText("AliasDerived<std::string> value;", aliasDerived));
    QCOMPARE(resolver.memberType(aliasDerived.specifiers.type, "value_type").toString(),
             "std::string");
    DeclarationInfo aliasVariable;
    QVERIFY(parseDeclarationText(
                "typename Outer<int>::template Inner<std::string>::type value;", aliasVariable));
    QCOMPARE(resolver.resolve(aliasVariable.specifiers.type).toString(), "std::string");
    DeclarationInfo reopenedVariable;
    QVERIFY(parseDeclarationText("Reopened::Value value;", reopenedVariable));
    PStatement reopenedDeclaration = resolver.resolve(reopenedVariable.specifiers.type).declaration.lock();
    QVERIFY(reopenedDeclaration);
    QCOMPARE(reopenedDeclaration->fullName, "Reopened::Value");
}


void TestCppTypeResolver::test_type_resolver_binds_dependent_base_members()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> struct Base {\n"
                "  Base();\n"
                "  using reference = T&;\n"
                "  reference get();\n"
                "};\n"
                "template<typename T> struct Derived : Base<T> {};\n"
                "template<typename T> struct StdAlloc { using value_type = T; };\n"
                "template<typename A> struct Traits { using value_type = typename A::value_type; };\n"
                "template<typename A, typename = typename A::value_type> struct Alloc : Traits<A> {\n"
                "  using reference = value_type&;\n"
                "  template<typename U> struct rebind { using other = StdAlloc<U>; };\n"
                "};\n"
                "template<typename T, typename _Alloc> struct ListBase {\n"
                "  using _Tp_alloc_type = typename Alloc<_Alloc>::template rebind<T>::other;\n"
                "  using _Tp_alloc_traits = Alloc<_Tp_alloc_type>;\n"
                "};\n"
                "template<typename T, typename _Allocator = StdAlloc<T>> struct List;\n"
                "template<typename T, typename _Alloc2> struct List;\n"
                "template<typename T, typename _Alloc> struct List : ListBase<T, _Alloc> {\n"
                "  using _Base = ListBase<T, _Alloc>;\n"
                "  using _Tp_alloc_traits = typename _Base::_Tp_alloc_traits;\n"
                "  using reference = typename _Tp_alloc_traits::reference;\n"
                "  reference front();\n"
                "};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    DeclarationInfo derivedVariable;
    QVERIFY(parseDeclarationText("Derived<std::string> value;", derivedVariable));
    QCOMPARE(resolver.memberType(derivedVariable.specifiers.type, "reference").toString(),
             "std::string &");
    QCOMPARE(resolver.invokeResult(derivedVariable.specifiers.type, "get").toString(),
             "std::string &");
    DeclarationInfo allocVariable;
    QVERIFY(parseDeclarationText("Alloc<StdAlloc<std::string>> value;", allocVariable));
    QCOMPARE(resolver.memberType(allocVariable.specifiers.type, "reference").toString(),
             "std::string &");
    DeclarationInfo listBaseVariable;
    QVERIFY(parseDeclarationText("ListBase<std::string, StdAlloc<std::string>> value;", listBaseVariable));
    QCOMPARE(resolver.memberType(listBaseVariable.specifiers.type, "_Tp_alloc_type").toString(),
             "StdAlloc<std::string>");
    QCOMPARE(resolver.memberType(listBaseVariable.specifiers.type, "_Tp_alloc_traits").toString(),
             "Alloc<StdAlloc<std::string>,std::string>");
    PStatement listClass = resolver.findStatement("List");
    QVERIFY(listClass);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(listClass)->templateParameters[1].name, "_Alloc");
    QVERIFY(std::dynamic_pointer_cast<Statement>(listClass)->templateParameters[1].defaultArgumentText.isEmpty());
    QCOMPARE(std::dynamic_pointer_cast<Statement>(listClass)->inheritedDefaultArgumentTexts[1], "StdAlloc<T>");
    DeclarationInfo listVariable;
    QVERIFY(parseDeclarationText("List<std::string> value;", listVariable));
    QCOMPARE(resolver.memberType(listVariable.specifiers.type, "reference").toString(),
             "std::string &");
    QCOMPARE(resolver.invokeResult(listVariable.specifiers.type, "front").toString(),
             "std::string &");
}


void TestCppTypeResolver::test_type_resolver_reference_collapsing_and_cycles()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std { struct string {}; }\n"
                "template<typename T> struct Forward { T&& get(); };\n"
                "using First = Second;\n"
                "using Second = First;\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    DeclarationInfo rvalue;
    QVERIFY(parseDeclarationText("Forward<std::string> value;", rvalue));
    QCOMPARE(resolver.invokeResult(rvalue.specifiers.type, "get").toString(), "std::string &&");
    DeclarationInfo lvalue;
    QVERIFY(parseDeclarationText("Forward<std::string&> value;", lvalue));
    QCOMPARE(resolver.invokeResult(lvalue.specifiers.type, "get").toString(), "std::string &");
    DeclarationInfo cycle;
    QVERIFY(parseDeclarationText("First value;", cycle));
    QVERIFY(resolver.resolve(cycle.specifiers.type).qualifiedName.isEmpty());

    DeclarationInfo builtinReference;
    QVERIFY(parseDeclarationText("__add_lvalue_reference(std::string) value;", builtinReference));
    QCOMPARE(builtinReference.specifiers.type.qualifiedName,
             QString("__add_lvalue_reference(std::string)"));
    QVERIFY(resolver.resolve(builtinReference.specifiers.type).qualifiedName.isEmpty());
    CppTypeResolver clangResolver(
                model, nullptr, {CppCompilerType::Clang, CppVersion::Cpp17});
    QCOMPARE(clangResolver.resolve(builtinReference.specifiers.type).toString(), "std::string &");
    DeclarationInfo builtinPointer;
    QVERIFY(parseDeclarationText("__add_pointer(std::string) value;", builtinPointer));
    QCOMPARE(clangResolver.resolve(builtinPointer.specifiers.type).toString(), "std::string *");
    DeclarationInfo builtinExtent;
    QVERIFY(parseDeclarationText("__remove_extent(std::string[]) value;", builtinExtent));
    QCOMPARE(clangResolver.resolve(builtinExtent.specifiers.type).toString(), "std::string");
    DeclarationInfo builtinRemoveReference;
    QVERIFY(parseDeclarationText("__remove_reference_t(std::string&) value;", builtinRemoveReference));
    QCOMPARE(clangResolver.resolve(builtinRemoveReference.specifiers.type).toString(), "std::string");
    DeclarationInfo builtinRemoveCv;
    QVERIFY(parseDeclarationText("__remove_cv_t(const std::string) value;", builtinRemoveCv));
    QVERIFY(!builtinRemoveCv.specifiers.type.qualifiers.testFlag(TypeQualifier::Const));
    QCOMPARE(clangResolver.resolve(builtinRemoveCv.specifiers.type).toString(), "std::string");

    DeclarationInfo dependent;
    QVERIFY(parseDeclarationText("std::move_iterator<_InputIterator> value;", dependent));
    TemplateBindings bindings;
    bindings.insert("_InputIterator", TemplateArgument::fromType(dependent.specifiers.type));
    QVERIFY(resolver.resolve(dependent.specifiers.type, bindings).qualifiedName.isEmpty());
}


void TestCppTypeResolver::test_type_resolver_selects_specializations_and_rebind()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> struct Allocator {\n"
                "  using value_type = T;\n"
                "  template<typename U> struct rebind { using other = Allocator<U>; };\n"
                "};\n"
                "template<typename T, typename = typename T::value_type>\n"
                "struct AllocTraits : Allocator<T> {};\n"
                "template<typename T> struct Holder {\n"
                "  using alloc = AllocTraits<Allocator<T>>;\n"
                "  typename alloc::template rebind<std::string>::other::value_type rebound();\n"
                "  typename alloc::template rebind<T>::other::value_type current();\n"
                "};\n"
                "template<typename T> struct Traits { using type = T; };\n"
                "template<typename T> struct Traits<T*> { using type = T; };\n"
                "template<> struct Traits<int> { using type = std::string; };\n"
                "template<typename T, typename U> struct BoolTraits { using type = T; };\n"
                "template<typename U> struct BoolTraits<bool, U> { using type = std::string; };\n"
                "template<int N> struct Size { using type = N; };\n"
                "template<> struct Size<2> { using type = std::string; };\n"
                "template<typename T> struct Ambiguous { using type = T; };\n"
                "template<typename T> struct Ambiguous<T*> { using type = T; };\n"
                "template<typename T> struct Ambiguous<const T> { using type = T; };\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    auto memberType = [&](const QString &declarationText) {
        DeclarationInfo variable;
        if (!parseDeclarationText(declarationText, variable))
            return TypeInfo();
        return resolver.memberType(variable.specifiers.type, "type");
    };

    PStatement pointerSpecialization = findStatement(model.childrenStatements(), "Traits<T*>");
    QVERIFY(pointerSpecialization);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(pointerSpecialization)->templateKind, TemplateDeclarationKind::PartialSpecialization);
    DeclarationInfo holder;
    QVERIFY(parseDeclarationText("Holder<int> value;", holder));
    QCOMPARE(resolver.invokeResult(holder.specifiers.type, "rebound").toString(), "std::string");
    QCOMPARE(resolver.invokeResult(holder.specifiers.type, "current").toString(), "int");
    QCOMPARE(memberType("Traits<int> value;").toString(), "std::string");
    QCOMPARE(memberType("Traits<int*> value;").toString(), "int");
    QCOMPARE(memberType("Traits<std::string> value;").toString(), "std::string");
    QCOMPARE(memberType("BoolTraits<bool, int> value;").toString(), "std::string");
    QCOMPARE(memberType("Size<2> value;").toString(), "std::string");
    QVERIFY(memberType("Size<1+1> value;").qualifiedName.isEmpty());
    QVERIFY(memberType("Ambiguous<const int*> value;").qualifiedName.isEmpty());
}


void TestCppTypeResolver::test_type_resolver_preserves_fixed_arguments_in_specialization_scope()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T, typename A> struct Vector { using reference = T; };\n"
                "template<typename A> struct Vector<bool, A> { using reference = Vector; };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("Vector<bool, int> value;", variable));
    TypeInfo reference = resolver.memberType(variable.specifiers.type, "reference");
    QCOMPARE(reference.qualifiedName.section('<', 0, 0), QString("Vector"));
    QCOMPARE(reference.templateArguments.count(), 2);
    QCOMPARE(reference.templateArguments[0].type->toString(), QString("bool"));
    QCOMPARE(reference.templateArguments[1].type->toString(), QString("int"));
}


void TestCppTypeResolver::test_type_resolver_evaluates_static_const_member_default_argument()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> struct HasStorage { static const bool value = true; };\n"
                "template<typename T, bool = HasStorage<T>::value> struct Reference { using type = int; };\n"
                "template<typename T> struct Reference<T, false> {};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    PStatement hasStorage = findStatement(model.childrenStatements(), "HasStorage");
    QVERIFY(hasStorage);
    PStatement valueStatement = findStatement(
                std::dynamic_pointer_cast<Statement>(hasStorage)->children, "value");
    QVERIFY(valueStatement);
    QVERIFY(statementIsStatic(valueStatement));
    QVERIFY(std::dynamic_pointer_cast<Statement>(valueStatement)->typeInfo.qualifiers.testFlag(
            TypeQualifier::Const));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(valueStatement)->value, QString("true"));
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("Reference<int> value;", variable));
    QCOMPARE(resolver.memberType(variable.specifiers.type, "type").toString(), QString("int"));
}


void TestCppTypeResolver::test_type_resolver_evaluates_specialized_static_const_member_default_argument()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> struct HasStorage { static const bool value = false; };\n"
                "template<typename T> struct HasStorage<T*> { static const bool value = true; };\n"
                "template<typename T, bool = HasStorage<T>::value> struct Reference { using type = int; };\n"
                "template<typename T> struct Reference<T, false> {};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("Reference<int*> value;", variable));
    QCOMPARE(resolver.memberType(variable.specifiers.type, "type").toString(), QString("int"));
}


void TestCppTypeResolver::test_type_resolver_matches_nested_class_specialization_default_argument()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T, typename A> struct Vector {};\n"
                "template<typename A> struct Vector<bool, A> {};\n"
                "template<typename T> struct HasStorage { static const bool value = false; };\n"
                "template<typename A> struct HasStorage<Vector<bool, A>> { static const bool value = true; };\n"
                "template<typename T, bool = HasStorage<T>::value> struct Reference { using type = int; };\n"
                "template<typename T> struct Reference<T, false> {};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("Reference<Vector<bool, int>> value;", variable));
    QCOMPARE(resolver.memberType(variable.specifiers.type, "type").toString(), QString("int"));
}


void TestCppTypeResolver::test_type_resolver_matches_injected_specialization_default_argument()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T, typename A> struct Vector { using reference = T; };\n"
                "template<typename A> struct Vector<bool, A> { using reference = Vector; };\n"
                "template<typename T> struct HasStorage { static const bool value = false; };\n"
                "template<typename A> struct HasStorage<Vector<bool, A>> { static const bool value = true; };\n"
                "template<typename T, bool = HasStorage<typename T::reference>::value>\n"
                "struct Reference { using type = int; };\n"
                "template<typename T> struct Reference<T, false> {};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("Reference<Vector<bool, int>> value;", variable));
    QCOMPARE(resolver.memberType(variable.specifiers.type, "type").toString(), QString("int"));
}


void TestCppTypeResolver::test_type_resolver_selects_specializations_across_reopened_namespaces()
{
    QVector<DeclarationInfo> declarations;
    StatementModel model;
    CppStatementBuilder builder(model);
    auto build = [&](const QString &text, const QString &fileName) {
        declarations.clear();
        QVERIFY(parseSyntaxText(text, declarations));
        StatementBuildContext context;
        context.fileName = fileName;
        context.boundaryAncestor = fileName;
        builder.build(declarations.first(), context);
    };
    build("namespace N { template<typename T> struct Traits { using type = T; }; }", "/tmp/a.h");
    build("namespace N { template<typename T> struct Traits<T*> { using type = std::string; }; }",
          "/tmp/b.h");
    CppTypeResolver resolver(model);
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("N::Traits<int*> value;", variable));
    QCOMPARE(resolver.memberType(variable.specifiers.type, "type").toString(), "std::string");
}


void TestCppTypeResolver::test_type_resolver_computes_bounded_traits()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  template<typename T> struct add_lvalue_reference {};\n"
                "  template<typename T> struct is_array {};\n"
                "  template<typename T> struct is_void {};\n"
                "  template<bool Cond, typename T = void> struct enable_if {};\n"
                "  template<typename T> struct enable_if<true, T> { using type = T; };\n"
                "  template<bool Cond, typename T = void>\n"
                "  using __enable_if_t = typename enable_if<Cond, T>::type;\n"
                "  template<typename T>\n"
                "  using _NonArray = __enable_if_t<!is_array<T>::value, T>;\n"
                "}\n"
                "template<typename T> struct Pointer {\n"
                "  typename std::add_lvalue_reference<T>::type get();\n"
                "};\n"
                "template<typename T, bool Array = std::is_array<T>::value>\n"
                "struct Access { using type = std::string; };\n"
                "template<typename T> struct Access<T, false> { using type = T; };\n"
                "template<typename T, bool Void = std::is_void<T>::value>\n"
                "struct VoidAccess { using type = T; };\n"
                "template<typename T> struct VoidAccess<T, true> { using type = std::string; };\n"
                "enum Lock { Default };\n"
                "template<Lock L> struct LockBase {};\n"
                "template<Lock L> struct LockDerived : LockBase<L> { using type = int; };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    DeclarationInfo pointer;
    QVERIFY(parseDeclarationText("Pointer<std::string> value;", pointer));
    QCOMPARE(resolver.invokeResult(pointer.specifiers.type, "get").toString(), "std::string &");
    auto memberType = [&](const QString &declarationText) {
        DeclarationInfo variable;
        if (!parseDeclarationText(declarationText, variable))
            return TypeInfo();
        return resolver.memberType(variable.specifiers.type, "type");
    };
    auto resolvedType = [&](const QString &declarationText) {
        DeclarationInfo variable;
        if (!parseDeclarationText(declarationText, variable))
            return TypeInfo();
        return resolver.resolve(variable.specifiers.type);
    };
    PStatement access = findStatement(model.childrenStatements(), "Access");
    QVERIFY(access);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(access)->templateParameters[1].defaultArgumentText, "std::is_array<T>::value");
    QCOMPARE(memberType("Access<int,false> value;").toString(), "int");
    QCOMPARE(memberType("Access<int> value;").toString(), "int");
    QCOMPARE(memberType("Access<int[]> value;").toString(), "std::string");
    QCOMPARE(memberType("VoidAccess<void> value;").toString(), "std::string");
    QCOMPARE(memberType("VoidAccess<int> value;").toString(), "int");
    QCOMPARE(memberType("LockDerived<Default> value;").toString(), "int");
    QCOMPARE(memberType("std::enable_if<true, std::string> value;").toString(), "std::string");
    QCOMPARE(resolvedType("std::__enable_if_t<true, std::string> value;").toString(), "std::string");
    QCOMPARE(resolvedType("std::__enable_if_t<!std::is_array<std::string>::value, std::string> value;").toString(),
             "std::string");
    QCOMPARE(resolvedType("std::_NonArray<std::string> value;").toString(), "std::string");
    QVERIFY(resolvedType("std::_NonArray<std::string[]> value;").qualifiedName.isEmpty());
}


void TestCppTypeResolver::test_type_operations_resolve_container_and_iterator_chain()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  template<typename I> struct iterator_traits;\n"
                "  template<typename T> struct iterator_traits<T*> {\n"
                "    using value_type = T;\n"
                "    using pointer = T*;\n"
                "    using reference = T&;\n"
                "  };\n"
                "}\n"
                "namespace __gnu_cxx {\n"
                "  using std::iterator_traits;\n"
                "  template<typename I, typename C> class __normal_iterator {\n"
                "    using __traits_type = iterator_traits<I>;\n"
                "  public:\n"
                "    using value_type = typename __traits_type::value_type;\n"
                "    using pointer = typename __traits_type::pointer;\n"
                "    using reference = typename __traits_type::reference;\n"
                "    pointer operator->();\n"
                "    reference operator*();\n"
                "  };\n"
                "}\n"
                "namespace std { inline namespace __cxx11 {\n"
                "  template<typename T> class vector {\n"
                "  public:\n"
                "    using value_type = T;\n"
                "    using pointer = T*;\n"
                "    using iterator = __gnu_cxx::__normal_iterator<pointer, vector>;\n"
                "    using const_iterator = const T*;\n"
                "    iterator begin();\n"
                "    const_iterator begin() const;\n"
                "    T& front();\n"
                "    T& operator[](unsigned long);\n"
                "    const T& operator[](unsigned long) const;\n"
                "  };\n"
                "  template<typename T> class iterator {\n"
                "  public:\n"
                "    T& operator*();\n"
                "  };\n"
                "} }\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    PStatement vector = resolver.findStatement("std::vector");
    QVERIFY(vector);
    QCOMPARE(vector->fullName, "std::__cxx11::vector");
    PStatement inlineNamespace = resolver.findStatement("std::__cxx11");
    QVERIFY(inlineNamespace);
    QCOMPARE(inlineNamespace->fullName, "std::__cxx11");

    DeclarationInfo vectorVariable;
    QVERIFY(parseDeclarationText("std::vector<std::string> lst;", vectorVariable));
    const TypeInfo &vectorType = vectorVariable.specifiers.type;
    TypeInfo beginType = resolver.invokeResult(vectorType, "begin");
    QCOMPARE(beginType.qualifiedName, "__gnu_cxx::__normal_iterator");
    QCOMPARE(beginType.templateArguments.count(), 2);
    QCOMPARE(beginType.templateArguments[0].type->toString(), "std::string *");
    QCOMPARE(beginType.templateArguments[1].type->toString(), "std::__cxx11::vector<std::string>");
    DeclarationInfo constVectorVariable;
    QVERIFY(parseDeclarationText("const std::vector<std::string> constLst;", constVectorVariable));
    QCOMPARE(resolver.invokeResult(constVectorVariable.specifiers.type, "begin").toString(),
             "const std::string *");
    QCOMPARE(resolver.invokeResult(constVectorVariable.specifiers.type, "operator[]").toString(),
             "const std::string &");
    QCOMPARE(resolver.invokeResult(beginType, "operator->").toString(), "std::string *");
    QCOMPARE(resolver.invokeResult(beginType, "operator*").toString(), "std::string &");
    QCOMPARE(operations.iteratorReference(beginType).toString(), "std::string &");
    QCOMPARE(operations.iteratorValue(beginType).toString(), "std::string");
    QCOMPARE(operations.containerValue(vectorType).toString(), "std::string");
    QCOMPARE(operations.containerReference(vectorType).toString(), "std::string &");
    QCOMPARE(operations.rangeValue(vectorType).toString(), "std::string");
    QCOMPARE(operations.rangeReference(vectorType).toString(), "std::string &");
    QCOMPARE(operations.subscriptValue(vectorType).toString(), "std::string");
    QCOMPARE(operations.subscriptReference(vectorType).toString(), "std::string &");

    DeclarationInfo iteratorVariable;
    QVERIFY(parseDeclarationText("std::iterator<std::string> it;", iteratorVariable));
    QCOMPARE(operations.iteratorReference(iteratorVariable.specifiers.type).toString(),
             "std::string &");
    QCOMPARE(operations.iteratorValue(iteratorVariable.specifiers.type).toString(),
             "std::string");
}


void TestCppTypeResolver::test_type_operations_resolve_standard_range_views()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  template<typename T> class vector_iterator { public: T& operator*(); };\n"
                "  template<typename T> class vector {\n"
                "  public:\n"
                "    using value_type = T;\n"
                "    vector_iterator<T> begin();\n"
                "    T& front();\n"
                "  };\n"
                "}\n"
                "class ToString { public: std::string operator()(int); };\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeOperationRegistry operations(resolver);
    auto parseType = [](const QString &declaration) {
        DeclarationInfo result;
        CppTokenizer tokenizer;
        tokenizer.tokenize({declaration});
        CppTokenCursor cursor(tokenizer);
        CppDeclarationParser parser(cursor);
        if (!parser.parseDeclaration(result))
            return TypeInfo();
        return result.specifiers.type;
    };

    QCOMPARE(operations.rangeValue(parseType("std::ranges::iota_view<int> view;")).toString(),
             "int");
    TypeInfo filter = parseType(
                "std::ranges::filter_view<std::vector<std::string>, Pred> view;");
    QCOMPARE(operations.rangeValue(filter).toString(), "std::string");
    QCOMPARE(operations.rangeReference(filter).toString(), "std::string &");
    QCOMPARE(operations.rangeValue(parseType(
                 "std::ranges::transform_view<std::ranges::iota_view<int>, ToString> view;"))
             .toString(), "std::string");
    QCOMPARE(operations.rangeValue(parseType(
                 "std::ranges::join_view<std::vector<std::vector<std::string>>> view;"))
             .toString(), "std::string");
    QCOMPARE(operations.rangeValue(parseType(
                 "std::ranges::elements_view<std::vector<std::pair<std::string, int>>, 0> view;"))
             .toString(), "std::string");
    QCOMPARE(operations.rangeValue(parseType(
                 "std::ranges::elements_view<std::vector<std::pair<std::string, int>>, 1> view;"))
             .toString(), "int");
}


void TestCppTypeResolver::test_inline_namespace_member_lookup()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("namespace N { namespace Inner { int early; } inline namespace Inner { using String = int; } }", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    builder.build(declarations.first(), context);
    CppTypeResolver resolver(model);

    PStatement string = resolver.findStatement("N::String");
    QVERIFY(string);
    QCOMPARE(string->command, "String");
    QCOMPARE(string->fullName, "N::Inner::String");
    PStatement early = resolver.findStatement("N::Inner::early");
    QVERIFY(early);
}


void TestCppTypeResolver::test_reopened_namespace_lookup_checks_all_declarations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("namespace N { int first; }", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext firstContext;
    firstContext.fileName = "/tmp/a.h";
    firstContext.boundaryAncestor = "/tmp/a.h";
    builder.build(declarations.first(), firstContext);
    declarations.clear();
    QVERIFY(parseSyntaxText("namespace N { int second; }", declarations));
    StatementBuildContext secondContext;
    secondContext.fileName = "/tmp/b.h";
    secondContext.boundaryAncestor = "/tmp/b.h";
    builder.build(declarations.first(), secondContext);
    CppTypeResolver resolver(model);

    QVERIFY(resolver.findStatement("N::first"));
    QVERIFY(resolver.findStatement("N::second"));
}


void TestCppTypeResolver::test_reopened_nested_namespace_lookup_checks_all_branches()
{
    for (bool reverseOrder : {false, true}) {
        QVector<DeclarationInfo> declarations;
        QVERIFY(parseSyntaxText(
                    "namespace N { namespace Inner { namespace Leaf {} } }",
                    declarations));
        StatementModel model;
        CppStatementBuilder builder(model);
        StatementBuildContext context;
        context.fileName = "/tmp/base.h";
        context.boundaryAncestor = "/tmp/base.h";
        builder.build(declarations.first(), context);
        CppTypeResolver resolver(model);
        PStatement n = resolver.findStatement("N");
        PStatement inner = resolver.findStatement("N::Inner");
        QVERIFY(n && inner);

        auto build = [&](const QString &text, const QString &fileName,
                         const PStatement &parent) {
            declarations.clear();
            QVERIFY(parseSyntaxText(text, declarations));
            context = StatementBuildContext();
            context.fileName = fileName;
            context.boundaryAncestor = fileName;
            context.parentScope = parent;
            builder.build(declarations.first(), context);
        };
        auto addFirst = [&] {
            build("namespace Leaf { int first; }", "/tmp/first.h", inner);
        };
        auto addSecond = [&] {
            build("namespace Inner { int second; }", "/tmp/second.h", n);
        };
        auto addThird = [&] {
            build("namespace Leaf { int third; }", "/tmp/third.h", inner);
        };
        if (reverseOrder) {
            addSecond();
            addThird();
            addFirst();
        } else {
            addFirst();
            addSecond();
            addThird();
        }

        QVERIFY(resolver.findStatement("N::Inner::Leaf::first"));
        QVERIFY(resolver.findStatement("N::Inner::second"));
        QVERIFY(resolver.findStatement("N::Inner::Leaf::third"));
    }
}


void TestCppTypeResolver::test_namespace_lookup_transparency_and_scope_boundaries()
{
    StatementModel model;
    CppStatementBuilder builder(model);
    QVector<DeclarationInfo> declarations;
    auto build = [&](const QString &text, const QString &fileName) {
        declarations.clear();
        QVERIFY(parseSyntaxText(text, declarations));
        StatementBuildContext context;
        context.fileName = fileName;
        context.boundaryAncestor = fileName;
        for (const DeclarationInfo &declaration : declarations)
            builder.build(declaration, context);
    };
    build("namespace N { inline namespace V { int inlineValue; } }", "/tmp/a.h");
    build("namespace N { inline namespace V { struct C { int member; }; } }",
          "/tmp/b.h");
    build("namespace { int anonymousValue; }", "/tmp/c.h");
    build("namespace N { namespace { int nestedAnonymous; } }", "/tmp/d.h");
    build("namespace A { namespace B { int direct; } "
          "inline namespace I { namespace B { int viaInline; } } }", "/tmp/e.h");
    build("void f() { int local; while (true) { int inner; } }", "/tmp/e.cpp");
    build("auto lambda = []() { int lambdaLocal; };", "/tmp/e.cpp");
    CppTypeResolver resolver(model);

    QVERIFY(resolver.findStatement("N::inlineValue"));
    QVERIFY(resolver.findStatement("N::V::inlineValue"));
    QVERIFY(resolver.findStatement("N::C::member"));
    QVERIFY(resolver.findStatement("anonymousValue"));
    QVERIFY(resolver.findStatement("::anonymousValue"));
    QVERIFY(resolver.findStatement("N::nestedAnonymous"));
    QVERIFY(resolver.findStatement("A::B::direct"));
    PStatement a = resolver.findStatement("A");
    QVERIFY(a);
    const auto bCandidates = model.childrenStatements(a).values("B");
    QCOMPARE(bCandidates.count(), 2);
    bool foundInlineAlias = false;
    for (const std::weak_ptr<AbstractStatement> &weak : bCandidates) {
        PStatement candidate = weak.lock();
        if (candidate && candidate->kind == StatementKind::InlineAlias) {
            auto alias = std::dynamic_pointer_cast<InlineAliasStatement>(candidate);
            QCOMPARE(alias->target.lock()->fullName, "A::I::B");
            QList<PStatement> aliasMembers = resolver.membersOf(candidate);
            QCOMPARE(aliasMembers.count(), 1);
            QCOMPARE(aliasMembers.first()->command, "viaInline");
            foundInlineAlias = true;
        }
    }
    QVERIFY(foundInlineAlias);
    QVERIFY(resolver.findStatement("A::B::viaInline"));
    QVERIFY(!resolver.findStatement("f::local"));
    QVERIFY(!resolver.findStatement("f::block::inner"));
    QVERIFY(!resolver.findStatement("lambda::lambdaLocal"));

    PStatement function = resolver.findStatement("f");
    QVERIFY(function);
    QVERIFY(resolver.findStatement("local", function));
    PStatement block = findStatement(std::dynamic_pointer_cast<Statement>(function)->children, "block");
    QVERIFY(block);
    QVERIFY(resolver.findStatement("inner", block));
}


void TestCppTypeResolver::test_namespace_delete_does_not_reinsert_transparent_descendants()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText("namespace outer { namespace regular {"
                            " inline namespace abi { int value; } } }", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    context.boundaryAncestor = context.fileName;
    builder.build(declarations.first(), context);
    CppTypeResolver resolver(model);
    PStatement regular = resolver.findStatement("outer::regular");
    QVERIFY(regular);
    QVERIFY(resolver.findStatement("outer::regular::value"));

    model.deleteStatement(regular);
    QVERIFY(resolver.findStatement("outer"));
    QVERIFY(!resolver.findStatement("outer::regular::value"));
}


void TestCppTypeResolver::test_qualified_lookup_respects_lexical_and_local_scopes()
{
    StatementModel model;
    CppStatementBuilder builder(model);
    QVector<DeclarationInfo> declarations;
    auto build = [&](const QString &text, const QString &fileName) {
        declarations.clear();
        QVERIFY(parseSyntaxText(text, declarations));
        StatementBuildContext context;
        context.fileName = fileName;
        context.boundaryAncestor = fileName;
        for (const DeclarationInfo &declaration : declarations)
            builder.build(declaration, context);
    };
    build("namespace N { int outer; }", "/tmp/a.h");
    build("namespace Outer { namespace N { int inner; } }", "/tmp/b.h");
    build("class Scope { public: class N { public: int inner; }; };",
          "/tmp/c.h");
    build("int value;", "/tmp/d.cpp");
    build("void f() { int value; while (true) { int inner; } }", "/tmp/d.cpp");
    CppTypeResolver resolver(model);

    PStatement outerNamespace = resolver.findStatement("Outer");
    QVERIFY(outerNamespace);
    QVERIFY(resolver.findStatement("N::inner", outerNamespace));
    QVERIFY(!resolver.findStatement("N::outer", outerNamespace));

    PStatement classScope = resolver.findStatement("Scope");
    QVERIFY(classScope);
    QVERIFY(resolver.findStatement("N::inner", classScope));
    QVERIFY(!resolver.findStatement("N::outer", classScope));
    QVERIFY(resolver.findStatement("::N::outer", classScope));

    PStatement function = resolver.findStatement("f");
    QVERIFY(function);
    PStatement local = resolver.findStatement("value", function);
    QVERIFY(local);
    QCOMPARE(local->parentScope.lock(), function);
    QVERIFY(!resolver.findStatement("f::value"));
    PStatement block = findStatement(std::dynamic_pointer_cast<Statement>(function)->children, "block");
    QVERIFY(block);
    QVERIFY(resolver.findStatement("inner", block));
    QVERIFY(!resolver.findStatement("inner", function));
}

void TestCppTypeResolver::test_inherited_member_enumeration_and_access()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "struct Base {\n"
                "  Base();\n"
                "  int publicValue;\n"
                "  virtual void action();\n"
                "  void action(int);\n"
                "protected:\n"
                "  int protectedValue;\n"
                "private:\n"
                "  int privateValue;\n"
                "};\n"
                "class Derived : Base { public: void action() override; };\n"
                "struct PublicDerived : Base {};\n"
                "struct ProtectedDerived : protected Base {};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/inheritance.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    int statementCount = model.count();

    PStatement base = resolver.findStatement("Base");
    PStatement derived = resolver.findStatement("Derived");
    PStatement publicDerived = resolver.findStatement("PublicDerived");
    PStatement protectedDerived = resolver.findStatement("ProtectedDerived");
    QVERIFY(base && derived && publicDerived && protectedDerived);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(base)->classKey, ClassKey::Struct);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(derived)->classKey, ClassKey::Class);
    PStatement basePublicValue = findStatement(std::dynamic_pointer_cast<Statement>(base)->children, "publicValue");
    PStatement baseProtectedValue = findStatement(std::dynamic_pointer_cast<Statement>(base)->children, "protectedValue");
    PStatement basePrivateValue = findStatement(std::dynamic_pointer_cast<Statement>(base)->children, "privateValue");
    PStatement baseConstructor = findStatement(std::dynamic_pointer_cast<Statement>(base)->children, "Base");
    PStatement baseAction;
    PStatement baseIntOverload;
    for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(base)->children.values("action")) {
        PStatement action = weak.lock();
        if (action && std::dynamic_pointer_cast<Statement>(action)->parameters.isEmpty())
            baseAction = action;
        else
            baseIntOverload = action;
    }
    QVERIFY(basePublicValue && baseProtectedValue && basePrivateValue);
    QVERIFY(baseConstructor && baseAction && baseIntOverload);

    QList<PStatement> members = resolver.membersOf(derived);
    QCOMPARE(model.count(), statementCount);
    QCOMPARE(members.count(), 5);
    QVERIFY(!members.contains(baseConstructor));
    QVERIFY(!members.contains(baseAction));
    QVERIFY(members.contains(baseIntOverload));
    PStatement override;
    for (const PStatement &member : members) {
        if (member->command == "action" && std::dynamic_pointer_cast<Statement>(member)->parameters.isEmpty()) {
            override = member;
            break;
        }
    }
    QVERIFY(override);
    QVERIFY(override->properties.testFlag(StatementProperty::Override));
    QCOMPARE(override->parentScope.lock(), derived);
    QVERIFY(members.contains(basePublicValue));
    QVERIFY(members.contains(baseProtectedValue));
    QVERIFY(members.contains(basePrivateValue));
    QCOMPARE(basePublicValue->accessibility, StatementAccessibility::Public);
    QCOMPARE(baseProtectedValue->accessibility, StatementAccessibility::Protected);
    QCOMPARE(basePrivateValue->accessibility, StatementAccessibility::Private);
}
