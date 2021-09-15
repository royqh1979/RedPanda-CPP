#ifndef PROJECTCOMPILERWIDGET_H
#define PROJECTCOMPILERWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectCompilerWidget;
}

class ProjectCompilerWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectCompilerWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectCompilerWidget();
private:
    void refreshOptions();
private:
    Ui::ProjectCompilerWidget *ui;
    QByteArray mOptions;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;

    // SettingsWidget interface
public:
    void init() override;
};

#endif // PROJECTCOMPILERWIDGET_H
