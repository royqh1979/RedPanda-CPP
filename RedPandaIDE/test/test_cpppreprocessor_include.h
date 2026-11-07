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
private:
    // Absolute path of resources/preprocessor/<name>
    static QString scenarioPath(const QString& name);
    // Result lines with the scenario dir replaced by "<DIR>"
    static QStringList normalizedResult(CppPreprocessor& p, const QString& scenarioDir);
    static QStringList readGolden(const QString& fileName);
};

#endif // TEST_CPPPREPROCESSOR_INCLUDE_H
