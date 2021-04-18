#ifndef COMPILERSETOPTIONWIDGET_H
#define COMPILERSETOPTIONWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class CompilerSetOptionWidget;
}

class CompilerSetDirectoriesWidget;
class CompilerSetOptionWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit CompilerSetOptionWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~CompilerSetOptionWidget();

private:
    Ui::CompilerSetOptionWidget *ui;
    CompilerSetDirectoriesWidget* mBinDirWidget;
    CompilerSetDirectoriesWidget* mCIncludeDirWidget;
    CompilerSetDirectoriesWidget* mCppIncludeDirWidget;
    CompilerSetDirectoriesWidget* mLibDirWidget;


    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private:
    void reloadCurrentCompilerSet();
    void saveCurrentCompilerSet();

private slots:
    void on_cbCompilerSet_currentIndexChanged(int index);
    void on_btnFindCompilers_pressed();
    void on_btnAddBlankCompilerSet_pressed();
    void on_btnAddCompilerSetByFolder_pressed();
    void on_btnRenameCompilerSet_pressed();
    void on_btnRemoveCompilerSet_pressed();
};

#endif // COMPILERSETOPTIONWIDGET_H
