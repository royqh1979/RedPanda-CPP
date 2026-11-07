#ifndef TEST_CPPPREPROCESSOR_INCLUDE_H
#define TEST_CPPPREPROCESSOR_INCLUDE_H
#include <QObject>
#include <QStringList>
#include <memory>

class CppPreprocessor;

class TestCppPreprocessorInclude: public QObject
{
    Q_OBJECT
public:
    TestCppPreprocessorInclude(QObject *parent=nullptr);
private slots:
    void test_basic_include();
    // group E: directives & include
    void test_nested_include();
    void test_include_guard();
    void test_pragma_once();
    void test_no_guard();
    void test_guard_after_comment();
    void test_guard_after_define();
    void test_guard_after_include();
    void test_include_via_macro();
    void test_recursive_include();
    void test_include_nonexistent();
    void test_include_next();
    void test_undef_external();
    void test_ignored_directives();
    void test_include_order_quote();
    void test_include_order_angle();
    void test_include_in_false_branch();
    void test_has_include();
    void test_recursive_include_env_sensitive();
    void test_env_sensitive_included_twice();
    void test_depth_bounded_iteration();
    void test_cycle_depth_cap();
    // group F: incremental / registry behavior
    void test_reprocess_same_instance();
    void test_reprocess_after_remove_scanned();
    void test_hard_define_restore();
    void test_undef_restore();
    void test_on_get_file_stream();
    // group G: replay correctness
    void test_replay_env_dependent();
    void test_replay_env_independent();
    void test_replay_config_macro_stable();
    void test_replay_unguarded_cross_reparse();
    void test_replay_external_modification();
private:
    // Absolute path of resources/preprocessor/<name>
    static QString scenarioPath(const QString& name);
    // Result lines with the scenario dir replaced by "<DIR>"
    static QStringList normalizedResult(CppPreprocessor& p, const QString& scenarioDir);
    static QStringList readGolden(const QString& fileName);
    // Preprocess <dir>/<mainFile> in the given include-once mode and
    // return the normalized result lines.
    static QStringList runScenario(CppPreprocessor& p, const QString& dir,
                                   const QString& mainFile);
};

#endif // TEST_CPPPREPROCESSOR_INCLUDE_H
