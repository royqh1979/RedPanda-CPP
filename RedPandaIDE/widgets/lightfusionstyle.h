#ifndef LIGHTFUSIONSTYLE_H
#define LIGHTFUSIONSTYLE_H

#include <QProxyStyle>
#include <QObject>

class LightFusionStyle : public QProxyStyle
{
    Q_OBJECT
public:
    LightFusionStyle();
    // QStyle interface
public:
    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;

};

#endif // LIGHTFUSIONSTYLE_H
