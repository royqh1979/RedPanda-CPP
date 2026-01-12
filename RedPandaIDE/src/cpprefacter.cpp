/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <QFile>
#include <QMessageBox>
#include <QProgressDialog>
#include <qsynedit/document.h>
#include "cpprefacter.h"
#include "mainwindow.h"
#include "settings.h"
#include "editor.h"
#include "editormanager.h"
#include "syntaxermanager.h"
#include "project.h"
#include "utils/file.h"

using QSynedit::CharPos;
CppRefacter::CppRefacter(MainWindow * pMain, QObject *parent) : QObject(parent)
{
    mMainWindow = pMain;
}

bool CppRefacter::findOccurence(Editor *editor, const CharPos &pos)
{
    if (!editor->parser())
        return false;
    if (!editor->parser()->freeze())
        return false;
    auto action = finally([&editor]{
        editor->parser()->unFreeze();
    });
    // get full phrase (such as s.name instead of name)
    QStringList expression = editor->getExpressionAtPosition(pos);
    // Find it's definition
    PStatement statement = editor->parser()->findStatementOf(
                editor->filename(),
                expression,
                pos.line);
    // definition of the symbol not found
    if (!statement)
        return false;

    if (statement->scope == StatementScope::Local) {
        doFindOccurenceInEditor(statement,editor,editor->parser());
    } else {
        std::shared_ptr<Project> project = mMainWindow->project();
        if (editor->inProject() && project) {
            doFindOccurenceInProject(statement,project,editor->parser());
        } else {
            doFindOccurenceInEditor(statement,editor,editor->parser());
        }
    }
    mMainWindow->searchResultModel()->notifySearchResultsUpdated();
    return true;
}

bool CppRefacter::findOccurence(Editor * editor, const QString &statementFullname, SearchFileScope scope)
{
    if (!editor->parser())
        return false;
    if (!editor->parser()->freeze())
        return false;
    auto action = finally([&editor]{
        editor->parser()->unFreeze();
    });
    PStatement statement = editor->parser()->findStatement(statementFullname);
    if (!statement)
        return false;

    if (statement->scope == StatementScope::Local) {
        doFindOccurenceInEditor(statement,editor,editor->parser());
    } else {
        std::shared_ptr<Project> project = mMainWindow->project();
        if (editor->inProject() && project) {
            doFindOccurenceInProject(statement,project,editor->parser());
        } else {
            doFindOccurenceInEditor(statement,editor,editor->parser());
        }
    }
    mMainWindow->searchResultModel()->notifySearchResultsUpdated();
    return true;
}

static QString fullParentName(PStatement statement) {
    PStatement parent = statement->parentScope.lock();
    if (parent) {
        return parent->fullName;
    } else {
        return "";
    }
}
void CppRefacter::renameSymbol(Editor *editor, const CharPos &pos, const QString &newWord)
{
    if (!editor->parser()->freeze())
        return;
    auto action = finally([&editor]{
        editor->parser()->unFreeze();
    });
    // get full phrase (such as s.name instead of name)
    QStringList expression;
    QChar s=editor->charAt(pos);
    if (!editor->isIdentStartChar(s)) {
        expression = editor->getExpressionAtPosition(CharPos{pos.ch-1,pos.line});
    } else {
        expression = editor->getExpressionAtPosition(pos);
    }
    // Find it's definition
    PStatement oldStatement = editor->parser()->findStatementOf(
                editor->filename(),
                expression,
                pos.line);
    // definition of the symbol not found
    if (!oldStatement)
        return;
    QString oldScope = fullParentName(oldStatement);
    // found but not in this file
    if (editor->filename() != oldStatement->fileName
            || editor->filename() != oldStatement->definitionFileName) {
            QMessageBox::critical(editor,
                              tr("Rename Symbol Error"),
                              tr("Can't rename symbols not defined in this file."));
        return;
    }

    QStringList newExpression = expression;
    newExpression[newExpression.count()-1]=newWord;
    PStatement newStatement = editor->parser()->findStatementOf(
                editor->filename(),
                newExpression,
                pos.line);
    if (newStatement && fullParentName(newStatement) == oldScope) {
        QMessageBox::critical(editor,
                              tr("Rename Symbol Error"),
                              tr("New symbol already exists!"));
        return;
    }
    renameSymbolInFile(editor->filename(),oldStatement,newWord, editor->parser());
}

void CppRefacter::renameUndefinedLocalVariable(Editor *editor, const CharPos &pos, const QString &newWord)
{
    if (!editor)
        return;
    if (!editor->parser()->freeze())
        return;
    auto action = finally([&editor]{
        editor->parser()->unFreeze();
    });
    PStatement scope = editor->parser()->findScopeStatement(editor->filename(), pos.line);
    if (!scope)
        return;
    if (scope->kind != StatementKind::Function
            && scope->kind != StatementKind::Constructor
            && scope->kind != StatementKind::Destructor
            && scope->kind != StatementKind::Lambda)
        return;
    if (scope->definitionFileName!=editor->filename())
        return;

    // get full phrase (such as s.name instead of name)
    QStringList expression;
    QChar s=editor->charAt(pos);
    if (!editor->isIdentStartChar(s)) {
        expression = editor->getExpressionAtPosition(CharPos{pos.ch-1,pos.line});
    } else {
        expression = editor->getExpressionAtPosition(pos);
    }

    if (expression.length()!=1)
        return;
    QString oldVarName = expression[0];
    // Find it's definition
    PStatement oldStatement = editor->parser()->findStatementOf(
                editor->filename(),
                expression,
                pos.line);
    if (oldStatement)
        return;

    QStringList newExpression = {newWord};
    PStatement newStatement = editor->parser()->findStatementOf(
                editor->filename(),
                newExpression,
                pos.line);
    QSynedit::PSyntaxer syntaxer = SyntaxerManager::getSyntaxer(QSynedit::ProgrammingLanguage::CPP);
    int posY = scope->definitionLine;
    editor->clearSelection();
    editor->beginEditing();
    while (posY < editor->lineCount()) {
        //check if still in the scope
        bool inScope = false;
        PStatement currentScope = editor->parser()->findScopeStatement(editor->filename(),posY);
        while (currentScope != nullptr) {
            if (currentScope == scope) {
                inScope = true;
                break;
            }
            currentScope = currentScope->parentScope.lock();
        }
        if (!inScope)
            break;

        QString line = editor->document()->getLine(posY);
        editor->startParseLine(syntaxer.get(), posY);
        QString newLine;
        while (!syntaxer->eol()) {
            QString token = syntaxer->getToken();
            if (token == oldVarName) {
                //same name symbol , test if the same statement;
                CharPos p;
                p.line = posY;
                p.ch = syntaxer->getTokenPos();

                QStringList expression = editor->getExpressionAtPosition(p);
                if (expression.length() == 1) {
                    PStatement tokenStatement = editor->parser()->findStatementOf(
                                editor->filename(),
                                expression, p.line);
                    if (!tokenStatement) {
                        token = newWord;
                    }
                }
            }
            newLine += token;
            syntaxer->next();
        }
        if (newLine!=line)
            editor->replaceLine(posY,newLine);
        posY++;
    }
    editor->setCaretXY(editor->ensureCharPosValid(editor->caretXY()));
    editor->endEditing();
}

void CppRefacter::doFindOccurenceInEditor(const PStatement &statement , Editor *editor, const PCppParser &parser)
{
    PSearchResults results = mMainWindow->searchResultModel()->addSearchResults(
                statement->command,
                statement->fullName,
                SearchFileScope::currentFile
                );
    PSearchResultTreeItem item = findOccurenceInFile(
                editor->filename(),
                editor->editorEncoding(),
                statement,
                parser);
    if (item && !(item->results.isEmpty())) {
        results->results.append(item);
    }
}

void CppRefacter::doFindOccurenceInProject(const PStatement &statement, std::shared_ptr<Project> project, const PCppParser &parser)
{
    PSearchResults results = mMainWindow->searchResultModel()->addSearchResults(
                statement->command,
                statement->fullName,
                SearchFileScope::wholeProject
                );
    QProgressDialog progressDlg(
                tr("Searching..."),
                tr("Abort"),
                0,
                mMainWindow->project()->unitList().count(),
                mMainWindow);
    progressDlg.setWindowModality(Qt::WindowModal);
    int i=0;
    foreach (const PProjectUnit& unit, project->unitList()) {
        i++;
        if (isCFile(unit->fileName()) || isHFile(unit->fileName())) {
            progressDlg.setValue(i);
            progressDlg.setLabelText(tr("Searching...")+"<br/>"+unit->fileName());

            if (progressDlg.wasCanceled())
                break;
            PSearchResultTreeItem item = findOccurenceInFile(
                        unit->fileName(),
                        unit->encoding()==ENCODING_PROJECT?project->options().encoding:unit->encoding(),
                        statement,
                        parser);
            if (item && !(item->results.isEmpty())) {
                results->results.append(item);
            }
        }
    }
}

PSearchResultTreeItem CppRefacter::findOccurenceInFile(
        const QString &filename,
        const QByteArray& fileEncoding,
        const PStatement &statement,
        const PCppParser& parser)
{
    PSearchResultTreeItem parentItem = std::make_shared<SearchResultTreeItem>();
    parentItem->filename = filename;
    parentItem->parent = nullptr;
    QStringList buffer;
    Editor editor(nullptr);
    FileType fileType = getFileType(filename);
    if (!isC_CPPSourceFile(fileType))
        fileType = FileType::CCppHeader;
    editor.setFileType(fileType);
    if (mMainWindow->editorManager()->getContentFromOpenedEditor(
                filename,buffer)){
        editor.setContent(buffer);
    } else if (!fileExists(filename)){
        return parentItem;
    } else {
        QByteArray encoding;
        try {
            editor.loadFromFile(filename,fileEncoding,encoding);
        } catch (FileError e) {
            //don't handle it;
            return parentItem;
        }
    }
    editor.setSyntaxer(SyntaxerManager::getSyntaxer(QSynedit::ProgrammingLanguage::CPP));
    int posY = 0;
    while (posY < editor.lineCount()) {
        QString line = editor.document()->getLine(posY);
        if (line.isEmpty()) {
            posY++;
            continue;
        }

        editor.startParseLine(editor.syntaxer().get(), posY);
        while (!editor.syntaxer()->eol()) {
            int start = editor.syntaxer()->getTokenPos();
            QString token = editor.syntaxer()->getToken();
            QSynedit::PTokenAttribute attr = editor.syntaxer()->getTokenAttribute();
            if (attr && attr->tokenType()==QSynedit::TokenType::Identifier) {
                if (token == statement->command) {
                    //same name symbol , test if the same statement;
                    CharPos p;
                    p.line = posY;
                    p.ch = start;
                    QStringList expression = editor.getExpressionAtPosition(p);
                    PStatement tokenStatement = parser->findStatementOf(
                                filename,
                                expression, p.line);
                    if (tokenStatement
                            && (tokenStatement->line == statement->line)
                            && (tokenStatement->fileName == statement->fileName)) {
                        PSearchResultTreeItem item = std::make_shared<SearchResultTreeItem>();
                        item->filename = filename;
                        item->line = p.line;
                        item->start = start;
                        item->len = token.length();
                        item->parent = parentItem.get();
                        item->text = line;
                        item->text.replace('\t',' ');
                        parentItem->results.append(item);
                    }
                }
            }
            editor.syntaxer()->next();
        }
        posY++;
    }
    return parentItem;
}

void CppRefacter::renameSymbolInFile(const QString &filename, const PStatement &statement,  const QString &newWord, const PCppParser &parser)
{
    QStringList buffer;
    Editor * oldEditor=mMainWindow->editorManager()->getOpenedEditor(filename);
    if (oldEditor){
        QSynedit::PSyntaxer syntaxer = SyntaxerManager::getSyntaxer(QSynedit::ProgrammingLanguage::CPP);
        int posY = 0;
        oldEditor->clearSelection();
        oldEditor->beginEditing();
        while (posY < oldEditor->lineCount()) {
            QString line = oldEditor->document()->getLine(posY);
            QString newLine;
            oldEditor->startParseLine(syntaxer.get(), posY);
            while (!syntaxer->eol()) {
                QString token = syntaxer->getToken();
                if (token == statement->command) {
                    //same name symbol , test if the same statement;
                    CharPos p;
                    p.line = posY;
                    p.ch = syntaxer->getTokenPos();

                    QStringList expression = oldEditor->getExpressionAtPosition(p);
                    PStatement tokenStatement = parser->findStatementOf(
                                filename,
                                expression, p.line);
                    if (tokenStatement
                            && (tokenStatement->line == statement->line)
                            && (tokenStatement->fileName == statement->fileName)) {
                        token = newWord;
                    }
                }
                newLine += token;
                syntaxer->next();
            }
            if (newLine!=line)
                oldEditor->replaceLine(posY,newLine);
            posY++;
        }
        oldEditor->setCaretXY(oldEditor->ensureCharPosValid(oldEditor->caretXY()));
        oldEditor->endEditing();
    } else {
        Editor editor(nullptr);
        QByteArray encoding;
        editor.setSyntaxer(SyntaxerManager::getSyntaxer(QSynedit::ProgrammingLanguage::CPP));
        try {
            editor.loadFromFile(filename,ENCODING_AUTO_DETECT,encoding);
        } catch(FileError e) {
            QMessageBox::critical(mMainWindow,
                        tr("Rename Symbol Error"),
                        e.reason());
            return;
        }

        QStringList newContents;
        int posY = 0;
        while (posY < editor.lineCount()) {
            QString line = editor.document()->getLine(posY);
            QString newLine;
            editor.startParseLine(editor.syntaxer().get(), posY);
            while (!editor.syntaxer()->eol()) {
                QString token = editor.syntaxer()->getToken();
                if (token == statement->command) {
                    //same name symbol , test if the same statement;
                    CharPos p;
                    p.line = posY;
                    p.ch = editor.syntaxer()->getTokenPos();

                    QStringList expression = editor.getExpressionAtPosition(p);
                    PStatement tokenStatement = parser->findStatementOf(
                                filename,
                                expression, p.line);
                    if (tokenStatement
                            && (tokenStatement->line == statement->line)
                            && (tokenStatement->fileName == statement->fileName)) {
                        token = newWord;
                    }
                }
                newLine += token;
                editor.syntaxer()->next();
            }
            newContents.append(newLine);
            posY++;
        }
        QByteArray realEncoding;
        QFile file(filename);
        try {
            editor.document()->saveToFile(file,editor.editorEncoding(),
                                       realEncoding);
        } catch(FileError e) {
            QMessageBox::critical(mMainWindow,
                        tr("Rename Symbol Error"),
                        e.reason());
            return;
        }

    }
}
