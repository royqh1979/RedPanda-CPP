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
    void drawControl(ControlElement ce, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget) const override;
    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;
};

#endif // DARKFUSIONSTYLE_H
