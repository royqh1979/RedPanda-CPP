#ifndef TEST_FILESEARCHENGINE_H
#define TEST_FILESEARCHENGINE_H

#include <QObject>
#include <qsynedit/searcher/filesearchengine.h>

namespace QSynedit {

class TestFileSearchEngine : public QObject
{
    Q_OBJECT
private slots:
    void test_search_text_plain_whole_word_and_case();
    void test_search_text_regex_unicode_and_zero_length();
    void test_search_text_batches_and_cancel();
    void test_search_file_encoding_and_errors();
    void test_collect_files_filters_recursion_and_cancel();
    void test_profile();
};

}

#endif // TEST_FILESEARCHENGINE_H
