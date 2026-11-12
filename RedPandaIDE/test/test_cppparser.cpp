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

