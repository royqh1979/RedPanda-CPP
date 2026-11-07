#include "test_cpppreprocessor_include.h"
#include "src/parser/cpppreprocessor.h"
#include "qt_utils/utils.h"
#include <QFileInfo>
#include <QTest>

TestCppPreprocessorInclude::TestCppPreprocessorInclude(QObject *parent):
    QObject{parent}
{

}

QString TestCppPreprocessorInclude::scenarioPath(const QString &name)
{
    QFileInfo info(QString("resources/preprocessor/%1").arg(name));
    return info.absoluteFilePath();
}

QStringList TestCppPreprocessorInclude::normalizedResult(CppPreprocessor &p, const QString &scenarioDir)
{
    QStringList result = p.result();
    for (QString& line : result) {
        line.replace(scenarioDir, "<DIR>");
    }
    return result;
}

QStringList TestCppPreprocessorInclude::readGolden(const QString &fileName)
{
    QStringList lines = readFileToLines(fileName);
    return lines;
}

void TestCppPreprocessorInclude::test_basic_include()
{
    QString dir = scenarioPath("basic_include");
    CppPreprocessor preprocessor;
    preprocessor.preprocess(dir + "/main.cpp");
    QCOMPARE(normalizedResult(preprocessor, dir),
             readGolden(dir + "/main-result.cpp"));
}
