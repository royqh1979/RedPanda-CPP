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

const PEditorCaret CaretList::NullCaret;

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
    for (int i=mList.size()-1;i>mIndex;i--) {
        removeCaret(i);
    }
    PEditorCaret caret = std::make_unique<EditorCaret>();
    caret->editor = editor;
    caret->line = line;
    caret->aChar = aChar;
    mList.push_back(std::move(caret));
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
    return mIndex<(int)mList.size()-1;
}

const PEditorCaret &CaretList::gotoAndGetPrevious()
{
    if (!hasPrevious())
        return NullCaret;
    mIndex--;
    //qDebug()<<"move previous:"<<mIndex<<":"<<mList.count();
    if (mIndex<(int)mList.size())
        return mList[mIndex];
    return NullCaret;
}

const PEditorCaret &CaretList::gotoAndGetNext()
{
    if (!hasNext())
        return NullCaret;
    mIndex++;
    //qDebug()<<"move next:"<<mIndex<<":"<<mList.count();
    if (mIndex>=0)
        return mList[mIndex];
    return NullCaret;
}

void CaretList::removeEditor(const Editor *editor)
{
    for (int i = mList.size()-1;i>=0;i--) {
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

void CaretList::onLinesDeleted(const Editor *editor, int firstLine, int count)
{
    //qDebug()<<"deleted:"<<mIndex<<":"<<mList.count();
    for (int i=mList.size()-1;i>=0;i--) {
        if (mList[i]->editor == editor
                && mList[i]->line>=firstLine) {
            if (mList[i]->line < (firstLine+count))
                removeCaret(i);
            else
                mList[i]->line-=count;
        }
    }
}

void CaretList::onLinesInserted(const Editor *editor, int firstLine, int count)
{
    //qDebug()<<"inserted:"<<mIndex<<":"<<mList.count();
    for(PEditorCaret& caret:mList) {
        if (caret->editor == editor
                && caret->line >= firstLine)
            caret->line+=count;
    }
}

void CaretList::onLinesMoved(const Editor *editor, int fromLine, int toLine)
{
    for(PEditorCaret& caret:mList) {
        if (caret->editor == editor) {
            if (caret->line == fromLine)
                caret->line = toLine;
            else if (fromLine < toLine ) {
                if (fromLine < caret->line && caret->line <= toLine)
                    --caret->line;
            } else if (toLine < fromLine) {
                if (toLine <= caret->line && caret->line <= fromLine)
                    caret->line++;
            }
        }
    }
}

void CaretList::removeCaret(int index)
{
    if (index<0 || index>=(int)mList.size())
        return;
    mList.erase(mList.begin()+index);
    if (mIndex>=index)
        mIndex--;
}
