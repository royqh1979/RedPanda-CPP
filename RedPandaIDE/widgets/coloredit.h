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
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    QSize minimumSizeHint() const override;
};

#endif // COLOREDIT_H
