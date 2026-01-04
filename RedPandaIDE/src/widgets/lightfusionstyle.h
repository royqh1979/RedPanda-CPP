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
