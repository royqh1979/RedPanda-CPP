#ifndef EDITORGENERALWIDGET_H
#define EDITORGENERALWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class editorgeneralwidget;
}

class EditorGeneralWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorGeneralWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorGeneralWidget();

private:
    Ui::editorgeneralwidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_chkCaretUseTextColor_stateChanged(int arg1);
    void on_chkShowIndentLines_stateChanged(int arg1);
};

#endif // EDITORGENERALWIDGET_H
