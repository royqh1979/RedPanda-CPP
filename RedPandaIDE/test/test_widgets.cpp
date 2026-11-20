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
#include "src/editor/cppexpressionextractor.h"

#include <QListView>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>
#ifdef REDPANDA_PARSER_STDLIB_TEST
#include "test_parser_utils.h"
#endif
#include "src/colorscheme.h"
#include "src/cpprefacter.h"
#include "src/editor/editor.h"
#include "src/editormanager.h"
#include "src/mainwindow.h"
#include "src/project.h"
#include "src/settings.h"
#include "src/settingsdialog/editorcolorschemewidget.h"
#include "src/utils.h"
#include "src/widgets/classbrowser.h"
#include "src/widgets/codecompletionpopup.h"
#include "src/widgets/functiontooltipwidget.h"
#include "src/widgets/searchresultview.h"
#include <qsynedit/formatter/cppformatter.h>
#include <qsynedit/syntaxer/cpp.h>

IconsManager *MainWindow::iconsManager() const
{
    return nullptr;
}

void MainWindow::updateEditorColorSchemes()
{
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
    int globalLine = mMainContent.count() - 1;
    QSet<QString> derivedNames = completionNames(popup, "d", mMainFile, CodeCompletionType::Normal,
                                                 "", "", {}, globalLine);
    QSet<QString> publicNames = completionNames(popup, "p", mMainFile, CodeCompletionType::Normal,
                                                "", "", {}, globalLine);
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
    QCOMPARE(statementDeclarationForNavigation(namespaceStatement, mMainFile).fileName, mMainFile);
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

void TestWidgets::test_code_completion_standard_library_make_shared()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QFile file(main);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("#include <bits/stdc++.h>\n\n"
               "int main() {\n"
               "    std::vector<std::string> lst;\n"
               "    auto p = std::make_shared<std::string>();\n"
               "    p->\n"
               "}\n");
    file.close();
    QString bitsStdcpp;
    mParser = std::make_shared<CppParser>();
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    mParser->addProjectFile(main, false);
    CppParser::parseFileBlocking(mParser, main, true, "");

    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QSet<QString> names = completionNames(popup, "p", main, CodeCompletionType::Normal,
                                          {}, {}, {}, 5, "->", {});
    QVERIFY(names.contains("size"));
    QVERIFY(names.contains("empty"));
    QVERIFY(names.contains("append"));
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
    QVERIFY(statementHasDefinition(stringAlias));

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
    QCOMPARE(targetFile, stringAlias->definition.fileName);
    QCOMPARE(target.line, stringAlias->definition.range.start.line);
}

void TestWidgets::test_class_browser_standard_library_header_context()
{
    QString bitsStdcpp;
    mParser = std::make_shared<CppParser>();
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    QDir includeDir = QFileInfo(bitsStdcpp).dir();
    QVERIFY(includeDir.cdUp());
    if (!QFileInfo::exists(includeDir.filePath("string")))
        QVERIFY(includeDir.cdUp());
    QString stringFile = includeDir.filePath("string");
    QString basicStringTcc = includeDir.filePath("bits/basic_string.tcc");
    if (!QFileInfo::exists(stringFile) || !QFileInfo::exists(basicStringTcc))
        QSKIP("This test requires the libstdc++ basic_string.tcc layout");

    CppParser::parseFileBlocking(mParser, basicStringTcc, false, stringFile);

    PParsedFileInfo fileInfo = mParser->findFileInfo(basicStringTcc);
    QVERIFY(fileInfo);
    QVERIFY(!fileInfo->statements().isEmpty());
    QVERIFY(!mParser->findReplayRecord(basicStringTcc));
    ClassBrowserModel model;
    model.setParser(mParser);
    model.setCurrentFile(basicStringTcc);
    model.fillStatements();
    QVERIFY(findNode(model, {}, "std").isValid());
}

void TestWidgets::test_class_browser_standard_library_ostream_context()
{
    QString bitsStdcpp;
    mParser = std::make_shared<CppParser>();
    QVERIFY(configureStandardLibraryParser(mParser, bitsStdcpp));
    QDir includeDir = QFileInfo(bitsStdcpp).dir();
    QVERIFY(includeDir.cdUp());
    if (!QFileInfo::exists(includeDir.filePath("ostream")))
        QVERIFY(includeDir.cdUp());
    QString ostreamTcc = includeDir.filePath("bits/ostream.tcc");
    if (!QFileInfo::exists(ostreamTcc))
        QSKIP("This test requires the libstdc++ ostream.tcc layout");

    CppParser::parseFileBlocking(mParser, ostreamTcc, false, bitsStdcpp);

    ClassBrowserModel model;
    model.setParser(mParser);
    model.setCurrentFile(ostreamTcc);
    model.fillStatements();
    QModelIndex stdNode = findNode(model, {}, "std");
    QVERIFY(stdNode.isValid());
    QVERIFY(findNode(model, stdNode, "basic_ostream").isValid());
    QVERIFY(!findNode(model, stdNode, "vector").isValid());
}

void TestWidgets::test_standard_library_template_consumers()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString main = tmp.filePath("main.cpp");
    QStringList content{
        "#include <bits/stdc++.h>",
        "",
        "int main() {",
        "    std::vector<std::string> lst;",
        "    auto unique = std::make_unique<std::string>(\"unique\");",
        "    auto it = lst.begin();",
        "    it->size();",
        "    auto found = std::find_if(lst.begin(), lst.end(), [](const std::string &s) { return s.empty(); });",
        "    found->size();",
        "}"
    };
    QFile file(main);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(content.join('\n').toUtf8());
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
    popup.setCurrentScope(mParser->findScopeStatement(main, 6));
    popup.prepareSearch("", {"it"}, "->", {}, main, 6, CodeCompletionType::Normal, {});
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

    QSynedit::CharPos sizePos{content[6].indexOf("size"), 6};
    QCOMPARE(CppExpressionExtractor::extract(&editor, editor.syntaxer(), sizePos), QStringList({"it", "->", "size"}));
    PStatement size = mParser->findStatement("std::basic_string::size");
    QVERIFY(size);
    editor.gotoDefinition(sizePos);
    QCOMPARE(targetFile, size->definition.fileName);
    QCOMPARE(target.line, size->definition.range.start.line);
    target = {-1, -1};
    targetFile.clear();
    editor.gotoDeclaration(sizePos);
    StatementOccurrence declaration = statementDeclarationForNavigation(size, main);
    QCOMPARE(targetFile, declaration.fileName);
    QCOMPARE(target.line, declaration.range.start.line);
    QString hint = editor.getParserHint({"it", "->", "size"}, sizePos);
    QVERIFY(hint.contains("size"));

    FunctionTooltipWidget tooltip;
    editor.setFunctionTooltip(&tooltip);
    editor.setCaretPos({content[7].indexOf("lst.begin"), 7});
    QVERIFY(QMetaObject::invokeMethod(&editor, "onFunctionTipsTimer", Qt::DirectConnection));
    QVERIFY(tooltip.tipCount() > 0);
    QLabel *label = tooltip.findChild<QLabel *>();
    QVERIFY(label);
    QVERIFY(label->text().contains("find_if"));

    editor.setCaretPos({content[6].indexOf(")"), 6});
    QVERIFY(QMetaObject::invokeMethod(&editor, "onFunctionTipsTimer", Qt::DirectConnection));
    QVERIFY(tooltip.tipCount() > 0);
    QVERIFY(label->text().contains("size"));
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
    QCOMPARE(std::dynamic_pointer_cast<Statement>(macro)->value, "1");
    QCOMPARE(macro->definition.fileName, mMainFile);
    int line = mMainContent.indexOf("#define TEST_MACRO 1");
    QVERIFY(line >= 0);
    QCOMPARE(macro->definition.range.start.line, line);
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

void TestWidgets::test_class_browser_reopened_namespace_single_node()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace N { int first; }",
            "namespace N { int second; }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "classbrowser.cpp", false, "");

    ClassBrowserModel model;
    model.setParser(mParser);
    model.setCurrentFile("classbrowser.cpp");
    model.fillStatements();
    QModelIndex root;
    QCOMPARE(model.rowCount(root), 1);
    QModelIndex n = findNode(model, root, "N");
    QVERIFY(n.isValid());
    QCOMPARE(childCommands(model, n), QSet<QString>({"first", "second"}));
}

void TestWidgets::test_class_browser_project_union()
{
    const QString firstFile = "first.cpp";
    const QString secondFile = "second.cpp";
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream([firstFile](const QString &fileName, QStringList &buffer) {
        if (fileName == firstFile)
            buffer = QStringList{"namespace N { int first; }"};
        else
            buffer = QStringList{"namespace N { int second; }"};
        return true;
    });
    mParser->addProjectFile(firstFile, false);
    mParser->addProjectFile(secondFile, false);
    CppParser::parseFileBlocking(mParser, firstFile, true, "");
    CppParser::parseFileBlocking(mParser, secondFile, true, "");

    ClassBrowserModel model;
    model.setParser(mParser);
    model.setClassBrowserType(ProjectClassBrowserType::WholeProject);
    model.fillStatements();
    QModelIndex n = findNode(model, {}, "N");
    QVERIFY(n.isValid());
    QCOMPARE(childCommands(model, n), QSet<QString>({"first", "second"}));
}

void TestWidgets::test_class_browser_current_file_limits_reopened_namespace()
{
    const QString contextFile = "context.cpp";
    const QString currentFile = "current.tcc";
    mParser->setOnGetFileStream([contextFile](const QString &fileName, QStringList &buffer) {
        if (fileName == contextFile)
            buffer = QStringList{"namespace N { int other; }"};
        else
            buffer = QStringList{"namespace N { int current; }"};
        return true;
    });
    CppParser::parseFileBlocking(mParser, currentFile, false, contextFile);

    ClassBrowserModel model;
    model.setParser(mParser);
    model.setCurrentFile(currentFile);
    model.fillStatements();
    QModelIndex n = findNode(model, {}, "N");
    QVERIFY(n.isValid());
    QCOMPARE(childCommands(model, n), QSet<QString>({"current"}));
}

void TestWidgets::test_class_browser_current_file_out_of_line_member()
{
    const QString contextFile = "context.h";
    const QString currentFile = "current.cpp";
    mParser->setOnGetFileStream([contextFile](const QString &fileName, QStringList &buffer) {
        if (fileName == contextFile) {
            buffer = QStringList{
                "class A {",
                "public:",
                "    void f();",
                "    void g();",
                "};"
            };
        } else {
            buffer = QStringList{"void A::f() {}"};
        }
        return true;
    });
    CppParser::parseFileBlocking(mParser, currentFile, false, contextFile);

    ClassBrowserModel model;
    model.setParser(mParser);
    model.setCurrentFile(currentFile);
    model.fillStatements();
    QModelIndex a = findNode(model, {}, "A");
    QVERIFY(a.isValid());
    QCOMPARE(childCommands(model, a), QSet<QString>({"f"}));
}

void TestWidgets::test_class_browser_inline_namespace_aliases()
{
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "namespace N { inline namespace V {",
            "  struct C { int member; };",
            "  int value;",
            "} }"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, "classbrowser.cpp", false, "");

    ClassBrowserModel model;
    model.setParser(mParser);
    model.setCurrentFile("classbrowser.cpp");
    model.fillStatements();
    QModelIndex n = findNode(model, {}, "N");
    QVERIFY(n.isValid());
    QCOMPARE(childCommands(model, n), QSet<QString>({"C", "V", "value"}));
    QModelIndex alias = findNode(model, n, "C");
    QVERIFY(alias.isValid());
    auto node = static_cast<ClassBrowserNode *>(alias.internalPointer());
    QVERIFY(node && node->statement);
    QCOMPARE(node->statement->kind, StatementKind::InlineAlias);
    QVERIFY(mParser->membersOfStatement(node->statement).isEmpty());
    QCOMPARE(model.data(alias, Qt::DisplayRole).toString(), "C");
}

void TestWidgets::test_class_browser_direct_members()
{
    ClassBrowserModel model;
    model.setParser(mParser);
    model.setCurrentFile(mMainFile);
    model.fillStatements();
    QModelIndex base = findNode(model, {}, "Base");
    QVERIFY(base.isValid());
    QVERIFY(findNode(model, base, "publicValue").isValid());
    QModelIndex derived = findNode(model, {}, "Derived");
    QVERIFY(derived.isValid());
    QCOMPARE(childCommands(model, derived), QSet<QString>({"action"}));
    QVERIFY(!findNode(model, derived, "publicValue").isValid());
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
    StatementOccurrence declaration = statementDeclarationForNavigation(classStatement, mMainFile);
    QSynedit::CharPos classPos{6, declaration.range.start.line};
    QCOMPARE(CppExpressionExtractor::extract(&editor, editor.syntaxer(), classPos), QStringList{"Foo"});
    QSynedit::CharPos target{-1, -1};
    connect(&editor, &Editor::openFileRequested,
            [&target](const QString &, FileType, const QString &,
                      const QSynedit::CharPos &caretPos) {
        target = caretPos;
    });
    editor.gotoDefinition(classPos);
    QCOMPARE(target.line, classStatement->definition.range.start.line);
    QCOMPARE(target.ch, classStatement->definition.range.start.ch);
    target = {-1, -1};
    editor.gotoDeclaration(classPos);
    QCOMPARE(target.line, declaration.range.start.line);
    QCOMPARE(target.ch, declaration.range.start.ch);
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
    for (const std::weak_ptr<AbstractStatement> &weak : std::dynamic_pointer_cast<Statement>(classStatement)->children.values("Foo")) {
        PStatement candidate = weak.lock();
        if (candidate && candidate->kind == StatementKind::Constructor) {
            constructorStatement = candidate;
            break;
        }
    }
    QVERIFY(constructorStatement);
    StatementOccurrence declaration = statementDeclarationForNavigation(constructorStatement, mMainFile);
    QSynedit::CharPos constructorPos{2, declaration.range.start.line};
    QCOMPARE(CppExpressionExtractor::extract(&editor, editor.syntaxer(), constructorPos), QStringList{"Foo"});
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
    QCOMPARE(target.line, constructorStatement->definition.range.start.line);
    QCOMPARE(target.ch, constructorStatement->definition.range.start.ch);
    target = {-1, -1};
    QSynedit::CharPos definitionPos{5, constructorStatement->definition.range.start.line};
    editor.gotoDeclaration(definitionPos);
    QCOMPARE(target.line, declaration.range.start.line);
    QCOMPARE(target.ch, declaration.range.start.ch);
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
        QVERIFY2(statementHasDefinition(statement), name);
    }
    for (int line : {13, 14, 18}) {
        QString name = line == 13 ? "param" : line == 14 ? "local" : "retry";
        PStatement statement = mParser->findStatementOf(mMainFile, QStringList{name}, line);
        QVERIFY2(statement, name.toUtf8().constData());
        QVERIFY(statementHasDefinition(statement));
    }
    for (const char *name : {"external", "declared", "Forward"}) {
        PStatement statement = mParser->findStatement(QString::fromLatin1(name));
        QVERIFY2(statement, name);
        QVERIFY2(!statementHasDefinition(statement), name);
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

void TestWidgets::test_goto_definition_prefers_current_file()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    auto writeLines = [](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        file.write(lines.join('\n').toUtf8());
        file.close();
        return true;
    };
    const QString declarationFile = tmp.filePath("declaration.h");
    const QString definitionFile = tmp.filePath("definition.h");
    QVERIFY(writeLines(declarationFile, {"extern int value;"}));
    QVERIFY(writeLines(definitionFile, {"int value;"}));
    mMainFile = tmp.filePath("main.cpp");
    for (bool definitionInCurrentFile : {true, false}) {
        mMainContent = QStringList{
            definitionInCurrentFile ? "#include \"declaration.h\"" : "#include \"definition.h\"",
            definitionInCurrentFile ? "int value;" : "extern int value;",
            "int readValue() { return value; }"
        };
        QVERIFY(writeLines(mMainFile, mMainContent));
        mParser = std::make_shared<CppParser>();
        mParser->addIncludePath(tmp.path());
        mParser->addProjectFile(mMainFile, false);
        CppParser::parseFileBlocking(mParser, mMainFile, false, "");

        PStatement value = mParser->findStatementOf(mMainFile, "value", 2);
        QVERIFY(value);
        QVERIFY(statementHasDefinition(value));
        QString expectedFile = definitionInCurrentFile ? mMainFile : definitionFile;
        int expectedLine = definitionInCurrentFile ? 1 : 0;
        QCOMPARE(value->definition.fileName, expectedFile);
        QCOMPARE(value->definition.range.start.line, expectedLine);

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
        QString targetFile;
        QSynedit::CharPos target{-1, -1};
        QObject context;
        connect(&editor, &Editor::openFileRequested, &context,
                [&targetFile, &target](const QString &fileName, FileType, const QString &,
                                       const QSynedit::CharPos &caretPos) {
            targetFile = fileName;
            target = caretPos;
        });
        editor.gotoDefinition({mMainContent[2].lastIndexOf("value"), 2});
        QCOMPARE(targetFile, expectedFile);
        QCOMPARE(target.line, expectedLine);
    }
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
    QSynedit::CharPos classPos{6, statementDeclarationForNavigation(classStatement, mMainFile).range.start.line};
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
    QSynedit::CharPos classPos{6, statementDeclarationForNavigation(classStatement, mMainFile).range.start.line};
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
    QSynedit::CharPos classPos{6, statementDeclarationForNavigation(classStatement, mMainFile).range.start.line};
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
    PStatement constructorStatement = std::dynamic_pointer_cast<Statement>(classStatement)->children.value("Foo").lock();
    QVERIFY(constructorStatement);
    QSynedit::CharPos constructorPos{2, statementDeclarationForNavigation(constructorStatement, mMainFile).range.start.line};
    QStringList expectedContent = mMainContent;
    for (QString &lineText : expectedContent)
        lineText.replace("Foo", "Bar");
    CppRefacter refactor(nullptr);
    refactor.renameSymbol(&editor, constructorPos, "Bar");
    QCOMPARE(editor.content(), expectedContent);
}

void TestWidgets::test_editor_color_scheme_demo_parser_is_self_contained()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString iostream = tmp.filePath("iostream");
    const QString conio = tmp.filePath("conio.h");
    for (const QString &path : {iostream, conio}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(path == iostream ? "int iostream_value;" : "int conio_value;");
        file.close();
    }

    const int previousDefault = pSettings->compilerSets().defaultIndex();
    const CompilerSetList previousSets = pSettings->compilerSets().clearSets();
    auto restoreCompilerSets = finally([this, previousDefault, previousSets] {
        pSettings->compilerSets().clearSets();
        for (const PCompilerSet &set : previousSets)
            pSettings->compilerSets().addSet(set);
        pSettings->compilerSets().setDefaultIndex(previousDefault);
    });
    PCompilerSet compilerSet = pSettings->compilerSets().addSet();
    compilerSet->CppIncludeDirs().append(tmp.path());
    pSettings->compilerSets().setDefaultIndex(0);

    EditorColorSchemeWidget widget(mColorManager.get(), "Color", "Editor", nullptr);
    Editor *demo = widget.findChild<Editor *>("editDemo");
    QVERIFY(demo);
    const PCppParser &parser = demo->parser();
    QVERIFY(parser);
    QVERIFY(parser->includePaths().isEmpty());
    QVERIFY(parser->projectIncludePaths().isEmpty());

    parser->addIncludePath(tmp.path());
    CppParser::parseFileBlocking(parser, demo->filename(), false, "");
    QVERIFY(parser->findStatement("iostream_value"));
    QVERIFY(parser->findStatement("conio_value"));
}

void TestWidgets::test_unscoped_enum_completion_and_class_browser()
{
    const QString fileName = "enum_completion.cpp";
    mParser->setOnGetFileStream([](const QString &, QStringList &buffer) {
        buffer = QStringList{
            "enum Color { red, green, blue };",
            "enum class Scoped { value };",
            "struct ZOrdered { int alpha; };"
        };
        return true;
    });
    CppParser::parseFileBlocking(mParser, fileName, false, "");

    CodeCompletionPopup popup(mColorManager.get(), nullptr);
    popup.setParser(mParser);
    QSet<QString> members = completionNames(popup, "Color", fileName,
            CodeCompletionType::Normal, "", "", {}, 0, "::", {"Color"});
    QVERIFY(members.contains("red") && members.contains("green") && members.contains("blue"));
    QVERIFY(completionNames(popup, "", fileName, CodeCompletionType::Normal,
                            "", "red").contains("red"));

    ClassBrowserModel model;
    model.setParser(mParser);
    model.setCurrentFile(fileName);
    model.fillStatements();
    QCOMPARE(model.rowCount({}), 3);
    QVERIFY(!findNode(model, {}, "red").isValid());
    QModelIndex color = findNode(model, {}, "Color");
    QVERIFY(color.isValid());
    QCOMPARE(model.rowCount(color), 3);
    QCOMPARE(childCommands(model, color), QSet<QString>({"red", "green", "blue"}));
    QModelIndex scoped = findNode(model, {}, "Scoped");
    QVERIFY(scoped.isValid());
    QCOMPARE(model.rowCount(scoped), 1);
    QCOMPARE(childCommands(model, scoped), QSet<QString>({"value"}));
    QModelIndex ordered = findNode(model, {}, "ZOrdered");
    QVERIFY(ordered.isValid());
    QCOMPARE(model.rowCount(ordered), 1);
    QCOMPARE(childCommands(model, ordered), QSet<QString>({"alpha"}));
}
