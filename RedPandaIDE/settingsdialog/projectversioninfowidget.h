#ifndef PROJECTVERSIONINFOWIDGET_H
#define PROJECTVERSIONINFOWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectVersionInfoWidget;
}

class ProjectVersionInfoWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectVersionInfoWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectVersionInfoWidget();

private:
    Ui::ProjectVersionInfoWidget *ui;


    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // PROJECTVERSIONINFOWIDGET_H
