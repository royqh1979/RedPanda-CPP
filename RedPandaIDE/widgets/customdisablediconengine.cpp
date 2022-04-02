#include "customdisablediconengine.h"

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

CustomDisabledIconEngine::CustomDisabledIconEngine()
{

}

void CustomDisabledIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
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

QPixmap CustomDisabledIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    if (mode == QIcon::Mode::Disabled)
        return mDisabledPixmap;
    else
        return mPixmap;
}

void CustomDisabledIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state)
{
    setPixmap(pixmap);
}

void CustomDisabledIconEngine::addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state)
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
