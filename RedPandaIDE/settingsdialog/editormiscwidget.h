#ifndef EDITORMISCWIDGET_H
#define EDITORMISCWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EditorMiscWidget;
}

class EditorMiscWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorMiscWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorMiscWidget();

private:
    Ui::EditorMiscWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // EDITORMISCWIDGET_H
