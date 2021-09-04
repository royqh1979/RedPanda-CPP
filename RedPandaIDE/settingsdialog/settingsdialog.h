#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QAbstractItemModel>
#include <QDialog>
#include <QStandardItemModel>

class SettingsWidget;
namespace Ui {
class SettingsDialog;
}

class CompilerSetOptionWidget;
class CompilerAutolinkWidget;
class EditorGeneralWidget;
class EditorFontWidget;
class EditorClipboardWidget;
class EditorSymbolCompletionWidget;
class EditorColorSchemeWidget;
class EditorSyntaxCheckWidget;
class EditorCodeCompletionWidget;
class EditorAutoSaveWidget;
class EditorMiscWidget;
class EnvironmentAppearenceWidget;
class ExecutorGeneralWidget;
class DebugGeneralWidget;
class FormatterGeneralWidget;
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

    CompilerSetOptionWidget *pCompilerSetOptionWidget;
    CompilerAutolinkWidget *pCompilerAutolinkWidget;
    EditorGeneralWidget *pEditorGeneralWidget;
    EditorFontWidget *pEditorFontWidget;
    EditorClipboardWidget *pEditorClipboardWidget;
    EditorColorSchemeWidget *pEditorColorSchemeWidget;
    EnvironmentAppearenceWidget *pEnvironmentAppearenceWidget;
    EditorSymbolCompletionWidget *pEditorSymbolCompletionWidget;
    EditorCodeCompletionWidget *pEditorCodeCompletionWidget;
    EditorSyntaxCheckWidget *pEditorSyntaxCheckWidget;
    EditorAutoSaveWidget *pEditorAutoSaveWidget;
    EditorMiscWidget *pEditorMiscWidget;
    ExecutorGeneralWidget  *pExecutorGeneralWidget;
    DebugGeneralWidget *pDebugGeneralWidget;
    FormatterGeneralWidget  *pFormatterGeneralWidget;
};

#endif // SETTINGSDIALOG_H
