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
public:
    void init() override;
protected:
    void doLoad() override;
    void doSave() override;
    void updateIcons(const QSize& size) override;

private:
    void reloadCurrentCompilerSet();
    void saveCurrentCompilerSet();
    QString getBinDir();

private slots:
    void on_cbCompilerSet_currentIndexChanged(int index);
    void on_btnFindCompilers_clicked();
    void on_btnAddBlankCompilerSet_clicked();
    void on_btnAddCompilerSetByFolder_clicked();
    void on_btnRenameCompilerSet_clicked();
    void on_btnRemoveCompilerSet_clicked();

    void on_cbEncoding_currentTextChanged(const QString &arg1);
    void on_cbEncodingDetails_currentTextChanged(const QString &arg1);
    void on_btnChooseCCompiler_clicked();
    void on_btnChooseCppCompiler_clicked();
    void on_btnChooseMake_clicked();
    void on_btnChooseGDB_clicked();
    void on_btnChooseGDBServer_clicked();
    void on_btnChooseResourceCompiler_clicked();
    void on_btnAddCompilerSetByFile_clicked();
};

#endif // COMPILERSETOPTIONWIDGET_H
