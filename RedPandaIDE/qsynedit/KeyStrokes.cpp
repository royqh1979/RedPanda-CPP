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
    if (keySequence.isEmpty()<=0)
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

PSynEditKeyStroke SynEditKeyStrokes::add(int key, Qt::KeyboardModifiers modifiers, SynEditorCommand command)
{
    PSynEditKeyStroke keyStroke = std::make_shared<SynEditKeyStroke>();
    keyStroke->setKey(key);
    keyStroke->setKeyModifiers(modifiers);
    keyStroke->setCommand(command);
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
