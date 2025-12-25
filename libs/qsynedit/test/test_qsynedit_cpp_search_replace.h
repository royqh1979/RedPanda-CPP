#ifndef TEST_QSYNEDIT_CPP_SEARCH_REPLACE_H
#define TEST_QSYNEDIT_CPP_SEARCH_REPLACE_H
#include <QtTest>
#include <QCoreApplication>
#include "qsynedit/qsynedit.h"

namespace QSynedit{

class QSynEdit;
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
    std::shared_ptr<Searcher> mBasicSearcher;
private slots:
    void initTestCase();
    void clearFounds();
    void test_basic_search_forward_in_empty_doc();
    void test_basic_search_forward_without_wrap();
    void test_basic_search_forward_without_wrap_with_callback();
    void test_basic_search_forward_with_selection();
    void test_basic_search_forward_ignore_caret_pos();
    void test_basic_search_forward_with_wrap();

    void test_search_all_in_whole_file_from_start();
    void test_search_all_in_whole_file_from_caret_and_wrap();
    void test_search_all_in_whole_file_from_caret_and_no_wrap();

    void test_basic_search_backward_in_empty_doc();
    void test_basic_search_backward_without_wrap();
    void test_basic_search_backward_without_wrap_with_callback();
    void test_basic_search_backward_with_selection();
    void test_basic_search_backward_with_wrap();

    void test_basic_forward_search_without_wrap2();
};

}
#endif
