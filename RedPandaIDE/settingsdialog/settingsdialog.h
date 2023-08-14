/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QAbstractItemModel>
#include <QDialog>
#include <QStandardItemModel>
#include <memory>

class SettingsWidget;
namespace Ui {
class SettingsDialog;
}

class SettingsDialog;
using PSettingsDialog = std::shared_ptr<SettingsDialog>;
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

    void selectFirstWidget();

    static PSettingsDialog optionDialog();
    static PSettingsDialog projectOptionDialog();

    bool setCurrentWidget(const QString &widgetName, const QString &groupName);

    bool appShouldQuit() const;

private slots:
    void closeAndQuit();
    void showWidget(const QModelIndex &index);

    void widget_settings_changed(bool value);

    void onWidgetsViewCurrentChanged(const QModelIndex &index, const QModelIndex &previous);

    void on_btnCancel_pressed();

    void on_btnApply_pressed();

    void on_btnOk_pressed();
private:
    void saveCurrentPageSettings(bool confirm);
private:
    Ui::SettingsDialog *ui;
    QList<SettingsWidget*> mSettingWidgets;
    QStandardItemModel model;
    bool mAppShouldQuit;

//    CompilerSetOptionWidget *pCompilerSetOptionWidget;
//    CompilerAutolinkWidget *pCompilerAutolinkWidget;
//    EditorGeneralWidget *pEditorGeneralWidget;
//    EditorFontWidget *pEditorFontWidget;
//    EditorClipboardWidget *pEditorClipboardWidget;
//    EditorColorSchemeWidget *pEditorColorSchemeWidget;
//    EnvironmentAppearanceWidget *pEnvironmentAppearanceWidget;
//    EditorSymbolCompletionWidget *pEditorSymbolCompletionWidget;
//    EditorCodeCompletionWidget *pEditorCodeCompletionWidget;
//    EditorSyntaxCheckWidget *pEditorSyntaxCheckWidget;
//    EditorAutoSaveWidget *pEditorAutoSaveWidget;
//    EditorMiscWidget *pEditorMiscWidget;
//    ExecutorGeneralWidget  *pExecutorGeneralWidget;
//    DebugGeneralWidget *pDebugGeneralWidget;
//    FormatterGeneralWidget  *pFormatterGeneralWidget;


    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // SETTINGSDIALOG_H
