#ifndef PROJECTFILESWIDGET_H
#define PROJECTFILESWIDGET_H

#include <QWidget>
#include "settings.h"

namespace Ui {
class ProjectFilesWidget;
}

class ProjectFilesWidget : public Settings
{
    Q_OBJECT

public:
    explicit ProjectFilesWidget(QWidget *parent = nullptr);
    ~ProjectFilesWidget();

private:
    Ui::ProjectFilesWidget *ui;
};

#endif // PROJECTFILESWIDGET_H
