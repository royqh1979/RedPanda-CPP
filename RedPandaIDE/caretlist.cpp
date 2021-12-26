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
#include "caretlist.h"
#include <QDebug>

CaretList::CaretList(QObject* parent):
    QObject(parent),
    mIndex(-1),
    mPauseAdd(false)
{

}

void CaretList::addCaret(Editor *editor, int line, int aChar)
{
    if (mPauseAdd)
        return;
    for (int i=mList.count()-1;i>mIndex;i--) {
        removeCaret(i);
    }
    PEditorCaret caret = std::make_shared<EditorCaret>();
    caret->editor = editor;
    caret->line = line;
    caret->aChar = aChar;
    mList.append(caret);
    mIndex++;
    //qDebug()<<"add caret:"<<mIndex<<":"<<mList.count();
}

bool CaretList::hasPrevious() const
{
    //qDebug()<<"has previous:"<<mIndex<<":"<<mList.count();
    return mIndex>0;
}

bool CaretList::hasNext() const
{
    //qDebug()<<"has next:"<<mIndex<<":"<<mList.count();
    return mIndex<mList.count()-1;
}

PEditorCaret CaretList::gotoAndGetPrevious()
{
    if (!hasPrevious())
        return PEditorCaret();
    mIndex--;
    //qDebug()<<"move previous:"<<mIndex<<":"<<mList.count();
    if (mIndex<mList.count())
        return mList[mIndex];
    return PEditorCaret();
}

PEditorCaret CaretList::gotoAndGetNext()
{
    if (!hasNext())
        return PEditorCaret();
    mIndex++;
    //qDebug()<<"move next:"<<mIndex<<":"<<mList.count();
    if (mIndex>=0)
        return mList[mIndex];
    return PEditorCaret();
}

void CaretList::removeEditor(const Editor *editor)
{
    for (int i = mList.count()-1;i>=0;i--) {
        if (mList[i]->editor == editor)
            removeCaret(i);
    }
}

void CaretList::reset()
{
    mList.clear();
    mIndex = -1;
}

void CaretList::pause()
{
    mPauseAdd = true;
}

void CaretList::unPause()
{
    mPauseAdd = false;
}

void CaretList::linesDeleted(const Editor *editor, int firstLine, int count)
{
    //qDebug()<<"deleted:"<<mIndex<<":"<<mList.count();
    for (int i=mList.count()-1;i>=0;i--) {
        if (mList[i]->editor == editor
                && mList[i]->line>=firstLine) {
            if (mList[i]->line < (firstLine+count))
                removeCaret(i);
            else
                mList[i]->line-=count;
        }
    }
}

void CaretList::linesInserted(const Editor *editor, int firstLine, int count)
{
    //qDebug()<<"inserted:"<<mIndex<<":"<<mList.count();
    for(PEditorCaret& caret:mList) {
        if (caret->editor == editor
                && caret->line >= firstLine)
            caret->line+=count;
    }
}

void CaretList::removeCaret(int index)
{
    if (index<0 || index>=mList.count())
        return;
    mList.removeAt(index);
    if (mIndex>=index)
        mIndex--;
}
