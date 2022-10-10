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
#include "shortcutinputedit.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QDebug>
#include <QAction>

ShortcutInputEdit::ShortcutInputEdit(QWidget* parent):QLineEdit(parent)
{
    QList<QAction *> acts = actions();
//    foreach (const QAction* action, acts) {
//        qDebug()<<action->shortcut()[0];
//    }
}

void ShortcutInputEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key()==Qt::Key_Delete && event->modifiers()==Qt::NoModifier) {
        setText("");
    } else if (event->key()==Qt::Key_Backspace && event->modifiers()==Qt::NoModifier) {
        setText("");
    } else if (event->key()==Qt::Key_Control ||
            event->key()==Qt::Key_Alt ||
            event->key()==Qt::Key_Shift ||
            event->key()==Qt::Key_Meta
            ) {
        //setText("");
        return;
    } else if (event->modifiers()==Qt::ShiftModifier
               && !event->text().isEmpty()
               && event->text().at(0).unicode()>32
               && event->text().at(0).unicode()<127){
        //setText("");
        return;
    } else if (event->modifiers()==Qt::NoModifier
               && !event->text().isEmpty()
               && event->text().at(0).unicode()>32
               && event->text().at(0).unicode()<127){
        //setText("");
        return;
    } else {
        int key = event->key();
        if (key==Qt::Key_Backtab)
            key = Qt::Key_Tab;
        QKeySequence seq(event->modifiers()|key);
        QString s=seq.toString();
        if (event->modifiers().testFlag(Qt::ShiftModifier)
                && !event->text().isEmpty()
                && event->text().at(0).unicode()>32
                && event->text().at(0).unicode()<127) {
            s = s.mid(0,s.lastIndexOf('+')+1) + event->text().at(0);
        }
        setText(s);
        if (key!=Qt::Key_Tab
                && key!=Qt::Key_Enter
                && key!=Qt::Key_Return)
            emit inputFinished(this);
    }
    event->accept();
}

bool ShortcutInputEdit::event(QEvent *event)
{
    if (event->type()==QEvent::ShortcutOverride) {
        keyPressEvent((QKeyEvent*)event);
        event->accept();
        return true;
    } else if (event->type()==QEvent::KeyPress) {
        keyPressEvent((QKeyEvent*)event);
        return true;
    }
    return QLineEdit::event(event);
}
