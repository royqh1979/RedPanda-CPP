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
#include "shrinkabletabwidget.h"
#include <QTabBar>
#include <QDebug>
#include <QResizeEvent>

QHash<const ShrinkableTabWidget*,QSize> ShrinkableTabWidget::BeforeShrinkSizes;

ShrinkableTabWidget::ShrinkableTabWidget(QWidget *parent):QTabWidget(parent),
    mShrinked(false)
{

}

void ShrinkableTabWidget::setShrinkedFlag(bool shrinked)
{
    mShrinked = shrinked;
}

void ShrinkableTabWidget::setShrinked(bool shrinked)
{
    if (!mShrinked && shrinked) {
        setBeforeShrinkSize(size());
    }
    mShrinked = shrinked;
    switch(this->tabPosition()) {
    case QTabWidget::East:
    case QTabWidget::West:
        if (mShrinked) {
            this->setFixedWidth(tabBar()->width());
        } else {
            this->setMaximumWidth(QWIDGETSIZE_MAX);
        }
        break;
    case QTabWidget::North:
    case QTabWidget::South:
        if (mShrinked) {
            this->setFixedHeight(tabBar()->height());
        } else {
            this->setMaximumHeight(QWIDGETSIZE_MAX);
        }
    }
}

bool ShrinkableTabWidget::isShrinked() const
{
    return mShrinked;
}

void ShrinkableTabWidget::toggleShrined()
{
    setShrinked(!mShrinked);
}

void ShrinkableTabWidget::setBeforeShrinkSize(const QSize &size)
{
    BeforeShrinkSizes.insert(this,size);
}

QSize ShrinkableTabWidget::beforeShrinkSize()
{
    QSize size = BeforeShrinkSizes.value(this,QSize());
    if (!size.isValid() || size.isNull()) {
        size = QTabWidget::size();
    }
    return size;
}

QSize ShrinkableTabWidget::currentSize()
{
    if (isShrinked())
        return beforeShrinkSize();
    else
        return size();
}

int ShrinkableTabWidget::beforeShrinkWidthOrHeight()
{
    if (shrinkOrientation()==Qt::Vertical)
        return beforeShrinkSize().height();
    else
        return beforeShrinkSize().width();
}

Qt::Orientation ShrinkableTabWidget::shrinkOrientation()
{
    switch(this->tabPosition()) {
    case QTabWidget::East:
    case QTabWidget::West:
        return Qt::Horizontal;
    default:
        return Qt::Vertical;
    }
}

QSize ShrinkableTabWidget::sizeHint() const
{
    return QTabWidget::sizeHint();
}

QSize ShrinkableTabWidget::minimumSizeHint() const
{
    QSize size=QTabWidget::minimumSizeHint();
    switch(this->tabPosition()) {
    case QTabWidget::East:
    case QTabWidget::West:
        if (isShrinked())
            size.setWidth(tabBar()->width());
        else
            size.setWidth(tabBar()->width()*2);
        break;
    case QTabWidget::North:
    case QTabWidget::South:
        if (isShrinked())
            size.setHeight(tabBar()->height());
        else
            size.setHeight(tabBar()->height()*2);
    }
    return size;
}

void ShrinkableTabWidget::resizeEvent(QResizeEvent *event)
{
    QTabWidget::resizeEvent(event);
    if (!isVisible())
        return;
    if (!isShrinked())
        setBeforeShrinkSize(event->size());
}
