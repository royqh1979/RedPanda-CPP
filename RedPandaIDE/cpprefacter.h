#ifndef CPPREFACTER_H
#define CPPREFACTER_H

#include <QObject>
#include "parser/parserutils.h"
#include "widgets/searchresultview.h"
#include "parser/cppparser.h"

class Editor;
class BufferCoord;
class CppRefacter : public QObject
{
    Q_OBJECT
public:
    explicit CppRefacter(QObject *parent = nullptr);

    bool findOccurence(Editor * editor, const BufferCoord& pos);

    void renameSymbol(Editor* editor, const BufferCoord& pos, const QString& word, const QString& newWord);
signals:
private:
    void findOccurenceInFile(
            const QString& phrase,
            const QString& filename,
            const PStatement& statement,
            int line,
            const PCppParser& parser);
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
