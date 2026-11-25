#include <QTest>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include "test_cppparserstandardlibrary.h"
#ifdef REDPANDA_PARSER_STDLIB_TEST
#include "test_parser_utils.h"
#include "src/parser/cppexpressionevaluator.h"
#include "src/parser/cpptyperesolver.h"

namespace {
void logEvalProfileCsv(const QString &suite, const QString &name, qint64 elapsedNs,
                       const EvalResult &result)
{
    QString type = result.type.toString();
    type.replace("\"", "\"\"");
    qWarning().noquote() << QString("%1,%2,%3,%4,\"%5\"")
            .arg(suite, name).arg(elapsedNs).arg(result.isValid() ? 1 : 0).arg(type);
}

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
}
#endif

#ifdef REDPANDA_PARSER_STDLIB_TEST
void TestCppParserStandardLibrary::test_bits_stdcpp_not_rescanned_on_stable_reparse()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QString other = tmp.filePath("other.cpp");
    QVERIFY(writeLines(main, {"#include <bits/stdc++.h>"}));
    QVERIFY(writeLines(other, {"// unrelated translation unit"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));

    CppParser::parseFileBlocking(mParser, main, "");
    QCOMPARE(mParser->preprocessorEventCountsForTest(bitsStdcpp).scan, 1);
    int statementCount = mParser->statementList().count();

    CppParser::parseFileBlocking(mParser, other, "");
    CppParser::parseFileBlocking(mParser, main, "");
    CppPreprocessor::EventCounts counts = mParser->preprocessorEventCountsForTest(bitsStdcpp);
    QCOMPARE(counts.scan, 1);
    QVERIFY(counts.replay > 0);
    QCOMPARE(mParser->statementList().count(), statementCount);
}

void TestCppParserStandardLibrary::test_standard_library_abort_alias()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <bits/stdc++.h>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");

    PStatement alias = mParser->findStatement("std::abort");
    QVERIFY(alias);
    PStatement target = mParser->findAliasedStatement(alias);
    QVERIFY(target);
    QCOMPARE(target->kind, StatementKind::Function);
    QCOMPARE(target->command, QString("abort"));
    if (alias->kind == StatementKind::Alias)
        QVERIFY(target != alias);
}

void TestCppParserStandardLibrary::test_standard_library_function_eval_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("expectedType");
    QTest::newRow("empty-variable") << "empty" << "std::function<void ()>";
    QTest::newRow("identity-variable") << "identity" << "std::function<int (int)>";
    QTest::newRow("empty-call") << "empty()" << "void";
    QTest::newRow("lambda-call") << "identity(1)" << "int";
    QTest::newRow("function-pointer-call") << "functionPointer(1)" << "int";
    QTest::newRow("const-ref-lambda-call") << "reader(1)" << "int";
    QTest::newRow("class-argument-call") << "func(std::string())" << "void";
    QTest::newRow("class-argument-braced-call") << "func({})" << "void";
    QTest::newRow("target") << "identity.target<int(*)(int)>()" << "int *(*)(int)";
    QTest::newRow("target-type") << "identity.target_type()" << "const std::type_info &";
    QTest::newRow("swap") << "identity.swap(functionPointer)" << "void";
    QTest::newRow("assignment") << "identity = functionPointer" << "std::function<int (int)> &";
    QTest::newRow("bool-conversion") << "static_cast<bool>(identity)" << "bool";
}

void TestCppParserStandardLibrary::test_standard_library_function_eval()
{
    QFETCH(QString, expression);
    QFETCH(QString, expectedType);
    const QByteArray caseFilter = qgetenv("REDPANDA_STDLIB_FUNCTION_CASE");
    if (!caseFilter.isEmpty() && caseFilter != expression)
        QSKIP("Filtered by REDPANDA_STDLIB_FUNCTION_CASE");
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <functional>",
        "#include <string>",
        "int freeFunction(int value) { return value; }",
        "int main() {",
        "    std::function<void()> empty;",
        "    std::function<int(int)> identity = [](int value) { return value; };",
        "    std::function<int(int)> functionPointer = freeFunction;",
        "    std::function<int(const int&)> reader = [](const int &value) { return value; };",
        "    std::function<void(std::string)> func = [](std::string) {};",
        "    empty();",
        "    identity(1);",
        "    functionPointer(1);",
        "    reader(1);",
        "    func(std::string());",
        "    func({});",
        "    identity.target<int(*)(int)>();",
        "    identity.target_type();",
        "    identity.swap(functionPointer);",
        "    identity = functionPointer;",
        "    bool valid = static_cast<bool>(identity);",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");
    PStatement scope = mParser->findScopeStatement(main, 10);
    QVERIFY(scope);

    CppTokenizer tokenizer;
    tokenizer.tokenize({expression});
    if (qEnvironmentVariableIsSet("REDPANDA_PARSER_PROFILE")
            || qEnvironmentVariableIsSet("REDPANDA_PARSER_TRACE")) {
        qputenv("REDPANDA_PARSER_PROFILE_RESET_TOKENIZER", "1");
        qputenv("REDPANDA_PARSER_PROFILE_RESET_RESOLVER", "1");
    }
    QElapsedTimer timer;
    timer.start();
    EvalResult result = mParser->evalExpression(main, tokenizer.tokens(), scope);
    qWarning() << "STDLIB_FUNCTION_EVAL" << expression << timer.nsecsElapsed()
               << result.isValid() << result.type.toString();
    QVERIFY(result.isValid());
    if (!expectedType.isEmpty()) {
        QString actualType = result.type.toString();
        actualType.remove("::__1");
        actualType.remove("::__cxx11");
        QCOMPARE(actualType, expectedType);
    }
}

void TestCppParserStandardLibrary::test_standard_library_string()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <string>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    if (mParser->compilerType() == CppCompilerType::Clang)
        QSKIP("This test requires the libstdc++ basic_string implementation");

    CppParser::parseFileBlocking(mParser, main, "");
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

void TestCppParserStandardLibrary::test_standard_library_header_context()
{
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    QDir includeDir = QFileInfo(bitsStdcpp).dir();
    QVERIFY(includeDir.cdUp());
    if (!QFileInfo::exists(includeDir.filePath("string")))
        QVERIFY(includeDir.cdUp());
    QString stringFile = includeDir.filePath("string");
    QString basicStringTcc = includeDir.filePath("bits/basic_string.tcc");
    if (!QFileInfo::exists(stringFile) || !QFileInfo::exists(basicStringTcc))
        QSKIP("This test requires the libstdc++ basic_string.tcc layout");

    CppParser::parseFileBlocking(mParser, basicStringTcc, stringFile);

    QCOMPARE(mParser->preprocessorEventCountsForTest(basicStringTcc).scan, 1);
    QVERIFY(mParser->hasReplayRecord(stringFile));
    QVERIFY(!mParser->hasReplayRecord(basicStringTcc));
    QVERIFY(mParser->isFileParsedInContext(basicStringTcc, stringFile));
    QVERIFY(mParser->findStatementOf(basicStringTcc, QStringList{"std"}, 50));
}

void TestCppParserStandardLibrary::test_standard_library_nested_include_guard_visibility()
{
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    QString stringFile = mParser->getHeaderFileName(bitsStdcpp, "<string>");
    QVERIFY(!stringFile.isEmpty());
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <bits/stdc++.h>"}));

    CppParser::parseFileBlocking(mParser, main, "");
    PParsedFileInfo stringInfo = mParser->findFileInfo(stringFile);
    QVERIFY(stringInfo);
    QVERIFY(stringInfo->includeProtected());
    QCOMPARE(mParser->preprocessorEventCountsForTest(stringFile).scan, 1);
    int guardLine = includeGuardDefineLine(stringFile);
    QVERIFY(guardLine >= 0);
    QVERIFY(mParser->isLineVisible(stringFile, guardLine));

    CppParser::parseFileBlocking(mParser, bitsStdcpp, main);
    QVERIFY(mParser->isLineVisible(stringFile, guardLine));
    CppParser::parseFileBlocking(mParser, stringFile, main);
    QVERIFY(mParser->isLineVisible(stringFile, guardLine));
}

void TestCppParserStandardLibrary::test_standard_library_direct_c_header_context()
{
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    QString stdioFile = mParser->getHeaderFileName(bitsStdcpp, "<stdio.h>");
    QVERIFY(!stdioFile.isEmpty());
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <stdio.h>"}));

    CppParser::parseFileBlocking(mParser, stdioFile, main);

    QVERIFY(mParser->isFileParsedInContext(stdioFile, main));
    QCOMPARE(mParser->preprocessorEventCountsForTest(stdioFile).scan, 1);
    PParsedFileInfo stdioInfo = mParser->findFileInfo(stdioFile);
    QVERIFY(stdioInfo);
    const int guardDefineLine = includeGuardDefineLine(stdioFile);
    if (guardDefineLine >= 0) {
        QVERIFY(stdioInfo->includeProtected());
        QVERIFY(mParser->isLineVisible(stdioFile, guardDefineLine));
    }

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
    if (structLine < 0)
        QSKIP("The selected stdio.h wrapper does not define _iobuf");
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

void TestCppParserStandardLibrary::test_standard_library_header_guard_context_visibility_data()
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

void TestCppParserStandardLibrary::test_standard_library_header_guard_context_visibility()
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

    CppParser::parseFileBlocking(mParser, headerFile, main);

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

void TestCppParserStandardLibrary::test_standard_library_front_append()
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
    CppParser::parseFileBlocking(mParser, main, "");

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

void TestCppParserStandardLibrary::test_standard_library_vector_subscript()
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

    CppParser::parseFileBlocking(mParser, main, "");
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

void TestCppParserStandardLibrary::test_standard_library_make_shared_default()
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
    CppParser::parseFileBlocking(mParser, main, "");

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

void TestCppParserStandardLibrary::test_standard_library_expression_chains()
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
    CppParser::parseFileBlocking(mParser, main, "");

    PStatement stringSize = mParser->findStatement("std::basic_string::size");
    QVERIFY(stringSize);
    PStatement iteratorScope = mParser->findScopeStatement(main, 20);
    QVERIFY(iteratorScope);
    QStringList beginExpression{"lst", ".", "begin", "(", ")"};
    QStringList iteratorExpression{"it"};
    EvalResult beginResult = mParser->evalExpression(main, beginExpression, iteratorScope);
    EvalResult iteratorResult = mParser->evalExpression(main, iteratorExpression, iteratorScope);
    QVERIFY(beginResult.isValid());
    QVERIFY(beginResult.instance.declaration);
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
    QVERIFY(entryResult.type.qualifiers.testFlag(TypeQualifier::Const));
    QVERIFY(entryDeclaration);
    QCOMPARE(entryDeclaration, mParser->findStatement("std::pair"));
    expectStringSize(42, {"entry", ".", "first", ".", "size"});
    expectStringSize(43, {"entry", ".", "second", ".", "size"});
    QStringList flagElementExpression{"flags", "[", "0", "]"};
    EvalResult flagElement = mParser->evalExpression(
                main, flagElementExpression, mParser->findScopeStatement(main, 37));
    PStatement flip = mParser->findStatementOf(
                main, {"flags", "[", "0", "]", ".", "flip"}, 37);
    QVERIFY2(flip, flagElement.type.toString().toUtf8().constData());
    QVERIFY(flagElement.instance.declaration);
    QCOMPARE(flip->parentScope.lock(), flagElement.instance.declaration);
}

void TestCppParserStandardLibrary::test_standard_library_trace_edge_cases()
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
    CppParser::parseFileBlocking(mParser, main, "");

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

void TestCppParserStandardLibrary::test_standard_library_container_eval_profile()
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
        "    for (const auto &entry : mapping) {",
        "        entry.first.size();",
        "    }",
        "}"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");
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
        {"classic_for.iterator_arrow", 29, {"i", "->"}},
        {"range_for.entry_first", 31, {"entry", ".", "first"}}
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

void TestCppParserStandardLibrary::test_standard_library_algorithm_eval_profile()
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
    CppParser::parseFileBlocking(mParser, main, "");
    PStatement scope = mParser->findScopeStatement(main, 3);
    QVERIFY(scope);

    struct Case {
        const char *name;
        const char *expectedType;
        const char *requiredStatement = nullptr;
    };
    const QVector<Case> cases{
        {"find_if", "iterator"},
        {"transform", "iterator"},
        {"count", "difference"},
        {"accumulate", "basic_string"},
        {"for_each", "main::__lambda_"},
        {"sort", "void"},
        {"lower_bound", "iterator"},
        {"min_element", "iterator"},
        {"binary_search", "bool"},
        {"any_of", "bool"},
        {"copy", "iterator"},
        {"remove", "iterator"},
        {"unique", "iterator"},
        {"reverse", "void"},
        {"insert", "iterator"},
        {"erase", "iterator"},
        {"stable_sort", "void"},
        {"partial_sort", "void"},
        {"nth_element", "void"},
        {"reduce", "int", "std::reduce"},
        {"inner_product", "int"},
        {"count_if", "difference"},
        {"shuffle", "void"},
        {"replace", "void"},
        {"rotate", "iterator"},
        {"fill", "void"},
        {"generate", "void"},
        {"move", "iterator"},
        {"copy_n", "iterator"},
        {"find", "iterator"},
        {"max_element", "iterator"},
        {"minmax_element", "pair"},
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
        {"int.count_if", "difference"},
        {"int.lower_bound", "iterator"},
        {"int.binary_search", "bool"},
        {"int.minmax_element", "pair"},
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
        {"deque.accumulate", "basic_string"},
        {"list.sort", "void"},
        {"list.reverse", "void"},
        {"list.unique", "void"},
        {"list.remove", "void"},
        {"list.find", "iterator"},
        {"list.copy", "iterator"},
        {"list.accumulate", "basic_string"},
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
        {"array.accumulate", "basic_string"},
        {"map.find", "iterator"},
        {"map.count", "size"},
        {"map.lower_bound", "iterator"},
        {"set.find", "iterator"},
        {"set.count", "size"},
        {"set.lower_bound", "iterator"},
        {"set.copy", "iterator"},
        {"unordered_map.find", "iterator"},
        {"unordered_map.count", "size"},
        {"unordered_set.find", "iterator"},
        {"unordered_set.count", "size"},
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
        CppTypeResolver resolver(mParser->statementList(), &evaluation,
                                 {mParser->compilerType(), mParser->cppVersion()});
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
        else if (test.expectedType == QStringLiteral("iterator")) {
            PStatement declaration = result.type.declaration.lock();
            QString typeName = declaration ? declaration->command : result.type.qualifiedName;
            QVERIFY(typeName.contains("iter", Qt::CaseInsensitive));
        } else if (test.expectedType == QStringLiteral("basic_string")
                   || test.expectedType == QStringLiteral("pair")) {
            PStatement declaration = result.type.declaration.lock();
            QVERIFY(declaration);
            QCOMPARE(declaration->command, QString::fromLatin1(test.expectedType));
        } else if (test.expectedType == QStringLiteral("difference")) {
            static const QSet<QString> supportedTypes{
                "std::ptrdiff_t", "ptrdiff_t", "long long int", "long int"
            };
            QVERIFY(supportedTypes.contains(result.type.qualifiedName));
        } else if (test.expectedType == QStringLiteral("size")) {
            static const QSet<QString> supportedTypes{
                "std::size_t", "size_t", "unsigned long long int", "long long unsigned int",
                "unsigned long int"
            };
            QVERIFY(supportedTypes.contains(result.type.qualifiedName));
        } else if (test.expectedType == QStringLiteral("pointer")) {
            QVERIFY(!result.type.derivations.isEmpty());
            QCOMPARE(result.type.derivations.first().kind, TypeDerivationKind::Pointer);
        } else
            QCOMPARE(result.type.qualifiedName, QString::fromLatin1(test.expectedType));
        ++line;
    }
}

void TestCppParserStandardLibrary::test_standard_library_type_traits_builtins()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <type_traits>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");

    QVERIFY(mParser->findStatement("std::is_aggregate"));
    QVERIFY(mParser->findStatement("std::is_aggregate_v"));
    QVERIFY(mParser->findStatement("std::has_unique_object_representations"));
    QVERIFY(mParser->findStatement("std::has_unique_object_representations_v"));
}

void TestCppParserStandardLibrary::test_standard_library_atomic_macros()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <atomic>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");

    QVERIFY(mParser->findStatement("ATOMIC_VAR_INIT"));
    QVERIFY(mParser->findStatement("ATOMIC_FLAG_INIT"));
    QVERIFY(mParser->findStatement("std::atomic_flag"));
    QVERIFY(mParser->findStatement("std::atomic_flag::operator="));
}

void TestCppParserStandardLibrary::test_standard_library_c_headers()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <cstdio>", "#include <cwctype>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");

    QVERIFY(mParser->findStatement("BUFSIZ"));
    QVERIFY(mParser->findStatement("std::iswalnum"));
}

void TestCppParserStandardLibrary::test_standard_library_filesystem()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <filesystem>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");

    QVERIFY(mParser->findStatement("std::filesystem::path"));
    QVERIFY(mParser->findStatement("std::filesystem::path::operator=="));
    QVERIFY(mParser->findStatement("std::filesystem::directory_entry"));
}

void TestCppParserStandardLibrary::test_standard_library_conversion_declarations()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <functional>", "#include <string>", "#include <cstdlib>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");

    if (mParser->compilerType() == CppCompilerType::Clang) {
        QVERIFY(mParser->findStatement("std::__1::reference_wrapper::operator type&"));
        QVERIFY(mParser->findStatement("std::__1::basic_string::operator __self_view"));
    } else {
        QVERIFY(mParser->findStatement("std::reference_wrapper::operator _Tp&"));
        QVERIFY(mParser->findStatement("std::__cxx11::basic_string::operator __sv_type"));
    }
    QVERIFY(mParser->findStatement("std::div_t"));
}

void TestCppParserStandardLibrary::test_standard_library_execution()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <execution>", "#include <mutex>", "#include <strstream>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");

    auto hasMember = [](const QList<PStatement> &members, const QString &command) {
        for (const PStatement &member : members) {
            if (member->command == command)
                return true;
        }
        return false;
    };
    if (mParser->findStatement("std::execution::sequenced_policy")) {
        QVERIFY(mParser->findStatement("std::execution::seq"));
        QVERIFY(mParser->findStatement("std::execution::par"));
        QVERIFY(mParser->findStatement("std::is_execution_policy"));
    } else {
        qWarning() << "Standard library implementation gap: execution policies are unavailable";
    }
    for (const QString &name : {"std::timed_mutex", "std::recursive_timed_mutex"}) {
        PStatement scope = mParser->findStatement(name);
        QVERIFY2(scope, qPrintable(name));
        const QList<PStatement> members = mParser->resolveMembersOfClass(scope, scope);
        if (hasMember(members, "native_handle_type")) {
            QVERIFY2(hasMember(members, "native_handle"), qPrintable(name + "::native_handle"));
        } else {
            qWarning() << "Standard library implementation gap:" << name + "::native_handle";
        }
    }
    PStatement strstreambuf = mParser->findStatement("std::strstreambuf");
    QVERIFY(strstreambuf);
    QVERIFY(hasMember(mParser->resolveMembersOfClass(strstreambuf, strstreambuf), "setbuf"));
}

void TestCppParserStandardLibrary::test_standard_library_inherited_members()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <memory>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");

    QHash<QString, QStringList> expectedMembers{
        {"std::shared_ptr", {"operator[]"}},
        {"std::owner_less", {"operator()"}},
        {"std::allocator", {"allocate", "max_size"}}
    };
    if (mParser->compilerType() != CppCompilerType::Clang) {
        expectedMembers.insert("std::__new_allocator", {"allocate", "max_size"});
        expectedMembers.insert("std::__allocator_base", {"allocate", "max_size"});
    }
    for (auto it = expectedMembers.constBegin(); it != expectedMembers.constEnd(); ++it) {
        PStatement scope = mParser->findStatement(it.key());
        QVERIFY2(scope, qPrintable(it.key()));
        PStatement target = mParser->findAliasedStatement(scope);
        if (target)
            scope = target;
        const QList<PStatement> members = mParser->resolveMembersOfClass(scope, {});
        for (const QString &command : it.value()) {
            bool found = false;
            for (const PStatement &member : members) {
                if (member->command == command) {
                    found = true;
                    break;
                }
            }
            QVERIFY2(found, qPrintable(it.key() + "::" + command));
        }
    }
}

void TestCppParserStandardLibrary::test_standard_library_libcxx_hash_requirements()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "struct FalseType {};",
        "template<bool B> struct Selector { using type = FalseType; };",
        "template<> struct Selector<true> { using type = std::string; };",
        "using Direct = Selector<std::integral_constant<bool,",
        "    __is_constructible(std::hash<std::string>, const std::hash<std::string>&)>::value>::type;",
        "using Copy = Selector<std::is_copy_constructible<std::hash<std::string>>::value>::type;",
        "using Invocable = Selector<std::__is_invocable_r_v<std::size_t,",
        "    std::hash<std::string>, const std::string&>::value>::type;",
        "using Check = Selector<std::__check_hash_requirements<",
        "    std::string, std::hash<std::string>>::value>::type;",
        "using Enabled = Selector<std::__has_enabled_hash<std::string>::value>::type;",
        "using All = Selector<std::__all<std::__has_enabled_hash<std::string>::value>::value>::type;"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    if (mParser->compilerType() != CppCompilerType::Clang)
        QSKIP("This test uses libc++ internal traits");
    CppParser::parseFileBlocking(mParser, main, "");
    PStatement string = mParser->findStatement("std::basic_string");
    QVERIFY(string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Direct", {}), string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Copy", {}), string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Invocable", {}), string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Check", {}), string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Enabled", {}), string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "All", {}), string);
}

void TestCppParserStandardLibrary::test_standard_library_libcxx_copy_constructible()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "struct FalseType {};",
        "template<bool B> struct Selector { using type = FalseType; };",
        "template<> struct Selector<true> { using type = std::string; };",
        "using Reference = std::__add_lvalue_reference_t<const std::hash<std::string>>;",
        "using DirectReference = Selector<std::integral_constant<bool, __is_constructible(",
        "    std::hash<std::string>, std::__add_lvalue_reference_t<const std::hash<std::string>>)",
        "    >::value>::type;",
        "using Constructible = Selector<std::integral_constant<bool,",
        "    __is_constructible(std::hash<std::string>, Reference)>::value>::type;",
        "using FullyQualified = Selector<std::integral_constant<bool, __is_constructible(",
        "    std::__1::hash<std::__1::basic_string<char, std::__1::char_traits<char>,",
        "    std::__1::allocator<char>>>, std::__add_lvalue_reference_t<const std::__1::hash<",
        "    std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>>)",
        "    >::value>::type;",
        "using Copy = Selector<std::is_copy_constructible<std::hash<std::string>>::value>::type;"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    if (mParser->compilerType() != CppCompilerType::Clang)
        QSKIP("This test uses libc++ internal traits");
    CppParser::parseFileBlocking(mParser, main, "");
    PStatement string = mParser->findStatement("std::basic_string");
    QVERIFY(string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "DirectReference", {}), string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Constructible", {}), string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "FullyQualified", {}), string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Copy", {}), string);
}

void TestCppParserStandardLibrary::test_standard_library_libcxx_invocable()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "struct FalseType {};",
        "template<bool B> struct Selector { using type = FalseType; };",
        "template<> struct Selector<true> { using type = std::string; };",
        "using Invocable = Selector<std::__is_invocable_r_v<std::size_t,",
        "    std::hash<std::string>, const std::string&>::value>::type;"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    if (mParser->compilerType() != CppCompilerType::Clang)
        QSKIP("This test uses libc++ internal traits");
    CppParser::parseFileBlocking(mParser, main, "");

    CppTypeResolver::EvaluationContext evaluation;
    CppTypeResolver resolver(mParser->statementList(), &evaluation,
                             {mParser->compilerType(), mParser->cppVersion()});
    TypeOperationRegistry operations(resolver);
    CppExpressionEvaluator evaluator(const_cast<StatementModel &>(mParser->statementList()),
                                     resolver, operations);
    auto evaluate = [&](const QString &text) {
        CppTokenizer tokenizer;
        tokenizer.tokenize({text});
        return evaluator.evaluate(tokenizer, {}, &evaluation);
    };
    EvalResult stringValue = evaluate("std::declval<std::string>()");
    QVERIFY2(stringValue.isValid(), "declval<string>");
    EvalResult directValue = evaluate("std::__declval<std::hash<std::string>>(0)");
    QVERIFY2(directValue.isValid(), "__declval<hash<string>>");
    EvalResult callable = evaluate("std::declval<std::hash<std::string>>()");
    QVERIFY2(callable.isValid(), "declval<hash<string>>");
    EvalResult call = evaluate(
                "std::declval<std::hash<std::string>>()(std::declval<const std::string&>())");
    QVERIFY2(call.isValid(), "hash<string>(string)");
    QVERIFY(call.invocation.statement);
    QCOMPARE(std::dynamic_pointer_cast<Statement>(call.invocation.statement)->noexceptKind,
             NoexceptKind::Always);
    QCOMPARE(resolver.evaluateNoexceptExpression(
                 "noexcept(std::declval<std::hash<std::string>>()"
                 "(std::declval<const std::string&>()))", {}, {}, &evaluation),
             ConstraintSatisfaction::Satisfied);
    PStatement string = mParser->findStatement("std::basic_string");
    QVERIFY(string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Invocable", {}), string);
}

void TestCppParserStandardLibrary::test_standard_library_nothrow_invocable()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {
        "#include <bits/stdc++.h>",
        "struct FalseType {};",
        "template<bool B> struct Selector { using type = FalseType; };",
        "template<> struct Selector<true> { using type = std::string; };",
        "using Nothrow = Selector<std::__is_nothrow_invocable<",
        "    const std::hash<std::string>&, const std::string&>::value>::type;",
        "using Fast = Selector<std::__is_fast_hash<std::hash<std::string>>::value>::type;",
        "using And = Selector<std::__and_<std::__is_fast_hash<std::hash<std::string>>,",
        "    std::__is_nothrow_invocable<const std::hash<std::string>&,",
        "    const std::string&>>::value>::type;",
        "using Not = Selector<std::__not_<std::__and_<",
        "    std::__is_fast_hash<std::hash<std::string>>,",
        "    std::__is_nothrow_invocable<const std::hash<std::string>&,",
        "    const std::string&>>>::value>::type;",
        "using Cache = Selector<std::__cache_default<",
        "    std::string, std::hash<std::string>>::value>::type;"
    }));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    if (mParser->compilerType() == CppCompilerType::Clang)
        QSKIP("This test uses libstdc++ internal traits");
    CppParser::parseFileBlocking(mParser, main, "");
    QVERIFY(mParser->findStatement("std::__is_nothrow_invocable"));
    PStatement string = mParser->findStatement("std::basic_string");
    QVERIFY(string);
    QStringList directNoexcept{
        "noexcept", "(", "std", "::", "declval", "<", "const", "std", "::", "hash",
        "<", "std", "::", "string", ">", "&", ">", "(", ")", "(",
        "std", "::", "declval", "<", "const", "std", "::", "string", "&", ">",
        "(", ")", ")", ")"
    };
    QVERIFY(mParser->evalExpression(main, directNoexcept, {}).isValid());
    PStatement falseType = mParser->findStatement("FalseType");
    QVERIFY(falseType);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Nothrow", {}), string);
    PStatement fastSpecialization = mParser->findTypeDefinitionOf(
                main, "std::__is_fast_hash<std::hash<std::string>>", {});
    QVERIFY(fastSpecialization);
    QVERIFY(fastSpecialization->definition.fileName.endsWith("basic_string.h"));
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Fast", {}), falseType);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "And", {}), falseType);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Not", {}), string);
    QCOMPARE(mParser->findTypeDefinitionOf(main, "Cache", {}), string);
}

void TestCppParserStandardLibrary::test_standard_library_parse_baseline()
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
    CppParser::parseFileBlocking(mParser, main, "");
    qWarning() << "STDLIB_TEST_TIMING completeParseMs" << timer.elapsed();
    qWarning() << "STDLIB_TEST_STATEMENTS" << mParser->statementList().count();
    QVERIFY(mParser->findStatement("std::vector"));
    QVERIFY(mParser->findStatement("std::basic_string::append"));
    PStatement variable = mParser->findStatementOf(main, "lst", 4);
    QVERIFY(variable);
    QCOMPARE(variable->command, "lst");
}

void TestCppParserStandardLibrary::test_standard_library_public_api_complete()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QString other = tmp.filePath("other.cpp");
    QVERIFY(writeLines(main, {"#include <bits/stdc++.h>"}));
    QVERIFY(writeLines(other, {"// unrelated translation unit"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));

    QElapsedTimer testTimer;
    testTimer.start();
    CppParser::parseFileBlocking(mParser, main, "");
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

    CppParser::parseFileBlocking(mParser, other, "");
    CppParser::parseFileBlocking(mParser, main, "");
    qWarning() << "STDLIB_TEST_TIMING secondParse" << testTimer.restart();
    QCOMPARE(standardLibraryStatementNames(mParser), firstStatements);
    qWarning() << "STDLIB_TEST_TIMING secondCollectAndCompare" << testTimer.restart();
    QCOMPARE(mParser->statementList().count(), statementCount);
    QCOMPARE(mParser->preprocessorEventCountsForTest(bitsStdcpp).scan, 1);
    QVERIFY(mParser->preprocessorEventCountsForTest(bitsStdcpp).replay > 0);
    mParser.reset();
    qWarning() << "STDLIB_TEST_TIMING destruction" << testTimer.restart();
}

void TestCppParserStandardLibrary::test_aino_replay_and_lambda_navigation()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString other = tmp.filePath("other.cpp");
    QVERIFY(writeLines(other, {"// unrelated translation unit"}));
    QString main = QFileInfo("resources/aino/untitled1.cpp").absoluteFilePath();
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    CppParser::parseFileBlocking(mParser, main, "");
    int statementCount = mParser->statementList().count();
    PStatement ui = mParser->findStatementOf(main, "ui", 13);
    QVERIFY(ui);
    QCOMPARE(ui->kind, StatementKind::Parameter);
    PStatement col = mParser->findStatementOf(main, "col", 16);
    QVERIFY(col);
    QCOMPARE(col->kind, StatementKind::Variable);
    QCOMPARE(col->scope, StatementScope::Local);

    CppParser::parseFileBlocking(mParser, other, "");
    CppParser::parseFileBlocking(mParser, main, "");
    QCOMPARE(mParser->statementList().count(), statementCount);
    QCOMPARE(mParser->findStatementOf(main, "ui", 13)->kind, StatementKind::Parameter);
    QCOMPARE(mParser->findStatementOf(main, "col", 16)->kind, StatementKind::Variable);
    QString windowsHeader = mParser->getHeaderFileName(main, "<windows.h>");
    QVERIFY(!windowsHeader.isEmpty());
    QCOMPARE(mParser->preprocessorEventCountsForTest(windowsHeader).scan, 1);
    QVERIFY(mParser->preprocessorEventCountsForTest(windowsHeader).replay > 0);
}

#endif
