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
#include "KeyStrokes.h"
#include "MiscProcs.h"

namespace QSynedit {
EditKeyStroke::EditKeyStroke()
{
    mKey = 0;
    mKeyModifiers = Qt::NoModifier;
    mKey2 = 0;
    mKeyModifiers2 = Qt::NoModifier;
    mCommand = EditCommand::ecNone;
}

QKeySequence EditKeyStroke::keySequence() const
{
    if (mKey2 == 0) {
        return QKeySequence(mKey + mKeyModifiers);
    } else {
        return QKeySequence(mKey + mKeyModifiers, mKey2+mKeyModifiers2);
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

KeyError::KeyError(const QString &reason):BaseError(reason)
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
    add(EditCommand::ecUp, Qt::Key_Up, Qt::NoModifier);
    add(EditCommand::ecSelUp, Qt::Key_Up, Qt::ShiftModifier);
    add(EditCommand::ecSelUp, Qt::Key_Up, Qt::ShiftModifier | Qt::AltModifier);
    add(EditCommand::ecScrollUp, Qt::Key_Up, Qt::ControlModifier);
    add(EditCommand::ecDown, Qt::Key_Down, Qt::NoModifier);
    add(EditCommand::ecSelDown, Qt::Key_Down, Qt::ShiftModifier);
    add(EditCommand::ecSelDown, Qt::Key_Down, Qt::ShiftModifier | Qt::AltModifier);
    add(EditCommand::ecScrollDown, Qt::Key_Down, Qt::ControlModifier);
    add(EditCommand::ecLeft, Qt::Key_Left, Qt::NoModifier);
    add(EditCommand::ecSelLeft, Qt::Key_Left, Qt::ShiftModifier);
    add(EditCommand::ecWordLeft, Qt::Key_Left, Qt::ControlModifier);
    add(EditCommand::ecSelWordLeft, Qt::Key_Left, Qt::ShiftModifier|Qt::ControlModifier);
    add(EditCommand::ecRight, Qt::Key_Right, Qt::NoModifier);
    add(EditCommand::ecSelRight, Qt::Key_Right, Qt::ShiftModifier);
    add(EditCommand::ecWordRight, Qt::Key_Right, Qt::ControlModifier);
    add(EditCommand::ecSelWordRight, Qt::Key_Right, Qt::ShiftModifier|Qt::ControlModifier);

    add(EditCommand::ecBlockStart, Qt::Key_Up, Qt::MetaModifier|Qt::ControlModifier);
    add(EditCommand::ecSelBlockStart, Qt::Key_Up, Qt::ShiftModifier|Qt::ControlModifier|Qt::MetaModifier);
    add(EditCommand::ecBlockEnd, Qt::Key_Down, Qt::MetaModifier|Qt::ControlModifier);
    add(EditCommand::ecSelBlockEnd, Qt::Key_Down, Qt::ShiftModifier|Qt::ControlModifier|Qt::MetaModifier);

//    add(SynEditorCommand::ecExpandSelection, Qt::Key_Right, Qt::ShiftModifier|Qt::AltModifier);
//    add(SynEditorCommand::ecShrinkSelection, Qt::Key_Left, Qt::ShiftModifier | Qt::AltModifier);

    add(EditCommand::ecPageDown, Qt::Key_PageDown, Qt::NoModifier);
    add(EditCommand::ecSelPageDown, Qt::Key_PageDown, Qt::ShiftModifier);
    add(EditCommand::ecPageBottom, Qt::Key_PageDown, Qt::ControlModifier);
    add(EditCommand::ecSelPageBottom, Qt::Key_PageDown, Qt::ShiftModifier|Qt::ControlModifier);
    add(EditCommand::ecPageUp, Qt::Key_PageUp, Qt::NoModifier);
    add(EditCommand::ecSelPageUp, Qt::Key_PageUp, Qt::ShiftModifier);
    add(EditCommand::ecPageTop, Qt::Key_PageUp, Qt::ControlModifier);
    add(EditCommand::ecSelPageTop, Qt::Key_PageUp, Qt::ShiftModifier|Qt::ControlModifier);
    add(EditCommand::ecLineStart, Qt::Key_Home, Qt::NoModifier);
    add(EditCommand::ecSelLineStart, Qt::Key_Home, Qt::ShiftModifier);
    add(EditCommand::ecEditorStart, Qt::Key_Home, Qt::ControlModifier);
    add(EditCommand::ecSelEditorStart, Qt::Key_Home, Qt::ShiftModifier|Qt::ControlModifier);
    add(EditCommand::ecLineEnd, Qt::Key_End, Qt::NoModifier);
    add(EditCommand::ecSelLineEnd, Qt::Key_End, Qt::ShiftModifier);
    add(EditCommand::ecEditorEnd, Qt::Key_End, Qt::ControlModifier);
    add(EditCommand::ecSelEditorEnd, Qt::Key_End, Qt::ShiftModifier|Qt::ControlModifier);
    add(EditCommand::ecToggleMode, Qt::Key_Insert, Qt::NoModifier);
//    add(SynEditorCommand::ecCopy, Qt::Key_Insert, Qt::ControlModifier);
//    add(SynEditorCommand::ecCut, Qt::Key_Delete, Qt::ShiftModifier);
//    add(SynEditorCommand::ecPaste, Qt::Key_Insert, Qt::ShiftModifier);
    add(EditCommand::ecDeleteChar, Qt::Key_Delete, Qt::NoModifier);
    add(EditCommand::ecDeleteLastChar, Qt::Key_Backspace, Qt::NoModifier);
//    add(SynEditorCommand::ecDeleteLastChar, Qt::Key_Backspace, Qt::ShiftModifier);
//    add(SynEditorCommand::ecDeleteWordStart, Qt::Key_Backspace, Qt::ControlModifier);
//    add(SynEditorCommand::ecDeleteWordEnd, Qt::Key_Delete, Qt::ControlModifier);
//    add(SynEditorCommand::ecUndo, Qt::Key_Backspace, Qt::AltModifier);
//    add(SynEditorCommand::ecRedo, Qt::Key_Backspace, Qt::AltModifier|Qt::ShiftModifier);
    add(EditCommand::ecLineBreak, Qt::Key_Return, Qt::NoModifier);
    add(EditCommand::ecLineBreak, Qt::Key_Return, Qt::ShiftModifier);
    add(EditCommand::ecLineBreakAtEnd, Qt::Key_Return, Qt::ControlModifier);
    add(EditCommand::ecLineBreak, Qt::Key_Enter, Qt::NoModifier);
    add(EditCommand::ecLineBreak, Qt::Key_Enter, Qt::ShiftModifier);
    add(EditCommand::ecLineBreakAtEnd, Qt::Key_Enter, Qt::ControlModifier);
//    add(SynEditorCommand::ecTab, Qt::Key_Tab, Qt::NoModifier);
//    add(SynEditorCommand::ecShiftTab, Qt::Key_Backtab, Qt::ShiftModifier);
//    add(SynEditorCommand::ecShiftTab, Qt::Key_Tab, Qt::ShiftModifier);
    add(EditCommand::ecContextHelp, Qt::Key_F1, Qt::NoModifier);

//    add(SynEditorCommand::ecSelectAll, Qt::Key_A, Qt::ControlModifier);
//    add(SynEditorCommand::ecCopy, Qt::Key_C, Qt::ControlModifier);
//    add(SynEditorCommand::ecPaste, Qt::Key_V, Qt::ControlModifier);
//    add(SynEditorCommand::ecCut, Qt::Key_X, Qt::ControlModifier);
//    add(SynEditorCommand::ecBlockIndent, Qt::Key_I, Qt::ControlModifier|Qt::ShiftModifier);
//    add(SynEditorCommand::ecBlockUnindent, Qt::Key_U, Qt::ControlModifier|Qt::ShiftModifier);
//    add(SynEditorCommand::ecLineBreak, Qt::Key_M, Qt::ControlModifier);
//    add(SynEditorCommand::ecInsertLine, Qt::Key_N, Qt::ControlModifier);
//    add(SynEditorCommand::ecDeleteWord, Qt::Key_T, Qt::ControlModifier);
//    add(SynEditorCommand::ecDeleteLine, Qt::Key_Y, Qt::ControlModifier);
//    add(SynEditorCommand::ecDeleteEOL, Qt::Key_Y, Qt::ControlModifier|Qt::ShiftModifier);
//    add(SynEditorCommand::ecDuplicateLine, Qt::Key_D, Qt::ControlModifier);

//    add(SynEditorCommand::ecUndo, Qt::Key_Z, Qt::ControlModifier);
//    add(SynEditorCommand::ecRedo, Qt::Key_Z, Qt::ControlModifier|Qt::ShiftModifier);
//    add(SynEditorCommand::ecNormalSelect, Qt::Key_N, Qt::ControlModifier | Qt::ShiftModifier);
//    add(SynEditorCommand::ecColumnSelect, Qt::Key_C, Qt::ControlModifier | Qt::ShiftModifier);
//    add(SynEditorCommand::ecLineSelect, Qt::Key_L, Qt::ControlModifier | Qt::ShiftModifier);
    //    add(SynEditorCommand::ecMatchBracket, Qt::Key_B, Qt::ControlModifier | Qt::ShiftModifier);
}

void EditKeyStrokes::setExtraKeyStrokes()
{
    add(EditCommand::ecDeleteWordStart, Qt::Key_Backspace, Qt::ControlModifier);
    add(EditCommand::ecDeleteWordEnd, Qt::Key_Delete, Qt::ControlModifier);

    add(EditCommand::ecDuplicateLine, Qt::Key_D, Qt::ControlModifier);
    add(EditCommand::ecDeleteLine, Qt::Key_E, Qt::ControlModifier);

    add(EditCommand::ecSelectAll, Qt::Key_A, Qt::ControlModifier);
    add(EditCommand::ecCopy, Qt::Key_C, Qt::ControlModifier);
    add(EditCommand::ecPaste, Qt::Key_V, Qt::ControlModifier);
    add(EditCommand::ecCut, Qt::Key_X, Qt::ControlModifier);

    add(EditCommand::ecUndo, Qt::Key_Z, Qt::ControlModifier);
    add(EditCommand::ecRedo, Qt::Key_Y, Qt::ControlModifier);
}

}
