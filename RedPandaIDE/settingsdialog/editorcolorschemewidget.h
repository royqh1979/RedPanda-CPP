#ifndef EDITORCOLORSCHEMEWIDGET_H
#define EDITORCOLORSCHEMEWIDGET_H

#include "settingswidget.h"

namespace Ui {
class EditorColorSchemeWidget;
}

class EditorColorSchemeWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorColorSchemeWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorColorSchemeWidget();

private:
    Ui::EditorColorSchemeWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // EDITORCOLORSCHEMEWIDGET_H
