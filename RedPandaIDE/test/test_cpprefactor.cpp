#include <QTest>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFile>
#include "test_cpprefactor.h"
#include "src/editor/editor.h"
#include "src/syntaxermanager.h"
#include "src/iconsmanager.h"
#include <qsynedit/formatter/cppformatter.h>
#include <qsynedit/syntaxer/cpp.h>

void TestCppRefactor::initTestCase()
{
    mSettings = std::make_unique<Settings>("test-refactor.ini");
    pSettings = mSettings.get();
    pSettings->dirs().load();
    pSettings->ui().load();
    mColorManager = std::make_unique<ColorManager>(&pSettings->dirs());
    mIconsManager = std::make_shared<IconsManager>(&pSettings->dirs(), "en_US");
}

void TestCppRefactor::cleanupTestCase()
{
    mProject.reset();
    mIconsManager.reset();
    mColorManager.reset();
    mSettings.reset();
    pSettings = nullptr;
}

void TestCppRefactor::init()
{
    mMainFile = QDir::current().absoluteFilePath("tmp/refactor.cpp");
    mFiles.clear();
    mProject.reset();
    mParser = std::make_shared<CppParser>();
    mParser->setOnGetFileStream([this](const QString &fileName, QStringList &buffer) {
        if (!mFiles.contains(fileName))
            return false;
        buffer = mFiles.value(fileName);
        return true;
    });
}

std::shared_ptr<Editor> TestCppRefactor::makeEditor(const QString &fileName)
{
    auto editor = std::make_shared<Editor>();
    editor->setEditorSettings(&pSettings->editor());
    editor->setColorManager(mColorManager.get());
    editor->setFilename(fileName);
    editor->setSyntaxer(std::make_shared<QSynedit::CppSyntaxer>());
    editor->setFormatter(std::make_shared<QSynedit::CppFormatter>());
    editor->applySettings();
    editor->setFileType(FileType::CppSource);
    editor->loadContent(mFiles.value(fileName));
    editor->setCppParser(mParser);
    if (mProject)
        editor->setInProjectQueryFunc([](const Editor *) { return true; });
    return editor;
}

void TestCppRefactor::test_find_occurrences_current_file()
{
    mFiles[mMainFile] = QStringList{
        "int value;",
        "int other;",
        "int read(){return value;}",
        "// value",
        "const char *s=\"value\";",
        "value=other;",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    SymbolReferenceResults results;
    QVERIFY(refactor.findOccurrences(editor.get(), {4, 0}, results));
    QCOMPARE(results.count(), 3);
    QCOMPARE(results.files.size(), 1);
    QCOMPARE(results.files.at(0).references.at(0).line, 0);
    QCOMPARE(results.files.at(0).references.at(1).line, 2);
    QCOMPARE(results.files.at(0).references.at(2).line, 5);
}

void TestCppRefactor::test_find_occurrences_same_line_distinct_declarations()
{
    mFiles[mMainFile] = QStringList{
        "int first; int second;",
        "int use(){return first+second;}",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    SymbolReferenceResults firstResults;
    QVERIFY(refactor.findOccurrences(editor.get(), {4, 0}, firstResults));
    QCOMPARE(firstResults.count(), 2);
    SymbolReferenceResults secondResults;
    QVERIFY(refactor.findOccurrences(editor.get(), {15, 0}, secondResults));
    QCOMPARE(secondResults.count(), 2);
    QVERIFY(firstResults.files.at(0).references.at(0).ch
            != secondResults.files.at(0).references.at(0).ch);
}

void TestCppRefactor::test_find_occurrences_project_unsaved_content()
{
    QString header = QDir::current().absoluteFilePath("tmp/refactor.h");
    mFiles[header] = QStringList{"int value;"};
    mFiles[mMainFile] = QStringList{"int use(){return value;}"};
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    mParser->addProjectFile(header, false);
    CppParser::parseFileBlocking(mParser, header, false, "");
    mProject = Project::load("test.dev", nullptr, mIconsManager.get(), new QFileSystemWatcher);
    mProject->addUnit(mMainFile, mProject->rootNode());
    mProject->addUnit(header, mProject->rootNode());
    auto editor = makeEditor(mMainFile);
    editor->loadContent(QStringList{"int use(){return value+1;}"});
    CppRefactor refactor(mProject, [this](const QString &fileName, QStringList &buffer) {
        if (!mFiles.contains(fileName))
            return false;
        buffer = mFiles.value(fileName);
        return true;
    });
    mFiles[mMainFile] = QStringList{"int use(){return value+1;}"};

    SymbolReferenceResults projectResults;
    QVERIFY(refactor.findOccurrences(editor.get(), "value", "",
                                     SymbolReferenceScope::WholeProject, projectResults));
    QCOMPARE(projectResults.count(), 2);
    QCOMPARE(projectResults.files.size(), 2);

    SymbolReferenceResults currentResults;
    QVERIFY(refactor.findOccurrences(editor.get(), "value", "",
                                     SymbolReferenceScope::CurrentFile, currentResults));
    QCOMPARE(currentResults.count(), 1);
    QCOMPARE(currentResults.files.at(0).fileName, mMainFile);
}

void TestCppRefactor::test_same_name_isolation_across_scopes()
{
    mFiles[mMainFile] = QStringList{
        "namespace A { int value; }",
        "namespace A { int read(){return value;} }",
        "namespace B { int value; }",
        "namespace B { int read(){return value;} }",
        "int value;",
        "int readGlobal(){return value;}",
        "struct C { int value; int read(){return value;} };",
        "void f(){int value; value=1;}",
        "void g(){int value; value=2;}",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;

    const QVector<QPair<QSynedit::CharPos, int>> cases{
        {{18, 0}, 2},
        {{18, 2}, 2},
        {{4, 4}, 2},
        {{15, 6}, 2},
        {{13, 7}, 2},
        {{13, 8}, 2},
    };
    for (const auto &testCase : cases) {
        SymbolReferenceResults results;
        QVERIFY(refactor.findOccurrences(editor.get(), testCase.first, results));
        QCOMPARE(results.count(), testCase.second);
    }

    QCOMPARE(refactor.renameSymbol(editor.get(), {18, 0}, "renamed"),
             RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({
                 "namespace A { int renamed; }",
                 "namespace A { int read(){return renamed;} }",
                 "namespace B { int value; }",
                 "namespace B { int read(){return value;} }",
                 "int value;",
                 "int readGlobal(){return value;}",
                 "struct C { int value; int read(){return value;} };",
                 "void f(){int value; value=1;}",
                 "void g(){int value; value=2;}"}));
}

void TestCppRefactor::test_rename_rejects_project_external_declaration()
{
    QString otherFile = QDir::current().absoluteFilePath("tmp/refactor_other.cpp");
    mFiles[mMainFile] = QStringList{"int value;", "int use(){return value;}"};
    mFiles[otherFile] = QStringList{"extern int value;", "int read(){return value;}"};
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    mParser->addProjectFile(otherFile, false);
    CppParser::parseFileBlocking(mParser, otherFile, false, "");
    mProject = Project::load("test.dev", nullptr, mIconsManager.get(), new QFileSystemWatcher);
    mProject->addUnit(mMainFile, mProject->rootNode());
    mProject->addUnit(otherFile, mProject->rootNode());
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor(mProject, [this](const QString &fileName, QStringList &buffer) {
        if (!mFiles.contains(fileName))
            return false;
        buffer = mFiles.value(fileName);
        return true;
    });
    bool couldUndo = editor->canUndo();
    QCOMPARE(refactor.renameSymbol(editor.get(), {4, 0}, "other"),
             RenameResult::NotInCurrentFile);
    QCOMPARE(editor->content(), mFiles.value(mMainFile));
    QCOMPARE(editor->canUndo(), couldUndo);
}

void TestCppRefactor::test_rename_same_class_defined_in_multiple_tus()
{
    QSKIP("Project-wide statement identity is handled in the project refactor stage");
    mFiles[mMainFile] = QStringList{"class Foo {};", "Foo value;"};
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;

    QString header = QDir::current().absoluteFilePath("tmp/refactor.h");
    mFiles[header] = QStringList{"class Foo {};"};
    mParser->addProjectFile(header, false);
    CppParser::parseFileBlocking(mParser, header, false, "");
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    QCOMPARE(static_cast<int>(refactor.renameSymbol(editor.get(), {6, 0}, "Bar")),
             static_cast<int>(RenameResult::NotInCurrentFile));
    QCOMPARE(editor->content(), mFiles.value(mMainFile));
}

void TestCppRefactor::test_rename_class_constructor_destructor()
{
    mFiles[mMainFile] = QStringList{
        "class Foo {public: Foo(); ~Foo();};",
        "Foo::Foo() {}",
        "Foo::~Foo() {}",
        "Foo value;",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QCOMPARE(refactor.renameSymbol(editor.get(), {6, 0}, "Bar"), RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({
                 "class Bar {public: Bar(); ~Bar();};",
                 "Bar::Bar() {}",
                 "Bar::~Bar() {}",
                 "Bar value;"}));
}

void TestCppRefactor::test_rename_overload_subset()
{
    mFiles[mMainFile] = QStringList{
        "void f(int) {}",
        "void f(double) {}",
        "void call(){f(1);f(1.0);}",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QVector<PStatement> overloads = refactor.overloadsOf(editor.get(), {5, 0});
    QCOMPARE(overloads.size(), 2);
    QVector<PStatement> selected;
    for (const PStatement &overload : overloads) {
        if (overload->definition.range.start.line == 0)
            selected.append(overload);
    }
    QCOMPARE(selected.size(), 1);
    QCOMPARE(refactor.renameSymbol(editor.get(), {5, 0}, "h", selected),
             RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({
                 "void h(int) {}",
                 "void f(double) {}",
                 "void call(){h(1);f(1.0);}"}));

    mFiles[mMainFile] = QStringList{"void f(int) {}", "void f(double) {}"};
    editor->loadContent(mFiles.value(mMainFile));
    QCOMPARE(refactor.renameSymbol(editor.get(), {5, 0}, "h", QVector<PStatement>{}),
             RenameResult::NoSelection);
    QCOMPARE(editor->content(), mFiles.value(mMainFile));
}

void TestCppRefactor::test_rename_overrides_both_directions()
{
    mFiles[mMainFile] = QStringList{
        "struct Base {virtual void f(int) {} virtual void f(double) {}};",
        "struct Derived:Base {void f(int) override {} void f(double) override {}};",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QVector<PStatement> selected;
    for (const PStatement &overload : refactor.overloadsOf(editor.get(), {26, 0})) {
        auto normal = std::dynamic_pointer_cast<Statement>(overload);
        if (normal->parameters.size() == 1 && normal->parameters.first().type
                && normal->parameters.first().type->qualifiedName == "int")
            selected.append(overload);
    }
    QCOMPARE(selected.size(), 1);
    QCOMPARE(refactor.renameSymbol(editor.get(), {26, 0}, "h", selected),
             RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({
                 "struct Base {virtual void h(int) {} virtual void f(double) {}};",
                 "struct Derived:Base {void h(int) override {} void f(double) override {}};"}));

    init();
    mFiles[mMainFile] = QStringList{
        "struct Base {virtual void f(int) {} virtual void f(double) {}};",
        "struct Derived:Base {void f(int) override {} void f(double) override {}};",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    editor = makeEditor(mMainFile);
    selected.clear();
    for (const PStatement &overload : refactor.overloadsOf(editor.get(), {26, 1})) {
        auto normal = std::dynamic_pointer_cast<Statement>(overload);
        if (normal->parameters.size() == 1 && normal->parameters.first().type
                && normal->parameters.first().type->qualifiedName == "int")
            selected.append(overload);
    }
    QCOMPARE(selected.size(), 1);
    QCOMPARE(refactor.renameSymbol(editor.get(), {26, 1}, "h", selected),
             RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({
                 "struct Base {virtual void h(int) {} virtual void f(double) {}};",
                 "struct Derived:Base {void h(int) override {} void f(double) override {}};"}));
}

void TestCppRefactor::test_rename_template_specializations()
{
    mFiles[mMainFile] = QStringList{
        "template<class T> class Foo {public: Foo(); ~Foo();};",
        "template<class T> Foo<T>::Foo() {}",
        "template<class T> Foo<T>::~Foo() {}",
        "template<> class Foo<int> {public: Foo(); ~Foo();};",
        "Foo<int>::Foo() {}",
        "Foo<int>::~Foo() {}",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QCOMPARE(refactor.renameSymbol(editor.get(), {25, 0}, "Bar"), RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({
                 "template<class T> class Bar {public: Bar(); ~Bar();};",
                 "template<class T> Bar<T>::Bar() {}",
                 "template<class T> Bar<T>::~Bar() {}",
                 "template<> class Bar<int> {public: Bar(); ~Bar();};",
                 "Bar<int>::Bar() {}",
                 "Bar<int>::~Bar() {}"}));
}

void TestCppRefactor::test_rename_partial_template_specialization()
{
    mFiles[mMainFile] = QStringList{
        "template<class T> class Foo {};",
        "template<class T> class Foo<T*> {};",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QCOMPARE(refactor.renameSymbol(editor.get(), {25, 0}, "Bar"),
             RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({
                 "template<class T> class Bar {};",
                 "template<class T> class Bar<T*> {};"}));
}

void TestCppRefactor::test_rename_visibility_and_using_declaration()
{
    mFiles[mMainFile] = QStringList{
        "struct Base {public:void f(int) {} protected:void f(double) {} private:void f(char) {}};",
        "struct Derived:Base {using Base::f;};",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QCOMPARE(refactor.renameSymbol(editor.get(), {26, 0}, "g"), RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({
                 "struct Base {public:void g(int) {} protected:void g(double) {} private:void g(char) {}};",
                 "struct Derived:Base {using Base::g;};"}));
}

void TestCppRefactor::test_rename_namespace_alias_and_enum()
{
    mFiles[mMainFile] = QStringList{
        "namespace N {enum class E {A};}",
        "namespace M=N;",
        "using N::E;",
        "E value=E::A;",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QCOMPARE(refactor.renameSymbol(editor.get(), {10, 0}, "O"), RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({
                 "namespace O {enum class E {A};}",
                 "namespace M=O;",
                 "using O::E;",
                 "E value=E::A;"}));
}

void TestCppRefactor::test_rename_local_lambda_and_structured_binding()
{
    mFiles[mMainFile] = QStringList{
        "int value;",
        "struct Pair {int a,b;};",
        "Pair pair;",
        "void f(){auto [x,y]=pair; int value=1; auto l=[&value]{return value+x;};}",
    };
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QCOMPARE(refactor.renameSymbol(editor.get(), {34, 3}, "local"), RenameResult::Success);
    QCOMPARE(editor->content().at(0), "int value;");
    QCOMPARE(editor->content().at(3),
             "void f(){auto [x,y]=pair; int local=1; auto l=[&local]{return local+x;};}");
}

void TestCppRefactor::test_rename_undefined_local_conflict_and_scope()
{
    mFiles[mMainFile] = QStringList{"void f(){int existing; foo=existing;}"};
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QCOMPARE(refactor.renameUndefinedLocalVariable(editor.get(), {23, 0}, "existing"),
             RenameResult::NameConflict);
    QCOMPARE(editor->content(), mFiles.value(mMainFile));
    QCOMPARE(refactor.renameUndefinedLocalVariable(editor.get(), {23, 0}, "bar"),
             RenameResult::Success);
    QCOMPARE(editor->content(), QStringList{"void f(){int existing; bar=existing;}"});
}

void TestCppRefactor::test_rename_invalid_and_cross_file()
{
    QString header = QDir::current().absoluteFilePath("tmp/refactor.h");
    mFiles[mMainFile] = QStringList{
        "class Foo {};", "#include \"refactor.h\"", "Foo value;", "Bar value;"};
    mFiles[header] = QStringList{"class Bar {};"};
    QVERIFY(QDir().mkpath("tmp"));
    QFile output(header);
    QVERIFY(output.open(QIODevice::WriteOnly));
    output.write("class Bar {};\n");
    output.close();
    mParser->addProjectFile(header, false);
    CppParser::parseFileBlocking(mParser, header, false, "");
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QCOMPARE(refactor.renameSymbol(nullptr, {0, 0}, "Other"), RenameResult::InvalidInput);
    QCOMPARE(refactor.renameSymbol(editor.get(), {6, 0}, "bad-name"), RenameResult::InvalidInput);
    QCOMPARE(refactor.renameSymbol(editor.get(), {0, 0}, "Other"), RenameResult::InvalidInput);
    QCOMPARE(static_cast<int>(refactor.renameSymbol(editor.get(), {0, 3}, "Other")),
             static_cast<int>(RenameResult::NotInCurrentFile));
    QCOMPARE(editor->content(), mFiles.value(mMainFile));
}

void TestCppRefactor::test_rename_prepare_apply_and_stale_content()
{
    mFiles[mMainFile] = QStringList{"int value;", "int use(){return value;}"};
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    SymbolReferenceResults results;
    QCOMPARE(refactor.prepareRename(editor.get(), {4, 0}, "other", std::nullopt, results),
             RenameResult::Success);
    QCOMPARE(results.count(), 2);
    QCOMPARE(editor->content(), mFiles.value(mMainFile));
    QCOMPARE(refactor.applyRename(editor.get(), results, "other"), RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({"int other;", "int use(){return other;}"}));

    editor->loadContent(mFiles.value(mMainFile));
    QCOMPARE(refactor.prepareRename(editor.get(), {4, 0}, "other", std::nullopt, results),
             RenameResult::Success);
    editor->loadContent(QStringList{"int changed;", "int use(){return value;}"});
    QCOMPARE(refactor.applyRename(editor.get(), results, "other"),
             RenameResult::ContentChanged);
    QCOMPARE(editor->content(), QStringList({"int changed;", "int use(){return value;}"}));
}

void TestCppRefactor::test_rename_undo_redo()
{
    mFiles[mMainFile] = QStringList{"int value;", "int use(){return value;}"};
    mParser->addProjectFile(mMainFile, false);
    CppParser::parseFileBlocking(mParser, mMainFile, false, "");
    auto editor = makeEditor(mMainFile);
    CppRefactor refactor;
    QCOMPARE(refactor.renameSymbol(editor.get(), {4, 0}, "other"), RenameResult::Success);
    QCOMPARE(editor->content(), QStringList({"int other;", "int use(){return other;}"}));
    editor->undo();
    QCOMPARE(editor->content(), mFiles.value(mMainFile));
    editor->redo();
    QCOMPARE(editor->content(), QStringList({"int other;", "int use(){return other;}"}));
}
