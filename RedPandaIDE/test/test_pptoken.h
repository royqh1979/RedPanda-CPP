#ifndef TEST_PPTOKEN_H
#define TEST_PPTOKEN_H
#include <QObject>
#include <QStringList>
#include "src/parser/pptoken.h"

class TestPpTokenizer: public QObject
{
    Q_OBJECT
public:
    TestPpTokenizer(QObject *parent=nullptr);
private slots:
    void test_words_and_punct();
    void test_space_before_flags();
    void test_numbers();
    void test_strings();
    void test_char_literals();
    void test_unterminated_string();
    void test_sharp_tokens();
    void test_raw_string_single_line();
    void test_raw_string_delimiter();
    void test_raw_string_multiline();
    void test_raw_string_ill_formed();
    void test_end_of_line_markers();
    void test_split_macro_params();
    void test_compile_replacement_list();
private:
    // tokenize all input and return "kind:text" pairs for comparison
    static QStringList dumpTokens(const QStringList& lines);
    static QString dumpToken(const PpToken& token);
    static QStringList dumpRepl(const ReplTokenList& entries);
};

#endif // TEST_PPTOKEN_H
