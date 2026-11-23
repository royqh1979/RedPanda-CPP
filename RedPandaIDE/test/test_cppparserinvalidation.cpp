#include <QTest>
#include <QTemporaryDir>
#include "test_cppparserinvalidation.h"

void TestCppParserInvalidation::test_shared_boundary_statement_invalidation()
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
    mParser->invalidateFileAndDependents(a);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("a_value") != nullptr);
    QVERIFY(mParser->findStatement("b_value") != nullptr);
    QVERIFY(mParser->findStatement("shared_value") != nullptr);
}

void TestCppParserInvalidation::test_shared_boundary_statement_invalidation_only()
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

    mParser->invalidateFileAndDependents(a);
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("a_value") != nullptr);
    QVERIFY(mParser->findStatement("b_value") != nullptr);
    QVERIFY(mParser->findStatement("shared_value") != nullptr);
}

void TestCppParserInvalidation::test_invalidation_preserves_earlier_boundary_record()
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

    mParser->invalidateFileAndDependents(changed);
    QVERIFY(mParser->hasReplayRecord(earlier));
    QVERIFY(!mParser->hasReplayRecord(changed));
    CppParser::parseFileBlocking(mParser, root, false, "");
    QCOMPARE(mParser->findStatement("plain_b"), before);
}

void TestCppParserInvalidation::test_removed_boundary_include_remains_inactive_cached_record()
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
    mParser->invalidateFileAndDependents(a);
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

void TestCppParserInvalidation::test_repeated_template_include_preserves_each_context()
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

void TestCppParserInvalidation::test_shared_template_statement_ownership()
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

void TestCppParserInvalidation::test_statement_ownership_declaration_definition_invalidation()
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

    mParser->invalidateFileAndDependents(definition);
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

    mParser->invalidateSingleFileForTest(declaration);
    statement = mParser->findStatement("declared_function");
    QVERIFY(statement);
    QVERIFY(statementHasDefinition(statement));
    QVERIFY(statement->declarations.isEmpty());
    QCOMPARE(statement->definition.fileName, definition);
    QCOMPARE(statement->definition.boundaryAncestor, definition);
}

void TestCppParserInvalidation::test_extern_declaration_invalidation_recomputes_variable_properties()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString declaration = tmp.filePath("declare.h");
    QString definition = tmp.filePath("define.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(declaration, {"extern int value;"}));
    QVERIFY(writeLines(definition, {"int value=1;"}));
    QVERIFY(writeLines(main, {"#include \"declare.h\"", "#include \"define.h\""}));
    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, true);
    CppParser::parseFileBlocking(mParser, main, false, "");

    PStatement statement = mParser->findStatement("value");
    QVERIFY(statement && statementHasDefinition(statement));
    QVERIFY(std::dynamic_pointer_cast<Statement>(statement)
            ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
    mParser->invalidateSingleFileForTest(declaration);
    statement = mParser->findStatement("value");
    QVERIFY(statement && statementHasDefinition(statement));
    QVERIFY(statement->declarations.isEmpty());
    QVERIFY(!std::dynamic_pointer_cast<Statement>(statement)
             ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));

    CppParser::parseFileBlocking(mParser, main, false, "");
    statement = mParser->findStatement("value");
    QVERIFY(statement);
    QVERIFY(std::dynamic_pointer_cast<Statement>(statement)
            ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));

    QVERIFY(writeLines(definition, {"extern int value=1;"}));
    mParser->invalidateFileAndDependents(definition);
    CppParser::parseFileBlocking(mParser, main, false, "");
    statement = mParser->findStatement("value");
    QVERIFY(statement && statementHasDefinition(statement));
    mParser->invalidateSingleFileForTest(declaration);
    statement = mParser->findStatement("value");
    QVERIFY(statement && statementHasDefinition(statement));
    QVERIFY(std::dynamic_pointer_cast<Statement>(statement)
            ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
}

void TestCppParserInvalidation::test_static_function_definition_invalidation_does_not_create_extern()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString declaration = tmp.filePath("declare.h");
    QString definition = tmp.filePath("define.h");
    QString main = tmp.filePath("main.cpp");
    QVERIFY(writeLines(declaration, {"static void f();"}));
    QVERIFY(writeLines(definition, {"static void f() {}"}));
    QVERIFY(writeLines(main, {"#include \"declare.h\"", "#include \"define.h\""}));
    mParser->addIncludePath(tmp.path());
    mParser->addProjectFile(main, true);
    CppParser::parseFileBlocking(mParser, main, false, "");

    PStatement statement = mParser->findStatement("f");
    QVERIFY(statement && statementHasDefinition(statement));
    mParser->invalidateSingleFileForTest(definition);
    statement = mParser->findStatement("f");
    QVERIFY(statement && !statementHasDefinition(statement));
    QVERIFY(statementIsStatic(statement));
    QVERIFY(!std::dynamic_pointer_cast<Statement>(statement)
             ->declSpecifierProperties.testFlag(DeclSpecifierProperty::Extern));
    QCOMPARE(statement->declarations.count(), 1);

    CppParser::parseFileBlocking(mParser, main, false, "");
    statement = mParser->findStatement("f");
    QVERIFY(statement && statementHasDefinition(statement));
    QVERIFY(statementIsStatic(statement));
}

void TestCppParserInvalidation::test_last_variable_occurrence_invalidation_removes_statement()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{"int value;"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, "text.cpp", false, "");
    QVERIFY(mParser->findStatement("value"));
    mParser->invalidateSingleFileForTest("text.cpp");
    QVERIFY(!mParser->findStatement("value"));
}

void TestCppParserInvalidation::test_boundary_invalidation_removes_defines_and_statements()
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

    mParser->invalidateFileAndDependents(header);
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

void TestCppParserInvalidation::test_replay_reuses_existing_statements()
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

void TestCppParserInvalidation::test_system_subtree_inheritance_uses_boundary_record()
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

void TestCppParserInvalidation::test_file_info_for_boundary_children()
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

    mParser->invalidateSingleFileForTest(inner);
    QVERIFY(!mParser->findFileInfo(inner));
    QVERIFY(!mParser->findStatement("Inner"));
    QVERIFY(mParser->findFileInfo(root));
    QVERIFY(mParser->findStatement("Root"));
}

void TestCppParserInvalidation::test_cross_boundary_declaration_definition_file_info()
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

            mParser->invalidateFileAndDependents(roles[declarationRole].fileName);
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

            mParser->invalidateFileAndDependents(roles[definitionRole].fileName);
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

void TestCppParserInvalidation::test_user_header_change_invalidates_dependent_roots()
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
    mParser->invalidateFileAndDependents(leaf);
    QVERIFY(!mParser->findStatement("leaf_old"));
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("leaf_new"));
    QVERIFY(mParser->findStatement("middle_value"));
    QVERIFY(mParser->findStatement("main_value"));
}

void TestCppParserInvalidation::test_shared_internal_header_change_invalidates_boundary_ancestors()
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
    mParser->invalidateFileAndDependents(inner);
    QVERIFY(!mParser->findStatement("shared_old"));
    CppParser::parseFileBlocking(mParser, main, false, "");
    QVERIFY(mParser->findStatement("shared_new"));
    QVERIFY(mParser->findStatement("a_value"));
    QVERIFY(mParser->findStatement("b_value"));
    QCOMPARE(mParser->statementList().count(), oldCount);
}

void TestCppParserInvalidation::test_root_switch_with_macro_change_removes_stale_statements()
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

void TestCppParserInvalidation::test_root_switch_with_include_order_change_removes_stale_statements()
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

void TestCppParserInvalidation::test_internal_header_disk_change_removes_stale_statements()
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

void TestCppParserInvalidation::test_bits_stdcpp_not_rescanned_when_switching_roots()
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

void TestCppParserInvalidation::test_reset_parser_clears_registry_and_statements()
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

    mParser->resetForReuse();
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

void TestCppParserInvalidation::test_auto_invalidation_preserves_definition_after_declaration_change()
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
