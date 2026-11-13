#include <QTest>
#include "test_cpptokenizer.h"

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
    QCOMPARE(mTokenizer[6].line, 2);
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
    QCOMPARE(mTokenizer[10].line, 1);
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
    QCOMPARE(mTokenizer[0].line, 0);
    QCOMPARE(mTokenizer[3].line, 1);
}
