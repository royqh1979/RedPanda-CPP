#ifndef EDITORCLIPBOARDWIDGET_H
#define EDITORCLIPBOARDWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EditorClipboardWidget;
}

class EditorClipboardWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorClipboardWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorClipboardWidget();

private:
    Ui::EditorClipboardWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad();
    void doSave();
};

#endif // EDITORCLIPBOARDWIDGET_H
