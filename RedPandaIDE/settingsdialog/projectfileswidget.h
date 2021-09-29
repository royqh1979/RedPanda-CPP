#ifndef PROJECTFILESWIDGET_H
#define PROJECTFILESWIDGET_H

#include <QWidget>
#include "../project.h"
#include "settingswidget.h"

namespace Ui {
class ProjectFilesWidget;
}

class ProjectFilesWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectFilesWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectFilesWidget();

private:
    Ui::ProjectFilesWidget *ui;
    QList<PProjectUnit> mUnits;


    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private:
    PProjectUnit currentUnit();
    void copyUnits();
    void disableFileOptions();
private slots:
    void on_treeProject_doubleClicked(const QModelIndex &index);
    void on_spinPriority_valueChanged(int arg1);
    void on_chkCompile_stateChanged(int arg1);
    void on_chkLink_stateChanged(int arg1);
    void on_chkCompileAsCPP_stateChanged(int arg1);
    void on_chkOverrideBuildCommand_stateChanged(int arg1);
    void on_txtBuildCommand_textChanged();
    void on_cbEncoding_currentTextChanged(const QString &arg1);
    void on_treeProject_clicked(const QModelIndex &index);

    // SettingsWidget interface
    void on_cbEncodingDetail_currentTextChanged(const QString &arg1);

public:
    void init() override;
};

#endif // PROJECTFILESWIDGET_H
