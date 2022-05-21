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
#include "customdisablediconengine.h"

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

CustomDisabledIconEngine::CustomDisabledIconEngine()
{

}

void CustomDisabledIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State /*state*/)
{
    painter->save();
    painter->setClipRect(rect);
    QRect newRect = rect;
    QPixmap pixmap;
    if (mode == QIcon::Mode::Disabled)
        pixmap = mDisabledPixmap;
    else
        pixmap = mPixmap;
    if (pixmap.size().width() < rect.width()) {
        newRect.setLeft( rect.left()+(rect.width() - pixmap.size().width())/2);
        newRect.setWidth(pixmap.size().width());
    }
    if (pixmap.size().height() < rect.height()) {
        newRect.setTop( rect.top()+(rect.height() - pixmap.size().height())/2);
        newRect.setHeight(pixmap.size().height());
    }
    painter->drawPixmap(newRect,pixmap);
    painter->restore();
}

QIconEngine *CustomDisabledIconEngine::clone() const
{
    CustomDisabledIconEngine* eng = new CustomDisabledIconEngine();
    eng->mPixmap = mPixmap;
    eng->mDisabledPixmap = mDisabledPixmap;
    return eng;
}

QPixmap CustomDisabledIconEngine::pixmap(const QSize &/*size*/, QIcon::Mode mode, QIcon::State /*state*/)
{
    if (mode == QIcon::Mode::Disabled)
        return mDisabledPixmap;
    else
        return mPixmap;
}

void CustomDisabledIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{
    setPixmap(pixmap);
}

void CustomDisabledIconEngine::addFile(const QString &fileName, const QSize &/*size*/, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{
    setPixmap(QPixmap(fileName));
}

void CustomDisabledIconEngine::setPixmap(const QPixmap &pixmap)
{
    mPixmap = pixmap;
    if (pixmap.isNull())
        mDisabledPixmap = pixmap;
    else {
        QImage oldImage = mPixmap.toImage();
        QImage image(mPixmap.size(), QImage::Format_ARGB32);
        for (int x=0;x<image.width();x++) {
            for (int y=0;y<image.height();y++) {
                QColor c = oldImage.pixelColor(x,y);
                int gray = 0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue();
                QColor c2(gray,gray,gray,c.alpha());
                c2 = c2.darker();
                image.setPixelColor(x,y,c2);
            }
        }
        mDisabledPixmap = QPixmap::fromImage(image);
        mDisabledPixmap.setDevicePixelRatio(mPixmap.devicePixelRatioF());
    }
}
