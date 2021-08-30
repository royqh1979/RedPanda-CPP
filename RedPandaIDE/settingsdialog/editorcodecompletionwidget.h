#ifndef EDITORCODECOMPLETIONWIDGET_H
#define EDITORCODECOMPLETIONWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EditorCodeCompletionWidget;
}

class EditorCodeCompletionWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorCodeCompletionWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorCodeCompletionWidget();

private:
    Ui::EditorCodeCompletionWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // EDITORCODECOMPLETIONWIDGET_H
