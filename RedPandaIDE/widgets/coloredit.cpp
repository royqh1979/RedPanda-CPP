#include "coloredit.h"

#include <QColorDialog>
#include <QPainter>
#include <QDebug>

ColorEdit::ColorEdit(QWidget *parent):QFrame(parent)
{
    setFrameStyle(QFrame::Panel);
    setLineWidth(1);
    mColor = QColorConstants::Black;
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
        update();
    }
}

QColor ColorEdit::contrast()
{
    int crBg = mColor.rgb() & 0xFFFFFF;
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
    QRect rect = fontMetrics().boundingRect(mColor.name());
    return QSize{rect.width()+ 10,
                rect.height()+ 6 };
}

void ColorEdit::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QRect rect = QRect(lineWidth(),lineWidth(),width()-2*lineWidth(),height()-2*lineWidth());
    painter.fillRect(rect,mColor);
    painter.setPen(contrast());
    painter.drawText(rect,Qt::AlignCenter, mColor.name());
}

void ColorEdit::mouseReleaseEvent(QMouseEvent *event)
{
    QColor c = QColorDialog::getColor();
    if (c.isValid()) {
        setColor(c);
    }
}

void ColorEdit::enterEvent(QEvent *event)
{
    setCursor(Qt::PointingHandCursor);
}

void ColorEdit::leaveEvent(QEvent *event)
{
    setCursor(Qt::ArrowCursor);
}

QSize ColorEdit::minimumSizeHint() const
{
    QRect rect = fontMetrics().boundingRect(mColor.name());
    return QSize{rect.width(),
                rect.height()};
}
