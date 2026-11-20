#ifndef TEST_CPPPARSER_TESTCASE_H
#define TEST_CPPPARSER_TESTCASE_H

#include <QObject>
#include <memory>
#include "src/parser/cppparser.h"

class CppParserTestCase : public QObject
{
public:
    explicit CppParserTestCase(QObject *parent = nullptr);

protected:
    void init_parser();
    static bool writeLines(const QString &path, const QStringList &lines);
    static QString parseSyntaxResource(const PCppParser &parser, const QString &fileName);

    std::shared_ptr<CppParser> mParser;
};

#endif // TEST_CPPPARSER_TESTCASE_H
