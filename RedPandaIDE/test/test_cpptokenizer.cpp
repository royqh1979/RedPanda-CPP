#include <QTest>
#include <QFile>
#include "test_cpptokenizer.h"

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

TestCppTokenizer::TestCppTokenizer(QObject *parent)
    : QObject(parent)
{
}

void TestCppTokenizer::tokenize(const QStringList &lines)
{
    mTokenizer.tokenize(lines);
}

QStringList TestCppTokenizer::tokenTexts() const
{
    QStringList result;
    for (const CppTokenizer::Token &token : mTokenizer.tokens())
        result.append(token.text);
    return result;
}

bool TestCppTokenizer::parseDeclarationText(const QString &text, DeclarationInfo &result)
{
    CppTokenizer tokenizer;
    tokenizer.tokenize({text});
    CppTokenCursor cursor(tokenizer);
    CppDeclarationParser parser(cursor);
    return parser.parseDeclaration(result);
}

bool TestCppTokenizer::parseSyntaxText(const QString &text, QVector<DeclarationInfo> &result)
{
    CppTokenizer tokenizer;
    tokenizer.tokenize(text.split('\n'));
    CppTokenCursor cursor(tokenizer);
    CppSyntaxParser parser(cursor);
    while (!cursor.atEnd()) {
        if (cursor.current().kind == CppTokenizer::TokenKind::Preprocessor
                || cursor.current().kind == CppTokenizer::TokenKind::FileBoundary) {
            cursor.advance();
            continue;
        }
        int before = cursor.index();
        DeclarationInfo declaration;
        if (!parser.parseDeclaration(declaration))
            return false;
        result.append(declaration);
        if (cursor.currentText() == "{") {
            cursor.skipToMatch();
            if (!cursor.atEnd())
                cursor.advance();
        }
        if (cursor.index() == before)
            return false;
    }
    return true;
}

bool TestCppTokenizer::parseSyntaxFile(const QString &fileName, QVector<DeclarationInfo> &result)
{
    QFile file("resources/syntax/" + fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QString text = QString::fromUtf8(file.readAll());
    text.remove('\r');
    return parseSyntaxText(text, result);
}

void TestCppTokenizer::test_identifiers_and_operators()
{
    tokenize({"value>>=1;"});
    QCOMPARE(tokenTexts(), QStringList({"value", ">>=", "1", ";"}));
    QCOMPARE(mTokenizer[1].kind, CppTokenizer::TokenKind::Operator);
}

void TestCppTokenizer::test_qualified_names_and_templates_are_split()
{
    tokenize({"std::vector<std::vector<int>> values;"});
    QCOMPARE(tokenTexts(), QStringList({
                 "std", "::", "vector", "<", "std", "::", "vector", "<",
                 "int", ">>", "values", ";"}));
}

void TestCppTokenizer::test_array_and_lambda_brackets_are_split()
{
    tokenize({"auto callback = [value](int n) { return n + value; };"});
    QCOMPARE(tokenTexts(), QStringList({
                 "auto", "callback", "=", "[", "value", "]", "(", "int", "n",
                 ")", "{", "return", "n", "+", "value", ";", "}", ";"}));
    QVERIFY(mTokenizer[3].matchIndex >= 0);
    QVERIFY(mTokenizer[6].matchIndex >= 0);
}

void TestCppTokenizer::test_integer_and_floating_literals()
{
    tokenize({"0 0x1f 0b10 1'000 1.0 .5 1e3 0x1p2 1f 1ULL 42_km"});
    QCOMPARE(tokenTexts().size(), 11);
    for (int i : {0, 1, 2, 3, 9, 10})
        QCOMPARE(mTokenizer[i].kind, CppTokenizer::TokenKind::IntegerLiteral);
    for (int i : {4, 5, 6, 7, 8})
        QCOMPARE(mTokenizer[i].kind, CppTokenizer::TokenKind::FloatingLiteral);
}

void TestCppTokenizer::test_string_and_character_literals()
{
    tokenize({"auto s = \"text\"s; auto c = 'c'; auto u = u8\"utf8\";"});
    QCOMPARE(mTokenizer[3].text, "\"text\"s");
    QCOMPARE(mTokenizer[3].kind, CppTokenizer::TokenKind::StringLiteral);
    QCOMPARE(mTokenizer[8].text, "'c'");
    QCOMPARE(mTokenizer[8].kind, CppTokenizer::TokenKind::CharacterLiteral);
    QCOMPARE(mTokenizer[13].text, "u8\"utf8\"");
}

void TestCppTokenizer::test_escaped_literals()
{
    tokenize({"auto s = \"a\\\"b\"; auto c = '\\'';"});
    QCOMPARE(mTokenizer[3].text, "\"a\\\"b\"");
    QCOMPARE(mTokenizer[3].kind, CppTokenizer::TokenKind::StringLiteral);
    QCOMPARE(mTokenizer[8].text, "'\\''");
    QCOMPARE(mTokenizer[8].kind, CppTokenizer::TokenKind::CharacterLiteral);
}

void TestCppTokenizer::test_raw_string_with_custom_delimiter()
{
    tokenize({"auto s = R\"tag(first", "second)tag\";", "int after;"});
    QCOMPARE(mTokenizer[3].text, "R\"tag(first\nsecond)tag\"");
    QCOMPARE(mTokenizer[3].kind, CppTokenizer::TokenKind::StringLiteral);
    QCOMPARE(mTokenizer[6].text, "after");
    QCOMPARE(mTokenizer[6].position.line, 2);
}

void TestCppTokenizer::test_unterminated_raw_string()
{
    tokenize({"auto s = R\"tag(unterminated"});
    QCOMPARE(mTokenizer[3].text, "R\"tag(unterminated");
    QCOMPARE(mTokenizer[3].kind, CppTokenizer::TokenKind::StringLiteral);
    QCOMPARE(mTokenizer.tokenCount(), 4);
}

void TestCppTokenizer::test_balanced_match_indices()
{
    tokenize({"int main(){ auto x = (values[0]); }"});
    int leftBrace = tokenTexts().indexOf("{");
    int rightBrace = tokenTexts().indexOf("}");
    int leftParenthesis = tokenTexts().lastIndexOf("(");
    int rightParenthesis = tokenTexts().lastIndexOf(")");
    int leftBracket = tokenTexts().indexOf("[");
    int rightBracket = tokenTexts().indexOf("]");
    QCOMPARE(mTokenizer[leftBrace].matchIndex, rightBrace);
    QCOMPARE(mTokenizer[rightBrace].matchIndex, leftBrace);
    QCOMPARE(mTokenizer[leftParenthesis].matchIndex, rightParenthesis);
    QCOMPARE(mTokenizer[rightParenthesis].matchIndex, leftParenthesis);
    QCOMPARE(mTokenizer[leftBracket].matchIndex, rightBracket);
    QCOMPARE(mTokenizer[rightBracket].matchIndex, leftBracket);
}

void TestCppTokenizer::test_unmatched_pairs_use_file_end()
{
    tokenize({"int main() {", "  if (x > 0"});
    int leftBrace = tokenTexts().indexOf("{");
    int leftParenthesis = tokenTexts().lastIndexOf("(");
    QCOMPARE(mTokenizer[leftBrace].matchIndex, mTokenizer.tokenCount());
    QCOMPARE(mTokenizer[leftParenthesis].matchIndex, mTokenizer.tokenCount());
}

void TestCppTokenizer::test_file_boundaries_and_file_names()
{
    tokenize({"#include /tmp/main.cpp:-1", "int root;", "#include /tmp/a.h:-1",
              "int included;", "#include /tmp/main.cpp:1", "int after;"});
    QCOMPARE(mTokenizer[0].kind, CppTokenizer::TokenKind::FileBoundary);
    QCOMPARE(mTokenizer[1].fileName, "/tmp/main.cpp");
    QCOMPARE(mTokenizer[5].fileName, "/tmp/a.h");
    QCOMPARE(mTokenizer[9].fileName, "/tmp/main.cpp");
    QCOMPARE(mTokenizer[10].position.line, 1);
}

void TestCppTokenizer::test_cross_file_match_indices()
{
    tokenize({"#include /tmp/main.cpp:-1", "#include /tmp/a.h:-1", "void f() {",
              "#include /tmp/main.cpp:1", "}"});
    int leftBrace = tokenTexts().indexOf("{");
    int rightBrace = tokenTexts().indexOf("}");
    QCOMPARE(mTokenizer[leftBrace].fileName, "/tmp/a.h");
    QCOMPARE(mTokenizer[rightBrace].fileName, "/tmp/main.cpp");
    QCOMPARE(mTokenizer[leftBrace].matchIndex, rightBrace);
    QCOMPARE(mTokenizer[rightBrace].matchIndex, leftBrace);
}

void TestCppTokenizer::test_unmatched_include_pair_uses_include_end()
{
    tokenize({"#include /tmp/main.cpp:-1", "#include /tmp/a.h:-1", "void f() {",
              "#include /tmp/main.cpp:1", "int after;"});
    int leftBrace = tokenTexts().indexOf("{");
    int includeEnd = tokenTexts().indexOf("#include /tmp/main.cpp:1");
    QCOMPARE(mTokenizer[leftBrace].matchIndex, includeEnd);
    QCOMPARE(mTokenizer[includeEnd + 2].text, "after");
}

void TestCppTokenizer::test_literal_prefixes()
{
    const QStringList strings{"\"s\"", "L\"s\"", "u\"s\"", "U\"s\"", "u8\"s\"",
                              "R\"(s)\"", "LR\"(s)\"", "uR\"(s)\"",
                              "UR\"(s)\"", "u8R\"(s)\""};
    for (const QString &literal : strings) {
        tokenize({"auto value = " + literal + ";"});
        QCOMPARE(mTokenizer[3].text, literal);
        QCOMPARE(mTokenizer[3].kind, CppTokenizer::TokenKind::StringLiteral);
    }

    const QStringList characters{"'c'", "L'c'", "u'c'", "U'c'", "u8'c'"};
    for (const QString &literal : characters) {
        tokenize({"auto value = " + literal + ";"});
        QCOMPARE(mTokenizer[3].text, literal);
        QCOMPARE(mTokenizer[3].kind, CppTokenizer::TokenKind::CharacterLiteral);
    }
}

void TestCppTokenizer::test_attributes_are_skipped()
{
    tokenize({"[[maybe_unused]] int value;"});
    QCOMPARE(tokenTexts(), QStringList({"int", "value", ";"}));
}

void TestCppTokenizer::test_preprocessor_tokens()
{
    tokenize({"#define VALUE 1", "int value = VALUE;"});
    QCOMPARE(mTokenizer[0].kind, CppTokenizer::TokenKind::Preprocessor);
    QCOMPARE(mTokenizer[0].text, "#define VALUE 1");
    QCOMPARE(tokenTexts().size(), 6);
}

void TestCppTokenizer::test_line_numbers()
{
    tokenize({"int first;", "int second;"});
    QCOMPARE(mTokenizer[0].position.line, 0);
    QCOMPARE(mTokenizer[3].position.line, 1);
}

void TestCppTokenizer::test_character_positions()
{
    tokenize({"int value; auto x = 1;"});
    QCOMPARE(mTokenizer[0].position, (SourcePosition{0, 0}));
    QCOMPARE(mTokenizer[1].position, (SourcePosition{0, 4}));
    QCOMPARE(mTokenizer[3].position, (SourcePosition{0, 11}));
    QCOMPARE(mTokenizer[6].position, (SourcePosition{0, 20}));
}

void TestCppTokenizer::test_cursor_advance_and_checkpoint()
{
    tokenize({"value+1"});
    CppTokenCursor cursor(mTokenizer);
    QVERIFY(cursor.consume("value"));
    QCOMPARE(cursor.currentText(), "+");
    CppTokenCursor::Checkpoint checkpoint = cursor.checkpoint();
    cursor.advance();
    QCOMPARE(cursor.currentText(), "1");
    cursor.restore(checkpoint);
    QCOMPARE(cursor.currentText(), "+");
    QVERIFY(cursor.consume("+"));
    QVERIFY(cursor.consume("1"));
    QVERIFY(cursor.atEnd());
}

void TestCppTokenizer::test_greater_token_split_and_rollback()
{
    tokenize({"std::vector<std::vector<int>> values;"});
    CppTokenCursor cursor(mTokenizer);
    while (cursor.currentText() != ">>")
        cursor.advance();

    CppTokenCursor::Checkpoint checkpoint = cursor.checkpoint();
    QVERIFY(cursor.consumeGreater());
    QCOMPARE(cursor.currentText(), ">");
    QCOMPARE(cursor.greaterConsumed(), 1);
    QVERIFY(cursor.consumeGreater());
    QCOMPARE(cursor.currentText(), "values");

    cursor.restore(checkpoint);
    QCOMPARE(cursor.currentText(), ">>");
    QCOMPARE(cursor.greaterConsumed(), 0);
    QVERIFY(cursor.consume(">"));
    QVERIFY(cursor.consume(">"));
    QCOMPARE(cursor.currentText(), "values");
}

void TestCppTokenizer::test_same_file_match_skip()
{
    tokenize({"int main(){ int value; }"});
    CppTokenCursor cursor(mTokenizer);
    while (cursor.currentText() != "{")
        cursor.advance();
    QCOMPARE(cursor.matchStatus(), CppTokenCursor::MatchStatus::SameFile);
    QCOMPARE(cursor.skipToMatch(), CppTokenCursor::MatchStatus::SameFile);
    QCOMPARE(cursor.currentText(), "}");
}

void TestCppTokenizer::test_unmatched_match_skip_to_file_end()
{
    tokenize({"int main() {"});
    CppTokenCursor cursor(mTokenizer);
    while (cursor.currentText() != "{")
        cursor.advance();
    QCOMPARE(cursor.skipToMatch(), CppTokenCursor::MatchStatus::FileEnd);
    QVERIFY(cursor.atEnd());
}

void TestCppTokenizer::test_file_end_token()
{
    tokenize({"int value;"});
    CppTokenCursor cursor(mTokenizer);
    while (!cursor.atEnd())
        cursor.advance();
    QCOMPARE(cursor.current().kind, CppTokenizer::TokenKind::FileEnd);
    QCOMPARE(cursor.peek().kind, CppTokenizer::TokenKind::FileEnd);
    QVERIFY(cursor.currentText().isEmpty());
}

void TestCppTokenizer::test_include_end_match_skip()
{
    tokenize({"#include /tmp/main.cpp:-1", "#include /tmp/a.h:-1", "void f() {",
              "#include /tmp/main.cpp:1", "int after;"});
    CppTokenCursor cursor(mTokenizer);
    while (cursor.currentText() != "{")
        cursor.advance();
    QCOMPARE(cursor.skipToMatch(), CppTokenCursor::MatchStatus::FileEnd);
    QCOMPARE(cursor.currentText(), "#include /tmp/main.cpp:1");
}

void TestCppTokenizer::test_cross_file_match_skip()
{
    tokenize({"#include /tmp/main.cpp:-1", "#include /tmp/a.h:-1", "void f() {",
              "#include /tmp/main.cpp:1", "}"});
    CppTokenCursor cursor(mTokenizer);
    while (cursor.currentText() != "{")
        cursor.advance();
    QCOMPARE(cursor.skipToMatch(), CppTokenCursor::MatchStatus::CrossFile);
    QCOMPARE(cursor.currentText(), "#include /tmp/main.cpp:1");
}

void TestCppTokenizer::test_invalid_match_skip()
{
    tokenize({"int value;"});
    CppTokenCursor cursor(mTokenizer);
    QCOMPARE(cursor.skipToMatch(), CppTokenCursor::MatchStatus::Unmatched);
    QCOMPARE(cursor.index(), 0);
}

void TestCppTokenizer::test_type_template_arguments()
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

void TestCppTokenizer::test_array_declarator_order()
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

void TestCppTokenizer::test_function_pointer_declarator()
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

void TestCppTokenizer::test_function_returning_function_pointer()
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

void TestCppTokenizer::test_member_function_qualifiers()
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

void TestCppTokenizer::test_parameter_rendering()
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

void TestCppTokenizer::test_decl_specifier_flags()
{
    DeclSpecifierInfo specifiers;
    specifiers.properties.setFlag(DeclSpecifierProperty::Static);
    specifiers.properties.setFlag(DeclSpecifierProperty::Register);
    QVERIFY(specifiers.properties.testFlag(DeclSpecifierProperty::Static));
    QVERIFY(specifiers.properties.testFlag(DeclSpecifierProperty::Register));
    QVERIFY(!specifiers.properties.testFlag(DeclSpecifierProperty::Extern));
}

void TestCppTokenizer::test_parse_simple_declaration()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("static const unsigned long value;", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "value");
    QCOMPARE(declaration.specifiers.type.qualifiedName, "unsigned long");
    QVERIFY(declaration.specifiers.type.qualifiers.testFlag(TypeQualifier::Const));
    QVERIFY(declaration.specifiers.properties.testFlag(DeclSpecifierProperty::Static));
}

void TestCppTokenizer::test_parse_multiple_declarators()
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

void TestCppTokenizer::test_parse_function_pointer_declaration()
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

void TestCppTokenizer::test_parse_complex_parameter_declaration()
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

void TestCppTokenizer::test_parse_trailing_return_declaration()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("auto fpif(int)->int(*)(int);", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "fpif");
    QVector<TypeDerivation> derivations = declaration.declarators[0].derivations;
    QCOMPARE(derivations.count(), 1);
    QCOMPARE(derivations[0].trailingReturnType, "int (*)(int)");
}

void TestCppTokenizer::test_parse_member_function_declarators()
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

void TestCppTokenizer::test_parse_direct_initialization_not_function()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("std::string value(\"text\");", declaration));
    QCOMPARE(declaration.declarators.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "value");
    QVERIFY(declaration.declarators[0].derivations.isEmpty());
    QCOMPARE(declaration.specifiers.type.toString(), "std::string");
}

void TestCppTokenizer::test_parse_template_type_arguments()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("std::vector<std::vector<int>> values;", declaration));
    QCOMPARE(declaration.declarators[0].name, "values");
    QCOMPARE(declaration.specifiers.type.qualifiedName, "std::vector");
    QCOMPARE(declaration.specifiers.type.templateArguments.count(), 1);
    TemplateArgument argument = declaration.specifiers.type.templateArguments[0];
    QCOMPARE(argument.kind, TemplateArgumentKind::Type);
    QVERIFY(argument.type);
    QCOMPARE(argument.type->toString(), "std::vector<int>");
}

void TestCppTokenizer::test_parse_operator_declaration()
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

void TestCppTokenizer::test_parse_member_pointer_declaration()
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

void TestCppTokenizer::test_parse_function_template()
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

void TestCppTokenizer::test_parse_template_parameter_kinds()
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
    QCOMPARE(declaration.templateParameters[1].kind, TemplateParameterKind::TemplateTemplate);
    QCOMPARE(declaration.templateParameters[1].name, "Container");
}

void TestCppTokenizer::test_parse_template_specializations()
{
    DeclarationInfo declaration;
    QVERIFY(parseDeclarationText("template<> int value<int>;", declaration));
    QCOMPARE(declaration.templateKind, TemplateDeclarationKind::ExplicitSpecialization);
    QVERIFY(declaration.templateParameters.isEmpty());
    QCOMPARE(declaration.declarators[0].name, "value<int>");

    QVERIFY(parseDeclarationText("template<typename T> int value<T*>;", declaration));
    QCOMPARE(declaration.templateKind, TemplateDeclarationKind::PartialSpecialization);
    QCOMPARE(declaration.templateParameters.count(), 1);
    QCOMPARE(declaration.declarators[0].name, "value<T *>");
}

void TestCppTokenizer::test_scope_range_queries()
{
    CppScopes scopes;
    auto outer = std::make_shared<Statement>();
    auto first = std::make_shared<Statement>();
    auto second = std::make_shared<Statement>();
    scopes.addScope({{0, 0}, {10, 0}}, outer);
    scopes.addScope({{2, 0}, {4, 0}}, first, 0);
    scopes.addScope({{6, 0}, {8, 0}}, second, 0);

    QCOMPARE(scopes.findScopeAt({3, 0}), first);
    QCOMPARE(scopes.findScopeAt({5, 0}), outer);
    QCOMPARE(scopes.findScopeAt({6, 0}), second);
    QVERIFY(!scopes.findScopeAt({11, 0}));
}

void TestCppTokenizer::test_scope_queries_same_line()
{
    CppScopes scopes;
    auto outer = std::make_shared<Statement>();
    auto first = std::make_shared<Statement>();
    auto second = std::make_shared<Statement>();
    scopes.addScope({{0, 0}, {0, 100}}, outer);
    scopes.addScope({{0, 10}, {0, 20}}, first, 0);
    scopes.addScope({{0, 30}, {0, 40}}, second, 0);

    QCOMPARE(scopes.findScopeAt({0, 15}), first);
    QCOMPARE(scopes.findScopeAt({0, 25}), outer);
    QCOMPARE(scopes.findScopeAt({0, 35}), second);
    QCOMPARE(scopes.findScopeAt({0, 99}), outer);
    QVERIFY(!scopes.findScopeAt({0, 101}));
}

void TestCppTokenizer::test_syntax_class_declarations()
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

void TestCppTokenizer::test_syntax_template_declarations()
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

void TestCppTokenizer::test_syntax_namespace_declarations()
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

void TestCppTokenizer::test_syntax_enum_declarations()
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

void TestCppTokenizer::test_syntax_using_declarations()
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

void TestCppTokenizer::test_declaration_source_ranges()
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
