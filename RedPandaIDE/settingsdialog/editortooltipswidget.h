#ifndef EDITORTOOLTIPSWIDGET_H
#define EDITORTOOLTIPSWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EditorTooltipsWidget;
}

class EditorTooltipsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorTooltipsWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorTooltipsWidget();

private:
    Ui::EditorTooltipsWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // EDITORTOOLTIPSWIDGET_H
