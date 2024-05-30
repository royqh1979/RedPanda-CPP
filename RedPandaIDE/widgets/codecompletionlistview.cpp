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
#include "codecompletionlistview.h"
#include "../mainwindow.h"
#include "../editor.h"
#include "../editorlist.h"
#include <QDebug>

CodeCompletionListView::CodeCompletionListView(QWidget *parent) : QListView(parent)
{
    setUniformItemSizes(true);
}

void CodeCompletionListView::keyPressEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_Up
            || event->key() == Qt::Key_Down
            || event->key() == Qt::Key_PageDown
            || event->key() == Qt::Key_PageUp
            || event->key() == Qt::Key_Home
            || event->key() == Qt::Key_End
            || event->key() == Qt::Key_CapsLock
            ) {
        QListView::keyPressEvent(event);
        return;
    }
    if (!mKeypressedCallback || !mKeypressedCallback(event)) {
        QListView::keyPressEvent(event);
    }
}

void CodeCompletionListView::focusInEvent(QFocusEvent *)
{
    Editor *editor = pMainWindow->editorList()->getEditor();
    if (editor) {
        editor->showCaret();
    }
}

void CodeCompletionListView::mouseDoubleClickEvent(QMouseEvent */*event*/)
{
    QKeyEvent keyEvent(QKeyEvent::Type::KeyPress,Qt::Key_Tab,Qt::KeyboardModifier::NoModifier,
                    "\t");
    keyPressEvent(&keyEvent);
}

const KeyPressedCallback &CodeCompletionListView::keypressedCallback() const
{
    return mKeypressedCallback;
}

void CodeCompletionListView::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mKeypressedCallback = newKeypressedCallback;
}
