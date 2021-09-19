#ifndef EXECUTORGENERALWIDGET_H
#define EXECUTORGENERALWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ExecutorGeneralWidget;
}

class ExecutorGeneralWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ExecutorGeneralWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ExecutorGeneralWidget();

private:
    Ui::ExecutorGeneralWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnBrowse_triggered(QAction *arg1);
};

#endif // EXECUTORGENERALWIDGET_H
