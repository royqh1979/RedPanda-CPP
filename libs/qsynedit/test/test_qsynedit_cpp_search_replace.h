#ifndef TEST_QSYNEDIT_CPP_SEARCH_REPLACE_H
#define TEST_QSYNEDIT_CPP_SEARCH_REPLACE_H
#include <qsynedit/qsynedit.h>

namespace QSynedit{

class Searcher;

class TestQSyneditCppSearchReplace : public QObject
{
    Q_OBJECT
protected:
    std::shared_ptr<QSynEdit> mEdit;
    QStringList mFoundStrings;
    QStringList mReplaces;
    QList<CharPos> mFoundPositions;
    QList<int> mFoundLens;

    SearchMatchedProc mBasicMatchedAndExitProc;
    SearchMatchedProc mBasicMatchedAndContinueProc;
    SearchMatchedProc mReplaceAndExitProc;
    SearchMatchedProc mReplaceAndContinueProc;
    std::shared_ptr<Searcher> mBasicSearcher;
    std::shared_ptr<Searcher> mRegexSearcher;
private slots:
    void initTestCase();
    void clearFounds();
    void test_basic_search_forward_in_empty_doc();
    void test_basic_search_forward_from_caret_without_wrap();
    void test_basic_search_forward_from_caret_without_wrap_with_callback();
    void test_basic_search_forward_from_caret_with_selection();
    void test_basic_search_forward_from_caret_include_current_selection();
    void test_basic_search_forward();
    void test_basic_search_forward_from_caret_with_wrap();

    void test_search_forward_from_caret_whole_word();
    void test_search_forward_whole_word2();
    void test_search_forward_from_caret_match_case();

    void test_search_all_forward_in_whole_file_from_start();
    void test_search_all_forward_in_whole_file_from_caret_and_wrap();
    void test_search_all_forward_in_whole_file_from_caret_and_no_wrap();

    void test_search_forward_in_scope();
    void test_ensure_caret_pos_not_used_in_scope_search();

    void test_basic_search_backward_in_empty_doc();
    void test_basic_search_backward_from_caret_without_wrap();
    void test_basic_search_backward_from_caret_without_wrap_with_callback();
    void test_basic_search_backward_from_caret_with_selection();
    void test_basic_search_backward_from_caret_include_current_selection();
    void test_basic_search_backward_from_caret_with_wrap();

    void test_search_all_backward_in_whole_file_from_end();
    void test_search_all_backward_in_whole_file_from_caret_and_wrap();
    void test_search_all_backward_in_whole_file_from_caret_and_no_wrap();

    void test_search_backward_in_scope();

    void test_search_regex();
    void test_basic_forward_search_without_wrap2();

    void test_replace_forward_from_caret();
    void test_replace_forward_from_caret_with_selection();
    void test_replace_forward_from_caret_include_selection();
    void test_replace_backward_from_caret();
    void test_replace_backward_from_caret_with_selection();
    void test_replace_backward_from_caret_include_selection();
    void test_replace_forward_scope();
    void test_replace_backward_scope();
};

}
#endif
