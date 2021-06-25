#ifndef EDITORSYNTAXCHECKWIDGET_H
#define EDITORSYNTAXCHECKWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EditorSyntaxCheckWidget;
}

class EditorSyntaxCheckWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorSyntaxCheckWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorSyntaxCheckWidget();

private:
    Ui::EditorSyntaxCheckWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad();
    void doSave();
};

#endif // EDITORSYNTAXCHECKWIDGET_H
