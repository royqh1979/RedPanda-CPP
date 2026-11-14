/*
 * Copyright (C) 2026 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "test_widgets.h"

#include <QListView>
#include <QSet>
#include <QTest>
#ifdef REDPANDA_PARSER_STDLIB_TEST
#include <QTemporaryDir>
#include "test_parser_utils.h"
#endif
#include "src/colorscheme.h"
#include "src/cpprefacter.h"
#include "src/editor.h"
#include "src/editormanager.h"
#include "src/mainwindow.h"
#include "src/project.h"
#include "src/settings.h"
#include "src/widgets/classbrowser.h"
#include "src/widgets/codecompletionpopup.h"
#include "src/widgets/searchresultview.h"
#include <qsynedit/formatter/cppformatter.h>
#include <qsynedit/syntaxer/cpp.h>

IconsManager *MainWindow::iconsManager() const
{
    return nullptr;
}

std::shared_ptr<Project> MainWindow::project()
{
    return nullptr;
}

EditorManager *MainWindow::editorManager() const
{
    return nullptr;
}

SearchResultModel *MainWindow::searchResultModel()
{
    return nullptr;
}

QList<PProjectUnit> Project::unitList()
{
    return {};
}

ProjectOptions &Project::options()
{
    static ProjectOptions options;
    return options;
}

bool EditorManager::getContentFromOpenedEditor(const QString &, QStringList &) const
{
    return false;
}

Editor *EditorManager::getOpenedEditor(const QString &) const
{
    return nullptr;
}

PSearchResults SearchResultModel::addSearchResults(const QString &, const QString &,
                                                   SearchFileScope)
{
    return {};
}

void SearchResultModel::addResultToSearchResults(PSearchResults, PSearchResultTreeItem)
{
}

const QString &ProjectUnit::fileName() const
{
    static const QString empty;
    return empty;
}

const QByteArray &ProjectUnit::encoding() const
{
    static const QByteArray empty;
    return empty;
}

namespace {
QSet<QString> completionNames(CodeCompletionPopup &popup, const QString &typeName,
                              const QString &fileName,
                              CodeCompletionType type = CodeCompletionType::Normal,
                              const QString &preWord = {}, const QString &searchText = {},
                              const QSet<QString> &customKeywords = {}, int line = 0,
                              const QString &memberOperator = ".",
                              const QStringList &memberExpression = {"a"})
{
    popup.setShowEditorCaretFunc([] {});
    popup.setKeypressedCallback([](QKeyEvent *) { return false; });
    popup.setCurrentScope(popup.parser()->findScopeStatement(fileName, line));
    if (type == CodeCompletionType::Normal) {
        if (typeName.isEmpty()) {
            popup.prepareSearch(preWord, {}, "", {searchText}, fileName, line, type, customKeywords);
        } else {
            popup.prepareSearch(preWord, {typeName}, memberOperator, memberExpression,
                                fileName, line, type, customKeywords);
        }
    } else {
        popup.prepareSearch(preWord, {}, "", {typeName}, fileName, line, type, customKeywords);
    }
    popup.search(searchText, false, "default");
    QListView *view = popup.findChild<QListView *>();
    if (!view)
        return {};
    CodeCompletionListModel *model = qobject_cast<CodeCompletionListModel *>(view->model());
    if (!model)
        return {};
    QSet<QString> result;
    for (int i = 0; i < model->rowCount({}); ++i) {
        PStatement statement = model->statement(model->index(i, 0));
        if (statement)
            result.insert(statement->command);
    }
    return result;
}

QModelIndex findNode(QAbstractItemModel &model, const QModelIndex &parent,
                     const QString &command)
{
    for (int i = 0; i < model.rowCount(parent); ++i) {
        QModelIndex index = model.index(i, 0, parent);
        ClassBrowserNode *node = static_cast<ClassBrowserNode *>(index.internalPointer());
        if (node && node->statement && node->statement->command == command)
            return index;
    }
    return {};
}

QSet<QString> childCommands(QAbstractItemModel &model, const QModelIndex &parent)
{
    QSet<QString> result;
    for (int i = 0; i < model.rowCount(parent); ++i) {
        ClassBrowserNode *node = static_cast<ClassBrowserNode *>(
                    model.index(i, 0, parent).internalPointer());
        if (node && node->statement)
            result.insert(node->statement->command);
    }
    return result;
}
}

TestWidgets::TestWidgets(QObject *parent) : QObject(parent)
{
}

TestWidgets::~TestWidgets() = default;

void TestWidgets::initTestCase()
{
    mSettings = std::make_unique<Settings>("test-widgets.ini");
    pSettings = mSettings.get();
    pSettings->dirs().load();
    pSettings->ui().load();
    pSettings->codeCompletion().load();
    mColorManager = std::make_unique<ColorManager>(&pSettings->dirs());
}

void TestWidgets::cleanupTestCase()
{
    mColorManager.reset();
    mSettings.reset();
    pSettings = nullptr;
}

void TestWidgets::init()
{
    mMainFile = "/tmp/widgets.cpp";
    mMainContent = QStringList{
        "struct Base {",
        "  Base();",
        "  int publicValue;",
        "  virtual void action();",
        "  void action(int);",
        "protected:",
        "  int protectedValue;",
        "private:",
        "  int privateValue;",
        "};",
        "class Derived : Base { public: void action() override; };",
        "struct PublicDerived : Base {};",
        "Derived d;",
        "PublicDerived p;",
        "class Foo {",
        "public:",
        "  Foo();",
        "};",
        "Foo::Foo() {}",
        "int declaredOnly();",
        "int definedFunction() { return 1; }",
        "namespace TestNamespace { int value; }",
        "#define TEST_MACRO 1",
        "int operator\"\"_test(const char*, unsigned long);",
        "void labelFunction() { retry: ; }",
        "void notLabels() { bool flag = true; flag ? retry : retry; "
        "switch (flag) { case true: break; default: break; } }"
    };
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream([this](const QString &, QStringList &buffer) {
        buffer = mMainContent;
        return true;
    });
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
}

void TestWidgets::test_code_completion_inherited_access()
{
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QSet<QString> derivedNames = completionNames(popup, "d", mMainFile);
    QSet<QString> publicNames = completionNames(popup, "p", mMainFile);
    QCOMPARE(derivedNames, QSet<QString>{"action"});
    QCOMPARE(publicNames, QSet<QString>({"action", "publicValue"}));
}

void TestWidgets::test_code_completion_complex_keyword()
{
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::ComplexKeyword, "long"),
             QSet<QString>({"long", "double", "int"}));
}

void TestWidgets::test_code_completion_labels()
{
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    int line = mMainContent.indexOf("void labelFunction() { retry: ; }");
    QVERIFY(line >= 0);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::Labels,
                             "", "retry", {}, line),
             QSet<QString>{"retry"});
    CodeCompletionPopup negativePopup(mColorManager.get(), nullptr);
    negativePopup.setParser(mParser);
    int negativeLine = mMainContent.indexOf(
                "void notLabels() { bool flag = true; flag ? retry : retry; "
                "switch (flag) { case true: break; default: break; } }");
    QVERIFY(negativeLine >= 0);
    QCOMPARE(completionNames(negativePopup, "", mMainFile, CodeCompletionType::Labels,
                             "", "retry", {}, negativeLine),
             QSet<QString>{});
}

void TestWidgets::test_code_completion_function_without_definition()
{
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QVERIFY(mParser->findStatement("declaredOnly"));
    int line = mMainContent.indexOf("int declaredOnly();");
    QVERIFY(line >= 0);
    QCOMPARE(completionNames(popup, "int", mMainFile,
                             CodeCompletionType::FunctionWithoutDefinition,
                             "", "declaredOnly", {}, line),
             QSet<QString>{"declaredOnly"});
}

void TestWidgets::test_code_completion_namespaces()
{
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    PStatement namespaceStatement = mParser->findStatement("TestNamespace");
    QVERIFY(namespaceStatement);
    QCOMPARE(namespaceStatement->fileName, mMainFile);
    QVERIFY(mParser->namespaces().contains("TestNamespace"));
    PStatementList namespaces = mParser->findNamespace("TestNamespace");
    QVERIFY(namespaces && !namespaces->isEmpty());
    int line = mMainContent.indexOf("namespace TestNamespace { int value; }");
    QVERIFY(line >= 0);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::Namespaces,
                             "", "TestNamespace", {}, line),
             QSet<QString>{"TestNamespace"});
}

void TestWidgets::test_code_completion_inline_namespaces()
{
    mMainContent = QStringList{
        "namespace std {",
        "  inline namespace __cxx11 {",
        "    class basic_string {};",
        "    using string = basic_string;",
        "  }",
        "}"
    };
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream([this](const QString &, QStringList &buffer) {
        buffer = mMainContent;
        return true;
    });
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    PStatement namespaceStatement = mParser->findStatement("std");
    QVERIFY(namespaceStatement);
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QSet<QString> names = completionNames(popup, "std", mMainFile, CodeCompletionType::Normal,
                                          {}, {}, {}, mMainContent.count(), "::", {});
    QVERIFY(names.contains("basic_string"));
    QVERIFY(names.contains("string"));
}

#ifdef REDPANDA_PARSER_STDLIB_TEST
void TestWidgets::test_code_completion_standard_library_string()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QFile file(main);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("#include <string>\n");
    file.close();
    QString bitsStdcpp;
    mParser = std::make_shared<CppParser>();
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QSet<QString> names = completionNames(popup, "std", main, CodeCompletionType::Normal,
                                          {}, {}, {}, 1000000, "::", {});
    QVERIFY(names.contains("basic_string"));
    QVERIFY(names.contains("string"));
    QVERIFY(!names.contains(""));
}

void TestWidgets::test_code_completion_standard_library_vector_subscript()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QFile file(main);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("#include <bits/stdc++.h>\n\n"
               "int main() {\n"
               "    std::vector<std::string> lst;\n"
               "    std::string s;\n"
               "    lst[0].\n"
               "}\n");
    file.close();
    QString bitsStdcpp;
    mParser = std::make_shared<CppParser>();
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");

    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    popup.setShowEditorCaretFunc([] {});
    popup.setKeypressedCallback([](QKeyEvent *) { return false; });
    popup.setCurrentScope(mParser->findScopeStatement(main, 5));
    popup.prepareSearch("", {"lst", "[", "0", "]"}, ".", {}, main, 5,
                        CodeCompletionType::Normal, {});
    popup.search("", false, "default");
    QListView *view = popup.findChild<QListView *>();
    QVERIFY(view);
    CodeCompletionListModel *model = qobject_cast<CodeCompletionListModel *>(view->model());
    QVERIFY(model);
    QSet<QString> names;
    for (int i = 0; i < model->rowCount({}); ++i) {
        PStatement statement = model->statement(model->index(i, 0));
        if (statement)
            names.insert(statement->command);
    }
    QVERIFY(names.contains("size"));
    QVERIFY(names.contains("empty"));
}

void TestWidgets::test_goto_standard_library_string()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QStringList content{"#include <string>", "std::string value;"};
    QFile file(main);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(content.join('\n').toUtf8());
    file.close();
    QString bitsStdcpp;
    mParser = std::make_shared<CppParser>();
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");
    PStatement stringAlias = mParser->findStatementOf(main, {"std", "::", "string"}, 1);
    QVERIFY(stringAlias);
    QVERIFY(stringAlias->hasDefinition());

    QString targetFile;
    QSynedit::CharPos target{-1, -1};
    QObject context;
    Editor editor;
    editor.setEditorSettings(&pSettings->editor());
    editor.setColorManager(mColorManager.get());
    editor.setFilename(main);
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor.applySettings();
    editor.setFileType(FileType::CppSource);
    editor.loadContent(content);
    editor.setCppParser(mParser);
    connect(&editor, &Editor::openFileRequested, &context,
            [&targetFile, &target](const QString &fileName, FileType, const QString &,
                                   const QSynedit::CharPos &caretPos) {
        targetFile = fileName;
        target = caretPos;
    });
    editor.gotoDefinition({5, 1});
    QCOMPARE(targetFile, stringAlias->definitionFileName);
    QCOMPARE(target.line, stringAlias->definitionRange.start.line);
}
#endif

void TestWidgets::test_code_completion_types()
{
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QVERIFY(mParser->findStatement("Foo"));
    int line = mMainContent.indexOf("class Foo {");
    QVERIFY(line >= 0);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::Types,
                             "", "Foo", {}, line),
             QSet<QString>{"Foo"});
}

void TestWidgets::test_code_completion_macros()
{
    mParser->addHardDefineByLine("#define HARD_TEST_MACRO 9");
    mParser->parseHardDefines();
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    PStatement macro = mParser->findStatement("TEST_MACRO");
    QVERIFY(macro);
    QCOMPARE(macro->kind, StatementKind::Preprocessor);
    QCOMPARE(macro->value, "1");
    QCOMPARE(macro->fileName, mMainFile);
    int line = mMainContent.indexOf("#define TEST_MACRO 1");
    QVERIFY(line >= 0);
    QCOMPARE(macro->declarationRange.start.line, line);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::Macros,
                             "", "TEST_MACRO", {}, line),
             QSet<QString>({"TEST_MACRO", "HARD_TEST_MACRO"}));
    PStatement hardMacro = mParser->findStatement("HARD_TEST_MACRO");
    QVERIFY(hardMacro);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::Macros,
                             "", "HARD_TEST_MACRO", {}, line),
             QSet<QString>{"HARD_TEST_MACRO"});
}

void TestWidgets::test_code_completion_normal_includes_macros()
{
    mParser->addHardDefineByLine("#define HARD_TEST_MACRO 9");
    mParser->parseHardDefines();
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    int line = mMainContent.indexOf("#define TEST_MACRO 1");
    QVERIFY(line >= 0);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::Normal,
                             "", "TEST_MACRO", {}, line),
             QSet<QString>({"TEST_MACRO", "HARD_TEST_MACRO"}));
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::Normal,
                             "", "HARD_TEST_MACRO", {}, line),
             QSet<QString>{"HARD_TEST_MACRO"});
}

void TestWidgets::test_code_completion_keywords_only()
{
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::KeywordsOnly,
                             "", "", {"custom_keyword"}),
             QSet<QString>{"custom_keyword"});
}

void TestWidgets::test_code_completion_operator_overloads_are_excluded()
{
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QVERIFY(mParser->findStatement("operator\"\"_test"));
    int line = mMainContent.indexOf("int operator\"\"_test(const char*, unsigned long);");
    QVERIFY(line >= 0);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::Normal,
                             "", "operator\"\"_test", {}, line),
             QSet<QString>{});
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::LiteralOperators,
                             "", "operator\"\"_test", {}, line),
             QSet<QString>{"operator\"\"_test"});
}

void TestWidgets::test_code_completion_exact_match_first()
{
    mMainContent = QStringList{"int s;"};
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream([this](const QString &, QStringList &buffer) {
        buffer = mMainContent;
        return true;
    });
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");

    auto snippet = std::make_shared<CodeSnippet>();
    snippet->prefix = "switch";
    snippet->code = "switch (value) {}";
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    popup.setCodeSnippets({snippet});
    popup.setShowEditorCaretFunc([] {});
    popup.prepareSearch("", {}, "", {"s"}, mMainFile, 0,
                        CodeCompletionType::Normal, {});
    popup.search("s", false, "default");
    QListView *view = popup.findChild<QListView *>();
    QVERIFY(view);
    auto *model = qobject_cast<CodeCompletionListModel *>(view->model());
    QVERIFY(model);
    QCOMPARE(model->rowCount({}), 2);
    QCOMPARE(model->statement(model->index(0, 0))->command, "s");
    QCOMPARE(model->statement(model->index(1, 0))->command, "switch");
}

void TestWidgets::test_class_browser_inherited_members()
{
    ClassBrowserModel model;
    model.setParser(mParser);
    model.setCurrentFile(mMainFile);
    model.fillStatements();
    QModelIndex derived = findNode(model, {}, "Derived");
    QVERIFY(derived.isValid());
    QSet<QString> members = childCommands(model, derived);
    QCOMPARE(members, QSet<QString>({"action", "publicValue", "protectedValue", "privateValue"}));
    QVERIFY(findNode(model, derived, "publicValue").isValid());
}

void TestWidgets::test_goto_class_declaration_and_definition()
{
    Editor editor;
    editor.setEditorSettings(&pSettings->editor());
    editor.setColorManager(mColorManager.get());
    editor.setFilename(mMainFile);
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor.applySettings();
    editor.setFileType(FileType::CppSource);
    editor.loadContent(mMainContent);
    editor.setCppParser(mParser);
    QCOMPARE(editor.fileType(), FileType::CppSource);
    QCOMPARE(editor.document()->syntaxerLanguage(), QSynedit::ProgrammingLanguage::CPP);
    QCOMPARE(editor.syntaxer()->language(), QSynedit::ProgrammingLanguage::CPP);
    QCOMPARE(editor.document()->count(), mMainContent.count());
    PStatement classStatement = mParser->findStatementOf(mMainFile, QStringList{"Foo"}, 0);
    QVERIFY(classStatement);
    QSynedit::CharPos classPos{6, classStatement->declarationRange.start.line};
    QCOMPARE(editor.getExpressionAtPosition(classPos), QStringList{"Foo"});
    QSynedit::CharPos target{-1, -1};
    connect(&editor, &Editor::openFileRequested,
            [&target](const QString &, FileType, const QString &,
                      const QSynedit::CharPos &caretPos) {
        target = caretPos;
    });
    editor.gotoDefinition(classPos);
    QCOMPARE(target.line, classStatement->definitionRange.start.line);
    target = {-1, -1};
    editor.gotoDeclaration(classPos);
    QCOMPARE(target.line, classStatement->declarationRange.start.line);
}

void TestWidgets::test_goto_constructor_declaration_and_definition()
{
    Editor editor;
    editor.setEditorSettings(&pSettings->editor());
    editor.setColorManager(mColorManager.get());
    editor.setFilename(mMainFile);
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor.applySettings();
    editor.setFileType(FileType::CppSource);
    editor.loadContent(mMainContent);
    editor.setCppParser(mParser);
    PStatement classStatement = mParser->findStatementOf(mMainFile, QStringList{"Foo"}, 0);
    QVERIFY(classStatement);
    PStatement constructorStatement;
    for (const std::weak_ptr<Statement> &weak : classStatement->children.values("Foo")) {
        PStatement candidate = weak.lock();
        if (candidate && candidate->kind == StatementKind::Constructor) {
            constructorStatement = candidate;
            break;
        }
    }
    QVERIFY(constructorStatement);
    QSynedit::CharPos constructorPos{2, constructorStatement->declarationRange.start.line};
    QCOMPARE(editor.getExpressionAtPosition(constructorPos), QStringList{"Foo"});
    PStatement resolved = mParser->findStatementOf(
                mMainFile, QStringList{"Foo"}, constructorPos.line);
    QVERIFY(resolved);
    QCOMPARE(resolved->kind, StatementKind::Constructor);
    QCOMPARE(editor.resolveClassOrConstructorAt(resolved, constructorPos), constructorStatement);
    QSynedit::CharPos target{-1, -1};
    connect(&editor, &Editor::openFileRequested,
            [&target](const QString &, FileType, const QString &,
                      const QSynedit::CharPos &caretPos) {
        target = caretPos;
    });
    editor.gotoDefinition(constructorPos);
    QCOMPARE(target.line, constructorStatement->definitionRange.start.line);
    target = {-1, -1};
    QSynedit::CharPos definitionPos{5, constructorStatement->definitionRange.start.line};
    editor.gotoDeclaration(definitionPos);
    QCOMPARE(target.line, constructorStatement->declarationRange.start.line);
    QString hint = editor.getParserHint({"Foo"}, constructorPos);
    QVERIFY(hint.contains("constructor"));
}

void TestWidgets::test_goto_statement_definitions()
{
    mMainContent = QStringList{
        "extern int external;",
        "int global;",
        "typedef int IntAlias;",
        "using DoubleAlias = double;",
        "enum Color { red, green };",
        "namespace LongName { int value; }",
        "namespace Short = LongName;",
        "class Forward;",
        "struct Defaulted { Defaulted() = default; };",
        "void deleted();",
        "void deleted() = delete;",
        "void declared();",
        "void withParam(int param) {",
        "    int local = param;",
        "    return local;",
        "}",
        "void labelFunction() {",
        "    retry: ;",
        "    goto retry;",
        "}",
        "#define MACRO_VALUE 1",
        "int use() { return global + red + MACRO_VALUE; }",
        "IntAlias aliasValue;",
        "DoubleAlias doubleValue;",
        "int namespaceValue = Short::value;",
        "Forward *forwardValue;",
        "int useExternal() { return external; }",
        "int useDeclared() { return declared(); }",
        "int useDeleted() { return deleted(); }"
    };
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream([this](const QString &, QStringList &buffer) {
        buffer = mMainContent;
        return true;
    });
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");

    QVERIFY(!mParser->findStatement("deleted"));
    for (const char *name : {"global", "IntAlias", "DoubleAlias", "red", "Short",
                             "Defaulted::Defaulted"}) {
        PStatement statement = mParser->findStatement(QString::fromLatin1(name));
        QVERIFY2(statement, name);
        QVERIFY2(statement->hasDefinition(), name);
    }
    for (int line : {13, 14, 18}) {
        QString name = line == 13 ? "param" : line == 14 ? "local" : "retry";
        PStatement statement = mParser->findStatementOf(mMainFile, QStringList{name}, line);
        QVERIFY2(statement, name.toUtf8().constData());
        QVERIFY(statement->hasDefinition());
    }
    for (const char *name : {"external", "declared", "Forward"}) {
        PStatement statement = mParser->findStatement(QString::fromLatin1(name));
        QVERIFY2(statement, name);
        QVERIFY2(!statement->hasDefinition(), name);
    }

    QSynedit::CharPos target{-1, -1};
    QObject context;
    Editor editor;
    editor.setEditorSettings(&pSettings->editor());
    editor.setColorManager(mColorManager.get());
    editor.setFilename(mMainFile);
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor.applySettings();
    editor.setFileType(FileType::CppSource);
    editor.loadContent(mMainContent);
    editor.setCppParser(mParser);
    connect(&editor, &Editor::openFileRequested, &context,
            [&target](const QString &, FileType, const QString &,
                      const QSynedit::CharPos &caretPos) {
        target = caretPos;
    });

    auto positionOf = [&](const QString &lineText,
                          const QString &identifier) -> QSynedit::CharPos {
        int line = mMainContent.indexOf(lineText);
        if (line < 0)
            return {-1, -1};
        int ch = lineText.indexOf(identifier);
        if (ch < 0)
            return {-1, -1};
        return QSynedit::CharPos{ch, line};
    };
    auto checkGoto = [&](bool definition, const QString &lineText,
                         const QString &identifier, const QString &expectedLineText) {
        target = {-1, -1};
        QSynedit::CharPos pos = positionOf(lineText, identifier);
        QVERIFY2(pos.line >= 0, lineText.toUtf8().constData());
        if (definition)
            editor.gotoDefinition(pos);
        else
            editor.gotoDeclaration(pos);
        int expectedLine = expectedLineText.isEmpty()
                ? -1 : mMainContent.indexOf(expectedLineText);
        QCOMPARE(target.line, expectedLine);
    };

    const QString useLine = "int use() { return global + red + MACRO_VALUE; }";
    checkGoto(true, useLine, "global", "int global;");
    checkGoto(false, useLine, "global", "int global;");
    checkGoto(true, useLine, "red", "enum Color { red, green };");
    checkGoto(true, useLine, "MACRO_VALUE", "#define MACRO_VALUE 1");
    checkGoto(true, "IntAlias aliasValue;", "IntAlias", "typedef int IntAlias;");
    checkGoto(true, "DoubleAlias doubleValue;", "DoubleAlias", "using DoubleAlias = double;");
    checkGoto(true, "int namespaceValue = Short::value;", "Short",
              "namespace Short = LongName;");
    checkGoto(true, "    int local = param;", "param", "void withParam(int param) {");
    checkGoto(true, "    return local;", "local", "    int local = param;");
    checkGoto(true, "    goto retry;", "retry", "    retry: ;");
    checkGoto(true, "struct Defaulted { Defaulted() = default; };", "Defaulted()",
              "struct Defaulted { Defaulted() = default; };");
    checkGoto(true, "int useExternal() { return external; }", "external", "");
    checkGoto(false, "int useExternal() { return external; }", "external",
              "extern int external;");
    checkGoto(true, "int useDeclared() { return declared(); }", "declared", "");
    checkGoto(false, "int useDeclared() { return declared(); }", "declared",
              "void declared();");
    checkGoto(true, "int useDeleted() { return deleted(); }", "deleted", "");
    checkGoto(true, "Forward *forwardValue;", "Forward", "");
    checkGoto(false, "Forward *forwardValue;", "Forward", "class Forward;");
}

void TestWidgets::test_ctrl_hover_uses_pointing_hand_cursor()
{
    Editor editor;
    editor.setEditorSettings(&pSettings->editor());
    editor.setColorManager(mColorManager.get());
    editor.setFilename(mMainFile);
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor.applySettings();
    editor.setFileType(FileType::CppSource);
    editor.loadContent(mMainContent);
    editor.setCppParser(mParser);
    PStatement classStatement = mParser->findStatement("Foo");
    QVERIFY(classStatement);
    QSynedit::CharPos classPos{6, classStatement->declarationRange.start.line};
    QPoint point = editor.displayCoordToPixels(editor.bufferToDisplayPos(classPos))
            + QPoint(0, 2);
    QMouseEvent event(QEvent::MouseMove, point, Qt::NoButton, Qt::NoButton,
                      Qt::ControlModifier);
    QApplication::sendEvent(editor.viewport(), &event);
    QCOMPARE(editor.viewport()->cursor().shape(), Qt::PointingHandCursor);
}

void TestWidgets::test_function_tip_class()
{
    Editor editor;
    editor.setEditorSettings(&pSettings->editor());
    editor.setColorManager(mColorManager.get());
    editor.setFilename(mMainFile);
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor.applySettings();
    editor.setFileType(FileType::CppSource);
    editor.loadContent(mMainContent);
    editor.setCppParser(mParser);
    PStatement classStatement = mParser->findStatementOf(mMainFile, QStringList{"Foo"}, 0);
    QVERIFY(classStatement);
    QSynedit::CharPos classPos{6, classStatement->declarationRange.start.line};
    QString hint = editor.getParserHint({"Foo"}, classPos);
    QVERIFY(hint.trimmed().startsWith("Foo"));
    QVERIFY(!hint.contains("constructor"));
}

void TestWidgets::test_rename_class_and_constructor()
{
    Editor editor;
    editor.setEditorSettings(&pSettings->editor());
    editor.setColorManager(mColorManager.get());
    editor.setFilename(mMainFile);
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor.applySettings();
    editor.setFileType(FileType::CppSource);
    editor.loadContent(mMainContent);
    editor.setCppParser(mParser);
    PStatement classStatement = mParser->findStatementOf(mMainFile, QStringList{"Foo"}, 0);
    QVERIFY(classStatement);
    QSynedit::CharPos classPos{6, classStatement->declarationRange.start.line};
    QStringList expectedContent = mMainContent;
    for (QString &lineText : expectedContent)
        lineText.replace("Foo", "Bar");
    CppRefacter refactor(nullptr);
    refactor.renameSymbol(&editor, classPos, "Bar");
    QCOMPARE(editor.content(), expectedContent);
}


void TestWidgets::test_rename_from_constructor()
{
    Editor editor;
    editor.setEditorSettings(&pSettings->editor());
    editor.setColorManager(mColorManager.get());
    editor.setFilename(mMainFile);
    editor.setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor.setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor.applySettings();
    editor.setFileType(FileType::CppSource);
    editor.loadContent(mMainContent);
    editor.setCppParser(mParser);
    PStatement classStatement = mParser->findStatementOf(mMainFile, QStringList{"Foo"}, 0);
    QVERIFY(classStatement);
    PStatement constructorStatement = classStatement->children.value("Foo").lock();
    QVERIFY(constructorStatement);
    QSynedit::CharPos constructorPos{2, constructorStatement->declarationRange.start.line};
    QStringList expectedContent = mMainContent;
    for (QString &lineText : expectedContent)
        lineText.replace("Foo", "Bar");
    CppRefacter refactor(nullptr);
    refactor.renameSymbol(&editor, constructorPos, "Bar");
    QCOMPARE(editor.content(), expectedContent);
}
