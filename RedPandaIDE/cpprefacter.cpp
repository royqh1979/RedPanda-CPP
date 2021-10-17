#include "cpprefacter.h"
#include "mainwindow.h"
#include "settings.h"
#include "editor.h"
#include "editorlist.h"
#include <QFile>
#include <QMessageBox>
#include "HighlighterManager.h"
#include "project.h"

CppRefacter::CppRefacter(QObject *parent) : QObject(parent)
{

}

bool CppRefacter::findOccurence(Editor *editor, const BufferCoord &pos)
{
    if (!editor->parser()->freeze())
        return false;
    auto action = finally([&editor]{
        editor->parser()->unFreeze();
    });
    // get full phrase (such as s.name instead of name)
    BufferCoord pBeginPos,pEndPos;
    QString phrase = getWordAtPosition(editor,pos,pBeginPos,pEndPos,Editor::WordPurpose::wpInformation);
    // Find it's definition
    PStatement statement = editor->parser()->findStatementOf(
                editor->filename(),
                phrase,
                pos.Line);
    // definition of the symbol not found
    if (!statement)
        return false;

    std::shared_ptr<Project> project = pMainWindow->project();
    if (editor->inProject() && project) {
        foreach (const PProjectUnit& unit, project->units()) {
            if (isCfile(unit->fileName()) || isHfile(unit->fileName())) {
                findOccurenceInFile(
                        phrase,
                        unit->fileName(),
                        statement,
                        pos.Line,
                        editor->parser());
            }
        }
    } else {
        findOccurenceInFile(
                    phrase,
                    editor->filename(),
                    statement,
                    pos.Line,
                    editor->parser());
    }
    pMainWindow->searchResultModel()->notifySearchResultsUpdated();
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
void CppRefacter::renameSymbol(Editor *editor, const BufferCoord &pos, const QString &word, const QString &newWord)
{
    if (!editor->parser()->freeze())
        return;
    auto action = finally([&editor]{
        editor->parser()->unFreeze();
    });
    // get full phrase (such as s.name instead of name)
    BufferCoord pBeginPos,pEndPos;
    QString phrase = getWordAtPosition(editor,pos,pBeginPos,pEndPos,Editor::WordPurpose::wpInformation);
    // Find it's definition
    PStatement oldStatement = editor->parser()->findStatementOf(
                editor->filename(),
                phrase,
                pos.Line);
    QString oldScope = fullParentName(oldStatement);
    // definition of the symbol not found
    if (!oldStatement)
        return;
    // found but not in this file
    if (editor->filename() != oldStatement->fileName
            || editor->filename() != oldStatement->definitionFileName) {
            QMessageBox::critical(editor,
                              tr("Rename Symbol Error"),
                              tr("Can't rename symbols not defined in this file."));
        return;
    }

    QString newPhrase = phrase.mid(0,phrase.length()-word.length()) + newWord;
    PStatement newStatement = editor->parser()->findStatementOf(
                editor->filename(),
                newPhrase,
                pos.Line);
    if (newStatement && fullParentName(newStatement) == oldScope) {
        QMessageBox::critical(editor,
                              tr("Rename Symbol Error"),
                              tr("New symbol already exists!"));
        return;
    }
    renameSymbolInFile(editor->filename(),oldStatement,word,newWord, editor->parser());
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
    SynEdit editor;
    if (pMainWindow->editorList()->getContentFromOpenedEditor(
                filename,buffer)){
        editor.lines()->setContents(buffer);
    } else {
        QByteArray encoding;
        editor.lines()->loadFromFile(filename,ENCODING_AUTO_DETECT,encoding);
    }
    editor.setHighlighter(HighlighterManager().getCppHighlighter());
    int posY = 0;
    while (posY < editor.lines()->count()) {
        QString line = editor.lines()->getString(posY);
        if (line.isEmpty()) {
            posY++;
            continue;
        }

        if (posY == 0) {
            editor.highlighter()->resetState();
        } else {
            editor.highlighter()->setState(
                        editor.lines()->ranges(posY-1));
        }
        editor.highlighter()->setLine(line,posY);
        while (!editor.highlighter()->eol()) {
            int start = editor.highlighter()->getTokenPos() + 1;
            QString token = editor.highlighter()->getToken();
            if (token == statement->command) {
                //same name symbol , test if the same statement;
                BufferCoord p,pBeginPos,pEndPos;
                p.Line = posY+1;
                p.Char = start;
                QString phrase = getWordAtPosition(&editor, p, pBeginPos,pEndPos,
                                                   Editor::WordPurpose::wpInformation);
                PStatement tokenStatement = parser->findStatementOf(
                            filename,
                            phrase, p.Line);
                if (tokenStatement
                        && (tokenStatement->line == statement->line)
                        && (tokenStatement->fileName == statement->fileName)) {
                    PSearchResultTreeItem item = std::make_shared<SearchResultTreeItem>();
                    item->filename = filename;
                    item->line = p.Line;
                    item->start = start;
                    item->len = phrase.length();
                    item->parent = parentItem.get();
                    item->text = line;
                    item->text.replace('\t',' ');
                    parentItem->results.append(item);
                }
            }
            editor.highlighter()->next();
        }
        posY++;
    }
    return parentItem;
}

void CppRefacter::renameSymbolInFile(const QString &filename, const PStatement &statement, const QString &word, const QString &newWord, const PCppParser &parser)
{
    QStringList buffer;
    SynEdit editor;
    if (pMainWindow->editorList()->getContentFromOpenedEditor(
                filename,buffer)){
        editor.lines()->setContents(buffer);
    } else {
        QByteArray encoding;
        editor.lines()->loadFromFile(filename,ENCODING_AUTO_DETECT,encoding);
    }
    QStringList newContents;
    editor.setHighlighter(HighlighterManager().getCppHighlighter());
    int posY = 0;
    while (posY < editor.lines()->count()) {
        QString line = editor.lines()->getString(posY);
        if (line.isEmpty()) {
            posY++;
            continue;
        }

        if (posY == 0) {
            editor.highlighter()->resetState();
        } else {
            editor.highlighter()->setState(
                        editor.lines()->ranges(posY-1));
        }
        editor.highlighter()->setLine(line,posY);
        QString newLine;
        while (!editor.highlighter()->eol()) {
            int start = editor.highlighter()->getTokenPos() + 1;
            QString token = editor.highlighter()->getToken();
            if (token == statement->command) {
                //same name symbol , test if the same statement;
                BufferCoord p,pBeginPos,pEndPos;
                p.Line = posY+1;
                p.Char = start;
                QString phrase = getWordAtPosition(&editor, p, pBeginPos,pEndPos,
                                                   Editor::WordPurpose::wpInformation);
                PStatement tokenStatement = parser->findStatementOf(
                            filename,
                            phrase, p.Line);
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
        oldEditor->selectAll();
        oldEditor->setSelText(newContents.join(oldEditor->lineBreak()));
    } else {
        QByteArray realEncoding;
        QFile file(filename);
        editor.lines()->saveToFile(file,ENCODING_AUTO_DETECT, realEncoding);
    }
}

void CppRefacter::findOccurenceInFile(
        const QString& phrase,
        const QString &filename,
        const PStatement &statement,
        int line,
        const PCppParser &parser)
{
    PSearchResults results = pMainWindow->searchResultModel()->addSearchResults(
                phrase,
                filename,
                line
                );

    PSearchResultTreeItem item = findOccurenceInFile(
                filename,
                statement,
                parser);
    if (item && !(item->results.isEmpty())) {
        results->results.append(item);
    }
}
