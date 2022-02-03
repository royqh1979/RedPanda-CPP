#ifndef CUSTOMDISABLEDICONENGINE_H
#define CUSTOMDISABLEDICONENGINE_H

#include <QIconEngine>

class CustomDisabledIconEngine : public QIconEngine
{
public:
    CustomDisabledIconEngine();

    // QIconEngine interface
public:
    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine *clone() const override;
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override;
    void addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state) override;
    void addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state) override;

private:
    void setPixmap(const QPixmap& pixmap);
private:
    QPixmap mPixmap;
    QPixmap mDisabledPixmap;
};

#endif // CUSTOMDISABLEDICONENGINE_H
