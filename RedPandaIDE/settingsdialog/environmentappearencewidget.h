#ifndef ENVIRONMENTAPPEARENCEWIDGET_H
#define ENVIRONMENTAPPEARENCEWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EnvironmentAppearenceWidget;
}

class EnvironmentAppearenceWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentAppearenceWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentAppearenceWidget();

private:
    Ui::EnvironmentAppearenceWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // ENVIRONMENTAPPEARENCEWIDGET_H
