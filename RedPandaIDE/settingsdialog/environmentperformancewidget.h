#ifndef ENVIRONMENTPERFORMANCEWIDGET_H
#define ENVIRONMENTPERFORMANCEWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EnvironmentPerformanceWidget;
}

class EnvironmentPerformanceWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentPerformanceWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentPerformanceWidget();

    void doLoad() override;
    void doSave() override;

private:
    Ui::EnvironmentPerformanceWidget *ui;
};

#endif // ENVIRONMENTPERFORMANCEWIDGET_H
