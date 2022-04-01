#include "shortcutinputedit.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QDebug>
#include <QAction>

ShortcutInputEdit::ShortcutInputEdit(QWidget* parent):QLineEdit(parent)
{
    QList<QAction *> acts = actions();
    foreach (const QAction* action, acts) {
        qDebug()<<action->shortcut()[0];
    }
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
        QKeySequence seq(event->modifiers());
        setText("");
    } else {
        int key = event->key();
        if (key==Qt::Key_Backtab)
            key = Qt::Key_Tab;
        QKeySequence seq(event->modifiers()|key);
        setText(seq.toString());
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
