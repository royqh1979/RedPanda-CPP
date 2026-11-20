#include <QTest>
#include "test_cppdeclarationparser.h"

namespace {
const DeclarationInfo *findDeclaration(const QVector<DeclarationInfo> &declarations,
                                       const QString &name)
{
    for (const DeclarationInfo &declaration : declarations) {
        for (const DeclaratorInfo &declarator : declaration.declarators) {
            QString qualifiedName = declarator.qualifier.isEmpty()
                    ? declarator.name : declarator.qualifier + "::" + declarator.name;
            if (declarator.name == name || qualifiedName == name)
                return &declaration;
        }
        if (const DeclarationInfo *child = findDeclaration(declaration.children, name))
            return child;
    }
    return nullptr;
}

const DeclarationInfo *findTopLevelDeclaration(const QVector<DeclarationInfo> &declarations,
                                               const QString &name)
{
    for (const DeclarationInfo &declaration : declarations) {
        for (const DeclaratorInfo &declarator : declaration.declarators) {
            if (declarator.name == name)
                return &declaration;
        }
    }
    return nullptr;
}
}

void TestCppDeclarationParser::test_type_template_arguments()
{
    TypeInfo stringType;
    stringType.qualifiedName = "std::string";
    TypeInfo vectorType;
    vectorType.qualifiedName = "std::vector";
    vectorType.templateArguments.append(TemplateArgument::fromType(stringType));
    QCOMPARE(vectorType.toString(), "std::vector<std::string>");

    TypeInfo elementsType;
    elementsType.qualifiedName = "std::ranges::elements_view";
    elementsType.templateArguments.append(TemplateArgument::fromType(vectorType));
    elementsType.templateArguments.append(TemplateArgument::fromConstant("2"));
    QCOMPARE(elementsType.toString(),
             "std::ranges::elements_view<std::vector<std::string>,2>");
}

void TestCppDeclarationParser::test_array_declarator_order()
{
    DeclaratorInfo arrayOfPointers;
    arrayOfPointers.name = "a";
    arrayOfPointers.derivations = {TypeDerivation::array("[5]"), TypeDerivation::pointer()};
    QCOMPARE(arrayOfPointers.toString(), "*a[5]");

    TypeInfo arrayOfPointersType;
    arrayOfPointersType.qualifiedName = "int";
    arrayOfPointersType.derivations = arrayOfPointers.derivations;
    QCOMPARE(arrayOfPointersType.toString(), "int *[5]");

    DeclaratorInfo pointerToArray;
    pointerToArray.name = "a";
    pointerToArray.derivations = {TypeDerivation::pointer(), TypeDerivation::array("[5]")};
    QCOMPARE(pointerToArray.toString(), "(*a)[5]");

    TypeInfo pointerToArrayType;
    pointerToArrayType.qualifiedName = "int";
    pointerToArrayType.derivations = pointerToArray.derivations;
    QCOMPARE(pointerToArrayType.toString(), "int (*)[5]");
}

void TestCppDeclarationParser::test_function_pointer_declarator()
{
    auto intType = std::make_shared<TypeInfo>();
    intType->qualifiedName = "int";
    ParameterInfo parameter;
    parameter.type = intType;

    DeclaratorInfo callback;
    callback.name = "callback";
    callback.derivations = {TypeDerivation::pointer(),
                            TypeDerivation::function({parameter})};
    QCOMPARE(callback.toString(), "(*callback)(int)");

    TypeInfo callbackType;
    callbackType.qualifiedName = "void";
    callbackType.derivations = callback.derivations;
    QCOMPARE(callbackType.toString(), "void (*)(int)");
}

void TestCppDeclarationParser::test_function_returning_function_pointer()
{
    auto doubleType = std::make_shared<TypeInfo>();
    doubleType->qualifiedName = "double";
    ParameterInfo parameter;
    parameter.type = doubleType;

    DeclaratorInfo factory;
    factory.name = "factory";
    factory.derivations = {TypeDerivation::function(), TypeDerivation::pointer(),
                           TypeDerivation::function({parameter})};
    QCOMPARE(factory.toString(), "(*factory())(double)");

    TypeInfo factoryType;
    factoryType.qualifiedName = "int";
    factoryType.derivations = factory.derivations;
    QCOMPARE(factoryType.toString(), "int (*())(double)");
}

void TestCppDeclarationParser::test_member_function_qualifiers()
{
    TypeDerivation function = TypeDerivation::function();
    function.qualifiers = TypeQualifier::Const;
    function.refQualifier = ReferenceQualifier::Rvalue;

    DeclaratorInfo method;
    method.name = "method";
    method.qualifier = "N::Class";
    method.derivations = {function};
    QCOMPARE(method.toString(), "N::Class::method() const &&");
}

void TestCppDeclarationParser::test_parameter_rendering()
{
    auto intType = std::make_shared<TypeInfo>();
    intType->qualifiedName = "int";
    ParameterInfo intParameter;
    intParameter.type = intType;
    intParameter.name = "value";
    QCOMPARE(intParameter.toString(), "int value");
    QCOMPARE(intParameter.toString(false), "int");

    TypeInfo callbackType;
    callbackType.qualifiedName = "void";
    callbackType.derivations = {TypeDerivation::pointer(), TypeDerivation::function()};
    ParameterInfo callbackParameter;
    callbackParameter.type = std::make_shared<TypeInfo>(callbackType);
    callbackParameter.name = "callback";
    QCOMPARE(callbackParameter.toString(), "void (*callback)()");
}

void TestCppDeclarationParser::test_decl_specifier_flags()
{
    DeclSpecifierInfo specifiers;
    specifiers.properties.setFlag(DeclSpecifierProperty::Static);
    specifiers.properties.setFlag(DeclSpecifierProperty::Register);
    QVERIFY(specifiers.properties.testFlag(DeclSpecifierProperty::Static));
    QVERIFY(specifiers.properties.testFlag(DeclSpecifierProperty::Register));
    QVERIFY(!specifiers.properties.testFlag(DeclSpecifierProperty::Extern));
}

void TestCppDeclarationParser::test_parse_simple_declaration()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("static const unsigned long value;", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "value");
    QCOMPARE(declaration.specifiers.type.qualifiedName, "unsigned long");
    QVERIFY(declaration.specifiers.type.qualifiers.testFlag(TypeQualifier::Const));
    QVERIFY(declaration.specifiers.properties.testFlag(DeclSpecifierProperty::Static));
}

void TestCppDeclarationParser::test_parse_multiple_declarators()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("int a = 1, *p, f(), (*pf)(double);", declaration));
    QCOMPARE(declaration.declarators.count(), 4);
    QCOMPARE(declaration.declarators[0].name, "a");
    QCOMPARE(declaration.declarators[1].name, "p");
    QCOMPARE(declaration.declarators[2].name, "f");
    QCOMPARE(declaration.declarators[3].name, "pf");

    TypeInfo functionPointer = declaration.specifiers.type;
    functionPointer.derivations = declaration.declarators[3].derivations;
    QCOMPARE(functionPointer.toString(), "int (*)(double)");
}

void TestCppDeclarationParser::test_parse_function_pointer_declaration()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("void (*callback)(int);", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "callback");
    QCOMPARE(declaration.declarators[0].derivations.count(), 2);
    QCOMPARE(declaration.declarators[0].derivations[0].kind,
             TypeDerivationKind::Pointer);
    QCOMPARE(declaration.declarators[0].derivations[1].kind,
             TypeDerivationKind::Function);
}

void TestCppDeclarationParser::test_parse_complex_parameter_declaration()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText(
                "int complex(int a, int* p, int (*(*x)(double))[3]);", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QVector<TypeDerivation> derivations = declaration.declarators[0].derivations;
    QCOMPARE(derivations.count(), 1);
    QCOMPARE(derivations[0].kind, TypeDerivationKind::Function);
    QCOMPARE(derivations[0].parameters.count(), 3);
    ParameterInfo complex = derivations[0].parameters[2];
    QCOMPARE(complex.name, "x");
    QCOMPARE(complex.type->toString(), "int (*(*)(double))[3]");
}

void TestCppDeclarationParser::test_parse_trailing_return_declaration()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("auto fpif(int)->int(*)(int);", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "fpif");
    QVector<TypeDerivation> derivations = declaration.declarators[0].derivations;
    QCOMPARE(derivations.count(), 1);
    QCOMPARE(derivations[0].trailingReturnType, "int (*)(int)");
}

void TestCppDeclarationParser::test_parse_member_function_declarators()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("virtual int f(char) const, g(int) &&;", declaration));
    QVERIFY(declaration.specifiers.properties.testFlag(DeclSpecifierProperty::Virtual));
    QCOMPARE(declaration.declarators.count(), 2);
    QCOMPARE(declaration.declarators[0].name, "f");
    QCOMPARE(declaration.declarators[1].name, "g");
    QVERIFY(declaration.declarators[0].derivations[0].qualifiers.testFlag(
            TypeQualifier::Const));
    QCOMPARE(declaration.declarators[1].derivations[0].refQualifier,
             ReferenceQualifier::Rvalue);
}

void TestCppDeclarationParser::test_parse_direct_initialization_not_function()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("std::string value(\"text\");", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "value");
    QVERIFY(declaration.declarators[0].derivations.isEmpty());
    QCOMPARE(declaration.specifiers.type.toString(), "std::string");

    QVERIFY(parseDeclarationText("Foo F(0);", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "F");
    QVERIFY(declaration.declarators[0].derivations.isEmpty());

    for (const QString &text : {"Foo F(this);", "Foo F(nullptr);", "Foo F(true);"}) {
        QVERIFY(parseDeclarationText(text, declaration));
        QCOMPARE(declaration.declarators.count(), 1);
        QCOMPARE(declaration.declarators[0].name, "F");
        QVERIFY(declaration.declarators[0].derivations.isEmpty());
    }

    QVERIFY(parseDeclarationText("Foo F();", declaration));
    QCOMPARE(declaration.declarators[0].derivations.count(), 1);
    QCOMPARE(declaration.declarators[0].derivations[0].kind, TypeDerivationKind::Function);
}

void TestCppDeclarationParser::test_parse_template_type_arguments()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("std::vector<std::vector<int>> values;", declaration));
    QCOMPARE(declaration.declarators[0].name, "values");
    QCOMPARE(declaration.specifiers.type.qualifiedName, "std::vector");
    QCOMPARE(declaration.specifiers.type.qualifiedNameText, "std::vector<std::vector<int>>");
    QVERIFY(declaration.specifiers.type.templateArguments.isEmpty());
    QCOMPARE(declaration.specifiers.type.toString(), "std::vector<std::vector<int>>");

    DeclarationInfo dependent;
    QVERIFY(parseDeclarationText("typename A<T>::template B<U>::type value;", dependent));
    QCOMPARE(dependent.declarators[0].name, "value");
    QCOMPARE(dependent.specifiers.type.qualifiedName, "typename A::B::type");
    QCOMPARE(dependent.specifiers.type.qualifiedNameText,
             "typename A<T>::template B<U>::type");

    DeclarationInfo multiSegment;
    QVERIFY(parseDeclarationText(
                "typename A<B<int>>::template C<D<E<int>>>::type value;", multiSegment));
    QCOMPARE(multiSegment.declarators[0].name, "value");
    QCOMPARE(multiSegment.specifiers.type.qualifiedName, "typename A::C::type");
    QCOMPARE(multiSegment.specifiers.type.qualifiedNameText,
             "typename A<B<int>>::template C<D<E<int>>>::type");

    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/templates.cpp";
    builder.build(declaration, context);
    CppTypeResolver resolver(model);
    TypeInfo resolved = resolver.resolve(declaration.specifiers.type);
    QCOMPARE(resolved.templateArguments.count(), 1);
    TemplateArgument argument = resolved.templateArguments[0];
    QCOMPARE(argument.kind, TemplateArgumentKind::Type);
    QVERIFY(argument.type);
    QCOMPARE(argument.type->toString(), "std::vector<int>");

    DeclarationInfo multiline;
    QVERIFY(parseDeclarationText("std::vector<\n  std::vector<int>\n> values;", multiline));
    QCOMPARE(multiline.specifiers.type.qualifiedNameText, "std::vector<   std::vector<int> >");
    QCOMPARE(multiline.declarators[0].name, "values");
    TypeInfo resolvedMultiline = resolver.resolve(multiline.specifiers.type);
    QCOMPARE(resolvedMultiline.templateArguments.count(), 1);
    QCOMPARE(resolvedMultiline.templateArguments[0].type->toString(), "std::vector<int>");
}

void TestCppDeclarationParser::test_parse_operator_declaration()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("int operator()(int value) const;", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "operator()");
    QCOMPARE(declaration.declarators[0].derivations.count(), 1);
    TypeDerivation function = declaration.declarators[0].derivations[0];
    QCOMPARE(function.kind, TypeDerivationKind::Function);
    QCOMPARE(function.parameters.count(), 1);
    QVERIFY(function.qualifiers.testFlag(TypeQualifier::Const));
}

void TestCppDeclarationParser::test_parse_member_pointer_declaration()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("void (S::*member)(int);", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "member");
    QCOMPARE(declaration.declarators[0].derivations.count(), 2);
    QCOMPARE(declaration.declarators[0].derivations[0].kind,
             TypeDerivationKind::MemberPointer);
    QCOMPARE(declaration.declarators[0].derivations[0].memberClass, "S");
    QCOMPARE(declaration.declarators[0].derivations[1].kind,
             TypeDerivationKind::Function);
}

void TestCppDeclarationParser::test_parse_function_template()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("template<typename T> T identity(T value);", declaration));
    QCOMPARE(declaration.templateKind, TemplateDeclarationKind::Primary);
    QCOMPARE(declaration.templateParameters.count(), 1);
    QCOMPARE(declaration.templateParameters[0].name, "T");
    QCOMPARE(declaration.templateParameters[0].kind, TemplateParameterKind::Type);
    QCOMPARE(declaration.declarators[0].name, "identity");
    QCOMPARE(declaration.declarators[0].derivations[0].parameters[0].name, "value");
}

void TestCppDeclarationParser::test_parse_template_parameter_kinds()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("template<typename... Args> void f(Args... args);", declaration));
    QCOMPARE(declaration.templateParameters.count(), 1);
    QCOMPARE(declaration.templateParameters[0].name, "Args");
    QVERIFY(declaration.templateParameters[0].parameterPack);

    QVERIFY(parseDeclarationText(
                "template<int Size, template<typename...> class Container> int value;",
                declaration));
    QCOMPARE(declaration.templateParameters.count(), 2);
    QCOMPARE(declaration.templateParameters[0].kind, TemplateParameterKind::NonType);
    QCOMPARE(declaration.templateParameters[0].name, "Size");
    QVERIFY(declaration.templateParameters[0].type);
    QCOMPARE(declaration.templateParameters[0].type->qualifiedName, "int");
    QCOMPARE(declaration.templateParameters[1].kind, TemplateParameterKind::TemplateTemplate);
    QCOMPARE(declaration.templateParameters[1].name, "Container");
}

void TestCppDeclarationParser::test_parse_template_specializations()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("template<> int value<int>;", declaration));
    QCOMPARE(declaration.templateKind, TemplateDeclarationKind::ExplicitSpecialization);
    QVERIFY(declaration.templateParameters.isEmpty());
    QCOMPARE(declaration.declarators[0].name, "value<int>");
    QCOMPARE(declaration.declarators[0].templatePattern.qualifiedName, "value");
    QCOMPARE(declaration.declarators[0].templatePattern.templateArguments.count(), 1);
    QCOMPARE(declaration.declarators[0].templatePattern.templateArguments[0].type->qualifiedName,
             "int");

    QVERIFY(parseDeclarationText("template<typename T> int value<T*>;", declaration));
    QCOMPARE(declaration.templateKind, TemplateDeclarationKind::PartialSpecialization);
    QCOMPARE(declaration.templateParameters.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "value<T*>");
    QCOMPARE(declaration.declarators[0].templatePattern.templateArguments.count(), 1);
    QCOMPARE(declaration.declarators[0].templatePattern.templateArguments[0].type->qualifiedName,
             "T");
    QCOMPARE(declaration.declarators[0].templatePattern.templateArguments[0].type->derivations.count(),
             1);
    QCOMPARE(declaration.declarators[0].templatePattern.templateArguments[0].type->derivations[0].kind,
             TypeDerivationKind::Pointer);

    QVERIFY(parseDeclarationText("template<typename T> int value< T * >;", declaration));
    QCOMPARE(declaration.declarators[0].name, "value<T*>");
    QCOMPARE(declaration.declarators[0].templatePattern.templateArguments.count(), 1);
    QCOMPARE(declaration.declarators[0].templatePattern.templateArguments[0].type->derivations.count(),
             1);

    QVERIFY(parseDeclarationText("template<typename T> int value<Optional<T*>>;", declaration));
    const TypeInfo &nested = *declaration.declarators[0].templatePattern.templateArguments[0].type;
    QCOMPARE(nested.qualifiedName, "Optional");
    QCOMPARE(nested.qualifiedNameText, "Optional<T*>");
    QVERIFY(nested.templateArguments.isEmpty());
}

void TestCppDeclarationParser::test_syntax_class_declarations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxFile("classes.cpp", declarations));
    const DeclarationInfo *s = findDeclaration(declarations, "S");
    QVERIFY(s);
    QCOMPARE(s->kind, DeclarationKind::Class);
    QCOMPARE(s->range.start, (SourcePosition{5, 0}));
    QCOMPARE(s->bodyRange.start, (SourcePosition{6, 0}));
    QCOMPARE(s->bodyRange.end, (SourcePosition{24, 0}));
    for (const char *name : {"d1", "a", "d2", "f1", "d3", "d4", "f2",
                             "NestedS", "d5", "d6", "value_type", "pointer_type"}) {
        QVERIFY2(findDeclaration(s->children, QString::fromLatin1(name)), name);
    }
    const DeclarationInfo *m = findDeclaration(declarations, "M");
    QVERIFY(m);
    QVERIFY(findDeclaration(m->children, "operator()"));

    const DeclarationInfo *derived = findDeclaration(declarations, "Derived");
    QVERIFY(derived);
    QCOMPARE(derived->baseClasses, QStringList{"public Base"});
    QVERIFY(findDeclaration(derived->children, "Base::d"));

    const DeclarationInfo *memberTemplates = findDeclaration(declarations, "MemberTemplates");
    QVERIFY(memberTemplates);
    const DeclarationInfo *nested = findDeclaration(memberTemplates->children, "NestedS");
    QVERIFY(nested);
    QCOMPARE(nested->templateKind, TemplateDeclarationKind::Primary);
}

void TestCppDeclarationParser::test_class_base_list_nested_commas()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "struct D :\n"
                "    public A<std::pair<int, float>>,\n"
                "    virtual B<std::array<std::pair<int, double>, 2>>,\n"
                "    C<void(*)(int, float)> {};\n", declarations));
    const DeclarationInfo *derived = findDeclaration(declarations, "D");
    QVERIFY(derived);
    QCOMPARE(derived->baseClasses, QStringList({
        "public A<std::pair<int,float>>",
        "virtual B<std::array<std::pair<int,double>,2>>",
        "C<void(*)(int,float)>"
    }));
}

void TestCppDeclarationParser::test_syntax_template_declarations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxFile("templates.cpp", declarations));
    const DeclarationInfo *a = findDeclaration(declarations, "A");
    QVERIFY(a);
    QCOMPARE(a->templateKind, TemplateDeclarationKind::Primary);
    QVERIFY(findDeclaration(a->children, "f"));

    const DeclarationInfo *specialization = findDeclaration(declarations, "A<MyClass>");
    QVERIFY(specialization);
    QCOMPARE(specialization->templateKind, TemplateDeclarationKind::ExplicitSpecialization);

    const DeclarationInfo *ptr = findDeclaration(declarations, "Ptr");
    QVERIFY(ptr);
    QCOMPARE(ptr->kind, DeclarationKind::Alias);
    QCOMPARE(ptr->specifiers.type.toString(), "T *");

    const DeclarationInfo *variadic = findTopLevelDeclaration(declarations, "f");
    QVERIFY(variadic);
    QCOMPARE(variadic->templateParameters.count(), 1);
    QVERIFY(variadic->templateParameters[0].parameterPack);
}

void TestCppDeclarationParser::test_syntax_namespace_declarations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxFile("namespaces.cpp", declarations));
    const DeclarationInfo *q = findDeclaration(declarations, "Q");
    QVERIFY(q);
    QCOMPARE(q->kind, DeclarationKind::Namespace);
    QVERIFY(findDeclaration(q->children, "V"));
    QVERIFY(findDeclaration(q->children, "V::f"));
    QVERIFY(findDeclaration(q->children, "V::C::m"));

    const DeclarationInfo *nested = findDeclaration(declarations, "A::B::C");
    QVERIFY(nested);
    QVERIFY(findDeclaration(nested->children, "nested_value"));

    const DeclarationInfo *lib1 = findDeclaration(declarations, "Lib_1");
    QVERIFY(lib1);
    QVERIFY(lib1->specifiers.properties.testFlag(DeclSpecifierProperty::Inline));
    QVERIFY(findDeclaration(declarations, "A<MyClass>"));
}

void TestCppDeclarationParser::test_syntax_enum_declarations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxFile("enums.cpp", declarations));
    const DeclarationInfo *color = findDeclaration(declarations, "color");
    QVERIFY(color);
    QCOMPARE(color->kind, DeclarationKind::Enum);
    QCOMPARE(color->enumerators.count(), 4);
    QCOMPARE(color->enumerators[2].name, "green");
    QCOMPARE(color->enumerators[2].value, "20");
    QCOMPARE(color->enumerators[3].name, "blue");

    const DeclarationInfo *altitude = findDeclaration(declarations, "altitude");
    QVERIFY(altitude);
    QVERIFY(altitude->enumScoped);
    QCOMPARE(altitude->enumerators[0].value, "'h'");
    QVERIFY(findDeclaration(declarations, "E11"));
}

void TestCppDeclarationParser::test_syntax_using_declarations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "using namespace std;\n"
                "using enum fruit;\n"
                "using Base::d;\n"
                "template<typename T> using Ptr = T*;\n", declarations));
    QCOMPARE(declarations.count(), 4);
    QCOMPARE(declarations[0].kind, DeclarationKind::Using);
    QCOMPARE(declarations[0].declarators[0].name, "std");
    QCOMPARE(declarations[1].kind, DeclarationKind::Using);
    QCOMPARE(declarations[1].declarators[0].name, "fruit");
    QCOMPARE(declarations[2].kind, DeclarationKind::Using);
    QCOMPARE(declarations[2].declarators[0].name, "Base::d");
    QCOMPARE(declarations[3].kind, DeclarationKind::Alias);
    QCOMPARE(declarations[3].templateKind, TemplateDeclarationKind::Primary);
    QCOMPARE(declarations[3].specifiers.type.toString(), "T *");
}

void TestCppDeclarationParser::test_failed_declaration_restores_cursor()
{
    tokenize({"namespace Broken ;"});
    CppTokenCursor cursor(mTokenizer);
    CppSyntaxParser parser(cursor);
    DeclarationInfo declaration;
    QVERIFY(!parser.parseDeclaration(declaration));
    QCOMPARE(cursor.index(), 0);
    tokenize({"namespace Broken ( )"});
    CppTokenCursor parenthesisCursor(mTokenizer);
    CppSyntaxParser parenthesisParser(parenthesisCursor);
    QVERIFY(!parenthesisParser.parseDeclaration(declaration));
    QCOMPARE(parenthesisCursor.index(), 0);
    tokenize({"std::vector<int value;"});
    CppTokenCursor templateCursor(mTokenizer);
    CppDeclarationParser templateParser(templateCursor);
    QVERIFY(!templateParser.parseDeclaration(declaration));
    QCOMPARE(templateCursor.index(), 0);
    tokenize({"value < (unsigned)base) return value;"});
    CppTokenCursor comparisonCursor(mTokenizer);
    CppDeclarationParser comparisonParser(comparisonCursor);
    QVERIFY(!comparisonParser.parseDeclaration(declaration));
    QCOMPARE(comparisonCursor.index(), 0);
}


void TestCppDeclarationParser::test_non_declaration_keywords_are_rejected()
{
    const QStringList keywords{
        "_Pragma", "__attribute", "__attribute__", "__asm",
        "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel",
        "atomic_commit", "atomic_noexcept", "bitand", "bitor", "break",
        "case", "catch", "compl", "concept", "const_cast", "continue",
        "co_await", "co_return", "co_yield", "default", "delete", "do",
        "dynamic_cast", "else", "false", "for", "goto", "if",
        "new", "noexcept", "not", "not_eq", "nullptr", "or", "or_eq",
        "private", "protected", "public", "reflexpr", "reinterpret_cast",
        "requires", "return", "sizeof", "static_assert", "static_cast",
        "switch", "this", "throw", "true", "try", "typeid", "while",
        "xor", "xor_eq"
    };
    for (const QString &keyword : keywords) {
        DeclarationInfo declaration;
        QVERIFY2(!parseDeclarationText(keyword + " ttt;", declaration),
                 keyword.toUtf8().constData());
    }

    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("export template<typename T> int f();", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators.first().name, "f");
}

void TestCppDeclarationParser::test_requires_clauses_are_consumed()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> int f() requires true { return 1; }\n"
                "template<typename T> int g() requires requires(T value) { value + 1; } { return 2; }\n"
                "template<typename T> requires (!is_array_v<T>) using NonArray = T;\n"
                "template<typename T> requires requires(T value) { value + 1; } using Addable = T;\n"
                "template<typename T> requires (!is_array_v<T>) class Constrained { public: Constrained(); };\n"
                "template<typename T> requires is_same_v<T, int> T identity(T value);\n"
                "int after;\n",
                declarations));
    QCOMPARE(declarations.count(), 7);
    QCOMPARE(declarations[0].declarators.first().name, "f");
    QVERIFY(declarations[0].bodyRange.isValid());
    QCOMPARE(declarations[1].declarators.first().name, "g");
    QVERIFY(declarations[1].bodyRange.isValid());
    QCOMPARE(declarations[2].kind, DeclarationKind::Alias);
    QCOMPARE(declarations[2].declarators.first().name, "NonArray");
    QCOMPARE(declarations[3].kind, DeclarationKind::Alias);
    QCOMPARE(declarations[3].declarators.first().name, "Addable");
    QCOMPARE(declarations[4].kind, DeclarationKind::Class);
    QCOMPARE(declarations[4].declarators.first().name, "Constrained");
    QCOMPARE(declarations[5].declarators.first().name, "identity");
    QCOMPARE(declarations[6].declarators.first().name, "after");
    QVERIFY(declarations[2].templateRequiresClause.isEmpty());
    QVERIFY(declarations[0].declarators.first().derivations.first().requiresClause.isEmpty());

    QVector<DeclarationInfo> constrained;
    QVERIFY(parseSyntaxText(
                "template<typename T> requires (!is_array_v<T>) using NonArray = T;\n"
                "template<typename T> int f() requires true;\n",
                constrained, CppVersion::Cpp20));
    QCOMPARE(constrained[0].templateRequiresClause, "(!is_array_v<T>)");
    QCOMPARE(constrained[1].declarators.first().derivations.first().requiresClause, "true");

    DeclarationInfo constant;
    QVERIFY(parseDeclarationText(
                "template<typename T> constexpr bool is_array_v = __is_array(T);",
                constant, CppVersion::Cpp20));
    QCOMPARE(constant.declarators.first().initializerText, "__is_array(T)");
}

void TestCppDeclarationParser::test_explicit_bool_declarations()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> class ExplicitBox {\n"
                "public:\n"
                "  explicit(true) ExplicitBox(int);\n"
                "  T& operator*() &;\n"
                "};\n", declarations));
    QCOMPARE(declarations.count(), 1);
    QCOMPARE(declarations.first().children.count(), 2);
    QVERIFY(declarations.first().children.first().specifiers.properties.testFlag(
                DeclSpecifierProperty::Explicit));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/explicit_bool.cpp";
    builder.build(declarations.first(), context);
    CppTypeResolver resolver(model);
    PStatement box = resolver.findStatement("ExplicitBox");
    QVERIFY(box);
    QVERIFY(resolver.findStatement("ExplicitBox::operator*"));
    PStatement constructor = resolver.findStatement("ExplicitBox::ExplicitBox");
    QVERIFY(constructor);
    QVERIFY(std::dynamic_pointer_cast<Statement>(constructor)->declSpecifierProperties.testFlag(
                DeclSpecifierProperty::Explicit));
}

void TestCppDeclarationParser::test_requires_before_member_declaration()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "template<typename T> class RequiresMember {\n"
                "public:\n"
                "  template<typename U> requires is_convertible_v<U, T>\n"
                "  RequiresMember(const U&) { }\n"
                "  T& operator*() &;\n"
                "  int after;\n"
                "};\n", declarations));
    QCOMPARE(declarations.count(), 1);
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/requires_member.cpp";
    builder.build(declarations.first(), context);
    CppTypeResolver resolver(model);
    QVERIFY(resolver.findStatement("RequiresMember::RequiresMember"));
    QVERIFY(resolver.findStatement("RequiresMember::operator*"));
    QVERIFY(resolver.findStatement("RequiresMember::after"));
}

void TestCppDeclarationParser::test_function_try_block_recovery()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "class TryBox {\n"
                "public:\n"
                "  TryBox() try { } catch (...) { }\n"
                "  int after;\n"
                "};\n", declarations));
    QCOMPARE(declarations.count(), 1);
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/function_try_block.cpp";
    builder.build(declarations.first(), context);
    CppTypeResolver resolver(model);
    QVERIFY(resolver.findStatement("TryBox::after"));
}

void TestCppDeclarationParser::test_constructor_declarations_with_specifiers()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
                "class C {\n"
                "public:\n"
                "  constexpr C();\n"
                "  explicit inline C(int value);\n"
                "};\n"
                "constexpr C::C() { }\n"
                "C::C(int value) { }\n"
                "template<typename T> class Box { public: constexpr Box(); };\n",
                declarations));
    StatementModel model;
    CppStatementBuilder builder(model);
    StatementBuildContext context;
    context.fileName = "/tmp/constructors.cpp";
    for (const DeclarationInfo &declaration : declarations)
        builder.build(declaration, context);
    CppTypeResolver resolver(model);

    PStatement c = resolver.findStatement("C");
    QVERIFY(c);
    const auto constructors = std::dynamic_pointer_cast<Statement>(c)->children.values("C");
    QCOMPARE(constructors.count(), 2);
    bool defaultConstructor = false;
    bool explicitConstructor = false;
    for (const std::weak_ptr<AbstractStatement> &weak : constructors) {
        PStatement constructor = weak.lock();
        QVERIFY(constructor);
        QCOMPARE(constructor->kind, StatementKind::Constructor);
        QVERIFY(statementHasDefinition(constructor));
        if (std::dynamic_pointer_cast<Statement>(constructor)->parameters.isEmpty()) {
            defaultConstructor = true;
            QVERIFY(std::dynamic_pointer_cast<Statement>(constructor)->declSpecifierProperties.testFlag(DeclSpecifierProperty::Constexpr));
        } else {
            explicitConstructor = true;
            QVERIFY(std::dynamic_pointer_cast<Statement>(constructor)->declSpecifierProperties.testFlag(DeclSpecifierProperty::Explicit));
        }
    }
    QVERIFY(defaultConstructor && explicitConstructor);
    for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(c)->children)
        QVERIFY(!weak.lock()->command.isEmpty());

    PStatement box = resolver.findStatement("Box");
    QVERIFY(box);
    PStatement boxConstructor = findStatement(std::dynamic_pointer_cast<Statement>(box)->children, "Box");
    QVERIFY(boxConstructor);
    QCOMPARE(boxConstructor->kind, StatementKind::Constructor);
}
