#ifndef PROJECTCOMPILEPARAMATERSWIDGET_H
#define PROJECTCOMPILEPARAMATERSWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectCompileParamatersWidget;
}

class ProjectCompileParamatersWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectCompileParamatersWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectCompileParamatersWidget();

private:
    Ui::ProjectCompileParamatersWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // PROJECTCOMPILEPARAMATERSWIDGET_H
