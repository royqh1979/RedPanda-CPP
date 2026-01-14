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
        if (lineText.startsWith("//"))
            return 0;
        line = std::min(line, editor->lineCount());
        if (line<=0)
            return 0;
        std::shared_ptr<CppSyntaxer> cppSyntaxer = std::dynamic_pointer_cast<CppSyntaxer>(editor->syntaxer());
        // test if previous line ending with '\', keep originalSpaces
        PSyntaxState  statePrevLine = editor->document()->getSyntaxState(line-1);
        if (cppSyntaxer->mergeWithNextLine(statePrevLine))
            return editor->leftSpaces(lineText);
        // test if last line is non-end string
        int lastLine = line-1;
        if (lastLine>=1) {
            PSyntaxState rangeLastLine = editor->document()->getSyntaxState(lastLine-1);
            if (editor->syntaxer()->isStringNotFinished(rangeLastLine) )
                return editor->leftSpaces(lineText);
        }
        // find the first non-empty preceeding line
        int startLine = line-1;
        QString startLineText;
        while (startLine>=0) {
            startLineText = editor->lineText(startLine);
            if (!startLineText.startsWith('#') && !startLineText.trimmed().isEmpty()) {
                break;
            }
            startLine -- ;
        }
        int indentSpaces = 0;
        if (startLine>=0) {
            //calculate the indents of last statement;
            indentSpaces = editor->leftSpaces(startLineText);
            PSyntaxState rangePreceeding = editor->document()->getSyntaxState(startLine);
            if (rangePreceeding->state == CppSyntaxer::RangeState::rsRawStringNotEscaping)
                return 0;

            if (rangePreceeding->getLastIndentType() == IndentType::Parenthesis) {
                bool lastLineHasLastParentheis = true;
                if (startLine > 0) {
                    PSyntaxState synState = editor->document()->getSyntaxState(startLine - 1);
                    lastLineHasLastParentheis = rangePreceeding->parenthesisLevel > synState->parenthesisLevel;
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
                editor->syntaxer()->setLine(line, trimmedLineText, editor->lineSeq(line));
                PSyntaxState rangeAfterFirstToken = editor->syntaxer()->getState();
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
                    editor->syntaxer()->setLine(line, "}", 0);
                    rangeAfterFirstToken = editor->syntaxer()->getState();
                    firstToken = editor->syntaxer()->getToken();
                    attr = editor->syntaxer()->getTokenAttribute();
                }
    //            qDebug()<<line<<lineText;
    //            qDebug()<<(int)rangeAfterFirstToken.lastUnindent.type<<rangeAfterFirstToken.lastUnindent.line;
                if (trimmedLineText.startsWith('#')
                           && attr == ((CppSyntaxer *)editor->syntaxer().get())->preprocessorAttribute()) {
                    indentSpaces=0;
                } else if (editor->syntaxer()->isDocstringNotFinished(rangePreceeding)
                           ) {
                    // last line is a not finished comment,
                    if  (trimmedLineText.startsWith("*")) {
                        // this line start with "* "
                        // it means this line is a docstring, should indents according to
                        // the line the comment beginning , and add 1 additional space
                        int commentStartLine = findCommentStartLine(startLine,editor);
                        PSyntaxState range;
                        indentSpaces = editor->leftSpaces(editor->lineText(commentStartLine))+1;
                        range = editor->document()->getSyntaxState(commentStartLine);
                    } else {
                        //indents according to the beginning of the comment and 2 additional space
                        int commentStartLine = findCommentStartLine(startLine,editor);
                        PSyntaxState range;
                        indentSpaces = editor->leftSpaces(editor->lineText(commentStartLine))+2;
                        range = editor->document()->getSyntaxState(commentStartLine);
                    }
                } else if (rangeAfterFirstToken->lastUnindent.type!=IndentType::None
                           && firstToken=="}") {
                    IndentInfo matchingIndents = rangeAfterFirstToken->lastUnindent;
                    QString matchingLineText;
                    if (editor->findLineTextBySeq(matchingIndents.lineSeq, matchingLineText)) {
                        indentSpaces = editor->leftSpaces(matchingLineText);
                    }
                } else if (firstToken=="{") {
                    IndentInfo matchingIndents = rangeAfterFirstToken->getLastIndent();
                    QString matchingLineText;
                    if (editor->lineSeq(line) != matchingIndents.lineSeq // not the same line
                            && editor->findLineTextBySeq(matchingIndents.lineSeq, matchingLineText)) {
                        indentSpaces = editor->leftSpaces(matchingLineText);
                    } else if (rangeAfterFirstToken->indents.count()>=2){
                        IndentInfo info =  rangeAfterFirstToken->indents[rangeAfterFirstToken->indents.count()-2];                        
                        QString infoLineText;
                        if (editor->findLineTextBySeq(info.lineSeq,infoLineText)) {
                            indentSpaces = editor->leftSpaces(infoLineText)+editor->tabSize();
                        }
                    } else
                        indentSpaces = 0;
                } else if (firstToken=="else") {
                    IndentInfo matchingIndents = rangeAfterFirstToken->getLastIndent();
                    if (matchingIndents.lineSeq == editor->lineSeq(line)) {
                        if (rangeAfterFirstToken->indents.count()>=2){
                            IndentInfo info =  rangeAfterFirstToken->indents[rangeAfterFirstToken->indents.count()-2];
                            QString infoLineText;
                            if (editor->findLineTextBySeq(info.lineSeq, infoLineText)) {
                                indentSpaces = editor->leftSpaces(infoLineText)+editor->tabSize();
                            }
                        } else {
                            indentSpaces = 0;
                        }
                    }
                } else if (rangePreceeding->getLastIndentType()!=IndentType::None) {
                    IndentInfo matchingIndents = rangePreceeding->getLastIndent();
                    QString matchingLineText;
                    if (editor->findLineTextBySeq(matchingIndents.lineSeq, matchingLineText)) {
                        indentSpaces = editor->leftSpaces(matchingLineText)+editor->tabSize();
                    }
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
        PSyntaxState range;
        while (commentStartLine>=0) {
            range = editor->document()->getSyntaxState(commentStartLine);
            if (!editor->syntaxer()->isCommentNotFinished(range)){
                commentStartLine++;
                break;
            }
            commentStartLine--;
        }
        if (commentStartLine<0)
            commentStartLine = 0;
        return commentStartLine;
    }

    void CppFormatter::doInitOptions()
    {
    }

    int CppFormatter::findLastParenthesis(int line, const QSynEdit *editor)
    {
        QString lineText = editor->lineText(line);
        int leading = editor->leftSpaces(lineText);
        editor->startParseLine(editor->syntaxer().get(), line);
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
