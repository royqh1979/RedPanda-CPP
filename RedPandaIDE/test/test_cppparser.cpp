#include <QTest>
#include <QTemporaryDir>
#ifdef REDPANDA_PARSER_STDLIB_TEST
#include <QElapsedTimer>
#include "test_parser_utils.h"
#include "src/parser/cppexpressionevaluator.h"
#endif
#include "src/parser/cppparser.h"
#include "src/parser/cpptyperesolver.h"
#include "test_cppparser.h"

namespace {
void logEvalProfileCsv(const QString &suite, const QString &name, qint64 elapsedNs,
                       const EvalResult &result)
{
    QString type = result.type.toString();
    type.replace("\"", "\"\"");
    qWarning().noquote() << QString("%1,%2,%3,%4,\"%5\"")
            .arg(suite, name).arg(elapsedNs).arg(result.isValid() ? 1 : 0).arg(type);
}

bool writeLines(const QString &path, const QStringList &lines)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(lines.join('\n').toUtf8());
    file.close();
    return true;
}

QString parseSyntaxResource(const PCppParser &parser, const QString &fileName)
{
    QString path = QFileInfo("resources/syntax/" + fileName).absoluteFilePath();
    parser->setOnGetFileStream(nullptr);
    CppParser::parseFileBlocking(parser, path, false, "");
    return path;
}

#ifdef REDPANDA_PARSER_STDLIB_TEST
void collectStatementNames(const PCppParser &parser, const PStatement &parent,
                           QSet<QString> &names)
{
    const StatementMap &children = parser->statementList().childrenStatements(parent);
    for (const std::weak_ptr<AbstractStatement> &weak : children) {
        PStatement statement = weak.lock();
        if (!statement)
            continue;
        if (!statement->fullName.isEmpty())
            names.insert(statement->fullName);
        if (statement->kind != StatementKind::InlineAlias)
            collectStatementNames(parser, statement, names);
    }
}

QSet<QString> standardLibraryStatementNames(const PCppParser &parser)
{
    QSet<QString> result;
    PStatementList namespaces = parser->findNamespace("std");
    if (!namespaces)
        return result;
    for (const PStatement &nameSpace : *namespaces)
        collectStatementNames(parser, nameSpace, result);
    return result;
}

int includeGuardDefineLine(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;
    const QStringList lines = QString::fromUtf8(file.readAll()).split('\n');
    for (int i = 0; i < lines.count(); ++i) {
        const QString directive = lines[i].trimmed();
        if (!directive.startsWith("#ifndef "))
            continue;
        const QString guard = directive.mid(8).trimmed();
        for (int j = i + 1; j < qMin(i + 5, lines.count()); ++j) {
            if (lines[j].trimmed().startsWith("#define " + guard))
                return j;
        }
        return -1;
    }
    return -1;
}
#endif
}

TestCppParser::TestCppParser(QObject *parent):
    QObject{parent}
{
    init_parser();
}

void TestCppParser::init()
{
    init_parser();
}

void TestCppParser::init_parser()
{
    mParser = std::make_shared<CppParser>();
}

void TestCppParser::test_parse_var()
{
    mParser->setOnGetFileStream([](const QString& filename, QStringList& buffer){
        buffer=QStringList({
                               "int xxx=10;"
                           });
           return true;
                                });
    CppParser::parseFileBlocking(mParser,"text.cpp",false,"");
    QCOMPARE(mParser->statementList().count(),1);
    PStatement statement = mParser->findStatement("xxx");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"xxx");
    QCOMPARE(statement->kind,StatementKind::Variable);

}

void TestCppParser::test_global_variable_reference_scope()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "int ttt;",
            "",
            "int main() {",
            "    return ttt;",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    QCOMPARE(mParser->statementList().count(), 3);
    QVERIFY(!mParser->findStatement("main::ttt"));
    PStatement declaration = mParser->findStatement("ttt");
    QVERIFY(declaration);
    QVERIFY(statementHasDefinition(declaration));
    QCOMPARE(declaration->definition.fileName, "text.cpp");
    QVERIFY(declaration->declarations.isEmpty());
    PStatement reference = mParser->findStatementOf("text.cpp", QStringList{"ttt"}, 3);
    QVERIFY(reference);
    QCOMPARE(reference, declaration);
    QCOMPARE(reference->scope, StatementScope::Global);
    QCOMPARE(getKindOfStatement(reference), StatementKind::GlobalVariable);
}

void TestCppParser::test_unresolved_member_function_reference()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<class T> using Result = typename T::type;",
            "struct C {",
            "    template<class T> Result<T> append();",
            "};",
            "C c;",
            "int main() {",
            "    c.append;",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement reference = mParser->findStatementOf(
                "text.cpp", QStringList{"c", ".", "append"}, 6);
    QVERIFY(reference);
    QCOMPARE(reference->command, "append");
    QCOMPARE(reference->kind, StatementKind::Function);
    QCOMPARE(reference->parentScope.lock(), mParser->findStatement("C"));
}

void TestCppParser::test_nested_template_member_definition()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename T> struct C {",
            "    template<typename U> C<T>& f(U);",
            "};",
            "template<typename T>",
            "template<typename U>",
            "C<T>&",
            "C<T>::",
            "f(U) { return *this; }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement function = mParser->findStatement("C::f");
    QVERIFY(function);
    QCOMPARE(function->kind, StatementKind::Function);
    QCOMPARE(function->parentScope.lock(), mParser->findStatement("C"));
    QVERIFY(statementHasDefinition(function));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(function)->templateParameters.count(), 2);
    QVERIFY(!mParser->findStatement("C<T>"));
}

void TestCppParser::test_extern_template_class_declaration()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename T> struct C {};",
            "extern template class C<int>;",
            "int after;"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement specialization = mParser->findStatement("C<int>");
    QVERIFY(specialization);
    QCOMPARE(specialization->kind, StatementKind::Class);
    QVERIFY(mParser->findStatement("after"));
    for (const PStatement &statement : mParser->statementList().statements()) {
        QVERIFY2(!statement->command.isEmpty() || statement->kind != StatementKind::Variable,
                 QString("empty variable at line %1")
                 .arg(statement->definition.range.start.line).toUtf8().constData());
    }
}

void TestCppParser::test_forward_declaration_prefers_definition_for_eval()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "class C;",
            "class C {",
            "public:",
            "    constexpr explicit operator bool() const noexcept { return true; }",
            "    constexpr const int& value() const& { return x; }",
            "    constexpr int& value() & { return x; }",
            "    int x;",
            "};",
            "C c;",
            "template<class T> class Box;",
            "template<class T> class Box { public: int value(); };",
            "Box<int> box;",
            "int main() {",
            "    c.value();",
            "    box.value();",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement reference = mParser->findStatementOf(
                "text.cpp", QStringList{"c", ".", "value", "(", ")"}, 13);
    QVERIFY(reference);
    QCOMPARE(reference->command, "value");
    QCOMPARE(reference->kind, StatementKind::Function);
    QCOMPARE(statementDeclarationForNavigation(reference, QString()).range.start.line, 5);
    PStatement templateReference = mParser->findStatementOf(
                "text.cpp", QStringList{"box", ".", "value", "(", ")"}, 14);
    QVERIFY(templateReference);
    QCOMPARE(templateReference->command, "value");
    QCOMPARE(statementDeclarationForNavigation(templateReference, QString()).range.start.line, 10);
}

void TestCppParser::test_base_member_return_uses_declaration_bindings()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<class K, class V> struct Pair { using type = V; };",
            "template<class K, class V> struct Base {",
            "    using value_type = V;",
            "    using iterator = Pair<K, value_type>;",
            "    iterator find();",
            "};",
            "template<class K, class V> struct Derived : Base<K, V> {};",
            "Derived<int, std::string> d;",
            "int main() {",
            "    d.find();",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement scope = mParser->findScopeStatement("text.cpp", 9);
    QVERIFY(scope);
    QStringList expression{"d", ".", "find", "(", ")"};
    EvalResult result = mParser->evalExpression("text.cpp", expression, scope);
    QVERIFY(result.isValid());
    PStatement declaration = result.type.declaration.lock();
    QVERIFY(declaration);
    QCOMPARE(declaration->fullName, "Pair");
    QCOMPARE(result.statement->command, "find");
}

void TestCppParser::test_member_overload_resolution()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Implicit { Implicit(const char *); };",
            "struct Explicit { explicit Explicit(const char *); };",
            "struct Other { Other(const char *); };",
            "struct Overloads {",
            "    int convert(Implicit);",
            "    template<class T> long convert(T);",
            "    int ambiguous(Implicit);",
            "    int ambiguous(Other);",
            "    int explicitOnly(Explicit);",
            "    int value();",
            "    long value() const;",
            "    int rvalueOnly() &&;",
            "};",
            "Overloads object;",
            "const Overloads constObject;",
            "int main() {",
            "    object.convert(\"x\");",
            "    object.ambiguous(\"x\");",
            "    object.explicitOnly(\"x\");",
            "    object.value();",
            "    constObject.value();",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement scope = mParser->findScopeStatement("text.cpp", 15);
    QVERIFY(scope);
    auto evaluate = [&](QStringList expression) {
        return mParser->evalExpression("text.cpp", expression, scope);
    };
    EvalResult conversion = evaluate({"object", ".", "convert", "(", "\"x\"", ")"});
    QVERIFY(conversion.isValid());
    QCOMPARE(conversion.type.qualifiedName, "int");
    EvalResult value = evaluate({"object", ".", "value", "(", ")"});
    QVERIFY(value.isValid());
    QCOMPARE(value.type.qualifiedName, "int");
    EvalResult constValue = evaluate({"constObject", ".", "value", "(", ")"});
    QVERIFY(constValue.isValid());
    QCOMPARE(constValue.type.qualifiedName, "long");
    QVERIFY(!evaluate({"object", ".", "explicitOnly", "(", "\"x\"", ")"}).isValid());
    QVERIFY(!evaluate({"object", ".", "ambiguous", "(", "\"x\"", ")"}).isValid());
    QVERIFY(!evaluate({"object", ".", "rvalueOnly", "(", ")"}).isValid());
}

void TestCppParser::test_expression_identifier_trace()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Object { int value; int get(); };",
            "Object object;",
            "Object *pointer;",
            "namespace outer { int value; } int globalValue;",
            "Object make(Object value) { return value; }",
            "template<class T> struct Box { T value; };",
            "Box<int> box;",
            "int main() {",
            "    object.get();",
            "    pointer->value;",
            "    outer::value;",
            "}"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement scope = mParser->findScopeStatement("text.cpp", 8);
    QVERIFY(scope);
    auto check = [&](QStringList expression, const QVector<int> &expectedIndexes,
                     const QVector<int> &expectedStarts = {}) {
        ExpressionEvalTrace trace;
        mParser->evalExpression("text.cpp", expression, scope, &trace);
        QCOMPARE(trace.size(), expectedIndexes.size());
        for (int i = 0; i < trace.size(); ++i) {
            QCOMPARE(trace[i].tokenIndex, expectedIndexes[i]);
            int start = expectedStarts.isEmpty() ? 0 : expectedStarts[i];
            PStatement expected = mParser->findStatementOf(
                        "text.cpp", expression.mid(start, trace[i].tokenIndex - start + 1), 8);
            if (trace[i].statement != expected) {
                qDebug() << "trace mismatch" << expression << trace[i].tokenIndex
                         << (trace[i].statement ? trace[i].statement->fullName : QString())
                         << (expected ? expected->fullName : QString());
            }
            QCOMPARE(trace[i].statement, expected);
        }
        CppTokenizer tokenizer;
        tokenizer.tokenize({expression.join(' ')});
        ExpressionEvalTrace directTrace;
        mParser->evalExpression("text.cpp", tokenizer.tokens(), scope, &directTrace);
        QCOMPARE(directTrace.size(), trace.size());
        for (int i = 0; i < directTrace.size(); ++i) {
            QCOMPARE(directTrace[i].tokenIndex, trace[i].tokenIndex);
            QCOMPARE(directTrace[i].statement, trace[i].statement);
        }
    };
    check({"object", ".", "get"}, {0, 2});
    check({"pointer", "->", "value"}, {0, 2});
    check({"outer", "::", "value"}, {0, 2});
    check({"::", "globalValue"}, {1});
    check({"make", "(", "object", ")", ".", "get"}, {0, 2, 5}, {0, 2, 0});
    check({"Box", "<", "int", ">", "::", "value"}, {0, 5});
    check({"object", ".", "missing"}, {0, 2});
}

void TestCppParser::test_parse_vars()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/vars.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    PStatement statement;
    statement = mParser->findStatement("a1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"extern const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[4]");

    statement = mParser->findStatement("a6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int &");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int **");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int **");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a9");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a9");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a10");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a10");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("a11");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a11");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const int &");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"const unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"extern const unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static unsigned int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[4]");

    statement = mParser->findStatement("b6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static unsigned int *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static unsigned int *const");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("b8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static unsigned int **");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[4]");

    statement = mParser->findStatement("s6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("s7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const std::string &");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("f1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"f1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const struct FILE *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("f2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"f2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statementType(statement),"static const struct FILE");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");

    statement = mParser->findStatement("ff1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statementType(statement),"int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"( )");

    statement = mParser->findStatement("ff2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statementType(statement),"static const int");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"( )");

    statement = mParser->findStatement("ff3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statementType(statement),"static const std::string");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"( )");

    //TODO:: ff4 ff5

    statement = mParser->findStatement("ff6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statementType(statement),"int *");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"( )");

    statement = mParser->findStatement("aa1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"aa1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(!(statement->properties & StatementProperty::FunctionPointer));
    QCOMPARE(statementType(statement),"int (*)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[5]");

    statement = mParser->findStatement("aa2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"aa2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(!(statement->properties & StatementProperty::FunctionPointer));
    QCOMPARE(statementType(statement),"static const std::string (*)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"[5]");

}

void TestCppParser::test_struct()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/struct.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    PStatement statement;
    statement = mParser->findStatement("TTT");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->command, "TTT");
    QCOMPARE(statement->kind, StatementKind::Class);
    PStatement child;
    child = std::dynamic_pointer_cast<Statement>(statement)->children.value("xxx").lock();
    QCOMPARE(child->command,"xxx");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::xxx");
    QCOMPARE(statementType(child), "int");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);
    child = std::dynamic_pointer_cast<Statement>(statement)->children.value("yyy").lock();
    QCOMPARE(child->command,"yyy");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::yyy");
    QCOMPARE(statementType(child), "short");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);
    child = std::dynamic_pointer_cast<Statement>(statement)->children.value("zzz").lock();
    QCOMPARE(child->command,"zzz");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::zzz");
    QCOMPARE(statementType(child), "double");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);

    QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->children.count(),4);
    child = std::dynamic_pointer_cast<Statement>(statement)->children.value("ppp").lock();
    QCOMPARE(child->command,"ppp");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::ppp");
    QCOMPARE(statementType(child), "int");
    QCOMPARE(child->accessibility, StatementAccessibility::Private);

}

void TestCppParser::test_resolve_members_of_class_access_scope()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "class Base {",
            "public: int publicValue;",
            "protected: int protectedValue;",
            "private: int privateValue;",
            "};",
            "class Derived : private Base { public: void action(); };",
            "struct PublicDerived : Base { void action(); };",
            "struct ProtectedDerived : protected Base { void action(); };"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    PStatement base = mParser->findStatement("Base");
    PStatement derived = mParser->findStatement("Derived");
    PStatement publicDerived = mParser->findStatement("PublicDerived");
    PStatement protectedDerived = mParser->findStatement("ProtectedDerived");
    QVERIFY(base && derived && publicDerived && protectedDerived);
    auto names = [](const StatementList &statements) {
        QSet<QString> result;
        for (const PStatement &statement : statements)
            result.insert(statement->command);
        return result;
    };

    QCOMPARE(names(mParser->resolveMembersOfClass(derived, {})),
             QSet<QString>{"action"});
    QCOMPARE(names(mParser->resolveMembersOfClass(derived, derived)),
             QSet<QString>({"action", "publicValue", "protectedValue"}));
    QCOMPARE(names(mParser->resolveMembersOfClass(derived, base)),
             QSet<QString>({"action", "publicValue", "protectedValue", "privateValue"}));
    QCOMPARE(names(mParser->resolveMembersOfClass(publicDerived, {})),
             QSet<QString>({"action", "publicValue"}));
    QCOMPARE(names(mParser->resolveMembersOfClass(protectedDerived, {})),
             QSet<QString>{"action"});
    QCOMPARE(names(mParser->resolveMembersOfClass(protectedDerived, protectedDerived)),
             QSet<QString>({"action", "publicValue", "protectedValue"}));
}

void TestCppParser::test_shared_boundary_statement_invalidation()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeFile = [](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(lines.join('\n').toUtf8());
        file.close();
        return true;
    };
    QString inner = tmp.filePath("inner.h");
    QString a = tmp.filePath("a.h");
    QString b = tmp.filePath("b.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeFile(inner, {"#pragma once", "int shared_value;"}));
    QVERIFY(writeFile(a, {"#include \"inner.h\"", "int a_value;"}));
    QVERIFY(writeFile(b, {"#include \"inner.h\"", "int b_value;"}));
    QVERIFY(writeFile(main, {"#include \"a.h\"", "#include \"b.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, true);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("shared_value"));

    // A stops including the shared header; main and B remain unchanged.
    QVERIFY(writeFile(a, {"int a_value;"}));
    mParser->invalidateFile(a);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("a_value") != nullptr);
    QVERIFY(mParser->findStatement("b_value") != nullptr);
    QVERIFY(mParser->findStatement("shared_value") != nullptr);
}

void TestCppParser::test_shared_boundary_statement_invalidation_only()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeFile = [](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(lines.join('\n').toUtf8());
        file.close();
        return true;
    };
    QString inner = tmp.filePath("inner.h");
    QString a = tmp.filePath("a.h");
    QString b = tmp.filePath("b.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeFile(inner, {"#pragma once", "int shared_value;"}));
    QVERIFY(writeFile(a, {"#include \"inner.h\"", "int a_value;"}));
    QVERIFY(writeFile(b, {"#include \"inner.h\"", "int b_value;"}));
    QVERIFY(writeFile(main, {"#include \"a.h\"", "#include \"b.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, true);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("shared_value"));

    mParser->invalidateFile(a);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("a_value") != nullptr);
    QVERIFY(mParser->findStatement("b_value") != nullptr);
    QVERIFY(mParser->findStatement("shared_value") != nullptr);
}

void TestCppParser::test_invalidation_preserves_earlier_boundary_record()
{
    QString dir = QFileInfo("resources/preprocessor/boundary_replay").absoluteFilePath();
    QString includeDir = dir + "/include";
    QString root = dir + "/order_ba.cpp";
    QString earlier = includeDir + "/plain_b.h";
    QString changed = includeDir + "/plain_a.h";
    mParser->addIncludePath(includeDir);
    mParser->addProjectFile(root, true);
    CppParser::parseFileBlocking(mParser, root, false, "");
    PStatement before = mParser->findStatement("plain_b");
    QVERIFY(before);

    mParser->invalidateFile(changed);
    QVERIFY(mParser->hasReplayRecord(earlier));
    QVERIFY(!mParser->hasReplayRecord(changed));
    CppParser::parseFileBlocking(mParser, root, false, "");
    QCOMPARE(mParser->findStatement("plain_b"), before);
}

void TestCppParser::test_removed_boundary_include_remains_inactive_cached_record()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString a = tmp.filePath("a.h");
    QString b = tmp.filePath("b.h");
    QString main = tmp.filePath("main.cpp");
    QString other = tmp.filePath("other.cpp");
    QVERIFY(writeLines(a, {"int a_v1;"}));
    QVERIFY(writeLines(b, {"int cached_b;"}));
    QVERIFY(writeLines(main, {"#include \"a.h\"", "#include \"b.h\""}));
    QVERIFY(writeLines(other, {"#include \"a.h\"", "#include \"b.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    mParser->addProjectFile(other, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    QVERIFY(mParser->findStatementOf(main, "cached_b", 0));

    QVERIFY(writeLines(a, {"int a_v2;"}));
    QVERIFY(writeLines(main, {"#include \"a.h\""}));
    mParser->invalidateFile(a);
    CppParser::parseFileBlocking(mParser, main, true, "");
    QVERIFY(mParser->findStatementOf(main, "a_v2", 0));
    QVERIFY(!mParser->findStatementOf(main, "cached_b", 0));
    QCOMPARE(mParser->preprocessorEventCountsForTest(b).invalidate, 0);

    CppParser::parseFileBlocking(mParser, other, true, "");
    QVERIFY(mParser->findStatementOf(other, "cached_b", 0));
    QCOMPARE(mParser->preprocessorEventCountsForTest(a).scan, 2);
    QVERIFY(mParser->preprocessorEventCountsForTest(a).replay > 0);
    QCOMPARE(mParser->preprocessorEventCountsForTest(b).scan, 1);
    QVERIFY(mParser->preprocessorEventCountsForTest(b).replay > 0);
}

void TestCppParser::test_repeated_template_include_preserves_each_context()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString tpl = tmp.filePath("member.tpl");
    QString host = tmp.filePath("host.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(tpl, {"#ifdef MODE", "int tpl_value;", "#else",
                             "int default_value;", "#endif"}));
    QVERIFY(writeLines(host, {"namespace N1 {", "#define MODE 1",
                              "#include \"member.tpl\"", "#undef MODE", "}",
                              "namespace N2 {", "#include \"member.tpl\"", "}"}));
    QVERIFY(writeLines(main, {"#include \"host.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatement first = mParser->findStatement("N1::tpl_value");
    PStatement second = mParser->findStatement("N2::default_value");
    QVERIFY(first);
    QVERIFY(second);
    QCOMPARE(first->definition.fileName, tpl);
    QCOMPARE(second->definition.fileName, tpl);
    QCOMPARE(first->definition.boundaryAncestor, host);
    QCOMPARE(second->definition.boundaryAncestor, host);
}

void TestCppParser::test_shared_template_statement_ownership()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString tpl = tmp.filePath("member.tpl");
    QString a = tmp.filePath("a.h");
    QString b = tmp.filePath("b.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(tpl, {"int tpl_value;"}));
    QVERIFY(writeLines(a, {"namespace N1 {", "#include \"member.tpl\"", "}"}));
    QVERIFY(writeLines(b, {"namespace N2 {", "#include \"member.tpl\"", "}"}));
    QVERIFY(writeLines(main, {"#include \"a.h\"", "#include \"b.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatement first = mParser->findStatement("N1::tpl_value");
    PStatement second = mParser->findStatement("N2::tpl_value");
    QVERIFY(first);
    QVERIFY(second);
    QVERIFY(first != second);
    QCOMPARE(first->definition.fileName, tpl);
    QCOMPARE(second->definition.fileName, tpl);
    QCOMPARE(first->definition.boundaryAncestor, a);
    QCOMPARE(second->definition.boundaryAncestor, b);
}

void TestCppParser::test_statement_ownership_declaration_definition_invalidation()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString declaration = tmp.filePath("declare.h");
    QString definition = tmp.filePath("define.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(declaration, {"int declared_function();"}));
    QVERIFY(writeLines(definition, {"int declared_function() { return 1; }"}));
    QVERIFY(writeLines(main, {"#include \"declare.h\"", "#include \"define.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, true);
    CppParser::parseFileBlocking(mParser, main, false, "");
    PStatement statement = mParser->findStatement("declared_function");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));
    QCOMPARE(statement->declarations.count(), 1);
    QVERIFY(statement->declarations.contains(declaration));
    QCOMPARE(statement->declarations.value(declaration).boundaryAncestor, declaration);
    QCOMPARE(statement->definition.boundaryAncestor, definition);

    mParser->invalidateFile(definition);
    statement = mParser->findStatement("declared_function");
    QVERIFY(statement);
    QVERIFY(!statementHasDefinition(statement));
    QCOMPARE(statement->declarations.count(), 1);
    QVERIFY(statement->declarations.contains(declaration));
    QVERIFY(!statement->definition.range.isValid());

    CppParser::parseFileBlocking(mParser, main, false, "");
    statement = mParser->findStatement("declared_function");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));
    QCOMPARE(statement->definition.boundaryAncestor, definition);

    mParser->invalidateFileRecordForTest(declaration);
    statement = mParser->findStatement("declared_function");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));
    QVERIFY(statement->declarations.isEmpty());
    QCOMPARE(statement->definition.fileName, definition);
    QCOMPARE(statement->definition.boundaryAncestor, definition);
}

void TestCppParser::test_boundary_invalidation_removes_defines_and_statements()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString header = tmp.filePath("product.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(header, {"#define BOUNDARY_VALUE 1", "int boundary_statement;"}));
    QVERIFY(writeLines(main, {"#include \"product.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, true);
    CppParser::parseFileBlocking(mParser, main, false, "");
    PParsedFileInfo fileInfo = mParser->findFileInfo(header);
    QVERIFY(fileInfo);
    PReplayRecord replayRecord = mParser->findReplayRecord(header);
    QVERIFY(replayRecord);
    QVERIFY(replayRecord->defines()->contains("BOUNDARY_VALUE"));
    QVERIFY(fileInfo->statements().contains("boundary_statement"));
    QVERIFY(mParser->findStatement("boundary_statement"));

    mParser->invalidateFile(header);
    QVERIFY(!mParser->findFileInfo(header));
    QVERIFY(!mParser->findReplayRecord(header));
    QVERIFY(!mParser->findStatement("boundary_statement"));

    CppParser::parseFileBlocking(mParser, main, false, "");
    fileInfo = mParser->findFileInfo(header);
    QVERIFY(fileInfo);
    replayRecord = mParser->findReplayRecord(header);
    QVERIFY(replayRecord);
    QVERIFY(replayRecord->defines()->contains("BOUNDARY_VALUE"));
    QVERIFY(fileInfo->statements().contains("boundary_statement"));
    QVERIFY(mParser->findStatement("boundary_statement"));
}

void TestCppParser::test_replay_reuses_existing_statements()
{
    QString dir = QFileInfo("resources/preprocessor/boundary_replay").absoluteFilePath();
    QString includeDir = dir + "/include";
    QString root = dir + "/order_ab.cpp";
    mParser->addIncludePath(includeDir);
    mParser->addProjectFile(root, true);
    CppParser::parseFileBlocking(mParser, root, false, "");
    PStatement before = mParser->findStatement("plain_a");
    QVERIFY(before);
    int count = mParser->statementList().count();

    CppParser::parseFileBlocking(mParser, root, false, "");
    QCOMPARE(mParser->findStatement("plain_a"), before);
    QCOMPARE(mParser->statementList().count(), count);
}

void TestCppParser::test_preprocessor_statements()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "#define OBJECT_MACRO 7",
            "#define FUNCTION_MACRO(x) ((x)+1)",
            "int value;"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "macros.cpp", false, "");

    PStatement object = mParser->findStatement("OBJECT_MACRO");
    QVERIFY(object);
    QCOMPARE(object->kind, StatementKind::Preprocessor);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(object)->args, "");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(object)->value, "7");
    QCOMPARE(object->definition.fileName, "macros.cpp");
    QCOMPARE(object->definition.range.start.line, 0);
    QVERIFY(statementHasDefinition(object));

    PStatement function = mParser->findStatement("FUNCTION_MACRO");
    QVERIFY(function);
    QCOMPARE(function->kind, StatementKind::Preprocessor);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(function)->args, "(x)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(function)->value, "((x)+1)");
    QCOMPARE(function->definition.fileName, "macros.cpp");
    QCOMPARE(function->definition.range.start.line, 1);
    QVERIFY(statementHasDefinition(function));
}

void TestCppParser::test_label_statements()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "void f() { retry: ; }",
            "void g() { bool flag = true; flag ? retry : retry; "
            "switch (flag) { case true: break; default: break; } }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "labels.cpp", false, "");

    PStatement function = mParser->findStatement("f");
    QVERIFY(function);
    PStatement label = std::dynamic_pointer_cast<Statement>(function)->children.value("retry").lock();
    QVERIFY(label);
    QCOMPARE(label->kind, StatementKind::Label);
    QCOMPARE(label->fullName, "f::retry");
    QCOMPARE(label->definition.range.start.line, 0);
    QCOMPARE(label->definition.fileName, "labels.cpp");

    PStatement notLabels = mParser->findStatement("g");
    QVERIFY(notLabels);
    for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(notLabels)->children) {
        PStatement child = weak.lock();
        QVERIFY(!child || child->kind != StatementKind::Label);
    }
}

void TestCppParser::test_hard_define_statements()
{
    mParser->addHardDefineByLine("#define HARD_MACRO(x) ((x)+1)");
    mParser->parseHardDefines();
    PStatement macro = mParser->findStatement("HARD_MACRO");
    QVERIFY(macro);
    QCOMPARE(macro->kind, StatementKind::Preprocessor);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(macro)->args, "(x)");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(macro)->value, "((x)+1)");
    QVERIFY(statementDeclarationForNavigation(macro, QString()).fileName.isEmpty());
    QVERIFY(!statementDefinitionForNavigation(macro).range.isValid());
    QVERIFY(statementHasDefinition(macro));
    QCOMPARE(mParser->statementList().count(), 1);
}

void TestCppParser::test_predefined_func_variable()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "void alpha() {}",
            "struct C { void beta() {} };",
            "void declarationOnly();"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement alphaFunc = mParser->findStatementOf("text.cpp", QStringList{"__func__"}, 0);
    QVERIFY(alphaFunc);
    QCOMPARE(alphaFunc->kind, StatementKind::LocalVariable);
    QCOMPARE(alphaFunc->fullName, "alpha::__func__");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(alphaFunc)->value, "\"alpha\"");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(alphaFunc)->typeInfo.qualifiedName, "char");
    QVERIFY(std::dynamic_pointer_cast<Statement>(alphaFunc)->typeInfo.qualifiers.testFlag(TypeQualifier::Const));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(alphaFunc)->typeInfo.derivations.count(), 1);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(alphaFunc)->typeInfo.derivations.first().kind,
             TypeDerivationKind::Array);
    QVERIFY(statementHasDefinition(alphaFunc));
    PStatement alpha = mParser->findStatement("alpha");
    QVERIFY(alpha);
    QCOMPARE(alphaFunc->definition.fileName, alpha->definition.fileName);
    QCOMPARE(alphaFunc->definition.range, alpha->definition.range);

    PStatement betaFunc = mParser->findStatementOf("text.cpp", QStringList{"__func__"}, 1);
    QVERIFY(betaFunc);
    QCOMPARE(betaFunc->fullName, "C::beta::__func__");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(betaFunc)->value, "\"beta\"");
    QVERIFY(!mParser->findStatementOf("text.cpp", QStringList{"__func__"}, 2));
    QVERIFY(!mParser->findStatementOf("text.cpp", QStringList{"__func__"}, 3));
    QVERIFY(!mParser->findStatement("__func__"));
}

void TestCppParser::test_structured_bindings()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/structured-bindings.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    for (const QString &name : QStringList{"a", "b", "c", "d", "e", "g"}) {
        PStatement statement = mParser->findStatement(name);
        QVERIFY(statement!=nullptr);
        QCOMPARE(statement->fullName,name);
        QCOMPARE(statement->kind,StatementKind::Variable);
        QCOMPARE(statementType(statement),"auto");
        QCOMPARE(std::dynamic_pointer_cast<Statement>(statement)->args,"");
    }
}

void TestCppParser::test_system_subtree_inheritance_uses_boundary_record()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString base = tmp.filePath("base.h");
    QString boundary = tmp.filePath("boundary.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(base, {"namespace N { struct Base { int inherited_value; }; }"}));
    QVERIFY(writeLines(boundary, {"#include \"base.h\"",
                                  "struct Derived : N::Base {};"}));
    QVERIFY(writeLines(main, {"#include \"boundary.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    QVERIFY(mParser->findFileInfo(base));
    QVERIFY(mParser->findFileInfo(base)->statements().contains("N::Base"));
    QVERIFY(!mParser->findReplayRecord(base));
    QVERIFY(mParser->findStatement("N::Base"));
    QVERIFY(mParser->findStatement("Derived"));
    QVERIFY(mParser->findStatement("Derived::inherited_value"));
}

void TestCppParser::test_file_info_for_boundary_children()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QString root = tmp.filePath("root.h");
    QString inner = tmp.filePath("inner.h");
    QVERIFY(writeLines(main, {"#define USER_MODE 1", "#include \"root.h\""}));
    QVERIFY(writeLines(root, {
        "namespace Root { void rootFunction() {} }",
        "using namespace Root;",
        "#ifdef USER_MODE",
        "int rootVisible;",
        "#else",
        "int rootHidden;",
        "#endif",
        "#include \"inner.h\""
    }));
    QVERIFY(writeLines(inner, {"namespace Inner { void innerFunction() {} }"}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");

    PParsedFileInfo rootInfo = mParser->findFileInfo(root);
    PParsedFileInfo innerInfo = mParser->findFileInfo(inner);
    QVERIFY(rootInfo && innerInfo);
    QVERIFY(rootInfo->statements().contains("Root"));
    QVERIFY(rootInfo->statements().contains("Root::rootFunction"));
    QVERIFY(rootInfo->statements().contains("rootVisible"));
    QVERIFY(!rootInfo->statements().contains("rootHidden"));
    QVERIFY(!rootInfo->statements().contains("Inner"));
    QVERIFY(innerInfo->statements().contains("Inner"));
    QVERIFY(innerInfo->statements().contains("Inner::innerFunction"));
    QVERIFY(!innerInfo->statements().contains("Root"));
    QVERIFY(rootInfo->findScopeAtLine(0));
    QVERIFY(innerInfo->findScopeAtLine(0));
    QVERIFY(rootInfo->usings().contains("Root"));
    QVERIFY(rootInfo->isLineVisible(3));
    QVERIFY(!rootInfo->isLineVisible(5));
    QVERIFY(mParser->findReplayRecord(root));
    QVERIFY(!mParser->findReplayRecord(inner));

    CppParser::parseFileBlocking(mParser, main, true, "");
    rootInfo = mParser->findFileInfo(root);
    innerInfo = mParser->findFileInfo(inner);
    QVERIFY(rootInfo && innerInfo);
    QVERIFY(rootInfo->findScopeAtLine(0));
    QVERIFY(innerInfo->findScopeAtLine(0));
    QVERIFY(rootInfo->isLineVisible(3));
    QVERIFY(!rootInfo->isLineVisible(5));
    QVERIFY(rootInfo->statements().contains("Root::rootFunction"));
    QVERIFY(innerInfo->statements().contains("Inner::innerFunction"));

    mParser->invalidateFileRecordForTest(inner);
    QVERIFY(!mParser->findFileInfo(inner));
    QVERIFY(!mParser->findStatement("Inner"));
    QVERIFY(mParser->findFileInfo(root));
    QVERIFY(mParser->findStatement("Root"));
}

void TestCppParser::test_cross_boundary_declaration_definition_file_info()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QString outside = tmp.filePath("outside.h");
    QString boundary = tmp.filePath("boundary.h");
    QString inner = tmp.filePath("inner.h");
    QVERIFY(writeLines(main, {"#include \"outside.h\"", "#include \"boundary.h\""}));

    struct Role {
        QString fileName;
        QString boundaryAncestor;
    };
    const Role roles[] = {
        {outside, QString()},
        {boundary, boundary},
        {inner, boundary}
    };
    auto affectedFiles = [&](int role) {
        if (role == 0)
            return QSet<QString>{outside};
        if (role == 1)
            return QSet<QString>{boundary, inner};
        return QSet<QString>{boundary, inner};
    };

    for (int declarationRole = 0; declarationRole < 3; ++declarationRole) {
        for (int definitionRole = 0; definitionRole < 3; ++definitionRole) {
            qInfo() << "cross boundary case" << declarationRole << definitionRole;
            for (int role = 0; role < 3; ++role) {
                QStringList lines;
                if (role == declarationRole)
                    lines.append("int cross();");
                if (role == definitionRole)
                    lines.append("int cross() { return 1; }");
                if (lines.isEmpty())
                    lines.append(QString("int marker_%1;").arg(role));
                if (role == 1)
                    lines.append("#include \"inner.h\"");
                QVERIFY(writeLines(roles[role].fileName, lines));
            }

            mParser = std::make_shared<CppParser>();
            mParser->addIncludePath(tmp.path());
            mParser->addProjectFile(main, false);
            mParser->addProjectFile(outside, false);
            CppParser::parseFileBlocking(mParser, main, true, "");

            auto hasOccurrence = [](const PStatement &statement, const Role &role) -> bool {
                for (const StatementOccurrence &occurrence : statement->declarations) {
                    if (occurrence.fileName == role.fileName)
                        return occurrence.boundaryAncestor == role.boundaryAncestor;
                }
                return false;
            };
            auto verify = [&]() {
                PStatement cross = mParser->findStatement("cross");
                QVERIFY(cross);
                QVERIFY(statementHasDefinition(cross));
                QCOMPARE(cross->declarations.count(), 1);
                QVERIFY(hasOccurrence(cross, roles[declarationRole]));
                QCOMPARE(cross->definition.fileName, roles[definitionRole].fileName);
                QCOMPARE(cross->definition.boundaryAncestor,
                         roles[definitionRole].boundaryAncestor);
                for (int role = 0; role < 3; ++role) {
                    PParsedFileInfo info = mParser->findFileInfo(roles[role].fileName);
                    QVERIFY(info);
                    QCOMPARE(info->statements().contains("cross"),
                             role == declarationRole || role == definitionRole);
                }
                QVERIFY(mParser->findReplayRecord(outside));
                QVERIFY(mParser->findReplayRecord(boundary));
                QVERIFY(!mParser->findReplayRecord(inner));
            };
            qInfo() << "verify initial";
            verify();

            CppParser::parseFileBlocking(mParser, main, true, "");
            qInfo() << "verify stable reparse";
            verify();

            mParser->invalidateFile(roles[declarationRole].fileName);
            PStatement cross = mParser->findStatement("cross");
            if (affectedFiles(declarationRole).contains(roles[definitionRole].fileName)) {
                QVERIFY(!cross);
            } else {
                QVERIFY(cross);
                QVERIFY(statementHasDefinition(cross));
                QVERIFY(cross->declarations.isEmpty());
                QCOMPARE(cross->definition.fileName, roles[definitionRole].fileName);
                QVERIFY(cross->definition.range.start.isValid());
                PParsedFileInfo definitionInfo = mParser->findFileInfo(
                            roles[definitionRole].fileName);
                QVERIFY(definitionInfo);
                QVERIFY(definitionInfo->statements().contains("cross"));
            }
            QVERIFY(!mParser->findFileInfo(roles[declarationRole].fileName));
            CppParser::parseFileBlocking(mParser, main, true, "");
            qInfo() << "verify declaration recovery";
            verify();

            mParser->invalidateFile(roles[definitionRole].fileName);
            cross = mParser->findStatement("cross");
            if (affectedFiles(definitionRole).contains(roles[declarationRole].fileName)) {
                QVERIFY(!cross);
            } else {
                QVERIFY(cross);
                QVERIFY(!statementHasDefinition(cross));
                QCOMPARE(cross->declarations.count(), 1);
                QVERIFY(hasOccurrence(cross, roles[declarationRole]));
                QVERIFY(!cross->definition.range.isValid());
                QVERIFY(cross->definition.fileName.isEmpty());
                QVERIFY(cross->definition.boundaryAncestor.isEmpty());
                PParsedFileInfo declarationInfo = mParser->findFileInfo(
                            roles[declarationRole].fileName);
                QVERIFY(declarationInfo);
                QVERIFY(declarationInfo->statements().contains("cross"));
            }
            QVERIFY(!mParser->findFileInfo(roles[definitionRole].fileName));
            CppParser::parseFileBlocking(mParser, main, true, "");
            qInfo() << "verify definition recovery";
            verify();
        }
    }
}

void TestCppParser::test_user_header_change_invalidates_dependent_roots()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString leaf = tmp.filePath("leaf.h");
    QString middle = tmp.filePath("middle.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(leaf, {"int leaf_old;"}));
    QVERIFY(writeLines(middle, {"#include \"leaf.h\"", "int middle_value;"}));
    QVERIFY(writeLines(main, {"#include \"middle.h\"", "int main_value;"}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    mParser->addProjectFile(middle, false);
    mParser->addProjectFile(leaf, false);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("leaf_old"));

    QVERIFY(writeLines(leaf, {"int leaf_new;"}));
    mParser->invalidateFile(leaf);
    QVERIFY(!mParser->findStatement("leaf_old"));
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("leaf_new"));
    QVERIFY(mParser->findStatement("middle_value"));
    QVERIFY(mParser->findStatement("main_value"));
}

void TestCppParser::test_shared_internal_header_change_invalidates_boundary_ancestors()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString inner = tmp.filePath("inner.h");
    QString a = tmp.filePath("a.h");
    QString b = tmp.filePath("b.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(inner, {"#pragma once", "int shared_old;"}));
    QVERIFY(writeLines(a, {"#include \"inner.h\"", "int a_value;"}));
    QVERIFY(writeLines(b, {"#include \"inner.h\"", "int b_value;"}));
    QVERIFY(writeLines(main, {"#include \"a.h\"", "#include \"b.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("shared_old"));
    int oldCount = mParser->statementList().count();

    QVERIFY(writeLines(inner, {"#pragma once", "int shared_new;"}));
    mParser->invalidateFile(inner);
    QVERIFY(!mParser->findStatement("shared_old"));
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("shared_new"));
    QVERIFY(mParser->findStatement("a_value"));
    QVERIFY(mParser->findStatement("b_value"));
    QCOMPARE(mParser->statementList().count(), oldCount);
}

void TestCppParser::test_root_switch_with_macro_change_removes_stale_statements()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString header = tmp.filePath("config.h");
    QString a = tmp.filePath("a.cpp");
    QString b = tmp.filePath("b.cpp");
    QVERIFY(writeLines(header, {"#ifdef MODE", "int mode_enabled;", "#else",
                                "int mode_disabled;", "#endif"}));
    QVERIFY(writeLines(a, {"#define MODE 1", "#include \"config.h\""}));
    QVERIFY(writeLines(b, {"#include \"config.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(a, false);
    mParser->addProjectFile(b, false);
    CppParser::parseFileBlocking(mParser, a, false, "");
    QVERIFY(mParser->findStatement("mode_enabled"));

    CppParser::parseFileBlocking(mParser, b, false, "");
    QVERIFY(mParser->findStatementOf(b, "mode_disabled", 0));
    QVERIFY(!mParser->findStatementOf(b, "mode_enabled", 0));
    QVERIFY(!mParser->findStatement("mode_enabled"));

    CppParser::parseFileBlocking(mParser, a, false, "");
    QVERIFY(mParser->findStatementOf(a, "mode_enabled", 0));
    QVERIFY(!mParser->findStatementOf(a, "mode_disabled", 0));
    QVERIFY(!mParser->findStatement("mode_disabled"));
}

void TestCppParser::test_root_switch_with_include_order_change_removes_stale_statements()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString first = tmp.filePath("first.h");
    QString second = tmp.filePath("second.h");
    QString ab = tmp.filePath("ab.cpp");
    QString ba = tmp.filePath("ba.cpp");
    QVERIFY(writeLines(first, {"#define FROM_FIRST 1", "int first_value;"}));
    QVERIFY(writeLines(second, {"#ifdef FROM_FIRST", "int second_after_first;", "#else",
                                "int second_alone;", "#endif"}));
    QVERIFY(writeLines(ab, {"#include \"first.h\"", "#include \"second.h\""}));
    QVERIFY(writeLines(ba, {"#include \"second.h\"", "#include \"first.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(ab, false);
    mParser->addProjectFile(ba, false);
    CppParser::parseFileBlocking(mParser, ab, false, "");
    QVERIFY(mParser->findStatement("second_after_first"));

    CppParser::parseFileBlocking(mParser, ba, false, "");
    QVERIFY(mParser->findStatementOf(ba, "second_alone", 0));
    QVERIFY(!mParser->findStatementOf(ba, "second_after_first", 0));
    QVERIFY(!mParser->findStatement("second_after_first"));

    CppParser::parseFileBlocking(mParser, ab, false, "");
    QVERIFY(mParser->findStatementOf(ab, "second_after_first", 0));
    QVERIFY(!mParser->findStatementOf(ab, "second_alone", 0));
    QVERIFY(!mParser->findStatement("second_alone"));
}

void TestCppParser::test_internal_header_disk_change_removes_stale_statements()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString inner = tmp.filePath("inner.h");
    QString outer = tmp.filePath("outer.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(inner, {"int inner_old;"}));
    QVERIFY(writeLines(outer, {"#include \"inner.h\"", "int outer_value;"}));
    QVERIFY(writeLines(main, {"#include \"outer.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("inner_old"));

    QVERIFY(writeLines(inner, {"int inner_new;", "int inner_new_extra;"}));
    CppParser::parseFileBlocking(mParser, main, false, "");
    QCOMPARE(mParser->preprocessorEventCountsForTest(outer).scan, 2);
    QVERIFY(mParser->findStatement("inner_new"));
    QVERIFY(mParser->findStatement("inner_new_extra"));
    QVERIFY(!mParser->findStatement("inner_old"));
    QVERIFY(mParser->findStatement("outer_value"));
}

void TestCppParser::test_bits_stdcpp_not_rescanned_when_switching_roots()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString a = tmp.filePath("a.cpp");
    QString b = tmp.filePath("b.cpp");
    QVERIFY(writeLines(a, {"#include <bits/stdc++.h>", "int root_a;"}));
    QVERIFY(writeLines(b, {"#include <bits/stdc++.h>", "int root_b;"}));
    QString includeDir = QFileInfo("resources/preprocessor/boundary_replay/include").absoluteFilePath();
    QString bitsStdcpp = includeDir + "/bits/stdc++.h";
    mParser->addIncludePath(includeDir);

    CppParser::parseFileBlocking(mParser, a, true, "");
    QCOMPARE(mParser->preprocessorEventCountsForTest(bitsStdcpp).scan, 1);
    int statementCount = mParser->statementList().count();

    for (int i = 0; i < 2; ++i) {
        CppParser::parseFileBlocking(mParser, b, true, "");
        QCOMPARE(mParser->preprocessorEventCountsForTest(bitsStdcpp).scan, 1);
        CppParser::parseFileBlocking(mParser, a, true, "");
        QCOMPARE(mParser->preprocessorEventCountsForTest(bitsStdcpp).scan, 1);
    }
    QVERIFY(mParser->preprocessorEventCountsForTest(bitsStdcpp).replay >= 4);
    QCOMPARE(mParser->statementList().count(), statementCount + 1);
}

void TestCppParser::test_reset_parser_clears_registry_and_statements()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString header = tmp.filePath("value.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(header, {"int header_value;"}));
    QVERIFY(writeLines(main, {"#include \"value.h\"", "int main_value;"}));
    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("header_value"));

    mParser->resetParser();
    QCOMPARE(mParser->statementList().count(), 0);
    QVERIFY(!mParser->hasReplayRecord(main));
    QVERIFY(!mParser->hasReplayRecord(header));
    QVERIFY(!mParser->findFileInfo(main));
    QVERIFY(!mParser->findFileInfo(header));
    QVERIFY(mParser->namespaces().isEmpty());
    QVERIFY(mParser->projectFiles().isEmpty());

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("header_value"));
    QVERIFY(mParser->findStatement("main_value"));
}

void TestCppParser::test_auto_invalidation_preserves_definition_after_declaration_change()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString declaration = tmp.filePath("declaration.h");
    QString definition = tmp.filePath("definition.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(declaration, {"int declared_function();"}));
    QVERIFY(writeLines(definition, {"int declared_function() { return 1; }"}));
    QVERIFY(writeLines(main, {"#include \"declaration.h\"", "#include \"definition.h\""}));
    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, false, "");
    PStatement statement = mParser->findStatement("declared_function");
    QVERIFY(statement && statementHasDefinition(statement));

    QVERIFY(writeLines(declaration, {"int declared_function();", "int declaration_marker;"}));
    CppParser::parseFileBlocking(mParser, main, false, "");
    statement = mParser->findStatement("declared_function");
    QVERIFY(statement && statementHasDefinition(statement));
    QCOMPARE(statement->declarations.value(declaration).boundaryAncestor, declaration);
    QCOMPARE(statement->definition.boundaryAncestor, definition);
    QVERIFY(mParser->findStatement("declaration_marker"));
    QCOMPARE(mParser->preprocessorEventCountsForTest(declaration).scan, 2);
    QCOMPARE(mParser->preprocessorEventCountsForTest(definition).scan, 2);
    QCOMPARE(mParser->preprocessorEventCountsForTest(definition).replay, 0);
}

void TestCppParser::test_cppreference_declarator_syntax()
{
    parseSyntaxResource(mParser, "declarators.cpp");
    PStatement statement = mParser->findStatement("a");
    QVERIFY(statement);
    QCOMPARE(statement->kind, StatementKind::Variable);
    QCOMPARE(statementType(statement), "int");

    statement = mParser->findStatement("p");
    QVERIFY(statement);
    QCOMPARE(statementType(statement), "int *");

    QVERIFY(mParser->findStatement("f"));
    statement = mParser->findStatement("pf");
    QVERIFY(statement);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);

    for (const char *name : {"S::f", "S::g", "complex_parameters", "complex_defaults",
                             "unnamed_parameters", "fpif", "trailing_noexcept",
                             "printf_like", "variadic_template"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
}

void TestCppParser::test_cppreference_function_syntax()
{
    parseSyntaxResource(mParser, "functions.cpp");
    PStatement f1 = mParser->findStatement("f1");
    QVERIFY(f1);
    QCOMPARE(statementDeclarationForNavigation(f1, QString()).range.start.line, 9);
    QCOMPARE(f1->definition.range.start.line, 31);
    for (const char *name : {"f0", "f1", "fp03", "fp11", "main", "f2"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
    QVERIFY(!mParser->findStatement("bar"));
    QCOMPARE(mParser->findStatement("f0")->kind, StatementKind::Function);
    QCOMPARE(mParser->findStatement("fp03")->kind, StatementKind::Function);
    QCOMPARE(mParser->findStatement("fp11")->kind, StatementKind::Function);
}

void TestCppParser::test_cppreference_class_syntax()
{
    parseSyntaxResource(mParser, "classes.cpp");
    PStatement d1 = mParser->findStatement("S::d1");
    QVERIFY(d1);
    QCOMPARE(d1->definition.range.start.line, 7);
    for (const char *name : {
             "S::d1", "S::a", "S::d2", "S::f1", "S::d3", "S::d4", "S::f2",
             "S::NORTH", "S::NestedS", "S::NestedS::s", "S::d5", "S::d6",
             "S::value_type", "S::pointer_type", "M::M", "M::operator()",
             "Base::d", "Derived::d", "MemberTemplates::f",
             "MemberTemplates::NestedS", "MemberTemplates::NestedS::s"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
}

void TestCppParser::test_cppreference_template_syntax()
{
    parseSyntaxResource(mParser, "templates.cpp");
    for (const char *name : {"vec::vector", "A", "A::f", "A<MyClass>", "Ptr",
                             "S::f", "S::NestedS", "S::NestedS::s", "f", "X", "T5"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
}

void TestCppParser::test_cppreference_enum_syntax()
{
    parseSyntaxResource(mParser, "enums.cpp");
    for (const char *name : {"smallenum", "color", "red", "yellow", "green", "blue",
                             "altitude", "altitude::high", "altitude::low", "d", "e", "f",
                             "E11", "E11::x", "E11::y", "E98::x", "N98::x", "S98::x",
                             "cxx20::long_long_long_name", "cxx20::using_enum_demo"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("green"))->value, "20");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("blue"))->value, "21");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("d"))->value, "0");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("e"))->value, "1");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatement("f"))->value, "3");
}

void TestCppParser::test_cppreference_namespace_syntax()
{
    parseSyntaxResource(mParser, "namespaces.cpp");
    for (const char *name : {"Q::V", "Q::V::C", "Q::V::C::m", "Q::V::f", "Q::V::h",
                             "A::B::C", "A::B::C::nested_value", "Lib::Lib_1",
                             "Lib::Lib_1::A", "Lib::Lib_1::g", "Lib::A<MyClass>",
                             "unnamed_value"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
}

void TestCppParser::test_cppreference_lambda_syntax()
{
    QString file = parseSyntaxResource(mParser, "lambdas.cpp");
    QVERIFY(mParser->findStatement("main"));
    QCOMPARE(mParser->findStatementOf(file, "x", 5)->definition.range.start.line, 5);
    for (const char *name : {"x", "simple", "func1", "func2",
                             "nth_fibonacci", "generic", "y"}) {
        QVERIFY2(mParser->findStatementOf(file, QString::fromLatin1(name), 5), name);
    }
    QCOMPARE(mParser->findStatementOf(file, "func1", 5)->kind, StatementKind::Variable);
    QCOMPARE(mParser->findStatementOf(file, "generic", 5)->kind, StatementKind::Variable);
}

void TestCppParser::test_cppreference_structured_binding_syntax()
{
    QString file = parseSyntaxResource(mParser, "structured_bindings.cpp");
    QVERIFY(mParser->findStatement("main"));
    QVERIFY(mParser->findStatementOf(file, "myset", 6));
    QCOMPARE(mParser->findStatementOf(file, "myset", 6)->definition.range.start.line, 6);
    QVERIFY(mParser->findStatementOf(file, "iter", 8));
    QVERIFY(mParser->findStatementOf(file, "success", 8));
    QVERIFY(mParser->findStatementOf(file, "BitFields", 12));
    for (const char *name : {"b", "d", "p", "q"}) {
        QVERIFY2(mParser->findStatementOf(file, QString::fromLatin1(name), 18), name);
    }
    QVERIFY(mParser->findStatementOf(file, "s", 26));
}

void TestCppParser::test_cppreference_range_for_syntax()
{
    QString file = parseSyntaxResource(mParser, "range_for.cpp");
    QVERIFY(mParser->findStatement("main"));
    QVERIFY(mParser->findStatementOf(file, "v", 5));
    QVERIFY(mParser->findStatementOf(file, "cv", 11));
    QVERIFY(mParser->findStatementOf(file, "a", 16));
    QVERIFY(mParser->findStatementOf(file, "i", 7));
    QVERIFY(mParser->findStatementOf(file, "n", 14));
    QVERIFY(mParser->findStatementOf(file, "elem_t", 23));
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatementOf(file, "v", 5))->typeInfo.qualifiedNameText,
             "std::vector<int>");
    QCOMPARE(statementType(mParser->findStatementOf(file, "v", 5)), "std::vector<int>");
    QCOMPARE(std::dynamic_pointer_cast<Statement>(mParser->findStatementOf(file, "a", 16))->args, "[]");
}

void TestCppParser::test_cppreference_ranges_syntax()
{
    QString file = parseSyntaxResource(mParser, "ranges.cpp");
    QVERIFY(mParser->findStatement("filter_example"));
    QVERIFY(mParser->findStatement("join_example"));
    QVERIFY(mParser->findStatementOf(file, "even", 9));
    QVERIFY(mParser->findStatementOf(file, "square", 9));
    QVERIFY(mParser->findStatementOf(file, "i", 12));
    QVERIFY(mParser->findStatementOf(file, "bits", 22));
    QVERIFY(mParser->findStatementOf(file, "c", 23));
    QVERIFY(mParser->findStatementOf(file, "vv", 25));
    QVERIFY(mParser->findStatementOf(file, "value", 26));
}

void TestCppParser::test_find_statement_of_inline_namespace()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace std {",
            "  inline namespace __cxx11 {",
            "    class basic_string {};",
            "    using string = basic_string;",
            "    int inlineValue;",
            "  }",
            "  basic_string value;",
            "}",
            "std::string text;",
            "namespace std { namespace literals {",
            "  inline namespace string_view_literals { void f(); }",
            "} }",
            "namespace { int anonymousValue; }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement stdNamespace = mParser->findStatementOf("text.cpp", QStringList{"std"}, 0);
    QVERIFY(stdNamespace);
    QCOMPARE(stdNamespace->fullName, "std");
    PStatement inlineNamespace = mParser->findStatement("std::__cxx11");
    QVERIFY(inlineNamespace);
    QCOMPARE(inlineNamespace->fullName, "std::__cxx11");
    PStatement basicString = mParser->findStatement("std::basic_string");
    QVERIFY(basicString);
    QCOMPARE(basicString->fullName, "std::__cxx11::basic_string");
    QCOMPARE(mParser->findStatement("std::__cxx11::basic_string"), basicString);
    QCOMPARE(mParser->findStatementOf("text.cpp", {"std", "::", "basic_string"}, 0),
             basicString);
    QCOMPARE(mParser->findStatementOf("text.cpp", QStringList{"basic_string"}, 5),
             basicString);
    PStatement value = mParser->findStatementOf("text.cpp", QStringList{"value"}, 5);
    QVERIFY(value);
    QCOMPARE(getKindOfStatement(value), StatementKind::GlobalVariable);
    PStatement inlineValue = mParser->findStatement("std::inlineValue");
    QVERIFY(inlineValue);
    QCOMPARE(inlineValue->fullName, "std::__cxx11::inlineValue");
    PStatement stringAlias = mParser->findStatement("std::string");
    QVERIFY(stringAlias);
    QCOMPARE(stringAlias->fullName, "std::__cxx11::string");
    QCOMPARE(mParser->findStatementOf("text.cpp", {"std", "::", "string"}, 0),
             stringAlias);
    QVERIFY(statementHasDefinition(stringAlias));
    PStatement literalFunction = mParser->findStatement("std::literals::f");
    QVERIFY(literalFunction);
    QCOMPARE(literalFunction->fullName, "std::literals::string_view_literals::f");
    PStatement anonymousValue = mParser->findStatement("anonymousValue");
    QVERIFY(anonymousValue);
    QCOMPARE(anonymousValue->fullName, "anonymousValue");
}

void TestCppParser::test_nested_namespace_scope_parentage()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace A {",
            "namespace B {",
            "namespace C {",
            "namespace D {",
            "class X {};",
            "}",
            "int cValue;",
            "}",
            "int bValue;",
            "}",
            "int aValue;",
            "}",
            "int global;"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    const QStringList expectedScopes{
        "A", "A::B", "A::B::C", "A::B::C::D", "A::B::C::D::X",
        "A::B::C::D", "A::B::C", "A::B::C", "A::B", "A::B",
        "A", "A", ""
    };
    for (int line = 0; line < expectedScopes.count(); ++line) {
        PStatement scope = mParser->findScopeStatement("text.cpp", line);
        QCOMPARE(scope ? scope->fullName : QString(), expectedScopes[line]);
    }
}

void TestCppParser::test_variable_template_specialization_name()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace Detail { struct Identity {}; }",
            "template<typename A, typename B> struct is_same;",
            "template<typename T> struct HashTable {",
            "    using key_type = T;",
            "    template<typename Arg, typename DArg = key_type, typename = Detail::Identity>",
            "    static constexpr bool is_key = false;",
            "    template<typename Arg>",
            "    static constexpr bool is_key<Arg, key_type, Detail::Identity> = is_same<Arg, key_type>::value;",
            "};"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    PStatement specialization;
    for (const PStatement &statement : mParser->statementList().statements()) {
        if (statement->fullName == "HashTable::is_key<Arg,key_type,Detail::Identity>")
            specialization = statement;
    }

    PStatement keyType = mParser->findStatement("HashTable::key_type");
    QVERIFY(keyType);
    QCOMPARE(keyType->kind, StatementKind::Alias);
    PStatement primary = mParser->findStatement("HashTable::is_key");
    QVERIFY(primary);
    QCOMPARE(primary->kind, StatementKind::Variable);
    QVERIFY(specialization);
    QCOMPARE(specialization->kind, StatementKind::Variable);
}

void TestCppParser::test_member_alias_template_specialization()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace std {",
            "    template<bool> struct __conditional {",
            "        template<typename T, typename> using type = T;",
            "    };",
            "    template<> struct __conditional<false> {",
            "        template<typename, typename U> using type = U;",
            "    };",
            "    template<bool C, typename I, typename E>",
            "    using __conditional_t = typename __conditional<C>::template type<I, E>;",
            "}",
            "struct Value {};"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement value = mParser->findStatement("Value");
    QVERIFY(value);
    QCOMPARE(mParser->findTypeDefinitionOf(
                 "text.cpp", "std::__conditional_t<false,int,Value>", {}), value);
}

void TestCppParser::test_unknown_non_type_default_preserves_alias_resolution()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename...> struct And {};",
            "template<bool> struct BoolConstant {};",
            "template<typename T> using CacheDefault = BoolConstant<!And<T>::value>;",
            "template<bool> struct Traits {};",
            "template<typename T, typename TTraits = Traits<CacheDefault<T>::value>>",
            "struct HashTable : And<T> { using key_type = T; };",
            "template<typename T> using Unordered = HashTable<T>;",
            "struct Value {};"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement value = mParser->findStatement("Value");
    QVERIFY(value);
    QCOMPARE(mParser->findTypeDefinitionOf("text.cpp", "Unordered<Value>::key_type", {}), value);
}

void TestCppParser::test_using_declaration_function_overload_set()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Result {};",
            "namespace std { template<typename T> Result remove(T, T); }",
            "int remove(const char *);",
            "namespace std { using ::remove; }",
            "auto value = std::remove(1, 2);"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    QStringList expression{"std", "::", "remove", "(", "1", ",", "2", ")"};
    EvalResult result = mParser->evalExpression("text.cpp", expression, {});
    QVERIFY(result.isValid());
    QCOMPARE(result.type.declaration.lock(), mParser->findStatement("Result"));
}

void TestCppParser::test_overload_rejects_unrelated_class_specialization()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace std {",
            "    template<typename T> struct iterator {};",
            "    template<typename T> struct deque_iterator {};",
            "    template<typename II, typename OI> OI copy(II, II, OI);",
            "    template<typename T> deque_iterator<T> copy(deque_iterator<T>, deque_iterator<T>, deque_iterator<T>);",
            "}",
            "std::iterator<int> it;",
            "std::deque_iterator<int> dequeIt;",
            "auto result = std::copy(it, it, it);",
            "auto dequeResult = std::copy(dequeIt, dequeIt, dequeIt);"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    for (const QString &name : {QString("it"), QString("dequeIt")}) {
        QStringList expression{"std", "::", "copy", "(", name, ",", name, ",", name, ")"};
        EvalResult result = mParser->evalExpression("text.cpp", expression, {});
        QVERIFY(result.isValid());
        PStatement expected = mParser->findStatement(
                    name == "dequeIt" ? "std::deque_iterator" : "std::iterator");
        QCOMPARE(result.type.declaration.lock(), expected);
        QCOMPARE(result.type.templateArguments.count(), 1);
        QVERIFY(result.type.templateArguments.first().type);
        QCOMPARE(result.type.templateArguments.first().type->qualifiedName, QString("int"));
    }
}

void TestCppParser::test_empty_template_argument_list()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename...> using __void_t = void;"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    CppTypeResolver resolver(mParser->statementList());
    TypeInfo type;
    type.qualifiedName = "__void_t";
    type.qualifiedNameText = "__void_t<>";
    TypeInfo resolved = resolver.resolve(type, {}, mParser->findStatement("std"));
    QCOMPARE(resolved.qualifiedName, QString("void"));
}

void TestCppParser::test_const_template_parameter_substitution()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename T> struct Pair {};",
            "template<typename T> struct Map { using value_type = Pair<const T>; };"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    CppTypeResolver resolver(mParser->statementList());
    TypeInfo type;
    type.qualifiedName = "Map<int>::value_type";
    type.qualifiedNameText = "Map<int>::value_type";
    TypeInfo resolved = resolver.resolve(type);
    QCOMPARE(resolved.qualifiedName, QString("Pair"));
    QCOMPARE(resolved.templateArguments.count(), 1);
    QVERIFY(resolved.templateArguments.first().type);
    QCOMPARE(resolved.templateArguments.first().type->qualifiedName, QString("int"));
    QCOMPARE(resolved.templateArguments.first().type->toString(), QString("const int"));
}

void TestCppParser::test_auto_reference_collapsing()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "struct Value {};",
            "Value &get();",
            "auto &value = get();",
            "auto &&other = get();"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    for (const QString &name : {QString("value"), QString("other")}) {
        PStatement statement = mParser->findStatementOf("text.cpp", name, 3);
        QVERIFY(statement);
        const TypeInfo &type = std::dynamic_pointer_cast<Statement>(statement)->typeInfo;
        QVERIFY(!type.derivations.isEmpty());
        QCOMPARE(type.derivations.first().kind, TypeDerivationKind::LvalueReference);
    }
}

void TestCppParser::test_void_t_detection_failure()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "template<typename...> using __void_t = void;",
            "template<typename T, typename = __void_t<>> struct Traits { };",
            "template<typename T> struct Traits<T, __void_t<typename T::missing>> { using type = int; };"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    CppTypeResolver resolver(mParser->statementList());
    TypeInfo type;
    type.qualifiedName = "Traits<int>";
    type.qualifiedNameText = "Traits<int>";
    TypeInfo resolved = resolver.resolve(type);
    QCOMPARE(resolved.qualifiedName, QString("Traits"));
    QCOMPARE(resolved.declaration.lock(), mParser->findStatement("Traits"));
}

#ifdef REDPANDA_PARSER_STDLIB_TEST
void TestCppParser::test_bits_stdcpp_not_rescanned_on_stable_reparse()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <bits/stdc++.h>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));

    CppParser::parseFileBlocking(mParser, main, true, "");
    QCOMPARE(mParser->preprocessorEventCountsForTest(bitsStdcpp).scan, 1);
    int statementCount = mParser->statementList().count();

    CppParser::parseFileBlocking(mParser, main, true, "");
    CppPreprocessor::EventCounts counts = mParser->preprocessorEventCountsForTest(bitsStdcpp);
    QCOMPARE(counts.scan, 1);
    QVERIFY(counts.replay > 0);
    QCOMPARE(mParser->statementList().count(), statementCount);
}

void TestCppParser::test_standard_library_string()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <string>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));

    CppParser::parseFileBlocking(mParser, main, true, "");
    int preprocessedReferences = 0;
    bool inlineCxx11Preprocessed = false;
    for (const QString &line : mParser->preprocessedForTest()) {
        if (line.contains("basic_string"))
            ++preprocessedReferences;
        if (line.contains("inline namespace __cxx11"))
            inlineCxx11Preprocessed = true;
    }
    QVERIFY(preprocessedReferences > 0);
    QVERIFY(inlineCxx11Preprocessed);
    bool forwardDeclarationTokens = false;
    bool definitionTokens = false;
    bool inlineCxx11Tokens = false;
    int definitionTokenIndex = -1;
    const CppTokenizer::TokenList &tokens = mParser->tokenizerTokensForTest();
    for (int i = 0; i < tokens.count(); ++i) {
        const CppTokenizer::Token &token = tokens[i];
        if (token.text == "basic_string") {
            if (token.fileName.endsWith("bits/stringfwd.h", Qt::CaseInsensitive))
                forwardDeclarationTokens = true;
            if (i > 0 && i + 1 < tokens.count() && tokens[i - 1].text == "class"
                    && tokens[i + 1].text == "{"
                    && token.fileName.endsWith("bits/basic_string.h", Qt::CaseInsensitive)) {
                definitionTokens = true;
                definitionTokenIndex = i;
            }
        }
        if (i > 1 && token.text == "__cxx11" && tokens[i - 1].text == "namespace"
                && tokens[i - 2].text == "inline"
                && token.fileName.endsWith("bits/c++config.h", Qt::CaseInsensitive)) {
            inlineCxx11Tokens = true;
        }
    }
    QVERIFY(forwardDeclarationTokens);
    QVERIFY(definitionTokens);
    QVERIFY(inlineCxx11Tokens);
    PStatement basicString = mParser->findStatement("std::__cxx11::basic_string");
    QVERIFY(basicString);
    QVERIFY(statementHasDefinition(basicString));
    QVERIFY(basicString->definition.fileName.endsWith("bits/basic_string.h", Qt::CaseInsensitive));
    QCOMPARE(mParser->findStatementOf(
                 main, {"std", "::", "__cxx11", "::", "basic_string"}, 0), basicString);
    QVERIFY(mParser->findStatement("std::string"));
    PStatement stringAlias = mParser->findStatementOf(main, {"std", "::", "string"}, 0);
    QVERIFY(stringAlias);
    QVERIFY(statementHasDefinition(stringAlias));
    QVERIFY(mParser->findStatement("std::basic_string"));
    QCOMPARE(mParser->findStatementOf(main, {"std", "::", "basic_string"}, 0), basicString);
    PStatement append = mParser->findStatement("std::basic_string::_M_append");
    QVERIFY(append);
    PStatement appendParent = append->parentScope.lock();
    QVERIFY2(appendParent == basicString,
             QString("%1 has parent %2; expected %3")
             .arg(append->fullName,
                  appendParent ? appendParent->fullName : QString("<null>"),
                  basicString->fullName).toUtf8().constData());
    QString basicStringTcc = QFileInfo(basicString->definition.fileName).dir().filePath("basic_string.tcc");
    PParsedFileInfo tccInfo = mParser->findFileInfo(basicStringTcc);
    QVERIFY(tccInfo);
    int appendDefinitions = 0;
    for (const std::weak_ptr<AbstractStatement> &weak : tccInfo->statements()) {
        PStatement statement = weak.lock();
        if (!statement || statement->command != "_M_append")
            continue;
        ++appendDefinitions;
        PStatement parent = statement->parentScope.lock();
        QVERIFY2(parent == basicString,
                 QString("%1 at line %2 has parent %3; expected %4")
                 .arg(statement->fullName)
                 .arg(statement->definition.range.start.line)
                 .arg(parent ? parent->fullName : QString("<null>"), basicString->fullName)
                 .toUtf8().constData());
    }
    QVERIFY(appendDefinitions > 0);
    PStatement replaceDispatch = mParser->findStatement("std::basic_string::_M_replace_dispatch");
    QVERIFY(replaceDispatch);
    QCOMPARE(replaceDispatch->kind, StatementKind::Function);
    QCOMPARE(replaceDispatch->parentScope.lock(), basicString);
    QStringList emptyVariables;
    for (const std::weak_ptr<AbstractStatement> &weak : tccInfo->statements()) {
        PStatement statement = weak.lock();
        QVERIFY(statement);
        if (statement->command.isEmpty() && statement->kind == StatementKind::Variable
                && statement->scope == StatementScope::Global) {
            emptyVariables.append(QString("fullName %1, definition line %2")
                                  .arg(statement->fullName)
                                  .arg(statement->definition.range.start.line));
        }
    }
    QVERIFY2(emptyVariables.isEmpty(), emptyVariables.join("; ").toUtf8().constData());
    bool hasWcharExplicitInstantiation = false;
    for (const QString &line : mParser->preprocessedForTest()) {
        if (line.contains("extern template class basic_string<wchar_t>")) {
            hasWcharExplicitInstantiation = true;
            break;
        }
    }
    if (hasWcharExplicitInstantiation) {
        PStatement wcharString = mParser->findStatement("std::basic_string<wchar_t>");
        QVERIFY(wcharString);
        QCOMPARE(wcharString->kind, StatementKind::Class);
    }
    PStatement size = mParser->findStatement("std::basic_string::size");
    QVERIFY(size);
    QCOMPARE(mParser->findStatementOf(
                 main, {"std", "::", "basic_string", "::", "size"}, 0), size);
}

void TestCppParser::test_standard_library_header_context()
{
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    QDir includeDir = QFileInfo(bitsStdcpp).dir();
    QVERIFY(includeDir.cdUp());
    if (!QFileInfo::exists(includeDir.filePath("string")))
        QVERIFY(includeDir.cdUp());
    QString stringFile = includeDir.filePath("string");
    QString basicStringTcc = includeDir.filePath("bits/basic_string.tcc");
    QVERIFY(QFileInfo::exists(stringFile));
    QVERIFY(QFileInfo::exists(basicStringTcc));

    CppParser::parseFileBlocking(mParser, basicStringTcc, false, stringFile);

    QCOMPARE(mParser->preprocessorEventCountsForTest(basicStringTcc).scan, 1);
    QVERIFY(mParser->hasReplayRecord(stringFile));
    QVERIFY(!mParser->hasReplayRecord(basicStringTcc));
    QVERIFY(mParser->isFileParsedInContext(basicStringTcc, stringFile));
    QVERIFY(mParser->findStatementOf(basicStringTcc, QStringList{"std"}, 50));
}

void TestCppParser::test_standard_library_nested_include_guard_visibility()
{
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    QDir includeDir = QFileInfo(bitsStdcpp).dir();
    QVERIFY(includeDir.cdUp());
    if (!QFileInfo::exists(includeDir.filePath("string")))
        QVERIFY(includeDir.cdUp());
    QString stringFile = includeDir.filePath("string");
    QVERIFY(QFileInfo::exists(stringFile));
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <bits/stdc++.h>"}));

    CppParser::parseFileBlocking(mParser, main, true, "");
    PParsedFileInfo stringInfo = mParser->findFileInfo(stringFile);
    QVERIFY(stringInfo);
    QVERIFY(stringInfo->includeProtected());
    QCOMPARE(mParser->preprocessorEventCountsForTest(stringFile).scan, 1);
    QVERIFY(mParser->isLineVisible(stringFile, 40));

    CppParser::parseFileBlocking(mParser, bitsStdcpp, false, main);
    QVERIFY(mParser->isLineVisible(stringFile, 40));
    CppParser::parseFileBlocking(mParser, stringFile, false, main);
    QVERIFY(mParser->isLineVisible(stringFile, 40));
}

void TestCppParser::test_standard_library_header_guard_context_visibility_data()
{
    QTest::addColumn<QByteArray>("headerName");
    const QByteArrayList cHeaders{
        "assert.h", "complex.h", "ctype.h", "errno.h", "fenv.h", "float.h",
        "inttypes.h", "iso646.h", "limits.h", "locale.h", "math.h", "setjmp.h",
        "signal.h", "stdalign.h", "stdarg.h", "stdatomic.h", "stdbool.h",
        "stddef.h", "stdint.h", "stdio.h", "stdlib.h", "stdnoreturn.h",
        "string.h", "tgmath.h", "threads.h", "time.h", "uchar.h", "wchar.h",
        "wctype.h"
    };
    const QByteArrayList cppHeaders{
        "algorithm", "any", "array", "atomic", "bitset", "cassert", "cctype",
        "cerrno", "cfenv", "cfloat", "charconv", "chrono", "cinttypes",
        "ciso646", "climits", "clocale", "cmath", "codecvt", "complex",
        "condition_variable", "csetjmp", "csignal", "cstdalign", "cstdarg",
        "cstdbool", "cstddef", "cstdint", "cstdio", "cstdlib", "cstring",
        "ctgmath", "ctime", "cuchar", "cwchar", "cwctype", "deque", "exception",
        "execution", "filesystem", "forward_list", "fstream", "functional",
        "future", "initializer_list", "iomanip", "ios", "iosfwd", "iostream",
        "istream", "iterator", "limits", "list", "locale", "map", "memory",
        "memory_resource", "mutex", "new", "numeric", "optional", "ostream",
        "queue", "random", "ratio", "regex", "scoped_allocator", "set",
        "shared_mutex", "sstream", "stack", "stdexcept", "streambuf", "string",
        "string_view", "strstream", "system_error", "thread", "tuple",
        "type_traits", "typeindex", "typeinfo", "unordered_map", "unordered_set",
        "utility", "valarray", "variant", "vector"
    };
    for (const QByteArray &header : cHeaders)
        QTest::addRow("C/%s", header.constData()) << header;
    for (const QByteArray &header : cppHeaders)
        QTest::addRow("C++/%s", header.constData()) << header;
}

void TestCppParser::test_standard_library_header_guard_context_visibility()
{
    QFETCH(QByteArray, headerName);
    const QString header = QString::fromLatin1(headerName);
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    const QString include = "<" + header + ">";
    const QString headerFile = mParser->getHeaderFileName(bitsStdcpp, include);
    if (headerFile.isEmpty())
        QSKIP("Header is not available in this toolchain");
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include " + include}));

    CppParser::parseFileBlocking(mParser, headerFile, false, main);

    QVERIFY2(mParser->isFileParsedInContext(headerFile, main), headerName.constData());
    PParsedFileInfo info = mParser->findFileInfo(headerFile);
    QVERIFY(info);
    const int guardDefineLine = includeGuardDefineLine(headerFile);
    if (guardDefineLine < 0)
        return;
    if (headerName == "stdarg.h") {
        // GCC's wrapper includes the next stdarg.h first, which defines
        // _STDARG_H and makes this wrapper's guarded body inactive.
        QVERIFY(!mParser->isLineVisible(headerFile, guardDefineLine));
        return;
    }
    QVERIFY2(mParser->isLineVisible(headerFile, guardDefineLine), headerName.constData());
    if (info->includeProtected())
        QCOMPARE(mParser->preprocessorEventCountsForTest(headerFile).scan, 1);
}

void TestCppParser::test_standard_library_direct_c_header_context()
{
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    QString stdioFile = mParser->getHeaderFileName(bitsStdcpp, "<stdio.h>");
    QVERIFY(!stdioFile.isEmpty());
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <stdio.h>"}));

    CppParser::parseFileBlocking(mParser, stdioFile, false, main);

    QVERIFY(mParser->isFileParsedInContext(stdioFile, main));
    QCOMPARE(mParser->preprocessorEventCountsForTest(stdioFile).scan, 1);
    PParsedFileInfo stdioInfo = mParser->findFileInfo(stdioFile);
    QVERIFY(stdioInfo);
    QVERIFY(stdioInfo->includeProtected());
    const int guardDefineLine = includeGuardDefineLine(stdioFile);
    QVERIFY(guardDefineLine >= 0);
    QVERIFY(mParser->isLineVisible(stdioFile, guardDefineLine));

    QFile source(stdioFile);
    QVERIFY(source.open(QIODevice::ReadOnly));
    const QStringList sourceLines = QString::fromUtf8(source.readAll()).split('\n');
    source.close();
    int structLine = -1;
    int structEndLine = -1;
    for (int line = 0; line < sourceLines.count(); ++line) {
        if (structLine < 0 && sourceLines[line].contains("struct _iobuf"))
            structLine = line;
        if (structLine >= 0 && sourceLines[line].trimmed() == "};") {
            structEndLine = line;
            break;
        }
    }
    QVERIFY(structLine >= 0);
    QVERIFY(structEndLine > structLine);
    QSet<int> tokenLines;
    for (const CppTokenizer::Token &token : mParser->tokenizerTokensForTest()) {
        if (token.fileName == stdioFile && token.kind != CppTokenizer::TokenKind::FileBoundary
                && token.kind != CppTokenizer::TokenKind::FileEnd) {
            tokenLines.insert(token.position.line);
        }
    }
    for (int line = structLine; line <= structEndLine; ++line) {
        const QString sourceLine = sourceLines[line].trimmed();
        if (sourceLine.isEmpty() || sourceLine.startsWith('#'))
            continue;
        QByteArray message = QString("%1:%2").arg(stdioFile).arg(line).toUtf8();
        QVERIFY2(mParser->isLineVisible(stdioFile, line) == tokenLines.contains(line),
                 message.constData());
    }
}

void TestCppParser::test_standard_library_front_append()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <string>",
        "#include <vector>",
        "",
        "int main() {",
        "    std::vector<std::string> lst;",
        "    lst.front().append(\"llala\");",
        "    std::cout<<lst.size()<<std::endl;",
        "    return 0;",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, true, "");

    PStatement append = mParser->findStatement("std::basic_string::append");
    QVERIFY(append);
    QElapsedTimer timer;
    timer.start();
    PStatement variable = mParser->findStatementOf(main, "lst", 5);
    qWarning() << "STDLIB_TEST_TIMING listLookupNs" << timer.nsecsElapsed();
    QVERIFY(variable);
    QCOMPARE(variable->command, "lst");
    timer.restart();
    PStatement front = mParser->findStatementOf(main, "lst.front()", 5);
    qWarning() << "STDLIB_TEST_TIMING frontLookupNs" << timer.nsecsElapsed();
    QVERIFY(front);
    QCOMPARE(front->command, "front");
    timer.restart();
    PStatement appendReference = mParser->findStatementOf(main, "lst.front().append", 5);
    qWarning() << "STDLIB_TEST_TIMING appendReferenceLookupNs" << timer.nsecsElapsed();
    QVERIFY(appendReference);
    QCOMPARE(appendReference->command, "append");
    QCOMPARE(appendReference->parentScope.lock(), append->parentScope.lock());
}

void TestCppParser::test_standard_library_vector_subscript()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "",
        "int main() {",
        "    std::vector<std::string> lst;",
        "    std::string s;",
        "    lst[0].size();",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));

    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatement scope = mParser->findScopeStatement(main, 5);
    QVERIFY(scope);
    QCOMPARE(scope->fullName, "main");
    QStringList expression{"lst", "[", "0", "]"};
    EvalResult result = mParser->evalExpression(main, expression, scope);
    PStatement basicString = mParser->findStatement("std::basic_string");
    QVERIFY(basicString);
    QCOMPARE(result.type.declaration.lock(), basicString);
    QCOMPARE(result.instance.declaration, basicString);
    QCOMPARE(result.instance.type.declaration.lock(), basicString);
    QVERIFY(!result.type.derivations.isEmpty());
    QCOMPARE(result.type.derivations.first().kind, TypeDerivationKind::LvalueReference);
    QCOMPARE(mParser->findStatementOf(main, {"lst", "[", "0", "]", ".", "size"}, 5),
             mParser->findStatement("std::basic_string::size"));
}

void TestCppParser::test_standard_library_make_shared_default()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "",
        "int main() {",
        "    std::vector<std::string> lst;",
        "    auto p = std::make_shared<std::string>();",
        "    p->size();",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, true, "");

    PStatement basicString = mParser->findStatement("std::basic_string");
    QVERIFY(basicString);
    QCOMPARE(mParser->findTypeDefinitionOf(
                 main, "std::enable_if<true, std::string>::type", {}), basicString);
    QCOMPARE(mParser->findTypeDefinitionOf(
                 main,
                 "std::enable_if_t<!std::is_array<std::string>::value, std::string>", {}),
             basicString);
    PStatement sharedPtr = mParser->findStatement("std::shared_ptr");
    QVERIFY(sharedPtr);
    if (mParser->cppVersion() >= CppVersion::Cpp20) {
        QCOMPARE(mParser->statementList().cppVersion(), mParser->cppVersion());
        QVERIFY(mParser->findStatement("std::is_array_v"));
        QVERIFY(mParser->findStatement("std::_NonArray"));
        QCOMPARE(mParser->findTypeDefinitionOf(main, "std::_NonArray<std::string>", {}), basicString);
        QVERIFY(!mParser->findTypeDefinitionOf(main, "std::_BoundedArray<std::string>", {}));
    }
    PStatement scope = mParser->findScopeStatement(main, 5);
    QVERIFY(scope);
    QStringList factoryExpression{
        "std", "::", "make_shared", "<", "std", "::", "string", ">", "(", ")"};
    EvalResult factory = mParser->evalExpression(main, factoryExpression, scope);
    QCOMPARE(factory.type.declaration.lock(), sharedPtr);
    QStringList variableExpression{"p"};
    EvalResult variable = mParser->evalExpression(main, variableExpression, scope);
    QCOMPARE(variable.type.declaration.lock(), sharedPtr);
    QStringList arrowExpression{"p", "->"};
    EvalResult arrow = mParser->evalExpression(main, arrowExpression, scope);
    QCOMPARE(arrow.type.declaration.lock(), basicString);
    QCOMPARE(mParser->findStatementOf(main, {"p", "->", "size"}, 5),
             mParser->findStatement("std::basic_string::size"));
}

void TestCppParser::test_standard_library_expression_chains()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "",
        "int main() {",
        "    std::vector<std::string> lst;",
        "    std::deque<std::string> deq;",
        "    std::list<std::string> linked;",
        "    std::map<std::string, std::string> mapping;",
        "    std::array<std::string, 2> arr;",
        "    lst.front().size();",
        "    lst.back().size();",
        "    lst.at(0).size();",
        "    deq[0].size();",
        "    linked.front().size();",
        "    mapping[\"key\"].size();",
        "    arr[0].size();",
        "    auto shared = std::make_shared<std::string>(\"shared\");",
        "    shared->size();",
        "    (*shared).size();",
        "    auto unique = std::make_unique<std::string>(\"unique\");",
        "    unique->size();",
        "    auto it = lst.begin();",
        "    it->size();",
        "    (*it).size();",
        "    while (++it != lst.end()) {",
        "        it->size();",
        "    }",
        "    for (auto &s : lst) {",
        "        s.size();",
        "    }",
        "    for (auto i = lst.begin(); i != lst.end(); ++i) {",
        "        i->size();",
        "    }",
        "    auto found = std::find_if(lst.begin(), lst.end(), [](const std::string &s) { return s.empty(); });",
        "    found->size();",
        "    auto output = std::transform(lst.begin(), lst.end(), lst.begin(), [](const std::string &s) { return s; });",
        "    output->size();",
        "    std::vector<bool> flags;",
        "    flags[0].flip();",
        "    for (const auto &s : lst) {",
        "        s.size();",
        "    }",
        "    for (const auto &entry : mapping) {",
        "        entry.first.size();",
        "        entry.second.size();",
        "    }",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, true, "");

    PStatement stringSize = mParser->findStatement("std::basic_string::size");
    QVERIFY(stringSize);
    PStatement iteratorScope = mParser->findScopeStatement(main, 20);
    QVERIFY(iteratorScope);
    QStringList beginExpression{"lst", ".", "begin", "(", ")"};
    QStringList iteratorExpression{"it"};
    EvalResult beginResult = mParser->evalExpression(main, beginExpression, iteratorScope);
    EvalResult iteratorResult = mParser->evalExpression(main, iteratorExpression, iteratorScope);
    QCOMPARE(beginResult.type.qualifiedName, "__gnu_cxx::__normal_iterator");
    QCOMPARE(iteratorResult.type.toString(), beginResult.type.toString());
    PStatement foundScope = mParser->findScopeStatement(main, 33);
    QVERIFY(foundScope);
    QStringList foundExpression{"found"};
    EvalResult foundResult = mParser->evalExpression(main, foundExpression, foundScope);
    QCOMPARE(foundResult.type.toString(), beginResult.type.toString());
    QList<PStatement> findIfOverloads = mParser->getListOfFunctions(main, "std::find_if", 32);
    bool hasClassicFindIf = false;
    for (const PStatement &overload : findIfOverloads)
        hasClassicFindIf |= std::dynamic_pointer_cast<Statement>(overload)->normalizedArgs == "(@T0,@T0,@T1)";
    QVERIFY(hasClassicFindIf);
    QStringList listFrontExpression{"linked", ".", "front", "(", ")"};
    EvalResult listFrontResult = mParser->evalExpression(
                main, listFrontExpression, mParser->findScopeStatement(main, 12));
    PStatement basicString = mParser->findStatement("std::basic_string");
    QCOMPARE(listFrontResult.type.declaration.lock(), basicString);
    QCOMPARE(listFrontResult.instance.declaration, basicString);
    QVERIFY(!listFrontResult.type.derivations.isEmpty());
    QCOMPARE(listFrontResult.type.derivations.first().kind, TypeDerivationKind::LvalueReference);
    QStringList sharedDereference{"(", "*", "shared", ")"};
    EvalResult sharedResult = mParser->evalExpression(
                main, sharedDereference, mParser->findScopeStatement(main, 17));
    QCOMPARE(sharedResult.type.declaration.lock(), basicString);
    QCOMPARE(sharedResult.instance.declaration, basicString);
    QStringList iteratorDereference{"(", "*", "it", ")"};
    EvalResult dereferencedIterator = mParser->evalExpression(
                main, iteratorDereference, mParser->findScopeStatement(main, 22));
    QCOMPARE(dereferencedIterator.type.declaration.lock(), basicString);
    QCOMPARE(dereferencedIterator.instance.declaration, basicString);
    auto expectStringSize = [&](int line, const QStringList &expression) {
        PStatement scope = mParser->findScopeStatement(main, line);
        QVERIFY2(scope, expression.join("").toUtf8().constData());
        PStatement statement = mParser->findStatementOf(main, expression, line);
        QVERIFY2(statement, expression.join("").toUtf8().constData());
        QCOMPARE(statement, stringSize);
    };
    expectStringSize(8, {"lst", ".", "front", "(", ")", ".", "size"});
    expectStringSize(9, {"lst", ".", "back", "(", ")", ".", "size"});
    expectStringSize(10, {"lst", ".", "at", "(", "0", ")", ".", "size"});
    expectStringSize(11, {"deq", "[", "0", "]", ".", "size"});
    expectStringSize(12, {"linked", ".", "front", "(", ")", ".", "size"});
    expectStringSize(13, {"mapping", "[", "\"key\"", "]", ".", "size"});
    expectStringSize(14, {"arr", "[", "0", "]", ".", "size"});
    expectStringSize(16, {"shared", "->", "size"});
    expectStringSize(17, {"(", "*", "shared", ")", ".", "size"});
    expectStringSize(19, {"unique", "->", "size"});
    expectStringSize(21, {"it", "->", "size"});
    expectStringSize(22, {"(", "*", "it", ")", ".", "size"});
    expectStringSize(24, {"it", "->", "size"});
    expectStringSize(27, {"s", ".", "size"});
    expectStringSize(30, {"i", "->", "size"});
    expectStringSize(33, {"found", "->", "size"});
    expectStringSize(35, {"output", "->", "size"});
    expectStringSize(39, {"s", ".", "size"});
    PStatement entryScope = mParser->findScopeStatement(main, 42);
    QVERIFY(entryScope);
    QStringList entryExpression{"entry"};
    EvalResult entryResult = mParser->evalExpression(main, entryExpression, entryScope);
    PStatement entryDeclaration = entryResult.type.declaration.lock();
    QVERIFY(entryResult.type.toString().startsWith("const std::pair<"));
    QVERIFY(entryDeclaration);
    QCOMPARE(entryDeclaration->fullName, QString("std::pair"));
    expectStringSize(42, {"entry", ".", "first", ".", "size"});
    expectStringSize(43, {"entry", ".", "second", ".", "size"});
    QStringList flagElementExpression{"flags", "[", "0", "]"};
    EvalResult flagElement = mParser->evalExpression(
                main, flagElementExpression, mParser->findScopeStatement(main, 37));
    PStatement flip = mParser->findStatementOf(
                main, {"flags", "[", "0", "]", ".", "flip"}, 37);
    QVERIFY2(flip, flagElement.type.toString().toUtf8().constData());
    QCOMPARE(flip, mParser->findStatement("std::_Bit_reference::flip"));
}

void TestCppParser::test_standard_library_trace_edge_cases()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "",
        "struct Record {",
        "    int value;",
        "};",
        "",
        "int main() {",
        "    auto sumRow = [](const std::vector<int> &row) { return std::accumulate(row.begin(), row.end(), 0); };",
        "    auto compare = [](const auto &left, const auto &right) { return left.size() < right.size(); };",
        "    auto readRecord = [](const Record &record) { return record.value; };",
        "    auto readShared = [](const std::shared_ptr<Record> &record) { return record->value; };",
        "    int outer = 0;",
        "    auto withLocal = [&](const Record &record) {",
        "        int result = record.value;",
        "        return result + outer;",
        "    };",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, true, "");

    auto check = [&](int line, QStringList expression, const QVector<int> &identifierIndexes) {
        PStatement scope = mParser->findScopeStatement(main, line);
        QVERIFY(scope);
        ExpressionEvalTrace trace;
        mParser->evalExpression(main, expression, scope, &trace);
        QCOMPARE(trace.size(), identifierIndexes.size());
        for (int i = 0; i < trace.size(); ++i) {
            QCOMPARE(trace[i].tokenIndex, identifierIndexes[i]);
            PStatement expected = mParser->findStatementOf(main, expression, line);
            QVERIFY(trace[i].statement);
            if (i + 1 == trace.size())
                QCOMPARE(trace[i].statement, expected);
        }
    };
    check(7, {"row", ".", "begin"}, {0, 2});
    check(7, {"row", ".", "end"}, {0, 2});
    check(9, {"record", ".", "value"}, {0, 2});
    check(10, {"record", "->", "value"}, {0, 2});
    PStatement withLocal = mParser->findStatementOf(main, QStringList{"withLocal"}, 12);
    QVERIFY(withLocal);
    PStatement lambda = std::dynamic_pointer_cast<Statement>(withLocal)->typeInfo.declaration.lock();
    QVERIFY(lambda);
    QCOMPARE(lambda->kind, StatementKind::Lambda);
    PStatement call;
    for (const PStatement &member : mParser->membersOfStatement(lambda)) {
        if (member->kind == StatementKind::OverloadedOperator && member->command == "operator()") {
            call = member;
            break;
        }
    }
    QVERIFY(call);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(call)->typeInfo.qualifiedName, QString("int"));
    check(13, {"record", ".", "value"}, {0, 2});
    check(14, {"result"}, {0});
    check(14, {"outer"}, {0});
    PStatement genericScope = mParser->findScopeStatement(main, 8);
    QVERIFY(genericScope);
    QCOMPARE(genericScope->kind, StatementKind::OverloadedOperator);
    PStatement left = mParser->findStatementOf(main, QStringList{"left"}, 8);
    QVERIFY(left);
    QCOMPARE(left->kind, StatementKind::Parameter);
}

void TestCppParser::test_standard_library_container_eval_profile()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "",
        "int main() {",
        "    std::vector<std::string> lst;",
        "    std::deque<std::string> deq;",
        "    std::list<std::string> linked;",
        "    std::forward_list<std::string> forward;",
        "    std::array<std::string, 2> arr;",
        "    std::map<std::string, std::string> mapping;",
        "    std::multimap<std::string, std::string> multiMapping;",
        "    std::unordered_map<std::string, std::string> unorderedMapping;",
        "    std::set<std::string> ordered;",
        "    std::multiset<std::string> multiSet;",
        "    std::unordered_set<std::string> unorderedSet;",
        "    std::unordered_multiset<std::string> unorderedMultiSet;",
        "    std::stack<std::string> stack;",
        "    std::queue<std::string> queue;",
        "    std::priority_queue<std::string> priority;",
        "    std::optional<std::string> optional;",
        "    auto shared = std::make_shared<std::string>();",
        "    auto unique = std::make_unique<std::string>();",
        "    auto it = lst.begin();",
        "    while (++it != lst.end()) {",
        "        it->size();",
        "    }",
        "    for (auto &s : lst) {",
        "        s.size();",
        "    }",
        "    for (auto i = lst.begin(); i != lst.end(); ++i) {",
        "        i->size();",
        "    }",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatement basicString = mParser->findStatement("std::basic_string");
    QVERIFY(basicString);
    struct Case {
        const char *name;
        int line;
        QStringList expression;
    };
    const QVector<Case> cases{
        {"vector.front", 21, {"lst", ".", "front", "(", ")"}},
        {"vector.back", 21, {"lst", ".", "back", "(", ")"}},
        {"vector.at", 21, {"lst", ".", "at", "(", "0", ")"}},
        {"vector.subscript", 21, {"lst", "[", "0", "]"}},
        {"vector.iterator_deref", 21, {"(", "*", "lst", ".", "begin", "(", ")", ")"}},
        {"deque.front", 21, {"deq", ".", "front", "(", ")"}},
        {"deque.back", 21, {"deq", ".", "back", "(", ")"}},
        {"deque.at", 21, {"deq", ".", "at", "(", "0", ")"}},
        {"deque.subscript", 21, {"deq", "[", "0", "]"}},
        {"list.front", 21, {"linked", ".", "front", "(", ")"}},
        {"list.back", 21, {"linked", ".", "back", "(", ")"}},
        {"list.iterator_deref", 21, {"(", "*", "linked", ".", "begin", "(", ")", ")"}},
        {"forward_list.front", 21, {"forward", ".", "front", "(", ")"}},
        {"forward_list.iterator_deref", 21,
         {"(", "*", "forward", ".", "begin", "(", ")", ")"}},
        {"array.subscript", 21, {"arr", "[", "0", "]"}},
        {"map.subscript", 21, {"mapping", "[", "\"key\"", "]"}},
        {"map.find_second", 21,
         {"mapping", ".", "find", "(", "\"key\"", ")", "->", "second"}},
        {"multimap.find_second", 21,
         {"multiMapping", ".", "find", "(", "\"key\"", ")", "->", "second"}},
        {"unordered_map.find_second", 21,
         {"unorderedMapping", ".", "find", "(", "\"key\"", ")", "->", "second"}},
        {"set.find", 21, {"ordered", ".", "find", "(", "\"key\"", ")", "->"}},
        {"multiset.find", 21, {"multiSet", ".", "find", "(", "\"key\"", ")", "->"}},
        {"unordered_set.find", 21, {"unorderedSet", ".", "find", "(", "\"key\"", ")", "->"}},
        {"unordered_multiset.find", 21,
         {"unorderedMultiSet", ".", "find", "(", "\"key\"", ")", "->"}},
        {"stack.top", 21, {"stack", ".", "top", "(", ")"}},
        {"queue.front", 21, {"queue", ".", "front", "(", ")"}},
        {"queue.back", 21, {"queue", ".", "back", "(", ")"}},
        {"priority_queue.top", 21, {"priority", ".", "top", "(", ")"}},
        {"optional.arrow", 21, {"optional", "->"}},
        {"optional.value", 21, {"optional", ".", "value", "(", ")"}},
        {"shared_ptr.arrow", 21, {"shared", "->"}},
        {"shared_ptr.deref", 21, {"(", "*", "shared", ")"}},
        {"unique_ptr.arrow", 21, {"unique", "->"}},
        {"unique_ptr.deref", 21, {"(", "*", "unique", ")"}},
        {"while.iterator_arrow", 23, {"it", "->"}},
        {"while.iterator_deref", 23, {"(", "*", "it", ")"}},
        {"while.prefix_increment_arrow", 23, {"(", "++", "it", ")", "->"}},
        {"range_for.element", 26, {"s"}},
        {"classic_for.iterator_arrow", 29, {"i", "->"}}
    };
    QElapsedTimer timer;
    timer.start();
    qWarning() << "suite,scenario,elapsed_ns,valid,type";
    int failures = 0;
    QByteArray caseFilter = qgetenv("REDPANDA_STDLIB_CONTAINER_CASE");
    for (const Case &test : cases) {
        if (!caseFilter.isEmpty() && caseFilter != test.name)
            continue;
        PStatement scope = mParser->findScopeStatement(main, test.line);
        QVERIFY2(scope, test.name);
        qint64 started = timer.nsecsElapsed();
        QStringList expression = test.expression;
        bool trace = qEnvironmentVariableIsSet("REDPANDA_PARSER_TRACE");
        if (trace) {
            qputenv("REDPANDA_PARSER_PROFILE_RESET_TOKENIZER", "1");
            qputenv("REDPANDA_PARSER_PROFILE_RESET_RESOLVER", "1");
        }
        EvalResult result = mParser->evalExpression(main, expression, scope);
        qWarning() << "STDLIB_CONTAINER_EVAL" << test.name
                   << timer.nsecsElapsed() - started;
        logEvalProfileCsv("STDLIB_CONTAINER_EVAL_CSV", test.name,
                          timer.nsecsElapsed() - started, result);
        if (!result.isValid() || result.type.declaration.lock() != basicString) {
            ++failures;
            qWarning() << "STDLIB_CONTAINER_INVALID" << test.name << result.type.toString();
        }
        if (test.name == QStringLiteral("range_for.element")) {
            QVERIFY(!result.type.derivations.isEmpty());
            QCOMPARE(result.type.derivations.first().kind, TypeDerivationKind::LvalueReference);
        }
    }
    QCOMPARE(failures, 0);
}

void TestCppParser::test_standard_library_algorithm_eval_profile()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "",
        "int main() {",
        "    std::vector<std::string> lst;",
        "    auto found = std::find_if(lst.begin(), lst.end(), [](const std::string &s) { return s.empty(); });",
        "    auto output = std::transform(lst.begin(), lst.end(), lst.begin(), [](const std::string &s) { return s; });",
        "    auto counted = std::count(lst.begin(), lst.end(), \"x\");",
        "    auto accumulated = std::accumulate(lst.begin(), lst.end(), std::string());",
        "    auto each = std::for_each(lst.begin(), lst.end(), [](const std::string &s) { s.size(); });",
        "    std::sort(lst.begin(), lst.end());",
        "    auto lower = std::lower_bound(lst.begin(), lst.end(), \"x\");",
        "    auto minimum = std::min_element(lst.begin(), lst.end());",
        "    auto binary = std::binary_search(lst.begin(), lst.end(), \"x\");",
        "    auto any = std::any_of(lst.begin(), lst.end(), [](const std::string &s) { return s.empty(); });",
        "    auto copied = std::copy(lst.begin(), lst.end(), lst.begin());",
        "    auto removed = std::remove(lst.begin(), lst.end(), \"x\");",
        "    auto unique = std::unique(lst.begin(), lst.end());",
        "    std::reverse(lst.begin(), lst.end());",
        "    auto inserted = lst.insert(lst.begin(), \"x\");",
        "    auto erased = lst.erase(lst.begin());",
        "    std::stable_sort(lst.begin(), lst.end());",
        "    std::partial_sort(lst.begin(), lst.begin() + 1, lst.end());",
        "    std::nth_element(lst.begin(), lst.begin() + 1, lst.end());",
        "    auto reduced = std::reduce(lst.begin(), lst.end(), 0);",
        "    auto product = std::inner_product(lst.begin(), lst.end(), lst.begin(), 0);",
        "    auto countedIf = std::count_if(lst.begin(), lst.end(), [](const std::string &s) { return s.empty(); });",
        "    std::shuffle(lst.begin(), lst.end(), std::mt19937());",
        "    auto replaced = std::replace(lst.begin(), lst.end(), \"x\", \"y\");",
        "    auto rotated = std::rotate(lst.begin(), lst.begin() + 1, lst.end());",
        "    std::fill(lst.begin(), lst.end(), \"z\");",
        "    std::generate(lst.begin(), lst.end(), [] { return std::string(); });",
        "    auto moved = std::move(lst.begin(), lst.end(), lst.begin());",
        "    auto copiedN = std::copy_n(lst.begin(), 1, lst.begin());",
        "    auto foundValue = std::find(lst.begin(), lst.end(), \"x\");",
        "    auto maximum = std::max_element(lst.begin(), lst.end());",
        "    auto minmax = std::minmax_element(lst.begin(), lst.end());",
        "    std::vector<int> ints;",
        "    auto intInserted = ints.insert(ints.begin(), 1);",
        "    auto intErased = ints.erase(ints.begin());",
        "    std::stable_sort(ints.begin(), ints.end());",
        "    std::shuffle(ints.begin(), ints.end(), std::mt19937());",
        "    auto intRotated = std::rotate(ints.begin(), ints.begin() + 1, ints.end());",
        "    auto intUnique = std::unique(ints.begin(), ints.end());",
        "    auto intRemoved = std::remove(ints.begin(), ints.end(), 1);",
        "    std::replace(ints.begin(), ints.end(), 1, 2);",
        "    auto intCopied = std::copy(ints.begin(), ints.end(), ints.begin());",
        "    auto intTransformed = std::transform(ints.begin(), ints.end(), ints.begin(), [](int value) { return value; });",
        "    auto intReduced = std::reduce(ints.begin(), ints.end(), 0);",
        "    auto intProduct = std::inner_product(ints.begin(), ints.end(), ints.begin(), 0);",
        "    auto intFound = std::find(ints.begin(), ints.end(), 1);",
        "    auto intCountedIf = std::count_if(ints.begin(), ints.end(), [](int value) { return value == 1; });",
        "    auto intLower = std::lower_bound(ints.begin(), ints.end(), 1);",
        "    auto intBinary = std::binary_search(ints.begin(), ints.end(), 1);",
        "    auto intMinmax = std::minmax_element(ints.begin(), ints.end());",
        "    auto intEach = std::for_each(ints.begin(), ints.end(), [](int value) { ++value; });",
        "    std::generate(ints.begin(), ints.end(), [] { return 1; });",
        "    auto intMoved = std::move(ints.begin(), ints.end(), ints.begin());",
        "    auto intCopiedN = std::copy_n(ints.begin(), 1, ints.begin());",
        "    std::deque<std::string> deq;",
        "    auto deqInserted = deq.insert(deq.begin(), \"x\");",
        "    auto deqErased = deq.erase(deq.begin());",
        "    std::stable_sort(deq.begin(), deq.end());",
        "    std::shuffle(deq.begin(), deq.end(), std::mt19937());",
        "    auto deqUnique = std::unique(deq.begin(), deq.end());",
        "    auto deqRemoved = std::remove(deq.begin(), deq.end(), \"x\");",
        "    auto deqFound = std::find(deq.begin(), deq.end(), \"x\");",
        "    auto deqCopied = std::copy(deq.begin(), deq.end(), deq.begin());",
        "    auto deqAccumulated = std::accumulate(deq.begin(), deq.end(), std::string());",
        "    std::list<std::string> linked;",
        "    linked.sort();",
        "    linked.reverse();",
        "    linked.unique();",
        "    linked.remove(\"x\");",
        "    auto linkedFound = std::find(linked.begin(), linked.end(), \"x\");",
        "    auto linkedCopied = std::copy(linked.begin(), linked.end(), linked.begin());",
        "    auto linkedAccumulated = std::accumulate(linked.begin(), linked.end(), std::string());",
        "    std::forward_list<int> forward;",
        "    forward.sort();",
        "    forward.reverse();",
        "    forward.unique();",
        "    forward.remove(1);",
        "    auto forwardFound = std::find(forward.begin(), forward.end(), 1);",
        "    auto forwardCopied = std::copy(forward.begin(), forward.end(), forward.begin());",
        "    auto forwardAccumulated = std::accumulate(forward.begin(), forward.end(), 0);",
        "    std::array<std::string, 2> arr;",
        "    std::stable_sort(arr.begin(), arr.end());",
        "    std::shuffle(arr.begin(), arr.end(), std::mt19937());",
        "    std::reverse(arr.begin(), arr.end());",
        "    auto arrUnique = std::unique(arr.begin(), arr.end());",
        "    auto arrRemoved = std::remove(arr.begin(), arr.end(), \"x\");",
        "    auto arrFound = std::find(arr.begin(), arr.end(), \"x\");",
        "    auto arrCopied = std::copy(arr.begin(), arr.end(), arr.begin());",
        "    auto arrAccumulated = std::accumulate(arr.begin(), arr.end(), std::string());",
        "    std::map<int, int> mapping;",
        "    auto mapFound = mapping.find(1);",
        "    auto mapCount = mapping.count(1);",
        "    auto mapLower = mapping.lower_bound(1);",
        "    std::set<std::string> ordered;",
        "    auto setFound = ordered.find(\"x\");",
        "    auto setCount = ordered.count(\"x\");",
        "    auto setLower = ordered.lower_bound(\"x\");",
        "    auto setCopied = std::copy(ordered.begin(), ordered.end(), ordered.begin());",
        "    std::unordered_map<int, int> unorderedMapping;",
        "    auto unorderedMapFound = unorderedMapping.find(1);",
        "    auto unorderedMapCount = unorderedMapping.count(1);",
        "    std::unordered_set<int> unorderedSet;",
        "    auto unorderedSetFound = unorderedSet.find(1);",
        "    auto unorderedSetCount = unorderedSet.count(1);",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatement scope = mParser->findScopeStatement(main, 3);
    QVERIFY(scope);

    struct Case {
        const char *name;
        const char *expectedType;
        const char *requiredStatement = nullptr;
    };
    const QVector<Case> cases{
        {"find_if", "__gnu_cxx::__normal_iterator"},
        {"transform", "__gnu_cxx::__normal_iterator"},
        {"count", "long long int"},
        {"accumulate", "std::__cxx11::basic_string"},
        {"for_each", "main::__lambda_"},
        {"sort", "void"},
        {"lower_bound", "__gnu_cxx::__normal_iterator"},
        {"min_element", "__gnu_cxx::__normal_iterator"},
        {"binary_search", "bool"},
        {"any_of", "bool"},
        {"copy", "__gnu_cxx::__normal_iterator"},
        {"remove", "__gnu_cxx::__normal_iterator"},
        {"unique", "__gnu_cxx::__normal_iterator"},
        {"reverse", "void"},
        {"insert", "__gnu_cxx::__normal_iterator"},
        {"erase", "__gnu_cxx::__normal_iterator"},
        {"stable_sort", "void"},
        {"partial_sort", "void"},
        {"nth_element", "void"},
        {"reduce", "int", "std::reduce"},
        {"inner_product", "int"},
        {"count_if", "long long int"},
        {"shuffle", "void"},
        {"replace", "void"},
        {"rotate", "__gnu_cxx::__normal_iterator"},
        {"fill", "void"},
        {"generate", "void"},
        {"move", "__gnu_cxx::__normal_iterator"},
        {"copy_n", "__gnu_cxx::__normal_iterator"},
        {"find", "__gnu_cxx::__normal_iterator"},
        {"max_element", "__gnu_cxx::__normal_iterator"},
        {"minmax_element", "std::pair"},
        {"int.insert", "iterator"},
        {"int.erase", "iterator"},
        {"int.stable_sort", "void"},
        {"int.shuffle", "void"},
        {"int.rotate", "iterator"},
        {"int.unique", "iterator"},
        {"int.remove", "iterator"},
        {"int.replace", "void"},
        {"int.copy", "iterator"},
        {"int.transform", "iterator"},
        {"int.reduce", "int", "std::reduce"},
        {"int.inner_product", "int"},
        {"int.find", "iterator"},
        {"int.count_if", "long long int"},
        {"int.lower_bound", "iterator"},
        {"int.binary_search", "bool"},
        {"int.minmax_element", "std::pair"},
        {"int.for_each", "main::__lambda_"},
        {"int.generate", "void"},
        {"int.move", "iterator"},
        {"int.copy_n", "iterator"},
        {"deque.insert", "iterator"},
        {"deque.erase", "iterator"},
        {"deque.stable_sort", "void"},
        {"deque.shuffle", "void"},
        {"deque.unique", "iterator"},
        {"deque.remove", "iterator"},
        {"deque.find", "iterator"},
        {"deque.copy", "iterator"},
        {"deque.accumulate", "std::__cxx11::basic_string"},
        {"list.sort", "void"},
        {"list.reverse", "void"},
        {"list.unique", "void"},
        {"list.remove", "void"},
        {"list.find", "iterator"},
        {"list.copy", "iterator"},
        {"list.accumulate", "std::__cxx11::basic_string"},
        {"forward_list.sort", "void"},
        {"forward_list.reverse", "void"},
        {"forward_list.unique", "void"},
        {"forward_list.remove", "void"},
        {"forward_list.find", "iterator"},
        {"forward_list.copy", "iterator"},
        {"forward_list.accumulate", "int"},
        {"array.stable_sort", "void"},
        {"array.shuffle", "void"},
        {"array.reverse", "void"},
        {"array.unique", "pointer"},
        {"array.remove", "pointer"},
        {"array.find", "pointer"},
        {"array.copy", "pointer"},
        {"array.accumulate", "std::__cxx11::basic_string"},
        {"map.find", "iterator"},
        {"map.count", "long long unsigned int"},
        {"map.lower_bound", "iterator"},
        {"set.find", "iterator"},
        {"set.count", "long long unsigned int"},
        {"set.lower_bound", "iterator"},
        {"set.copy", "iterator"},
        {"unordered_map.find", "iterator"},
        {"unordered_map.count", "long long unsigned int"},
        {"unordered_set.find", "iterator"},
        {"unordered_set.count", "long long unsigned int"},
    };
    QFile source(main);
    QVERIFY(source.open(QIODevice::ReadOnly));
    const QStringList sourceLines = QString::fromUtf8(source.readAll()).split('\n');
    source.close();
    QElapsedTimer timer;
    timer.start();
    qWarning() << "suite,scenario,elapsed_ns,valid,type";
    QByteArray caseFilter = qgetenv("REDPANDA_STDLIB_ALGORITHM_CASE");
    int line = 4;
    for (int caseIndex = 0; caseIndex < cases.count(); ++caseIndex) {
        const Case &test = cases[caseIndex];
        while (line < sourceLines.count()) {
            const QString sourceLine = sourceLines[line];
            if (sourceLine.contains("= ") || sourceLine.contains('('))
                break;
            ++line;
        }
        QVERIFY(line < sourceLines.count());
        if (test.requiredStatement && !mParser->findStatement(test.requiredStatement)) {
            ++line;
            continue;
        }
        if (!caseFilter.isEmpty() && caseFilter != test.name) {
            ++line;
            continue;
        }
        if (qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE")
                || qEnvironmentVariableIsSet("REDPANDA_PARSER_TRACE")) {
            qputenv("REDPANDA_PARSER_PROFILE_RESET_TOKENIZER", "1");
            qputenv("REDPANDA_PARSER_PROFILE_RESET_RESOLVER", "1");
        }
        qint64 started = timer.nsecsElapsed();
        CppTypeResolver::EvaluationContext evaluation;
        CppTypeResolver resolver(mParser->statementList(), &evaluation);
        TypeOperationRegistry operations(resolver);
        CppExpressionEvaluator evaluator(const_cast<StatementModel &>(mParser->statementList()),
                                         resolver, operations);
        QString sourceLine = sourceLines[line];
        int expressionStart = sourceLine.indexOf("= ");
        if (expressionStart >= 0)
            expressionStart += 2;
        else
            expressionStart = sourceLine.indexOf("std::");
        QString expressionText = sourceLine.mid(expressionStart).trimmed();
        expressionText.chop(1);
        for (const PStatement &statement : mParser->statementList().statements()) {
            if (statement->kind == StatementKind::Lambda
                    && statement->definition.range.start.line == line) {
                expressionStart = statement->definition.range.start.ch - expressionText.indexOf('[');
                break;
            }
        }
        CppTokenizer tokenizer;
        tokenizer.tokenize({expressionText});
        tokenizer.relocateTokens(main, SourcePosition{line, expressionStart});
        EvalResult result = evaluator.evaluate(tokenizer, scope, &evaluation);
        qWarning() << "STDLIB_ALGORITHM_EVAL" << test.name
                   << timer.nsecsElapsed() - started << result.isValid() << result.type.toString();
        logEvalProfileCsv("STDLIB_ALGORITHM_EVAL_CSV", test.name, timer.nsecsElapsed() - started, result);
        QVERIFY2(result.isValid(), test.name);
        if (test.expectedType == QStringLiteral("main::__lambda_"))
            QVERIFY(result.type.qualifiedName.startsWith("main::__lambda_"));
        else if (test.expectedType == QStringLiteral("iterator"))
            QVERIFY(result.type.qualifiedName.contains("iterator"));
        else if (test.expectedType == QStringLiteral("pointer")) {
            QVERIFY(!result.type.derivations.isEmpty());
            QCOMPARE(result.type.derivations.first().kind, TypeDerivationKind::Pointer);
        } else
            QCOMPARE(result.type.qualifiedName, QString::fromLatin1(test.expectedType));
        ++line;
    }
}

void TestCppParser::test_standard_library_parse_baseline()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "",
        "int main() {",
        "    std::vector<std::string> lst;",
        "    lst.front().append(\"llala\");",
        "    std::cout<<lst.size()<<std::endl;",
        "    return 0;",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));

    QElapsedTimer timer;
    timer.start();
    CppParser::parseFileBlocking(mParser, main, true, "");
    qWarning() << "STDLIB_TEST_TIMING completeParseMs" << timer.elapsed();
    QVERIFY(mParser->findStatement("std::vector"));
    QVERIFY(mParser->findStatement("std::basic_string::append"));
    PStatement variable = mParser->findStatementOf(main, "lst", 4);
    QVERIFY(variable);
    QCOMPARE(variable->command, "lst");
}

void TestCppParser::test_standard_library_public_api_complete()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <bits/stdc++.h>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));

    QElapsedTimer testTimer;
    testTimer.start();
    CppParser::parseFileBlocking(mParser, main, true, "");
    qWarning() << "STDLIB_TEST_TIMING firstParse" << testTimer.restart();
    for (const char *name : {
         "std::string", "std::vector", "std::map", "std::set",
         "std::sort", "std::find", "std::cout", "std::endl",
         "std::numeric_limits", "std::basic_string::size",
         "std::vector::size", "std::vector::push_back", "std::map::find"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
    qWarning() << "STDLIB_TEST_TIMING lookups" << testTimer.restart();
    QSet<QString> firstStatements = standardLibraryStatementNames(mParser);
    qWarning() << "STDLIB_TEST_TIMING firstCollect" << testTimer.restart();
    QVERIFY(firstStatements.count() > 100);
    int statementCount = mParser->statementList().count();

    CppParser::parseFileBlocking(mParser, main, true, "");
    qWarning() << "STDLIB_TEST_TIMING secondParse" << testTimer.restart();
    QCOMPARE(standardLibraryStatementNames(mParser), firstStatements);
    qWarning() << "STDLIB_TEST_TIMING secondCollectAndCompare" << testTimer.restart();
    QCOMPARE(mParser->statementList().count(), statementCount);
    QCOMPARE(mParser->preprocessorEventCountsForTest(bitsStdcpp).scan, 1);
    QVERIFY(mParser->preprocessorEventCountsForTest(bitsStdcpp).replay > 0);
    mParser.reset();
    qWarning() << "STDLIB_TEST_TIMING destruction" << testTimer.restart();
}
#endif



void TestCppParser::test_member_declaration_definition_in_namespace()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString declaration = tmp.filePath("declare.h");
    QString definition = tmp.filePath("define.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(declaration, {"namespace N { class C { public: int method(); }; }"}));
    QVERIFY(writeLines(definition, {"namespace N { int C::method() { return 1; } }"}));
    QVERIFY(writeLines(main, {"#include \"declare.h\"", "#include \"define.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatement statement = mParser->findStatement("N::C::method");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));
    QCOMPARE(statement->declarations.value(declaration).boundaryAncestor, declaration);
    QCOMPARE(statement->definition.boundaryAncestor, definition);

    mParser->invalidateFile(definition);
    statement = mParser->findStatement("N::C::method");
    QVERIFY(statement);
    QVERIFY(!statementHasDefinition(statement));

    CppParser::parseFileBlocking(mParser, main, true, "");
    statement = mParser->findStatement("N::C::method");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));

    mParser->invalidateFile(declaration);
    QVERIFY(!mParser->findStatement("N::C::method"));

    CppParser::parseFileBlocking(mParser, main, true, "");
    statement = mParser->findStatement("N::C::method");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));
    QCOMPARE(statement->declarations.value(declaration).fileName, declaration);
    QCOMPARE(statement->declarations.value(declaration).boundaryAncestor, declaration);
    QCOMPARE(statement->definition.fileName, definition);
    QCOMPARE(statement->definition.boundaryAncestor, definition);
    PStatement parent = statement->parentScope.lock();
    QVERIFY(parent);
    QCOMPARE(parent->fullName, QString("N::C"));
    bool memberIndexed = false;
    for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(parent)->children.values("method")) {
        if (weak.lock() == statement) {
            memberIndexed = true;
            break;
        }
    }
    QVERIFY(memberIndexed);
}

void TestCppParser::test_overloaded_declaration_definition_merge()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString declaration = tmp.filePath("declare.h");
    QString otherDeclaration = tmp.filePath("declare_other.h");
    QString definition = tmp.filePath("define.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(declaration, {"namespace N { int f(int); }"}));
    QVERIFY(writeLines(otherDeclaration, {"namespace N { int f(double); }"}));
    QVERIFY(writeLines(definition, {"namespace N { int f(double value) { return int(value); } }"}));
    QVERIFY(writeLines(main, {"#include \"declare.h\"", "#include \"declare_other.h\"",
                              "#include \"define.h\""}));

    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatementList namespaces = mParser->findNamespace("N");
    QVERIFY(namespaces && namespaces->count() == 1);
    auto nameSpace = std::dynamic_pointer_cast<NamespaceStatement>(namespaces->first());
    QVERIFY(nameSpace);
    QCOMPARE(nameSpace->declarations.count(), 3);
    auto overloadedFunctions = [&]() {
        QList<PStatement> result;
        for (const PStatement &nameSpace : *namespaces) {
            for (const PStatement &statement : mParser->membersOfStatement(nameSpace)) {
                if (statement->fullName == "N::f")
                    result.append(statement);
            }
        }
        return result;
    };
    PStatement intDeclaration;
    PStatement doubleDefinition;
    for (const PStatement &statement : overloadedFunctions()) {
        if (std::dynamic_pointer_cast<Statement>(statement)->normalizedArgs == "(int)")
            intDeclaration = statement;
        else if (std::dynamic_pointer_cast<Statement>(statement)->normalizedArgs == "(double)")
            doubleDefinition = statement;
    }
    QVERIFY(intDeclaration);
    QVERIFY(doubleDefinition);
    QVERIFY(intDeclaration != doubleDefinition);
    QVERIFY(!statementHasDefinition(intDeclaration));
    QVERIFY(statementHasDefinition(doubleDefinition));
    QCOMPARE(doubleDefinition->declarations.value(otherDeclaration).boundaryAncestor, otherDeclaration);
    QCOMPARE(doubleDefinition->definition.boundaryAncestor, definition);
    QCOMPARE(doubleDefinition->definition.fileName, definition);

    mParser->invalidateFile(definition);
    QVERIFY(!statementHasDefinition(doubleDefinition));
    QVERIFY(!statementHasDefinition(intDeclaration));
    QCOMPARE(overloadedFunctions().count(), 2);
}

void TestCppParser::test_statement_declaration_navigation_selection()
{
    StatementModel model;
    PStatement statement = StatementFactory::createVariable({}, "value", "value");
    model.addDeclaration(statement, "other.h", SourceRange{{10, 0}, {10, 1}},
                         "current.tcc", {});
    model.addDeclaration(statement, "current.tcc", SourceRange{{20, 0}, {20, 1}},
                         "context.h", {});
    StatementOccurrence selected = statementDeclarationForNavigation(statement, "current.tcc");
    QCOMPARE(selected.fileName, "current.tcc");
    QCOMPARE(selected.range.start.line, 20);

    PStatement contextStatement = StatementFactory::createVariable({}, "contextValue", "contextValue");
    model.addDeclaration(contextStatement, "inner.h", SourceRange{{30, 0}, {30, 1}},
                         "context.h", {});
    selected = statementDeclarationForNavigation(contextStatement, "context.h");
    QCOMPARE(selected.fileName, "inner.h");
    QCOMPARE(selected.range.start.line, 30);

    PStatement definitionStatement = StatementFactory::createVariable({}, "definedValue", "definedValue");
    definitionStatement->definition = StatementOccurrence{
        "definition.cpp", SourceRange{{40, 0}, {40, 1}}, QString(), {}};
    setStatementHasDefinition(definitionStatement, true);
    selected = statementDeclarationForNavigation(definitionStatement, "current.tcc");
    QCOMPARE(selected.fileName, "definition.cpp");
    QCOMPARE(selected.range.start.line, 40);
}

void TestCppParser::test_local_declarations_in_disjoint_blocks_remain_distinct()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"void f() {", "  { int value; }", "  { int value; }", "}"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "main.cpp", false, "");
    PStatement function = mParser->findStatement("f");
    QVERIFY(function);
    QList<PStatement> variables;
    for (const std::weak_ptr<AbstractStatement> &weakBlock : std::dynamic_pointer_cast<Statement>(function)->children) {
        PStatement block = weakBlock.lock();
        if (!block || block->kind != StatementKind::Block)
            continue;
        for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(block)->children.values("value")) {
            if (PStatement variable = weak.lock())
                variables.append(variable);
        }
    }
    QCOMPARE(variables.count(), 2);
    QVERIFY(variables[0]->parentScope.lock() != variables[1]->parentScope.lock());
}

void TestCppParser::test_namespace_index_inline_filter_and_invalidation()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"namespace std { int value; }",
                             "namespace std { inline namespace __cxx11 { int inlineValue; } }"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "main.cpp", false, "");
    QCOMPARE(mParser->findNamespace("std")->count(), 1);
    QCOMPARE(mParser->findNamespace("std", true)->count(), 1);
    QVERIFY(mParser->findStatement("std::inlineValue"));
    QCOMPARE(mParser->findNamespace("std::__cxx11")->count(), 1);
    mParser->invalidateFile("main.cpp");
    QVERIFY(mParser->findNamespace("std")->isEmpty());
    QVERIFY(mParser->findNamespace("std", true)->isEmpty());
    QVERIFY(mParser->findNamespace("std::__cxx11")->isEmpty());
}

void TestCppParser::test_namespace_index_does_not_promote_inline_descendants()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"namespace outer { namespace regular { } }",
                             "namespace outer::regular { inline namespace abi { int value; } }",
                             "namespace outer::regular { int reopened; }"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "main.cpp", false, "");
    QCOMPARE(mParser->findNamespace("outer")->count(), 1);
    QCOMPARE(mParser->findNamespace("outer", true)->count(), 1);
    QCOMPARE(mParser->findNamespace("outer::regular")->count(), 1);
    QCOMPARE(mParser->findNamespace("outer::regular", true)->count(), 1);
    QVERIFY(mParser->findStatement("outer::regular::value"));
    QVERIFY(mParser->findStatement("outer::regular::reopened"));
    QVERIFY(!mParser->findStatement("outer::value"));
    mParser->invalidateFile("main.cpp");
    QVERIFY(mParser->findNamespace("outer", true)->isEmpty());
    QVERIFY(mParser->findNamespace("outer::regular", true)->isEmpty());
}

void TestCppParser::test_reopened_namespaces_share_unique_entity()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace A::B { int first; }",
            "namespace A { namespace B { int second; } }",
            "namespace A::B::C::D { int deep; }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement a = mParser->findStatement("A");
    PStatement b = mParser->findStatement("A::B");
    PStatement c = mParser->findStatement("A::B::C");
    PStatement d = mParser->findStatement("A::B::C::D");
    QVERIFY(a && b && c && d);
    QCOMPARE(b->parentScope.lock(), a);
    QCOMPARE(c->parentScope.lock(), b);
    QCOMPARE(d->parentScope.lock(), c);
    QCOMPARE(mParser->findNamespace("A")->count(), 1);
    QCOMPARE(mParser->findNamespace("A::B")->count(), 1);
    auto namespaceA = std::dynamic_pointer_cast<NamespaceStatement>(a);
    auto namespaceB = std::dynamic_pointer_cast<NamespaceStatement>(b);
    QCOMPARE(namespaceA->declarations.count(), 3);
    QCOMPARE(namespaceB->declarations.count(), 3);
    QSet<int> namespaceBLines;
    for (const StatementOccurrence &occurrence : namespaceB->declarations) {
        QCOMPARE(occurrence.fileName, "text.cpp");
        namespaceBLines.insert(occurrence.range.start.line);
    }
    QCOMPARE(namespaceBLines, QSet<int>({0, 1, 2}));
    QVERIFY(mParser->findStatement("A::B::first"));
    QVERIFY(mParser->findStatement("A::B::second"));
    QVERIFY(mParser->findStatement("A::B::C::D::deep"));
}

void TestCppParser::test_namespace_occurrence_invalidation_across_roots()
{
    mParser->setOnGetFileStream([](const QString &fileName, QStringList &buffer) {
        if (fileName == "a.cpp")
            buffer = QStringList{"namespace N::Deep { int fromA; }"};
        else if (fileName == "b.cpp")
            buffer = QStringList{"namespace N { namespace Deep { int fromB; } }"};
        else
            return false;
        return true;
    });
    CppParser::parseFileBlocking(mParser, "a.cpp", false, "");
    CppParser::parseFileBlocking(mParser, "b.cpp", false, "");

    PStatement n = mParser->findStatement("N");
    PStatement deep = mParser->findStatement("N::Deep");
    QVERIFY(n && deep);
    auto namespaceN = std::dynamic_pointer_cast<NamespaceStatement>(n);
    auto namespaceDeep = std::dynamic_pointer_cast<NamespaceStatement>(deep);
    QCOMPARE(namespaceN->declarations.count(), 2);
    QCOMPARE(namespaceDeep->declarations.count(), 2);
    QVERIFY(namespaceN->declarations.contains("a.cpp"));
    QVERIFY(namespaceN->declarations.contains("b.cpp"));
    QVERIFY(mParser->findStatement("N::Deep::fromA"));
    QVERIFY(mParser->findStatement("N::Deep::fromB"));

    mParser->invalidateFile("a.cpp");
    QVERIFY(!mParser->findStatement("N::Deep::fromA"));
    QVERIFY(mParser->findStatement("N::Deep::fromB"));
    QCOMPARE(mParser->findStatement("N"), n);
    QCOMPARE(mParser->findStatement("N::Deep"), deep);
    QCOMPARE(namespaceDeep->declarations.count(), 1);
    QVERIFY(namespaceDeep->declarations.contains("b.cpp"));
    QCOMPARE(namespaceDeep->declarations.value("b.cpp").fileName, "b.cpp");
    QCOMPARE(namespaceDeep->declarations.value("b.cpp").range.start.line, 0);

    mParser->invalidateFile("b.cpp");
    QVERIFY(!mParser->findStatement("N"));
    QVERIFY(!mParser->findStatement("N::Deep"));
}

void TestCppParser::test_anonymous_namespaces_remain_distinct()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace { int first; }",
            "namespace { int second; }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    const auto anonymous = mParser->statementList().childrenStatements().values("");
    QCOMPARE(anonymous.count(), 2);
    PStatement firstScope = mParser->findScopeStatement("text.cpp", 0);
    PStatement secondScope = mParser->findScopeStatement("text.cpp", 1);
    QVERIFY(firstScope && secondScope);
    QVERIFY(firstScope != secondScope);
    QVERIFY(mParser->findStatement("first"));
    QVERIFY(mParser->findStatement("second"));
}

void TestCppParser::test_inline_namespace_occurrence_invalidation()
{
    mParser->setOnGetFileStream([](const QString &fileName, QStringList &buffer) {
        if (fileName == "plain.cpp")
            buffer = QStringList{"namespace N { namespace V { int plain; } }"};
        else if (fileName == "inline.cpp")
            buffer = QStringList{"namespace N { inline namespace V { int visible; } }"};
        else
            return false;
        return true;
    });
    CppParser::parseFileBlocking(mParser, "plain.cpp", false, "");
    CppParser::parseFileBlocking(mParser, "inline.cpp", false, "");

    PStatement v = mParser->findStatement("N::V");
    QVERIFY(v);
    QCOMPARE(std::dynamic_pointer_cast<NamespaceStatement>(v)->declarations.count(), 2);
    QVERIFY(mParser->findStatement("N::visible"));
    QVERIFY(mParser->findStatement("N::plain"));

    mParser->invalidateFile("inline.cpp");
    QVERIFY(!mParser->findStatement("N::visible"));
    QVERIFY(mParser->findStatement("N::V::plain"));
    QCOMPARE(mParser->findStatement("N::V"), v);
    auto namespaceV = std::dynamic_pointer_cast<NamespaceStatement>(v);
    QCOMPARE(namespaceV->declarations.count(), 1);
    QVERIFY(namespaceV->declarations.contains("plain.cpp"));
    QVERIFY(!namespaceV->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline));
    QCOMPARE(mParser->findNamespace("N", true)->count(), 1);

    CppParser::parseFileBlocking(mParser, "inline.cpp", false, "");
    QCOMPARE(mParser->findStatement("N::V"), v);
    QCOMPARE(namespaceV->declarations.count(), 2);
    QVERIFY(namespaceV->declSpecifierProperties.testFlag(DeclSpecifierProperty::Inline));
    QVERIFY(mParser->findStatement("N::visible"));
}

void TestCppParser::test_inline_namespace_overloads()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace N { inline namespace V {",
            "  void f(int);",
            "  void f(double);",
            "} }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");

    PStatement n = mParser->findStatement("N");
    QVERIFY(n);
    const auto aliases = mParser->statementList().childrenStatements(n).values("f");
    QCOMPARE(aliases.count(), 2);
    QSet<const AbstractStatement *> targets;
    for (const std::weak_ptr<AbstractStatement> &weak : aliases) {
        PStatement statement = weak.lock();
        QVERIFY(statement && statement->kind == StatementKind::InlineAlias);
        PStatement target = std::dynamic_pointer_cast<InlineAliasStatement>(statement)->target.lock();
        QVERIFY(target);
        targets.insert(target.get());
    }
    QCOMPARE(targets.count(), 2);
    int visibleOverloads = 0;
    for (const PStatement &statement : mParser->membersOfStatement(n)) {
        if (statement->command == "f")
            ++visibleOverloads;
    }
    QCOMPARE(visibleOverloads, 2);

    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    n = mParser->findStatement("N");
    QVERIFY(n);
    QCOMPARE(mParser->statementList().childrenStatements(n).values("f").count(), 2);
}
