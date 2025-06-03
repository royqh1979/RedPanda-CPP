#include "cppformatter.h"
#include "../qsynedit.h"
#include "../document.h"
#include "../syntaxer/cpp.h"
#include <QDebug>

namespace QSynedit {
    CppFormatter::CppFormatter()
    {

    }

    ProgrammingLanguage CppFormatter::supportLanguage()
    {
        return ProgrammingLanguage::CPP;
    }

    int CppFormatter::calcIndentSpaces(int line, const QString &lineText, bool addIndent,
                                              const QSynEdit *editor)
    {
        Q_ASSERT(editor!=nullptr);
        line = std::min(line, editor->lineCount()+1);
        if (line<=1)
            return 0;
        // test if last line is non-end string
        int lastLine = line-1;
        if (lastLine>=1) {
            SyntaxState rangeLastLine = editor->document()->getSyntaxState(lastLine-1);
            if (rangeLastLine.state == CppSyntaxer::RangeState::rsStringNextLine)
                return editor->leftSpaces(lineText);
        }
        // find the first non-empty preceeding line
        int startLine = line-1;
        QString startLineText;
        while (startLine>=1) {
            startLineText = editor->lineText(startLine);
            if (!startLineText.startsWith('#') && !startLineText.trimmed().isEmpty()) {
                break;
            }
            startLine -- ;
        }
        int indentSpaces = 0;
        if (startLine>=1) {
            //calculate the indents of last statement;
            indentSpaces = editor->leftSpaces(startLineText);
            if (editor->syntaxer()->language() != ProgrammingLanguage::CPP)
                return indentSpaces;
            SyntaxState rangePreceeding = editor->document()->getSyntaxState(startLine-1);
            if (rangePreceeding.state == CppSyntaxer::RangeState::rsRawStringNotEscaping)
                return 0;

            if (rangePreceeding.getLastIndentType() == IndentType::Parenthesis) {
                bool lastLineHasLastParentheis = true;
                if (startLine > 1) {
                    SyntaxState synState = editor->document()->getSyntaxState(startLine - 2);
                    lastLineHasLastParentheis = rangePreceeding.parenthesisLevel > synState.parenthesisLevel;
                }
                if (lastLineHasLastParentheis) {
                    indentSpaces = findLastParenthesis(startLine, editor);
                    if (addIndent)
                        indentSpaces += 1;
                }
                return indentSpaces;
            }
            if (addIndent) {
    //            QString trimmedS = s.trimmed();
                QString trimmedLineText = lineText.trimmed();
                editor->syntaxer()->setState(rangePreceeding);
                editor->syntaxer()->setLine(trimmedLineText,line-1);
                SyntaxState rangeAfterFirstToken = editor->syntaxer()->getState();
                QString firstToken = editor->syntaxer()->getToken();
                PTokenAttribute attr = editor->syntaxer()->getTokenAttribute();
                if (
                        attr->tokenType() == TokenType::Keyword

                        &&  lineText.endsWith(':')
                        && (
                            ( (
                                firstToken == "public" || firstToken == "private"
                                || firstToken == "protected"
                                 )
                            )
                            ||
                            ( (
                                 firstToken == "case"
                                 || firstToken == "default"
                                 )
                            )
                          )
                        ) {
                    // public: private: protecte: case: should indents like it's parent statement
                    editor->syntaxer()->setState(rangePreceeding);
                    editor->syntaxer()->setLine("}",line-1);
                    rangeAfterFirstToken = editor->syntaxer()->getState();
                    firstToken = editor->syntaxer()->getToken();
                    attr = editor->syntaxer()->getTokenAttribute();
                }
    //            qDebug()<<line<<lineText;
    //            qDebug()<<(int)rangeAfterFirstToken.lastUnindent.type<<rangeAfterFirstToken.lastUnindent.line;
                if (trimmedLineText.startsWith('#')
                           && attr == ((CppSyntaxer *)editor->syntaxer().get())->preprocessorAttribute()) {
                    indentSpaces=0;
                } else if (editor->syntaxer()->isDocstringNotFinished(rangePreceeding.state)
                           ) {
                    // last line is a not finished comment,
                    if  (trimmedLineText.startsWith("*")) {
                        // this line start with "* "
                        // it means this line is a docstring, should indents according to
                        // the line the comment beginning , and add 1 additional space
                        int commentStartLine = findCommentStartLine(startLine-1,editor);
                        SyntaxState range;
                        indentSpaces = editor->leftSpaces(editor->lineText(commentStartLine))+1;
                        range = editor->document()->getSyntaxState(commentStartLine-1);
                    } else {
                        //indents according to the beginning of the comment and 2 additional space
                        int commentStartLine = findCommentStartLine(startLine-1,editor);
                        SyntaxState range;
                        indentSpaces = editor->leftSpaces(editor->lineText(commentStartLine))+2;
                        range = editor->document()->getSyntaxState(commentStartLine-1);
                    }
                } else if (rangeAfterFirstToken.lastUnindent.type!=IndentType::None
                           && firstToken=="}") {
                    IndentInfo matchingIndents = rangeAfterFirstToken.lastUnindent;
                    indentSpaces = editor->leftSpaces(editor->lineText(matchingIndents.line+1));
                } else if (firstToken=="{") {
                    IndentInfo matchingIndents = rangeAfterFirstToken.getLastIndent();
                    if (matchingIndents.line!=line-1) {
                        indentSpaces = editor->leftSpaces(editor->lineText(matchingIndents.line+1));
                    } else if (rangeAfterFirstToken.indents.count()>=2){
                        IndentInfo info =  rangeAfterFirstToken.indents[rangeAfterFirstToken.indents.count()-2];
                        indentSpaces = editor->leftSpaces(editor->lineText(info.line+1))+editor->tabSize();
                    } else
                        indentSpaces = 0;
                } else if (firstToken=="else") {
                    IndentInfo matchingIndents = rangeAfterFirstToken.getLastIndent();
                    if (matchingIndents.line == line-1) {
                        if (rangeAfterFirstToken.indents.count()>=2){
                            IndentInfo info =  rangeAfterFirstToken.indents[rangeAfterFirstToken.indents.count()-2];
                            indentSpaces = editor->leftSpaces(editor->lineText(info.line+1))+editor->tabSize();
                        } else {
                            indentSpaces = 0;
                        }
                    }
                } else if (rangePreceeding.getLastIndentType()!=IndentType::None) {
                    IndentInfo matchingIndents = rangePreceeding.getLastIndent();
                    indentSpaces = editor->leftSpaces(editor->lineText(matchingIndents.line+1))+editor->tabSize();
                } else {
                    indentSpaces = 0;
                }
            }
        }
        return std::max(0,indentSpaces);
    }

    int CppFormatter::findCommentStartLine(int searchStartLine, const QSynEdit* editor)
    {
        int commentStartLine = searchStartLine;
        SyntaxState range;
        while (commentStartLine>=1) {
            range = editor->document()->getSyntaxState(commentStartLine-1);
            if (!editor->syntaxer()->isCommentNotFinished(range.state)){
                commentStartLine++;
                break;
            }
            commentStartLine--;
        }
        if (commentStartLine<1)
            commentStartLine = 1;
        return commentStartLine;
    }

    void CppFormatter::doInitOptions()
    {
    }

    int CppFormatter::findLastParenthesis(int line, const QSynEdit *editor)
    {
        QString lineText = editor->lineText(line);
        int leading = editor->leftSpaces(lineText);
        editor->prepareSyntaxerState(*(editor->syntaxer()), line-1, lineText.trimmed());
//        if (line == 1)
//            editor->syntaxer()->resetState();
//        else {
//            SyntaxState rangeState = editor->document()->getSyntaxState(line-2);
//            editor->syntaxer()->setState(rangeState);
//        }
//        editor->syntaxer()->setLine(lineText.trimmed(),line);
        QList<int> posList;
        while (!editor->syntaxer()->eol()) {
            if (editor->syntaxer()->getTokenAttribute() == editor->syntaxer()->symbolAttribute()) {
                if (editor->syntaxer()->getToken() == "(") {
                    posList.push_back(editor->syntaxer()->getTokenPos());
                } else if (editor->syntaxer()->getToken() == ")") {
                    posList.pop_back();
                }
            }
            editor->syntaxer()->next();
        }
        if (posList.isEmpty())
            return leading;
        else
            return leading+posList.last();
    }
}
