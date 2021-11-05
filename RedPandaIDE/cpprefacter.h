#ifndef CPPREFACTER_H
#define CPPREFACTER_H

#include <QObject>
#include "parser/parserutils.h"
#include "widgets/searchresultview.h"
#include "parser/cppparser.h"

class Editor;
class BufferCoord;
class Project;
class CppRefacter : public QObject
{
    Q_OBJECT
public:
    explicit CppRefacter(QObject *parent = nullptr);

    bool findOccurence(Editor * editor, const BufferCoord& pos);
    bool findOccurence(const QString& statementFullname, SearchFileScope scope);

    void renameSymbol(Editor* editor, const BufferCoord& pos, const QString& word, const QString& newWord);
signals:
private:
    void doFindOccurenceInEditor(PStatement statement, Editor* editor, const PCppParser& parser);
    void doFindOccurenceInProject(PStatement statement, std::shared_ptr<Project> project, const PCppParser& parser);
    PSearchResultTreeItem findOccurenceInFile(
            const QString& filename,
            const PStatement& statement,
            const PCppParser& parser);
    void renameSymbolInFile(
            const QString& filename,
            const PStatement& statement,
            const QString& newWord,
            const PCppParser& parser);
};

#endif // CPPREFACTER_H
