#include <QTest>
#include <QFile>
#include "test_cpptokenizer.h"
#include "src/parser/cpppreprocessor.h"

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

bool TestCppTokenizer::parseDeclarationText(const QString &text, DeclarationInfo &result,
                                            int cppVersion)
{
    CppTokenizer tokenizer;
    tokenizer.tokenize(text.split('\n'));
    CppTokenCursor cursor(tokenizer);
    CppDeclarationParser parser(cursor);
    parser.setCppVersion(cppVersion);
    return parser.parseDeclaration(result);
}

bool TestCppTokenizer::parseSyntaxText(const QString &text, QVector<DeclarationInfo> &result,
                                       int cppVersion)
{
    CppTokenizer tokenizer;
    tokenizer.tokenize(text.split('\n'));
    CppTokenCursor cursor(tokenizer);
    CppSyntaxParser parser(cursor);
    parser.setCppVersion(cppVersion);
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
    QStringList lines = text.split('\n');
    CppPreprocessor::replaceCommentsBySpaceChar(lines);
    return parseSyntaxText(lines.join('\n'), result);
}

void TestCppTokenizer::test_identifiers_and_operators()
{
    tokenize({"value>>=1;"});
    QCOMPARE(tokenTexts(), QStringList({"value", ">>=", "1", ";"}));
    QCOMPARE(mTokenizer[1].kind, CppTokenizer::TokenKind::Operator);

    const QStringList operators{
        "<<=", ">>=", "<=>", "...", "->*",
        "::", "++", "--", "+=", "-=", "*=", "/=", "%=", "==", "!=",
        "<=", ">=", "&&", "||", "&=", "|=", "^=", "<<", ">>", "->", ".*"
    };
    tokenize({operators.join(' ')});
    QCOMPARE(tokenTexts(), operators);
    for (int i = 0; i < operators.count(); ++i)
        QCOMPARE(mTokenizer[i].kind, CppTokenizer::TokenKind::Operator);

    tokenize({"<== >>=="});
    QCOMPARE(tokenTexts(), QStringList({"<=", "=", ">>=", "="}));
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
    for (int i = 0; i < mTokenizer.tokenCount(); ++i)
        QCOMPARE(CppTokenizer::numberTokenKind(mTokenizer[i].text), mTokenizer[i].kind);
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
    QCOMPARE(mTokenizer[0].fileName, "/tmp/main.cpp");
    QCOMPARE(mTokenizer[1].fileName, "/tmp/main.cpp");
    QCOMPARE(mTokenizer[5].fileName, "/tmp/a.h");
    QCOMPARE(mTokenizer[9].fileName, "/tmp/main.cpp");
    QCOMPARE(mTokenizer[10].position.line, 2);
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
    tokenize({"inline namespace __cxx11 __attribute__((__abi_tag__(\"cxx11\"))) { }"});
    QCOMPARE(tokenTexts(), QStringList({"inline", "namespace", "__cxx11", "{", "}"}));
    tokenize({"__attribute ((__abi_tag__(\"cxx11\"))) iterator erase(iterator position);"});
    QCOMPARE(tokenTexts(), QStringList({"iterator", "erase", "(", "iterator", "position", ")", ";"}));
    tokenize({"alignas(16) int value;"});
    QCOMPARE(tokenTexts(), QStringList({"int", "value", ";"}));
    tokenize({"__extension__ typedef int value;"});
    QCOMPARE(tokenTexts(), QStringList({"typedef", "int", "value", ";"}));
    tokenize({"_Pragma(\"GCC diagnostic push\") int value;"});
    QCOMPARE(tokenTexts(), QStringList({"int", "value", ";"}));
}

void TestCppTokenizer::test_preprocessor_tokens()
{
    tokenize({"#define VALUE 1", "int value = VALUE;"});
    QCOMPARE(mTokenizer[0].kind, CppTokenizer::TokenKind::Preprocessor);
    QCOMPARE(mTokenizer[0].text, "#define VALUE 1");
    QCOMPARE(tokenTexts().size(), 6);
}

void TestCppTokenizer::test_preprocessor_cursor_stays_valid_after_continued_lines()
{
    tokenize({"#define FIRST 1\\", "#define SECOND 2", "#define THIRD 3", "int value;"});
    QCOMPARE(mTokenizer[0].kind, CppTokenizer::TokenKind::Preprocessor);
    QCOMPARE(mTokenizer[0].text, "#define FIRST 1\\\n#define SECOND 2");
    QCOMPARE(mTokenizer[1].kind, CppTokenizer::TokenKind::Preprocessor);
    QCOMPARE(mTokenizer[1].text, "#define THIRD 3");
    QCOMPARE(mTokenizer[2].text, "int");
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

void TestCppTokenizer::test_parse_template_type_arguments()
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
    QVERIFY(declaration.templateParameters[0].type);
    QCOMPARE(declaration.templateParameters[0].type->qualifiedName, "int");
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

void TestCppTokenizer::test_statement_factory_initializes_statements()
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

void TestCppTokenizer::test_scope_range_queries()
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

void TestCppTokenizer::test_scope_queries_same_line()
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

void TestCppTokenizer::test_class_base_list_nested_commas()
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


namespace {
PStatement findStatement(const StatementMap &statements, const QString &name)
{
    const auto values = statements.values(name);
    for (const std::weak_ptr<AbstractStatement> &statement : values) {
        if (PStatement result = statement.lock())
            return result;
    }
    return nullptr;
}
}

void TestCppTokenizer::test_statement_builder_expands_declarators_and_types()
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

void TestCppTokenizer::test_statement_builder_preserves_function_and_access_properties()
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

void TestCppTokenizer::test_statement_builder_preserves_scopes_positions_and_ancestors()
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

void TestCppTokenizer::test_statement_builder_assigns_out_of_line_special_members_to_class()
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


void TestCppTokenizer::test_statement_builder_merges_forward_class_with_definition()
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


void TestCppTokenizer::test_statement_model_owns_statements_and_weak_indexes()
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


void TestCppTokenizer::test_statement_builder_preserves_array_suffixes()
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


void TestCppTokenizer::test_statement_builder_registers_nested_body_scopes()
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

void TestCppTokenizer::test_statement_builder_scope_uses_recovered_file_end()
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

void TestCppTokenizer::test_statement_builder_distinguishes_class_body_from_variable()
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


void TestCppTokenizer::test_type_resolver_binds_custom_template_members()
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


void TestCppTokenizer::test_type_resolver_applies_default_template_arguments()
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


void TestCppTokenizer::test_type_resolver_resolves_multisegment_aliases()
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


void TestCppTokenizer::test_type_resolver_binds_dependent_base_members()
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


void TestCppTokenizer::test_type_resolver_namespace_defaults_do_not_capture_caller_bindings()
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


void TestCppTokenizer::test_member_lookup_cache_ignores_model_changes()
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


void TestCppTokenizer::test_type_resolver_reference_collapsing_and_cycles()
{
    QVector<DeclarationInfo> declarations;
    QVERIFY(parseSyntaxText(
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
    DeclarationInfo dependent;
    QVERIFY(parseDeclarationText("std::move_iterator<_InputIterator> value;", dependent));
    TemplateBindings bindings;
    bindings.insert("_InputIterator", TemplateArgument::fromType(dependent.specifiers.type));
    QVERIFY(resolver.resolve(dependent.specifiers.type, bindings).qualifiedName.isEmpty());
}


void TestCppTokenizer::test_type_resolver_selects_specializations_and_rebind()
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


void TestCppTokenizer::test_type_resolver_selects_specializations_across_reopened_namespaces()
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


void TestCppTokenizer::test_type_resolver_computes_bounded_traits()
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


void TestCppTokenizer::test_function_template_explicit_arguments()
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
    model.setCppVersion(CppVersion::Cpp20);
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

    model.setCppVersion(CppVersion::Cpp17);
    QVERIFY(resolver.invokeFunction(makeShared, {},
            {TemplateArgument::fromType(stringVariable.specifiers.type)}).qualifiedName.isEmpty());
}


void TestCppTokenizer::test_type_operations_resolve_container_and_iterator_chain()
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


void TestCppTokenizer::test_expression_evaluator_follows_container_member_chain()
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


void TestCppTokenizer::test_function_scopes_auto_loops_catch_and_using()
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


void TestCppTokenizer::test_type_operations_resolve_standard_range_views()
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


void TestCppTokenizer::test_expression_evaluator_builds_range_adaptor_pipelines()
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


void TestCppTokenizer::test_lambda_inference_preserves_source_position()
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


void TestCppTokenizer::test_function_template_argument_deduction_for_algorithms()
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


void TestCppTokenizer::test_normalized_template_function_signatures()
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


void TestCppTokenizer::test_inline_namespace_member_lookup()
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


void TestCppTokenizer::test_nested_namespace_declaration()
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


void TestCppTokenizer::test_reopened_namespace_lookup_checks_all_declarations()
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


void TestCppTokenizer::test_reopened_nested_namespace_lookup_checks_all_branches()
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


void TestCppTokenizer::test_namespace_lookup_transparency_and_scope_boundaries()
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


void TestCppTokenizer::test_qualified_lookup_respects_lexical_and_local_scopes()
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

void TestCppTokenizer::test_namespace_delete_does_not_reinsert_transparent_descendants()
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


void TestCppTokenizer::test_template_forward_declaration_with_default_arguments()
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


void TestCppTokenizer::test_scope_recovery_preserves_following_declarations()
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


void TestCppTokenizer::test_failed_declaration_restores_cursor()
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


void TestCppTokenizer::test_non_declaration_keywords_are_rejected()
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

void TestCppTokenizer::test_requires_clauses_are_consumed()
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

void TestCppTokenizer::test_requires_before_member_declaration()
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

void TestCppTokenizer::test_function_try_block_recovery()
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

void TestCppTokenizer::test_explicit_bool_declarations()
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

void TestCppTokenizer::test_constructor_declarations_with_specifiers()
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


void TestCppTokenizer::test_integer_constant_expression_invalid_division()
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


void TestCppTokenizer::test_deduction_guides_are_consumed_without_statements()
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


void TestCppTokenizer::test_inherited_member_enumeration_and_access()
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


void TestCppTokenizer::test_expression_evaluator_preserves_constructor_statement()
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


void TestCppTokenizer::test_expression_evaluator_forbids_model_changes_outside_parse()
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
