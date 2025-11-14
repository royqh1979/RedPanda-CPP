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
#ifndef CPPREFACTER_H
#define CPPREFACTER_H

#include <QObject>
#include "parser/parserutils.h"
#include "widgets/searchresultview.h"
#include "parser/cppparser.h"

class Editor;
namespace QSynedit {
    struct BufferCoord;
}
class Project;
class CppRefacter : public QObject
{
    Q_OBJECT
public:
    explicit CppRefacter(QObject *parent = nullptr);

    bool findOccurence(Editor * editor, const QSynedit::BufferCoord& pos);
    bool findOccurence(Editor * editor, const QString& statementFullname, SearchFileScope scope);

    void renameSymbol(Editor* editor, const QSynedit::BufferCoord& pos, const QString& newWord);
    void renameUndefinedLocalVariable(Editor* editor, const QSynedit::BufferCoord& pos, const QString& newWord);
private:
    void doFindOccurenceInEditor(const PStatement &statement, Editor* editor, const PCppParser& parser);
    void doFindOccurenceInProject(const PStatement &statement, std::shared_ptr<Project> project, const PCppParser& parser);
    PSearchResultTreeItem findOccurenceInFile(
            const QString& filename,
            const QByteArray& fileEncoding,
            const PStatement& statement,
            const PCppParser& parser);
    void renameSymbolInFile(
            const QString& filename,
            const PStatement& statement,
            const QString& newWord,
            const PCppParser& parser);
};

#endif // CPPREFACTER_H
