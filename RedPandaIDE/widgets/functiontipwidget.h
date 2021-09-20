#ifndef FUNCTIONTIPWIDGET_H
#define FUNCTIONTIPWIDGET_H

#include <QLabel>
#include <QWidget>

class Editor;
class FunctionTipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FunctionTipWidget(QWidget *parent = nullptr);

signals:
private:
    QLabel* mLabel;

};

#endif // FUNCTIONTIPWIDGET_H
