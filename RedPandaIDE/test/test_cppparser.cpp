#include <QTest>
#include <QTemporaryDir>
#ifdef REDPANDA_PARSER_STDLIB_TEST
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>
#endif
#include "src/parser/cppparser.h"
#include "test_cppparser.h"

namespace {
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
QString latestChildDirectory(const QString &path)
{
    QDir dir(path);
    QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    if (names.isEmpty())
        return QString();
    return dir.filePath(names.last());
}

bool configureStandardLibraryParser(const PCppParser &parser, QString &bitsStdcpp)
{
    QString root = QStringLiteral(REDPANDA_PARSER_STDLIB_MINGW_ROOT);
    QString compiler = root + "/bin/g++.exe";
    QString cxxInclude = latestChildDirectory(root + "/include/c++");
    QString gccTarget = latestChildDirectory(root + "/lib/gcc");
    QString gccVersion = latestChildDirectory(gccTarget);
    if (cxxInclude.isEmpty() || gccVersion.isEmpty()) {
        qWarning() << "Cannot locate MinGW C++ include directories under" << root;
        return false;
    }
    bitsStdcpp = cxxInclude + "/bits/stdc++.h";
    if (!QFileInfo::exists(bitsStdcpp))
        bitsStdcpp = cxxInclude + "/x86_64-w64-mingw32/bits/stdc++.h";
    QStringList includePaths{
        cxxInclude,
        cxxInclude + "/x86_64-w64-mingw32",
        cxxInclude + "/backward",
        gccVersion + "/include",
        gccVersion + "/include-fixed",
        root + "/include"
    };
    for (const QString &path : includePaths) {
        if (QDir(path).exists())
            parser->addIncludePath(path);
    }
    if (!QFileInfo::exists(bitsStdcpp)) {
        qWarning() << "Cannot find" << bitsStdcpp;
        return false;
    }

    QTemporaryFile source(QDir::temp().filePath("redpanda-stdlib-XXXXXX.cpp"));
    if (!source.open()) {
        qWarning() << "Cannot create a temporary source for compiler defines";
        return false;
    }
    source.write("\n");
    source.close();
    QProcess process;
    process.start(compiler, {"-std=c++17", "-dM", "-E", source.fileName()});
    if (!process.waitForStarted() || !process.waitForFinished(30000)
            || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << "Cannot obtain compiler defines:" << process.errorString()
                   << process.readAllStandardError();
        return false;
    }
    const QStringList defines = QString::fromUtf8(process.readAllStandardOutput())
            .split('\n', Qt::SkipEmptyParts);
    for (const QString &define : defines)
        parser->addHardDefineByLine(define);
    return true;
}

void collectStatementNames(const PCppParser &parser, const PStatement &parent,
                           QSet<QString> &names)
{
    const StatementMap &children = parser->statementList().childrenStatements(parent);
    for (const PStatement &statement : children) {
        if (!statement->fullName.isEmpty())
            names.insert(statement->fullName);
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
    QCOMPARE(statement->type,"int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"const int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"extern const int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int");
    QCOMPARE(statement->args,"[4]");

    statement = mParser->findStatement("a6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int &");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int **");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int **");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a9");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a9");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a10");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a10");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("a11");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a11");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const int &");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"unsigned int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"const unsigned int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const unsigned int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"extern const unsigned int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static unsigned int");
    QCOMPARE(statement->args,"[4]");

    statement = mParser->findStatement("b6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static unsigned int *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static unsigned int *const");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b8");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b8");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static unsigned int **");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"std::string");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"const std::string");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s4");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s4");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s5");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s5");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string");
    QCOMPARE(statement->args,"[4]");

    statement = mParser->findStatement("s6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("s7");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"s7");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const std::string &");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("f1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"f1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const struct FILE *");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("f2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"f2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"static const struct FILE");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("ff1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statement->type,"int");
    QCOMPARE(statement->args,"( )");

    statement = mParser->findStatement("ff2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statement->type,"static const int");
    QCOMPARE(statement->args,"( )");

    statement = mParser->findStatement("ff3");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff3");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statement->type,"static const std::string");
    QCOMPARE(statement->args,"( )");

    //TODO:: ff4 ff5

    statement = mParser->findStatement("ff6");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"ff6");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(statement->properties & StatementProperty::FunctionPointer);
    QCOMPARE(statement->type,"int *");
    QCOMPARE(statement->args,"( )");

    statement = mParser->findStatement("aa1");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"aa1");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(!(statement->properties & StatementProperty::FunctionPointer));
    QCOMPARE(statement->type,"int (*)");
    QCOMPARE(statement->args,"[5]");

    statement = mParser->findStatement("aa2");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"aa2");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QVERIFY(!(statement->properties & StatementProperty::FunctionPointer));
    QCOMPARE(statement->type,"static const std::string (*)");
    QCOMPARE(statement->args,"[5]");

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
    QCOMPARE(statement->publicProperties.count(), 3);
    PStatement child;
    child = statement->publicProperties[0];
    QCOMPARE(child->command,"xxx");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::xxx");
    QCOMPARE(child->type, "int");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);
    child = statement->publicProperties[1];
    QCOMPARE(child->command,"yyy");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::yyy");
    QCOMPARE(child->type, "short");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);
    child = statement->publicProperties[2];
    QCOMPARE(child->command,"zzz");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::zzz");
    QCOMPARE(child->type, "double");
    QCOMPARE(child->accessibility, StatementAccessibility::Public);

    QCOMPARE(statement->children.count(),4);
    child = statement->children.value("ppp");
    QCOMPARE(child->command,"ppp");
    QCOMPARE(child->kind,StatementKind::Variable);
    QCOMPARE(child->fullName,"TTT::ppp");
    QCOMPARE(child->type, "int");
    QCOMPARE(child->accessibility, StatementAccessibility::Private);

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
    QVERIFY(mParser->fileScanned(earlier));
    QVERIFY(!mParser->fileScanned(changed));
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
    QCOMPARE(first->fileName, tpl);
    QCOMPARE(second->fileName, tpl);
    QCOMPARE(first->declarationBoundaryAncestor, host);
    QCOMPARE(second->declarationBoundaryAncestor, host);
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
    QCOMPARE(first->fileName, tpl);
    QCOMPARE(second->fileName, tpl);
    QCOMPARE(first->declarationBoundaryAncestor, a);
    QCOMPARE(second->declarationBoundaryAncestor, b);
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
    QVERIFY(statement->hasDefinition());
    QCOMPARE(statement->declarationBoundaryAncestor, declaration);
    QCOMPARE(statement->definitionBoundaryAncestor, definition);

    mParser->invalidateFile(definition);
    statement = mParser->findStatement("declared_function");
    QVERIFY(statement);
    QVERIFY(!statement->hasDefinition());
    QCOMPARE(statement->declarationBoundaryAncestor, declaration);
    QVERIFY(statement->definitionBoundaryAncestor.isEmpty());
    QCOMPARE(statement->definitionFileName, statement->fileName);

    CppParser::parseFileBlocking(mParser, main, false, "");
    statement = mParser->findStatement("declared_function");
    QVERIFY(statement);
    QVERIFY(statement->hasDefinition());
    QCOMPARE(statement->definitionBoundaryAncestor, definition);

    mParser->invalidateFileRecordForTest(declaration);
    statement = mParser->findStatement("declared_function");
    QVERIFY(statement);
    QVERIFY(statement->hasDefinition());
    QCOMPARE(statement->fileName, definition);
    QCOMPARE(statement->declarationBoundaryAncestor, definition);
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
    QVERIFY(fileInfo->defines()->contains("BOUNDARY_VALUE"));
    QVERIFY(mParser->findStatement("boundary_statement"));

    mParser->invalidateFile(header);
    QVERIFY(!mParser->findFileInfo(header));
    QVERIFY(!mParser->findStatement("boundary_statement"));

    CppParser::parseFileBlocking(mParser, main, false, "");
    fileInfo = mParser->findFileInfo(header);
    QVERIFY(fileInfo);
    QVERIFY(fileInfo->defines()->contains("BOUNDARY_VALUE"));
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

void TestCppParser::test_structured_bindings()
{
    mParser->setOnGetFileStream(nullptr);
    QFileInfo file("resources/structured-bindings.cpp");
    QVERIFY(file.exists());
    CppParser::parseFileBlocking(mParser,file.absoluteFilePath(),false,"");
    PStatement statement;
    statement = mParser->findStatement("a");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"a");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("b");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"b");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"short");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("c");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"c");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"double");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("d");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"d");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"int");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("e");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"e");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"short");
    QCOMPARE(statement->args,"");

    statement = mParser->findStatement("g");
    QVERIFY(statement!=nullptr);
    QCOMPARE(statement->fullName,"g");
    QCOMPARE(statement->kind,StatementKind::Variable);
    QCOMPARE(statement->type,"double");
    QCOMPARE(statement->args,"");
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
    QVERIFY(!mParser->findFileInfo(base));
    QVERIFY(mParser->findStatement("N::Base"));
    QVERIFY(mParser->findStatement("Derived"));
    QVERIFY(mParser->findStatement("Derived::inherited_value"));
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
    QVERIFY(!mParser->fileScanned(main));
    QVERIFY(!mParser->fileScanned(header));
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
    QVERIFY(statement && statement->hasDefinition());

    QVERIFY(writeLines(declaration, {"int declared_function();", "int declaration_marker;"}));
    CppParser::parseFileBlocking(mParser, main, false, "");
    statement = mParser->findStatement("declared_function");
    QVERIFY(statement && statement->hasDefinition());
    QCOMPARE(statement->declarationBoundaryAncestor, declaration);
    QCOMPARE(statement->definitionBoundaryAncestor, definition);
    QVERIFY(mParser->findStatement("declaration_marker"));
    QCOMPARE(mParser->preprocessorEventCountsForTest(declaration).scan, 2);
    QCOMPARE(mParser->preprocessorEventCountsForTest(definition).scan, 1);
    QVERIFY(mParser->preprocessorEventCountsForTest(definition).replay > 0);
}

void TestCppParser::test_cppreference_declarator_syntax()
{
    parseSyntaxResource(mParser, "declarators.cpp");
    PStatement statement = mParser->findStatement("a");
    QVERIFY(statement);
    QCOMPARE(statement->kind, StatementKind::Variable);
    QCOMPARE(statement->type, "int");

    statement = mParser->findStatement("p");
    QVERIFY(statement);
    QCOMPARE(statement->type, "int *");

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
    QCOMPARE(f1->declarationRange.start.line, 9);
    QCOMPARE(f1->definitionRange.start.line, 31);
    for (const char *name : {"f0", "f1", "fp03", "fp11", "main", "f2", "bar"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
    QCOMPARE(mParser->findStatement("f0")->kind, StatementKind::Function);
    QCOMPARE(mParser->findStatement("fp03")->kind, StatementKind::Function);
    QCOMPARE(mParser->findStatement("fp11")->kind, StatementKind::Function);
}

void TestCppParser::test_cppreference_class_syntax()
{
    parseSyntaxResource(mParser, "classes.cpp");
    PStatement d1 = mParser->findStatement("S::d1");
    QVERIFY(d1);
    QCOMPARE(d1->declarationRange.start.line, 7);
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
    QCOMPARE(mParser->findStatement("green")->value, "20");
    QCOMPARE(mParser->findStatement("blue")->value, "21");
    QCOMPARE(mParser->findStatement("d")->value, "0");
    QCOMPARE(mParser->findStatement("e")->value, "1");
    QCOMPARE(mParser->findStatement("f")->value, "3");
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
    QCOMPARE(mParser->findStatementOf(file, "x", 5)->declarationRange.start.line, 5);
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
    QCOMPARE(mParser->findStatementOf(file, "myset", 6)->declarationRange.start.line, 6);
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
    QCOMPARE(mParser->findStatementOf(file, "v", 5)->type, "std::vector<int>");
    QCOMPARE(mParser->findStatementOf(file, "a", 16)->args, "[]");
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

void TestCppParser::test_standard_library_public_api_complete()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(main, {"#include <bits/stdc++.h>"}));
    QString bitsStdcpp;
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));

    CppParser::parseFileBlocking(mParser, main, true, "");
    for (const char *name : {
         "std::string", "std::vector", "std::map", "std::set",
         "std::sort", "std::find", "std::cout", "std::endl",
         "std::numeric_limits", "std::basic_string::size",
         "std::vector::size", "std::vector::push_back", "std::map::find"}) {
        QVERIFY2(mParser->findStatement(QString::fromLatin1(name)), name);
    }
    QSet<QString> firstStatements = standardLibraryStatementNames(mParser);
    QVERIFY(firstStatements.count() > 100);
    int statementCount = mParser->statementList().count();

    CppParser::parseFileBlocking(mParser, main, true, "");
    QCOMPARE(standardLibraryStatementNames(mParser), firstStatements);
    QCOMPARE(mParser->statementList().count(), statementCount);
    QCOMPARE(mParser->preprocessorEventCountsForTest(bitsStdcpp).scan, 1);
    QVERIFY(mParser->preprocessorEventCountsForTest(bitsStdcpp).replay > 0);
}
#endif

