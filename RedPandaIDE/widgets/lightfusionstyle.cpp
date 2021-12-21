#include "lightfusionstyle.h"
#include "../settings.h"

LightFusionStyle::LightFusionStyle():QProxyStyle("fusion")
{

}

int LightFusionStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch ( metric ) {
    case QStyle::PM_SmallIconSize:
        return pointToPixel(pSettings->environment().interfaceFontSize());
    default:
        return QProxyStyle::pixelMetric( metric, option, widget );
    }
}
