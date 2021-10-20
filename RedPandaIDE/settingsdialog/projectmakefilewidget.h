#ifndef PROJECTMAKEFILEWIDGET_H
#define PROJECTMAKEFILEWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectMakefileWidget;
}

class CompilerSetDirectoriesWidget;
class ProjectMakefileWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectMakefileWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectMakefileWidget();

private:
    Ui::ProjectMakefileWidget *ui;
    CompilerSetDirectoriesWidget * mIncludesDirWidget;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnBrowse_clicked();
    void on_pushButton_clicked();
};

#endif // PROJECTMAKEFILEWIDGET_H
