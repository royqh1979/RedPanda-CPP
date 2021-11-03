#include "coloredit.h"

#include <QColorDialog>
#include <QPainter>
#include <QDebug>
#include <QApplication>

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

void ColorEdit::enterEvent(QEvent *)
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
