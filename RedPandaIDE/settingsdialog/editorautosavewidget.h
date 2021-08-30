#ifndef EDITORAUTOSAVEWIDGET_H
#define EDITORAUTOSAVEWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EditorAutoSaveWidget;
}

class EditorAutoSaveWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorAutoSaveWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorAutoSaveWidget();

private:
    void onAutoSaveStrategyChanged();
private:
    Ui::EditorAutoSaveWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_rbOverwrite_toggled(bool checked);
    void on_rbAppendUNIXTimestamp_toggled(bool checked);
    void on_rbAppendFormattedTimestamp_toggled(bool checked);
};

#endif // EDITORAUTOSAVEWIDGET_H
