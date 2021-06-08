#ifndef EDITORFONTWIDGET_H
#define EDITORFONTWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EditorFontWidget;
}

class EditorFontWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorFontWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorFontWidget();


private slots:
    void on_chkOnlyMonospacedFonts_stateChanged(int arg1);
    void on_chkGutterOnlyMonospacedFonts_stateChanged(int arg1);

private:
    Ui::EditorFontWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // EDITORFONTWIDGET_H
