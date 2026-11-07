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
private:
    // Absolute path of resources/preprocessor/<name>
    static QString scenarioPath(const QString& name);
    // Result lines with the scenario dir replaced by "<DIR>"
    static QStringList normalizedResult(CppPreprocessor& p, const QString& scenarioDir);
    static QStringList readGolden(const QString& fileName);
    // Preprocess <dir>/<mainFile> in the given include-once mode and
    // return the normalized result lines.
    static QStringList runScenario(CppPreprocessor& p, const QString& dir,
                                   const QString& mainFile, bool includeOnce);
};

#endif // TEST_CPPPREPROCESSOR_INCLUDE_H
