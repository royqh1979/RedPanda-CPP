#ifndef EDITORSYMBOLCOMPLETIONWIDGET_H
#define EDITORSYMBOLCOMPLETIONWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EditorSymbolCompletionWidget;
}

class EditorSymbolCompletionWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorSymbolCompletionWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorSymbolCompletionWidget();

private:
    Ui::EditorSymbolCompletionWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad();
    void doSave();
};

#endif // EDITORSYMBOLCOMPLETIONWIDGET_H
