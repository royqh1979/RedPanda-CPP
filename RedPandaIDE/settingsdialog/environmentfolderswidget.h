#ifndef ENVIRONMENTFOLDERSWIDGET_H
#define ENVIRONMENTFOLDERSWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EnvironmentFoldersWidget;
}

class EnvironmentFoldersWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentFoldersWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentFoldersWidget();
signals:
    void shouldQuitApp();
private:
    Ui::EnvironmentFoldersWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnOpenConfigFolderInBrowser_clicked();
    void on_btnResetDefault_clicked();
};

#endif // ENVIRONMENTFOLDERSWIDGET_H
