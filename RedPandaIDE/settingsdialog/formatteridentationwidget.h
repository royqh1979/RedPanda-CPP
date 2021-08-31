#ifndef FORMATTERIDENTATIONWIDGET_H
#define FORMATTERIDENTATIONWIDGET_H

#include <QWidget>

namespace Ui {
class FormatterIdentationWidget;
}

class FormatterIdentationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FormatterIdentationWidget(QWidget *parent = nullptr);
    ~FormatterIdentationWidget();

private:
    Ui::FormatterIdentationWidget *ui;
};

#endif // FORMATTERIDENTATIONWIDGET_H
