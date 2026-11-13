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

void TestWidgets::test_code_completion_literal_operators()
{
    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QVERIFY(mParser->findStatement("operator\"\"_test"));
    int line = mMainContent.indexOf("int operator\"\"_test(const char*, unsigned long);");
    QVERIFY(line >= 0);
    QCOMPARE(completionNames(popup, "", mMainFile, CodeCompletionType::LiteralOperators,
                             "", "operator\"\"_test", {}, line),
             QSet<QString>{"operator\"\"_test"});
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
