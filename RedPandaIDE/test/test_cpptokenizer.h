#ifndef TEST_CPPTOKENIZER_H
#define TEST_CPPTOKENIZER_H

#include <QObject>
#include <QStringList>
#include "src/parser/cpptokenizer.h"

class TestCppTokenizer : public QObject
{
    Q_OBJECT
public:
    explicit TestCppTokenizer(QObject *parent = nullptr);

private slots:
    void test_identifiers_and_operators();
    void test_qualified_names_and_templates_are_split();
    void test_array_and_lambda_brackets_are_split();
    void test_integer_and_floating_literals();
    void test_string_and_character_literals();
    void test_escaped_literals();
    void test_raw_string_with_custom_delimiter();
    void test_unterminated_raw_string();
    void test_balanced_match_indices();
    void test_unmatched_pairs_use_file_end();
    void test_file_boundaries_and_file_names();
    void test_cross_file_match_indices();
    void test_unmatched_include_pair_uses_include_end();
    void test_literal_prefixes();
    void test_attributes_are_skipped();
    void test_preprocessor_tokens();
    void test_line_numbers();

private:
    void tokenize(const QStringList &lines);
    QStringList tokenTexts() const;

    CppTokenizer mTokenizer;
};

#endif // TEST_CPPTOKENIZER_H
