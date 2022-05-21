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

ShrinkableTabWidget::ShrinkableTabWidget(QWidget *parent):QTabWidget(parent),
    mShrinked(false)
{

}

void ShrinkableTabWidget::setShrinked(bool shrinked)
{
    if (!mShrinked) {
        mBeforeShrinkSize = size();
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

void ShrinkableTabWidget::toggleShrined()
{
    setShrinked(!mShrinked);
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
        size.setWidth(tabBar()->width());
        break;
    case QTabWidget::North:
    case QTabWidget::South:
        size.setHeight(tabBar()->height());
    }
//    qDebug()<<"min size hint()"<<size;
//    qDebug()<<"mininum size"<<minimumSize();
    return size;
}
