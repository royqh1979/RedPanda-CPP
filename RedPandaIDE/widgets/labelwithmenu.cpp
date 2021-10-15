#include "labelwithmenu.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QMouseEvent>

LabelWithMenu::LabelWithMenu(QWidget* parent):QLabel(parent)
{

}

void LabelWithMenu::mousePressEvent(QMouseEvent *event)
{
    QContextMenuEvent *e = new QContextMenuEvent(QContextMenuEvent::Reason::Mouse,
                                                 event->pos());
    QApplication::instance()->postEvent(this,e);
    event->accept();
}
