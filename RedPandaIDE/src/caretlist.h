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
#ifndef CARETLIST_H
#define CARETLIST_H
#include <QString>
#include <memory>
#include <QVector>
#include <QObject>

class Editor;

struct EditorCaret{
    Editor* editor;
    int line;
    int aChar;
};
using PEditorCaret = std::unique_ptr<EditorCaret>;

class CaretList:public QObject
{
    Q_OBJECT
public:
    explicit CaretList(QObject* parent = nullptr);
public:
    void addCaret(Editor *editor, int line, int aChar);
    bool hasPrevious() const;
    bool hasNext() const;
    const PEditorCaret &gotoAndGetPrevious();
    const PEditorCaret &gotoAndGetNext();
    void removeEditor(const Editor* editor);
    void reset();
    void pause();
    void unPause();
public slots:
    void onLinesDeleted(const Editor* editor, int firstLine, int count);
    void onLinesInserted(const Editor* editor, int firstLine, int count);
    void onLinesMoved(const Editor* editor, int fromLine, int toLine);
private:
    void removeCaret(int index);
private:
    std::vector<PEditorCaret> mList;
    int mIndex;
    bool mPauseAdd;
    static const PEditorCaret NullCaret;
};

#endif // CARETLIST_H
