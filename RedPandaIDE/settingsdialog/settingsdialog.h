#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QAbstractItemModel>
#include <QDialog>
#include <QStandardItemModel>

class SettingsWidget;
namespace Ui {
class SettingsDialog;
}

class PCompilerSet;
class CompilerSetOptionWidget;
class EditorGeneralWidget;
class EditorFontWidget;
class EditorClipboardWidget;
class EditorSymbolCompletionWidget;
class EditorColorSchemeWidget;
class EnvironmentAppearenceWidget;
class SettingsWidget;
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    enum {
        GetWidgetIndexRole = Qt::UserRole + 1
    };
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    void addWidget(SettingsWidget* pWidget);
private slots:
    void widget_settings_changed(bool value);
    void on_widgetsView_clicked(const QModelIndex &index);

    void on_btnCancle_pressed();

    void on_btnApply_pressed();

    void on_btnOk_pressed();
private:
    void saveCurrentPageSettings(bool confirm);
private:
    Ui::SettingsDialog *ui;
    QList<SettingsWidget*> mSettingWidgets;
    QStandardItemModel model;

    CompilerSetOptionWidget* pCompilerSetOptionWidget;
    EditorGeneralWidget* pEditorGeneralWidget;
    EditorFontWidget* pEditorFontWidget;
    EditorClipboardWidget *pEditorClipboardWidget;
    EditorColorSchemeWidget *pEditorColorSchemeWidget;
    EnvironmentAppearenceWidget* pEnvironmentAppearenceWidget;
    EditorSymbolCompletionWidget* pEditorSymbolCompletionWidget;
};

#endif // SETTINGSDIALOG_H
