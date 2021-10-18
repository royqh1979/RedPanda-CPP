#ifndef DARKFUSIONSTYLE_H
#define DARKFUSIONSTYLE_H

#include <QProxyStyle>

class DarkFusionStyle : public QProxyStyle
{
    Q_OBJECT
public:
    DarkFusionStyle();

    // QStyle interface
public:
    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w) const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;
};

#endif // DARKFUSIONSTYLE_H
