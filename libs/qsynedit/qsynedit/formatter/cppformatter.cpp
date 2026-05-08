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
        QSynedit::PSyntaxer pSyntaxer = editor->syntaxer()->createInstance();
        std::shared_ptr<CppSyntaxer> cppSyntaxer = std::dynamic_pointer_cast<CppSyntaxer>(pSyntaxer);
        // test if previous line ending with '\', keep originalSpaces
        if (line>=1) {
            PSyntaxState  statePrevLine = editor->lineSyntaxState(line-1);
            if (cppSyntaxer->mergeWithNextLine(statePrevLine))
                return editor->leftSpaces(lineText);
        }
        // test if last line is non-end string
        int lastLine = line-1;
        if (lastLine>=0) {
            PSyntaxState rangeLastLine = editor->lineSyntaxState(lastLine);
            if (pSyntaxer->isStringNotFinished(rangeLastLine) )
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
            PSyntaxState rangePreceeding = editor->lineSyntaxState(startLine);
            if (rangePreceeding->state == CppSyntaxer::RangeState::rsRawStringNotEscaping)
                return 0;

            if (rangePreceeding->getLastIndentType() == IndentType::Parenthesis) {
                bool lastLineHasLastParentheis = true;
                if (startLine > 0) {
                    PSyntaxState synState = editor->lineSyntaxState(startLine - 1);
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
                pSyntaxer->setState(rangePreceeding);
                pSyntaxer->setLine(line, trimmedLineText, editor->lineSeq(line));
                PSyntaxState rangeAfterFirstToken = pSyntaxer->getState();
                QString firstToken = pSyntaxer->getToken();
                PTokenAttribute attr = pSyntaxer->getTokenAttribute();
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
                    pSyntaxer->setState(rangePreceeding);
                    pSyntaxer->setLine(line, "}", 0);
                    rangeAfterFirstToken = pSyntaxer->getState();
                    firstToken = pSyntaxer->getToken();
                    attr = pSyntaxer->getTokenAttribute();
                }
    //            qDebug()<<line<<lineText;
    //            qDebug()<<(int)rangeAfterFirstToken.lastUnindent.type<<rangeAfterFirstToken.lastUnindent.line;
                if (trimmedLineText.startsWith('#')
                           && attr == cppSyntaxer->preprocessorAttribute()) {
                    indentSpaces=0;
                } else if (pSyntaxer->isDocstringNotFinished(rangePreceeding)
                           ) {
                    // last line is a not finished comment,
                    if  (trimmedLineText.startsWith("*")) {
                        // this line start with "* "
                        // it means this line is a docstring, should indents according to
                        // the line the comment beginning , and add 1 additional space
                        int commentStartLine = findCommentStartLine(startLine,editor);
                        PSyntaxState range;
                        indentSpaces = editor->leftSpaces(editor->lineText(commentStartLine))+1;
                        range = editor->lineSyntaxState(commentStartLine);
                    } else {
                        //indents according to the beginning of the comment and 2 additional space
                        int commentStartLine = findCommentStartLine(startLine,editor);
                        PSyntaxState range;
                        indentSpaces = editor->leftSpaces(editor->lineText(commentStartLine))+2;
                        range = editor->lineSyntaxState(commentStartLine);
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

    bool CppFormatter::shouldRecalcIndent(int line, const QSynEdit *editor)
    {
        Q_ASSERT(editor!=nullptr);
        Q_ASSERT(line>=0);
        if (line==0)
            return true;
        std::shared_ptr<const CppSyntaxer> cppSyntaxer = std::dynamic_pointer_cast<const CppSyntaxer>(editor->syntaxer());
        // test if previous line ending with '\', keep original indents
        PSyntaxState  statePrevLine = editor->lineSyntaxState(line-1);
        if (cppSyntaxer->mergeWithNextLine(statePrevLine))
            return false;
        // test if previous line is non-end string
        if (cppSyntaxer->isRawStringNoEscape(statePrevLine))
            return false;

        return true;
    }

    int CppFormatter::findCommentStartLine(int searchStartLine, const QSynEdit* editor)
    {
        std::shared_ptr<const CppSyntaxer> cppSyntaxer = std::dynamic_pointer_cast<const CppSyntaxer>(editor->syntaxer());
        int commentStartLine = searchStartLine;
        PSyntaxState range;
        while (commentStartLine>=0) {
            range = editor->lineSyntaxState(commentStartLine);
            if (!cppSyntaxer->isBlockCommentNotFinished(range)){
                commentStartLine++;
                break;
            }
            commentStartLine--;
        }
        if (commentStartLine<0)
            commentStartLine = 0;
        return commentStartLine;
    }

    int CppFormatter::findLastParenthesis(int line, const QSynEdit *editor)
    {
        QString lineText = editor->lineText(line);
        int leading = editor->leftSpaces(lineText);
        PSyntaxer pSyntaxer = editor->syntaxer()->createInstance();
        editor->startParseLine(pSyntaxer.get(), line);
        QList<int> posList;
        while (!pSyntaxer->eol()) {
            if (pSyntaxer->getTokenAttribute() == pSyntaxer->symbolAttribute()) {
                if (pSyntaxer->getToken() == "(") {
                    posList.push_back(pSyntaxer->getTokenPos());
                } else if (pSyntaxer->getToken() == ")") {
                    if(!posList.empty())posList.pop_back();
                }
            }
            pSyntaxer->next();
        }
        if (posList.isEmpty())
            return leading;
        else
            return leading+posList.last();
    }
}
