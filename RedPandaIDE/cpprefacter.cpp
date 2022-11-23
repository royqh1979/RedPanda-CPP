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
#include "cpprefacter.h"
#include "mainwindow.h"
#include "settings.h"
#include "editor.h"
#include "editorlist.h"
#include <QFile>
#include <QMessageBox>
#include <QTextCodec>
#include "HighlighterManager.h"
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

    std::shared_ptr<Project> project = pMainWindow->project();
    if (editor->inProject() && project) {
        doFindOccurenceInProject(statement,project,editor->parser());
    } else {
        doFindOccurenceInEditor(statement,editor,editor->parser());
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

        if (scope == SearchFileScope::wholeProject) {
            doFindOccurenceInProject(statement,project,parser);
        } else if (scope == SearchFileScope::currentFile) {
            doFindOccurenceInEditor(statement, editor,parser);
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
    if (!editor->isIdentChar(s)) {
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
    foreach (const PProjectUnit& unit, project->unitList()) {
        if (isCFile(unit->fileName()) || isHFile(unit->fileName())) {
            PSearchResultTreeItem item = findOccurenceInFile(
                        unit->fileName(),
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
    } else {
        QByteArray encoding;
        editor.document()->loadFromFile(filename,ENCODING_AUTO_DETECT,encoding);
    }
    editor.setHighlighter(HighlighterManager().getCppHighlighter());
    int posY = 0;
    while (posY < editor.document()->count()) {
        QString line = editor.document()->getString(posY);
        if (line.isEmpty()) {
            posY++;
            continue;
        }

        if (posY == 0) {
            editor.highlighter()->resetState();
        } else {
            editor.highlighter()->setState(
                        editor.document()->ranges(posY-1));
        }
        editor.highlighter()->setLine(line,posY);
        while (!editor.highlighter()->eol()) {
            int start = editor.highlighter()->getTokenPos() + 1;
            QString token = editor.highlighter()->getToken();
            QSynedit::PHighlighterAttribute attr = editor.highlighter()->getTokenAttribute();
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
            editor.highlighter()->next();
        }
        posY++;
    }
    return parentItem;
}

void CppRefacter::renameSymbolInFile(const QString &filename, const PStatement &statement,  const QString &newWord, const PCppParser &parser)
{
    QStringList buffer;
    Editor editor(nullptr);
    if (pMainWindow->editorList()->getContentFromOpenedEditor(
                filename,buffer)){
        editor.document()->setContents(buffer);
    } else {
        QByteArray encoding;
        editor.document()->loadFromFile(filename,ENCODING_AUTO_DETECT,encoding);
    }
    QStringList newContents;
    editor.setHighlighter(HighlighterManager().getCppHighlighter());
    int posY = 0;
    while (posY < editor.document()->count()) {
        QString line = editor.document()->getString(posY);

        if (posY == 0) {
            editor.highlighter()->resetState();
        } else {
            editor.highlighter()->setState(
                        editor.document()->ranges(posY-1));
        }
        editor.highlighter()->setLine(line,posY);
        QString newLine;
        while (!editor.highlighter()->eol()) {
            int start = editor.highlighter()->getTokenPos() + 1;
            QString token = editor.highlighter()->getToken();
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
            editor.highlighter()->next();
        }
        newContents.append(newLine);
        posY++;
    }

    Editor * oldEditor = pMainWindow->editorList()->getOpenedEditorByFilename(filename);
    if (oldEditor) {
        QSynedit::BufferCoord oldXY=oldEditor->caretXY();
        int topLine = oldEditor->topLine();
        int leftChar = oldEditor->leftChar();
        oldEditor->beginUndoBlock();
        oldEditor->addLeftTopToUndo();
        oldEditor->addCaretToUndo();
        oldEditor->replaceAll(newContents.join(oldEditor->lineBreak()));
        oldEditor->setTopLine(topLine);
        oldEditor->setLeftChar(leftChar);
        oldEditor->setCaretXY(oldXY);
        oldEditor->endUndoBlock();
    } else {
        QByteArray realEncoding;
        QFile file(filename);
        editor.document()->saveToFile(file,ENCODING_AUTO_DETECT,
                                   pSettings->editor().defaultEncoding(),
                                   realEncoding);
    }
}
