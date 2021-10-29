#include "KeyStrokes.h"

SynEditKeyStroke::SynEditKeyStroke()
{
    mKey = 0;
    mKeyModifiers = Qt::NoModifier;
    mKey2 = 0;
    mKeyModifiers2 = Qt::NoModifier;
    mCommand = SynEditorCommand::ecNone;
}

QKeySequence SynEditKeyStroke::keySequence() const
{
    if (mKey2 == 0) {
        return QKeySequence(mKey + mKeyModifiers);
    } else {
        return QKeySequence(mKey + mKeyModifiers, mKey2+mKeyModifiers2);
    }
}

void SynEditKeyStroke::setKeySequence(QKeySequence &keySequence)
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

int SynEditKeyStroke::key() const
{
    return mKey;
}

void SynEditKeyStroke::setKey(int key)
{
    mKey = key;
}

Qt::KeyboardModifiers SynEditKeyStroke::keyModifiers() const
{
    return mKeyModifiers;
}

void SynEditKeyStroke::setKeyModifiers(const Qt::KeyboardModifiers &keyModifiers)
{
    mKeyModifiers = keyModifiers;
}

int SynEditKeyStroke::key2() const
{
    return mKey2;
}

void SynEditKeyStroke::setKey2(int key2)
{
    mKey2 = key2;
}

Qt::KeyboardModifiers SynEditKeyStroke::keyModifiers2() const
{
    return mKeyModifiers2;
}

void SynEditKeyStroke::setKeyModifiers2(const Qt::KeyboardModifiers &keyModifiers2)
{
    mKeyModifiers2 = keyModifiers2;
}

SynEditorCommand SynEditKeyStroke::command() const
{
    return mCommand;
}

void SynEditKeyStroke::setCommand(const SynEditorCommand &command)
{
    mCommand = command;
}

SynKeyError::SynKeyError(const QString &reason):BaseError(reason)
{

}

PSynEditKeyStroke SynEditKeyStrokes::add(SynEditorCommand command, int key, Qt::KeyboardModifiers modifiers)
{
    PSynEditKeyStroke keyStroke = std::make_shared<SynEditKeyStroke>();
    keyStroke->setKey(key);
    keyStroke->setKeyModifiers(modifiers);
    keyStroke->setCommand(command);
    mList.append(keyStroke);
    return keyStroke;
}

PSynEditKeyStroke SynEditKeyStrokes::findCommand(SynEditorCommand command)
{
    for (PSynEditKeyStroke& keyStroke:mList) {
        if (keyStroke->command() == command)
            return keyStroke;
    }
    return PSynEditKeyStroke();
}

PSynEditKeyStroke SynEditKeyStrokes::findKeycode(int key, Qt::KeyboardModifiers modifiers)
{
    for (PSynEditKeyStroke& keyStroke:mList) {
        if (keyStroke->key() == key && keyStroke->keyModifiers()==modifiers && keyStroke->key2()==0)
            return keyStroke;
    }
    return PSynEditKeyStroke();
}

PSynEditKeyStroke SynEditKeyStrokes::findKeycode2(int key, Qt::KeyboardModifiers modifiers,
                                                  int key2, Qt::KeyboardModifiers modifiers2)
{
    for (PSynEditKeyStroke& keyStroke:mList) {
        if (keyStroke->key() == key && keyStroke->keyModifiers()==modifiers && keyStroke->key2()==key2
                && keyStroke->keyModifiers2() ==modifiers2)
            return keyStroke;
    }
    return PSynEditKeyStroke();
}

PSynEditKeyStroke SynEditKeyStrokes::findKeySequence(const QKeySequence &keySeq)
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
        return PSynEditKeyStroke();
    }
}

void SynEditKeyStrokes::clear()
{
    return mList.clear();
}

void SynEditKeyStrokes::resetDefaults()
{
    clear();
    add(SynEditorCommand::ecUp, Qt::Key_Up, Qt::NoModifier);
    add(SynEditorCommand::ecSelUp, Qt::Key_Up, Qt::ShiftModifier);
    add(SynEditorCommand::ecScrollUp, Qt::Key_Up, Qt::ControlModifier);
    add(SynEditorCommand::ecDown, Qt::Key_Down, Qt::NoModifier);
    add(SynEditorCommand::ecSelDown, Qt::Key_Down, Qt::ShiftModifier);
    add(SynEditorCommand::ecScrollDown, Qt::Key_Down, Qt::ControlModifier);
    add(SynEditorCommand::ecLeft, Qt::Key_Left, Qt::NoModifier);
    add(SynEditorCommand::ecSelLeft, Qt::Key_Left, Qt::ShiftModifier);
    add(SynEditorCommand::ecWordLeft, Qt::Key_Left, Qt::ControlModifier);
    add(SynEditorCommand::ecSelWordLeft, Qt::Key_Left, Qt::ShiftModifier|Qt::ControlModifier);
    add(SynEditorCommand::ecRight, Qt::Key_Right, Qt::NoModifier);

    add(SynEditorCommand::ecSelRight, Qt::Key_Right, Qt::ShiftModifier);
    add(SynEditorCommand::ecWordRight, Qt::Key_Right, Qt::ControlModifier);
    add(SynEditorCommand::ecSelWordRight, Qt::Key_Right, Qt::ShiftModifier|Qt::ControlModifier);
    add(SynEditorCommand::ecPageDown, Qt::Key_PageDown, Qt::NoModifier);
    add(SynEditorCommand::ecSelPageDown, Qt::Key_PageDown, Qt::ShiftModifier);
    add(SynEditorCommand::ecPageBottom, Qt::Key_PageDown, Qt::ControlModifier);
    add(SynEditorCommand::ecSelPageBottom, Qt::Key_PageDown, Qt::ShiftModifier|Qt::ControlModifier);
    add(SynEditorCommand::ecPageUp, Qt::Key_PageUp, Qt::NoModifier);
    add(SynEditorCommand::ecSelPageUp, Qt::Key_PageUp, Qt::ShiftModifier);
    add(SynEditorCommand::ecPageTop, Qt::Key_PageUp, Qt::ControlModifier);
    add(SynEditorCommand::ecSelPageTop, Qt::Key_PageUp, Qt::ShiftModifier|Qt::ControlModifier);
    add(SynEditorCommand::ecLineStart, Qt::Key_Home, Qt::NoModifier);
    add(SynEditorCommand::ecSelLineStart, Qt::Key_Home, Qt::ShiftModifier);
    add(SynEditorCommand::ecEditorStart, Qt::Key_Home, Qt::ControlModifier);
    add(SynEditorCommand::ecSelEditorStart, Qt::Key_Home, Qt::ShiftModifier|Qt::ControlModifier);
    add(SynEditorCommand::ecLineEnd, Qt::Key_End, Qt::NoModifier);
    add(SynEditorCommand::ecSelLineEnd, Qt::Key_End, Qt::ShiftModifier);
    add(SynEditorCommand::ecEditorEnd, Qt::Key_End, Qt::ControlModifier);
    add(SynEditorCommand::ecSelEditorEnd, Qt::Key_End, Qt::ShiftModifier|Qt::ControlModifier);
    add(SynEditorCommand::ecToggleMode, Qt::Key_Insert, Qt::NoModifier);
    add(SynEditorCommand::ecCopy, Qt::Key_Insert, Qt::ControlModifier);
    add(SynEditorCommand::ecCut, Qt::Key_Delete, Qt::ShiftModifier);
    add(SynEditorCommand::ecPaste, Qt::Key_Insert, Qt::ShiftModifier);
    add(SynEditorCommand::ecDeleteChar, Qt::Key_Delete, Qt::NoModifier);
    add(SynEditorCommand::ecDeleteLastChar, Qt::Key_Backspace, Qt::NoModifier);
    add(SynEditorCommand::ecDeleteLastChar, Qt::Key_Backspace, Qt::ShiftModifier);
    add(SynEditorCommand::ecDeleteLastWord, Qt::Key_Backspace, Qt::ControlModifier);
    add(SynEditorCommand::ecUndo, Qt::Key_Backspace, Qt::AltModifier);
    add(SynEditorCommand::ecRedo, Qt::Key_Backspace, Qt::AltModifier|Qt::ShiftModifier);
    add(SynEditorCommand::ecLineBreak, Qt::Key_Return, Qt::NoModifier);
    add(SynEditorCommand::ecLineBreak, Qt::Key_Return, Qt::ShiftModifier);
    add(SynEditorCommand::ecLineBreak, Qt::Key_Enter, Qt::KeypadModifier);
    add(SynEditorCommand::ecLineBreak, Qt::Key_Enter, Qt::KeypadModifier|Qt::ShiftModifier);
    add(SynEditorCommand::ecTab, Qt::Key_Tab, Qt::NoModifier);
    add(SynEditorCommand::ecShiftTab, Qt::Key_Backtab, Qt::ShiftModifier);
    add(SynEditorCommand::ecShiftTab, Qt::Key_Tab, Qt::ShiftModifier);
    add(SynEditorCommand::ecContextHelp, Qt::Key_F1, Qt::NoModifier);

    add(SynEditorCommand::ecSelectAll, Qt::Key_A, Qt::ControlModifier);
    add(SynEditorCommand::ecCopy, Qt::Key_C, Qt::ControlModifier);
    add(SynEditorCommand::ecPaste, Qt::Key_V, Qt::ControlModifier);
    add(SynEditorCommand::ecCut, Qt::Key_X, Qt::ControlModifier);
    add(SynEditorCommand::ecBlockIndent, Qt::Key_I, Qt::ControlModifier|Qt::ShiftModifier);
    add(SynEditorCommand::ecBlockUnindent, Qt::Key_U, Qt::ControlModifier|Qt::ShiftModifier);
    add(SynEditorCommand::ecLineBreak, Qt::Key_M, Qt::ControlModifier);
    add(SynEditorCommand::ecInsertLine, Qt::Key_N, Qt::ControlModifier);
    add(SynEditorCommand::ecDeleteWord, Qt::Key_T, Qt::ControlModifier);
    add(SynEditorCommand::ecDeleteLine, Qt::Key_Y, Qt::ControlModifier);
    add(SynEditorCommand::ecDeleteEOL, Qt::Key_Y, Qt::ControlModifier|Qt::ShiftModifier);
    add(SynEditorCommand::ecUndo, Qt::Key_Z, Qt::ControlModifier);
    add(SynEditorCommand::ecRedo, Qt::Key_Z, Qt::ControlModifier|Qt::ShiftModifier);
    add(SynEditorCommand::ecGotoMarker0, Qt::Key_0, Qt::ControlModifier);
    add(SynEditorCommand::ecGotoMarker1, Qt::Key_1, Qt::ControlModifier);
    add(SynEditorCommand::ecGotoMarker2, Qt::Key_2, Qt::ControlModifier);
    add(SynEditorCommand::ecGotoMarker3, Qt::Key_3, Qt::ControlModifier);
    add(SynEditorCommand::ecGotoMarker4, Qt::Key_4, Qt::ControlModifier);
    add(SynEditorCommand::ecGotoMarker5, Qt::Key_5, Qt::ControlModifier);
    add(SynEditorCommand::ecGotoMarker6, Qt::Key_6, Qt::ControlModifier);
    add(SynEditorCommand::ecGotoMarker7, Qt::Key_7, Qt::ControlModifier);
    add(SynEditorCommand::ecGotoMarker8, Qt::Key_8, Qt::ControlModifier);
    add(SynEditorCommand::ecGotoMarker9, Qt::Key_9, Qt::ControlModifier);
    add(SynEditorCommand::ecSetMarker0, Qt::Key_0, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecSetMarker1, Qt::Key_1, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecSetMarker2, Qt::Key_2, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecSetMarker3, Qt::Key_3, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecSetMarker4, Qt::Key_4, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecSetMarker5, Qt::Key_5, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecSetMarker6, Qt::Key_6, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecSetMarker7, Qt::Key_7, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecSetMarker8, Qt::Key_8, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecSetMarker9, Qt::Key_9, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecNormalSelect, Qt::Key_N, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecColumnSelect, Qt::Key_C, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecLineSelect, Qt::Key_L, Qt::ControlModifier | Qt::ShiftModifier);
    add(SynEditorCommand::ecMatchBracket, Qt::Key_B, Qt::ControlModifier | Qt::ShiftModifier);
}
