#ifndef PROJECTDLLHOSTWIDGET_H
#define PROJECTDLLHOSTWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectDLLHostWidget;
}

class ProjectDLLHostWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectDLLHostWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectDLLHostWidget();

private:
    Ui::ProjectDLLHostWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnBrowse_clicked();
};

#endif // PROJECTDLLHOSTWIDGET_H
