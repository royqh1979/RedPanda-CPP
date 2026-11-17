#include "test_pptoken.h"
#include <QTest>

TestPpTokenizer::TestPpTokenizer(QObject *parent):
    QObject{parent}
{

}

QString TestPpTokenizer::dumpToken(const PpToken &token)
{
    QString kindName;
    switch (token.kind) {
    case PpTokenKind::Identifier: kindName = "id"; break;
    case PpTokenKind::Number: kindName = "num"; break;
    case PpTokenKind::StringLiteral: kindName = "str"; break;
    case PpTokenKind::CharLiteral: kindName = "chr"; break;
    case PpTokenKind::RawStringFragment: kindName = "raw"; break;
    case PpTokenKind::Punct: kindName = "pun"; break;
    case PpTokenKind::EndOfLine: kindName = "eol"; break;
    case PpTokenKind::EndOfInput: kindName = "eof"; break;
    }
    return QString("%1%2:%3")
            .arg(kindName)
            .arg(token.whitespaceBefore.isEmpty()?"":"+")
            .arg(token.text);
}

QStringList TestPpTokenizer::dumpTokens(const QStringList &lines)
{
    PpTokenizer tokenizer(lines);
    QStringList result;
    while (true) {
        PpToken token = tokenizer.next();
        result.append(dumpToken(token));
        if (token.kind == PpTokenKind::EndOfInput)
            break;
    }
    return result;
}

void TestPpTokenizer::test_words_and_punct()
{
    QCOMPARE(dumpTokens({"int x = a+b;"}),
             QStringList({
                 "id:int", "id+:x", "pun+:=", "id+:a",
                 "pun:+", "id:b", "pun:;", "eol:", "eof:"}));
}

void TestPpTokenizer::test_space_before_flags()
{
    QCOMPARE(dumpTokens({"a  b\tc"}),
             QStringList({"id:a", "id+:b", "id+:c", "eol:", "eof:"}));
}

void TestPpTokenizer::test_numbers()
{
    QCOMPARE(dumpTokens({"0x10UL 3.14e-10 1.5f 010"}),
             QStringList({
                 "num:0x10UL", "num+:3.14e-10", "num+:1.5f",
                 "num+:010", "eol:", "eof:"}));
}

void TestPpTokenizer::test_strings()
{
    QCOMPARE(dumpTokens({"\"a\\\"b\" \"x\""}),
             QStringList({"str:\"a\\\"b\"", "str+:\"x\"", "eol:", "eof:"}));
}

void TestPpTokenizer::test_char_literals()
{
    QCOMPARE(dumpTokens({"'\\'' 'ab' '\\n'"}),
             QStringList({
                 "chr:'\\''", "chr+:'ab'", "chr+:'\\n'", "eol:", "eof:"}));
}

void TestPpTokenizer::test_unterminated_string()
{
    // an unterminated string runs to the end of the line
    QCOMPARE(dumpTokens({"\"abc", "int x;"}),
             QStringList({
                 "str:\"abc", "eol:",
                 "id:int", "id+:x", "pun:;", "eol:", "eof:"}));
}

void TestPpTokenizer::test_sharp_tokens()
{
    QCOMPARE(dumpTokens({"# ##a#b"}),
             QStringList({
                 "pun:#", "pun+:##", "id:a", "pun:#", "id:b",
                 "eol:", "eof:"}));
}

void TestPpTokenizer::test_raw_string_single_line()
{
    QCOMPARE(dumpTokens({"const char* s = R\"(x)\";"}),
             QStringList({
                 "id:const", "id+:char", "pun:*", "id+:s", "pun+:=",
                 "raw+:R\"(", "raw:x)\"", "pun:;",
                 "eol:", "eof:"}));
}

void TestPpTokenizer::test_raw_string_delimiter()
{
    // ')' alone does not close a delimited raw string
    QCOMPARE(dumpTokens({"R\"delim(a ) still)delim\";"}),
             QStringList({
                 "raw:R\"delim(", "raw:a ) still)delim\"", "pun:;",
                 "eol:", "eof:"}));
}

void TestPpTokenizer::test_raw_string_multiline()
{
    // raw content spans lines; each physical line is a fragment and
    // line boundaries are still marked
    QCOMPARE(dumpTokens({"const char* s = R\"(multi", "line)\";"}),
             QStringList({
                 "id:const", "id+:char", "pun:*", "id+:s", "pun+:=",
                 "raw+:R\"(", "raw:multi", "eol:",
                 "raw:line)\"", "pun:;",
                 "eol:", "eof:"}));
}

void TestPpTokenizer::test_raw_string_ill_formed()
{
    // R followed by a quote but no '(' is just an identifier + string
    QCOMPARE(dumpTokens({"R\"abc\""}),
             QStringList({
                 "id:R", "str:\"abc\"", "eol:", "eof:"}));
    // an R inside an identifier does not start a raw string
    QCOMPARE(dumpTokens({"COLORR\"(y)\""}),
             QStringList({
                 "id:COLORR", "str:\"(y)\"", "eol:", "eof:"}));
}

void TestPpTokenizer::test_end_of_line_markers()
{
    QCOMPARE(dumpTokens({"a", "", "b"}),
             QStringList({
                 "id:a", "eol:", "eol:", "id:b", "eol:", "eof:"}));
}

void TestPpTokenizer::test_empty_and_whitespace_only_lines()
{
    QCOMPARE(dumpTokens({}), QStringList({"eof:"}));
    QCOMPARE(dumpTokens({""}), QStringList({"eol:", "eof:"}));
    QStringList lines{" \t "};
    PpTokenizer tokenizer(lines);
    PpToken eol = tokenizer.next();
    QCOMPARE(eol.kind, PpTokenKind::EndOfLine);
    QCOMPARE(eol.whitespaceBefore, " \t ");
    QCOMPARE(tokenizer.next().kind, PpTokenKind::EndOfInput);
}

void TestPpTokenizer::test_exact_whitespace()
{
    QStringList lines{" \t a  b \t"};
    PpTokenizer tokenizer(lines);
    PpToken a = tokenizer.next();
    QCOMPARE(a.kind, PpTokenKind::Identifier);
    QCOMPARE(a.text, "a");
    QCOMPARE(a.whitespaceBefore, " \t ");
    PpToken b = tokenizer.next();
    QCOMPARE(b.text, "b");
    QCOMPARE(b.whitespaceBefore, "  ");
    PpToken eol = tokenizer.next();
    QCOMPARE(eol.kind, PpTokenKind::EndOfLine);
    QCOMPARE(eol.whitespaceBefore, " \t");
}

void TestPpTokenizer::test_number_exponents_and_separators()
{
    QCOMPARE(dumpTokens({"1e+2 0x1p-2 1+2 1'e+3"}),
             QStringList({
                 "num:1e+2", "num+:0x1p-2", "num+:1", "pun:+", "num:2",
                 "num+:1'e+3", "eol:", "eof:"}));
}

void TestPpTokenizer::test_identifiers_and_punctuation()
{
    QCOMPARE(dumpTokens({"_x x1 中文 + - * / % ^ & | ~ ! = < > [ ] ( ) { } , ; : . ?"}),
             QStringList({
                 "id:_x", "id+:x1", "id+:中文",
                 "pun+:+", "pun+:-", "pun+:*", "pun+:/", "pun+:%", "pun+:^",
                 "pun+:&", "pun+:|", "pun+:~", "pun+:!", "pun+:=", "pun+:<",
                 "pun+:>", "pun+:[", "pun+:]", "pun+:(", "pun+:)", "pun+:{",
                 "pun+:}", "pun+:,", "pun+:;", "pun+::", "pun+:.", "pun+:?",
                 "eol:", "eof:"}));
}

void TestPpTokenizer::test_multi_character_operators()
{
    QCOMPARE(dumpTokens({"a==b && c!=d <= 2 >= 1 << 3 >> 1"}),
             QStringList({"id:a", "pun:==", "id:b", "pun+:&&", "id+:c",
                          "pun:!=", "id:d", "pun+:<=", "num+:2", "pun+:>=",
                          "num+:1", "pun+:<<", "num+:3", "pun+:>>", "num+:1",
                          "eol:", "eof:"}));
}

void TestPpTokenizer::test_unterminated_escape()
{
    QCOMPARE(dumpTokens({"\"abc\\", "'x\\"}),
             QStringList({"str:\"abc\\", "eol:", "chr:'x\\", "eol:", "eof:"}));
}

void TestPpTokenizer::test_raw_string_prefixes_and_delimiter_validation()
{
    QCOMPARE(dumpTokens({"LR\"(x)\" uR\"(y)\" UR\"(z)\" u8R\"(w)\""}),
             QStringList({
                 "raw:LR\"(", "raw:x)\"", "raw+:uR\"(", "raw:y)\"",
                 "raw+:UR\"(", "raw:z)\"", "raw+:u8R\"(", "raw:w)\"",
                 "eol:", "eof:"}));
    QCOMPARE(dumpTokens({"R\"abcdefghijklmnop(x)abcdefghijklmnop\""}),
             QStringList({"raw:R\"abcdefghijklmnop(", "raw:x)abcdefghijklmnop\"",
                          "eol:", "eof:"}));
    QCOMPARE(dumpTokens({"R\"abcdefghijklmnopq(x)abcdefghijklmnopq\""}),
             QStringList({"id:R", "str:\"abcdefghijklmnopq(x)abcdefghijklmnopq\"",
                          "eol:", "eof:"}));
    QCOMPARE(dumpTokens({"R\"a b(x)a b\""}),
             QStringList({"id:R", "str:\"a b(x)a b\"", "eol:", "eof:"}));
}

void TestPpTokenizer::test_raw_string_empty_and_unterminated()
{
    QCOMPARE(dumpTokens({"R\"()\""}),
             QStringList({"raw:R\"(", "raw:)\"", "eol:", "eof:"}));
    QCOMPARE(dumpTokens({"R\"(a", "", "b)\""}),
             QStringList({"raw:R\"(", "raw:a", "eol:", "eol:",
                          "raw:b)\"", "eol:", "eof:"}));
    QCOMPARE(dumpTokens({"R\"(abc", "def"}),
             QStringList({"raw:R\"(", "raw:abc", "eol:", "raw:def", "eol:", "eof:"}));
}

void TestPpTokenizer::test_start_position_and_line_text()
{
    QStringList lines{"skip", "  int x"};
    PpTokenizer tokenizer(lines, 1, 2);
    QCOMPARE(tokenizer.line(), 1);
    QCOMPARE(tokenizer.lineText(0), "skip");
    QCOMPARE(tokenizer.lineText(1), "  int x");
    QCOMPARE(tokenizer.lineText(-1), QString());
    QCOMPARE(tokenizer.lineText(2), QString());
    PpToken first = tokenizer.next();
    QCOMPARE(first.kind, PpTokenKind::Identifier);
    QCOMPARE(first.text, "int");
    QCOMPARE(tokenizer.next().text, "x");
    QCOMPARE(tokenizer.next().kind, PpTokenKind::EndOfLine);
    QCOMPARE(tokenizer.line(), 2);
    QCOMPARE(tokenizer.next().kind, PpTokenKind::EndOfInput);
}

void TestPpTokenizer::test_prefix_before()
{
    QStringList lines{" \t int  value = alpha+1;"};
    PpTokenizer tokenizer(lines);
    QCOMPARE(tokenizer.next().text, "int");
    QCOMPARE(tokenizer.next().text, "value");
    QCOMPARE(tokenizer.next().text, "=");
    PpToken alpha = tokenizer.next();
    QCOMPARE(alpha.text, "alpha");
    QCOMPARE(tokenizer.prefixBefore(alpha), " \t int  value =");
    PpToken plus = tokenizer.next();
    QCOMPARE(tokenizer.prefixBefore(plus), " \t int  value = alpha");
    QStringList leadingLines{"  x"};
    PpTokenizer leadingWhitespace(leadingLines);
    PpToken x = leadingWhitespace.next();
    QCOMPARE(leadingWhitespace.prefixBefore(x), QString());
}

void TestPpTokenizer::test_end_of_input_is_stable_and_predicates()
{
    QStringList lines{"a"};
    PpTokenizer tokenizer(lines);
    PpToken word = tokenizer.next();
    QVERIFY(word.isWordLike());
    PpToken eol = tokenizer.next();
    QVERIFY(!eol.isWordLike());
    QCOMPARE(tokenizer.next().kind, PpTokenKind::EndOfInput);
    QCOMPARE(tokenizer.next().kind, PpTokenKind::EndOfInput);
    QCOMPARE(tokenizer.line(), 1);
    QVERIFY(PpTokenizer::isIdentStartChar('_'));
    QVERIFY(PpTokenizer::isIdentStartChar('a'));
    QVERIFY(!PpTokenizer::isIdentStartChar('1'));
    QVERIFY(PpTokenizer::isWordChar('1'));
    QVERIFY(!PpTokenizer::isWordChar('+'));
}

QStringList TestPpTokenizer::dumpRepl(const ReplTokenList &entries)
{
    QStringList result;
    for (const ReplToken& entry : entries) {
        switch (entry.kind) {
        case ReplToken::Kind::VarOptBegin:
            result.append("voB");
            continue;
        case ReplToken::Kind::VarOptEnd:
            result.append("voE");
            continue;
        default:
            break;
        }
        if (entry.kind == ReplToken::Kind::Text) {
            QString kindName;
            switch (entry.token.kind) {
            case PpTokenKind::Identifier: kindName = "id"; break;
            case PpTokenKind::Number: kindName = "num"; break;
            case PpTokenKind::StringLiteral: kindName = "str"; break;
            case PpTokenKind::CharLiteral: kindName = "chr"; break;
            case PpTokenKind::RawStringFragment: kindName = "raw"; break;
            default: kindName = "pun"; break;
            }
            result.append(QString("%1%2%3:%4")
                          .arg(kindName)
                          .arg(entry.pasteBefore?"##":"")
                          .arg(entry.token.whitespaceBefore.isEmpty()?"":"+")
                          .arg(entry.token.text));
        } else {
            result.append(QString("arg%1%2%3:%4")
                          .arg(entry.stringize?"#":"")
                          .arg(entry.pasteBefore?"##":"")
                          .arg(entry.whitespaceBefore.isEmpty()?"":"+")
                          .arg(entry.argIndex));
        }
    }
    return result;
}

void TestPpTokenizer::test_split_macro_params()
{
    QCOMPARE(splitMacroParams(""), QStringList());
    QCOMPARE(splitMacroParams("a"), QStringList({"a"}));
    QCOMPARE(splitMacroParams("a,b"), QStringList({"a", "b"}));
    QCOMPARE(splitMacroParams(" a , b "), QStringList({"a", "b"}));
    QCOMPARE(splitMacroParams("..."), QStringList({"..."}));
    QCOMPARE(splitMacroParams("fmt, ..."), QStringList({"fmt", "..."}));
    QCOMPARE(splitMacroParams(","), QStringList({"", ""}));
    QCOMPARE(splitMacroParams("a, void(*b)(int)"),
             QStringList({"a", "void(*b)(int)"}));
}

void TestPpTokenizer::test_compile_replacement_list()
{
    // simple substitution
    QCOMPARE(dumpRepl(compileReplacementList("x+1", {"x"})),
             QStringList({"arg:0", "pun:+", "num:1"}));
    // paste between two arguments
    QCOMPARE(dumpRepl(compileReplacementList("a ## b", {"a", "b"})),
             QStringList({"arg:0", "arg##:1"}));
    // paste chains
    QCOMPARE(dumpRepl(compileReplacementList("a##b ## c", {"a", "b", "c"})),
             QStringList({"arg:0", "arg##:1", "arg##:2"}));
    // paste between an argument and text, with per-token entries so
    // identifiers stay re-expandable during rescan
    QCOMPARE(dumpRepl(compileReplacementList(
                          "int fun_##name() { return a; }",
                          {"name", "a"})),
             QStringList({
                 "id:int", "id+:fun_", "arg##:0",
                 "pun:(", "pun:)", "pun+:{", "id+:return",
                 "arg+:1", "pun:;", "pun+:}"}));
    // stringize
    QCOMPARE(dumpRepl(compileReplacementList("#x", {"x"})),
             QStringList({"arg#:0"}));
    // stringize keeps the space before '#'
    QCOMPARE(dumpRepl(compileReplacementList("WORD #__VA_ARGS__", {"..."})),
             QStringList({"id:WORD", "arg#+:0"}));
    // a literal '%' in the body is preserved as-is
    QCOMPARE(dumpRepl(compileReplacementList("100% done", {})),
             QStringList({"num:100", "pun:%", "id+:done"}));
    // more than 9 parameters work
    QCOMPARE(dumpRepl(compileReplacementList(
                          "p10", {"p0","p1","p2","p3","p4","p5",
                                  "p6","p7","p8","p9","p10"})),
             QStringList({"arg:10"}));
    // leading paste
    QCOMPARE(dumpRepl(compileReplacementList("##a", {"a"})),
             QStringList({"arg##:0"}));
    // __VA_OPT__ markers
    QCOMPARE(dumpRepl(compileReplacementList("f() __VA_OPT__(, )", {"..."})),
             QStringList({"id:f", "pun:(", "pun:)", "voB", "pun:,", "voE"}));
    // __VA_OPT__ with nested parens in content
    QCOMPARE(dumpRepl(compileReplacementList(
                          "__VA_OPT__(call(a, b))", {"..."})),
             QStringList({"voB", "id:call", "pun:(", "id:a", "pun:,",
                          "id+:b", "pun:)", "voE"}));
}
