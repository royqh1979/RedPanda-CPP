#ifndef LABELWITHMENU_H
#define LABELWITHMENU_H

#include <QLabel>

class LabelWithMenu : public QLabel
{
    Q_OBJECT
public:
    explicit LabelWithMenu(QWidget* parent = nullptr);

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // LABELWITHMENU_H
