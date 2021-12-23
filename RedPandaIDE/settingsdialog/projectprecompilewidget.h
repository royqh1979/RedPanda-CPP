#ifndef PROJECTPRECOMPILEWIDGET_H
#define PROJECTPRECOMPILEWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectPreCompileWidget;
}

class ProjectPreCompileWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectPreCompileWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectPreCompileWidget();

private:
    Ui::ProjectPreCompileWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnBrowse_clicked();

    // SettingsWidget interface
protected:
    void updateIcons(const QSize &size) override;
};

#endif // PROJECTPRECOMPILEWIDGET_H
