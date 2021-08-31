#ifndef FORMATTERGENERALWIDGET_H
#define FORMATTERGENERALWIDGET_H

#include <QWidget>

namespace Ui {
class FormatterGeneralWidget;
}

class FormatterGeneralWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FormatterGeneralWidget(QWidget *parent = nullptr);
    ~FormatterGeneralWidget();

private:
    Ui::FormatterGeneralWidget *ui;
};

#endif // FORMATTERGENERALWIDGET_H
