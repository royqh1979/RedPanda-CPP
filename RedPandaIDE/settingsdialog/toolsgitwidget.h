#ifndef TOOLSGITWIDGET_H
#define TOOLSGITWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ToolsGitWidget;
}

class ToolsGitWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ToolsGitWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ToolsGitWidget();

private:
    Ui::ToolsGitWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
    void updateIcons(const QSize &size) override;
private slots:
    void on_btnBrowseGit_clicked();
    void on_btnTestGit_clicked();
};

#endif // TOOLSGITWIDGET_H
