#ifndef TEST_CPPPARSERCOMPONENTTESTCASE_H
#define TEST_CPPPARSERCOMPONENTTESTCASE_H

#include <QObject>
#include <QStringList>
#include "src/parser/cpptokenizer.h"
#include "src/parser/cpptokencursor.h"
#include "src/parser/cpptypes.h"
#include "src/parser/cppdeclarationparser.h"
#include "src/parser/cppsyntaxparser.h"
#include "src/parser/cppstatementbuilder.h"
#include "src/parser/cpptyperesolver.h"
#include "src/parser/typeoperationregistry.h"
#include "src/parser/cppexpressionevaluator.h"
#include "src/parser/cppscopes.h"
#include "src/parser/parserutils.h"

class CppParserComponentTestCase : public QObject
{
public:
    explicit CppParserComponentTestCase(QObject *parent = nullptr);

protected:
    void tokenize(const QStringList &lines);
    bool parseDeclarationText(const QString &text, DeclarationInfo &result,
                              int cppVersion = CppVersion::Cpp17);
    bool parseSyntaxText(const QString &text, QVector<DeclarationInfo> &result,
                         int cppVersion = CppVersion::Cpp17);
    bool parseSyntaxFile(const QString &fileName, QVector<DeclarationInfo> &result);
    QStringList tokenTexts() const;
    static PStatement findStatement(const StatementMap &statements, const QString &name);

    CppTokenizer mTokenizer;
};

#endif // TEST_CPPPARSERCOMPONENTTESTCASE_H
