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
    void onLoaded() override;
private:
    PProjectUnit currentUnit();
    void copyUnits();
    void disableFileOptions();
    void loadUnitEncoding(PProjectUnit unit);
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
