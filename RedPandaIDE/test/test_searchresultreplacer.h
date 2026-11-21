#ifndef TEST_SEARCHRESULTREPLACER_H
#define TEST_SEARCHRESULTREPLACER_H

#include <QObject>

class TestSearchResultReplacer : public QObject
{
    Q_OBJECT
private slots:
    void test_selected_matches_only_and_undo();
    void test_case_insensitive_and_first_column();
    void test_regex_captures_and_variable_length();
    void test_zero_length_matches();
    void test_changed_content_has_no_partial_edit();
    void test_unicode_multiline_and_empty_replacements();
    void test_tab_match();
    void test_no_selection_does_not_create_undo();
};

#endif // TEST_SEARCHRESULTREPLACER_H
