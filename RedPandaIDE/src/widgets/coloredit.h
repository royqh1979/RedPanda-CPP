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
#ifndef COLOREDIT_H
#define COLOREDIT_H

#include <QFrame>

class ColorEdit : public QFrame
{
    Q_OBJECT
public:
    ColorEdit(QWidget* parent = nullptr);
    QColor color();
    void setColor(const QColor& value);
signals:
    void colorChanged(const QColor& value);
private:
    QColor mColor;

    QColor contrast();

public:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
#if QT_VERSION_MAJOR >= 6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    QSize minimumSizeHint() const override;
};

#endif // COLOREDIT_H
