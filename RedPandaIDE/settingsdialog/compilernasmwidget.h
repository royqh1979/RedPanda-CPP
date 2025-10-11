#ifndef COMPILERNASMWIDGET_H
#define COMPILERNASMWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class CompilerNASMWidget;
}

class CompilerNASMWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit CompilerNASMWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~CompilerNASMWidget();

private:
    Ui::CompilerNASMWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
    void updateIcons(const QSize &size) override;
private slots:
    void on_btnBrowserNASM_clicked();
    void on_btnTestNASM_clicked();
};

#endif // COMPILERNASMWIDGET_H
