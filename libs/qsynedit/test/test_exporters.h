#ifndef TEST_EXPORTERS_H
#define TEST_EXPORTERS_H

#include <QObject>

class TestHTMLExporter : public QObject
{
    Q_OBJECT
private slots:
    void test_full_document_and_reserved_chars();
    void test_ranges_data();
    void test_ranges();
    void test_empty_and_reused_exports();
    void test_fragment_and_file_output();
    void test_write_errors();
    void test_line_numbers_data();
    void test_line_numbers();
    void test_combined_text_decorations();
#ifdef QSYNEDIT_PROFILE_TESTS
    void test_large_document_profile_data();
    void test_large_document_profile();
#endif
};

class TestRTFExporter : public QObject
{
    Q_OBJECT
private slots:
    void test_full_document_and_reserved_chars();
    void test_ranges();
    void test_empty_and_reused_exports();
    void test_file_output_and_errors();
    void test_styles_and_colors();
    void test_background_not_overridden_by_whitespace();
    void test_unicode();
#ifdef QSYNEDIT_PROFILE_TESTS
    void test_large_document_profile_data();
    void test_large_document_profile();
#endif
};

#endif
