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
#include "editorlist.h"
#include "syntaxermanager.h"
#include "project.h"

CppRefacter::CppRefacter(QObject *parent) : QObject(parent)
{

}

bool CppRefacter::findOccurence(Editor *editor, const QSynedit::BufferCoord &pos)
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
        std::shared_ptr<Project> project = pMainWindow->project();
        if (editor->inProject() && project) {
            doFindOccurenceInProject(statement,project,editor->parser());
        } else {
            doFindOccurenceInEditor(statement,editor,editor->parser());
        }
    }
    pMainWindow->searchResultModel()->notifySearchResultsUpdated();
    return true;
}

bool CppRefacter::findOccurence(const QString &statementFullname, SearchFileScope scope)
{
    PCppParser parser;
    Editor * editor=nullptr;
    std::shared_ptr<Project> project;
    if (scope == SearchFileScope::currentFile) {
        editor = pMainWindow->editorList()->getEditor();
        if (!editor)
            return false;
        parser = editor->parser();
    } else if (scope == SearchFileScope::wholeProject) {
        project = pMainWindow->project();
        if (!project)
            return false;
        parser = project->cppParser();
    }
    if (!parser)
        return false;
    {
        parser->freeze();
        auto action = finally([&parser]{
            parser->unFreeze();
        });
        PStatement statement = parser->findStatement(statementFullname);
        // definition of the symbol not found
        if (!statement)
            return false;

        if (statement->scope == StatementScope::Local || scope == SearchFileScope::currentFile) {
            doFindOccurenceInEditor(statement, editor,parser);
        } else if (scope == SearchFileScope::wholeProject) {
            doFindOccurenceInProject(statement,project,parser);
        }
        pMainWindow->searchResultModel()->notifySearchResultsUpdated();
        return true;
    }
}

static QString fullParentName(PStatement statement) {
    PStatement parent = statement->parentScope.lock();
    if (parent) {
        return parent->fullName;
    } else {
        return "";
    }
}
void CppRefacter::renameSymbol(Editor *editor, const QSynedit::BufferCoord &pos, const QString &newWord)
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
        expression = editor->getExpressionAtPosition(QSynedit::BufferCoord{pos.ch-1,pos.line});
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

void CppRefacter::doFindOccurenceInEditor(PStatement statement , Editor *editor, const PCppParser &parser)
{
    PSearchResults results = pMainWindow->searchResultModel()->addSearchResults(
                statement->command,
                statement->fullName,
                SearchFileScope::currentFile
                );
    PSearchResultTreeItem item = findOccurenceInFile(
                editor->filename(),
                editor->encodingOption(),
                statement,
                parser);
    if (item && !(item->results.isEmpty())) {
        results->results.append(item);
    }
}

void CppRefacter::doFindOccurenceInProject(PStatement statement, std::shared_ptr<Project> project, const PCppParser &parser)
{
    PSearchResults results = pMainWindow->searchResultModel()->addSearchResults(
                statement->command,
                statement->fullName,
                SearchFileScope::wholeProject
                );
    QProgressDialog progressDlg(
                tr("Searching..."),
                tr("Abort"),
                0,
                pMainWindow->project()->unitList().count(),
                pMainWindow);
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
    if (pMainWindow->editorList()->getContentFromOpenedEditor(
                filename,buffer)){
        editor.document()->setContents(buffer);
    } else if (!fileExists(filename)){
        return parentItem;
    } else {
        QByteArray encoding;
        try {
            editor.document()->loadFromFile(filename,fileEncoding,encoding);
        } catch (FileError e) {
            //don't handle it;
            return parentItem;
        }
    }
    editor.setSyntaxer(syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::CPP));
    int posY = 0;
    while (posY < editor.lineCount()) {
        QString line = editor.document()->getLine(posY);
        if (line.isEmpty()) {
            posY++;
            continue;
        }

        if (posY == 0) {
            editor.syntaxer()->resetState();
        } else {
            editor.syntaxer()->setState(
                        editor.document()->getSyntaxState(posY-1));
        }
        editor.syntaxer()->setLine(line,posY);
        while (!editor.syntaxer()->eol()) {
            int start = editor.syntaxer()->getTokenPos() + 1;
            QString token = editor.syntaxer()->getToken();
            QSynedit::PTokenAttribute attr = editor.syntaxer()->getTokenAttribute();
            if (attr && attr->tokenType()==QSynedit::TokenType::Identifier) {
                if (token == statement->command) {
                    //same name symbol , test if the same statement;
                    QSynedit::BufferCoord p;
                    p.line = posY+1;
                    p.ch = start+1;

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
    Editor * oldEditor=pMainWindow->editorList()->getOpenedEditorByFilename(filename);
    if (oldEditor){
        QSynedit::PSyntaxer syntaxer = syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::CPP);
        int posY = 0;
        oldEditor->clearSelection();
        oldEditor->addGroupBreak();
        oldEditor->beginEditing();
        while (posY < oldEditor->lineCount()) {
            QString line = oldEditor->document()->getLine(posY);
            if (posY == 0) {
                syntaxer->resetState();
            } else {
                syntaxer->setState(
                            oldEditor->document()->getSyntaxState(posY-1));
            }
            syntaxer->setLine(line,posY);
            QString newLine;
            while (!syntaxer->eol()) {
                int start = syntaxer->getTokenPos() + 1;
                QString token = syntaxer->getToken();
                if (token == statement->command) {
                    //same name symbol , test if the same statement;
                    QSynedit::BufferCoord p;
                    p.line = posY+1;
                    p.ch = start;

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
                oldEditor->replaceLine(posY+1,newLine);
            posY++;
        }
        oldEditor->endEditing();
    } else {
        Editor editor(nullptr);
        QByteArray encoding;
        editor.setSyntaxer(syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::CPP));
        try {
            editor.document()->loadFromFile(filename,ENCODING_AUTO_DETECT,encoding);
        } catch(FileError e) {
            QMessageBox::critical(pMainWindow,
                        tr("Rename Symbol Error"),
                        e.reason());
            return;
        }

        QStringList newContents;
        int posY = 0;
        while (posY < editor.lineCount()) {
            QString line = editor.document()->getLine(posY);
            if (posY == 0) {
                editor.syntaxer()->resetState();
            } else {
                editor.syntaxer()->setState(
                            editor.document()->getSyntaxState(posY-1));
            }
            editor.syntaxer()->setLine(line,posY);
            QString newLine;
            while (!editor.syntaxer()->eol()) {
                int start = editor.syntaxer()->getTokenPos() + 1;
                QString token = editor.syntaxer()->getToken();
                if (token == statement->command) {
                    //same name symbol , test if the same statement;
                    QSynedit::BufferCoord p;
                    p.line = posY+1;
                    p.ch = start;

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
            editor.document()->saveToFile(file,ENCODING_AUTO_DETECT,
                                       pSettings->editor().defaultEncoding(),
                                       realEncoding);
        } catch(FileError e) {
            QMessageBox::critical(pMainWindow,
                        tr("Rename Symbol Error"),
                        e.reason());
            return;
        }

    }
}
