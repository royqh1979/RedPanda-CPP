#ifndef QSYNEDIT_CPPFORMATTER_H
#define QSYNEDIT_CPPFORMATTER_H

#include "formatter.h"

namespace QSynedit {
class CppFormatter : public Formatter
{
public:
    CppFormatter();

    // IndentCalculator interface
public:
    ProgrammingLanguage supportLanguage() override;
    int calcIndentSpaces(int line, const QString &lineText, bool addIndent,
                         const QSynEdit *editor) override;


    // IndentCalculator interface
protected:
    int findCommentStartLine(int searchStartLine, const QSynEdit *editor);
    void doInitOptions() override;
    int findLastParenthesis(int line, const QSynEdit *editor);
};
}

#endif // CPPFORMATTER_H
