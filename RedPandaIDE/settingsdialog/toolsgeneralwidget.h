#ifndef TOOLSGENERALWIDGET_H
#define TOOLSGENERALWIDGET_H

#include <QWidget>

namespace Ui {
class ToolsGeneralWidget;
}

class ToolsGeneralWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ToolsGeneralWidget(QWidget *parent = nullptr);
    ~ToolsGeneralWidget();

private:
    Ui::ToolsGeneralWidget *ui;
};

#endif // TOOLSGENERALWIDGET_H
