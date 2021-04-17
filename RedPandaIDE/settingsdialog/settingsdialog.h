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
class PCompilerSet;
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

private:
    Ui::SettingsDialog *ui;
    QList<SettingsWidget*> mSettingWidgets;
    QStandardItemModel model;

    CompilerSetOptionWidget* pCompilerSetOptionWidget;
};

#endif // SETTINGSDIALOG_H
