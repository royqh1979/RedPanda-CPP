#include "test_cppparsertestcase.h"
#include <QFile>
#include <QFileInfo>

CppParserTestCase::CppParserTestCase(QObject *parent):
    QObject{parent}
{
    init_parser();
}

void CppParserTestCase::init_parser()
{
    mParser = std::make_shared<CppParser>();
}

bool CppParserTestCase::writeLines(const QString &path, const QStringList &lines)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(lines.join('\n').toUtf8());
    file.close();
    return true;
}

QString CppParserTestCase::parseSyntaxResource(const PCppParser &parser, const QString &fileName)
{
    QString path = QFileInfo("resources/syntax/" + fileName).absoluteFilePath();
    parser->setOnGetFileStream(nullptr, nullptr);
    CppParser::parseFileBlocking(parser, path, "");
    return path;
}
