#include <QTest>
#include "test_searchresultreplacer.h"
#include "src/search/searchresultreplacer.h"

static PSearchResults makeResults(
        const QString &keyword, bool useRegex = false, bool matchCase = true)
{
    QSynedit::SearchOptions options = matchCase
            ? QSynedit::ssoMatchCase : QSynedit::ssoNone;
    return std::make_shared<SearchResults>(
                keyword, options, useRegex, SearchFileScope::currentFile,
                SearchType::Search, QString(), QString(), true);
}

static PSearchResultTreeItem addMatch(
        SearchResultTreeItem &file, int line, int ch, int length,
        const QString &matchedText, bool selected = true)
{
    PSearchResultTreeItem item = std::make_shared<SearchResultTreeItem>();
    item->line = line;
    item->start = ch;
    item->len = length;
    item->matchedText = matchedText;
    item->selected = selected;
    file.results.append(item);
    return item;
}

void TestSearchResultReplacer::test_selected_matches_only_and_undo()
{
    QSynedit::QSynEdit editor;
    editor.loadContent({"int int", "int"});
    PSearchResults results = makeResults("int");
    SearchResultTreeItem file;
    addMatch(file, 0, 0, 3, "int");
    addMatch(file, 0, 4, 3, "int", false);
    addMatch(file, 1, 0, 3, "int");

    QCOMPARE(SearchResultReplacer::replaceSelected(editor, *results, file, "long"),
             SearchResultReplaceError::None);
    QCOMPARE(editor.content(), QStringList({"long int", "long"}));
    editor.undo();
    QCOMPARE(editor.content(), QStringList({"int int", "int"}));
    editor.redo();
    QCOMPARE(editor.content(), QStringList({"long int", "long"}));
}

void TestSearchResultReplacer::test_case_insensitive_and_first_column()
{
    QSynedit::QSynEdit editor;
    editor.loadContent(QStringList{"INT int"});
    PSearchResults results = makeResults("int", false, false);
    SearchResultTreeItem file;
    addMatch(file, 0, 0, 3, "INT");

    QCOMPARE(SearchResultReplacer::replaceSelected(editor, *results, file, "value"),
             SearchResultReplaceError::None);
    QCOMPARE(editor.content(), QStringList({"value int"}));
}

void TestSearchResultReplacer::test_regex_captures_and_variable_length()
{
    QSynedit::QSynEdit editor;
    editor.loadContent(QStringList{"a1 bb22"});
    PSearchResults results = makeResults("([a-z]+)(\\d+)", true);
    SearchResultTreeItem file;
    addMatch(file, 0, 0, 2, "a1");
    addMatch(file, 0, 3, 4, "bb22");

    QCOMPARE(SearchResultReplacer::replaceSelected(editor, *results, file, "\\2:\\1"),
             SearchResultReplaceError::None);
    QCOMPARE(editor.content(), QStringList({"1:a 22:bb"}));
}

void TestSearchResultReplacer::test_zero_length_matches()
{
    QSynedit::QSynEdit editor;
    editor.loadContent(QStringList{"ab"});
    PSearchResults results = makeResults("x*", true);
    SearchResultTreeItem file;
    addMatch(file, 0, 0, 0, "");
    addMatch(file, 0, 1, 0, "");
    addMatch(file, 0, 2, 0, "");

    QCOMPARE(SearchResultReplacer::replaceSelected(editor, *results, file, "X"),
             SearchResultReplaceError::None);
    QCOMPARE(editor.content(), QStringList({"XaXbX"}));
}

void TestSearchResultReplacer::test_changed_content_has_no_partial_edit()
{
    QSynedit::QSynEdit editor;
    editor.loadContent(QStringList{"a a"});
    PSearchResults results = makeResults("a");
    SearchResultTreeItem file;
    addMatch(file, 0, 0, 1, "a");
    addMatch(file, 0, 2, 1, "changed");

    QCOMPARE(SearchResultReplacer::replaceSelected(editor, *results, file, "b"),
             SearchResultReplaceError::ContentChanged);
    QCOMPARE(editor.content(), QStringList({"a a"}));
    QVERIFY(!editor.canUndo());
}

void TestSearchResultReplacer::test_unicode_multiline_and_empty_replacements()
{
    QSynedit::QSynEdit editor;
    QString variable = QString::fromUtf8("变量");
    editor.loadContent(QStringList{variable + " " + variable});
    PSearchResults results = makeResults(variable);
    SearchResultTreeItem file;
    addMatch(file, 0, 0, 2, variable);
    addMatch(file, 0, 3, 2, variable);
    QCOMPARE(SearchResultReplacer::replaceSelected(editor, *results, file, ""),
             SearchResultReplaceError::None);
    QCOMPARE(editor.content(), QStringList({" "}));

    editor.loadContent(QStringList{"a a"});
    results = makeResults("a");
    SearchResultTreeItem multilineFile;
    addMatch(multilineFile, 0, 0, 1, "a");
    addMatch(multilineFile, 0, 2, 1, "a");
    QCOMPARE(SearchResultReplacer::replaceSelected(editor, *results, multilineFile, "x\ny"),
             SearchResultReplaceError::None);
    QCOMPARE(editor.content(), QStringList({"x", "y x", "y"}));
}

void TestSearchResultReplacer::test_tab_match()
{
    QSynedit::QSynEdit editor;
    editor.loadContent(QStringList{"\tfoo"});
    PSearchResults results = makeResults("\\t", true);
    SearchResultTreeItem file;
    addMatch(file, 0, 0, 1, "\t");
    QCOMPARE(SearchResultReplacer::replaceSelected(editor, *results, file, "  "),
             SearchResultReplaceError::None);
    QCOMPARE(editor.content(), QStringList({"  foo"}));
}

void TestSearchResultReplacer::test_no_selection_does_not_create_undo()
{
    QSynedit::QSynEdit editor;
    editor.loadContent(QStringList{"a"});
    PSearchResults results = makeResults("a");
    SearchResultTreeItem file;
    addMatch(file, 0, 0, 1, "a", false);
    QCOMPARE(SearchResultReplacer::replaceSelected(editor, *results, file, "b"),
             SearchResultReplaceError::None);
    QCOMPARE(editor.content(), QStringList({"a"}));
    QVERIFY(!editor.canUndo());
}
