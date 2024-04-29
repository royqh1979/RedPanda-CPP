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
#include "keystrokes.h"
#include "miscprocs.h"

namespace QSynedit {
EditKeyStroke::EditKeyStroke()
{
    mKey = 0;
    mKeyModifiers = Qt::NoModifier;
    mKey2 = 0;
    mKeyModifiers2 = Qt::NoModifier;
    mCommand = EditCommand::None;
}

QKeySequence EditKeyStroke::keySequence() const
{
    if (mKey2 == 0) {
        return QKeySequence(mKey | mKeyModifiers);
    } else {
        return QKeySequence(mKey | mKeyModifiers, mKey2 | mKeyModifiers2);
    }
}

void EditKeyStroke::setKeySequence(QKeySequence &keySequence)
{
    if (keySequence.isEmpty())
        return;
    decodeKey(keySequence[0],mKey,mKeyModifiers);
    if (keySequence.count()>1) {
        decodeKey(keySequence[1],mKey2,mKeyModifiers2);
    } else {
        mKey2=0;
        mKeyModifiers2=Qt::NoModifier;
    }
}

int EditKeyStroke::key() const
{
    return mKey;
}

void EditKeyStroke::setKey(int key)
{
    mKey = key;
}

Qt::KeyboardModifiers EditKeyStroke::keyModifiers() const
{
    return mKeyModifiers;
}

void EditKeyStroke::setKeyModifiers(const Qt::KeyboardModifiers &keyModifiers)
{
    mKeyModifiers = keyModifiers;
}

int EditKeyStroke::key2() const
{
    return mKey2;
}

void EditKeyStroke::setKey2(int key2)
{
    mKey2 = key2;
}

Qt::KeyboardModifiers EditKeyStroke::keyModifiers2() const
{
    return mKeyModifiers2;
}

void EditKeyStroke::setKeyModifiers2(const Qt::KeyboardModifiers &keyModifiers2)
{
    mKeyModifiers2 = keyModifiers2;
}

EditCommand EditKeyStroke::command() const
{
    return mCommand;
}

void EditKeyStroke::setCommand(const EditCommand &command)
{
    mCommand = command;
}

EditKeyStrokes::EditKeyStrokes()
{

}

PEditKeyStroke EditKeyStrokes::add(EditCommand command, int key, Qt::KeyboardModifiers modifiers)
{
    PEditKeyStroke keyStroke = std::make_shared<EditKeyStroke>();
    keyStroke->setKey(key);
    keyStroke->setKeyModifiers(modifiers);
    keyStroke->setCommand(command);
    mList.append(keyStroke);
    return keyStroke;
}

PEditKeyStroke EditKeyStrokes::findCommand(EditCommand command)
{
    for (PEditKeyStroke& keyStroke:mList) {
        if (keyStroke->command() == command)
            return keyStroke;
    }
    return PEditKeyStroke();
}

PEditKeyStroke EditKeyStrokes::findKeycode(int key, Qt::KeyboardModifiers modifiers)
{
    for (PEditKeyStroke& keyStroke:mList) {
        if (keyStroke->key() == key
                && keyStroke->keyModifiers()  == (modifiers & ~ Qt::KeypadModifier)
                && keyStroke->key2()==0)
            return keyStroke;
    }
    return PEditKeyStroke();
}

PEditKeyStroke EditKeyStrokes::findKeycode2(int key, Qt::KeyboardModifiers modifiers,
                                                  int key2, Qt::KeyboardModifiers modifiers2)
{
    for (PEditKeyStroke& keyStroke:mList) {
        if (keyStroke->key() == key
                && keyStroke->keyModifiers()==(modifiers & ~ Qt::KeypadModifier)
                && keyStroke->key2()==key2
                && keyStroke->keyModifiers2()== (modifiers2 & ~ Qt::KeypadModifier))
            return keyStroke;
    }
    return PEditKeyStroke();
}

PEditKeyStroke EditKeyStrokes::findKeySequence(const QKeySequence &keySeq)
{
    switch (keySeq.count()) {
    case 1: {
            int key;
            Qt::KeyboardModifiers modifiers;
            decodeKey(keySeq[0],key,modifiers);
            return findKeycode(key,modifiers);
        }
    case 2:
    case 3:
    case 4:
        {
            int key;
            Qt::KeyboardModifiers modifiers;
            int key2;
            Qt::KeyboardModifiers modifiers2;
            decodeKey(keySeq[0],key,modifiers);
            decodeKey(keySeq[1],key2,modifiers2);
            return findKeycode2(key,modifiers,key2,modifiers2);
        }
    default:
        return PEditKeyStroke();
    }
}

void EditKeyStrokes::clear()
{
    return mList.clear();
}

void EditKeyStrokes::resetDefaults()
{
    clear();
    add(EditCommand::Up, Qt::Key_Up, Qt::NoModifier);
    add(EditCommand::SelUp, Qt::Key_Up, Qt::ShiftModifier);
    add(EditCommand::SelUp, Qt::Key_Up, Qt::KeyboardModifiers(Qt::ShiftModifier | Qt::AltModifier));
    add(EditCommand::ScrollUp, Qt::Key_Up, Qt::ControlModifier);
    add(EditCommand::Down, Qt::Key_Down, Qt::NoModifier);
    add(EditCommand::SelDown, Qt::Key_Down, Qt::ShiftModifier);
    add(EditCommand::SelDown, Qt::Key_Down, Qt::KeyboardModifiers(Qt::ShiftModifier | Qt::AltModifier));
    add(EditCommand::ScrollDown, Qt::Key_Down, Qt::ControlModifier);
    add(EditCommand::Left, Qt::Key_Left, Qt::NoModifier);
    add(EditCommand::SelLeft, Qt::Key_Left, Qt::ShiftModifier);
    add(EditCommand::SelLeft, Qt::Key_Left, Qt::KeyboardModifiers(Qt::ShiftModifier | Qt::AltModifier));
    add(EditCommand::WordLeft, Qt::Key_Left, Qt::ControlModifier);
    add(EditCommand::SelWordLeft, Qt::Key_Left, Qt::KeyboardModifiers(Qt::ShiftModifier|Qt::ControlModifier));
    add(EditCommand::Right, Qt::Key_Right, Qt::NoModifier);
    add(EditCommand::SelRight, Qt::Key_Right, Qt::ShiftModifier);
    add(EditCommand::SelRight, Qt::Key_Right, Qt::KeyboardModifiers(Qt::ShiftModifier | Qt::AltModifier));
    add(EditCommand::WordRight, Qt::Key_Right, Qt::ControlModifier);
    add(EditCommand::SelWordRight, Qt::Key_Right, Qt::KeyboardModifiers(Qt::ShiftModifier|Qt::ControlModifier));

    add(EditCommand::BlockStart, Qt::Key_Up, Qt::KeyboardModifiers(Qt::MetaModifier|Qt::ControlModifier));
    add(EditCommand::SelBlockStart, Qt::Key_Up, Qt::KeyboardModifiers(Qt::ShiftModifier|Qt::ControlModifier|Qt::MetaModifier));
    add(EditCommand::BlockEnd, Qt::Key_Down, Qt::KeyboardModifiers(Qt::MetaModifier|Qt::ControlModifier));
    add(EditCommand::SelBlockEnd, Qt::Key_Down, Qt::KeyboardModifiers(Qt::ShiftModifier|Qt::ControlModifier|Qt::MetaModifier));

    add(EditCommand::PageDown, Qt::Key_PageDown, Qt::NoModifier);
    add(EditCommand::SelPageDown, Qt::Key_PageDown, Qt::ShiftModifier);
    add(EditCommand::PageBottom, Qt::Key_PageDown, Qt::ControlModifier);
    add(EditCommand::SelPageBottom, Qt::Key_PageDown, Qt::KeyboardModifiers(Qt::ShiftModifier|Qt::ControlModifier));
    add(EditCommand::PageUp, Qt::Key_PageUp, Qt::NoModifier);
    add(EditCommand::SelPageUp, Qt::Key_PageUp, Qt::ShiftModifier);
    add(EditCommand::PageTop, Qt::Key_PageUp, Qt::ControlModifier);
    add(EditCommand::SelPageTop, Qt::Key_PageUp, Qt::KeyboardModifiers(Qt::ShiftModifier|Qt::ControlModifier));
    add(EditCommand::LineStart, Qt::Key_Home, Qt::NoModifier);
    add(EditCommand::SelLineStart, Qt::Key_Home, Qt::ShiftModifier);
    add(EditCommand::FileStart, Qt::Key_Home, Qt::ControlModifier);
    add(EditCommand::SelFileStart, Qt::Key_Home, Qt::KeyboardModifiers(Qt::ShiftModifier|Qt::ControlModifier));
    add(EditCommand::LineEnd, Qt::Key_End, Qt::NoModifier);
    add(EditCommand::SelLineEnd, Qt::Key_End, Qt::ShiftModifier);
    add(EditCommand::FileEnd, Qt::Key_End, Qt::ControlModifier);
    add(EditCommand::SelFileEnd, Qt::Key_End, Qt::KeyboardModifiers(Qt::ShiftModifier|Qt::ControlModifier));
    add(EditCommand::ToggleMode, Qt::Key_Insert, Qt::NoModifier);
    add(EditCommand::DeleteChar, Qt::Key_Delete, Qt::NoModifier);
    add(EditCommand::DeleteLastChar, Qt::Key_Backspace, Qt::NoModifier);
    add(EditCommand::LineBreak, Qt::Key_Return, Qt::NoModifier);
    add(EditCommand::LineBreak, Qt::Key_Return, Qt::ShiftModifier);
    add(EditCommand::LineBreakAtEnd, Qt::Key_Return, Qt::ControlModifier);
    add(EditCommand::LineBreak, Qt::Key_Enter, Qt::NoModifier);
    add(EditCommand::LineBreak, Qt::Key_Enter, Qt::ShiftModifier);
    add(EditCommand::LineBreakAtEnd, Qt::Key_Enter, Qt::ControlModifier);

}

void EditKeyStrokes::setExtraKeyStrokes()
{
    add(EditCommand::DeleteWordStart, Qt::Key_Backspace, Qt::ControlModifier);
    add(EditCommand::DeleteWordEnd, Qt::Key_Delete, Qt::ControlModifier);

    add(EditCommand::DuplicateLine, Qt::Key_D, Qt::ControlModifier);
    add(EditCommand::DeleteLine, Qt::Key_E, Qt::ControlModifier);

    add(EditCommand::SelectAll, Qt::Key_A, Qt::ControlModifier);
    add(EditCommand::Copy, Qt::Key_C, Qt::ControlModifier);
    add(EditCommand::Paste, Qt::Key_V, Qt::ControlModifier);
    add(EditCommand::Cut, Qt::Key_X, Qt::ControlModifier);

    add(EditCommand::Undo, Qt::Key_Z, Qt::ControlModifier);
    add(EditCommand::Redo, Qt::Key_Y, Qt::ControlModifier);
}

}
