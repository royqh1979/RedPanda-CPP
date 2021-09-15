#ifndef PROJECTOUTPUTWIDGET_H
#define PROJECTOUTPUTWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectOutputWidget;
}

class ProjectOutputWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectOutputWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectOutputWidget();

private:
    Ui::ProjectOutputWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnOutputDir_triggered(QAction *arg1);
    void on_btnObjOutputDir_triggered(QAction *arg1);
    void on_btnCompileLog_triggered(QAction *arg1);
};

#endif // PROJECTOUTPUTWIDGET_H
