#ifndef TEST_PPEXPANDER_H
#define TEST_PPEXPANDER_H
#include <QObject>
#include <QHash>
#include "src/parser/ppexpander.h"

class TestPpExpander: public QObject
{
    Q_OBJECT
public:
    TestPpExpander(QObject *parent=nullptr);
private slots:
    void test_object_macro();
    void test_empty_body_macro();
    void test_function_macro();
    void test_multiline_invocation();
    void test_chain_rescan();
    void test_mutual_recursion();
    void test_object_cycle();
    void test_stringize();
    void test_stringize_escaping();
    void test_paste();
    void test_paste_chain();
    void test_varargs();
    void test_gnu_comma_paste_idiom();
    void test_va_opt();
    void test_count_mismatch();
    void test_zero_param_macro();
    void test_unterminated_invocation();
    void test_not_invoked();
    void test_arg_pre_expansion();
    void test_word_boundaries();
    void test_empty_args();
    void test_condition_mode();
    void test_condition_macro_rescan();
private:
    static PDefine makeDefine(const QString& name, const QString& args,
                              const QString& value);
    static QString expand(const QHash<QString, PDefine>& defines,
                          const QString& line);
    static QString expandCondition(const QHash<QString, PDefine>& defines,
                                   const QString& expr);
};

#endif // TEST_PPEXPANDER_H
