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
signals:
private:
    PSearchResultTreeItem findOccurenceInFile(
            const QString& filename,
            const PStatement& statement,
            const PCppParser& parser);

};

#endif // CPPREFACTER_H
