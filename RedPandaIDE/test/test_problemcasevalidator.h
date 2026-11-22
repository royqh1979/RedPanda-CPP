#ifndef TEST_PROBLEMCASEVALIDATOR_H
#define TEST_PROBLEMCASEVALIDATOR_H

#include <QObject>

class TestProblemCaseValidator : public QObject
{
    Q_OBJECT
private slots:
    void test_exact_data();
    void test_exact();
    void test_ignore_leading_trailing_spaces_data();
    void test_ignore_leading_trailing_spaces();
    void test_ignore_spaces_data();
    void test_ignore_spaces();
    void test_external_expected_file_and_fallback();
    void test_non_ascii_utf8();
    void test_invalid_utf8();
    void test_result_fields_are_reset();
    void test_null_case();
    void test_large_expected_file_data();
    void test_large_expected_file();
#ifdef REDPANDA_PROBLEMSET_PROFILE_TESTS
    void test_large_expected_file_profile_data();
    void test_large_expected_file_profile();
#endif
};

#endif
