#include "test_cppparsercomponenttestcase.h"
#include <QFile>
#include "src/parser/cpppreprocessor.h"

CppParserComponentTestCase::CppParserComponentTestCase(QObject *parent)
    : QObject(parent)
{
}

void CppParserComponentTestCase::tokenize(const QStringList &lines)
{
    mTokenizer.tokenize(lines);
}

QStringList CppParserComponentTestCase::tokenTexts() const
{
    QStringList result;
    for (const CppTokenizer::Token &token : mTokenizer.tokens())
        result.append(token.text);
    return result;
}

PStatement CppParserComponentTestCase::findStatement(const StatementMap &statements,
                                                      const QString &name)
{
    const auto values = statements.values(name);
    for (const std::weak_ptr<AbstractStatement> &statement : values) {
        if (PStatement result = statement.lock())
            return result;
    }
    return nullptr;
}

bool CppParserComponentTestCase::parseDeclarationText(const QString &text, DeclarationInfo &result,
                                            int cppVersion)
{
    CppTokenizer tokenizer;
    tokenizer.tokenize(text.split('\n'));
    CppTokenCursor cursor(tokenizer);
    CppDeclarationParser parser(cursor);
    parser.setCppVersion(cppVersion);
    return parser.parseDeclaration(result);
}

bool CppParserComponentTestCase::parseSyntaxText(const QString &text, QVector<DeclarationInfo> &result,
                                       int cppVersion)
{
    CppTokenizer tokenizer;
    tokenizer.tokenize(text.split('\n'));
    CppTokenCursor cursor(tokenizer);
    CppSyntaxParser parser(cursor);
    parser.setCppVersion(cppVersion);
    while (!cursor.atEnd()) {
        if (cursor.current().kind == CppTokenizer::TokenKind::Preprocessor
                || cursor.current().kind == CppTokenizer::TokenKind::FileBoundary) {
            cursor.advance();
            continue;
        }
        int before = cursor.index();
        DeclarationInfo declaration;
        if (!parser.parseDeclaration(declaration))
            return false;
        result.append(declaration);
        if (cursor.currentText() == "{") {
            cursor.skipToMatch();
            if (!cursor.atEnd())
                cursor.advance();
        }
        if (cursor.index() == before)
            return false;
    }
    return true;
}

bool CppParserComponentTestCase::parseSyntaxFile(const QString &fileName, QVector<DeclarationInfo> &result)
{
    QFile file("resources/syntax/" + fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QString text = QString::fromUtf8(file.readAll());
    text.remove('\r');
    QStringList lines = text.split('\n');
    CppPreprocessor::replaceCommentsBySpaceChar(lines);
    return parseSyntaxText(lines.join('\n'), result);
}
