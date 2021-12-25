#ifndef ENVIRONMENTPROGRAMSWIDGET_H
#define ENVIRONMENTPROGRAMSWIDGET_H

#include "settingswidget.h"

namespace Ui {
class EnvironmentProgramsWidget;
}

class EnvironmentProgramsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentProgramsWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentProgramsWidget();

private:
    Ui::EnvironmentProgramsWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad();
    void doSave();
    void updateIcons(const QSize &size);
private slots:
    void on_btnChooseTerminal_clicked();
};

#endif // ENVIRONMENTPROGRAMSWIDGET_H
