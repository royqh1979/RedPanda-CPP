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
    case QStyle::PM_TabCloseIndicatorHeight:
    case QStyle::PM_TabCloseIndicatorWidth:
        return 1.2*pointToPixel(pSettings->environment().interfaceFontSize());
    default:
        return QProxyStyle::pixelMetric( metric, option, widget );
    }
}
