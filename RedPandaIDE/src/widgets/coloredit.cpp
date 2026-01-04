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
#include "coloredit.h"

#include <QColorDialog>
#include <QPainter>
#include <QDebug>
#include <QApplication>

ColorEdit::ColorEdit(QWidget *parent):QFrame(parent)
{
    setFrameStyle(QFrame::Panel);
    setLineWidth(1);
    mColor = Qt::black;
}

QColor ColorEdit::color()
{
    return mColor;
}

void ColorEdit::setColor(const QColor &value)
{
    if (mColor!=value) {
        mColor=value;
        emit colorChanged(value);
        resize(sizeHint());
        repaint();
    }
}

QColor ColorEdit::contrast()
{
    int crBg;
    if (!mColor.isValid())
        crBg = palette().color(QPalette::Base).rgb() & 0xFFFFFF;
    else
        crBg = mColor.rgb() & 0xFFFFFF;
    int TOLERANCE = 30;
    int result;

    if (
        abs(((crBg ) & 0xFF) - 0x80) <= TOLERANCE &&
        abs(((crBg >> 8) & 0xFF) - 0x80) <= TOLERANCE &&
        abs(((crBg >> 16) & 0xFF) - 0x80) <= TOLERANCE
    )
        result = (0x7F7F7F + crBg) & 0xFFFFFF;
    else
        result = crBg ^ 0xFFFFFF;
    return QColor(result);
}

QSize ColorEdit::sizeHint() const
{
    QRect rect;
    if (mColor.isValid() )
        rect = fontMetrics().boundingRect(mColor.name(QColor::HexArgb));
    else
        rect = fontMetrics().boundingRect(tr("NONE"));
    return QSize{rect.width()+ 10,
                rect.height()+ 6 };
}

void ColorEdit::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QRect rect = QRect(lineWidth(),lineWidth(),width()-2*lineWidth(),height()-2*lineWidth());
    if (mColor.isValid() ) {
        //painter.fillRect(rect,mColor);
        if (isEnabled()) {
            painter.setPen(contrast());
            painter.setBrush(mColor);
        } else {
            painter.setBrush(palette().color(QPalette::Disabled,QPalette::Text));
            painter.setBrush(palette().color(QPalette::Disabled,QPalette::Base));
        }
        painter.drawRect(rect);
        painter.drawText(rect,Qt::AlignCenter, mColor.name(QColor::HexArgb));
    } else {
        //painter.fillRect(rect,palette().color(QPalette::Base));
        if (isEnabled()) {
            painter.setBrush(palette().color(QPalette::Text));
            painter.setBrush(palette().color(QPalette::Base));
        } else {
            painter.setBrush(palette().color(QPalette::Disabled,QPalette::Text));
            painter.setBrush(palette().color(QPalette::Disabled,QPalette::Base));
        }
        painter.setPen(contrast());
        painter.setBrush(palette().color(QPalette::Base));
        painter.drawRect(rect);
        painter.drawText(rect,Qt::AlignCenter, tr("NONE"));
    }
}

void ColorEdit::mouseReleaseEvent(QMouseEvent *)
{
    QColor c = QColorDialog::getColor(mColor,nullptr,tr("Color"),
                                      QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    if (c.isValid()) {
        setColor(c);
    }
}

#if QT_VERSION_MAJOR >= 6
void ColorEdit::enterEvent(QEnterEvent *)
#else
void ColorEdit::enterEvent(QEvent *)
#endif
{
    setCursor(Qt::PointingHandCursor);
}

void ColorEdit::leaveEvent(QEvent *)
{
    setCursor(Qt::ArrowCursor);
}

QSize ColorEdit::minimumSizeHint() const
{
    return sizeHint();
}
