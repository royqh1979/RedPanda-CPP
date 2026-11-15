#include "test_ppexpander.h"
#include <QTest>

TestPpExpander::TestPpExpander(QObject *parent):
    QObject{parent}
{

}

PDefine TestPpExpander::makeDefine(const QString &name, const QString &args,
                                   const QString &value)
{
    PDefine define = std::make_shared<Define>();
    define->name = name;
    define->args = args;
    define->value = value;
    if (!args.isEmpty()) {
        define->paramNames = splitMacroParams(args.mid(1, args.length()-2));
        define->replacementList = compileReplacementList(value, define->paramNames);
    }
    return define;
}

QString TestPpExpander::expand(const QHash<QString, PDefine> &defines,
                               const QString &line)
{
    PpExpander expander([&defines](const QString& name) {
        return defines.value(name, PDefine());
    });
    QStringList lines{line};
    PpTokenizer tokenizer(lines);
    QString out;
    expander.expandLine(tokenizer, out);
    return out;
}

QString TestPpExpander::expandCondition(const QHash<QString, PDefine> &defines,
                                        const QString &expr)
{
    PpExpander expander([&defines](const QString& name) {
        return defines.value(name, PDefine());
    });
    expander.setConditionMode(true);
    QStringList lines{expr};
    PpTokenizer tokenizer(lines);
    QString out;
    expander.expandLine(tokenizer, out);
    return out;
}

void TestPpExpander::test_object_macro()
{
    QHash<QString, PDefine> defs{{"N", makeDefine("N", "", "12")}};
    QCOMPARE(expand(defs, "x N y"), "x 12 y");
    QCOMPARE(expand(defs, "N N"), "12 12");
    QCOMPARE(expand(defs, "  int\t x  =  N + 1;  "), "  int\t x  =  12 + 1;  ");
    QCOMPARE(expand(defs, "  int\t x  =  1;  "), "  int\t x  =  1;  ");
    QCOMPARE(expand(defs, "auto s = R\"(N)\"; N"), "auto s = R\"(N)\"; 12");
}

void TestPpExpander::test_empty_body_macro()
{
    QHash<QString, PDefine> defs{{"E", makeDefine("E", "", "")}};
    QCOMPARE(expand(defs, "a E b"), "a  b");
    QCOMPARE(expand(defs, "E"), "");
}

void TestPpExpander::test_function_macro()
{
    QHash<QString, PDefine> defs{{"F", makeDefine("F", "(x)", "x+1")}};
    QCOMPARE(expand(defs, "F(5)"), "5+1");
    QCOMPARE(expand(defs, "int a = F(2);"), "int a = 2+1;");
}

void TestPpExpander::test_multiline_invocation()
{
    QHash<QString, PDefine> defs{{"F", makeDefine("F", "(x)", "x+1")}};
    PpExpander expander([&defs](const QString& name) {
        return defs.value(name, PDefine());
    });
    QStringList lines{"int a = F(", "2);", "int b;"};
    PpTokenizer tokenizer(lines);
    QString out;
    int consumed = expander.expandLine(tokenizer, out);
    QCOMPARE(out, "int a = 2+1;");
    QCOMPARE(consumed, 2);
}

void TestPpExpander::test_chain_rescan()
{
    QHash<QString, PDefine> defs{
        {"A", makeDefine("A", "", "B")},
        {"B", makeDefine("B", "(x)", "x+1")}};
    QCOMPARE(expand(defs, "A(5)"), "5+1");
}

void TestPpExpander::test_mutual_recursion()
{
    // blue paint terminates the rescan: F is painted while its own
    // expansion is rescanned
    QHash<QString, PDefine> defs{
        {"F", makeDefine("F", "(x)", "G(x)")},
        {"G", makeDefine("G", "(x)", "F(x)")}};
    QCOMPARE(expand(defs, "F(1)"), "F(1)");
}

void TestPpExpander::test_object_cycle()
{
    QHash<QString, PDefine> defs{
        {"N", makeDefine("N", "", "TEST")},
        {"TEST", makeDefine("TEST", "", "N")}};
    QCOMPARE(expand(defs, "ttt TEST TEST"), "ttt TEST TEST");
    QCOMPARE(expand(defs, "ttt N N"), "ttt N N");
}

void TestPpExpander::test_stringize()
{
    QHash<QString, PDefine> defs{{"S", makeDefine("S", "(x)", "#x")}};
    QCOMPARE(expand(defs, "S(abc)"), "\"abc\"");
    QCOMPARE(expand(defs, "S(a  b)"), "\"a  b\"");
}

void TestPpExpander::test_stringize_escaping()
{
    QHash<QString, PDefine> defs{{"S", makeDefine("S", "(x)", "#x")}};
    QCOMPARE(expand(defs, "S(a\\b)"), "\"a\\\\b\"");
    QCOMPARE(expand(defs, "S(\"q\")"), "\"\\\"q\\\"\"");
}

void TestPpExpander::test_paste()
{
    QHash<QString, PDefine> defs{
        {"P", makeDefine("P", "(a,b)", "a##b")},
        {"F", makeDefine("F", "(name,a)", "int fun_##name() { return a; }")}};
    QCOMPARE(expand(defs, "P(x,y)"), "xy");
    QCOMPARE(expand(defs, "F(abcd, 12)"), "int fun_abcd() { return 12; }");
}

void TestPpExpander::test_paste_chain()
{
    QHash<QString, PDefine> defs{
        {"C", makeDefine("C", "(a,b,c)", "a##b##c")}};
    QCOMPARE(expand(defs, "C(x,y,z)"), "xyz");
}

void TestPpExpander::test_varargs()
{
    QHash<QString, PDefine> defs{
        {"M", makeDefine("M", "(...)", "f(__VA_ARGS__)")},
        {"L", makeDefine("L", "(fmt, ...)", "printf(fmt, __VA_ARGS__)")}};
    QCOMPARE(expand(defs, "M(1,2,3)"), "f(1,2,3)");
    QCOMPARE(expand(defs, "L(\"%d\", 1)"), "printf(\"%d\", 1)");
}

void TestPpExpander::test_gnu_comma_paste_idiom()
{
    // the GNU ", ## __VA_ARGS__" idiom drops the comma when the
    // variadic arguments are empty
    QHash<QString, PDefine> defs{
        {"LOG", makeDefine("LOG", "(fmt, ...)", "printf(fmt, ##__VA_ARGS__)")}};
    QCOMPARE(expand(defs, "LOG(\"x\")"), "printf(\"x\")");
    QCOMPARE(expand(defs, "LOG(\"x\", 1, 2)"), "printf(\"x\", 1, 2)");
}

void TestPpExpander::test_va_opt()
{
    QHash<QString, PDefine> defs{
        {"M", makeDefine("M", "(fmt, ...)",
                         "printf(fmt __VA_OPT__(, __VA_ARGS__))")}};
    QCOMPARE(expand(defs, "M(\"x\")"), "printf(\"x\")");
    QCOMPARE(expand(defs, "M(\"x\", 7)"), "printf(\"x\", 7)");
}

void TestPpExpander::test_count_mismatch()
{
    QHash<QString, PDefine> defs{{"F", makeDefine("F", "(a,b)", "a+b")}};
    QCOMPARE(expand(defs, "F(1)"), "F(1)");
    QCOMPARE(expand(defs, "F(1,2,3)"), "F(1,2,3)");
    QCOMPARE(expand(defs, "F(1,2)"), "1+2");
}

void TestPpExpander::test_zero_param_macro()
{
    QHash<QString, PDefine> defs{{"Z", makeDefine("Z", "()", "42")}};
    QCOMPARE(expand(defs, "Z()"), "42");
    QCOMPARE(expand(defs, "Z(1)"), "Z(1)");
}

void TestPpExpander::test_unterminated_invocation()
{
    // ill-formed input: the call is left unexpanded
    QHash<QString, PDefine> defs{{"F", makeDefine("F", "(x)", "x")}};
    QCOMPARE(expand(defs, "F("), "F(");
}

void TestPpExpander::test_not_invoked()
{
    // a function-like macro name without '(' is not expanded
    QHash<QString, PDefine> defs{{"F", makeDefine("F", "(x)", "x+1")}};
    QCOMPARE(expand(defs, "F"), "F");
    QCOMPARE(expand(defs, "F + 1"), "F + 1");
}

void TestPpExpander::test_arg_pre_expansion()
{
    QHash<QString, PDefine> defs{
        {"F", makeDefine("F", "(x)", "x+1")},
        {"G", makeDefine("G", "(x)", "x*2")}};
    QCOMPARE(expand(defs, "F(G(2))"), "2*2+1");
}

void TestPpExpander::test_word_boundaries()
{
    QHash<QString, PDefine> defs{{"N", makeDefine("N", "", "12")}};
    QCOMPARE(expand(defs, "NxN"), "NxN");
    QCOMPARE(expand(defs, "N12"), "N12");
}

void TestPpExpander::test_empty_args()
{
    QHash<QString, PDefine> defs{{"F", makeDefine("F", "(a,b)", "a+b")}};
    QCOMPARE(expand(defs, "F(,)"), "+");
}

void TestPpExpander::test_condition_mode()
{
    QHash<QString, PDefine> defs{
        {"X", makeDefine("X", "", "1")},
        {"MIN", makeDefine("MIN", "(a,b)", "((a)<(b)?(a):(b))")}};
    QCOMPARE(expandCondition(defs, "defined(X)"), "1");
    QCOMPARE(expandCondition(defs, "defined(Y)"), "0");
    QCOMPARE(expandCondition(defs, "defined X"), "1");
    QCOMPARE(expandCondition(defs, "UNDEFINED"), "0");
    QCOMPARE(expandCondition(defs, "UNDEFINED == 0"), "0 == 0");
    QCOMPARE(expandCondition(defs, "true"), "1");
    QCOMPARE(expandCondition(defs, "X && MIN(1,2)"), "1 && ((1)<(2)?(1):(2))");
    QCOMPARE(expandCondition(defs, "1 and 1"), "1 && 1");
}

