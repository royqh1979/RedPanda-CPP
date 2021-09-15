#ifndef PROJECTCOMPILERWIDGET_H
#define PROJECTCOMPILERWIDGET_H

#include <QWidget>

namespace Ui {
class ProjectCompilerWidget;
}

class ProjectCompilerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectCompilerWidget(QWidget *parent = nullptr);
    ~ProjectCompilerWidget();

private:
    Ui::ProjectCompilerWidget *ui;
};

#endif // PROJECTCOMPILERWIDGET_H
