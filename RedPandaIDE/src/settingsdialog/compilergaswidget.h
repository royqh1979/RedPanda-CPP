#ifndef COMPILERGASWIDGET_H
#define COMPILERGASWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class CompilerGASWidget;
}

class CompilerGASWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit CompilerGASWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~CompilerGASWidget();

private:
    Ui::CompilerGASWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
    void updateIcons(const QSize &size) override;
};

#endif // COMPILERGASWIDGET_H
