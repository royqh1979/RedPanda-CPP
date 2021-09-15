#ifndef PROJECTCOMPILEPARAMATERSWIDGET_H
#define PROJECTCOMPILEPARAMATERSWIDGET_H

#include <QWidget>

namespace Ui {
class ProjectCompileParamatersWidget;
}

class ProjectCompileParamatersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectCompileParamatersWidget(QWidget *parent = nullptr);
    ~ProjectCompileParamatersWidget();

private:
    Ui::ProjectCompileParamatersWidget *ui;
};

#endif // PROJECTCOMPILEPARAMATERSWIDGET_H
