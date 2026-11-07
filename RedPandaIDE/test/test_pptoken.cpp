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
