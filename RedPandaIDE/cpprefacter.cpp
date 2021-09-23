#include "cpprefacter.h"
#include "mainwindow.h"
#include "settings.h"
#include "editor.h"
#include "editorlist.h"
#include <QFile>
#include "HighlighterManager.h"

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

    PSearchResults results = pMainWindow->searchResultModel()->addSearchResults(
                phrase,
                editor->filename(),
                pos.Line
                );


    PSearchResultTreeItem item = findOccurenceInFile(
                editor->filename(),
                statement,
                editor->parser());
    if (item && !(item->results.isEmpty())) {
        results->results.append(item);
    }
    pMainWindow->searchResultModel()->notifySearchResultsUpdated();
    return true;
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
        QFile file(filename);
        editor.lines()->LoadFromFile(file,ENCODING_AUTO_DETECT,encoding);
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
