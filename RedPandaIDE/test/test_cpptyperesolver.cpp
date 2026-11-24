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


void TestCppTypeResolver::test_type_resolver_ignores_recursive_nonmatching_specialization()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  template<typename T, T v> struct integral_constant {\n"
                "    static constexpr T value = v;\n"
                "  };\n"
                "  struct string {};\n"
                "  template<typename... T> struct variant {};\n"
                "  template<typename T> struct hash {};\n"
                "  template<bool... B> struct __all {};\n"
                "  template<bool B, typename T = void> struct enable_if {};\n"
                "  template<typename T> struct enable_if<true, T> { using type = T; };\n"
                "  template<bool B, typename T> using enable_if_t = typename enable_if<B, T>::type;\n"
                "  template<typename T> using remove_const_t = T;\n"
                "  template<typename Key, typename Hash = hash<Key>> using has_enabled =\n"
                "    integral_constant<bool, __is_constructible(Hash, const Hash&)>;\n"
                "  template<typename Type, typename> using enable_helper_imp = Type;\n"
                "  template<typename Type, typename... Keys> using enable_helper =\n"
                "    enable_helper_imp<Type, enable_if_t<__all<has_enabled<Keys>::value...>::value>>;\n"
                "  template<typename... Types>\n"
                "  struct hash<enable_helper<variant<Types...>, remove_const_t<Types>...>> {};\n"
                "}\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/recursive_specialization.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model, nullptr, {CppCompilerType::Clang, CppVersion::Cpp17});
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("std::hash<std::string> value;", variable));
    QCOMPARE(resolver.resolveInstance(variable.specifiers.type).declaration,
             resolver.findStatement("std::hash"));
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


void TestCppTypeResolver::test_namespace_using_lookup_prefers_declared_function()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "void abort();\n"
                "namespace std { using ::abort; }\n"
                "using std::abort;\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/main.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    PStatement stdAlias = resolver.findStatement("std::abort");
    QVERIFY(stdAlias);
    QCOMPARE(stdAlias->kind, StatementKind::Alias);
    PStatement target = resolver.findStatement(
                std::dynamic_pointer_cast<Statement>(stdAlias)->value,
                stdAlias->parentScope.lock());
    QVERIFY(target);
    QCOMPARE(target->kind, StatementKind::Function);
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

void TestCppTypeResolver::test_type_resolver_evaluates_nothrow_invocable_for_functors()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T, typename U = T&&> U __declval(int);\n"
                "  template<typename T> T __declval(long);\n"
                "  template<typename T> auto declval() noexcept -> decltype(__declval<T>(0));\n"
                "  template<typename Fn, typename... Args> struct __is_nothrow_invocable {};\n"
                "}\n"
                "struct NoexceptFunctor { void operator()(int) noexcept; };\n"
                "struct ThrowingFunctor { void operator()(int); };\n"
                "template<bool B> struct ConditionalFunctor {\n"
                "  void operator()(int) noexcept(B);\n"
                "  };\n"
                "struct UnknownFunctor { void operator()(int) noexcept(Unknown); };\n"
                "template<typename T, bool = std::__is_nothrow_invocable<T, int>::value>\n"
                "struct Selector { using type = int; };\n"
                "template<typename T> struct Selector<T, true> { using type = std::string; };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/nothrow_invocable.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    auto memberType = [&](const QString &text) {
        DeclarationInfo variable;
        if (!parseDeclarationText(text, variable))
            return TypeInfo();
        return resolver.memberType(variable.specifiers.type, "type");
    };

    QCOMPARE(memberType("Selector<NoexceptFunctor> value;").toString(), "std::string");
    QCOMPARE(memberType("Selector<ThrowingFunctor> value;").toString(), "int");

    auto type = [&](const QString &text) {
        DeclarationInfo declaration;
        if (!parseDeclarationText(text + " value;", declaration))
            return TypeInfo();
        TypeInfo result = declaration.specifiers.type;
        if (!declaration.declarators.isEmpty())
            result.derivations += declaration.declarators.first().derivations;
        return resolver.resolve(result);
    };
    TypeInfo integer = type("int");
    auto evaluate = [&](const QString &callable) {
        QVector<TemplateArgument> arguments{
            TemplateArgument::fromType(type(callable)), TemplateArgument::fromType(integer)
        };
        CppTypeResolver::EvaluationContext evaluation;
        EvalResult call = resolver.evaluateInvocation(arguments, 0, {}, {}, evaluation);
        return resolver.evaluateNoexceptCall(call, evaluation);
    };
    QCOMPARE(evaluate("NoexceptFunctor"), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate("ThrowingFunctor"), ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(evaluate("ConditionalFunctor<true>"), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate("ConditionalFunctor<false>"), ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(evaluate("UnknownFunctor"), ConstraintSatisfaction::Unknown);

    CppTypeResolver::EvaluationContext evaluation;
    EvalResult literal;
    literal.type = integer;
    QCOMPARE(resolver.evaluateNoexceptCall(literal, evaluation), ConstraintSatisfaction::Satisfied);
    QCOMPARE(resolver.evaluateNoexceptCall({}, evaluation), ConstraintSatisfaction::Unknown);
}

void TestCppTypeResolver::test_type_resolver_matches_explicit_specialization_through_alias()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  template<typename T> struct basic_string {};\n"
                "  using string = basic_string<char>;\n"
                "}\n"
                "template<typename T> struct Trait { using type = int; };\n"
                "template<> struct Trait<std::string> { using type = std::string; };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/alias_specialization.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    PStatement specialization = findStatement(model.childrenStatements(), "Trait<std::string>");
    QVERIFY(specialization);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(specialization)->templateKind,
             TemplateDeclarationKind::ExplicitSpecialization);
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("Trait<std::string> value;", variable));
    QCOMPARE(resolver.memberType(variable.specifiers.type, "type").qualifiedName,
             "std::basic_string");
}

void TestCppTypeResolver::test_type_resolver_evaluates_constructible_traits()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T, T v> struct integral_constant {\n"
                "    static constexpr T value = v;\n"
                "  };\n"
                "}\n"
                "struct FalseType {};\n"
                "template<bool B> struct Selector { using type = FalseType; };\n"
                "template<> struct Selector<true> { using type = std::string; };\n"
                "struct Default { Default(); };\n"
                "struct NoDefault { NoDefault(int); };\n"
                "struct Implicit {};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/constructible.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    auto evaluate = [&](const QString &text) {
        DeclarationInfo variable;
        if (!parseDeclarationText("Selector<std::integral_constant<bool, __is_constructible("
                                  + text + ")>::value> value;", variable))
            return TypeInfo();
        return resolver.memberType(variable.specifiers.type, "type");
    };
    PStatement string = resolver.findStatement("std::string");
    PStatement falseType = resolver.findStatement("FalseType");
    QVERIFY(string && falseType);
    QVERIFY(resolver.findStatement("std::integral_constant"));

    QCOMPARE(evaluate("Default").declaration.lock(), string);
    QCOMPARE(evaluate("NoDefault").declaration.lock(), falseType);
    QCOMPARE(evaluate("NoDefault,int").declaration.lock(), string);
    QCOMPARE(evaluate("Default,int").declaration.lock(), falseType);
    QCOMPARE(evaluate("Implicit").declaration.lock(), string);
    QCOMPARE(evaluate("Implicit,const Implicit&").declaration.lock(), string);
    QCOMPARE(evaluate("int").declaration.lock(), string);
    QCOMPARE(evaluate("void").declaration.lock(), falseType);
}

void TestCppTypeResolver::test_type_resolver_evaluates_constructible_from_types()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "struct Default { Default(); };\n"
                "struct NoDefault { NoDefault(int); };\n"
                "struct Implicit {};\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/constructible_types.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    auto type = [&](const QString &text) {
        DeclarationInfo declaration;
        if (!parseDeclarationText(text + " value;", declaration))
            return TypeInfo();
        TypeInfo result = declaration.specifiers.type;
        if (!declaration.declarators.isEmpty())
            result.derivations += declaration.declarators.first().derivations;
        return resolver.resolve(result);
    };
    auto evaluate = [&](const TypeInfo &target, const QVector<TypeInfo> &arguments) {
        CppTypeResolver::EvaluationContext evaluation;
        return resolver.evaluateConstructible(target, arguments, {}, {}, evaluation);
    };
    TypeInfo defaultType = type("Default");
    TypeInfo noDefault = type("NoDefault");
    TypeInfo implicit = type("Implicit");
    TypeInfo integer = type("int");
    TypeInfo voidType = type("void");
    TypeInfo missing = type("Missing");
    TypeInfo implicitConstReference = type("const Implicit&");
    QVERIFY(!defaultType.qualifiedName.isEmpty() && !noDefault.qualifiedName.isEmpty()
            && !implicit.qualifiedName.isEmpty() && !integer.qualifiedName.isEmpty()
            && !voidType.qualifiedName.isEmpty() && !missing.qualifiedName.isEmpty()
            && !implicitConstReference.qualifiedName.isEmpty());

    QCOMPARE(evaluate(defaultType, {}), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate(noDefault, {}), ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(evaluate(noDefault, {integer}), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate(defaultType, {integer}), ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(evaluate(implicit, {implicitConstReference}), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate(integer, {}), ConstraintSatisfaction::Satisfied);
    QCOMPARE(evaluate(integer, {integer, integer}), ConstraintSatisfaction::Unknown);
    QCOMPARE(evaluate(voidType, {}), ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(evaluate(missing, {}), ConstraintSatisfaction::Unknown);
    QCOMPARE(evaluate(noDefault, {missing}), ConstraintSatisfaction::Unknown);
}

void TestCppTypeResolver::test_type_resolver_evaluates_copy_constructible_through_reference_alias()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T, T v> struct integral_constant {\n"
                "    static constexpr T value = v;\n"
                "  };\n"
                "  template<typename T> using __add_lvalue_reference_t = __add_lvalue_reference(T);\n"
                "  template<typename T> struct is_copy_constructible\n"
                "    : integral_constant<bool, __is_constructible(T, __add_lvalue_reference_t<const T>)> {};\n"
                "}\n"
                "struct FalseType {};\n"
                "template<bool B> struct Selector { using type = FalseType; };\n"
                "template<> struct Selector<true> { using type = std::string; };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/copy_constructible.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model, nullptr, {CppCompilerType::Clang, CppVersion::Cpp17});
    auto evaluate = [&](const QString &text) {
        DeclarationInfo variable;
        if (!parseDeclarationText("Selector<" + text + "> value;", variable))
            return TypeInfo();
        return resolver.memberType(variable.specifiers.type, "type");
    };
    PStatement string = resolver.findStatement("std::string");
    QVERIFY(string);
    QCOMPARE(evaluate("std::integral_constant<bool, __is_constructible("
                      "std::string, const std::string&)>::value").declaration.lock(), string);
    QCOMPARE(evaluate("std::is_copy_constructible<std::string>::value").declaration.lock(), string);
}

void TestCppTypeResolver::test_type_resolver_evaluates_copy_constructible_in_inline_namespace()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std { inline namespace __1 {\n"
                "  struct string {};\n"
                "  template<typename T, T v> struct integral_constant {\n"
                "    static constexpr T value = v;\n"
                "  };\n"
                "  template<typename T> using __add_lvalue_reference_t = __add_lvalue_reference(T);\n"
                "  template<typename T> struct is_copy_constructible\n"
                "    : integral_constant<bool, __is_constructible(T, __add_lvalue_reference_t<const T>)> {};\n"
                "} }\n"
                "struct FalseType {};\n"
                "template<bool B> struct Selector { using type = FalseType; };\n"
                "template<> struct Selector<true> { using type = std::string; };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/inline_copy_constructible.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model, nullptr, {CppCompilerType::Clang, CppVersion::Cpp17});
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText(
                "Selector<std::is_copy_constructible<std::string>::value> value;", variable));
    QCOMPARE(resolver.memberType(variable.specifiers.type, "type").declaration.lock(),
             resolver.findStatement("std::string"));
}

void TestCppTypeResolver::test_type_resolver_evaluates_move_and_default_constructible()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T, T v> struct integral_constant {\n"
                "    static constexpr T value = v;\n"
                "  };\n"
                "  template<typename T> using __add_rvalue_reference_t = __add_rvalue_reference(T);\n"
                "  template<typename T> struct is_move_constructible\n"
                "    : integral_constant<bool, __is_constructible(T, __add_rvalue_reference_t<T>)> {};\n"
                "  template<typename T> struct is_default_constructible\n"
                "    : integral_constant<bool, __is_constructible(T)> {};\n"
                "}\n"
                "struct FalseType {};\n"
                "template<bool B> struct Selector { using type = FalseType; };\n"
                "template<> struct Selector<true> { using type = std::string; };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/move_default_constructible.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model, nullptr, {CppCompilerType::Clang, CppVersion::Cpp17});
    auto evaluate = [&](const QString &text) {
        DeclarationInfo variable;
        if (!parseDeclarationText("Selector<" + text + "> value;", variable))
            return TypeInfo();
        return resolver.memberType(variable.specifiers.type, "type");
    };
    PStatement string = resolver.findStatement("std::string");
    QVERIFY(string);
    QCOMPARE(evaluate("std::is_move_constructible<std::string>::value").declaration.lock(), string);
    QCOMPARE(evaluate("std::is_default_constructible<std::string>::value").declaration.lock(),
             string);
}

void TestCppTypeResolver::test_type_resolver_evaluates_invocable_r()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T, typename U = T&&> U __declval(int);\n"
                "  template<typename T> T __declval(long);\n"
                "  template<typename T> auto declval() noexcept -> decltype(__declval<T>(0));\n"
                "  template<typename Ret, typename Fn, typename... Args>\n"
                "  constexpr bool __is_invocable_r_v = false;\n"
                "}\n"
                "struct FalseType {};\n"
                "struct Functor { std::string operator()(int); };\n"
                "template<bool B> struct Selector { using type = FalseType; };\n"
                "template<> struct Selector<true> { using type = std::string; };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/invocable_r.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    auto evaluate = [&](const QString &text) {
        DeclarationInfo variable;
        if (!parseDeclarationText("Selector<std::__is_invocable_r_v<" + text + ">::value> value;", variable))
            return TypeInfo();
        return resolver.memberType(variable.specifiers.type, "type");
    };
    PStatement string = resolver.findStatement("std::string");
    PStatement falseType = resolver.findStatement("FalseType");
    QVERIFY(string && falseType);
    QVERIFY(resolver.findStatement("std::__is_invocable_r_v"));
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator expressionEvaluator(model, resolver, operations);
    CppTokenizer directExpression;
    directExpression.tokenize({"std::declval<Functor>()(std::declval<int>())"});
    QCOMPARE(expressionEvaluator.evaluate(directExpression).type.toString(), "std::string");

    QCOMPARE(evaluate("std::string,Functor,int").declaration.lock(), string);
    QCOMPARE(evaluate("int,Functor,int").declaration.lock(), falseType);
    QCOMPARE(evaluate("void,Functor,int").declaration.lock(), string);
}

void TestCppTypeResolver::test_type_resolver_evaluates_invocable_r_with_inherited_call()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std { inline namespace __1 {\n"
                "  using size_t = unsigned long;\n"
                "  template<typename T> struct allocator {};\n"
                "  template<typename T> struct char_traits {};\n"
                "  template<typename C, typename T, typename A> struct basic_string {};\n"
                "  using string = basic_string<char, char_traits<char>, allocator<char>>;\n"
                "  template<typename T, typename U = T&&> U __declval(int);\n"
                "  template<typename T> T __declval(long);\n"
                "  template<typename T> auto declval() noexcept -> decltype(__declval<T>(0));\n"
                "  template<typename Ret, typename Fn, typename... Args>\n"
                "  constexpr bool __is_invocable_r_v = false;\n"
                "  template<typename C, typename A> struct string_hash {\n"
                "    size_t operator()(const basic_string<C, char_traits<C>, A>&) const;\n"
                "  };\n"
                "  template<typename T> struct hash {};\n"
                "  template<typename A> struct hash<basic_string<char, char_traits<char>, A>>\n"
                "    : string_hash<char, A> {};\n"
                "} }\n"
                "struct FalseType {};\n"
                "template<bool B> struct Selector { using type = FalseType; };\n"
                "template<> struct Selector<true> { using type = std::string; };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/inherited_invocable_r.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model, nullptr, {CppCompilerType::Clang, CppVersion::Cpp17});
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText(
                "Selector<std::__is_invocable_r_v<std::size_t, std::hash<std::string>,"
                " const std::string&>::value> value;", variable));
    QCOMPARE(resolver.memberType(variable.specifiers.type, "type").declaration.lock(),
             resolver.findStatement("std::basic_string"));
}

void TestCppTypeResolver::test_type_resolver_expands_non_type_pack_arguments()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std { template<bool... B> struct __all {}; }\n"
                "template<typename... T> using AllArrays = std::__all<(__is_array(T))...>;\n"
                "struct FalseType {};\n"
                "template<bool B> struct Selector { using type = FalseType; };\n"
                "template<> struct Selector<true> { using type = int; };\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/non_type_pack.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    auto evaluate = [&](const QString &text) {
        DeclarationInfo variable;
        if (!parseDeclarationText("Selector<AllArrays<" + text + ">::value> value;", variable))
            return TypeInfo();
        return resolver.memberType(variable.specifiers.type, "type");
    };
    DeclarationInfo selectedVariable;
    QVERIFY(parseDeclarationText("Selector<AllArrays<int[],int[]>::value> value;", selectedVariable));
    PStatement selected = resolver.resolveInstance(selectedVariable.specifiers.type).declaration;
    QVERIFY(selected);
    QCOMPARE(selected->command, "Selector<true>");

    QCOMPARE(evaluate("int[],int[]").qualifiedName, "int");
    QCOMPARE(evaluate("int[],int").qualifiedName, "FalseType");
    QCOMPARE(evaluate("").qualifiedName, "int");
}

void TestCppTypeResolver::test_type_resolver_evaluates_logical_trait_single_values()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std { inline namespace __1 {\n"
                "  template<bool... B> struct __all {};\n"
                "  template<typename T, T v> struct integral_constant {\n"
                "    static constexpr T value = v;\n"
                "  };\n"
                "  struct string {};\n"
                "  template<typename T> using Enabled =\n"
                "    integral_constant<bool, __is_constructible(T)>;\n"
                "  template<typename T> struct hash {};\n"
                "  template<typename Key, typename Hash = hash<Key>> using EnabledWithDefault =\n"
                "    integral_constant<bool, __is_constructible(Hash, const Hash&) &&\n"
                "      __is_constructible(Hash)>;\n"
                "  template<typename... T> struct __and_ {};\n"
                "  template<typename T> struct __not_ {};\n"
                "  template<typename T, typename Hash> using Cache = __not_<__and_<"
                "    integral_constant<bool, false>,"
                "    integral_constant<bool, __is_constructible(Hash, const Hash&)>>>;\n"
                "  template<typename Hash, typename T> using Forward = Cache<T, Hash>;\n"
                "} }\n"
                "struct FalseType {};\n"
                "template<bool B> struct Selector { using type = FalseType; };\n"
                "template<> struct Selector<true> { using type = int; };\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/logical_single_values.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    auto evaluate = [&](const QString &text) {
        DeclarationInfo variable;
        if (!parseDeclarationText("Selector<std::__all<" + text + ">::value> value;", variable))
            return TypeInfo();
        return resolver.memberType(variable.specifiers.type, "type");
    };
    QCOMPARE(evaluate("true").qualifiedName, "int");
    QCOMPARE(evaluate("false").qualifiedName, "FalseType");
    DeclarationInfo direct;
    QVERIFY(parseDeclarationText(
                "Selector<std::integral_constant<bool, true>::value> value;", direct));
    QCOMPARE(resolver.memberType(direct.specifiers.type, "type").qualifiedName, "int");
    QCOMPARE(evaluate("std::integral_constant<bool, true>::value").qualifiedName, "int");
    QCOMPARE(evaluate("std::Enabled<std::string>::value").qualifiedName, "int");
    QCOMPARE(evaluate("std::EnabledWithDefault<std::string>::value").qualifiedName, "int");
    QCOMPARE(evaluate("std::Cache<std::string, std::hash<std::string>>::value").qualifiedName,
             "int");
    DeclarationInfo cache;
    QVERIFY(parseDeclarationText(
                "Selector<std::Cache<std::string, std::hash<std::string>>::value> value;", cache));
    QCOMPARE(resolver.memberType(cache.specifiers.type, "type").qualifiedName, "int");
    QVERIFY(resolver.findStatement("std::Forward"));
    DeclarationInfo forwardAlias;
    QVERIFY(parseDeclarationText(
                "std::Forward<std::hash<std::string>, std::string> value;", forwardAlias));
    TypeInfo resolvedForward = resolver.resolve(forwardAlias.specifiers.type);
    QCOMPARE(resolvedForward.qualifiedName, "std::__1::__not_");
    CppTypeResolver::EvaluationContext forwardEvaluation;
    QCOMPARE(resolver.evaluateConstantValue(
                 resolvedForward.toString() + "::value", {}, {}, forwardEvaluation), "true");
}

void TestCppTypeResolver::test_type_resolver_resolves_alias_to_clang_type_builtin()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T> using Reference = __add_lvalue_reference(T);\n"
                "}\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/builtin_alias.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model, nullptr, {CppCompilerType::Clang, CppVersion::Cpp17});
    DeclarationInfo direct;
    QVERIFY(parseDeclarationText("const std::string value;", direct));
    QCOMPARE(direct.specifiers.type.qualifiedName, "std::string");
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("std::Reference<const std::string> value;", variable));
    TypeInfo resolved = resolver.resolve(variable.specifiers.type);
    QCOMPARE(resolved.qualifiedName, "std::string");
    QCOMPARE(resolved.toString(), "const std::string &");
    for (const QString &typeText : {QString("std::string const"),
                                    QString("const volatile std::string"),
                                    QString("std::string const volatile")}) {
        DeclarationInfo qualified;
        QVERIFY(parseDeclarationText("std::Reference<" + typeText + "> value;", qualified));
        TypeInfo result = resolver.resolve(qualified.specifiers.type);
        QCOMPARE(result.qualifiedName, "std::string");
        QCOMPARE(result.toString(), typeText.contains("volatile")
                 ? "const volatile std::string &" : "const std::string &");
    }
}

void TestCppTypeResolver::test_type_resolver_internal_member_interfaces()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "struct Base {\n"
                "  Base();\n"
                "  int action();\n"
                "  int publicValue;\n"
                "protected: int protectedValue;\n"
                "private: int privateValue;\n"
                "};\n"
                "struct Derived : public Base { int action(); };\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/resolver_member_interfaces.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    CppTypeResolver::EvaluationContext evaluation;
    PStatement base = resolver.findStatement("Base");
    PStatement derived = resolver.findStatement("Derived");
    QVERIFY(base && derived);
    DeclarationInfo variable;
    QVERIFY(parseDeclarationText("Derived value;", variable));
    TypeInfo derivedType = resolver.resolve(variable.specifiers.type);
    CppTypeResolver::ClassInstance instance = resolver.resolveInstance(derivedType);
    QCOMPARE(instance.declaration, derived);

    StatementAccessibility inheritance = StatementAccessibility::None;
    QCOMPARE(resolver.baseClass(derived, "public Base", inheritance), base);
    QCOMPARE(inheritance, StatementAccessibility::Public);
    QCOMPARE(resolver.applyInheritance(StatementAccessibility::Public, inheritance),
             StatementAccessibility::Public);
    QCOMPARE(resolver.applyInheritance(StatementAccessibility::Protected, inheritance),
             StatementAccessibility::Protected);
    QCOMPARE(resolver.applyInheritance(StatementAccessibility::Public,
                                       StatementAccessibility::Private),
             StatementAccessibility::Private);
    QCOMPARE(resolver.applyInheritance(StatementAccessibility::None, inheritance),
             StatementAccessibility::None);

    CppTypeResolver::MemberEntryList entries;
    QSet<const AbstractStatement *> visited;
    resolver.appendMembers(derived, false, visited, entries);
    QSet<QString> names;
    for (const auto &entry : entries)
        names.insert(entry.statement->command);
    QVERIFY(names.contains("action") && names.contains("publicValue")
            && names.contains("protectedValue") && names.contains("privateValue"));
    QCOMPARE(names.count(), 4);

    QSet<QString> candidateVisited;
    QList<CppTypeResolver::MemberResolution> candidates = resolver.memberCandidates(
                instance, "action", candidateVisited, evaluation);
    QCOMPARE(candidates.count(), 1);
    QCOMPARE(candidates.first().statement->parentScope.lock(), derived);
    candidateVisited.clear();
    candidates = resolver.memberCandidates(instance, "publicValue", candidateVisited, evaluation);
    QCOMPARE(candidates.count(), 1);
    QCOMPARE(candidates.first().statement->parentScope.lock(), base);
    QCOMPARE(resolver.findMember(instance, "publicValue", evaluation).statement,
             candidates.first().statement);
    candidateVisited.clear();
    QCOMPARE(resolver.findMember(instance, "publicValue", candidateVisited, evaluation).statement,
             candidates.first().statement);
    QCOMPARE(resolver.findMemberDeclaration(derived, "action")->parentScope.lock(), derived);
    QCOMPARE(resolver.findMemberDeclaration(derived, "publicValue")->parentScope.lock(), base);

    PStatement action = resolver.findMemberDeclaration(derived, "action");
    PStatement publicValue = candidates.first().statement;
    QVERIFY(action && publicValue);
    QCOMPARE(resolver.memberStatement(derivedType, "publicValue", &evaluation), publicValue);
    QCOMPARE(resolver.memberType(derivedType, "publicValue", &evaluation).qualifiedName, "int");
    QCOMPARE(resolver.memberAccess(derivedType, "publicValue", &evaluation).statement, publicValue);
    QCOMPARE(resolver.memberAccess(instance, "publicValue", &evaluation).statement, publicValue);
    QCOMPARE(resolver.invokeResult(instance, "action", &evaluation).statement, action);
    QCOMPARE(resolver.invokeMember(derivedType, "action", {}, &evaluation).statement, action);
    QCOMPARE(resolver.invokeMember(instance, "action", {}, &evaluation).statement, action);
    QCOMPARE(resolver.resolveMember(instance, "publicValue", false, {}, evaluation).statement,
             publicValue);
    QCOMPARE(resolver.resolveMember(instance, "action", true, {}, evaluation).statement, action);

    QList<PStatement> members = resolver.membersOf(derived);
    QVERIFY(members.contains(action) && members.contains(publicValue));
    QList<PStatement> publicMembers = resolver.resolveMembersOfClass(base, {});
    QVERIFY(publicMembers.contains(publicValue));
    QVERIFY(!publicMembers.contains(resolver.findMemberDeclaration(base, "privateValue")));
    QList<PStatement> ownMembers = resolver.resolveMembersOfClass(base, base);
    QVERIFY(ownMembers.contains(resolver.findMemberDeclaration(base, "privateValue")));
    QList<PStatement> derivedAccess = resolver.resolveMembersOfClass(base, derived);
    QVERIFY(derivedAccess.contains(resolver.findMemberDeclaration(base, "protectedValue")));
    QVERIFY(!derivedAccess.contains(resolver.findMemberDeclaration(base, "privateValue")));
    QVERIFY(resolver.resolveMembersOfClass({}, {}).isEmpty());
}

void TestCppTypeResolver::test_type_resolver_internal_binding_interfaces()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "struct Base {};\n"
                "struct Derived : Base {};\n"
                "struct Other {};\n"
                "template<typename T> struct Box {};\n"
                "template<typename T> struct Box<T*> {};\n"
                "template<typename... T> struct Pack {};\n"
                "template<typename T> int identity(T);\n"
                "int overloaded(int);\n"
                "double overloaded(double);\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/resolver_binding_interfaces.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
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
    TypeInfo integer = type("int");
    TypeInfo integerPointer = type("int*");
    TypeInfo constIntegerReference = type("const int&");

    TemplateBindings bindings;
    resolver.bindParameterType(type("T&"), constIntegerReference, {"T"}, bindings, evaluation);
    QVERIFY(bindings.contains("T"));
    QCOMPARE(bindings["T"].type->toString(), "int");
    bindings.clear();
    resolver.bindParameterType(type("T*"), integerPointer, {"T"}, bindings, evaluation);
    QCOMPARE(bindings["T"].type->toString(), "int");
    bindings.clear();
    resolver.bindParameterType(type("Box<T>"), type("Box<int>"), {"T"}, bindings, evaluation);
    QCOMPARE(bindings["T"].type->toString(), "int");

    TypeInfo materialized;
    materialized.qualifiedName = "Box";
    materialized.qualifiedNameText = "Box<int>";
    resolver.materializeTemplateArguments(materialized, evaluation);
    QCOMPARE(materialized.templateArguments.count(), 1);
    QCOMPARE(materialized.templateArguments.first().type->qualifiedName, "int");
    TypeInfo pattern;
    pattern.qualifiedName = "Box";
    pattern.qualifiedNameText = "Box<T>";
    resolver.materializeTemplateArgumentPatterns(pattern, evaluation);
    QCOMPARE(pattern.templateArguments.count(), 1);
    QCOMPARE(pattern.templateArguments.first().constantExpression, "T");

    PStatement overloaded = resolver.findStatement("overloaded");
    QCOMPARE(resolver.functionCandidates(overloaded).count(), 2);
    QVERIFY(resolver.functionCandidates({}).isEmpty());
    PStatement identity = resolver.findStatement("identity");
    int score = 0;
    CppTypeResolver::PTemplateBindingFrame functionBindings = resolver.matchFunctionCandidate(
                identity, {integer}, {}, evaluation, {}, &score, false);
    QVERIFY(functionBindings);
    QCOMPARE(functionBindings->values["T"].type->toString(), "int");
    QVERIFY(score > 0);
    QVERIFY(!resolver.matchFunctionCandidate(identity, {}, {}, evaluation, {}, nullptr, false));

    TypeInfo resolvedParameter;
    QVERIFY(resolver.compatibleArgument(integer, integer, {}, {}, evaluation, resolvedParameter));
    QCOMPARE(resolvedParameter.qualifiedName, "int");
    QVERIFY(resolver.compatibleArgument(type("Base"), type("Derived"), {}, {}, evaluation,
                                        resolvedParameter));
    QVERIFY(!resolver.compatibleArgument(type("Base"), type("Other"), {}, {}, evaluation,
                                         resolvedParameter));

    TemplateArgument constant = resolver.resolveArgument(
                "true", TemplateParameterKind::NonType, {}, {}, evaluation);
    QCOMPARE(constant.kind, TemplateArgumentKind::NonType);
    QCOMPARE(constant.constantExpression, "true");
    TemplateArgument resolvedType = resolver.resolveArgument(
                "Box<int>", TemplateParameterKind::Type, {}, {}, evaluation);
    QCOMPARE(resolvedType.kind, TemplateArgumentKind::Type);
    QCOMPARE(resolvedType.type->qualifiedName, "Box");

    PStatement box = resolver.findStatement("Box");
    CppTypeResolver::PTemplateBindingFrame boxBindings = resolver.declarationBindings(
                box, type("Box<int>"), {}, evaluation);
    QVERIFY(boxBindings && boxBindings->complete);
    QCOMPARE(resolver.findBinding(boxBindings, "T")->type->toString(), "int");
    QVERIFY(!resolver.findBinding(boxBindings, "Missing"));
    CppTypeResolver::PTemplateBindingFrame packBindings = resolver.declarationBindings(
                resolver.findStatement("Pack"), type("Pack<int, double>"), {}, evaluation);
    const QVector<TemplateArgument> *pack = resolver.findParameterPack(packBindings, "T");
    QVERIFY(pack);
    QCOMPARE(pack->count(), 2);
    QVERIFY(!resolver.findParameterPack(packBindings, "Missing"));

    TypeInfo referenced = integer;
    TypeInfo reference = type("int&");
    resolver.applyReference(referenced, reference);
    QCOMPARE(referenced.toString(), "int &");
    TypeInfo collapsed = type("int&");
    TypeInfo rvalue = type("int&&");
    resolver.applyReference(collapsed, rvalue);
    QCOMPARE(collapsed.toString(), "int &");

    TypeInfo pointerBox = type("Box<int*>");
    CppTypeResolver::ClassInstance primary;
    primary.type = pointerBox;
    primary.declaration = box;
    primary.bindings = resolver.declarationBindings(box, pointerBox, {}, evaluation);
    CppTypeResolver::ClassInstance selected = resolver.selectSpecialization(primary, evaluation);
    QCOMPARE(selected.declaration, findStatement(model.childrenStatements(), "Box<T*>"));
    TemplateBindings matchedBindings;
    QCOMPARE(resolver.matchSpecialization(selected.declaration, pointerBox.templateArguments,
                                          {}, matchedBindings, evaluation),
             CppTypeResolver::SpecializationMatch::Match);
    QCOMPARE(matchedBindings["T"].type->toString(), "int");
    TemplateBindings typeBindings;
    QCOMPARE(resolver.matchType(type("T*"), integerPointer, {"T"}, typeBindings, {}, evaluation),
             CppTypeResolver::SpecializationMatch::Match);
    QCOMPARE(resolver.matchType(type("T*"), type("double"), {"T"}, typeBindings, {}, evaluation),
             CppTypeResolver::SpecializationMatch::NoMatch);
    TemplateArgument pointerPattern = TemplateArgument::fromType(type("T*"));
    QCOMPARE(resolver.matchArgument(pointerPattern, TemplateArgument::fromType(integerPointer),
                                    {"T"}, typeBindings, {}, evaluation),
             CppTypeResolver::SpecializationMatch::Match);
    QCOMPARE(resolver.matchArgument(TemplateArgument::fromConstant("1+1"),
                                    TemplateArgument::fromConstant("2"), {}, typeBindings,
                                    {}, evaluation),
             CppTypeResolver::SpecializationMatch::Unknown);
}

void TestCppTypeResolver::test_type_resolver_ranks_partial_function_candidates()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "struct First {};\n"
                "void select(First first, int value);\n"
                "void select(const char *text, int value);\n"
                "void select(int first, int value);\n"
                "void select(double first, double value);\n"
                "void select(First first);\n"
                "void defaulted(int value = 0);\n"
                "void variadic(int count, ...);\n"
                "struct Logger { void log(int count, ...) const; };\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/resolver_partial_candidates.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    auto type = [&](const QString &text) {
        DeclarationInfo declaration;
        if (!parseDeclarationText(text + " value;", declaration))
            return TypeInfo();
        TypeInfo result = declaration.specifiers.type;
        if (!declaration.declarators.isEmpty())
            result.derivations += declaration.declarators.first().derivations;
        return resolver.resolve(result);
    };

    PStatement select = resolver.findStatement("select");
    QList<PStatement> candidates = resolver.functionCandidates(select);
    QCOMPARE(candidates.count(), 5);
    QList<PStatement> matches = resolver.rankFunctionCandidates(
                candidates, {type("const char*")}, 2);
    QCOMPARE(matches.count(), 1);
    QVERIFY(std::dynamic_pointer_cast<Statement>(matches.first())->normalizedArgs.contains("char"));

    matches = resolver.rankFunctionCandidates(candidates, {type("int")}, 2);
    QCOMPARE(matches.count(), 2);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(matches.first())->normalizedArgs, "(int,int)");
    QVERIFY(std::dynamic_pointer_cast<Statement>(matches.last())->normalizedArgs.contains("double"));

    matches = resolver.rankFunctionCandidates(candidates, {}, 2);
    QCOMPARE(matches.count(), 4);

    PStatement defaulted = resolver.findStatement("defaulted");
    auto defaultedNormal = std::dynamic_pointer_cast<Statement>(defaulted);
    QCOMPARE(defaultedNormal->args, "(int value = 0)");
    QCOMPARE(defaultedNormal->normalizedArgs, "(int)");
    QCOMPARE(resolver.invokeFunction(defaulted, {}).qualifiedName, "void");
    PStatement variadic = resolver.findStatement("variadic");
    QCOMPARE(resolver.invokeFunction(
                 variadic, {type("int"), type("int"), type("double")}).qualifiedName, "void");
    PStatement memberVariadic = resolver.findStatement("Logger::log");
    QVERIFY(memberVariadic);
    QCOMPARE(resolver.invokeFunction(
                 memberVariadic, {type("int"), type("double")}).qualifiedName, "void");
}

void TestCppTypeResolver::test_type_resolver_internal_type_and_lookup_interfaces()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace Outer {\n"
                "  struct Class { struct Nested {}; };\n"
                "  template<typename T> struct Box {};\n"
                "}\n"
                "namespace Outer { struct Other {}; }\n"
                "namespace std { inline namespace __1 {\n"
                "  struct string {};\n"
                "  template<typename T> struct add_lvalue_reference {};\n"
                "} }\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/resolver_type_lookup_interfaces.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    CppTypeResolver::EvaluationContext evaluation;
    PStatement outer = resolver.findStatement("Outer");
    PStatement classStatement = resolver.findStatement("Outer::Class");
    PStatement nested = resolver.findStatement("Outer::Class::Nested");
    QVERIFY(outer && classStatement && nested);

    QVector<TypeNameSegment> segments;
    bool globallyQualified = false;
    QVERIFY(resolver.parseTypeNameSegments("const Outer::Class", segments, globallyQualified,
                                           evaluation));
    QCOMPARE(segments.count(), 2);
    QVERIFY(segments.first().qualifiers.testFlag(TypeQualifier::Const));
    QVERIFY(!globallyQualified);
    QVERIFY(resolver.parseTypeNameSegments("::Outer::Class", segments, globallyQualified,
                                           evaluation));
    QVERIFY(globallyQualified);
    QVERIFY(!resolver.parseTypeNameSegments("Outer::", segments, globallyQualified, evaluation));
    QVERIFY(resolver.parseTypeNameSegments("Outer::Box<int>", segments, globallyQualified,
                                           evaluation, false));
    QCOMPARE(segments.last().arguments.count(), 1);
    QCOMPARE(segments.last().arguments.first().constantExpression, "int");

    TypeInfo parsed = resolver.parseTypeText("const Outer::Class&", evaluation);
    QCOMPARE(parsed.qualifiedName, "Outer::Class");
    QVERIFY(parsed.qualifiers.testFlag(TypeQualifier::Const));
    QCOMPARE(parsed.derivations.first().kind, TypeDerivationKind::LvalueReference);
    TypeInfo parsedTemplate = resolver.parseTypeText("Outer::Box<int>", evaluation);
    resolver.materializeTemplateArguments(parsedTemplate, evaluation);
    QCOMPARE(parsedTemplate.templateArguments.count(), 1);
    TypeInfo lazyTemplate = resolver.parseTypeText("Outer::Box<int>", evaluation, false);
    QCOMPARE(lazyTemplate.templateArguments.count(), 1);
    QCOMPARE(lazyTemplate.templateArguments.first().constantExpression, "int");

    TypeInfo classType;
    classType.qualifiedName = classType.qualifiedNameText = "Outer::Class";
    QCOMPARE(resolver.resolveType(classType, {}, {}, evaluation).qualifiedName, "Outer::Class");
    QCOMPARE(resolver.resolve(classType).qualifiedName, "Outer::Class");
    QCOMPARE(resolver.resolveInstance(classType).declaration, classStatement);
    QCOMPARE(resolver.resolveInContext(classType, {}, {}, &evaluation).declaration, classStatement);
    QVERIFY(resolver.parseTypeNameSegments("Outer::Class", segments, globallyQualified, evaluation));
    QCOMPARE(resolver.resolveSegments({}, segments, false, {}, {}, evaluation).declaration,
             classStatement);
    QCOMPARE(resolver.classInstanceFromResolvedType(classType, {}, {}, evaluation).declaration,
             classStatement);

    QCOMPARE(resolver.findDeclaration("Outer::Class", {}, true), classStatement);
    QCOMPARE(resolver.findStatement("Outer::Class"), classStatement);
    bool firstPartFound = false;
    QCOMPARE(resolver.findInNamespaceScope(outer, {"Class", "Nested"}, firstPartFound, true),
             nested);
    QVERIFY(firstPartFound);
    firstPartFound = false;
    QVERIFY(!resolver.findInNamespaceScope(outer, {"Class", "Missing"}, firstPartFound, true));
    QVERIFY(firstPartFound);
    firstPartFound = false;
    QVERIFY(!resolver.findInNamespaceScope(outer, {"Missing"}, firstPartFound, true));
    QVERIFY(!firstPartFound);
    QCOMPARE(resolver.findDeclarationRemainder(outer, {"Outer", "Class", "Nested"}, true),
             nested);
    QVERIFY(!resolver.findDeclarationRemainder(outer, {"Outer", "Missing"}, true));
    QVERIFY(resolver.supportsTypeTrait("add_lvalue_reference"));
    QVERIFY(resolver.supportsConstantTrait("is_array"));
    QVERIFY(!resolver.supportsTypeTrait("missing"));
    QVERIFY(!resolver.supportsConstantTrait("missing"));

    TypeInfo referenceTrait;
    referenceTrait.qualifiedName = referenceTrait.qualifiedNameText = "std::add_lvalue_reference<int>";
    CppTypeResolver::ClassInstance trait;
    trait.type = resolver.resolve(referenceTrait);
    trait.declaration = resolver.findStatement("std::add_lvalue_reference");
    QCOMPARE(resolver.traitMemberType(trait, "type", evaluation).toString(), "int &");
    QVERIFY(resolver.traitMemberType(trait, "value", evaluation).qualifiedName.isEmpty());
}

void TestCppTypeResolver::test_type_resolver_internal_constant_interfaces()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std {\n"
                "  struct string {};\n"
                "  template<typename T, T v> struct integral_constant {\n"
                "    static constexpr T value = v;\n"
                "  };\n"
                "  template<bool... B> struct __all {};\n"
                "  template<typename... T> struct __and_ {};\n"
                "  template<typename T> struct __not_ {};\n"
                "  template<typename T> using Alias = integral_constant<bool, true>;\n"
                "  struct Functor { int operator()(int) noexcept; };\n"
                "}\n"
                "template<bool B> requires (B) int constrained();\n", declarations,
                CppVersion::Cpp20));
    StatementModel model;
    CppStatementBuilder builder(model, {CppCompilerType::GCC, CppVersion::Cpp20});
    StatementBuildContext context;
    context.fileName = "/tmp/resolver_constant_interfaces.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model, nullptr, {CppCompilerType::GCC, CppVersion::Cpp20});
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

    QCOMPARE(resolver.evaluateConstantValue("true", {}, {}, evaluation), "true");
    QCOMPARE(resolver.evaluateConstantValue("!false", {}, {}, evaluation), "true");
    QCOMPARE(resolver.evaluateConstantValue("true&&false", {}, {}, evaluation), "false");
    QCOMPARE(resolver.evaluateConstantValue("false||true", {}, {}, evaluation), "true");
    QCOMPARE(resolver.evaluateConstantValue("1==1", {}, {}, evaluation), "true");
    QCOMPARE(resolver.evaluateConstantValue("(false)", {}, {}, evaluation), "false");
    QCOMPARE(resolver.evaluateConstantValue(
                 "std::integral_constant<bool, true>::value", {}, {}, evaluation), "true");
    QCOMPARE(resolver.evaluateConstantValue("missing", {}, {}, evaluation), "");
    QCOMPARE(resolver.resolveConstantExpression(
                 "std::integral_constant<bool, false>::value", {}, {}, evaluation), "false");
    QCOMPARE(resolver.resolveConstantExpression("std::Alias<int>::value", {}, {}, evaluation),
             "true");
    QCOMPARE(resolver.evaluateConstraintAtom("true", {}, {}, evaluation),
             ConstraintSatisfaction::Satisfied);
    QCOMPARE(resolver.evaluateConstraint("true&&false", {}, {}, evaluation),
             ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(resolver.evaluateConstraint("unknown", {}, {}, evaluation),
             ConstraintSatisfaction::Unknown);

    PStatement constrained = resolver.findStatement("constrained");
    QVERIFY(constrained);
    auto falseBindings = std::make_shared<TemplateBindingFrame>();
    falseBindings->values.insert("B", TemplateArgument::fromConstant("false"));
    QCOMPARE(resolver.constraintsSatisfied(constrained, falseBindings, {}, evaluation),
             ConstraintSatisfaction::Unsatisfied);
    auto trueBindings = std::make_shared<TemplateBindingFrame>();
    trueBindings->values.insert("B", TemplateArgument::fromConstant("true"));
    QCOMPARE(resolver.constraintsSatisfied(constrained, trueBindings, {}, evaluation),
             ConstraintSatisfaction::Satisfied);

    QVector<TemplateArgument> logicalArguments{
        TemplateArgument::fromConstant("true"), TemplateArgument::fromConstant("false")
    };
    QCOMPARE(resolver.evaluateLogicalTrait(logicalArguments, true, {}, {}, evaluation),
             ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(resolver.evaluateLogicalTrait(logicalArguments, false, {}, {}, evaluation),
             ConstraintSatisfaction::Satisfied);
    QCOMPARE(resolver.evaluateLogicalTrait({}, true, {}, {}, evaluation),
             ConstraintSatisfaction::Satisfied);

    QCOMPARE(resolver.evaluateConstructibleExpression(
                 "__is_constructible(std::Functor)", {}, {}, evaluation),
             ConstraintSatisfaction::Satisfied);
    QCOMPARE(resolver.evaluateConstructibleExpression("not-a-builtin", {}, {}, evaluation),
             ConstraintSatisfaction::Unknown);

    QVector<TemplateArgument> invocationArguments{
        TemplateArgument::fromType(type("std::Functor")), TemplateArgument::fromType(type("int"))
    };
    EvalResult invocation = resolver.evaluateInvocation(invocationArguments, 0, {}, {}, evaluation);
    QVERIFY(invocation.isValid());
    QCOMPARE(invocation.type.qualifiedName, "int");
    QVERIFY(!resolver.evaluateInvocation(invocationArguments, -1, {}, {}, evaluation).isValid());
    QVERIFY(!resolver.evaluateInvocation({TemplateArgument::fromConstant("1")}, 0, {}, {}, evaluation)
                     .isValid());
    QVector<TemplateArgument> invocableArguments{TemplateArgument::fromType(type("int"))};
    invocableArguments += invocationArguments;
    QCOMPARE(resolver.evaluateInvocableR(invocableArguments, {}, {}, evaluation),
             ConstraintSatisfaction::Satisfied);
    invocableArguments[0] = TemplateArgument::fromType(type("std::string"));
    QCOMPARE(resolver.evaluateInvocableR(invocableArguments, {}, {}, evaluation),
             ConstraintSatisfaction::Unsatisfied);
    QCOMPARE(resolver.evaluateInvocableR({}, {}, {}, evaluation),
             ConstraintSatisfaction::Unknown);
    QCOMPARE(resolver.evaluateNoexceptExpression("noexcept(1)", {}, {}, &evaluation),
             ConstraintSatisfaction::Satisfied);
    QCOMPARE(resolver.evaluateNoexceptExpression("noexcept(missing())", {}, {}, &evaluation),
             ConstraintSatisfaction::Unknown);
}

void TestCppTypeResolver::test_type_resolver_matches_alias_patterns_structurally()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "namespace std { inline namespace __1 {\n"
                "  struct string {};\n"
                "  template<typename... T> struct variant {};\n"
                "  template<typename T> struct hash {};\n"
                "  template<typename T, T v> struct integral_constant {\n"
                "    static constexpr T value = v;\n"
                "  };\n"
                "  template<bool... B> struct __all {};\n"
                "  template<bool B, typename T = void> struct enable_if {};\n"
                "  template<typename T> struct enable_if<true, T> { using type = T; };\n"
                "  template<bool B, typename T> using enable_if_t = typename enable_if<B, T>::type;\n"
                "  template<typename T> using remove_const_t = T;\n"
                "  template<typename Key, typename Hash = hash<Key>> using has_enabled =\n"
                "    integral_constant<bool, __is_constructible(Hash, const Hash&)>;\n"
                "  template<typename Type, typename> using enable_helper_imp = Type;\n"
                "  template<typename Type, typename... Keys> using enable_helper =\n"
                "    enable_helper_imp<Type, enable_if_t<__all<has_enabled<Keys>::value...>::value>>;\n"
                "  template<typename T> using Identity = T;\n"
                "  typedef int Int;\n"
                "  template<typename T, typename U = int> struct Pair {};\n"
                "  template<typename T> using First = Pair<T>;\n"
                "  template<bool B, typename T> using Conditional = enable_if_t<B, T>;\n"
                "  template<typename T> using Cycle = Cycle<T>;\n"
                "} }\n", declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/structural_alias_matching.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);
    CppTypeResolver::EvaluationContext evaluation;
    PStatement stdScope = resolver.findStatement("std");
    QVERIFY(stdScope);
    TypeInfo pattern = resolver.parseTypeText(
                "std::enable_helper<std::variant<Types...>, std::remove_const_t<Types>...>",
                evaluation, false);
    TypeInfo actual = resolver.parseTypeText("std::string", evaluation);
    QVERIFY(!pattern.qualifiedName.isEmpty() && !actual.qualifiedName.isEmpty());

    QSet<const AbstractStatement *> visited;
    TypeInfo expanded = resolver.expandTypePattern(pattern, stdScope, visited, evaluation);
    QCOMPARE(expanded.qualifiedName, "std::variant");
    QCOMPARE(expanded.templateArguments.count(), 1);
    QCOMPARE(evaluation.operationCount, 0);
    TemplateBindings bindings;
    QCOMPARE(resolver.matchType(pattern, actual, {"Types"}, bindings, stdScope, evaluation),
             CppTypeResolver::SpecializationMatch::NoMatch);
    QCOMPARE(evaluation.operationCount, 0);

    QCOMPARE(resolver.matchType(resolver.parseTypeText("std::Identity<int>", evaluation),
                                actual, {}, bindings, stdScope, evaluation),
             CppTypeResolver::SpecializationMatch::NoMatch);
    TypeInfo integer = resolver.parseTypeText("int", evaluation);
    QCOMPARE(resolver.matchType(resolver.parseTypeText("std::Identity<int>", evaluation),
                                integer, {}, bindings, stdScope, evaluation),
             CppTypeResolver::SpecializationMatch::Match);
    QCOMPARE(resolver.matchType(resolver.parseTypeText("std::Int", evaluation),
                                integer, {}, bindings, stdScope, evaluation),
             CppTypeResolver::SpecializationMatch::Match);
    TypeInfo firstPattern = resolver.parseTypeText("std::First<char>", evaluation);
    TypeInfo expandedFirst = resolver.expandTypePattern(firstPattern, stdScope, visited, evaluation);
    QCOMPARE(expandedFirst.qualifiedName, "Pair");
    QCOMPARE(expandedFirst.templateArguments.count(), 2);
    QCOMPARE(resolver.matchType(firstPattern,
                                resolver.resolve(resolver.parseTypeText(
                                        "std::Pair<char, int>", evaluation)),
                                {}, bindings, stdScope, evaluation),
             CppTypeResolver::SpecializationMatch::Match);
    QCOMPARE(resolver.matchType(resolver.parseTypeText("std::Conditional<true, int>", evaluation),
                                integer, {}, bindings, stdScope, evaluation),
             CppTypeResolver::SpecializationMatch::Match);
    QCOMPARE(resolver.matchType(resolver.parseTypeText("std::Conditional<false, int>", evaluation),
                                integer, {}, bindings, stdScope, evaluation),
             CppTypeResolver::SpecializationMatch::Unknown);
    QCOMPARE(resolver.matchType(resolver.parseTypeText("std::Cycle<int>", evaluation),
                                integer, {}, bindings, stdScope, evaluation),
             CppTypeResolver::SpecializationMatch::Unknown);
}
