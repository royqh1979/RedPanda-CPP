#ifndef DEBUGGENERALWIDGET_H
#define DEBUGGENERALWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class DebugGeneralWidget;
}

class DebugGeneralWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit DebugGeneralWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~DebugGeneralWidget();

private:
    Ui::DebugGeneralWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_chkOnlyMono_stateChanged(int arg1);
};

#endif // DEBUGGENERALWIDGET_H
